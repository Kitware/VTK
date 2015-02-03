#!/usr/bin/env python
from vtk import *

csv_source = vtkDelimitedTextReader()
csv_source.SetFieldDelimiterCharacters(",")
csv_source.SetHaveHeaders(True)
csv_source.SetDetectNumericColumns(True)
csv_source.SetFileName("authors.csv")
csv_source.Update()

T = csv_source.GetOutput()

print "Table loaded from CSV file:"
T.Dump(10)
