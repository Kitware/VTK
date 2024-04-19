// import related modules
import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

// import the VTK module
import Vtk 1.0

// item containing the application
Item {
  // title of the application
  width: 800
  height: 800

  RowLayout {
    anchors.fill: parent

    MyConeItem {
      Layout.column: 0
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumWidth: 100
      Layout.preferredWidth: 200
      focus: true
    }
    MyWidgetItem {
      Layout.column: 1
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumWidth: 100
      Layout.preferredWidth: 400
      focus: true
    }
  }
}
