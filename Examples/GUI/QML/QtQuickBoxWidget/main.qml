import QtQuick 2.9
import QtQuick.Window 2.2
import com.vtk.example 1.0

Window {
  id: win
  visible: true
  width: 640
  height: 640
  title: qsTr("Hello World")

  Rectangle {
    anchors.fill: parent
    color: "darkseagreen"
  }

  Rectangle {
    id: r
    border { width: 5; color: "steelblue" }
    radius: 5
    color: "pink"
    anchors.fill: parent
    anchors.margins: 100

    MyVtkItem {
      id: vtk
      anchors.fill: parent
      anchors.margins: 5

      transform: Rotation{
        angle: 33
        origin.x: vtk.width/2
        origin.y: vtk.height/2
      }
    }
  }
}
