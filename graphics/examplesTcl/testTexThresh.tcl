catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# construct simple pixmap with test scalars
#
vtkStructuredPoints plane
  plane SetDimensions 3 3 1
vtkFloatScalars scalars
  scalars InsertScalar 0 0.0
  scalars InsertScalar 1 1.0
  scalars InsertScalar 2 0.0
  scalars InsertScalar 3 1.0
  scalars InsertScalar 4 2.0
  scalars InsertScalar 5 1.0
  scalars InsertScalar 6 0.0
  scalars InsertScalar 7 1.0
  scalars InsertScalar 8 0.0
[plane GetPointData] SetScalars scalars

# read in texture map
#
vtkStructuredPointsReader tmap
  tmap SetFileName "../../../data/texThres.vtk"
vtkTexture texture
  texture SetInput [tmap GetOutput]
  texture InterpolateOff
  texture RepeatOff

# Cut data with texture
#
vtkStructuredPointsGeometryFilter planePolys
    planePolys SetInput plane
    planePolys SetExtent 0 3 0 3 0 0
vtkThresholdTextureCoords thresh
#    thresh SetInput plane
    thresh SetInput [planePolys GetOutput]
    thresh ThresholdByUpper 0.5
vtkDataSetMapper planeMap
    planeMap SetInput [thresh GetOutput]
    eval planeMap SetScalarRange 0 2
vtkActor planeActor
    planeActor SetMapper planeMap
    planeActor SetTexture texture

# we set the opacity to 0.999 to indicate that we are doing stuff with
# alpha. Ideally we shouldn't have to do this, but leaving the alpha
# funcs on all the time in OpenGL kills performance on some systems.
[planeActor GetProperty] SetOpacity 0.999

# Add the actors to the renderer, set the background and size
#
ren1 AddActor planeActor
ren1 SetBackground 0.5 0.5 0.5
renWin SetSize 450 450

iren Initialize
#renWin SetFileName "testTexThresh.tcl.ppm"
#renWin SaveImageAsPPM

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .








