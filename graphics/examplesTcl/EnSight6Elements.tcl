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

vtkGenericEnSightReader reader
    reader SetCaseFileName "C:/EnSightFiles/elements6/elements6.case"
    reader Update

vtkGeometryFilter geom
    geom SetInput [reader GetOutput]

vtkArrayCalculator calc
    calc SetInput [geom GetOutput]
    calc SetAttributeModeToUsePointData
    calc SetFunction "pointCVectors_r . pointCVectors_i + pointScalars"
    calc AddScalarArrayName pointScalars 0
    calc AddVectorArrayName pointCVectors_r 0 1 2
    calc AddVectorArrayName pointCVectors_i 0 1 2
    calc SetResultArrayName test

vtkPolyDataMapper mapper
    mapper SetInput [calc GetOutput]
    mapper SetColorModeToMapScalars
    mapper SetScalarModeToUsePointFieldData
    mapper ColorByArrayComponent test 0
    mapper SetScalarRange 0 36000

vtkActor actor
    actor SetMapper mapper

# assign our actor to the renderer
ren1 AddActor actor

# enable user interface interactor
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

