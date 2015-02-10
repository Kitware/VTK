#!/usr/bin/env python
from PyQt4 import QtCore
from PyQt4 import QtGui
from PyQt4 import uic
from vtk import *
import sys

# Create a PyQt window using a .ui file generated with Qt Designer ...
application = QtGui.QApplication(sys.argv)

window = uic.loadUi("gui.ui")
vertex_slider = window.findChild(QtGui.QSlider, "vertexCount")
edge_slider = window.findChild(QtGui.QSlider, "edgeCount")

# Create a simple pipeline source ...
source = vtkRandomGraphSource()
source.SetNumberOfVertices(vertex_slider.value())
source.SetNumberOfEdges(edge_slider.value())
source.SetStartWithTree(True)

# Define event-handlers that update the pipeline in response to widget events ...
def change_vertex_count(count):
  source.SetNumberOfVertices(count)
  edge_slider.setValue(source.GetNumberOfEdges())
  view.GetRenderer().ResetCamera()
  render_window.Render()

def change_edge_count(count):
  source.SetNumberOfEdges(count)
  edge_slider.setValue(source.GetNumberOfEdges())
  view.GetRenderer().ResetCamera()
  render_window.Render()

# Connect the GUI widgets to the event-handlers ...
QtCore.QObject.connect(vertex_slider, QtCore.SIGNAL("valueChanged(int)"), change_vertex_count)
QtCore.QObject.connect(edge_slider, QtCore.SIGNAL("valueChanged(int)"), change_edge_count)
window.show()

# Setup a VTK view ...
view = vtkGraphLayoutView()
view.AddRepresentationFromInputConnection(source.GetOutputPort())

view.GetRenderWindow().SetSize(600, 600)

# This initializes the VTK window for interaction, but doesn't start an event-loop ...
view.GetRenderWindow().Start()

# Start the Qt event-loop ...
application.exec_()

