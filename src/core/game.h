#pragma once

#include "card.h"

#include <random>

namespace plump {

enum class Phase {
    Bid,      // each player says how many tricks they will take
    Play,
    Score,    // the deal is over and counted
    GameOver,
};

struct Play {
    int player;
    Card card;
};

// Three to five players sit round the table in playing order, you at 0;
// play goes to the next number. Nineteen deals, from ten cards down to one
// and back up to ten; the card after the deal is turned up for trumps.
class Game {
public:
    static constexpr int MinPlayers = 3;
    static constexpr int MaxPlayers = 5;
    static constexpr int MaxHand = 10;
    static constexpr int Deals = 19;

    explicit Game(unsigned seed = std::random_device{}());

    // How many cards each player gets in deal 1 to 19.
    static int handSize(int deal);
    // What an exact bid scores; a miss scores nothing.
    static int bidScore(int bid) { return bid == 0 ? 10 : bid * 10; }

    // Clears the scores and deals the first deal, the dealer chosen at random.
    void newGame(int players);
    // As above, with the dealer and the pack given.
    void newGame(int players, int dealer, const Cards &deck);
    // After a deal is counted, the next player deals the next one.
    bool nextDeal();
    // As above, from `deck`: card i goes to the dealer's left plus i, round
    // the table, and the card after the hands is turned up.
    bool nextDeal(const Cards &deck);

    bool canBid(int player, int tricks) const;
    // The bid the dealer may not make, as it would add up to the tricks in
    // the deal; -1 when every bid is open.
    int forbiddenBid() const;
    bool bid(int player, int tricks);
    bool canPlayCard(int player, const Card &card) const;
    Cards legalCards(int player) const;
    bool play(int player, const Card &card);
    // Gives a finished trick to its winner, who leads the next.
    bool collectTrick();

    int players() const { return m_players; }
    Phase phase() const { return m_phase; }
    // 1 to 19.
    int deal() const { return m_deal; }
    int handSize() const { return handSize(m_deal); }
    int dealer() const { return m_dealer; }
    const Card &trumpCard() const { return m_trumpCard; }
    int trumps() const { return m_trumpCard.suit; }

    const Cards &hand(int player) const { return m_hand[player]; }
    // The player's bid, -1 before they make it.
    int bidOf(int player) const { return m_bid[player]; }
    // The bids made so far added up.
    int totalBid() const;
    int tricksTaken(int player) const { return m_taken[player]; }
    // The cards of the tricks the player has taken this deal.
    const Cards &won(int player) const { return m_won[player]; }
    const std::vector<Play> &trick() const { return m_trick; }
    // Every card played this deal, in order, the current trick included.
    const std::vector<Play> &played() const { return m_played; }
    // The suit of the trick's first card, -1 before it is led.
    int ledSuit() const { return m_trick.empty() ? -1 : m_trick.front().card.suit; }
    int leader() const { return m_leader; }
    // Whose turn it is to bid or play; -1 while a finished trick is still on
    // the table, and between deals.
    int turn() const { return m_turn; }
    bool trickComplete() const { return static_cast<int>(m_trick.size()) == m_players; }
    // The highest trump, or the highest card of the suit led.
    int trickWinner() const;
    int tricksPlayed() const { return m_tricks; }

    // What the finished deal added to each score.
    int dealScore(int player) const { return m_dealScore[player]; }
    int score(int player) const { return m_score[player]; }
    // After the last deal: the highest score wins, shared on a tie.
    bool isWinner(int player) const;

    // Lets the bot try out guessed hands on a copy of the game.
    void replaceHand(int player, const Cards &cards) { m_hand[player] = cards; }

private:
    void dealFrom(const Cards &deck);
    void scoreDeal();
    bool holds(int player, const Card &card) const;
    bool holdsSuit(int player, int suit) const;

    std::mt19937 m_rng;
    int m_players = 4;
    Phase m_phase = Phase::Bid;
    int m_deal = 0;
    int m_dealer = 0;
    Card m_trumpCard;
    Cards m_hand[MaxPlayers];
    int m_bid[MaxPlayers] = {};
    int m_taken[MaxPlayers] = {};
    Cards m_won[MaxPlayers];
    std::vector<Play> m_trick;
    std::vector<Play> m_played;
    int m_leader = 0;
    int m_turn = -1;
    int m_tricks = 0;
    int m_dealScore[MaxPlayers] = {};
    int m_score[MaxPlayers] = {};
};

} // namespace plump
