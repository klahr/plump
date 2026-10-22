import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.plump 1.0
import "../components"
import "../cards"
import "../cards/cards.js" as Cards
import "../online"

Page {
    id: page

    // One card size everywhere: seven overlapping cards fill a row of your hand.
    readonly property real cardWidth: Math.floor((width - 2 * Theme.horizontalPageMargin) / (1 + 5 * 0.76))
    readonly property real cardHeight: Math.round(cardWidth * 1.42)
    readonly property real margin: Theme.horizontalPageMargin
    // Your hand lies in two rows while it holds more than seven cards.
    readonly property real handRowStep: Math.round(cardHeight * 0.5)
    // How much of the side players' cards shows at the edge of the screen.
    readonly property real sideShown: Math.round(cardHeight * 0.42)
    readonly property bool scoring: game.phase === Plump.Score || game.phase === Plump.GameOver
    readonly property int players: game.players

    readonly property var places: computePlaces()
    // Holding the table: the last trick shows over this one.
    readonly property bool peeking: peek.peeking && game.lastTrick.length > 0

    function lastTrickWinner() {
        var last = game.lastTrick
        for (var i = 0; i < last.length; ++i)
            if (last[i].won)
                return last[i].player
        return -1
    }

    allowedOrientations: Orientation.Portrait

    function startNewGame() {
        if (game.phase === Plump.GameOver || game.deal <= 1 && game.tricksPlayed === 0 && game.bids[0] < 0)
            game.newGame()
        else
            remorse.execute(qsTr("Starting a new game"), function() { game.newGame() })
    }

    // A player's name, or a line meant for you, in the player's colour.
    // Grey and faint for a player who has lost the connection, until a bot plays for them.
    function seatColor(player) { return online.seats.length && online.away(online.toTable(player)) ? Cards.awayColor(player) : Cards.playerColor(player) }
    function coloured(player, text) { return "<font color=\"" + seatColor(player) + "\">" + text + "</font>" }
    function named(player) { return coloured(player, game.playerName(player)) }

    // Where each player sits: you at the bottom, the others round the table.
    function position(player) {
        switch (players) {
        case 3: return ["bottom", "left", "right"][player]
        case 5: return ["bottom", "left", "topLeft", "topRight", "right"][player]
        }
        return ["bottom", "left", "top", "right"][player]
    }

    // Where each player's card goes on the table.
    function trickSpot(player) {
        var x = (page.width - cardWidth) / 2
        var top = cluster.y + tableRow.y
        var middle = top + (tableRow.height - cardHeight) / 2
        // With two players at the top, the sides sit lower.
        var side = players === 5 ? top + (tableRow.height - cardHeight) * 0.75 : middle
        switch (position(player)) {
        case "left": return { x: x - cardWidth * 1.15, y: side }
        case "right": return { x: x + cardWidth * 1.15, y: side }
        case "top": return { x: x, y: top }
        case "topLeft": return { x: x - cardWidth * 0.6, y: top }
        case "topRight": return { x: x + cardWidth * 0.6, y: top }
        }
        return { x: x, y: top + tableRow.height - cardHeight }
    }

    // The middle of a top player's cards across the top.
    function topCentre(player) {
        switch (position(player)) {
        case "topLeft": return page.width / 4
        case "topRight": return page.width * 3 / 4
        }
        return page.width / 2
    }

    // Where each player keeps the tricks taken: under their own cards, yours
    // behind the first row of your hand.
    function pileSpot(player) {
        var w = page.width
        var middle = cluster.y + tableRow.y + (tableRow.height - cardHeight) / 2
        switch (position(player)) {
        case "left": return { x: sideShown - (cardHeight + cardWidth) / 2, y: middle, rot: 90 }
        case "right": return { x: w - sideShown + (cardHeight - cardWidth) / 2, y: middle, rot: 90 }
        case "top":
        case "topLeft":
        case "topRight":
            return { x: topCentre(player) - cardWidth / 2, y: cluster.y + northRow.y, rot: 0 }
        }
        return { x: (w - cardWidth) / 2, y: cluster.y + handRow.y + Theme.paddingLarge, rot: 0 }
    }

    // Where every one of the 52 cards lies, in content coordinates.
    function computePlaces() {
        var cw = cardWidth
        var ch = cardHeight
        var w = page.width
        var hands = game.hands
        var tableY = cluster.y + tableRow.y
        var handY = cluster.y + handRow.y + Theme.paddingLarge
        // Cards fly out one after the other as they are dealt.
        var dealing = game.phase === Plump.Bid
        var p = []
        var i, k, s, n, off, x0, ids, spot

        // Cards out of play are hidden where your hand is, and dealt from there.
        for (i = 0; i < 52; ++i)
            p.push({ x: (w - cw) / 2, y: handY, z: 1, down: true, rot: 0, role: "deck", delay: 0, hidden: true })

        if (scoring || hands.length !== players)
            return p

        // Tricks taken lie face down under the taker's cards.
        for (s = 0; s < players; ++s) {
            spot = pileSpot(s)
            ids = game.won[s]
            for (i = 0; i < ids.length; ++i)
                p[ids[i]] = { x: spot.x, y: spot.y, z: 5 + i, down: true, rot: spot.rot, role: "taken",
                              delay: 0, glowColor: Cards.playerColor(s) }
        }

        // Your cards, centred, in two rows while there are many.
        ids = hands[0]
        n = ids.length
        var top = n > 7 ? Math.ceil(n / 2) : n
        off = top > 1 ? Math.min(cw * 0.76, (w - 2 * margin - cw) / (top - 1)) : 0
        for (i = 0; i < n; ++i) {
            var row = i < top ? 0 : 1
            var inRow = row === 0 ? top : n - top
            k = row === 0 ? i : i - top
            x0 = (w - cw - (inRow - 1) * off) / 2
            p[ids[i]] = { x: x0 + k * off, y: handY + row * handRowStep, z: 50 + i, down: false, rot: 0,
                          role: "hand", delay: dealing ? i * 30 : 0 }
        }

        // The sides hold their cards sideways at the edges, the others
        // across the top.
        for (s = 1; s < players; ++s) {
            ids = hands[s]
            n = ids.length
            var where = position(s)
            if (where === "left" || where === "right") {
                off = n > 1 ? Math.min(cw * 0.3, (tableRow.height - cw) / (n - 1)) : 0
                var centreX = where === "left" ? sideShown - ch / 2 : w - sideShown + ch / 2
                var centreY = tableY + (tableRow.height - (n - 1) * off) / 2
                for (i = 0; i < n; ++i)
                    p[ids[i]] = { x: centreX - cw / 2, y: centreY + i * off - ch / 2, z: 30 + i, down: true, rot: 90,
                                  role: "bot", delay: dealing ? i * 30 : 0 }
            } else {
                var room = where === "top" ? w - 2 * margin : w / 2 - margin
                off = n > 1 ? Math.min(cw * 0.22, (room - cw) / (n - 1)) : 0
                x0 = topCentre(s) - (cw + (n - 1) * off) / 2
                for (i = 0; i < n; ++i)
                    p[ids[i]] = { x: x0 + i * off, y: cluster.y + northRow.y, z: 30 + i, down: true, rot: 0,
                                  role: "bot", delay: dealing ? i * 30 : 0 }
            }
        }

        // The trick, each card in front of whoever played it; the winning
        // card lights up once everyone's is down.
        var trick = game.trick
        for (i = 0; i < trick.length; ++i) {
            spot = trickSpot(trick[i].player)
            p[trick[i].card] = { x: spot.x, y: spot.y, z: 100 + i, down: false, rot: 0, role: "trick", delay: 0,
                                 glow: trick[i].player === game.trickWinner,
                                 glowColor: Cards.playerColor(trick[i].player) }
        }

        // Held down on the table: the last trick back up, over this one.
        if (peeking) {
            for (i = 0; i < trick.length; ++i)
                p[trick[i].card].hidden = true
            var last = game.lastTrick
            for (i = 0; i < last.length; ++i) {
                spot = trickSpot(last[i].player)
                p[last[i].card] = { x: spot.x, y: spot.y, z: 300 + i, down: false, rot: 0, role: "last", delay: 0,
                                    glow: last[i].won, glowColor: Cards.playerColor(last[i].player) }
            }
        }
        return p
    }

    RemorsePopup { id: remorse }

    ConnectionBanner { }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(page.height, cluster.y + cluster.height + Theme.paddingLarge)

        PullDownMenu {
            MenuItem {
                text: qsTr("About and rules")
                onClicked: pageStack.push(Qt.resolvedUrl("RulesPage.qml"))
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: online.active ? qsTr("Online table") : qsTr("Play online")
                onClicked: pageStack.push(Qt.resolvedUrl(online.atTable ? "../online/TablePage.qml"
                                                                        : "../online/LobbyPage.qml"))
            }
            MenuItem {
                text: qsTr("New game")
                // Online, a new game starts only once this one is over.
                visible: !online.active || game.phase === Plump.GameOver
                onClicked: page.startNewGame()
            }
        }

        Item {
            id: content
            width: parent.width
            height: parent.contentHeight

            // ---------------------------------------------------- board
            Row {
                id: board
                x: margin
                // Below the notch, on phones with one.
                y: Screen.topCutout.height + Theme.paddingLarge
                width: parent.width - 2 * margin

                Repeater {
                    model: page.players
                    PlayerScore {
                        width: board.width / page.players
                        name: game.playerName(index)
                        score: game.scores[index] || 0
                        leader: Math.max.apply(null, game.scores.concat([0]))
                        bid: game.bids[index] !== undefined ? game.bids[index] : -1
                        taken: game.taken[index] || 0
                        dealer: game.dealer === index
                        showBid: !page.scoring
                        active: (game.phase === Plump.Bid || game.phase === Plump.Play) && game.turn === index
                        color: page.seatColor(index)
                    }
                }
            }

            // Everything else sits together in the middle of the screen, so
            // your hand is within easy reach of the thumb.
            Column {
                id: cluster
                width: parent.width
                y: board.y + board.height
                   + Math.max(Theme.paddingLarge,
                              (page.height - board.y - board.height
                               - (height - info.height + messageBlock.height)) * 0.4)
                spacing: Theme.paddingMedium

                // The top players' cards (drawn by the card layer).
                Item {
                    id: northRow
                    width: parent.width
                    height: cardHeight
                }

                // The trick, with the side players at the edges.
                Item {
                    id: tableRow
                    width: parent.width
                    height: Math.round(cardHeight * 1.8)
                }

                // Messages, the count of the deal, or the result.
                Item {
                    id: info
                    width: parent.width
                    height: game.phase === Plump.GameOver ? overPanel.height : messageBlock.height

                    Column {
                        id: messageBlock
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: page.scoring ? 0 : 1
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { } }

                        // The deal, trumps and the bids so far.
                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            truncationMode: TruncationMode.Fade
                            textFormat: Text.StyledText
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeSmall
                            text: {
                                if (!game.started)
                                    return ""
                                var trump = game.trumpCard
                                var suit = "<font color=\"" + (Cards.isRed(trump) ? "#ff6e6e" : Theme.primaryColor) + "\">"
                                        + Cards.name(trump) + "</font>"
                                return qsTr("Deal %1 of %2 · %n card(s)", "", game.handSize).arg(game.deal).arg(game.deals)
                                        + " · " + qsTr("Trumps %1").arg(suit)
                                        + " · " + qsTr("Bids %1").arg(game.totalBid)
                            }
                        }
                        Label {
                            width: parent.width
                            height: fontMetrics.height
                            horizontalAlignment: Text.AlignHCenter
                            truncationMode: TruncationMode.Fade
                            textFormat: Text.StyledText
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            text: {
                                switch (game.phase) {
                                case Plump.Bid:
                                    if (game.humanBidding)
                                        return page.coloured(0, game.forbiddenBid >= 0
                                                             ? qsTr("How many tricks will you take? Not %1.").arg(game.forbiddenBid)
                                                             : qsTr("How many tricks will you take?"))
                                    return qsTr("%1 is bidding…").arg(page.named(game.turn))
                                case Plump.Play:
                                    if (page.peeking)
                                        return page.lastTrickWinner() === 0 ? page.coloured(0, qsTr("You took the last trick."))
                                                                            : qsTr("%1 took the last trick.").arg(page.named(page.lastTrickWinner()))
                                    if (game.trickWinner >= 0)
                                        return game.trickWinner === 0 ? page.coloured(0, qsTr("You take the trick."))
                                                                      : qsTr("%1 takes the trick.").arg(page.named(game.trickWinner))
                                    if (game.humanTurn)
                                        return page.coloured(0, game.trick.length > 0 ? qsTr("Your turn. Tap a card to play it.")
                                                                                      : qsTr("Your lead."))
                                    return qsTr("%1 is playing…").arg(page.named(game.turn))
                                }
                                return ""
                            }
                        }

                        FontMetrics {
                            id: fontMetrics
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }

                    Column {
                        id: overPanel
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: game.phase === Plump.GameOver ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { duration: 400 } }

                        readonly property var winners: {
                            var out = []
                            for (var p = 0; p < game.winners.length; ++p)
                                if (game.winners[p])
                                    out.push(p)
                            return out
                        }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            textFormat: Text.StyledText
                            text: {
                                var w = overPanel.winners
                                if (w.length === 1)
                                    return w[0] === 0 ? page.coloured(0, qsTr("You won"))
                                                      : qsTr("%1 won").arg(page.named(w[0]))
                                var names = []
                                for (var i = 0; i < w.length; ++i)
                                    names.push(page.named(w[i]))
                                return qsTr("Shared win: %1").arg(names.join(", "))
                            }
                            font.pixelSize: overPanel.winners.length === 1 ? Theme.fontSizeHuge : Theme.fontSizeLarge
                            font.family: Theme.fontFamilyHeading
                        }
                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: overPanel.winners.length > 0
                                  ? qsTr("Highest score with %1 points").arg(game.scores[overPanel.winners[0]]) : ""
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeMedium
                        }
                    }
                }

                // Space for your hand (cards are drawn by the card layer).
                Item {
                    id: handRow
                    width: parent.width
                    height: cardHeight + handRowStep + Theme.paddingLarge
                }

                Item {
                    width: parent.width
                    height: Math.max(Theme.itemSizeSmall, bidFlow.height)

                    // Your bid, from nothing to every trick, keyed like Bridge's bidding box.
                    Grid {
                        id: bidFlow
                        x: margin
                        width: parent.width - 2 * margin
                        visible: game.humanBidding
                        columns: 6

                        Repeater {
                            model: game.humanBidding ? game.handSize + 1 : 0
                            CallButton {
                                width: bidFlow.width / 6
                                text: index
                                // The dealer may not make the bids add up to the tricks.
                                enabled: index !== game.forbiddenBid
                                onClicked: game.bid(index)
                            }
                        }
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: Theme.paddingLarge

                        Button {
                            visible: game.phase === Plump.Score
                            text: qsTr("Next deal")
                            onClicked: game.nextDeal()
                        }
                        Button {
                            visible: game.phase === Plump.GameOver
                            text: qsTr("New game")
                            onClicked: game.newGame()
                        }
                    }
                }
            }

            // Each player's bid, tricks and what the deal scored, over the
            // empty table so that the button below stays in view.
            Column {
                id: scorePanel
                x: margin
                y: cluster.y + northRow.y
                width: parent.width - 2 * margin
                opacity: game.phase === Plump.Score ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { FadeAnimation { } }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Deal %1 of %2").arg(game.deal).arg(game.deals)
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                }
                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Next deal: %n card(s)", "", game.nextHandSize)
                    color: Theme.secondaryHighlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                Item { width: 1; height: Theme.paddingMedium }
                Repeater {
                    model: page.players
                    Item {
                        width: scorePanel.width
                        height: Theme.itemSizeExtraSmall

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width * 0.4
                            text: game.playerName(index)
                            color: page.seatColor(index)
                            truncationMode: TruncationMode.Fade
                        }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            x: parent.width * 0.4
                            width: parent.width * 0.4
                            //: Tricks taken of the bid
                            text: qsTr("%1 of %2").arg(game.taken[index]).arg(game.bids[index])
                            color: Theme.secondaryColor
                        }
                        Label {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            text: game.dealScores[index] > 0 ? "+" + game.dealScores[index] : "0"
                            color: game.dealScores[index] > 0 ? Cards.playerColor(index) : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeLarge
                            font.family: Theme.fontFamilyHeading
                        }
                    }
                }
            }

            // Hold the table to see the last trick.
            PeekArea {
                id: peek
                y: cluster.y + tableRow.y
                width: parent.width
                height: tableRow.height
                enabled: game.phase === Plump.Play && game.lastTrick.length > 0
            }

            // ---------------------------------------------------- cards
            TapToPlay {
                id: tapToPlay
                twice: game.tapTwice
            }

            // All 52 cards live here and glide to wherever the game puts them.
            Repeater {
                model: 52
                TableCard {
                    readonly property bool inHand: place.role === "hand"

                    place: page.places[index]
                    width: cardWidth
                    cardId: index
                    selected: tapToPlay.raised === index
                    // The card winning the trick.
                    glowing: place.glow === true
                    glowColor: place.glowColor !== undefined ? place.glowColor : Theme.highlightColor
                    // game.trick makes these follow the play.
                    dimmed: inHand && game.humanTurn && game.trick.length >= 0 && !game.canPlay(index)
                    enabled: inHand && game.humanTurn && game.trick.length >= 0 && game.canPlay(index)

                    onEnabledChanged: if (!enabled) tapToPlay.lower(index)
                    onClicked: if (tapToPlay.tap(index)) game.play(index)
                }
            }
        }
    }
}
