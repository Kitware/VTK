package require vtk
package require vtkinteraction

# create reader and extract the velocity and temperature
vtkPNGReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/vtk.png"

vtkImageQuantizeRGBToIndex quant
    quant SetInput [reader GetOutput]
    quant SetNumberOfColors 32

vtkImageToPolyDataFilter i2pd
    i2pd SetInput [quant GetOutput]
    i2pd SetLookupTable [quant GetLookupTable]
    i2pd SetColorModeToLUT
    i2pd SetOutputStyleToPolygonalize
    i2pd SetError 0
    i2pd DecimationOn
    i2pd SetDecimationError 0.0
    i2pd SetSubImageSize 25

#Need a triangle filter because the polygons are complex and concave
vtkTriangleFilter tf
    tf SetInput [i2pd GetOutput]

vtkPolyDataMapper mapper
    mapper SetInput [tf GetOutput]

vtkActor actor
    actor SetMapper mapper

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor actor

ren1 SetBackground 1 1 1
renWin SetSize 300 250

vtkCamera acamera
    acamera SetClippingRange 343.331 821.78
    acamera SetPosition -139.802 -85.6604 437.485
    acamera SetFocalPoint 117.424 106.656 -14.6
    acamera SetViewUp 0.430481 0.716032 0.549532
    acamera SetViewAngle 30
ren1 SetActiveCamera acamera

iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
