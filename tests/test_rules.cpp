// Rule tests for the plump engine. Plain C++ so they build anywhere:
//   make -C tests

#include "../src/core/bot.h"
#include "../src/core/game.h"
#include "cards/testing.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace plump;

namespace {

// A deck that deals each of `players` the cards given for them, for the
// first deal of ten, with the last player dealing so that player 0 gets the
// first card; then `trump` is turned up, and the rest of the pack follows.
Cards stackedDeck(int players, const std::vector<std::string> &given, const std::string &trump)
{
    std::vector<Cards> hands(players);
    Cards used = {parseCard(trump)};
    for (size_t p = 0; p < given.size(); ++p) {
        hands[p] = parseCards(given[p]);
        used.insert(used.end(), hands[p].begin(), hands[p].end());
    }
    Cards rest;
    for (const Card &c : fullDeck())
        if (std::find(used.begin(), used.end(), c) == used.end())
            rest.push_back(c);
    for (Cards &h : hands)
        while (h.size() < static_cast<size_t>(Game::MaxHand)) {
            h.push_back(rest.front());
            rest.erase(rest.begin());
        }
    Cards deck;
    for (int i = 0; i < Game::MaxHand; ++i)
        for (int p = 0; p < players; ++p)
            deck.push_back(hands[p][i]);
    deck.push_back(parseCard(trump));
    deck.insert(deck.end(), rest.begin(), rest.end());
    return deck;
}

bool play(Game &g, int player, const std::string &card)
{
    return g.play(player, parseCard(card));
}

// Everyone bids, from the player on the dealer's left.
void bidAll(Game &g, const std::vector<int> &bids)
{
    for (size_t i = 0; i < bids.size(); ++i) {
        const int p = g.turn();
        CHECK(g.bid(p, bids[p]));
    }
}

} // namespace

TEST(hand_sizes_go_down_and_up)
{
    CHECK_EQ(Game::handSize(1), 10);
    CHECK_EQ(Game::handSize(10), 1);
    CHECK_EQ(Game::handSize(11), 2);
    CHECK_EQ(Game::handSize(19), 10);
}

TEST(deal_starts_left_of_dealer_and_turns_up_trumps)
{
    Game g(1);
    const Cards deck = stackedDeck(4, {"AS"}, "7H");
    g.newGame(4, 3, deck);
    CHECK_EQ(g.players(), 4);
    CHECK_EQ(g.dealer(), 3);
    CHECK_EQ(g.hand(0).size(), size_t(10));
    CHECK(std::find(g.hand(0).begin(), g.hand(0).end(), parseCard("AS")) != g.hand(0).end());
    CHECK(g.trumpCard() == parseCard("7H"));
    CHECK_EQ(g.trumps(), int(Hearts));
    CHECK(g.phase() == Phase::Bid);
    CHECK_EQ(g.turn(), 0);
}

TEST(five_players_leave_a_card_to_turn_up)
{
    Game g(2);
    g.newGame(5);
    int cards = 0;
    for (int p = 0; p < 5; ++p)
        cards += static_cast<int>(g.hand(p).size());
    CHECK_EQ(cards, 50);
    CHECK(g.trumpCard().valid());
    for (int p = 0; p < 5; ++p)
        CHECK(std::find(g.hand(p).begin(), g.hand(p).end(), g.trumpCard()) == g.hand(p).end());
}

TEST(dealer_bids_last_then_left_of_dealer_leads)
{
    Game g(1);
    g.newGame(3, 1, stackedDeck(3, {}, "2C"));
    CHECK_EQ(g.turn(), 2);
    CHECK(!g.bid(0, 1));
    CHECK(!g.bid(2, 11));
    CHECK(g.bid(2, 3));
    CHECK(g.bid(0, 0));
    CHECK(g.phase() == Phase::Bid);
    CHECK(g.bid(1, 10));
    CHECK(g.phase() == Phase::Play);
    CHECK_EQ(g.turn(), 2);
    CHECK_EQ(g.totalBid(), 13);
}

TEST(dealer_may_not_make_the_bids_add_up)
{
    Game g(1);
    g.newGame(3, 1, stackedDeck(3, {}, "2C"));
    CHECK_EQ(g.forbiddenBid(), -1);
    CHECK(g.bid(2, 4));
    CHECK(g.bid(0, 3));
    // Ten tricks, seven bid: the dealer may say anything but three.
    CHECK_EQ(g.forbiddenBid(), 3);
    CHECK(!g.canBid(1, 3));
    CHECK(g.canBid(1, 2));
    CHECK(g.canBid(1, 4));
    Bot bot(Difficulty::Hard, 1);
    CHECK(bot.chooseBid(g, 1) != 3);
    CHECK(Bot::nearestBid(g, 1, 3.0) != 3);
    CHECK(g.bid(1, 0));
    CHECK_EQ(g.forbiddenBid(), -1);
}

TEST(overbid_leaves_the_dealer_free)
{
    Game g(1);
    g.newGame(3, 1, stackedDeck(3, {}, "2C"));
    CHECK(g.bid(2, 6));
    CHECK(g.bid(0, 5));
    CHECK_EQ(g.forbiddenBid(), -1);
    CHECK(g.canBid(1, 0));
}

TEST(must_follow_suit_but_need_not_trump)
{
    Game g(1);
    g.newGame(3, 2, stackedDeck(3, {"AS KS 2H 3H 4H 5H 6H 7H 8H 9H", "QS 2D 3D 4D 5D 6D 7D 8D 9D 10D", "JC"}, "2C"));
    bidAll(g, {1, 1, 1});
    CHECK(play(g, 0, "AS"));
    // Player 1 holds a spade and must play it.
    CHECK(!g.canPlayCard(1, parseCard("2D")));
    CHECK(play(g, 1, "QS"));
    // Player 2 has no spades in this hand: any card, trump or not.
    const Cards &h2 = g.hand(2);
    const bool spades = std::any_of(h2.begin(), h2.end(), [](const Card &c) { return c.suit == Spades; });
    if (!spades) {
        for (const Card &c : h2)
            CHECK(g.canPlayCard(2, c));
    }
}

TEST(trump_takes_the_trick)
{
    Game g(1);
    g.newGame(3, 2, stackedDeck(3, {"AS 2H 3H 4H 5H 6H 7H 8H 9H 10H", "2C 3C 4C 5C 6C 7C 8C 9C 10C JC",
                                    "KS 2D 3D 4D 5D 6D 7D 8D 9D 10D"}, "QC"));
    bidAll(g, {0, 0, 0});
    CHECK(play(g, 0, "AS"));
    CHECK(play(g, 1, "2C"));
    CHECK(play(g, 2, "KS"));
    CHECK_EQ(g.trickWinner(), 1);
    CHECK(g.collectTrick());
    CHECK_EQ(g.tricksTaken(1), 1);
    CHECK_EQ(g.turn(), 1);
}

TEST(highest_of_suit_led_wins_without_trumps)
{
    Game g(1);
    g.newGame(3, 2, stackedDeck(3, {"9S 2H 3H 4H 5H 6H 7H 8H 9H 10H", "JS 2C 3C 4C 5C 6C 7C 8C 9C 10C",
                                    "AD 2D 3D 4D 5D 6D 7D 8D 9D 10D"}, "QC"));
    bidAll(g, {0, 0, 0});
    CHECK(play(g, 0, "9S"));
    CHECK(play(g, 1, "JS"));
    CHECK(play(g, 2, "AD"));
    CHECK_EQ(g.trickWinner(), 1);
}

TEST(exact_bids_score)
{
    CHECK_EQ(Game::bidScore(0), 10);
    CHECK_EQ(Game::bidScore(1), 10);
    CHECK_EQ(Game::bidScore(3), 30);
    // Play a whole deal of the last player's careful choices and count it.
    Game g(7);
    g.newGame(4);
    while (g.phase() == Phase::Bid)
        g.bid(g.turn(), 1);
    while (g.phase() == Phase::Play) {
        if (g.trickComplete())
            g.collectTrick();
        else
            CHECK(g.play(g.turn(), Bot::cautiousPlay(g, g.turn())));
    }
    CHECK(g.phase() == Phase::Score);
    int tricks = 0;
    for (int p = 0; p < 4; ++p) {
        tricks += g.tricksTaken(p);
        CHECK_EQ(g.dealScore(p), g.tricksTaken(p) == 1 ? 10 : 0);
        CHECK_EQ(g.score(p), g.dealScore(p));
    }
    CHECK_EQ(tricks, 10);
}

TEST(nineteen_deals_make_a_game)
{
    for (int players = Game::MinPlayers; players <= Game::MaxPlayers; ++players) {
        for (int level = 0; level < 3; ++level) {
            Game g(players * 10 + level);
            Bot bot(static_cast<Difficulty>(level), players + level);
            g.newGame(players);
            int deals = 1;
            bool legal = true;
            while (g.phase() != Phase::GameOver) {
                if (g.phase() == Phase::Score) {
                    CHECK(g.nextDeal());
                    ++deals;
                    CHECK_EQ(static_cast<int>(g.hand(0).size()), Game::handSize(deals));
                } else if (g.phase() == Phase::Bid) {
                    legal = g.bid(g.turn(), bot.chooseBid(g, g.turn())) && legal;
                } else if (g.trickComplete()) {
                    g.collectTrick();
                } else {
                    legal = g.play(g.turn(), bot.choosePlay(g, g.turn())) && legal;
                }
            }
            CHECK(legal);
            CHECK_EQ(deals, Game::Deals);
            CHECK(!g.nextDeal());
            int winners = 0;
            for (int p = 0; p < players; ++p)
                winners += g.isWinner(p);
            CHECK(winners >= 1);
        }
    }
}

TEST(careful_bots_make_more_bids_than_random_ones)
{
    int made[3] = {};
    for (int level = 0; level < 3; ++level) {
        Bot bot(static_cast<Difficulty>(level), 5);
        for (int game = 0; game < 4; ++game) {
            Game g(100 + game);
            g.newGame(4);
            while (g.phase() != Phase::GameOver) {
                if (g.phase() == Phase::Score) {
                    for (int p = 0; p < 4; ++p)
                        made[level] += g.dealScore(p) > 0;
                    g.nextDeal();
                } else if (g.phase() == Phase::Bid) {
                    g.bid(g.turn(), bot.chooseBid(g, g.turn()));
                } else if (g.trickComplete()) {
                    g.collectTrick();
                } else {
                    g.play(g.turn(), bot.choosePlay(g, g.turn()));
                }
            }
        }
    }
    std::printf("bids made out of %d: easy %d, medium %d, hard %d\n", 4 * 18 * 4, made[0], made[1], made[2]);
    CHECK(made[1] > made[0]);
    CHECK(made[2] >= made[1]);
}

int main()
{
    return cards::testing::run();
}
