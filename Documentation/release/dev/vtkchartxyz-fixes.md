# vtkChartXYZ fixes

Some longstanding issues with vtkChartXYZ have been fixed.
- A wrong order of matrix-transformations caused misalignment between plot and axes (see bug 17542)
- The plot can now resize dynamically with the scene by setting 4 margins,
  before this users had to call SetGeometry at every resize.
- The rescaling-algorithm won't produce large margins anymore: your screen real estate will be used better.
- Plots can be removed.
- In the past when a user wanted to zoom into some detail he would find that his plot disappeared
  behind clipping planes. These planes can now be turned of.
- When zooming the user now has the option to let the axes zoom with the rest of the plot.

In short: vtkChartXYZ should be more enjoyable now.
