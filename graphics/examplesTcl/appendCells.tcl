catch {load vtktcl}
# Append, read, and write every cell type

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# create a scene with one of each cell type

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
vtkScalars voxelScalars
  voxelScalars SetNumberOfScalars 8
  voxelScalars InsertScalar 0 0
  voxelScalars InsertScalar 1 1
  voxelScalars InsertScalar 2 0
  voxelScalars InsertScalar 3 0
  voxelScalars InsertScalar 4 0
  voxelScalars InsertScalar 5 0
  voxelScalars InsertScalar 6 0
  voxelScalars InsertScalar 7 0
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
  [aVoxelGrid GetPointData] SetScalars voxelScalars
vtkTransform aVoxelTransform
  aVoxelTransform Translate 0 0 0
vtkTransformFilter transformVoxel
  transformVoxel SetInput aVoxelGrid
  transformVoxel SetTransform aVoxelTransform

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
vtkScalars hexahedronScalars
  hexahedronScalars SetNumberOfScalars 8
  hexahedronScalars InsertScalar 0 0
  hexahedronScalars InsertScalar 1 1
  hexahedronScalars InsertScalar 2 0
  hexahedronScalars InsertScalar 3 0
  hexahedronScalars InsertScalar 4 0
  hexahedronScalars InsertScalar 5 0
  hexahedronScalars InsertScalar 6 0
  hexahedronScalars InsertScalar 7 0
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
  [aHexahedronGrid GetPointData] SetScalars hexahedronScalars
vtkTransform aHexahedronTransform
  aHexahedronTransform Translate 2 0 0
vtkTransformFilter transformHexahedron
  transformHexahedron SetInput aHexahedronGrid
  transformHexahedron SetTransform aHexahedronTransform

vtkPoints tetraPoints
  tetraPoints SetNumberOfPoints 4
  tetraPoints InsertPoint 0 0 0 0
  tetraPoints InsertPoint 1 1 0 0
  tetraPoints InsertPoint 2 .5 1 0
  tetraPoints InsertPoint 3 .5 .5 1
vtkScalars tetraScalars
  tetraScalars SetNumberOfScalars 4
  tetraScalars InsertScalar 0 1
  tetraScalars InsertScalar 1 0
  tetraScalars InsertScalar 2 0
  tetraScalars InsertScalar 3 0
vtkTetra aTetra
  [aTetra GetPointIds] SetId 0 0
  [aTetra GetPointIds] SetId 1 1
  [aTetra GetPointIds] SetId 2 2
  [aTetra GetPointIds] SetId 3 3
vtkUnstructuredGrid aTetraGrid
  aTetraGrid Allocate 1 1
  aTetraGrid InsertNextCell [aTetra GetCellType] [aTetra GetPointIds]
  aTetraGrid SetPoints tetraPoints
  [aTetraGrid GetPointData] SetScalars tetraScalars
vtkTransform aTetraTransform
  aTetraTransform Translate 4 0 0
vtkTransformFilter transformTetra
  transformTetra SetInput aTetraGrid
  transformTetra SetTransform aTetraTransform

vtkPoints wedgePoints
  wedgePoints SetNumberOfPoints 6
  wedgePoints InsertPoint 0 0 1 0
  wedgePoints InsertPoint 1 0 0 0
  wedgePoints InsertPoint 2 0 .5 .5
  wedgePoints InsertPoint 3 1 1 0
  wedgePoints InsertPoint 4 1 0 0
  wedgePoints InsertPoint 5 1 .5 .5
vtkScalars wedgeScalars
  wedgeScalars SetNumberOfScalars 6
  wedgeScalars InsertScalar 0 1
  wedgeScalars InsertScalar 1 1
  wedgeScalars InsertScalar 2 0
  wedgeScalars InsertScalar 3 1
  wedgeScalars InsertScalar 4 1
  wedgeScalars InsertScalar 5 0
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
  [aWedgeGrid GetPointData] SetScalars wedgeScalars
vtkTransform aWedgeTransform
  aWedgeTransform Translate 6 0 0
vtkTransformFilter transformWedge
  transformWedge SetInput aWedgeGrid
  transformWedge SetTransform aWedgeTransform

vtkPoints pyramidPoints
  pyramidPoints SetNumberOfPoints 5
  pyramidPoints InsertPoint 0 0 0 0
  pyramidPoints InsertPoint 1 1 0 0
  pyramidPoints InsertPoint 2 1 1 0
  pyramidPoints InsertPoint 3 0 1 0
  pyramidPoints InsertPoint 4 .5 .5 1
vtkScalars pyramidScalars
  pyramidScalars SetNumberOfScalars 5
  pyramidScalars InsertScalar 0 1
  pyramidScalars InsertScalar 1 1
  pyramidScalars InsertScalar 2 1
  pyramidScalars InsertScalar 3 1
  pyramidScalars InsertScalar 4 0
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
  [aPyramidGrid GetPointData] SetScalars pyramidScalars
vtkTransform aPyramidTransform
  aPyramidTransform Translate 8 0 0
vtkTransformFilter transformPyramid
  transformPyramid SetInput aPyramidGrid
  transformPyramid SetTransform aPyramidTransform

vtkPoints pixelPoints
  pixelPoints SetNumberOfPoints 4
  pixelPoints InsertPoint 0 0 0 0
  pixelPoints InsertPoint 1 1 0 0
  pixelPoints InsertPoint 2 0 1 0
  pixelPoints InsertPoint 3 1 1 0
vtkScalars pixelScalars
  pixelScalars SetNumberOfScalars 4
  pixelScalars InsertScalar 0 1
  pixelScalars InsertScalar 1 0
  pixelScalars InsertScalar 2 0
  pixelScalars InsertScalar 3 0
vtkPixel aPixel
  [aPixel GetPointIds] SetId 0 0
  [aPixel GetPointIds] SetId 1 1
  [aPixel GetPointIds] SetId 2 2
  [aPixel GetPointIds] SetId 3 3
vtkUnstructuredGrid aPixelGrid
  aPixelGrid Allocate 1 1
  aPixelGrid InsertNextCell [aPixel GetCellType] [aPixel GetPointIds]
  aPixelGrid SetPoints pixelPoints
  [aPixelGrid GetPointData] SetScalars pixelScalars
vtkTransform aPixelTransform
  aPixelTransform Translate 0 0 2
vtkTransformFilter transformPixel
  transformPixel SetInput aPixelGrid
  transformPixel SetTransform aPixelTransform

vtkPoints quadPoints
  quadPoints SetNumberOfPoints 4
  quadPoints InsertPoint 0 0 0 0
  quadPoints InsertPoint 1 1 0 0
  quadPoints InsertPoint 2 1 1 0
  quadPoints InsertPoint 3 0 1 0
vtkScalars quadScalars
  quadScalars SetNumberOfScalars 4
  quadScalars InsertScalar 0 1
  quadScalars InsertScalar 1 0
  quadScalars InsertScalar 2 0
  quadScalars InsertScalar 3 0
vtkQuad aQuad
  [aQuad GetPointIds] SetId 0 0
  [aQuad GetPointIds] SetId 1 1
  [aQuad GetPointIds] SetId 2 2
  [aQuad GetPointIds] SetId 3 3
vtkUnstructuredGrid aQuadGrid
  aQuadGrid Allocate 1 1
  aQuadGrid InsertNextCell [aQuad GetCellType] [aQuad GetPointIds]
  aQuadGrid SetPoints quadPoints
  [aQuadGrid GetPointData] SetScalars quadScalars
vtkTransform aQuadTransform
  aQuadTransform Translate 2 0 2
vtkTransformFilter transformQuad
  transformQuad SetInput aQuadGrid
  transformQuad SetTransform aQuadTransform

vtkPoints trianglePoints
  trianglePoints SetNumberOfPoints 3
  trianglePoints InsertPoint 0 0 0 0
  trianglePoints InsertPoint 1 1 0 0
  trianglePoints InsertPoint 2 .5 .5 0
vtkScalars triangleScalars
  triangleScalars SetNumberOfScalars 3
  triangleScalars InsertScalar 0 1
  triangleScalars InsertScalar 1 0
  triangleScalars InsertScalar 2 0
vtkTriangle aTriangle
  [aTriangle GetPointIds] SetId 0 0
  [aTriangle GetPointIds] SetId 1 1
  [aTriangle GetPointIds] SetId 2 2
vtkUnstructuredGrid aTriangleGrid
  aTriangleGrid Allocate 1 1
  aTriangleGrid InsertNextCell [aTriangle GetCellType] [aTriangle GetPointIds]
  aTriangleGrid SetPoints trianglePoints
  [aTriangleGrid GetPointData] SetScalars triangleScalars
vtkTransform aTriangleTransform
  aTriangleTransform Translate 4 0 2
vtkTransformFilter transformTriangle
  transformTriangle SetInput aTriangleGrid
  transformTriangle SetTransform aTriangleTransform

vtkPoints polygonPoints
  polygonPoints SetNumberOfPoints 4
  polygonPoints InsertPoint 0 0 0 0
  polygonPoints InsertPoint 1 1 0 0
  polygonPoints InsertPoint 2 1 1 0
  polygonPoints InsertPoint 3 0 1 0
vtkScalars polygonScalars
  polygonScalars SetNumberOfScalars 4
  polygonScalars InsertScalar 0 1
  polygonScalars InsertScalar 1 0
  polygonScalars InsertScalar 2 0
  polygonScalars InsertScalar 3 0
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
  [aPolygonGrid GetPointData] SetScalars polygonScalars
vtkTransform aPolygonTransform
  aPolygonTransform Translate 6 0 2
vtkTransformFilter transformPolygon
  transformPolygon SetInput aPolygonGrid
  transformPolygon SetTransform aPolygonTransform

vtkPoints triangleStripPoints
  triangleStripPoints SetNumberOfPoints 5
  triangleStripPoints InsertPoint 0 0 1 0
  triangleStripPoints InsertPoint 1 0 0 0
  triangleStripPoints InsertPoint 2 1 1 0
  triangleStripPoints InsertPoint 3 1 0 0
  triangleStripPoints InsertPoint 4 2 1 0
vtkScalars triangleStripScalars
  triangleStripScalars SetNumberOfScalars 5
  triangleStripScalars InsertScalar 0 1
  triangleStripScalars InsertScalar 1 0
  triangleStripScalars InsertScalar 2 0
  triangleStripScalars InsertScalar 3 0
  triangleStripScalars InsertScalar 4 0
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
  [aTriangleStripGrid GetPointData] SetScalars triangleStripScalars
vtkTransform aTriangleStripTransform
  aTriangleStripTransform Translate 8 0 2
vtkTransformFilter transformTriangleStrip
  transformTriangleStrip SetInput aTriangleStripGrid
  transformTriangleStrip SetTransform aTriangleStripTransform

vtkPoints linePoints
  linePoints SetNumberOfPoints 2
  linePoints InsertPoint 0 0 0 0
  linePoints InsertPoint 1 1 1 0
vtkScalars lineScalars
  lineScalars SetNumberOfScalars 2
  lineScalars InsertScalar 0 1
  lineScalars InsertScalar 1 0
vtkLine aLine
  [aLine GetPointIds] SetId 0 0
  [aLine GetPointIds] SetId 1 1
vtkUnstructuredGrid aLineGrid
  aLineGrid Allocate 1 1
  aLineGrid InsertNextCell [aLine GetCellType] [aLine GetPointIds]
  aLineGrid SetPoints linePoints
  [aLineGrid GetPointData] SetScalars lineScalars
vtkTransform aLineTransform
  aLineTransform Translate 0 0 4
vtkTransformFilter transformLine
  transformLine SetInput aLineGrid
  transformLine SetTransform aLineTransform

vtkPoints polyLinePoints
  polyLinePoints SetNumberOfPoints 3
  polyLinePoints InsertPoint 0 0 0 0
  polyLinePoints InsertPoint 1 1 1 0
  polyLinePoints InsertPoint 2 1 0 0
vtkScalars polyLineScalars
  polyLineScalars SetNumberOfScalars 3
  polyLineScalars InsertScalar 0 1
  polyLineScalars InsertScalar 1 0
  polyLineScalars InsertScalar 2 0
vtkPolyLine aPolyLine
  [aPolyLine GetPointIds] SetNumberOfIds 3
  [aPolyLine GetPointIds] SetId 0 0
  [aPolyLine GetPointIds] SetId 1 1
  [aPolyLine GetPointIds] SetId 2 2
vtkUnstructuredGrid aPolyLineGrid
  aPolyLineGrid Allocate 1 1
  aPolyLineGrid InsertNextCell [aPolyLine GetCellType] [aPolyLine GetPointIds]
  aPolyLineGrid SetPoints polyLinePoints
  [aPolyLineGrid GetPointData] SetScalars polyLineScalars
vtkTransform aPolyLineTransform
  aPolyLineTransform Translate 2 0 4
vtkTransformFilter transformPolyLine
  transformPolyLine SetInput aPolyLineGrid
  transformPolyLine SetTransform aPolyLineTransform

vtkPoints vertexPoints
  vertexPoints SetNumberOfPoints 1
  vertexPoints InsertPoint 0 0 0 0
vtkScalars vertexScalars
  vertexScalars SetNumberOfScalars 1
  vertexScalars InsertScalar 0 1
vtkVertex aVertex
  [aVertex GetPointIds] SetId 0 0
vtkUnstructuredGrid aVertexGrid
  aVertexGrid Allocate 1 1
  aVertexGrid InsertNextCell [aVertex GetCellType] [aVertex GetPointIds]
  aVertexGrid SetPoints vertexPoints
  [aVertexGrid GetPointData] SetScalars vertexScalars
vtkTransform aVertexTransform
  aVertexTransform Translate 0 0 6
vtkTransformFilter transformVertex
  transformVertex SetInput aVertexGrid
  transformVertex SetTransform aVertexTransform

vtkPoints polyVertexPoints
  polyVertexPoints SetNumberOfPoints 3
  polyVertexPoints InsertPoint 0 0 0 0
  polyVertexPoints InsertPoint 1 1 0 0
  polyVertexPoints InsertPoint 2 1 1 0
vtkScalars polyVertexScalars
  polyVertexScalars SetNumberOfScalars 3
  polyVertexScalars InsertScalar 0 1
  polyVertexScalars InsertScalar 1 0
  polyVertexScalars InsertScalar 2 0
vtkPolyVertex aPolyVertex
  [aPolyVertex GetPointIds] SetNumberOfIds 3
  [aPolyVertex GetPointIds] SetId 0 0
  [aPolyVertex GetPointIds] SetId 1 1
  [aPolyVertex GetPointIds] SetId 2 2
vtkUnstructuredGrid aPolyVertexGrid
  aPolyVertexGrid Allocate 1 1
  aPolyVertexGrid InsertNextCell [aPolyVertex GetCellType] [aPolyVertex GetPointIds]
  aPolyVertexGrid SetPoints polyVertexPoints
  [aPolyVertexGrid GetPointData] SetScalars polyVertexScalars
vtkTransform aPolyVertexTransform
  aPolyVertexTransform Translate 2 0 6
vtkTransformFilter transformPolyVertex
  transformPolyVertex SetInput aPolyVertexGrid
  transformPolyVertex SetTransform aPolyVertexTransform

# Okay, append the data
vtkAppendFilter append
  append AddInput [transformVoxel GetOutput]
  append AddInput [transformHexahedron GetOutput]
  append AddInput [transformTetra GetOutput]
  append AddInput [transformWedge GetOutput]
  append AddInput [transformPyramid GetOutput]
  append AddInput [transformPixel GetOutput]
  append AddInput [transformQuad GetOutput]
  append AddInput [transformTriangle GetOutput]
  append AddInput [transformPolygon GetOutput]
  append AddInput [transformTriangleStrip GetOutput]
  append AddInput [transformLine GetOutput]
  append AddInput [transformPolyLine GetOutput]
  append AddInput [transformVertex GetOutput]
  append AddInput [transformPolyVertex GetOutput]

# Write it out and display it
vtkUnstructuredGridWriter writer
  writer SetInput [append GetOutput]
  writer SetFileName "appendCells.vtk"
  writer Write
vtkDataSetMapper mapper
  mapper SetInput [append GetOutput]
vtkActor allCellsActor
  allCellsActor SetMapper mapper

# Read it in and display
vtkUnstructuredGridReader reader
  reader SetFileName "appendCells.vtk"
vtkDataSetMapper mapper2
  mapper2 SetInput [reader GetOutput]
vtkActor allCellsActor2
  allCellsActor2 SetMapper mapper2

# graphics stuff
vtkRenderer ren1
  ren1 SetViewport 0 0 0.5 1.0
vtkRenderer ren2
  ren2 SetViewport 0.5 0 1.0 1.0
vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin AddRenderer ren2
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

ren1 SetBackground 1 1 1
ren2 SetBackground 1 1 1
renWin SetSize 750 250

ren1 AddActor allCellsActor
ren2 AddActor allCellsActor2
renWin Render

[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
[ren1 GetActiveCamera] Dolly 2

[ren2 GetActiveCamera] Azimuth 30
[ren2 GetActiveCamera] Elevation 20
[ren2 GetActiveCamera] Dolly 2


# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .

renWin SetFileName "appendCells.tcl.ppm"
#renWin SaveImageAsPPM

