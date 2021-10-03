// import related modules
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

// import the VTK module
import VTK @VTK_MAJOR_VERSION@.@VTK_MINOR_VERSION@

// window containing the application
ApplicationWindow {
  // title of the application
  title: qsTr("VTK QtQuick App")
  width: 800
  height: 600
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
  // Create a VTK render window to represent the opengl context for all vtk items in this app
  VTKRenderWindow {
    id: vtkwindow
    anchors.fill: parent
  }

  SplitView {
    anchors.fill: parent
    orientation: Qt.Horizontal

    VTKRenderItem {
      objectName: "VolumeView"
      SplitView.fillHeight: true
      SplitView.fillWidth: true
      SplitView.minimumHeight: 100
      SplitView.minimumWidth: 100
      SplitView.preferredHeight: 200
      SplitView.preferredWidth: 200
      renderWindow: vtkwindow
    }

    ColumnLayout {
      SplitView.fillHeight: true
      SplitView.fillWidth: true
      SplitView.minimumWidth: 200
      SplitView.preferredWidth: 200
      VTKRenderItem {
        objectName: "GlyphView"
        renderWindow: vtkwindow
        focus: true
        Layout.fillHeight: true
        Layout.fillWidth: true
      }
      VTKRenderItem {
        objectName: "GeomView"
        renderWindow: vtkwindow
        Layout.fillHeight: true
        Layout.fillWidth: true
      }
    }
  }
}
