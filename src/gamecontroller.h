#pragma once

#include "core/bot.h"
#include "core/game.h"
#include "cards/gamelog.h"
#include "online/session.h"

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QVariantList>

#include <random>

class GameController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int phase READ phase NOTIFY changed)
    Q_PROPERTY(int players READ players NOTIFY changed)
    Q_PROPERTY(int deal READ deal NOTIFY changed)
    Q_PROPERTY(int deals READ deals CONSTANT)
    Q_PROPERTY(int handSize READ handSize NOTIFY changed)
    Q_PROPERTY(int nextHandSize READ nextHandSize NOTIFY changed)
    Q_PROPERTY(int dealer READ dealer NOTIFY changed)
    Q_PROPERTY(int trumpCard READ trumpCard NOTIFY changed)
    Q_PROPERTY(int trumps READ trumps NOTIFY changed)
    Q_PROPERTY(QVariantList hands READ hands NOTIFY changed)
    Q_PROPERTY(QVariantList won READ won NOTIFY changed)
    Q_PROPERTY(QVariantList trick READ trick NOTIFY changed)
    Q_PROPERTY(QVariantList lastTrick READ lastTrick NOTIFY changed)
    Q_PROPERTY(int trickWinner READ trickWinner NOTIFY changed)
    Q_PROPERTY(int tricksPlayed READ tricksPlayed NOTIFY changed)
    Q_PROPERTY(int turn READ turn NOTIFY changed)
    Q_PROPERTY(bool humanTurn READ humanTurn NOTIFY changed)
    Q_PROPERTY(bool humanBidding READ humanBidding NOTIFY changed)
    Q_PROPERTY(QVariantList bids READ bids NOTIFY changed)
    Q_PROPERTY(int totalBid READ totalBid NOTIFY changed)
    Q_PROPERTY(int forbiddenBid READ forbiddenBid NOTIFY changed)
    Q_PROPERTY(QVariantList taken READ taken NOTIFY changed)
    Q_PROPERTY(QVariantList scores READ scores NOTIFY changed)
    Q_PROPERTY(QVariantList dealScores READ dealScores NOTIFY changed)
    Q_PROPERTY(QVariantList winners READ winners NOTIFY changed)
    Q_PROPERTY(bool started READ started NOTIFY changed)
    Q_PROPERTY(int gameDifficulty READ gameDifficulty NOTIFY changed)
    Q_PROPERTY(int difficulty READ difficulty WRITE setDifficulty NOTIFY difficultyChanged)
    Q_PROPERTY(int playerCount READ playerCount WRITE setPlayerCount NOTIFY playerCountChanged)
    Q_PROPERTY(QVariantList stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool tapTwice READ tapTwice WRITE setTapTwice NOTIFY tapTwiceChanged)

public:
    enum Phase { Bid, Play, Score, GameOver };
    Q_ENUM(Phase)

    explicit GameController(Session *online, QObject *parent = nullptr);

    int phase() const;
    int players() const { return m_game.players(); }
    int deal() const { return m_game.deal(); }
    int deals() const { return plump::Game::Deals; }
    int handSize() const { return m_game.handSize(); }
    int nextHandSize() const { return plump::Game::handSize(m_game.deal() + 1); }
    int dealer() const { return m_game.dealer(); }
    int trumpCard() const { return m_started ? m_game.trumpCard().id() : -1; }
    int trumps() const { return m_game.trumps(); }
    // Every player's cards, sorted; ids let the table animate each card from
    // where it lies.
    QVariantList hands() const;
    // The cards of each player's tricks this deal.
    QVariantList won() const;
    // The cards on the table, each with who played it.
    QVariantList trick() const;
    // The trick gathered before, to look back at.
    QVariantList lastTrick() const;
    int trickWinner() const { return m_game.trickWinner(); }
    int tricksPlayed() const { return m_game.tricksPlayed(); }
    int turn() const { return m_game.turn(); }
    bool humanTurn() const;
    bool humanBidding() const;
    // Each player's bid, -1 until they make it.
    QVariantList bids() const;
    int totalBid() const { return m_game.totalBid(); }
    // The bid the dealer may not make, so the bids do not add up to the
    // tricks; -1 when every bid is open.
    int forbiddenBid() const { return m_game.forbiddenBid(); }
    // Tricks each player has taken this deal.
    QVariantList taken() const;
    QVariantList scores() const;
    QVariantList dealScores() const;
    // Whether each player has the highest score once the game is over.
    QVariantList winners() const;
    bool started() const { return m_started; }
    int gameDifficulty() const { return static_cast<int>(m_bot.difficulty()); }
    int difficulty() const { return m_difficulty; }
    void setDifficulty(int difficulty);
    // How many play against the bots, you included: 3 to 5.
    int playerCount() const { return m_playerCount; }
    void setPlayerCount(int count);
    QVariantList stats() const;
    // 0 = slow ... 3 = very fast; how long the bots think and tricks stay.
    int speed() const { return m_speed; }
    void setSpeed(int speed);
    // A card is played by a second tap; the first raises it.
    bool tapTwice() const { return m_tapTwice; }
    void setTapTwice(bool on);

    // Carries on the game against the bots from when the app last closed,
    // or deals a new one.
    Q_INVOKABLE void start();
    Q_INVOKABLE void newGame();
    Q_INVOKABLE bool canBid(int tricks) const;
    Q_INVOKABLE bool bid(int tricks);
    Q_INVOKABLE bool canPlay(int cardId) const;
    Q_INVOKABLE bool play(int cardId);
    Q_INVOKABLE bool nextDeal();
    Q_INVOKABLE void resetStats();

    Q_INVOKABLE QString playerName(int player) const;

signals:
    void changed();
    void difficultyChanged();
    void playerCountChanged();
    void statsChanged();
    void speedChanged();
    void tapTwiceChanged();

private:
    static constexpr int Human = 0;

    void afterAction();
    void botStep();
    void recordResult();
    // Whether a bot here bids or plays for `player` now: online, only on
    // the app that runs the bots.
    bool botPlays(int player) const;

    // Online, every move is one of the table's log, with its seats; the
    // session turns them round so that you sit at seat 0 here.
    bool online() const { return m_online->active(); }
    // Makes a move: online it goes to the table's log and comes back; against
    // the bots it is made at once and written down.
    bool perform(const QVariantMap &action);
    // Applies a move from the log; one the rules refuse is dropped, the same
    // way on every player's phone.
    bool apply(const QVariantMap &action);
    // Plays the whole log from the start, as on joining or coming back.
    void replay();
    void onTableChanged();
    QVariantList shuffledDeck();
    void sendNew();
    // A new game with the settings, as a move.
    QVariantMap newAction();

    Session *m_online;
    // The moves of the game against the bots.
    cards::GameLog m_log;
    plump::Game m_game;
    // Plays all the other seats; it only knows what the seat may see.
    plump::Bot m_bot;
    QSettings m_settings;
    QTimer m_botTimer;
    int m_difficulty = 1;
    int m_playerCount = 4;
    int m_speed = 1;
    bool m_tapTwice = false;
    bool m_started = false;
    bool m_recorded = false;
    int m_scoredDeal = 0; // the last deal counted for the statistics
    std::mt19937 m_rng{std::random_device{}()};
    bool m_wasOnline = false;
};
