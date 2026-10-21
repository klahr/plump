import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                label: qsTr("Bots")
                currentIndex: game.difficulty
                description: [
                    qsTr("Bid by guesswork and play at random, within the rules."),
                    qsTr("Count their high cards and trumps, win cheaply while they need tricks and duck once they have them."),
                    qsTr("Keep track of who is out of which suit and think every bid and card through to the end of the deal.")
                ][currentIndex] + " " + qsTr("Takes effect in the next game.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Easy") }
                    MenuItem { text: qsTr("Medium") }
                    MenuItem { text: qsTr("Hard") }
                }
                onCurrentIndexChanged: game.difficulty = currentIndex
            }

            ComboBox {
                label: qsTr("Players")
                currentIndex: game.playerCount - 3
                description: qsTr("You and the bots.") + " " + qsTr("Takes effect in the next game.")
                menu: ContextMenu {
                    MenuItem { text: "3" }
                    MenuItem { text: "4" }
                    MenuItem { text: "5" }
                }
                onCurrentIndexChanged: game.playerCount = currentIndex + 3
            }

            ComboBox {
                label: qsTr("Play speed")
                currentIndex: game.speed
                description: qsTr("How long the bots take to play and a finished trick stays on the table.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Slow") }
                    MenuItem { text: qsTr("Normal") }
                    MenuItem { text: qsTr("Fast") }
                    MenuItem { text: qsTr("Very fast") }
                }
                onCurrentIndexChanged: game.speed = currentIndex
            }

            TextSwitch {
                text: qsTr("Tap twice to play")
                description: qsTr("The first tap raises a card and the second plays it; tap anywhere else to lower it again.")
                checked: game.tapTwice
                onCheckedChanged: game.tapTwice = checked
            }

            SectionHeader { text: qsTr("Statistics") }

            Repeater {
                model: game.stats
                DetailItem {
                    label: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")][index]
                    value: qsTr("%1 won, %2 lost").arg(modelData.won).arg(modelData.lost)
                           + (modelData.bids > 0 ? " · " + qsTr("%1% bids made").arg(Math.round(100 * modelData.made / modelData.bids)) : "")
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reset statistics")
                onClicked: remorse.execute(qsTr("Resetting statistics"), function() { game.resetStats() })
            }
        }
        VerticalScrollDecorator { }
    }
}
