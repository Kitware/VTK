catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Test field data reading - Thanks to Alexander Supalov

wm withdraw .

vtkUnstructuredGridReader r
    r SetFileName "../../../vtkdata/fieldfile.vtk"
    r Update

set a [[[[r GetOutput] GetCellData] GetFieldData] GetArray 0]

vtkScalars s
    s SetData $a
    [[r GetOutput] GetCellData] SetScalars s
    s Delete

vtkGeometryFilter f
    f SetInput [r GetOutput]

vtkLookupTable l
    l SetHueRange 0.66667 0.0

vtkDataSetMapper m
    m SetInput [f GetOutput]
    m SetLookupTable l
    m SetScalarRange 1 3

vtkProperty p
    p SetDiffuse 0.5
    p SetAmbient 0.5

vtkActor a
    a SetMapper m
    a SetProperty p

vtkRenderer ren
    ren AddActor a
    ren SetBackground 1 1 1

vtkRenderWindowInteractor iren
iren SetUserMethod {wm deiconify .vtkInteract}

vtkRenderWindow renWin
    renWin AddRenderer ren
    renWin SetInteractor iren
    renWin Render

renWin SetFileName valid/fieldfile.tcl.ppm
#renWin SaveImageAsPPM

