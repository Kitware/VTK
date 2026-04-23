Fix mesa drivers primitive-id shader bug

Mesa drivers 25.3.6 and later introduce a regression where
shader pipelines using `gl_PrimitiveID` break, and do not
generate desired rendering with vtkOpenGLPolyDataMapper.
This is a temporary work-around that will be undone at a
later date when mesa drivers fix this regression.

A bug report has been filed with Mesa3D, here:
https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15660
