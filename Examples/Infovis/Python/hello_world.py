#!/usr/bin/env python

from vtk import *

source = vtkRandomGraphSource()

view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()
view.GetInteractor().Start()


