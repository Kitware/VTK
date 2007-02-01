#!/usr/bin/env python

# This example demonstrates the use of vtkLabeledDataMapper.  This
# class is used for displaying numerical data from an underlying data
# set.  In the case of this example, the underlying data are the point
# and cell ids.

import vtk

# Create a selection window.  We will display the point and cell ids
# that lie within this window.
xmin = 200
xLength = 100
xmax = xmin + xLength
ymin = 200
yLength = 100
ymax = ymin + yLength

pts = vtk.vtkPoints()
pts.InsertPoint(0, xmin, ymin, 0)
pts.InsertPoint(1, xmax, ymin, 0)
pts.InsertPoint(2, xmax, ymax, 0)
pts.InsertPoint(3, xmin, ymax, 0)
rect = vtk.vtkCellArray()
rect.InsertNextCell(5)
rect.InsertCellPoint(0)
rect.InsertCellPoint(1)
rect.InsertCellPoint(2)
rect.InsertCellPoint(3)
rect.InsertCellPoint(0)
selectRect = vtk.vtkPolyData()
selectRect.SetPoints(pts)
selectRect.SetLines(rect)
rectMapper = vtk.vtkPolyDataMapper2D()
rectMapper.SetInput(selectRect)
rectActor = vtk.vtkActor2D()
rectActor.SetMapper(rectMapper)

# Create a sphere and its associated mapper and actor.
sphere = vtk.vtkSphereSource()
sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereMapper.GlobalImmediateModeRenderingOn()
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)

# Generate data arrays containing point and cell ids
ids = vtk.vtkIdFilter()
ids.SetInputConnection(sphere.GetOutputPort())
ids.PointIdsOn()
ids.CellIdsOn()
ids.FieldDataOn()

# Create the renderer here because vtkSelectVisiblePoints needs it.
ren = vtk.vtkRenderer()

# Create labels for points
visPts = vtk.vtkSelectVisiblePoints()
visPts.SetInputConnection(ids.GetOutputPort())
visPts.SetRenderer(ren)
visPts.SelectionWindowOn()
visPts.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

# Create the mapper to display the point ids.  Specify the format to
# use for the labels.  Also create the associated actor.
ldm = vtk.vtkLabeledDataMapper()
# ldm.SetLabelFormat("%g")
ldm.SetInputConnection(visPts.GetOutputPort())
ldm.SetLabelModeToLabelFieldData()
pointLabels = vtk.vtkActor2D()
pointLabels.SetMapper(ldm)

# Create labels for cells
cc = vtk.vtkCellCenters()
cc.SetInputConnection(ids.GetOutputPort())
visCells = vtk.vtkSelectVisiblePoints()
visCells.SetInputConnection(cc.GetOutputPort())
visCells.SetRenderer(ren)
visCells.SelectionWindowOn()
visCells.SetSelection(xmin, xmin + xLength, ymin, ymin + yLength)

# Create the mapper to display the cell ids.  Specify the format to
# use for the labels.  Also create the associated actor.
cellMapper = vtk.vtkLabeledDataMapper()
cellMapper.SetInputConnection(visCells.GetOutputPort())
# cellMapper.SetLabelFormat("%g")
cellMapper.SetLabelModeToLabelFieldData()
cellMapper.GetLabelTextProperty().SetColor(0, 1, 0)
cellLabels = vtk.vtkActor2D()
cellLabels.SetMapper(cellMapper)

# Create the RenderWindow and RenderWindowInteractor
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer; set the background and size;
# render
ren.AddActor(sphereActor)
ren.AddActor2D(rectActor)
ren.AddActor2D(pointLabels)
ren.AddActor2D(cellLabels)

ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)

# Create a function to move the selection window across the data set.
def MoveWindow():
    for y in range(100, 300, 25):
        for x in range(100, 300, 25):
            PlaceWindow(x, y) 


# Create a function to draw the selection window at each location it
# is moved to.
def PlaceWindow(xmin, ymin):
    global xLength, yLength, visPts, visCells, pts, renWin

    xmax = xmin + xLength
    ymax = ymin + yLength

    visPts.SetSelection(xmin, xmax, ymin, ymax)
    visCells.SetSelection(xmin, xmax, ymin, ymax)

    pts.InsertPoint(0, xmin, ymin, 0)
    pts.InsertPoint(1, xmax, ymin, 0)
    pts.InsertPoint(2, xmax, ymax, 0)
    pts.InsertPoint(3, xmin, ymax, 0)
    # Call Modified because InsertPoints does not modify vtkPoints
    # (for performance reasons)
    pts.Modified()
    renWin.Render()
 

# Initialize the interactor.
iren.Initialize()
renWin.Render()

# Move the selection window across the data set.
MoveWindow()

# Put the selection window in the center of the render window.
# This works because the xmin = ymin = 200, xLength = yLength = 100, and
# the render window size is 500 x 500.
PlaceWindow(xmin, ymin)

# Now start normal interaction.
iren.Start()
