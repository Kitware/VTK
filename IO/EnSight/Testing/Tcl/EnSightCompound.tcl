package require vtk
package require vtkinteraction

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkEnSightMasterServerReader reader
# Make sure all algorithms use the composite data pipeline
vtkCompositeDataPipeline cdp
reader SetDefaultExecutivePrototype cdp
    reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/elements.sos"
    reader SetCurrentPiece 0

vtkGeometryFilter geom0
    geom0 SetInputConnection [reader GetOutputPort]


vtkHierarchicalPolyDataMapper mapper0
    mapper0 SetInputConnection [geom0 GetOutputPort]
    mapper0 SetColorModeToMapScalars
    mapper0 SetScalarModeToUsePointFieldData
    mapper0 ColorByArrayComponent "pointScalars" 0
    mapper0 SetScalarRange 0 112

vtkActor actor0
    actor0 SetMapper mapper0

# assign our actor to the renderer
ren1 AddActor actor0

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

reader SetDefaultExecutivePrototype {}
