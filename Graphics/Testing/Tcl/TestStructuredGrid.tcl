package require vtk
package require vtkinteraction

# Remove cullers so single vertex will render
vtkRenderer ren1
    [ren1 GetCullers] RemoveAllItems 
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkGenericCell cell
vtkIdList ptIds

# 0D
vtkPoints ZeroDPts
ZeroDPts SetNumberOfPoints 1
ZeroDPts SetPoint 0 0 0 0

vtkStructuredGrid ZeroDGrid
ZeroDGrid SetDimensions 1 1 1
ZeroDGrid SetPoints ZeroDPts
ZeroDGrid GetCell 0
ZeroDGrid GetCell 0 cell
ZeroDGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter ZeroDGeom
ZeroDGeom SetInput ZeroDGrid
ZeroDGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper ZeroDMapper
ZeroDMapper SetInput [ZeroDGeom GetOutput]

vtkActor ZeroDActor
ZeroDActor SetMapper ZeroDMapper
ZeroDActor SetPosition 0 0 0

ren1 AddActor ZeroDActor

# 1D - X
vtkPoints XPts
XPts SetNumberOfPoints 2
XPts SetPoint 0 0 0 0
XPts SetPoint 1 1 0 0

vtkStructuredGrid XGrid
XGrid SetDimensions 2 1 1
XGrid SetPoints XPts
XGrid GetCell 0
XGrid GetCell 0 cell
XGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter XGeom
XGeom SetInput XGrid
XGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper XMapper
XMapper SetInput [XGeom GetOutput]

vtkActor XActor
XActor SetMapper XMapper
XActor SetPosition 2 0 0

ren1 AddActor XActor

# 1D - Y
vtkPoints YPts
YPts SetNumberOfPoints 2
YPts SetPoint 0 0 0 0
YPts SetPoint 1 0 1 0

vtkStructuredGrid YGrid
YGrid SetDimensions 1 2 1
YGrid SetPoints YPts
YGrid GetCell 0
YGrid GetCell 0 cell
YGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter YGeom
YGeom SetInput YGrid
YGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper YMapper
YMapper SetInput [YGeom GetOutput]

vtkActor YActor
YActor SetMapper YMapper
YActor SetPosition 4 0 0

ren1 AddActor YActor


# 1D - Z
vtkPoints ZPts
ZPts SetNumberOfPoints 2
ZPts SetPoint 0 0 0 0
ZPts SetPoint 1 0 0 1

vtkStructuredGrid ZGrid
ZGrid SetDimensions 1 1 2
ZGrid SetPoints ZPts
ZGrid GetCell 0
ZGrid GetCell 0 cell
ZGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter ZGeom
ZGeom SetInput ZGrid
ZGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper ZMapper
ZMapper SetInput [ZGeom GetOutput]

vtkActor ZActor
ZActor SetMapper ZMapper
ZActor SetPosition 6 0 0

ren1 AddActor ZActor

# 2D - XY
vtkPoints XYPts
XYPts SetNumberOfPoints 4
XYPts SetPoint 0 0 0 0
XYPts SetPoint 1 1 0 0
XYPts SetPoint 2 0 1 0
XYPts SetPoint 3 1 1 0

vtkStructuredGrid XYGrid
XYGrid SetDimensions 2 2 1
XYGrid SetPoints XYPts
XYGrid GetCell 0
XYGrid GetCell 0 cell
XYGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter XYGeom
XYGeom SetInput XYGrid
XYGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper XYMapper
XYMapper SetInput [XYGeom GetOutput]

vtkActor XYActor
XYActor SetMapper XYMapper
XYActor SetPosition 0 2 0

ren1 AddActor XYActor

# 2D - YZ
vtkPoints YZPts
YZPts SetNumberOfPoints 4
YZPts SetPoint 0 0 0 0
YZPts SetPoint 1 0 1 0
YZPts SetPoint 2 0 0 1
YZPts SetPoint 3 0 1 1

vtkStructuredGrid YZGrid
YZGrid SetDimensions 1 2 2
YZGrid SetPoints YZPts
YZGrid GetCell 0
YZGrid GetCell 0 cell
YZGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter YZGeom
YZGeom SetInput YZGrid
YZGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper YZMapper
YZMapper SetInput [YZGeom GetOutput]

vtkActor YZActor
YZActor SetMapper YZMapper
YZActor SetPosition 2 2 0

ren1 AddActor YZActor

# 2D - XZ
vtkPoints XZPts
XZPts SetNumberOfPoints 4
XZPts SetPoint 0 0 0 0
XZPts SetPoint 1 1 0 0
XZPts SetPoint 2 0 0 1
XZPts SetPoint 3 1 0 1

vtkStructuredGrid XZGrid
XZGrid SetDimensions 2 1 2
XZGrid SetPoints XZPts
XZGrid GetCell 0
XZGrid GetCell 0 cell
XZGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter XZGeom
XZGeom SetInput XZGrid
XZGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper XZMapper
XZMapper SetInput [XZGeom GetOutput]

vtkActor XZActor
XZActor SetMapper XZMapper
XZActor SetPosition 4 2 0

ren1 AddActor XZActor

# 3D
vtkPoints XYZPts
XYZPts SetNumberOfPoints 8
XYZPts SetPoint 0 0 0 0
XYZPts SetPoint 1 1 0 0
XYZPts SetPoint 2 0 1 0
XYZPts SetPoint 3 1 1 0
XYZPts SetPoint 4 0 0 1
XYZPts SetPoint 5 1 0 1
XYZPts SetPoint 6 0 1 1
XYZPts SetPoint 7 1 1 1

vtkStructuredGrid XYZGrid
XYZGrid SetDimensions 2 2 2
XYZGrid SetPoints XYZPts
XYZGrid GetCell 0
XYZGrid GetCell 0 cell
XYZGrid GetCellPoints 0 ptIds

vtkStructuredGridGeometryFilter XYZGeom
XYZGeom SetInput XYZGrid
XYZGeom SetExtent 0 2 0 2 0 2

vtkPolyDataMapper XYZMapper
XYZMapper SetInput [XYZGeom GetOutput]

vtkActor XYZActor
XYZActor SetMapper XYZMapper
XYZActor SetPosition 6 2 0

ren1 AddActor XYZActor

# render the image
#
renWin SetSize 300 150
set cam1 [ren1 GetActiveCamera]
    $cam1 SetClippingRange 2.27407 14.9819
    $cam1 SetFocalPoint 3.1957 1.74012 0.176603
    $cam1 SetPosition -0.380779 6.13894 5.59404
    $cam1 SetViewUp 0.137568 0.811424 -0.568037

renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
