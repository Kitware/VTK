catch {load vtktcl}
#
# Contour data stored as a matrix of values in a vtk structured points file
#
source ../../examplesTcl/vtkInt.tcl

# create pipeline
#
vtkStructuredPointsReader reader
  reader SetFileName "../../../vtkdata/matrix.vtk"

vtkContourFilter contour
  contour SetInput [reader GetOutput]
  contour SetValue 0 .5

vtkDataSetMapper contourMapper
  contourMapper SetInput [contour GetOutput]
  contourMapper ScalarVisibilityOff

vtkActor contourActor
  contourActor SetMapper contourMapper
  contourActor SetPosition 0 0 5

vtkStructuredPointsGeometryFilter toGeometry
  toGeometry SetInput [reader GetOutput]  

vtkWarpScalar carpet
  carpet SetInput [toGeometry GetOutput]
  carpet SetNormal 0 0 1
  carpet SetScaleFactor 3

vtkDataSetMapper carpetMapper
  carpetMapper SetInput [carpet GetOutput]
  carpetMapper ScalarVisibilityOff

vtkActor carpetActor
  carpetActor SetMapper carpetMapper

# Create graphics stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor contourActor
ren1 AddActor carpetActor


renWin SetSize 400 400
[ren1 GetActiveCamera] Dolly 1.5
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName "contourMatrix.tcl.ppm"
#renWin SaveImageAsPPM


