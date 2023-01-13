## vtkPolyLine: Clip function generates polylines whenever possible

vtkPolyLine's Clip function used to always generate lines. This was problematic because
if a filter was generating polylines, clipping them would result in a set of lines instead of polylines.
vtkPolyLine::Clip now generates polylines whenever possible.
