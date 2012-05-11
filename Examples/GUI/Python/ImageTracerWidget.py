#!/usr/bin/env python

# initial translation from the tcl by VTK/Utilities/tcl2py.py
# further cleanup and fixes to the translation by Charl P. Botha

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This example demonstrates how to use the vtkImageTracerWidget
# to trace on a slice of a 3D image dataset on one of its orthogonal planes.
# The button actions and key modifiers are as follows for controlling the
# widget:
# 1) left button click over the image, hold and drag draws a free hand line.
# 2) left button click and release erases the widget line, if it exists, and
# repositions the handle.
# 3) middle button click starts a snap line.  The snap line can be
# terminated by clicking the middle button while depressing the ctrl key.
# 4) when tracing or snap drawing a line, if the last cursor position is
# within specified tolerance to the first handle, the widget line will form
# a closed loop with only one handle.
# 5) right button clicking and holding on any handle that is part of a snap
# line allows handle dragging.  Any existing line segments are updated
# accordingly.
# 6) ctrl key + right button down on any handle will erase it. Any existing
# snap line segments are updated accordingly.  If the line was formed by
# continous tracing, the line is deleted leaving one handle.
# 7) shift key + right button down on any snap line segment will insert a
# handle at the cursor position.  The snap line segment is split accordingly.
#
#

def AdjustSpline(evt, obj):
    itw.GetPath(poly)
    npts = itw.GetNumberOfHandles()

    if npts < 2:
        imageActor2.GetMapper().SetInputConnection(extract.GetOutputPort())
        return

    closed = itw.IsClosed()

    if closed:
        isw.ClosedOn()
    else:
        isw.ClosedOff()
        imageActor2.GetMapper().SetInputConnection(extract.GetOutputPort())

    isw.SetNumberOfHandles(npts)

    for i in range(0, npts):
        pt = poly.GetPoints().GetPoint(i)
        isw.SetHandlePosition(i, pt[0], pt[1], pt[2])

    if closed:
        isw.GetPolyData(spoly)
        imageActor2.GetMapper().SetInputConnection(stencil.GetOutputPort())
        stencil.Update()

def AdjustTracer(evt, obj):
    npts = isw.GetNumberOfHandles()
    points.SetNumberOfPoints(npts)

    for i in range(0, npts):
        pt = isw.GetHandlePosition(i)
        points.SetPoint(i, pt[0], pt[1], pt[2])

    closed = isw.GetClosed()

    if closed:
        isw.GetPolyData(spoly)
        imageActor2.GetMapper().SetInputConnection(stencil.GetOutputPort())
        stencil.Update()

    itw.InitializeHandles(points)


# Start by loading some data.
v16 = vtk.vtkVolume16Reader()
v16.SetDataDimensions(64, 64)
v16.SetDataByteOrderToLittleEndian()
v16.SetImageRange(1, 93)
v16.SetDataSpacing(3.2, 3.2, 1.5)
v16.SetFilePrefix("%s/Data/headsq/quarter" % (VTK_DATA_ROOT,))
v16.Update()
#

srange = v16.GetOutput().GetScalarRange()
min = srange[0]
max = srange[1]

diff = max-min
slope = 255.0/diff
inter = -slope*min
shift = inter/slope

shifter = vtk.vtkImageShiftScale()
shifter.SetShift(shift)
shifter.SetScale(slope)
shifter.SetOutputScalarTypeToUnsignedChar()
shifter.SetInputConnection(v16.GetOutputPort())
shifter.ReleaseDataFlagOff()
shifter.Update()

# Display a y-z plane.
#
imageActor = vtk.vtkImageActor()
imageActor.GetMapper().SetInputConnection(shifter.GetOutputPort())
imageActor.VisibilityOn()
imageActor.SetDisplayExtent(31, 31, 0, 63, 0, 92)
imageActor.InterpolateOff()
#

spc = shifter.GetOutput().GetSpacing()
orig = shifter.GetOutput().GetOrigin()
x0 = orig[0]
xspc = spc[0]
pos = x0+xspc*31.0

# An alternative would be to formulate position in this case by:
# set bounds [imageActor GetBounds]
# set pos [lindex $bounds 0]
#
#

ren = vtk.vtkRenderer()
ren.SetBackground(0.4, 0.4, 0.5)
ren2 = vtk.vtkRenderer()
ren2.SetBackground(0.5, 0.4, 0.4)
#

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.AddRenderer(ren2)
renWin.SetSize(600, 300)
#

ren.SetViewport(0, 0, 0.5, 1)
ren2.SetViewport(0.5, 0, 1, 1)
#

interactor = vtk.vtkInteractorStyleImage()
#

iren = vtk.vtkRenderWindowInteractor()
iren.SetInteractorStyle(interactor)
iren.SetRenderWindow(renWin)
#

extract = vtk.vtkExtractVOI()
extract.SetVOI(31, 31, 0, 63, 0, 92)
extract.SetSampleRate(1, 1, 1)
extract.SetInputConnection(shifter.GetOutputPort())
extract.ReleaseDataFlagOff()
#

imageActor2 = vtk.vtkImageActor()
imageActor2.GetMapper().SetInputConnection(extract.GetOutputPort())
imageActor2.VisibilityOn()
imageActor2.SetDisplayExtent(31, 31, 0, 63, 0, 92)
imageActor2.InterpolateOff()
#

# Set up the image tracer widget
#
itw = vtk.vtkImageTracerWidget()
#
# Set the tolerance for capturing last handle when near first handle
# to form closed paths.
#
itw.SetCaptureRadius(1.5)
itw.GetGlyphSource().SetColor(1, 0, 0)
#
# Set the size of the glyph handle
#
itw.GetGlyphSource().SetScale(3.0)
#
# Set the initial rotation of the glyph if desired.  The default glyph
# set internally by the widget is a '+' so rotating 45 deg. gives a 'x'
#
itw.GetGlyphSource().SetRotationAngle(45.0)
itw.GetGlyphSource().Modified()
itw.ProjectToPlaneOn()
itw.SetProjectionNormalToXAxes()
itw.SetProjectionPosition(pos)
itw.SetViewProp(imageActor)
itw.SetInputConnection(shifter.GetOutputPort())
itw.SetInteractor(iren)
itw.PlaceWidget()

#
# When the underlying vtkDataSet is a vtkImageData, the widget can be
# forced to snap to either nearest pixel points, or pixel centers.  Here
# it is turned off.
#
itw.SnapToImageOff()

#
# Automatically form closed paths.
#
#itw AutoCloseOn
itw.AutoCloseOn()
#

# Set up a vtkSplineWidget in the second renderer and have
# its handles set by the tracer widget.
#
isw = vtk.vtkSplineWidget()
isw.SetCurrentRenderer(ren2)
isw.SetDefaultRenderer(ren2)
isw.SetInputConnection(extract.GetOutputPort())
isw.SetInteractor(iren)
bnds = imageActor2.GetBounds()
isw.PlaceWidget(bnds[0], bnds[1], bnds[2], bnds[3], bnds[4], bnds[5])
isw.ProjectToPlaneOn()
isw.SetProjectionNormalToXAxes()
isw.SetProjectionPosition(pos)
#

# Have the widgets control each others handle positions.
#
itw.AddObserver('EndInteractionEvent',AdjustSpline)
isw.AddObserver('EndInteractionEvent',AdjustTracer)
#

itw.On()
isw.On()
#

poly = vtk.vtkPolyData()
points = vtk.vtkPoints()
spoly = vtk.vtkPolyData()
#

# Set up a pipleline to demonstrate extraction of a 2D
# region of interest.  Defining a closed clockwise path using the
# tracer widget will extract all pixels within the loop.  A counter
# clockwise path provides the dual region of interest.
#
extrude = vtk.vtkLinearExtrusionFilter()
extrude.SetInputData(spoly)
extrude.SetScaleFactor(1)
extrude.SetExtrusionTypeToNormalExtrusion()
extrude.SetVector(1, 0, 0)
#

dataToStencil = vtk.vtkPolyDataToImageStencil()
dataToStencil.SetInputConnection(extrude.GetOutputPort())
#

stencil = vtk.vtkImageStencil()
stencil.SetInputConnection(extract.GetOutputPort())
stencil.SetStencilConnection(dataToStencil.GetOutputPort())
stencil.ReverseStencilOff()
stencil.SetBackgroundValue(128)
#

# Add all the actors.
#
ren.AddViewProp(imageActor)
ren2.AddViewProp(imageActor2)
#

# Render the image.
#
renWin.Render()
#

ren.GetActiveCamera().SetViewUp(0, 1, 0)
ren.GetActiveCamera().Azimuth(270)
ren.GetActiveCamera().Roll(270)
ren.GetActiveCamera().Dolly(1.7)
ren.ResetCameraClippingRange()
#

ren2.GetActiveCamera().SetViewUp(0, 1, 0)
ren2.GetActiveCamera().Azimuth(270)
ren2.GetActiveCamera().Roll(270)
ren2.GetActiveCamera().Dolly(1.7)
ren2.ResetCameraClippingRange()
#

# if we don't do this, the widgets disappear behind the imageActor.
vtk.vtkMapper.SetResolveCoincidentTopologyToPolygonOffset()
vtk.vtkMapper.SetResolveCoincidentTopologyPolygonOffsetParameters(10,10)


renWin.Render()
#

iren.Initialize()
renWin.Render()
iren.Start()
