## Fix vtkQuadricDecimation PointData attributes

Fix regression from VTK v9.3. More context in issue #19384.
PointData attributes were incorrectly computed.
It is now fixed for attributes that count in the metric.
It is now improved for the other attributes but not 100% correct.
