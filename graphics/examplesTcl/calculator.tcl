catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Create blow molding image (data point 9)
# get the interactor
source $VTK_TCL/vtkInt.tcl

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin StereoCapableWindowOn
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create reader and warp data with vectors
vtkGenericEnSightReader reader
    reader SetCaseFileName $VTK_DATA/EnSight/elements6.case
    reader Update

vtkGeometryFilter geom
    geom SetInput [reader GetOutput]

vtkArrayCalculator calc
    calc SetInput [geom GetOutput]
    calc SetAttributeModeToUsePointData
    calc SetFunction "cos(-3 * (v_0 * v_1 * v_2) + abs(s)) - sqrt(mag(v1 + v2 ))"
    calc AddScalarVariable s pointScalars 0
    calc AddScalarVariable v_0 pointVectors 0
    calc AddScalarVariable v_1 pointVectors 1
    calc AddScalarVariable v_2 pointVectors 2
    calc AddVectorVariable v1 pointVectors 0 1 2
    calc AddVectorVariable v2 pointCVectors_r 0 1 2
    calc SetResultArrayName resArray

vtkPolyDataMapper mapper
    mapper SetInput [calc GetOutput]
    mapper SetColorModeToMapScalars
    mapper SetScalarModeToUsePointFieldData
    mapper ColorByArrayComponent resArray 0
    mapper SetScalarRange -21 0

vtkActor actor
    actor SetMapper mapper

# Add the actors to the renderer, set the background and size
ren1 AddActor actor

iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

