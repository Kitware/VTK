import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import com.vtk.example 1.0

ApplicationWindow {
  visible: true
  width: 640
  height: 640
  title: qsTr("Hello World")

  header: ToolBar {
    id: header
    RowLayout {
      anchors.fill: parent
      RowLayout {
        spacing: 1
        Button {
          id: btn1
          text: "Split Horizontal"
          rightPadding: 8
          onClicked: dsv.focused.split(Qt.Horizontal)
        }
        Button {
          id: btn2
          text: "Split Vertical"
          onClicked: dsv.focused.split(Qt.Vertical)
        }
        Button {
          id: btn3
          text: "UnSplit"
          onClicked: dsv.focused.unsplit(), dsv.pop()
        }
        Component.onCompleted: {
          let width = Math.max(btn1.width, btn2.width, btn3.width)
          btn1.Layout.preferredWidth = width
          btn2.Layout.preferredWidth = width
          btn3.Layout.preferredWidth = width
        }
      }
      Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }
      Rectangle {
        color: "black"
        Layout.preferredWidth: 1
        Layout.fillHeight: true
      }
      Text {
        Layout.leftMargin: 10
        text: "vtkSource:"
      }
      ComboBox {
        id: sources
        Layout.fillHeight: true
        Layout.preferredWidth: childrenRect.width
        model: presenter.sources
      }
    }
  }

  DynamicSplitView {
    id: dsv
    anchors.fill: parent

    property var focused: null
    property var stack: []
    function push(item) {
      stack.push(item)
      focused = item
    }
    function pop() {
      if (stack.length > 1) {

        // remove all copies of the item at the top of the stack
        stack = stack.filter(function(f) { return f !== focused})

        // remove all duplicate runs of identical items in the stack
        stack = stack.filter(function(item, pos, ary) { return !pos || item !== ary[pos - 1];});

        // the new focused element is now at the top of the stack
        focused = stack.length>0 ? stack[stack.length -1]: null
      }
    }
    itemDelegate: DynamicSplitView.ItemDelegate
    {
      id: item

      Rectangle {
        border {
          id: border
          width: 5;
          color: dsv.focused === item ? "goldenrod" : "steelblue"
        }
        radius: 5
        color: "magenta"
        anchors.fill: parent

        MyVtkItem {
          anchors.fill: parent
          anchors.margins: border.width
          source: sources.currentText
          onClicked: {
            if (dsv.focused != item)
              dsv.push(item)
          }
        }
      }

      Component.onCompleted: {
        dsv.push(item)
      }
    }
  }
}
