catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# user interface command widget
source $VTK_TCL/vtkInt.tcl

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn  
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkEnSight6Reader reader
    reader SetFilePath $VTK_DATA/EnSight/
    reader SetCaseFileName "en6.case"
    reader Update

vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [reader GetOutput]
    fd2ad SetInputFieldToPointDataField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetScalarComponent 0 Nsca 0

vtkGeometryFilter geom0
    geom0 SetInput [fd2ad GetOutput]
vtkGeometryFilter geom1
    geom1 SetInput [reader GetOutput 1]
vtkGeometryFilter geom2
    geom2 SetInput [reader GetOutput 2]

vtkPolyDataMapper mapper0
    mapper0 SetInput [geom0 GetOutput]
    mapper0 SetScalarRange 0 12
vtkPolyDataMapper mapper1
    mapper1 SetInput [geom1 GetOutput]
vtkPolyDataMapper mapper2
    mapper2 SetInput [geom2 GetOutput]

vtkActor actor0
    actor0 SetMapper mapper0
vtkActor actor1
    actor1 SetMapper mapper1
vtkActor actor2
    actor2 SetMapper mapper2

# assign our actor to the renderer
ren1 AddActor actor0
ren1 AddActor actor1
ren1 AddActor actor2

# enable user interface interactor
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

