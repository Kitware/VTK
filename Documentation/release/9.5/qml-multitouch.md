# Multi-touch gestures in QtQuick/QML

Added support multi-touch interaction to `QQuickVTKItem` using the
[PinchHandler](https://doc.qt.io/qt-6/qml-qtquick-pinchhandler.html) QML component. To attach the
handler to a QQuickVTKItem, simply connect the new slots to the
`[translation|rotation|scale]Changed` signals of the handler.

## Example usage

```qml

  QVTKItem{
    id: qvtkitem
    anchors.fill: parent
  }

  PinchHandler {
    id: pch
    target: null
    onTranslationChanged: (delta) => qvtkitem.pinchHandlerTranslate(pch.centroid.position, delta)
    onScaleChanged: (delta) => qvtkitem.pinchHandlerScale(pch.centroid.position, delta)
    onRotationChanged: (delta) => qvtkitem.pinchHandlerRotate(pch.centroid.position, delta)
  }

```
