## label-outer-charts-sharexy

It is now possible to share x and (or) y axis among other charts in a `vtkChartMatrix`.
The API exposes a higher level `LabelOuter(leftBottom, rightTop)` method to offer
a neat and tight layout of linked charts. Any pre-existing gutter space is compensated.

The API also exposes a few lower level methods to `Link(c1, c2)` or `Unlink(c1, c2)`
two charts in a `vtkChartMatrix`.

One could group charts and either create a vertical/horizontal stack panel
or simply create a rectangular panel in which each chart is linked to share
x, y axes with every other chart.
