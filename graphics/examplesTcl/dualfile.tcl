#
# this regression test was contributed by:
#
# Dr. Alexander Supalov
# GMD  -- German National Research Center for Information Technology
# SCAI -- Institute for Algorithms and Scientific Computing
# Schloss Birlinghoven                    phone:  +49 2241 14 2371
# 53754 Sankt Augustin                    fax:    +49 2241 14 2181
# Germany                                 e-mail: supalov@gmd.de

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

wm withdraw .

catch {load vtktcl}

#
#	input static grid data once
#
vtkUnstructuredGridReader r
    r SetFileName "../../../vtkdata/dualgrid.vtk"
    r Update

#
#	input cell attributes
#
vtkDataObjectReader c
    c SetFileName "../../../vtkdata/dualcell.vtk"
    c Update

#
#	input selected step data
#
vtkDataObjectReader s
    s SetFileName "../../../vtkdata/dualpoint.vtk"
    s Update

#
#	combine the grid and step data
#
vtkScalars cs
    cs SetData [[[c GetOutput] GetFieldData] GetArray 0]
    [[r GetOutput] GetCellData] SetScalars cs
    cs Delete

vtkScalars ps
    ps SetData [[[s GetOutput] GetFieldData] GetArray 0]
    [[r GetOutput] GetPointData] SetScalars ps
    ps Delete

#
#	now, business as usual
#
vtkGeometryFilter f1
    f1 SetInput [r GetOutput]

vtkPolyDataNormals f
    f SetInput [f1 GetOutput]

vtkLookupTable l
    l SetHueRange 0.66667 0.0

vtkPolyDataMapper m
    m SetInput [f GetOutput]
    m SetScalarModeToUsePointData
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

renWin SetFileName valid/dualfile.tcl.ppm
#renWin SaveImageAsPPM

