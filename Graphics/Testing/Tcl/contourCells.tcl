package require vtk
package require vtkinteraction
package require vtktesting

# Contour every cell type


# Since some of our actors are a single vertex, we need to remove all
# cullers so the single vertex actors will render
vtkRenderer ren1
[ren1 GetCullers] RemoveAllItems

vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create a scene with one of each cell type
# Voxel

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

vtkFloatArray voxelScalars
  voxelScalars SetNumberOfTuples 8
  voxelScalars InsertValue 0 0
  voxelScalars InsertValue 1 1
  voxelScalars InsertValue 2 0
  voxelScalars InsertValue 3 0
  voxelScalars InsertValue 4 0
  voxelScalars InsertValue 5 0
  voxelScalars InsertValue 6 0
  voxelScalars InsertValue 7 0

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

vtkContourFilter voxelContours
  voxelContours SetInput aVoxelGrid
  voxelContours SetValue 0 .5

vtkDataSetMapper aVoxelContourMapper
  aVoxelContourMapper SetInputConnection [voxelContours GetOutputPort]
  aVoxelContourMapper ScalarVisibilityOff

vtkDataSetMapper aVoxelMapper
  aVoxelMapper SetInput aVoxelGrid
  aVoxelMapper ScalarVisibilityOff

vtkActor aVoxelActor
  aVoxelActor SetMapper aVoxelMapper
  [aVoxelActor GetProperty] SetRepresentationToWireframe

vtkActor aVoxelContourActor
  aVoxelContourActor SetMapper aVoxelContourMapper
  [aVoxelContourActor GetProperty] BackfaceCullingOn

# Hexahedron

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
  
vtkFloatArray hexahedronScalars
  hexahedronScalars SetNumberOfTuples 8
  hexahedronScalars InsertValue 0 0
  hexahedronScalars InsertValue 1 1
  hexahedronScalars InsertValue 2 0
  hexahedronScalars InsertValue 3 0
  hexahedronScalars InsertValue 4 0
  hexahedronScalars InsertValue 5 0
  hexahedronScalars InsertValue 6 0
  hexahedronScalars InsertValue 7 0

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

vtkContourFilter hexahedronContours
  hexahedronContours SetInput aHexahedronGrid
  hexahedronContours SetValue 0 .5

vtkDataSetMapper aHexahedronContourMapper
  aHexahedronContourMapper SetInputConnection [hexahedronContours GetOutputPort]
  aHexahedronContourMapper ScalarVisibilityOff

vtkDataSetMapper aHexahedronMapper
  aHexahedronMapper SetInput aHexahedronGrid
  aHexahedronMapper ScalarVisibilityOff

vtkActor aHexahedronActor
  aHexahedronActor SetMapper aHexahedronMapper
  [aHexahedronActor GetProperty] BackfaceCullingOn
  [aHexahedronActor GetProperty] SetRepresentationToWireframe

vtkActor aHexahedronContourActor
  aHexahedronContourActor SetMapper aHexahedronContourMapper
  [aHexahedronContourActor GetProperty] BackfaceCullingOn

# Tetra

vtkPoints tetraPoints
  tetraPoints SetNumberOfPoints 4
  tetraPoints InsertPoint 0 0 0 0
  tetraPoints InsertPoint 1 1 0 0
  tetraPoints InsertPoint 2 .5 1 0
  tetraPoints InsertPoint 3 .5 .5 1

vtkFloatArray tetraScalars
  tetraScalars SetNumberOfTuples 4
  tetraScalars InsertValue 0 1
  tetraScalars InsertValue 1 0
  tetraScalars InsertValue 2 0
  tetraScalars InsertValue 3 0

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

vtkContourFilter tetraContours
  tetraContours SetInput aTetraGrid
  tetraContours SetValue 0 .5

vtkDataSetMapper aTetraContourMapper
  aTetraContourMapper SetInputConnection [tetraContours GetOutputPort]
  aTetraContourMapper ScalarVisibilityOff

vtkDataSetMapper aTetraMapper
  aTetraMapper SetInput aTetraGrid
  aTetraMapper ScalarVisibilityOff

vtkActor aTetraContourActor
  aTetraContourActor SetMapper aTetraContourMapper

vtkActor aTetraActor
  aTetraActor SetMapper aTetraMapper
  [aTetraActor GetProperty] SetRepresentationToWireframe

# Wedge

vtkPoints wedgePoints
  wedgePoints SetNumberOfPoints 6
  wedgePoints InsertPoint 0 0 1 0
  wedgePoints InsertPoint 1 0 0 0
  wedgePoints InsertPoint 2 0 .5 .5
  wedgePoints InsertPoint 3 1 1 0
  wedgePoints InsertPoint 4 1 0 0
  wedgePoints InsertPoint 5 1 .5 .5

vtkFloatArray wedgeScalars
  wedgeScalars SetNumberOfTuples 6
  wedgeScalars InsertValue 0 1
  wedgeScalars InsertValue 1 1
  wedgeScalars InsertValue 2 0
  wedgeScalars InsertValue 3 1
  wedgeScalars InsertValue 4 1
  wedgeScalars InsertValue 5 0

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

vtkContourFilter wedgeContours
  wedgeContours SetInput aWedgeGrid
  wedgeContours SetValue 0 .5

vtkDataSetMapper aWedgeContourMapper
  aWedgeContourMapper SetInputConnection [wedgeContours GetOutputPort]
  aWedgeContourMapper ScalarVisibilityOff

vtkDataSetMapper aWedgeMapper
  aWedgeMapper SetInput aWedgeGrid
  aWedgeMapper ScalarVisibilityOff

vtkActor aWedgeContourActor
  aWedgeContourActor SetMapper aWedgeContourMapper

vtkActor aWedgeActor
  aWedgeActor SetMapper aWedgeMapper
  [aWedgeActor GetProperty] SetRepresentationToWireframe

# Pyramid

vtkPoints pyramidPoints
  pyramidPoints SetNumberOfPoints 5
  pyramidPoints InsertPoint 0 0 0 0
  pyramidPoints InsertPoint 1 1 0 0
  pyramidPoints InsertPoint 2 1 1 0
  pyramidPoints InsertPoint 3 0 1 0
  pyramidPoints InsertPoint 4 .5 .5 1

vtkFloatArray pyramidScalars
  pyramidScalars SetNumberOfTuples 5
  pyramidScalars InsertValue 0 1
  pyramidScalars InsertValue 1 1
  pyramidScalars InsertValue 2 1
  pyramidScalars InsertValue 3 1
  pyramidScalars InsertValue 4 0

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

vtkContourFilter pyramidContours
  pyramidContours SetInput aPyramidGrid
  pyramidContours SetValue 0 .5

vtkDataSetMapper aPyramidContourMapper
  aPyramidContourMapper SetInputConnection [pyramidContours GetOutputPort]
  aPyramidContourMapper ScalarVisibilityOff

vtkDataSetMapper aPyramidMapper
  aPyramidMapper SetInput aPyramidGrid
  aPyramidMapper ScalarVisibilityOff

vtkActor aPyramidContourActor
  aPyramidContourActor SetMapper aPyramidContourMapper

vtkActor aPyramidActor
  aPyramidActor SetMapper aPyramidMapper
  [aPyramidActor GetProperty] SetRepresentationToWireframe

# Pixel

vtkPoints pixelPoints
  pixelPoints SetNumberOfPoints 4
  pixelPoints InsertPoint 0 0 0 0
  pixelPoints InsertPoint 1 1 0 0
  pixelPoints InsertPoint 2 0 1 0
  pixelPoints InsertPoint 3 1 1 0

vtkFloatArray pixelScalars
  pixelScalars SetNumberOfTuples 4
  pixelScalars InsertValue 0 1
  pixelScalars InsertValue 1 0
  pixelScalars InsertValue 2 0
  pixelScalars InsertValue 3 0

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

vtkContourFilter pixelContours
  pixelContours SetInput aPixelGrid
  pixelContours SetValue 0 .5

vtkDataSetMapper aPixelContourMapper
  aPixelContourMapper SetInputConnection [pixelContours GetOutputPort]
  aPixelContourMapper ScalarVisibilityOff

vtkDataSetMapper aPixelMapper
  aPixelMapper SetInput aPixelGrid
  aPixelMapper ScalarVisibilityOff

vtkActor aPixelContourActor
  aPixelContourActor SetMapper aPixelContourMapper

vtkActor aPixelActor
  aPixelActor SetMapper aPixelMapper
  [aPixelActor GetProperty] BackfaceCullingOn
  [aPixelActor GetProperty] SetRepresentationToWireframe

# Quad

vtkPoints quadPoints
  quadPoints SetNumberOfPoints 4
  quadPoints InsertPoint 0 0 0 0
  quadPoints InsertPoint 1 1 0 0
  quadPoints InsertPoint 2 1 1 0
  quadPoints InsertPoint 3 0 1 0

vtkFloatArray quadScalars
  quadScalars SetNumberOfTuples 4
  quadScalars InsertValue 0 1
  quadScalars InsertValue 1 0
  quadScalars InsertValue 2 0
  quadScalars InsertValue 3 0

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

vtkContourFilter quadContours
  quadContours SetInput aQuadGrid
  quadContours SetValue 0 .5

vtkDataSetMapper aQuadContourMapper
  aQuadContourMapper SetInputConnection [quadContours GetOutputPort]
  aQuadContourMapper ScalarVisibilityOff

vtkDataSetMapper aQuadMapper
  aQuadMapper SetInput aQuadGrid
  aQuadMapper ScalarVisibilityOff

vtkActor aQuadContourActor
  aQuadContourActor SetMapper aQuadContourMapper

vtkActor aQuadActor
  aQuadActor SetMapper aQuadMapper
  [aQuadActor GetProperty] BackfaceCullingOn
  [aQuadActor GetProperty] SetRepresentationToWireframe

# Triangle

vtkPoints trianglePoints
  trianglePoints SetNumberOfPoints 3
  trianglePoints InsertPoint 0 0 0 0
  trianglePoints InsertPoint 1 1 0 0
  trianglePoints InsertPoint 2 .5 .5 0

vtkFloatArray triangleScalars
  triangleScalars SetNumberOfTuples 3
  triangleScalars InsertValue 0 1
  triangleScalars InsertValue 1 0
  triangleScalars InsertValue 2 0

vtkTriangle aTriangle
  [aTriangle GetPointIds] SetId 0 0
  [aTriangle GetPointIds] SetId 1 1
  [aTriangle GetPointIds] SetId 2 2

vtkUnstructuredGrid aTriangleGrid
  aTriangleGrid Allocate 1 1
  aTriangleGrid InsertNextCell [aTriangle GetCellType] [aTriangle GetPointIds]
  aTriangleGrid SetPoints trianglePoints
  [aTriangleGrid GetPointData] SetScalars triangleScalars

vtkContourFilter triangleContours
  triangleContours SetInput aTriangleGrid
  triangleContours SetValue 0 .5

vtkDataSetMapper aTriangleContourMapper
  aTriangleContourMapper SetInputConnection [triangleContours GetOutputPort]
  aTriangleContourMapper ScalarVisibilityOff

vtkActor aTriangleContourActor
  aTriangleContourActor SetMapper aTriangleContourMapper

vtkDataSetMapper aTriangleMapper
  aTriangleMapper SetInput aTriangleGrid
  aTriangleMapper ScalarVisibilityOff

vtkActor aTriangleActor
  aTriangleActor SetMapper aTriangleMapper
  [aTriangleActor GetProperty] BackfaceCullingOn
  [aTriangleActor GetProperty] SetRepresentationToWireframe

# Polygon

vtkPoints polygonPoints
  polygonPoints SetNumberOfPoints 4
  polygonPoints InsertPoint 0 0 0 0
  polygonPoints InsertPoint 1 1 0 0
  polygonPoints InsertPoint 2 1 1 0
  polygonPoints InsertPoint 3 0 1 0

vtkFloatArray polygonScalars
  polygonScalars SetNumberOfTuples 4
  polygonScalars InsertValue 0 1
  polygonScalars InsertValue 1 0
  polygonScalars InsertValue 2 0
  polygonScalars InsertValue 3 0

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

vtkContourFilter polygonContours
  polygonContours SetInput aPolygonGrid
  polygonContours SetValue 0 .5

vtkDataSetMapper aPolygonContourMapper
  aPolygonContourMapper SetInputConnection [polygonContours GetOutputPort]
  aPolygonContourMapper ScalarVisibilityOff

vtkDataSetMapper aPolygonMapper
  aPolygonMapper SetInput aPolygonGrid
  aPolygonMapper ScalarVisibilityOff

vtkActor aPolygonContourActor
  aPolygonContourActor SetMapper aPolygonContourMapper

vtkActor aPolygonActor
  aPolygonActor SetMapper aPolygonMapper
  [aPolygonActor GetProperty] BackfaceCullingOn
  [aPolygonActor GetProperty] SetRepresentationToWireframe

# Triangle strip

vtkPoints triangleStripPoints
  triangleStripPoints SetNumberOfPoints 5
  triangleStripPoints InsertPoint 0 0 1 0
  triangleStripPoints InsertPoint 1 0 0 0
  triangleStripPoints InsertPoint 2 1 1 0
  triangleStripPoints InsertPoint 3 1 0 0
  triangleStripPoints InsertPoint 4 2 1 0

vtkFloatArray triangleStripScalars
  triangleStripScalars SetNumberOfTuples 5
  triangleStripScalars InsertValue 0 1
  triangleStripScalars InsertValue 1 0
  triangleStripScalars InsertValue 2 0
  triangleStripScalars InsertValue 3 0
  triangleStripScalars InsertValue 4 0

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

vtkDataSetMapper aTriangleStripMapper
  aTriangleStripMapper SetInput aTriangleStripGrid
  aTriangleStripMapper ScalarVisibilityOff

vtkContourFilter triangleStripContours
  triangleStripContours SetInput aTriangleStripGrid
  triangleStripContours SetValue 0 .5

vtkDataSetMapper aTriangleStripContourMapper
  aTriangleStripContourMapper SetInputConnection [triangleStripContours GetOutputPort]
  aTriangleStripContourMapper ScalarVisibilityOff

vtkActor aTriangleStripContourActor
  aTriangleStripContourActor SetMapper aTriangleStripContourMapper

vtkActor aTriangleStripActor
  aTriangleStripActor SetMapper aTriangleStripMapper
  [aTriangleStripActor GetProperty] BackfaceCullingOn
  [aTriangleStripActor GetProperty] SetRepresentationToWireframe

# Line

vtkPoints linePoints
  linePoints SetNumberOfPoints 2
  linePoints InsertPoint 0 0 0 0
  linePoints InsertPoint 1 1 1 0

vtkFloatArray lineScalars
  lineScalars SetNumberOfTuples 2
  lineScalars InsertValue 0 1
  lineScalars InsertValue 1 0

vtkLine aLine
  [aLine GetPointIds] SetId 0 0
  [aLine GetPointIds] SetId 1 1

vtkUnstructuredGrid aLineGrid
  aLineGrid Allocate 1 1
  aLineGrid InsertNextCell [aLine GetCellType] [aLine GetPointIds]
  aLineGrid SetPoints linePoints
  [aLineGrid GetPointData] SetScalars lineScalars

vtkContourFilter lineContours
  lineContours SetInput aLineGrid
  lineContours SetValue 0 .5

vtkDataSetMapper aLineContourMapper
  aLineContourMapper SetInputConnection [lineContours GetOutputPort]
  aLineContourMapper ScalarVisibilityOff

vtkActor aLineContourActor
  aLineContourActor SetMapper aLineContourMapper

vtkDataSetMapper aLineMapper
  aLineMapper SetInput aLineGrid
  aLineMapper ScalarVisibilityOff

vtkActor aLineActor
  aLineActor SetMapper aLineMapper
  [aLineActor GetProperty] BackfaceCullingOn
  [aLineActor GetProperty] SetRepresentationToWireframe

# Polyline

vtkPoints polyLinePoints
  polyLinePoints SetNumberOfPoints 3
  polyLinePoints InsertPoint 0 0 0 0
  polyLinePoints InsertPoint 1 1 1 0
  polyLinePoints InsertPoint 2 1 0 0

vtkFloatArray polyLineScalars
  polyLineScalars SetNumberOfTuples 3
  polyLineScalars InsertValue 0 1
  polyLineScalars InsertValue 1 0
  polyLineScalars InsertValue 2 0

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

vtkContourFilter polyLineContours
  polyLineContours SetInput aPolyLineGrid
  polyLineContours SetValue 0 .5

vtkDataSetMapper aPolyLineContourMapper
  aPolyLineContourMapper SetInputConnection [polyLineContours GetOutputPort]
  aPolyLineContourMapper ScalarVisibilityOff

vtkActor aPolyLineContourActor
  aPolyLineContourActor SetMapper aPolyLineContourMapper

vtkDataSetMapper aPolyLineMapper
  aPolyLineMapper SetInput aPolyLineGrid
  aPolyLineMapper ScalarVisibilityOff

vtkActor aPolyLineActor
  aPolyLineActor SetMapper aPolyLineMapper
  [aPolyLineActor GetProperty] BackfaceCullingOn
  [aPolyLineActor GetProperty] SetRepresentationToWireframe

# Vertex

vtkPoints vertexPoints
  vertexPoints SetNumberOfPoints 1
  vertexPoints InsertPoint 0 0 0 0

vtkFloatArray vertexScalars
  vertexScalars SetNumberOfTuples 1
  vertexScalars InsertValue 0 1

vtkVertex aVertex
  [aVertex GetPointIds] SetId 0 0

vtkUnstructuredGrid aVertexGrid
  aVertexGrid Allocate 1 1
  aVertexGrid InsertNextCell [aVertex GetCellType] [aVertex GetPointIds]
  aVertexGrid SetPoints vertexPoints
  [aVertexGrid GetPointData] SetScalars vertexScalars

vtkContourFilter vertexContours
  vertexContours SetInput aVertexGrid
  vertexContours SetValue 0 1

vtkDataSetMapper aVertexContourMapper
  aVertexContourMapper SetInputConnection [vertexContours GetOutputPort]
  aVertexContourMapper ScalarVisibilityOff

vtkActor aVertexContourActor
  aVertexContourActor SetMapper aVertexContourMapper
  [aVertexContourActor GetProperty] SetRepresentationToWireframe

vtkDataSetMapper aVertexMapper
  aVertexMapper SetInput aVertexGrid
  aVertexMapper ScalarVisibilityOff

vtkActor aVertexActor
  aVertexActor SetMapper aVertexMapper
  [aVertexActor GetProperty] BackfaceCullingOn

# Poly Vertex

vtkPoints polyVertexPoints
  polyVertexPoints SetNumberOfPoints 3
  polyVertexPoints InsertPoint 0 0 0 0
  polyVertexPoints InsertPoint 1 1 0 0
  polyVertexPoints InsertPoint 2 1 1 0

vtkFloatArray polyVertexScalars
  polyVertexScalars SetNumberOfTuples 3
  polyVertexScalars InsertValue 0 1
  polyVertexScalars InsertValue 1 0
  polyVertexScalars InsertValue 2 0

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

vtkContourFilter polyVertexContours
  polyVertexContours SetInput aPolyVertexGrid
  polyVertexContours SetValue 0 0

vtkDataSetMapper aPolyVertexContourMapper
  aPolyVertexContourMapper SetInputConnection [polyVertexContours GetOutputPort]
  aPolyVertexContourMapper ScalarVisibilityOff

vtkActor aPolyVertexContourActor
  aPolyVertexContourActor SetMapper aPolyVertexContourMapper
  [aPolyVertexContourActor GetProperty] SetRepresentationToWireframe

vtkDataSetMapper aPolyVertexMapper
  aPolyVertexMapper SetInput aPolyVertexGrid
  aPolyVertexMapper ScalarVisibilityOff

vtkActor aPolyVertexActor
  aPolyVertexActor SetMapper aPolyVertexMapper

# Pentagonal prism

vtkPoints pentaPoints
  pentaPoints SetNumberOfPoints 10
  pentaPoints InsertPoint 0 0.25 0.0 0.0  
  pentaPoints InsertPoint 1 0.75 0.0 0.0 
  pentaPoints InsertPoint 2 1.0  0.5 0.0  
  pentaPoints InsertPoint 3 0.5  1.0 0.0 
  pentaPoints InsertPoint 4 0.0  0.5 0.0  
  pentaPoints InsertPoint 5 0.25 0.0 1.0  
  pentaPoints InsertPoint 6 0.75 0.0 1.0  
  pentaPoints InsertPoint 7 1.0  0.5 1.0  
  pentaPoints InsertPoint 8 0.5  1.0 1.0 
  pentaPoints InsertPoint 9 0.0  0.5 1.0 

vtkFloatArray pentaScalars
  pentaScalars SetNumberOfTuples 10
  pentaScalars InsertValue 0 0
  pentaScalars InsertValue 1 0
  pentaScalars InsertValue 2 0
  pentaScalars InsertValue 3 0
  pentaScalars InsertValue 4 0
  pentaScalars InsertValue 5 1
  pentaScalars InsertValue 6 1
  pentaScalars InsertValue 7 1
  pentaScalars InsertValue 8 1
  pentaScalars InsertValue 9 1

vtkPentagonalPrism aPenta
  [aPenta GetPointIds] SetId 0 0
  [aPenta GetPointIds] SetId 1 1
  [aPenta GetPointIds] SetId 2 2
  [aPenta GetPointIds] SetId 3 3
  [aPenta GetPointIds] SetId 4 4
  [aPenta GetPointIds] SetId 5 5
  [aPenta GetPointIds] SetId 6 6
  [aPenta GetPointIds] SetId 7 7
  [aPenta GetPointIds] SetId 8 8
  [aPenta GetPointIds] SetId 9 9

  
vtkUnstructuredGrid aPentaGrid
  aPentaGrid Allocate 1 1
  aPentaGrid InsertNextCell [aPenta GetCellType] [aPenta GetPointIds]
  aPentaGrid SetPoints pentaPoints
  [aPentaGrid GetPointData] SetScalars pentaScalars

vtkContourFilter pentaContours
  pentaContours SetInput aPentaGrid
  pentaContours SetValue 0 .5

vtkDataSetMapper aPentaContourMapper
  aPentaContourMapper SetInputConnection [pentaContours GetOutputPort]
  aPentaContourMapper ScalarVisibilityOff

vtkDataSetMapper aPentaMapper
  aPentaMapper SetInput aPentaGrid
  aPentaMapper ScalarVisibilityOff

vtkActor aPentaActor
  aPentaActor SetMapper aPentaMapper
  [aPentaActor GetProperty] BackfaceCullingOn
  [aPentaActor GetProperty] SetRepresentationToWireframe

vtkActor aPentaContourActor
  aPentaContourActor SetMapper aPentaContourMapper
  [aPentaContourActor GetProperty] BackfaceCullingOn


# Hexagonal prism

vtkPoints hexaPoints
  hexaPoints SetNumberOfPoints 12
  hexaPoints InsertPoint 0 0.0 0.0 0.0  
  hexaPoints InsertPoint 1 0.5 0.0 0.0 
  hexaPoints InsertPoint 2 1.0 0.5 0.0  
  hexaPoints InsertPoint 3 1.0 1.0 0.0 
  hexaPoints InsertPoint 4 0.5 1.0 0.0  
  hexaPoints InsertPoint 5 0.0 0.5 0.0 
  hexaPoints InsertPoint 6 0.0 0.0 1.0  
  hexaPoints InsertPoint 7 0.5 0.0 1.0 
  hexaPoints InsertPoint 8 1.0 0.5 1.0  
  hexaPoints InsertPoint 9 1.0 1.0 1.0 
  hexaPoints InsertPoint 10 0.5 1.0 1.0  
  hexaPoints InsertPoint 11 0.0 0.5 1.0
                         
vtkFloatArray hexaScalars
  hexaScalars SetNumberOfTuples 12
  hexaScalars InsertValue 0 0
  hexaScalars InsertValue 1 0
  hexaScalars InsertValue 2 0
  hexaScalars InsertValue 3 0
  hexaScalars InsertValue 4 0
  hexaScalars InsertValue 5 0
  hexaScalars InsertValue 6 1
  hexaScalars InsertValue 7 1
  hexaScalars InsertValue 8 1
  hexaScalars InsertValue 9 1
  hexaScalars InsertValue 10 1
  hexaScalars InsertValue 11 1

vtkHexagonalPrism aHexa
  [aHexa GetPointIds] SetId 0 0
  [aHexa GetPointIds] SetId 1 1
  [aHexa GetPointIds] SetId 2 2
  [aHexa GetPointIds] SetId 3 3
  [aHexa GetPointIds] SetId 4 4
  [aHexa GetPointIds] SetId 5 5
  [aHexa GetPointIds] SetId 6 6
  [aHexa GetPointIds] SetId 7 7
  [aHexa GetPointIds] SetId 8 8
  [aHexa GetPointIds] SetId 9 9
  [aHexa GetPointIds] SetId 10 10
  [aHexa GetPointIds] SetId 11 11

  
vtkUnstructuredGrid aHexaGrid
  aHexaGrid Allocate 1 1
  aHexaGrid InsertNextCell [aHexa GetCellType] [aHexa GetPointIds]
  aHexaGrid SetPoints hexaPoints
  [aHexaGrid GetPointData] SetScalars hexaScalars

vtkContourFilter hexaContours
  hexaContours SetInput aHexaGrid
  hexaContours SetValue 0 .5

vtkDataSetMapper aHexaContourMapper
  aHexaContourMapper SetInputConnection [hexaContours GetOutputPort]
  aHexaContourMapper ScalarVisibilityOff

vtkDataSetMapper aHexaMapper
  aHexaMapper SetInput aHexaGrid
  aHexaMapper ScalarVisibilityOff

vtkActor aHexaActor
  aHexaActor SetMapper aHexaMapper
  [aHexaActor GetProperty] BackfaceCullingOn
  [aHexaActor GetProperty] SetRepresentationToWireframe

vtkActor aHexaContourActor
  aHexaContourActor SetMapper aHexaContourMapper
  [aHexaContourActor GetProperty] BackfaceCullingOn



ren1 SetBackground .1 .2 .3
renWin SetSize 400 400

ren1 AddActor aVoxelActor; [aVoxelActor GetProperty] SetDiffuseColor 1 0 0
ren1 AddActor aVoxelContourActor; [aVoxelContourActor GetProperty] SetDiffuseColor 1 0 0

ren1 AddActor aHexahedronActor; [aHexahedronActor GetProperty] SetDiffuseColor 1 1 0
ren1 AddActor aHexahedronContourActor; [aHexahedronContourActor GetProperty] SetDiffuseColor 1 1 0

ren1 AddActor aTetraActor; [aTetraActor GetProperty] SetDiffuseColor 0 1 0
ren1 AddActor aTetraContourActor; [aTetraContourActor GetProperty] SetDiffuseColor 0 1 0

ren1 AddActor aWedgeActor; [aWedgeActor GetProperty] SetDiffuseColor 0 1 1
ren1 AddActor aWedgeContourActor; [aWedgeContourActor GetProperty] SetDiffuseColor 0 1 1

ren1 AddActor aPyramidActor; [aPyramidActor GetProperty] SetDiffuseColor 1 0 1
ren1 AddActor aPyramidContourActor; [aPyramidContourActor GetProperty] SetDiffuseColor 1 0 1

ren1 AddActor aPixelActor; [aPixelActor GetProperty] SetDiffuseColor 0 1 1
ren1 AddActor aPixelContourActor; [aPixelContourActor GetProperty] SetDiffuseColor 0 1 1

ren1 AddActor aQuadActor; [aQuadActor GetProperty] SetDiffuseColor 1 0 1
ren1 AddActor aQuadContourActor; [aQuadContourActor GetProperty] SetDiffuseColor 1 0 1

ren1 AddActor aTriangleActor; [aTriangleActor GetProperty] SetDiffuseColor .3 1 .5
ren1 AddActor aTriangleContourActor; [aTriangleContourActor GetProperty] SetDiffuseColor .3 1 .5

ren1 AddActor aPolygonActor; [aPolygonActor GetProperty] SetDiffuseColor 1 .4 .5
ren1 AddActor aPolygonContourActor; [aPolygonContourActor GetProperty] SetDiffuseColor 1 .4 .5

ren1 AddActor aTriangleStripActor; [aTriangleStripActor GetProperty] SetDiffuseColor .3 .7 1
ren1 AddActor aTriangleStripContourActor; [aTriangleStripContourActor GetProperty] SetDiffuseColor .3 .7 1

ren1 AddActor aLineActor; [aLineActor GetProperty] SetDiffuseColor .2 1 1
ren1 AddActor aLineContourActor; [aLineContourActor GetProperty] SetDiffuseColor .2 1 1

ren1 AddActor aPolyLineActor; [aPolyLineActor GetProperty] SetDiffuseColor 1 1 1
ren1 AddActor aPolyLineContourActor; [aPolyLineContourActor GetProperty] SetDiffuseColor 1 1 1

ren1 AddActor aVertexActor; [aVertexActor GetProperty] SetDiffuseColor 1 1 1
ren1 AddActor aVertexContourActor; [aVertexContourActor GetProperty] SetDiffuseColor 1 1 1

ren1 AddActor aPolyVertexActor; [aPolyVertexActor GetProperty] SetDiffuseColor 1 1 1
ren1 AddActor aPolyVertexContourActor; [aPolyVertexContourActor GetProperty] SetDiffuseColor 1 1 1

ren1 AddActor aPentaActor; [aPentaActor GetProperty] SetDiffuseColor .2 .4 .7
ren1 AddActor aPentaContourActor; [aPentaContourActor GetProperty] SetDiffuseColor .2 .4 .7

ren1 AddActor aHexaActor; [aHexaActor GetProperty] SetDiffuseColor .7 .5 1
ren1 AddActor aHexaContourActor; [aHexaContourActor GetProperty] SetDiffuseColor .7 .5 1

# places everyone!!
aVoxelContourActor AddPosition 0 0 0
aVoxelContourActor AddPosition 0 2 0
aHexahedronContourActor AddPosition 2 0 0
aHexahedronContourActor AddPosition 0 2 0
aHexahedronActor AddPosition 2 0 0
aTetraContourActor AddPosition 4 0 0
aTetraContourActor AddPosition 0 2 0
aTetraActor AddPosition 4 0 0
aWedgeContourActor AddPosition 6 0 0
aWedgeContourActor AddPosition 0 2 0
aWedgeActor AddPosition 6 0 0
aPyramidContourActor AddPosition 8 0 0
aPyramidContourActor AddPosition 0 2 0
aPyramidActor AddPosition 8 0 0

aPixelContourActor AddPosition 0 4 0
aPixelContourActor AddPosition 0 2 0
aPixelActor AddPosition 0 4 0
aQuadContourActor AddPosition 2 4 0
aQuadContourActor AddPosition 0 2 0
aQuadActor AddPosition 2 4 0
aTriangleContourActor AddPosition 4 4 0
aTriangleContourActor AddPosition 0 2 0
aTriangleActor AddPosition 4 4 0
aPolygonContourActor AddPosition 6 4 0
aPolygonContourActor AddPosition 0 2 0
aPolygonActor AddPosition 6 4 0
aTriangleStripContourActor AddPosition 8 4 0
aTriangleStripContourActor AddPosition 0 2 0
aTriangleStripActor AddPosition 8 4 0

aLineContourActor AddPosition 0 8 0
aLineContourActor AddPosition 0 2 0
aLineActor AddPosition 0 8 0
aPolyLineContourActor AddPosition 2 8 0
aPolyLineContourActor AddPosition 0 2 0
aPolyLineActor AddPosition 2 8 0

aVertexContourActor AddPosition 0 12 0
aVertexContourActor AddPosition 0 2 0
aVertexActor AddPosition 0 12 0
aPolyVertexContourActor AddPosition 2 12 0
aPolyVertexContourActor AddPosition 0 2 0
aPolyVertexActor AddPosition 2 12 0

aPentaContourActor AddPosition 4 8 0
aPentaContourActor AddPosition 0 2 0
aPentaActor AddPosition 4 8 0
aHexaContourActor AddPosition 6 8 0
aHexaContourActor AddPosition 0 2 0
aHexaActor AddPosition 6 8 0


BuildBackdrop -1 11 -1 16 -1 2 .1

ren1 AddActor base
[base GetProperty] SetDiffuseColor .2 .2 .2
ren1 AddActor left
[left GetProperty] SetDiffuseColor .2 .2 .2
ren1 AddActor back
[back GetProperty] SetDiffuseColor .2 .2 .2

ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.5
ren1 ResetCameraClippingRange

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


