#include "bot.h"

#include <algorithm>
#include <cmath>

namespace plump {

namespace {

const int HardSamples = 24;
const int HardBidSamples = 12;

// How strongly a card plays to the trick: trumps above the suit led, and
// anything else nothing.
int power(const Card &card, int led, int trumps)
{
    if (card.suit == trumps)
        return 100 + card.high();
    return card.suit == led ? card.high() : 0;
}

// The power of the best card in the trick so far.
int bestSoFar(const Game &game)
{
    int best = 0;
    for (const Play &p : game.trick())
        best = std::max(best, power(p.card, game.ledSuit(), game.trumps()));
    return best;
}

// The lowest card to throw away: no trumps, if there is anything else.
Card lowestDiscard(const Cards &cards, int trumps)
{
    const Card *best = nullptr;
    for (const Card &c : cards) {
        if (!best || (c.suit == trumps) < (best->suit == trumps)
            || ((c.suit == trumps) == (best->suit == trumps) && c.high() < best->high()))
            best = &c;
    }
    return *best;
}

// The highest card to throw away, trumps last.
Card highestDiscard(const Cards &cards, int trumps)
{
    const Card *best = nullptr;
    for (const Card &c : cards) {
        if (!best || (c.suit == trumps) < (best->suit == trumps)
            || ((c.suit == trumps) == (best->suit == trumps) && c.high() > best->high()))
            best = &c;
    }
    return *best;
}

} // namespace

Bot::Bot(Difficulty difficulty, unsigned seed)
    : m_difficulty(difficulty)
    , m_rng(seed)
{
}

double Bot::expectedTricks(const Game &game, int player)
{
    const Cards &hand = game.hand(player);
    const int trumps = game.trumps();
    const int nTrumps = countSuit(hand, trumps);
    // With more players a side king is less likely to stand up.
    const double crowd = 4.0 / game.players();
    double tricks = 0;
    for (const Card &c : hand) {
        const int n = countSuit(hand, c.suit);
        if (c.suit == trumps) {
            if (c.high() == 14)
                tricks += 1;
            else if (c.high() == 13)
                tricks += nTrumps >= 2 ? 0.85 : 0.6;
            else if (c.high() == 12)
                tricks += nTrumps >= 3 ? 0.65 : 0.35;
            else
                tricks += nTrumps >= 4 ? 0.5 : 0.25;
        } else if (c.high() == 14) {
            tricks += n <= 4 ? 0.85 : 0.6;
        } else if (c.high() == 13 && n >= 2 && n <= 4) {
            tricks += 0.45 * crowd;
        }
    }
    // A short side suit lets the small trumps take tricks.
    int spare = nTrumps;
    for (int s = 0; s < 4 && spare > 0; ++s) {
        if (s == trumps)
            continue;
        const int n = countSuit(hand, s);
        if (n <= 1 && game.handSize() >= 4) {
            tricks += 0.35 * (2 - n);
            --spare;
        }
    }
    return std::min<double>(tricks, game.handSize());
}

int Bot::nearestBid(const Game &game, int player, double expected)
{
    int best = -1;
    for (int b = 0; b <= game.handSize(); ++b)
        if (game.canBid(player, b) && (best < 0 || std::abs(b - expected) < std::abs(best - expected)))
            best = b;
    return best;
}

int Bot::chooseBid(const Game &game, int player)
{
    const double expected = expectedTricks(game, player);
    switch (m_difficulty) {
    case Difficulty::Easy:
        return nearestBid(game, player, expected + std::uniform_int_distribution<int>(-1, 1)(m_rng));
    case Difficulty::Medium:
        return nearestBid(game, player, expected);
    case Difficulty::Hard:
        break;
    }
    return bidSimulated(game, player);
}

Card Bot::choosePlay(const Game &game, int player)
{
    const Cards legal = game.legalCards(player);
    if (legal.size() == 1)
        return legal.front();
    switch (m_difficulty) {
    case Difficulty::Easy:
        return legal[std::uniform_int_distribution<size_t>(0, legal.size() - 1)(m_rng)];
    case Difficulty::Medium:
        return cautiousPlay(game, player);
    case Difficulty::Hard:
        break;
    }
    return playSimulated(game, player, legal);
}

Card Bot::cautiousPlay(const Game &game, int player)
{
    const Cards legal = game.legalCards(player);
    const int trumps = game.trumps();
    const int need = game.bidOf(player) - game.tricksTaken(player);
    const bool want = need > 0;

    if (game.trick().empty()) {
        if (!want)
            return lowestDiscard(legal, trumps);
        // A side ace first, then the highest trump, then the highest card.
        for (const Card &c : legal)
            if (c.suit != trumps && c.high() == 14)
                return c;
        const Card *top = nullptr;
        for (const Card &c : legal)
            if (c.suit == trumps && (!top || c.high() > top->high()))
                top = &c;
        if (top && countSuit(legal, trumps) >= need)
            return *top;
        return highestDiscard(legal, trumps);
    }

    const int led = game.ledSuit();
    const int best = bestSoFar(game);
    const bool last = static_cast<int>(game.trick().size()) == game.players() - 1;
    Cards winners, losers;
    for (const Card &c : legal)
        (power(c, led, trumps) > best ? winners : losers).push_back(c);
    auto byPower = [led, trumps](const Card &a, const Card &b) { return power(a, led, trumps) < power(b, led, trumps); };

    if (want) {
        if (winners.empty())
            return lowestDiscard(legal, trumps);
        // Last to play, the cheapest winner; otherwise the strongest, to hold.
        return last ? *std::min_element(winners.begin(), winners.end(), byPower)
                    : *std::max_element(winners.begin(), winners.end(), byPower);
    }
    if (!losers.empty()) {
        // The highest card that still ducks, to be rid of it.
        const Card top = *std::max_element(losers.begin(), losers.end(), byPower);
        return power(top, led, trumps) > 0 ? top : highestDiscard(losers, trumps);
    }
    // Every card wins: if it must, with the highest; else the lowest, hoping
    // to be beaten.
    return last ? *std::max_element(winners.begin(), winners.end(), byPower)
                : *std::min_element(winners.begin(), winners.end(), byPower);
}

bool Bot::guessHands(const Game &game, int player, Cards hands[Game::MaxPlayers])
{
    const int n = game.players();
    bool voids[Game::MaxPlayers][4] = {};
    // Showing out of the suit led.
    for (size_t i = 0; i < game.played().size(); i += n) {
        const size_t end = std::min(game.played().size(), i + n);
        const int led = game.played()[i].card.suit;
        for (size_t j = i + 1; j < end; ++j)
            if (game.played()[j].card.suit != led)
                voids[game.played()[j].player][led] = true;
    }

    Cards unseen;
    for (const Card &c : fullDeck()) {
        if (contains(game.hand(player), c) || c == game.trumpCard())
            continue;
        bool played = false;
        for (const Play &p : game.played())
            if (p.card == c)
                played = true;
        if (!played)
            unseen.push_back(c);
    }

    for (int attempt = 0; attempt < 20; ++attempt) {
        std::shuffle(unseen.begin(), unseen.end(), m_rng);
        for (int p = 0; p < n; ++p)
            hands[p].clear();
        hands[player] = game.hand(player);
        // What is left over is the undealt part of the pack.
        Cards rest = unseen;
        bool ok = true;
        for (int p = 0; p < n && ok; ++p) {
            if (p == player)
                continue;
            const size_t size = game.hand(p).size();
            for (auto it = rest.begin(); it != rest.end() && hands[p].size() < size;) {
                if (!voids[p][it->suit]) {
                    hands[p].push_back(*it);
                    it = rest.erase(it);
                } else {
                    ++it;
                }
            }
            ok = hands[p].size() == size;
        }
        if (ok)
            return true;
    }
    return false;
}

bool Bot::playsOut(Game game, int player)
{
    while (game.phase() == Phase::Play) {
        if (game.trickComplete())
            game.collectTrick();
        else
            game.play(game.turn(), cautiousPlay(game, game.turn()));
    }
    return game.tricksTaken(player) == game.bidOf(player);
}

Card Bot::playSimulated(const Game &game, int player, const Cards &legal)
{
    std::vector<int> made(legal.size(), 0);
    Cards hands[Game::MaxPlayers];
    for (int s = 0; s < HardSamples; ++s) {
        if (!guessHands(game, player, hands))
            return cautiousPlay(game, player);
        Game world = game;
        for (int p = 0; p < game.players(); ++p)
            world.replaceHand(p, hands[p]);
        for (size_t i = 0; i < legal.size(); ++i) {
            Game g = world;
            g.play(player, legal[i]);
            made[i] += playsOut(g, player);
        }
    }
    const Card careful = cautiousPlay(game, player);
    size_t best = 0;
    for (size_t i = 0; i < legal.size(); ++i) {
        // On a tie, the careful card.
        if (made[i] > made[best] || (made[i] == made[best] && legal[i] == careful))
            best = i;
    }
    return legal[best];
}

int Bot::bidSimulated(const Game &game, int player)
{
    const int estimate = nearestBid(game, player, expectedTricks(game, player));
    // Bids near the estimate, each tried on the same guessed deals.
    const int low = std::max(0, estimate - 2);
    const int high = std::min(game.handSize(), estimate + 2);
    std::vector<int> made(high - low + 1, 0);
    Cards hands[Game::MaxPlayers];
    for (int s = 0; s < HardBidSamples; ++s) {
        if (!guessHands(game, player, hands))
            return estimate;
        Game world = game;
        for (int p = 0; p < game.players(); ++p)
            world.replaceHand(p, hands[p]);
        for (int b = low; b <= high; ++b) {
            if (!game.canBid(player, b))
                continue;
            Game g = world;
            g.bid(player, b);
            // The others still to bid go by their guessed hands.
            while (g.phase() == Phase::Bid) {
                const int p = g.turn();
                g.bid(p, nearestBid(g, p, expectedTricks(g, p)));
            }
            made[b - low] += playsOut(g, player);
        }
    }
    int best = estimate;
    for (int b = low; b <= high; ++b)
        if (made[b - low] > made[best - low])
            best = b;
    return best;
}

} // namespace plump
