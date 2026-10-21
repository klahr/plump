#include "gamecontroller.h"

#include "cards/settings.h"
#include "cards/variant.h"

#include <QVariantMap>

#include <algorithm>

using namespace plump;

namespace {

// How long the bots think, and how long a finished trick stays on the
// table, from slow to very fast.
const int BotDelayMs[] = {1100, 750, 450, 200};
const int TrickHoldMs[] = {1800, 1200, 800, 450};

} // namespace

GameController::GameController(Session *online, QObject *parent)
    : QObject(parent)
    , m_online(online)
    , m_settings(cards::settingsPath(), QSettings::IniFormat)
{
    m_difficulty = qBound(0, m_settings.value(QStringLiteral("difficulty"), 1).toInt(), 2);
    m_playerCount = qBound(Game::MinPlayers, m_settings.value(QStringLiteral("players"), 4).toInt(), Game::MaxPlayers);
    m_speed = qBound(0, m_settings.value(QStringLiteral("speed"), 1).toInt(), 3);
    m_tapTwice = m_settings.value(QStringLiteral("tapTwice"), false).toBool();
    m_botTimer.setSingleShot(true);
    connect(&m_botTimer, &QTimer::timeout, this, &GameController::botStep);
    connect(m_online, &Session::reset, this, &GameController::replay);
    connect(m_online, &Session::action, this, [this](const QVariantMap &action) {
        if (m_online->active() && apply(action))
            afterAction();
    });
    connect(m_online, &Session::tableChanged, this, &GameController::onTableChanged);
}

int GameController::phase() const
{
    switch (m_game.phase()) {
    case plump::Phase::Bid:
        return Bid;
    case plump::Phase::Play:
        return Play;
    case plump::Phase::Score:
        return Score;
    case plump::Phase::GameOver:
        break;
    }
    return GameOver;
}

QVariantList GameController::hands() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << QVariant(ids(m_game.hand(p)));
    return out;
}

QVariantList GameController::won() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << QVariant(ids(m_game.won(p)));
    return out;
}

QVariantList GameController::trick() const
{
    QVariantList out;
    for (const plump::Play &p : m_game.trick()) {
        QVariantMap m;
        m.insert(QStringLiteral("card"), p.card.id());
        m.insert(QStringLiteral("player"), p.player);
        out << m;
    }
    return out;
}

QVariantList GameController::lastTrick() const
{
    return cards::lastTrick(m_game.played(), m_game.tricksPlayed(), players(), m_game.leader());
}

bool GameController::humanTurn() const
{
    return m_started && m_game.phase() == plump::Phase::Play && m_game.turn() == Human;
}

bool GameController::humanBidding() const
{
    return m_started && m_game.phase() == plump::Phase::Bid && m_game.turn() == Human;
}

QVariantList GameController::bids() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << m_game.bidOf(p);
    return out;
}

QVariantList GameController::taken() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << m_game.tricksTaken(p);
    return out;
}

QVariantList GameController::scores() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << m_game.score(p);
    return out;
}

QVariantList GameController::dealScores() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << m_game.dealScore(p);
    return out;
}

QVariantList GameController::winners() const
{
    QVariantList out;
    for (int p = 0; p < players(); ++p)
        out << m_game.isWinner(p);
    return out;
}

void GameController::setDifficulty(int difficulty)
{
    difficulty = qBound(0, difficulty, 2);
    if (difficulty == m_difficulty)
        return;
    m_difficulty = difficulty;
    m_settings.setValue(QStringLiteral("difficulty"), difficulty);
    emit difficultyChanged();
}

void GameController::setPlayerCount(int count)
{
    count = qBound(Game::MinPlayers, count, Game::MaxPlayers);
    if (count == m_playerCount)
        return;
    m_playerCount = count;
    m_settings.setValue(QStringLiteral("players"), count);
    emit playerCountChanged();
}

void GameController::setSpeed(int speed)
{
    speed = qBound(0, speed, 3);
    if (speed == m_speed)
        return;
    m_speed = speed;
    m_settings.setValue(QStringLiteral("speed"), speed);
    emit speedChanged();
}

void GameController::setTapTwice(bool on)
{
    if (on == m_tapTwice)
        return;
    m_tapTwice = on;
    m_settings.setValue(QStringLiteral("tapTwice"), on);
    emit tapTwiceChanged();
}

QVariantList GameController::stats() const
{
    QVariantList out;
    for (int level = 0; level < 3; ++level) {
        const QString key = QStringLiteral("stats/%1/").arg(level);
        QVariantMap m;
        m.insert(QStringLiteral("won"), m_settings.value(key + QStringLiteral("won"), 0).toInt());
        m.insert(QStringLiteral("lost"), m_settings.value(key + QStringLiteral("lost"), 0).toInt());
        m.insert(QStringLiteral("bids"), m_settings.value(key + QStringLiteral("bids"), 0).toInt());
        m.insert(QStringLiteral("made"), m_settings.value(key + QStringLiteral("made"), 0).toInt());
        out << m;
    }
    return out;
}

void GameController::resetStats()
{
    m_settings.remove(QStringLiteral("stats"));
    emit statsChanged();
}

QString GameController::playerName(int player) const
{
    if (online() && player > 0 && player < players())
        return m_online->playerName(m_online->toTable(player));
    if (player == 0)
        return tr("You");
    // Named by where they sit, as in the other card games: you at the
    // bottom, the others round the table to your left.
    switch (players()) {
    case 3:
        if (player == 1)
            return tr("West");
        if (player == 2)
            return tr("East");
        break;
    case 5:
        switch (player) {
        case 1:
            return tr("West");
        case 2:
            return tr("Northwest");
        case 3:
            return tr("Northeast");
        case 4:
            return tr("East");
        }
        break;
    default:
        switch (player) {
        case 1:
            return tr("West");
        case 2:
            return tr("North");
        case 3:
            return tr("East");
        }
    }
    return QString();
}

void GameController::start()
{
    if (online())
        return;
    m_botTimer.stop();
    m_game = plump::Game();
    m_started = false;
    bool ok = true;
    for (const QVariant &action : m_log.load())
        ok = ok && apply(action.toMap());
    if (!ok || !m_started) {
        newGame();
        return;
    }
    // Whatever was counted before the app closed is not counted again.
    if (m_game.phase() == plump::Phase::Score || m_game.phase() == plump::Phase::GameOver)
        m_scoredDeal = m_game.deal();
    m_recorded = m_game.phase() == plump::Phase::GameOver;
    afterAction();
}

void GameController::newGame()
{
    if (online()) {
        if (!m_started || m_game.phase() == plump::Phase::GameOver)
            sendNew();
        return;
    }
    m_botTimer.stop();
    m_log.clear();
    m_started = false;
    perform(newAction());
}

bool GameController::canBid(int tricks) const
{
    return humanBidding() && m_game.canBid(Human, tricks);
}

bool GameController::bid(int tricks)
{
    if (!canBid(tricks))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("bid")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("bid"), tricks}});
}

bool GameController::canPlay(int cardId) const
{
    return humanTurn() && m_game.canPlayCard(Human, Card::fromId(cardId));
}

bool GameController::play(int cardId)
{
    if (!canPlay(cardId))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("play")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("card"), cardId}});
}

bool GameController::nextDeal()
{
    if (m_game.phase() != plump::Phase::Score)
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("next")}, {QStringLiteral("deck"), shuffledDeck()}});
}

bool GameController::botPlays(int player) const
{
    if (player < 0 || player == Human)
        return false;
    return !online() || (m_started && m_online->driver() && m_online->isBot(m_online->toTable(player)));
}

void GameController::afterAction()
{
    const plump::Phase p = m_game.phase();
    if (!online() && (p == plump::Phase::Score || p == plump::Phase::GameOver) && m_scoredDeal != m_game.deal()) {
        m_scoredDeal = m_game.deal();
        const QString key = QStringLiteral("stats/%1/").arg(gameDifficulty());
        m_settings.setValue(key + QStringLiteral("bids"), m_settings.value(key + QStringLiteral("bids"), 0).toInt() + 1);
        if (m_game.tricksTaken(Human) == m_game.bidOf(Human))
            m_settings.setValue(key + QStringLiteral("made"), m_settings.value(key + QStringLiteral("made"), 0).toInt() + 1);
        emit statsChanged();
    }
    if (p == plump::Phase::GameOver)
        recordResult();

    // Online, one app at the table runs the bots and gathers the tricks.
    if (p == plump::Phase::Bid && botPlays(m_game.turn())) {
        m_botTimer.setInterval(BotDelayMs[m_speed]);
        m_botTimer.start();
    } else if (p == plump::Phase::Play) {
        if (m_game.trickComplete() && (!online() || (m_started && m_online->driver()))) {
            m_botTimer.setInterval(TrickHoldMs[m_speed]);
            m_botTimer.start();
        } else if (botPlays(m_game.turn())) {
            m_botTimer.setInterval(BotDelayMs[m_speed]);
            m_botTimer.start();
        }
    }
    emit changed();
}

void GameController::botStep()
{
    if (online() && !m_online->driver())
        return;
    if (m_game.phase() == plump::Phase::Bid) {
        const int player = m_game.turn();
        if (!botPlays(player))
            return;
        perform({{QStringLiteral("t"), QStringLiteral("bid")},
                 {QStringLiteral("seat"), m_online->toTable(player)},
                 {QStringLiteral("bid"), m_bot.chooseBid(m_game, player)}});
    } else if (m_game.phase() == plump::Phase::Play) {
        if (m_game.trickComplete()) {
            perform({{QStringLiteral("t"), QStringLiteral("collect")}});
        } else {
            const int player = m_game.turn();
            if (!botPlays(player))
                return;
            perform({{QStringLiteral("t"), QStringLiteral("play")},
                     {QStringLiteral("seat"), m_online->toTable(player)},
                     {QStringLiteral("card"), m_bot.choosePlay(m_game, player).id()}});
        }
    }
}

void GameController::recordResult()
{
    if (m_recorded)
        return;
    m_recorded = true;
    const QString key = QStringLiteral("stats/%1/").arg(gameDifficulty());
    const QString result = m_game.isWinner(Human) ? QStringLiteral("won") : QStringLiteral("lost");
    m_settings.setValue(key + result, m_settings.value(key + result, 0).toInt() + 1);
    emit statsChanged();
}

QVariantList GameController::shuffledDeck()
{
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    return ids(deck);
}

void GameController::sendNew()
{
    m_online->send(newAction());
}

QVariantMap GameController::newAction()
{
    // Online, everyone at the table plays; against the bots, as many as set.
    const int players = online() ? m_online->seatCount() : m_playerCount;
    return {{QStringLiteral("t"), QStringLiteral("new")},
            {QStringLiteral("players"), players},
            {QStringLiteral("difficulty"), m_difficulty},
            {QStringLiteral("dealer"), std::uniform_int_distribution<int>(0, players - 1)(m_rng)},
            {QStringLiteral("deck"), shuffledDeck()}};
}

bool GameController::perform(const QVariantMap &action)
{
    if (online()) {
        m_online->send(action);
        return true;
    }
    if (!apply(action))
        return false;
    m_log.append(action);
    afterAction();
    return true;
}

bool GameController::apply(const QVariantMap &action)
{
    const QString t = action.value(QStringLiteral("t")).toString();
    if (t == QLatin1String("new")) {
        const int n = online() ? m_online->seatCount() : action.value(QStringLiteral("players")).toInt();
        const int dealer = m_online->toLocal(action.value(QStringLiteral("dealer")).toInt());
        Cards deck;
        if (!deckFrom(action.value(QStringLiteral("deck")), &deck) || (m_started && m_game.phase() != plump::Phase::GameOver) || n < Game::MinPlayers || n > Game::MaxPlayers
            || dealer < 0)
            return false;
        m_botTimer.stop();
        m_bot.setDifficulty(static_cast<Difficulty>(qBound(0, action.value(QStringLiteral("difficulty"), m_difficulty).toInt(), 2)));
        m_game.newGame(n, dealer, deck);
        m_started = true;
        // Online games stay out of the statistics.
        m_recorded = online();
        m_scoredDeal = 0;
        return true;
    }
    if (!m_started)
        return false;
    if (t == QLatin1String("bid")) {
        const int player = m_online->toLocal(action.value(QStringLiteral("seat")).toInt());
        return player >= 0 && m_game.bid(player, action.value(QStringLiteral("bid")).toInt());
    }
    if (t == QLatin1String("play")) {
        const int player = m_online->toLocal(action.value(QStringLiteral("seat")).toInt());
        return player >= 0
               && m_game.play(player, Card::fromId(qBound(0, action.value(QStringLiteral("card")).toInt(), 51)));
    }
    if (t == QLatin1String("collect"))
        return m_game.collectTrick();
    if (t == QLatin1String("next")) {
        Cards deck;
        return deckFrom(action.value(QStringLiteral("deck")), &deck) && m_game.nextDeal(deck);
    }
    return false;
}

void GameController::replay()
{
    if (!online())
        return;
    m_botTimer.stop();
    m_game = plump::Game();
    m_started = false;
    for (const QVariant &action : m_online->actions())
        apply(action.toMap());
    if (m_online->actions().isEmpty() && m_online->driver())
        sendNew();
    afterAction();
}

void GameController::onTableChanged()
{
    if (!online()) {
        // Left the table: back to your game against the bots.
        if (m_wasOnline) {
            m_wasOnline = false;
            start();
        }
        return;
    }
    if (!m_wasOnline) {
        // Nothing of the game against the bots carries over; the log follows.
        m_wasOnline = true;
        m_botTimer.stop();
        m_game = plump::Game();
        m_started = false;
    }
    // Someone may have come or gone, or a bot taken over.
    if (m_online->synced() && m_online->actions().isEmpty() && m_online->driver())
        sendNew();
    afterAction();
}
