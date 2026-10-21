#include "game.h"

#include <algorithm>

namespace plump {

Game::Game(unsigned seed)
    : m_rng(seed)
{
}

int Game::handSize(int deal)
{
    // 10, 9 ... 1, then 2 ... 10.
    return deal <= MaxHand ? MaxHand + 1 - deal : deal - MaxHand + 1;
}

void Game::newGame(int players)
{
    players = std::max(MinPlayers, std::min(MaxPlayers, players));
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    newGame(players, std::uniform_int_distribution<int>(0, players - 1)(m_rng), deck);
}

void Game::newGame(int players, int dealer, const Cards &deck)
{
    m_players = std::max(MinPlayers, std::min(MaxPlayers, players));
    for (int p = 0; p < MaxPlayers; ++p) {
        m_score[p] = 0;
        m_dealScore[p] = 0;
    }
    m_deal = 1;
    m_dealer = (dealer % m_players + m_players) % m_players;
    dealFrom(deck);
}

bool Game::nextDeal()
{
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    return nextDeal(deck);
}

bool Game::nextDeal(const Cards &deck)
{
    if (m_phase != Phase::Score)
        return false;
    ++m_deal;
    m_dealer = (m_dealer + 1) % m_players;
    dealFrom(deck);
    return true;
}

void Game::dealFrom(const Cards &deck)
{
    const int size = handSize();
    for (int p = 0; p < MaxPlayers; ++p) {
        m_hand[p].clear();
        m_bid[p] = -1;
        m_taken[p] = 0;
        m_won[p].clear();
        m_dealScore[p] = 0;
    }
    for (int i = 0; i < m_players * size; ++i)
        m_hand[(m_dealer + 1 + i) % m_players].push_back(deck[i]);
    for (Cards &hand : m_hand)
        sortAlternating(hand);
    // Five players take fifty cards, so one is always left to turn up.
    m_trumpCard = deck[m_players * size];
    m_trick.clear();
    m_played.clear();
    m_tricks = 0;
    m_phase = Phase::Bid;
    m_leader = (m_dealer + 1) % m_players;
    m_turn = m_leader;
}

int Game::totalBid() const
{
    int total = 0;
    for (int p = 0; p < m_players; ++p)
        total += std::max(0, m_bid[p]);
    return total;
}

int Game::forbiddenBid() const
{
    if (m_phase != Phase::Bid || m_turn != m_dealer)
        return -1;
    const int left = handSize() - totalBid();
    return left >= 0 ? left : -1;
}

bool Game::canBid(int player, int tricks) const
{
    return m_phase == Phase::Bid && player == m_turn && tricks >= 0 && tricks <= handSize()
           && tricks != forbiddenBid();
}

bool Game::bid(int player, int tricks)
{
    if (!canBid(player, tricks))
        return false;
    m_bid[player] = tricks;
    // The dealer bids last; then the player on their left leads.
    if (player == m_dealer) {
        m_phase = Phase::Play;
        m_turn = m_leader;
    } else {
        m_turn = (player + 1) % m_players;
    }
    return true;
}

bool Game::holds(int player, const Card &card) const
{
    return contains(m_hand[player], card);
}

bool Game::holdsSuit(int player, int suit) const
{
    for (const Card &c : m_hand[player])
        if (c.suit == suit)
            return true;
    return false;
}

bool Game::canPlayCard(int player, const Card &card) const
{
    if (m_phase != Phase::Play || m_turn != player || !holds(player, card))
        return false;
    // Follow suit if you can; otherwise anything, trumps or not.
    return m_trick.empty() || card.suit == ledSuit() || !holdsSuit(player, ledSuit());
}

Cards Game::legalCards(int player) const
{
    Cards out;
    for (const Card &c : m_hand[player])
        if (canPlayCard(player, c))
            out.push_back(c);
    return out;
}

bool Game::play(int player, const Card &card)
{
    if (!canPlayCard(player, card))
        return false;
    removeCard(m_hand[player], card);
    m_trick.push_back({player, card});
    m_played.push_back({player, card});
    m_turn = trickComplete() ? -1 : (player + 1) % m_players;
    return true;
}

int Game::trickWinner() const
{
    if (!trickComplete())
        return -1;
    const Play *best = &m_trick.front();
    for (const Play &p : m_trick) {
        if (p.card.suit == best->card.suit) {
            if (p.card.high() > best->card.high())
                best = &p;
        } else if (p.card.suit == trumps()) {
            best = &p;
        }
    }
    return best->player;
}

bool Game::collectTrick()
{
    if (m_phase != Phase::Play || !trickComplete())
        return false;
    const int winner = trickWinner();
    ++m_taken[winner];
    for (const Play &p : m_trick)
        m_won[winner].push_back(p.card);
    m_trick.clear();
    ++m_tricks;
    m_leader = winner;
    m_turn = winner;
    if (m_tricks == handSize())
        scoreDeal();
    return true;
}

void Game::scoreDeal()
{
    m_turn = -1;
    for (int p = 0; p < m_players; ++p) {
        m_dealScore[p] = m_taken[p] == m_bid[p] ? bidScore(m_bid[p]) : 0;
        m_score[p] += m_dealScore[p];
    }
    m_phase = m_deal == Deals ? Phase::GameOver : Phase::Score;
}

bool Game::isWinner(int player) const
{
    return m_phase == Phase::GameOver && player >= 0 && player < m_players
           && m_score[player] == *std::max_element(m_score, m_score + m_players);
}

} // namespace plump
