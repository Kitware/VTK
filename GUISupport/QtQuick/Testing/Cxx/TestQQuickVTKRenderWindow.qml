// import related modules
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

// window containing the application
ApplicationWindow {
  // title of the application
  title: qsTr("VTK QtQuick App")
  width: 400
  height: 400

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
}
