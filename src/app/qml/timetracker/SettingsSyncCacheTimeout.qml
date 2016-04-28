import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import TimeLog 1.0

Page {
    id: dialog

    function close() {
        TimeTracker.backRequested()
    }

    title: qsTranslate("settings", "Sync timeout")
    visible: false

    header: ToolBarMaterial {
        title: dialog.title

        onLeftActivated: dialog.close()
    }

    Flickable {
        anchors.bottomMargin: Qt.inputMethod.keyboardRectangle.height / Screen.devicePixelRatio
        anchors.fill: parent
        contentWidth: settingsItem.width
        contentHeight: settingsItem.height
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar { }

        Item {
            id: settingsItem

            width: dialog.width
            implicitHeight: container.height + container.anchors.margins * 2

            Column {
                id: container

                anchors.margins: 16
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 16

                SpinBox {
                    anchors.horizontalCenter: parent.horizontalCenter
                    to: 24 * 3600
                    editable: true
                    value: Settings.syncCacheTimeout
                    onValueChanged: Settings.syncCacheTimeout = value
                }

                Label {
                    width: parent.width
                    color: Material.hintTextColor
                    wrapMode: Text.Wrap
                    text: qsTranslate("settings", "With this setting you can control, how much time"
                                      + " (in seconds) application would wait after last record"
                                      + " before the sync.\n\n"
                                      + "To disable sync by timeout, set it to \u20180\u2019.")
                }
            }
        }
    }
}
