package require vtk
package require vtkinteraction

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

vtkBMPReader pnmReader
  pnmReader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"
vtkTexture texture
  texture SetInput [pnmReader GetOutput]

vtkPoints triangleStripPoints
  triangleStripPoints SetNumberOfPoints 5
  triangleStripPoints InsertPoint 0 0 1 0
  triangleStripPoints InsertPoint 1 0 0 .5
  triangleStripPoints InsertPoint 2 1 1 .3
  triangleStripPoints InsertPoint 3 1 0 .6
  triangleStripPoints InsertPoint 4 2 1 .1

vtkTCoords triangleStripTCoords
  triangleStripTCoords SetNumberOfTCoords 5
  triangleStripTCoords InsertTCoord 0 0 1 0
  triangleStripTCoords InsertTCoord 1 0 0 0
  triangleStripTCoords InsertTCoord 2 .5 1 0
  triangleStripTCoords InsertTCoord 3 .5 0 0
  triangleStripTCoords InsertTCoord 4 1 1 0

vtkScalars triangleStripPointScalars
  triangleStripPointScalars SetNumberOfScalars 5
  triangleStripPointScalars InsertScalar 0 1
  triangleStripPointScalars InsertScalar 1 0
  triangleStripPointScalars InsertScalar 2 0
  triangleStripPointScalars InsertScalar 3 0
  triangleStripPointScalars InsertScalar 4 0

vtkScalars triangleStripCellScalars
  triangleStripCellScalars SetNumberOfScalars 1
  triangleStripCellScalars InsertScalar 0 1

vtkNormals triangleStripPointNormals
  triangleStripPointNormals SetNumberOfNormals 5
  triangleStripPointNormals InsertNormal 0 0 0 1
  triangleStripPointNormals InsertNormal 1 0 1 0
  triangleStripPointNormals InsertNormal 2 0 1 1
  triangleStripPointNormals InsertNormal 3 1 0 0
  triangleStripPointNormals InsertNormal 4 1 0 1

vtkNormals triangleStripCellNormals
  triangleStripCellNormals SetNumberOfNormals 1
  triangleStripCellNormals InsertNormal 0 1 1 1

vtkTriangleStrip aTriangleStrip
  [aTriangleStrip GetPointIds] SetNumberOfIds 5
  [aTriangleStrip GetPointIds] SetId 0 0
  [aTriangleStrip GetPointIds] SetId 1 1
  [aTriangleStrip GetPointIds] SetId 2 2
  [aTriangleStrip GetPointIds] SetId 3 3
  [aTriangleStrip GetPointIds] SetId 4 4

vtkLookupTable lut
  lut SetNumberOfColors 5
  lut SetTableValue 0 0 0 1 1
  lut SetTableValue 1 0 1 0 1
  lut SetTableValue 2 0 1 1 1
  lut SetTableValue 3 1 0 0 1
  lut SetTableValue 4 1 0 1 1

set masks "0 1 2 3 4 5 6 7 10 11 14 15 16 18 20 22 26 30"
set i 0; set j 0 ; set k 0
set types "strip triangle"
foreach type $types {
  foreach mask $masks {
    vtkUnstructuredGrid grid$i
      grid$i Allocate 1 1
      grid$i InsertNextCell [aTriangleStrip GetCellType] [aTriangleStrip GetPointIds]
      grid$i SetPoints triangleStripPoints
    vtkGeometryFilter geometry$i
      geometry$i SetInput grid$i
    vtkTriangleFilter triangles$i
      triangles$i SetInput [geometry$i GetOutput]
    vtkPolyDataMapper mapper$i
    if {$type == "strip"} {mapper$i SetInput [geometry$i GetOutput]}
    if {$type == "triangle"} {mapper$i SetInput [triangles$i GetOutput]}
      mapper$i SetLookupTable lut
      mapper$i SetScalarRange 0 4
    vtkActor actor$i
      actor$i SetMapper mapper$i
  
      if {[expr $mask & 1] != 0} {
        [grid$i GetPointData] SetNormals triangleStripPointNormals
      }
      if {[expr $mask & 2] != 0} {
        [grid$i GetPointData] SetScalars triangleStripPointScalars
        mapper$i SetScalarModeToUsePointData
      }
      if {[expr $mask & 4] != 0} {
        [grid$i GetPointData] SetTCoords triangleStripTCoords
        actor$i SetTexture texture
      }
      if {[expr $mask & 8] != 0} {
        [grid$i GetCellData] SetScalars triangleStripCellScalars
        mapper$i SetScalarModeToUseCellData
      }
      if {[expr $mask & 16] != 0} {
        [grid$i GetCellData] SetNormals triangleStripCellNormals
      }
    actor$i AddPosition [expr $j * 2] [expr $k * 2] 0
    ren1 AddActor actor$i 
    [actor$i GetProperty] SetRepresentationToWireframe
    incr j
    if {$j >= 6} {set j 0; incr k};
    incr i
  }
}
renWin SetSize 480 480
ren1 SetBackground .7 .3 .1
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .



