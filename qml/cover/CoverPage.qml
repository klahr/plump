import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.plump 1.0
import "../cards/cards.js" as Cards

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Plump")
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }

        Repeater {
            model: game.players
            Item {
                width: parent.width
                height: scoreLabel.height
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - scoreLabel.width - Theme.paddingSmall
                    text: game.playerName(index)
                    color: Cards.playerColor(index)
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    id: scoreLabel
                    anchors.right: parent.right
                    text: game.scores[index]
                    color: Cards.playerColor(index)
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                }
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: {
                switch (game.phase) {
                case Plump.Bid:
                case Plump.Play:
                    return qsTr("Deal %1 of %2").arg(game.deal).arg(game.deals) + " · " + Cards.suits[Cards.suit(game.trumpCard)]
                case Plump.Score:
                    return qsTr("Deal %1 counted").arg(game.deal)
                }
                var won = []
                for (var p = 0; p < game.winners.length; ++p)
                    if (game.winners[p])
                        won.push(p)
                if (won.length !== 1)
                    return qsTr("Shared win")
                return won[0] === 0 ? qsTr("You won") : qsTr("%1 won").arg(game.playerName(won[0]))
            }
        }
    }

    CoverActionList {
        enabled: game.phase === Plump.GameOver
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: game.newGame()
        }
    }
}
