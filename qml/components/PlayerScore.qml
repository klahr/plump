import QtQuick 2.6
import Sailfish.Silica 1.0

// One player's place on the score board: name, total, a bar towards the
// leader's score and the tricks taken of the bid this deal.
Item {
    id: root

    property string name
    property int score
    // The highest score at the table.
    property int leader
    property int bid: -1
    property int taken
    property bool dealer
    property bool showBid: true
    // Their turn to bid or play.
    property bool active
    property color color: Theme.highlightColor

    height: column.height + Theme.paddingSmall

    property int _shown: 0
    onScoreChanged: {
        var gained = score - _shown
        _shown = score
        if (gained > 0) {
            gain.text = "+" + gained
            gainAnimation.restart()
        }
    }

    Rectangle {
        anchors { fill: parent; leftMargin: Theme.paddingSmall / 2; rightMargin: Theme.paddingSmall / 2 }
        radius: Theme.paddingSmall
        color: Theme.rgba(root.color, 0.18)
        opacity: root.active ? 1 : 0
        Behavior on opacity { FadeAnimation { } }
    }

    Column {
        id: column
        width: parent.width

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.name
            color: root.color
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }
        Label {
            id: scoreLabel
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.score
            color: root.color
            font.pixelSize: Theme.fontSizeExtraLarge
            font.family: Theme.fontFamilyHeading
        }

        // How close to the lead.
        Item {
            x: Theme.paddingMedium
            width: parent.width - 2 * Theme.paddingMedium
            height: Theme.paddingMedium

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: Math.max(2, Math.round(Theme.pixelRatio * 2))
                radius: height / 2
                color: Theme.rgba(Theme.primaryColor, 0.15)
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: root.leader > 0 ? parent.width * root.score / root.leader : 0
                height: Math.max(2, Math.round(Theme.pixelRatio * 2))
                radius: height / 2
                color: root.color
                Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            //: Tricks taken of the bid, on the score board
            text: !root.showBid ? " "
                  : root.bid >= 0 ? qsTr("%1 of %2").arg(root.taken).arg(root.bid)
                  : root.dealer ? qsTr("Deals") : " "
            color: root.bid >= 0 && root.taken > root.bid ? Theme.errorColor
                   : root.bid >= 0 && root.taken === root.bid ? root.color : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            truncationMode: TruncationMode.Fade
        }
    }

    Label {
        id: gain
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: scoreLabel.contentWidth / 2 + width / 2 + Theme.paddingSmall
        y: scoreLabel.y
        opacity: 0
        color: root.color
        font.pixelSize: Theme.fontSizeSmall
        font.bold: true
    }

    ParallelAnimation {
        id: gainAnimation
        NumberAnimation { target: gain; property: "opacity"; from: 1; to: 0; duration: 1400; easing.type: Easing.InQuad }
        NumberAnimation {
            target: gain; property: "y"
            from: scoreLabel.y; to: scoreLabel.y - Theme.paddingLarge
            duration: 1400; easing.type: Easing.OutQuad
        }
    }
}
