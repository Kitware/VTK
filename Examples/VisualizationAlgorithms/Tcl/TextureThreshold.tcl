# Load libraries for VTK
package require vtk
package require vtkinteraction
package require vtktesting

# This example shows how to use a transparent texture map to perform
# thresholding. The key is the vtkThresholdTextureCoords filter which
# creates texture coordinates based on a threshold value. These texture
# coordinates are used in conjuntion with a texture map with varying
# opacity and intensity to create an inside, transition, and outside
# region.
#

# Begin by reading some structure grid data.
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/bluntfinxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/bluntfinq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

# Now extract surfaces from the grid corresponding to boundary geometry.
# First the wall.
vtkStructuredGridGeometryFilter wall
    wall SetInput [pl3d GetOutput]
    wall SetExtent 0 100 0 0 0 100
vtkPolyDataMapper wallMap
    wallMap SetInput [wall GetOutput]
    wallMap ScalarVisibilityOff
vtkActor wallActor
    wallActor SetMapper wallMap
    eval [wallActor GetProperty] SetColor 0.8 0.8 0.8

# Now the fin.
# 
vtkStructuredGridGeometryFilter fin
    fin SetInput [pl3d GetOutput]
    fin SetExtent 0 100 0 100 0 0
vtkPolyDataMapper finMap
    finMap SetInput [fin GetOutput]
    finMap ScalarVisibilityOff
vtkActor finActor
    finActor SetMapper finMap
    eval [finActor GetProperty] SetColor 0.8 0.8 0.8

# Extract planes to threshold. Start by reading the specially
# designed texture map that has three regions: an inside, boundary,
# and outside region. The opacity and intensity of this texture map
# are varied.
vtkStructuredPointsReader tmap
  tmap SetFileName "$VTK_DATA_ROOT/Data/texThres2.vtk"
vtkTexture texture
  texture SetInput [tmap GetOutput]
  texture InterpolateOff
  texture RepeatOff

# Here are the three planes which will be texture thresholded.
#
vtkStructuredGridGeometryFilter plane1
    plane1 SetInput [pl3d GetOutput]
    plane1 SetExtent 10 10 0 100 0 100
vtkThresholdTextureCoords thresh1
    thresh1 SetInput [plane1 GetOutput]
    thresh1 ThresholdByUpper 1.5
vtkDataSetMapper plane1Map
    plane1Map SetInput [thresh1 GetOutput]
    eval plane1Map SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor plane1Actor
    plane1Actor SetMapper plane1Map
    plane1Actor SetTexture texture
[plane1Actor GetProperty] SetOpacity 0.999

vtkStructuredGridGeometryFilter plane2
    plane2 SetInput [pl3d GetOutput]
    plane2 SetExtent 30 30 0 100 0 100
vtkThresholdTextureCoords thresh2
    thresh2 SetInput [plane2 GetOutput]
    thresh2 ThresholdByUpper 1.5
vtkDataSetMapper plane2Map
    plane2Map SetInput [thresh2 GetOutput]
    eval plane2Map SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor plane2Actor
    plane2Actor SetMapper plane2Map
    plane2Actor SetTexture texture
    [plane2Actor GetProperty] SetOpacity 0.999

vtkStructuredGridGeometryFilter plane3
    plane3 SetInput [pl3d GetOutput]
    plane3 SetExtent 35 35 0 100 0 100
vtkThresholdTextureCoords thresh3
    thresh3 SetInput [plane3 GetOutput]
    thresh3 ThresholdByUpper 1.5
vtkDataSetMapper plane3Map
    plane3Map SetInput [thresh3 GetOutput]
    eval plane3Map SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor plane3Actor
    plane3Actor SetMapper plane3Map
    plane3Actor SetTexture texture
    [plane3Actor GetProperty] SetOpacity 0.999

# For context create an outline around the data.
#
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor 0 0 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor wallActor
ren1 AddActor finActor
ren1 AddActor plane1Actor
ren1 AddActor plane2Actor
ren1 AddActor plane3Actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500

# Set up a nice view.
#
vtkCamera cam1
  cam1 SetClippingRange 1.51176 75.5879
  cam1 SetFocalPoint 2.33749 2.96739 3.61023
  cam1 SetPosition 10.8787 5.27346 15.8687
  cam1 SetViewAngle 30
  cam1 SetViewPlaneNormal 0.564986 0.152542 0.810877
  cam1 SetViewUp -0.0610856 0.987798 -0.143262
ren1 SetActiveCamera cam1

iren Initialize

# Set up an observer to invoke a proc when the UserEvent (keypress-u)
# is invoked.
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



