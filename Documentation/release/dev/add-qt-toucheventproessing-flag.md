## Add EnableTouchEventProcessing flag to QVTKOpenGL*Widgets & QVTKOpenGLWindow

As Qt touch event will automatically be translated to mouse event, so the mouse event will be processed twice for one touch in VTK interactor.
So for some native vtk widgets, such as vtkDistanceWidget, it designed behavior is broken on Qt6 platform. (By wrong use of QTouchEvent::TouchPoint::id(), the touch event is not processed accidentally with Qt5).
With this new flag for QVTKOpenGL*Widget/QVTKOpenGLWindow, you can switch on/off the Qt touch event processing by purpose.
