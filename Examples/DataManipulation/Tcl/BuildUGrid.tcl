# This example shows how to manually construct unstructured grids using
# Tcl. Unstructured grids require explicit point and cell representations,
# so every point and cell must be created, and then added to the
# vtkUnstructuredGrid instance.
#

package require vtk
package require vtkinteraction

# Create several unstructured grids each containing a cell of a different type.
vtkPoints voxelPoints
  voxelPoints SetNumberOfPoints 8
  voxelPoints InsertPoint 0 0 0 0
  voxelPoints InsertPoint 1 1 0 0
  voxelPoints InsertPoint 2 0 1 0
  voxelPoints InsertPoint 3 1 1 0
  voxelPoints InsertPoint 4 0 0 1
  voxelPoints InsertPoint 5 1 0 1
  voxelPoints InsertPoint 6 0 1 1
  voxelPoints InsertPoint 7 1 1 1
vtkVoxel aVoxel
  [aVoxel GetPointIds] SetId 0 0
  [aVoxel GetPointIds] SetId 1 1
  [aVoxel GetPointIds] SetId 2 2
  [aVoxel GetPointIds] SetId 3 3
  [aVoxel GetPointIds] SetId 4 4
  [aVoxel GetPointIds] SetId 5 5
  [aVoxel GetPointIds] SetId 6 6
  [aVoxel GetPointIds] SetId 7 7
vtkUnstructuredGrid aVoxelGrid
  aVoxelGrid Allocate 1 1
  aVoxelGrid InsertNextCell [aVoxel GetCellType] [aVoxel GetPointIds]
  aVoxelGrid SetPoints voxelPoints
vtkDataSetMapper aVoxelMapper
  aVoxelMapper SetInputData aVoxelGrid
vtkActor aVoxelActor
  aVoxelActor SetMapper aVoxelMapper
  [aVoxelActor GetProperty] SetDiffuseColor 1 0 0

vtkPoints hexahedronPoints
  hexahedronPoints SetNumberOfPoints 8
  hexahedronPoints InsertPoint 0 0 0 0
  hexahedronPoints InsertPoint 1 1 0 0
  hexahedronPoints InsertPoint 2 1 1 0
  hexahedronPoints InsertPoint 3 0 1 0
  hexahedronPoints InsertPoint 4 0 0 1
  hexahedronPoints InsertPoint 5 1 0 1
  hexahedronPoints InsertPoint 6 1 1 1
  hexahedronPoints InsertPoint 7 0 1 1
vtkHexahedron aHexahedron
  [aHexahedron GetPointIds] SetId 0 0
  [aHexahedron GetPointIds] SetId 1 1
  [aHexahedron GetPointIds] SetId 2 2
  [aHexahedron GetPointIds] SetId 3 3
  [aHexahedron GetPointIds] SetId 4 4
  [aHexahedron GetPointIds] SetId 5 5
  [aHexahedron GetPointIds] SetId 6 6
  [aHexahedron GetPointIds] SetId 7 7
vtkUnstructuredGrid aHexahedronGrid
  aHexahedronGrid Allocate 1 1
  aHexahedronGrid InsertNextCell [aHexahedron GetCellType] [aHexahedron GetPointIds]
  aHexahedronGrid SetPoints hexahedronPoints
vtkDataSetMapper aHexahedronMapper
  aHexahedronMapper SetInputData aHexahedronGrid
vtkActor aHexahedronActor
  aHexahedronActor SetMapper aHexahedronMapper
  aHexahedronActor AddPosition 2 0 0
  [aHexahedronActor GetProperty] SetDiffuseColor 1 1 0

vtkPoints tetraPoints
  tetraPoints SetNumberOfPoints 4
  tetraPoints InsertPoint 0 0 0 0
  tetraPoints InsertPoint 1 1 0 0
  tetraPoints InsertPoint 2 .5 1 0
  tetraPoints InsertPoint 3 .5 .5 1
vtkTetra aTetra
  [aTetra GetPointIds] SetId 0 0
  [aTetra GetPointIds] SetId 1 1
  [aTetra GetPointIds] SetId 2 2
  [aTetra GetPointIds] SetId 3 3
vtkUnstructuredGrid aTetraGrid
  aTetraGrid Allocate 1 1
  aTetraGrid InsertNextCell [aTetra GetCellType] [aTetra GetPointIds]
  aTetraGrid SetPoints tetraPoints
vtkDataSetMapper aTetraMapper
  aTetraMapper SetInputData aTetraGrid
vtkActor aTetraActor
  aTetraActor SetMapper aTetraMapper
  aTetraActor AddPosition 4 0 0
  [aTetraActor GetProperty] SetDiffuseColor 0 1 0

vtkPoints wedgePoints
  wedgePoints SetNumberOfPoints 6
  wedgePoints InsertPoint 0 0 1 0
  wedgePoints InsertPoint 1 0 0 0
  wedgePoints InsertPoint 2 0 .5 .5
  wedgePoints InsertPoint 3 1 1 0
  wedgePoints InsertPoint 4 1 0 0
  wedgePoints InsertPoint 5 1 .5 .5
vtkWedge aWedge
  [aWedge GetPointIds] SetId 0 0
  [aWedge GetPointIds] SetId 1 1
  [aWedge GetPointIds] SetId 2 2
  [aWedge GetPointIds] SetId 3 3
  [aWedge GetPointIds] SetId 4 4
  [aWedge GetPointIds] SetId 5 5
vtkUnstructuredGrid aWedgeGrid
  aWedgeGrid Allocate 1 1
  aWedgeGrid InsertNextCell [aWedge GetCellType] [aWedge GetPointIds]
  aWedgeGrid SetPoints wedgePoints
vtkDataSetMapper aWedgeMapper
  aWedgeMapper SetInputData aWedgeGrid
vtkActor aWedgeActor
  aWedgeActor SetMapper aWedgeMapper
  aWedgeActor AddPosition 6 0 0
  [aWedgeActor GetProperty] SetDiffuseColor 0 1 1

vtkPoints pyramidPoints
  pyramidPoints SetNumberOfPoints 5
  pyramidPoints InsertPoint 0 0 0 0
  pyramidPoints InsertPoint 1 1 0 0
  pyramidPoints InsertPoint 2 1 1 0
  pyramidPoints InsertPoint 3 0 1 0
  pyramidPoints InsertPoint 4 .5 .5 1
vtkPyramid aPyramid
  [aPyramid GetPointIds] SetId 0 0
  [aPyramid GetPointIds] SetId 1 1
  [aPyramid GetPointIds] SetId 2 2
  [aPyramid GetPointIds] SetId 3 3
  [aPyramid GetPointIds] SetId 4 4
vtkUnstructuredGrid aPyramidGrid
  aPyramidGrid Allocate 1 1
  aPyramidGrid InsertNextCell [aPyramid GetCellType] [aPyramid GetPointIds]
  aPyramidGrid SetPoints pyramidPoints
vtkDataSetMapper aPyramidMapper
  aPyramidMapper SetInputData aPyramidGrid
vtkActor aPyramidActor
  aPyramidActor SetMapper aPyramidMapper
  aPyramidActor AddPosition 8 0 0
  [aPyramidActor GetProperty] SetDiffuseColor 1 0 1

vtkPoints pixelPoints
  pixelPoints SetNumberOfPoints 4
  pixelPoints InsertPoint 0 0 0 0
  pixelPoints InsertPoint 1 1 0 0
  pixelPoints InsertPoint 2 0 1 0
  pixelPoints InsertPoint 3 1 1 0
vtkPixel aPixel
  [aPixel GetPointIds] SetId 0 0
  [aPixel GetPointIds] SetId 1 1
  [aPixel GetPointIds] SetId 2 2
  [aPixel GetPointIds] SetId 3 3
vtkUnstructuredGrid aPixelGrid
  aPixelGrid Allocate 1 1
  aPixelGrid InsertNextCell [aPixel GetCellType] [aPixel GetPointIds]
  aPixelGrid SetPoints pixelPoints
vtkDataSetMapper aPixelMapper
  aPixelMapper SetInputData aPixelGrid
vtkActor aPixelActor
  aPixelActor SetMapper aPixelMapper
  aPixelActor AddPosition 0 0 2
  [aPixelActor GetProperty] SetDiffuseColor 0 1 1

vtkPoints quadPoints
  quadPoints SetNumberOfPoints 4
  quadPoints InsertPoint 0 0 0 0
  quadPoints InsertPoint 1 1 0 0
  quadPoints InsertPoint 2 1 1 0
  quadPoints InsertPoint 3 0 1 0
vtkQuad aQuad
  [aQuad GetPointIds] SetId 0 0
  [aQuad GetPointIds] SetId 1 1
  [aQuad GetPointIds] SetId 2 2
  [aQuad GetPointIds] SetId 3 3
vtkUnstructuredGrid aQuadGrid
  aQuadGrid Allocate 1 1
  aQuadGrid InsertNextCell [aQuad GetCellType] [aQuad GetPointIds]
  aQuadGrid SetPoints quadPoints
vtkDataSetMapper aQuadMapper
  aQuadMapper SetInputData aQuadGrid
vtkActor aQuadActor
  aQuadActor SetMapper aQuadMapper
  aQuadActor AddPosition 2 0 2
  [aQuadActor GetProperty] SetDiffuseColor 1 0 1

vtkPoints trianglePoints
  trianglePoints SetNumberOfPoints 3
  trianglePoints InsertPoint 0 0 0 0
  trianglePoints InsertPoint 1 1 0 0
  trianglePoints InsertPoint 2 .5 .5 0
vtkFloatArray triangleTCoords
  triangleTCoords SetNumberOfComponents 3
  triangleTCoords SetNumberOfTuples 3
  triangleTCoords InsertTuple3 0 1 1 1
  triangleTCoords InsertTuple3 1 2 2 2
  triangleTCoords InsertTuple3 2 3 3 3
vtkTriangle aTriangle
  [aTriangle GetPointIds] SetId 0 0
  [aTriangle GetPointIds] SetId 1 1
  [aTriangle GetPointIds] SetId 2 2
vtkUnstructuredGrid aTriangleGrid
  aTriangleGrid Allocate 1 1
  aTriangleGrid InsertNextCell [aTriangle GetCellType] [aTriangle GetPointIds]
  aTriangleGrid SetPoints trianglePoints
  [aTriangleGrid GetPointData] SetTCoords triangleTCoords
vtkDataSetMapper aTriangleMapper
  aTriangleMapper SetInputData aTriangleGrid
vtkActor aTriangleActor
  aTriangleActor SetMapper aTriangleMapper
  aTriangleActor AddPosition 4 0 2
  [aTriangleActor GetProperty] SetDiffuseColor .3 1 .5

vtkPoints polygonPoints
  polygonPoints SetNumberOfPoints 4
  polygonPoints InsertPoint 0 0 0 0
  polygonPoints InsertPoint 1 1 0 0
  polygonPoints InsertPoint 2 1 1 0
  polygonPoints InsertPoint 3 0 1 0
vtkPolygon aPolygon
  [aPolygon GetPointIds] SetNumberOfIds 4
  [aPolygon GetPointIds] SetId 0 0
  [aPolygon GetPointIds] SetId 1 1
  [aPolygon GetPointIds] SetId 2 2
  [aPolygon GetPointIds] SetId 3 3
vtkUnstructuredGrid aPolygonGrid
  aPolygonGrid Allocate 1 1
  aPolygonGrid InsertNextCell [aPolygon GetCellType] [aPolygon GetPointIds]
  aPolygonGrid SetPoints polygonPoints
vtkDataSetMapper aPolygonMapper
  aPolygonMapper SetInputData aPolygonGrid
vtkActor aPolygonActor
  aPolygonActor SetMapper aPolygonMapper
  aPolygonActor AddPosition 6 0 2
  [aPolygonActor GetProperty] SetDiffuseColor 1 .4 .5

vtkPoints triangleStripPoints
  triangleStripPoints SetNumberOfPoints 5
  triangleStripPoints InsertPoint 0 0 1 0
  triangleStripPoints InsertPoint 1 0 0 0
  triangleStripPoints InsertPoint 2 1 1 0
  triangleStripPoints InsertPoint 3 1 0 0
  triangleStripPoints InsertPoint 4 2 1 0
vtkFloatArray triangleStripTCoords
  triangleStripTCoords SetNumberOfComponents 3
  triangleStripTCoords SetNumberOfTuples 3
  triangleStripTCoords InsertTuple3 0 1 1 1
  triangleStripTCoords InsertTuple3 1 2 2 2
  triangleStripTCoords InsertTuple3 2 3 3 3
  triangleStripTCoords InsertTuple3 3 4 4 4
  triangleStripTCoords InsertTuple3 4 5 5 5
vtkTriangleStrip aTriangleStrip
  [aTriangleStrip GetPointIds] SetNumberOfIds 5
  [aTriangleStrip GetPointIds] SetId 0 0
  [aTriangleStrip GetPointIds] SetId 1 1
  [aTriangleStrip GetPointIds] SetId 2 2
  [aTriangleStrip GetPointIds] SetId 3 3
  [aTriangleStrip GetPointIds] SetId 4 4
vtkUnstructuredGrid aTriangleStripGrid
  aTriangleStripGrid Allocate 1 1
  aTriangleStripGrid InsertNextCell [aTriangleStrip GetCellType] [aTriangleStrip GetPointIds]
  aTriangleStripGrid SetPoints triangleStripPoints
  [aTriangleStripGrid GetPointData] SetTCoords triangleStripTCoords
vtkDataSetMapper aTriangleStripMapper
  aTriangleStripMapper SetInputData aTriangleStripGrid
vtkActor aTriangleStripActor
  aTriangleStripActor SetMapper aTriangleStripMapper
  aTriangleStripActor AddPosition 8 0 2
  [aTriangleStripActor GetProperty] SetDiffuseColor .3 .7 1

vtkPoints linePoints
  linePoints SetNumberOfPoints 2
  linePoints InsertPoint 0 0 0 0
  linePoints InsertPoint 1 1 1 0
vtkLine aLine
  [aLine GetPointIds] SetId 0 0
  [aLine GetPointIds] SetId 1 1
vtkUnstructuredGrid aLineGrid
  aLineGrid Allocate 1 1
  aLineGrid InsertNextCell [aLine GetCellType] [aLine GetPointIds]
  aLineGrid SetPoints linePoints
vtkDataSetMapper aLineMapper
  aLineMapper SetInputData aLineGrid
vtkActor aLineActor
  aLineActor SetMapper aLineMapper
  aLineActor AddPosition 0 0 4
  [aLineActor GetProperty] SetDiffuseColor .2 1 1

vtkPoints polyLinePoints
  polyLinePoints SetNumberOfPoints 3
  polyLinePoints InsertPoint 0 0 0 0
  polyLinePoints InsertPoint 1 1 1 0
  polyLinePoints InsertPoint 2 1 0 0
vtkPolyLine aPolyLine
  [aPolyLine GetPointIds] SetNumberOfIds 3
  [aPolyLine GetPointIds] SetId 0 0
  [aPolyLine GetPointIds] SetId 1 1
  [aPolyLine GetPointIds] SetId 2 2
vtkUnstructuredGrid aPolyLineGrid
  aPolyLineGrid Allocate 1 1
  aPolyLineGrid InsertNextCell [aPolyLine GetCellType] [aPolyLine GetPointIds]
  aPolyLineGrid SetPoints polyLinePoints
vtkDataSetMapper aPolyLineMapper
  aPolyLineMapper SetInputData aPolyLineGrid
vtkActor aPolyLineActor
  aPolyLineActor SetMapper aPolyLineMapper
  aPolyLineActor AddPosition 2 0 4
  [aPolyLineActor GetProperty] SetDiffuseColor 1 1 1

vtkPoints vertexPoints
  vertexPoints SetNumberOfPoints 1
  vertexPoints InsertPoint 0 0 0 0
vtkVertex aVertex
  [aVertex GetPointIds] SetId 0 0
vtkUnstructuredGrid aVertexGrid
  aVertexGrid Allocate 1 1
  aVertexGrid InsertNextCell [aVertex GetCellType] [aVertex GetPointIds]
  aVertexGrid SetPoints vertexPoints
vtkDataSetMapper aVertexMapper
  aVertexMapper SetInputData aVertexGrid
vtkActor aVertexActor
  aVertexActor SetMapper aVertexMapper
  aVertexActor AddPosition 0 0 6
  [aVertexActor GetProperty] SetDiffuseColor 1 1 1

vtkPoints polyVertexPoints
  polyVertexPoints SetNumberOfPoints 3
  polyVertexPoints InsertPoint 0 0 0 0
  polyVertexPoints InsertPoint 1 1 0 0
  polyVertexPoints InsertPoint 2 1 1 0
vtkPolyVertex aPolyVertex
  [aPolyVertex GetPointIds] SetNumberOfIds 3
  [aPolyVertex GetPointIds] SetId 0 0
  [aPolyVertex GetPointIds] SetId 1 1
  [aPolyVertex GetPointIds] SetId 2 2
vtkUnstructuredGrid aPolyVertexGrid
  aPolyVertexGrid Allocate 1 1
  aPolyVertexGrid InsertNextCell [aPolyVertex GetCellType] [aPolyVertex GetPointIds]
  aPolyVertexGrid SetPoints polyVertexPoints
vtkDataSetMapper aPolyVertexMapper
  aPolyVertexMapper SetInputData aPolyVertexGrid
vtkActor aPolyVertexActor
  aPolyVertexActor SetMapper aPolyVertexMapper
  aPolyVertexActor AddPosition 2 0 6
 [aPolyVertexActor GetProperty] SetDiffuseColor 1 1 1

# Create the usual rendering stuff.
vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin SetSize 300 150
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren1 SetBackground .1 .2 .4

ren1 AddActor aVoxelActor
ren1 AddActor aHexahedronActor
ren1 AddActor aTetraActor
ren1 AddActor aWedgeActor
ren1 AddActor aPyramidActor
ren1 AddActor aPixelActor
ren1 AddActor aQuadActor
ren1 AddActor aTriangleActor
ren1 AddActor aPolygonActor
ren1 AddActor aTriangleStripActor
ren1 AddActor aLineActor
ren1 AddActor aPolyLineActor
ren1 AddActor aVertexActor
ren1 AddActor aPolyVertexActor

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
[ren1 GetActiveCamera] Dolly 2.8
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .
iren Start