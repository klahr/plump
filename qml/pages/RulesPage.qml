import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Plump") }

            Repeater {
                model: [
                    { title: qsTr("The game"), text: qsTr("Three to five players, each for themselves, try to say exactly how many tricks they will take. A game is nineteen deals: ten cards each in the first, one fewer in each deal down to one card, and then one more each deal back up to ten. The highest score after the last deal wins.") },
                    { title: qsTr("Trumps"), text: qsTr("After the deal, the next card of the pack is turned up and its suit is trumps for the deal.") },
                    { title: qsTr("The bidding"), text: qsTr("Starting on the dealer's left, each player says how many tricks they will take, from none to all of them. The dealer bids last and may not make the bids add up to the number of tricks in the deal, so someone always misses.") },
                    { title: qsTr("The play"), text: qsTr("The player on the dealer's left leads to the first trick. Players must follow suit if they can; if not, they may play any card, a trump or not. The highest trump takes the trick, or if there is none, the highest card of the suit led, aces high. The taker leads the next.") },
                    { title: qsTr("Scoring"), text: qsTr("Taking exactly the tricks bid scores ten points a trick, and a bid of none that takes none scores ten. Taking more or fewer than the bid scores nothing.") },
                    { title: qsTr("Playing here"), text: qsTr("You sit at the bottom. To bid, tap a number; to play, tap a card, and the cards you may not play are dimmed. The score board shows each player's tricks taken of their bid, and the line above your hand shows trumps and the bids added up. Hold the table to see the last trick again.") }
                ]
                Column {
                    width: column.width
                    SectionHeader { text: modelData.title }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        text: modelData.text
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }

            SectionHeader { text: qsTr("About") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: qsTr("Plump %1 for Sailfish OS.").arg(Qt.application.version)
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        VerticalScrollDecorator { }
    }
}
