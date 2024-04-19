import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
  id: root
  objectName: "DynamicSplitView_root"

  property Component itemDelegate: null

  component ItemDelegate : Item {
    objectName: "<constructing ItemDelegate>"

    property int myIID: 0
    property var mySplitBase: this
    property var mySplitView: SplitView.view

    anchors.fill: mySplitView ? null : parent

    SplitView.preferredWidth: mySplitView
                  ? mySplitView.orientation === Qt.Horizontal
                    ? mySplitView.width / mySplitView.count
                    : mySplitView.width
                  : 0

    SplitView.preferredHeight: mySplitView
                  ? mySplitView.orientation === Qt.Vertical
                    ? mySplitView.height / mySplitView.count
                    : mySplitView.height
                  : 0

    function split(orientation) {
      if (mySplitView)
        mySplitView.split(this, orientation)
      else
        console.warn("YIKES!! DynamicSplitView.ItemDelegate.split() called when this item is NOT inside a QML SplitView")
    }

    function unsplit() {
      if (mySplitView)
        mySplitView.unsplit(this)
      else
        console.warn("YIKES!! DynamicSplitView.ItemDelegate.unsplit() called when this item is NOT inside a QML SplitView")
    }

    Component.onDestruction: {
      console.log("destructed", this, mySplitView)
    }
  }

  QtObject {
    id: d

    property int splitItemCount: 0
    property int splitViewCount: 0
    property var topSplitView: null
    property Component splitViewComponent: ItemDelegate
    {
      SplitView {
        anchors.fill: parent

        objectName: "<constructing SplitView>"

        function __indexOf(item)
        {
          // Find the item's index
          for (var index=0; index<count; ++index)
            if (itemAt(index) === item)
              break;

          // Check for errors
          if (index >= count) {
            console.warn("YIKES!! DynamicSplitView can't find ", item, "in it's container", contentChildren)
            index = -1
          }

          return index
        }

        function split(item, orientation)
        {
          // Do we only have 1 child?
          if (count === 1)
          {
            // Yes, switch our orientation to the one requested
            this.orientation = orientation
          }

          // Are we adding a child in our current orientation?
          if (orientation === this.orientation)
          {
            // Yes, Create and configure a new child
            var newChildBase = d.createItem()
            if (newChildBase)
            {
              // Add it to our splitter items
              addItem(newChildBase)
            }
          }

          // Otherwise, we're splitting a child with a different orientation
          else
          {
            // Find the item's index
            var index = __indexOf(item)
            if (index >= 0)
            {
              // Create a new SplitView
              var newViewBase = d.createSplitView(null)
              var newSplitter = newViewBase.children[0]

              // Replace the item with the new one
              item = takeItem(index)
              insertItem(index, newViewBase)

              // Add the item to this new SplitView
              newSplitter.addItem(item)

              // Split the new SplitView to finish
              newSplitter.split(null, orientation)
            }
          }
        }

        function unsplit(item)
        {
          // Find the item's index
          var index = __indexOf(item)
          if (index >= 0)
          {
            // Can we delete the item?
            if (count > 1 || mySplitView)
            {
              // Remove and destroy the item
              takeItem(index).destroy()

              // Clean degenerate trees
              d.topSplitView.__cleanDegenerateTree()
            }
            else
            {
              console.log("DynamicSplitView, can't delete last item in top-most SplitView")
            }
          }
        }

        function __cleanDegenerateTree(index)
        {
          // Traverse the tree depth-first
          for(var i = 0; i<count; ++i)
            if (itemAt(i).objectName.startsWith("viewBase"))
              itemAt(i).children[0].__cleanDegenerateTree(i)

          // Okay, working bottom up ... If I've only a single child item, replace myself in my parent with my child
          if (mySplitView && count === 1)
          {
            // Remove myself from my parent
            mySplitView.takeItem(index)

            // Move my child to my parent
            mySplitView.insertItem(index, takeItem(0))

            // destroy myself (safe because this is last call in a tail-recursion)
            mySplitBase.destroy()
          }
        }
      }
    }

    property Component undefinedItemDelegate: ItemDelegate {
      Text {
        anchors.centerIn: parent
        text: "You forgot to define the\nDynamicSplitView.itemDelegate\nproperty"
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 24
      }
    }

    property Component notDerivedItemDelegate: ItemDelegate {
      Text {
        anchors.centerIn: parent
        text: "Your itemDelegate must derive from\nDynamicSplitView.ItemDelegate"
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: 24
      }
    }

    function createSplitView(parent)
    {
      var item = splitViewComponent.createObject(parent)
      item.myIID = splitViewCount++
      item.objectName = "viewBase " + item.myIID
      item.children[0].objectName = "splitView " + item.myIID
      console.log("constructed", item, item.children[0])

      return item;
    }

    function createItem()
    {
      if (!root.itemDelegate)
      {
        console.warn("YIKES! You forgot to define the itemDelegate property.");
        root.itemDelegate = undefinedItemDelegate
      }

      var item = root.itemDelegate.createObject(null)
      if (item.objectName !== "<constructing ItemDelegate>")
      {
        console.warn("YIKES! Your itemDelegate must derive from DynamicSplitView.ItemDelegate")
        item.destroy()
        item = notDerivedItemDelegate.createObject(null)
      }
      else
      {
        item.myIID = splitItemCount++
        item.objectName = "itemBase " + item.myIID
        console.log("constructed", item)
      }

      return item
    }

    Component.onCompleted: {
      // Create initial item
      topSplitView = createSplitView(root).children[0]
      topSplitView.addItem(createItem())
    }
  }
}
