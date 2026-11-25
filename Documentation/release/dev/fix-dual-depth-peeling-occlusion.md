# Dual depth peeling preserves translucent layers over opaque geometry

`vtkDualDepthPeelingPass` now composites all visible translucent layers when
opaque geometry fills the view behind them. Previously, peeling could stop
after the frontmost layer, causing missing surfaces and flickering as the
camera moved. The initial translucent depth range now excludes opaque depths,
while volume rendering continues to use the opaque depth to clip its rays.
