#pragma once

#include "game.h"

#include <random>

namespace plump {

enum class Difficulty { Easy = 0, Medium = 1, Hard = 2 };

class Bot {
public:
    explicit Bot(Difficulty difficulty = Difficulty::Medium, unsigned seed = std::random_device{}());

    void setDifficulty(Difficulty d) { m_difficulty = d; }
    Difficulty difficulty() const { return m_difficulty; }

    // A bid, only valid on the player's turn to bid.
    int chooseBid(const Game &game, int player);
    // A legal card to play; only valid on the player's turn.
    Card choosePlay(const Game &game, int player);

    // How many tricks the hand looks good for, counting high cards, long
    // trumps and short suits to trump in.
    static double expectedTricks(const Game &game, int player);
    // The allowed bid closest to `expected`.
    static int nearestBid(const Game &game, int player, double expected);
    // The card a careful player would play: win cheaply while the bid needs
    // tricks, duck under the trick and throw the high cards away once it has
    // them. Also plays out every deal the hard bot tries.
    static Card cautiousPlay(const Game &game, int player);

private:
    int bidSimulated(const Game &game, int player);
    Card playSimulated(const Game &game, int player, const Cards &legal);
    // Deals the cards `player` cannot see to the others, consistent with
    // the suits they have shown out of.
    bool guessHands(const Game &game, int player, Cards hands[Game::MaxPlayers]);
    // Plays the deal to its end, everyone careful; whether `player` takes
    // exactly what they bid.
    static bool playsOut(Game game, int player);

    Difficulty m_difficulty;
    std::mt19937 m_rng;
};

} // namespace plump
