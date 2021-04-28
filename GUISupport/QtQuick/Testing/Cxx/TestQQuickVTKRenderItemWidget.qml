// import related modules
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

// import the VTK module
import VTK 9.0

// window containing the application
ApplicationWindow {
  // title of the application
  title: qsTr("VTK QtQuick App")
  width: 800
  height: 800
  color: palette.window

  SystemPalette {
    id: palette
    colorGroup: SystemPalette.Active
  }

  // menubar with two menus
  menuBar: MenuBar {
    Menu {
      title: qsTr("File")
      MenuItem {
        text: qsTr("&Quit")
        onTriggered: Qt.quit()
      }
    }
    Menu {
      title: qsTr("Edit")
    }
  }

  // Content area

  // a rectangle in the middle of the content area
  Rectangle {
    width: 100
    height: 100
    color: "blue"
    border.color: "red"
    border.width: 5
    radius: 10
  }
  Text {
    id: label
    color: "white"
    wrapMode: Text.WordWrap
    text: "Custom QML\nrectangle &\ntext"
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.margins: 10
    width: 100
  }

  VTKRenderWindow {
    id: vtkwindow
    anchors.fill: parent
  }

  RowLayout {
    anchors.fill: parent

    VTKRenderItem {
      objectName: "ConeView"
      Layout.column: 0
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumWidth: 100
      Layout.preferredWidth: 200
      renderWindow: vtkwindow
      focus: true
    }
    VTKRenderItem {
      objectName: "WidgetView"
      Layout.column: 1
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumWidth: 100
      Layout.preferredWidth: 400
      renderWindow: vtkwindow
      focus: true
    }
  }
}
