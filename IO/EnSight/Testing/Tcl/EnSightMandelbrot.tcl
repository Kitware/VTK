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
# Make sure all algorithms use the composite data pipeline
vtkCompositeDataPipeline cdp
reader1 SetDefaultExecutivePrototype cdp
    reader1 SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/mandelbrot.sos"
    reader1 SetCurrentPiece 0

vtkGeometryFilter geom0
    geom0 SetInputConnection [reader1 GetOutputPort]

vtkHierarchicalPolyDataMapper mapper0
    mapper0 SetInputConnection [geom0 GetOutputPort]
    mapper0 SetColorModeToMapScalars
    mapper0 SetScalarModeToUsePointFieldData
    mapper0 ColorByArrayComponent "Iterations" 0
    mapper0 SetScalarRange 0 112

vtkActor actor0
    actor0 SetMapper mapper0

vtkEnSightMasterServerReader reader2
    reader2 SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/mandelbrot.sos"
    reader2 SetCurrentPiece 1

vtkGeometryFilter geom2
    geom2 SetInputConnection [reader2 GetOutputPort]

vtkHierarchicalPolyDataMapper mapper2
    mapper2 SetInputConnection [geom2 GetOutputPort]
    mapper2 SetColorModeToMapScalars
    mapper2 SetScalarModeToUsePointFieldData
    mapper2 ColorByArrayComponent "Iterations" 0
    mapper2 SetScalarRange 0 112

vtkActor actor2
    actor2 SetMapper mapper2

# assign our actor to the renderer
ren1 AddActor actor0
ren1 AddActor actor2

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

reader1 SetDefaultExecutivePrototype {}
