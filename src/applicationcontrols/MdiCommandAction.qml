import QtQuick 2.0
import QtQuick.Controls 1.2
import Machinekit.Application 1.0

Action {
    property var status: {"synced": false}
    property var command: {"connected": false}
    property string mdiCommand: ""

    property bool _ready: status.synced && command.connected
                          && (status.task.taskState === ApplicationStatus.TaskStateOn)

    id: root
    text: qsTr("Go")
    shortcut: ""
    tooltip: qsTr("Execute MDI command") + " [" + shortcut + "]"
    onTriggered: {
        if (status.task.taskMode !== ApplicationStatus.TaskModeMdi)
            command.setTaskMode('execute', ApplicationCommand.TaskModeMdi)
        command.executeMdi('execute', mdiCommand)
    }

    //checked: _ready && (status.task.taskState === ApplicationStatus.TaskStateEstop)
    enabled: _ready && !status.running
}