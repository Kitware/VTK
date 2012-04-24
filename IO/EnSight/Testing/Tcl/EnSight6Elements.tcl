package require vtk
package require vtkinteraction

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkGenericEnSightReader reader
# Make sure all algorithms use the composite data pipeline
vtkCompositeDataPipeline cdp
reader SetDefaultExecutivePrototype cdp
cdp Delete
    reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/elements6.case"

vtkGeometryFilter geom
    geom SetInputConnection [reader GetOutputPort]

vtkArrayCalculator calc
    calc SetInputConnection [geom GetOutputPort]
    calc SetAttributeModeToUsePointData
    calc SetFunction "pointCVectors_r . pointCVectors_i + pointScalars"
    calc AddScalarArrayName "pointScalars" 0
    calc AddVectorArrayName "pointCVectors_r" 0 1 2
    calc AddVectorArrayName "pointCVectors_i" 0 1 2
    calc SetResultArrayName "test"

vtkHierarchicalPolyDataMapper mapper
    mapper SetInputConnection [calc GetOutputPort]
    mapper SetColorModeToMapScalars
    mapper SetScalarModeToUsePointFieldData
    mapper ColorByArrayComponent "test" 0
    mapper SetScalarRange 0 36000

vtkActor actor
    actor SetMapper mapper

# assign our actor to the renderer
ren1 AddActor actor

# enable user interface interactor
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

reader SetDefaultExecutivePrototype {}
