## QVTKOpenGLStereoWidget and QVTKOpenGLNativeWidget: add custom cursor methods

Add custom methods to set and get the cursor shape in QVTKOpenGLStereoWidget.
For instance, calling QWidget::setCursor directly has no effect since this call should be forwarded to the embedded window.
These methods are also added in QVTKOpenGLNativeWidget because they need to share the same interface, in a ParaView scope (see pqQVTKWidget).
