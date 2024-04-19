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
    color: "hotpink"
    anchors.fill: parent
    anchors.margins: 100

    MyVtkItem {
      id: vtk
      anchors.fill: parent
      anchors.margins: 5
      opacity: 0.7

      transform: Rotation{
        angle: 0
        origin.x: vtk.width/2
        origin.y: vtk.height/2

         SequentialAnimation on angle {
          loops: Animation.Infinite
          PauseAnimation  { duration: 5000}
          PropertyAnimation { to: 360; duration: 5000}
          PauseAnimation  { duration: 5000}
          PropertyAnimation { to: 0; duration: 5000}
          PauseAnimation  { duration: 5000}
        }
      }
    }
  }

  Rectangle {
    anchors.leftMargin: parent.width/2
    anchors.topMargin: parent.height/2
    anchors.fill: parent

    border { width: 5; color: "gold" }
    radius: 11
    color: "peachpuff"
    opacity: 0.8
  }
}
