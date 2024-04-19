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
  height: 600

  RowLayout {
    anchors.fill: parent

    MyVolumeItem {
      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.minimumHeight: 200
      Layout.minimumWidth: 200
    }

    ColumnLayout {
      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.minimumWidth: 200
      Layout.preferredWidth: 200

      MyGlyphItem {
        focus: true
        Layout.fillHeight: true
        Layout.fillWidth: true
      }
      MyGeomItem {
        Layout.fillHeight: true
        Layout.fillWidth: true
      }
    }
  }
}
