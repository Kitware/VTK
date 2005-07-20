package require vtk
package require vtkinteraction

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn  
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkEnSightMasterServerReader reader1
    reader1 SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/mandelbrot.sos"
    reader1 SetCurrentPiece 0
    reader1 Update

vtkGeometryFilter geom0
    geom0 SetInputConnection [reader1 GetOutputPort]
vtkGeometryFilter geom1
    geom1 SetInput [reader1 GetOutput 1]

vtkPolyDataMapper mapper0
    mapper0 SetInputConnection [geom0 GetOutputPort]
    mapper0 SetColorModeToMapScalars
    mapper0 SetScalarModeToUsePointFieldData
    mapper0 ColorByArrayComponent "Iterations" 0
    mapper0 SetScalarRange 0 112
vtkPolyDataMapper mapper1
    mapper1 SetInputConnection [geom1 GetOutputPort]
    mapper1 SetColorModeToMapScalars
    mapper1 SetScalarModeToUsePointFieldData
    mapper1 ColorByArrayComponent "Iterations" 0
    mapper1 SetScalarRange 0 112

vtkActor actor0
    actor0 SetMapper mapper0
vtkActor actor1
    actor1 SetMapper mapper1

vtkEnSightMasterServerReader reader2
    reader2 SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/mandelbrot.sos"
    reader2 SetCurrentPiece 1
    reader2 Update

vtkGeometryFilter geom2
    geom2 SetInputConnection [reader2 GetOutputPort]
vtkGeometryFilter geom3
    geom3 SetInput [reader2 GetOutput 1]

vtkPolyDataMapper mapper2
    mapper2 SetInputConnection [geom2 GetOutputPort]
    mapper2 SetColorModeToMapScalars
    mapper2 SetScalarModeToUsePointFieldData
    mapper2 ColorByArrayComponent "Iterations" 0
    mapper2 SetScalarRange 0 112
vtkPolyDataMapper mapper3
    mapper3 SetInputConnection [geom3 GetOutputPort]
    mapper3 SetColorModeToMapScalars
    mapper3 SetScalarModeToUsePointFieldData
    mapper3 ColorByArrayComponent "Iterations" 0
    mapper3 SetScalarRange 0 112

vtkActor actor2
    actor2 SetMapper mapper2
vtkActor actor3
    actor3 SetMapper mapper3

# assign our actor to the renderer
ren1 AddActor actor0
ren1 AddActor actor1
ren1 AddActor actor2
ren1 AddActor actor3

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

