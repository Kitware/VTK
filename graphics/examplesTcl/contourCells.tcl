catch {load vtktcl}
# Contour every cell type

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

vtkRenderer ren1
vtkRenderWindow renWin
  renWin AddRenderer ren1
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

# create a scene with one of each cell type

vtkFloatPoints voxelPoints
  voxelPoints SetNumberOfPoints 8
  voxelPoints InsertPoint 0 0 0 0
  voxelPoints InsertPoint 1 1 0 0
  voxelPoints InsertPoint 2 0 1 0
  voxelPoints InsertPoint 3 1 1 0
  voxelPoints InsertPoint 4 0 0 1
  voxelPoints InsertPoint 5 1 0 1
  voxelPoints InsertPoint 6 0 1 1
  voxelPoints InsertPoint 7 1 1 1

vtkFloatScalars voxelScalars
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

vtkContourFilter voxelContours
  voxelContours SetInput aVoxelGrid
  voxelContours SetValue 0 .5

vtkDataSetMapper aVoxelContourMapper
  aVoxelContourMapper SetInput [voxelContours GetOutput]
  aVoxelContourMapper ScalarVisibilityOff

vtkDataSetMapper aVoxelMapper
  aVoxelMapper SetInput aVoxelGrid
  aVoxelMapper ScalarVisibilityOff

vtkActor aVoxelActor
  aVoxelActor SetMapper aVoxelMapper
  [aVoxelActor GetProperty] SetRepresentationToWireframe

vtkActor aVoxelContourActor
  aVoxelContourActor SetMapper aVoxelContourMapper
  aVoxelContourActor AddPosition 0 0 0
  aVoxelContourActor AddPosition 0 2 0
  [aVoxelContourActor GetProperty] BackfaceCullingOn

vtkFloatPoints hexahedronPoints
  hexahedronPoints SetNumberOfPoints 8
  hexahedronPoints InsertPoint 0 0 0 0
  hexahedronPoints InsertPoint 1 1 0 0
  hexahedronPoints InsertPoint 2 1 1 0
  hexahedronPoints InsertPoint 3 0 1 0
  hexahedronPoints InsertPoint 4 0 0 1
  hexahedronPoints InsertPoint 5 1 0 1
  hexahedronPoints InsertPoint 6 1 1 1
  hexahedronPoints InsertPoint 7 0 1 1
  
vtkFloatScalars hexahedronScalars
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

vtkContourFilter hexahedronContours
  hexahedronContours SetInput aHexahedronGrid
  hexahedronContours SetValue 0 .5

vtkDataSetMapper aHexahedronContourMapper
  aHexahedronContourMapper SetInput [hexahedronContours GetOutput]
  aHexahedronContourMapper ScalarVisibilityOff

vtkDataSetMapper aHexahedronMapper
  aHexahedronMapper SetInput aHexahedronGrid
  aHexahedronMapper ScalarVisibilityOff

vtkActor aHexahedronActor
  aHexahedronActor SetMapper aHexahedronMapper
  aHexahedronActor AddPosition 2 0 0
  [aHexahedronActor GetProperty] BackfaceCullingOn
  [aHexahedronActor GetProperty] SetRepresentationToWireframe

vtkActor aHexahedronContourActor
  aHexahedronContourActor SetMapper aHexahedronContourMapper
  aHexahedronContourActor AddPosition 2 0 0
  aHexahedronContourActor AddPosition 0 2 0
  [aHexahedronContourActor GetProperty] BackfaceCullingOn


vtkFloatPoints tetraPoints
  tetraPoints SetNumberOfPoints 4
  tetraPoints InsertPoint 0 0 0 0
  tetraPoints InsertPoint 1 1 0 0
  tetraPoints InsertPoint 2 .5 1 0
  tetraPoints InsertPoint 3 .5 .5 1

vtkFloatScalars tetraScalars
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

vtkContourFilter tetraContours
  tetraContours SetInput aTetraGrid
  tetraContours SetValue 0 .5

vtkDataSetMapper aTetraContourMapper
  aTetraContourMapper SetInput [tetraContours GetOutput]
  aTetraContourMapper ScalarVisibilityOff

vtkDataSetMapper aTetraMapper
  aTetraMapper SetInput aTetraGrid
  aTetraMapper ScalarVisibilityOff

vtkActor aTetraContourActor
  aTetraContourActor SetMapper aTetraContourMapper
  aTetraContourActor AddPosition 4 0 0
  aTetraContourActor AddPosition 0 2 0

vtkActor aTetraActor
  aTetraActor SetMapper aTetraMapper
  aTetraActor AddPosition 4 0 0
  [aTetraActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints pixelPoints
  pixelPoints SetNumberOfPoints 4
  pixelPoints InsertPoint 0 0 0 0
  pixelPoints InsertPoint 1 1 0 0
  pixelPoints InsertPoint 2 0 1 0
  pixelPoints InsertPoint 3 1 1 0

vtkFloatScalars pixelScalars
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

vtkContourFilter pixelContours
  pixelContours SetInput aPixelGrid
  pixelContours SetValue 0 .5

vtkDataSetMapper aPixelContourMapper
  aPixelContourMapper SetInput [pixelContours GetOutput]
  aPixelContourMapper ScalarVisibilityOff

vtkDataSetMapper aPixelMapper
  aPixelMapper SetInput aPixelGrid
  aPixelMapper ScalarVisibilityOff

vtkActor aPixelContourActor
  aPixelContourActor SetMapper aPixelContourMapper
  aPixelContourActor AddPosition 0 0 2
  aPixelContourActor AddPosition 0 2 0

vtkActor aPixelActor
  aPixelActor SetMapper aPixelMapper
  aPixelActor AddPosition 0 0 2
  [aPixelActor GetProperty] BackfaceCullingOn
  [aPixelActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints quadPoints
  quadPoints SetNumberOfPoints 4
  quadPoints InsertPoint 0 0 0 0
  quadPoints InsertPoint 1 1 0 0
  quadPoints InsertPoint 2 1 1 0
  quadPoints InsertPoint 3 0 1 0

vtkFloatScalars quadScalars
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

vtkContourFilter quadContours
  quadContours SetInput aQuadGrid
  quadContours SetValue 0 .5

vtkDataSetMapper aQuadContourMapper
  aQuadContourMapper SetInput [quadContours GetOutput]
  aQuadContourMapper ScalarVisibilityOff

vtkDataSetMapper aQuadMapper
  aQuadMapper SetInput aQuadGrid
  aQuadMapper ScalarVisibilityOff

vtkActor aQuadContourActor
  aQuadContourActor SetMapper aQuadContourMapper
  aQuadContourActor AddPosition 2 0 2
  aQuadContourActor AddPosition 0 2 0

vtkActor aQuadActor
  aQuadActor SetMapper aQuadMapper
  aQuadActor AddPosition 2 0 2
  [aQuadActor GetProperty] BackfaceCullingOn
  [aQuadActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints trianglePoints
  trianglePoints SetNumberOfPoints 3
  trianglePoints InsertPoint 0 0 0 0
  trianglePoints InsertPoint 1 1 0 0
  trianglePoints InsertPoint 2 .5 .5 0

vtkFloatScalars triangleScalars
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

vtkContourFilter triangleContours
  triangleContours SetInput aTriangleGrid
  triangleContours SetValue 0 .5

vtkDataSetMapper aTriangleContourMapper
  aTriangleContourMapper SetInput [triangleContours GetOutput]
  aTriangleContourMapper ScalarVisibilityOff

vtkActor aTriangleContourActor
  aTriangleContourActor SetMapper aTriangleContourMapper
  aTriangleContourActor AddPosition 4 0 2
  aTriangleContourActor AddPosition 0 2 0

vtkDataSetMapper aTriangleMapper
  aTriangleMapper SetInput aTriangleGrid
  aTriangleMapper ScalarVisibilityOff

vtkActor aTriangleActor
  aTriangleActor SetMapper aTriangleMapper
  aTriangleActor AddPosition 4 0 2
  [aTriangleActor GetProperty] BackfaceCullingOn
  [aTriangleActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints polygonPoints
  polygonPoints SetNumberOfPoints 4
  polygonPoints InsertPoint 0 0 0 0
  polygonPoints InsertPoint 1 1 0 0
  polygonPoints InsertPoint 2 1 1 0
  polygonPoints InsertPoint 3 0 1 0

vtkFloatScalars polygonScalars
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

vtkContourFilter polygonContours
  polygonContours SetInput aPolygonGrid
  polygonContours SetValue 0 .5

vtkDataSetMapper aPolygonContourMapper
  aPolygonContourMapper SetInput [polygonContours GetOutput]
  aPolygonContourMapper ScalarVisibilityOff

vtkDataSetMapper aPolygonMapper
  aPolygonMapper SetInput aPolygonGrid
  aPolygonMapper ScalarVisibilityOff

vtkActor aPolygonContourActor
  aPolygonContourActor SetMapper aPolygonContourMapper
  aPolygonContourActor AddPosition 6 0 2
  aPolygonContourActor AddPosition 0 2 0

vtkActor aPolygonActor
  aPolygonActor SetMapper aPolygonMapper
  aPolygonActor AddPosition 6 0 2
  [aPolygonActor GetProperty] BackfaceCullingOn
  [aPolygonActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints triangleStripPoints
  triangleStripPoints SetNumberOfPoints 5
  triangleStripPoints InsertPoint 0 0 1 0
  triangleStripPoints InsertPoint 1 0 0 0
  triangleStripPoints InsertPoint 2 1 1 0
  triangleStripPoints InsertPoint 3 1 0 0
  triangleStripPoints InsertPoint 4 2 1 0

vtkFloatScalars triangleStripScalars
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

vtkDataSetMapper aTriangleStripMapper
  aTriangleStripMapper SetInput aTriangleStripGrid
  aTriangleStripMapper ScalarVisibilityOff

vtkContourFilter triangleStripContours
  triangleStripContours SetInput aTriangleStripGrid
  triangleStripContours SetValue 0 .5

vtkDataSetMapper aTriangleStripContourMapper
  aTriangleStripContourMapper SetInput [triangleStripContours GetOutput]
  aTriangleStripContourMapper ScalarVisibilityOff

vtkActor aTriangleStripContourActor
  aTriangleStripContourActor SetMapper aTriangleStripContourMapper
  aTriangleStripContourActor AddPosition 8 0 2
  aTriangleStripContourActor AddPosition 0 2 0

vtkActor aTriangleStripActor
  aTriangleStripActor SetMapper aTriangleStripMapper
  aTriangleStripActor AddPosition 8 0 2
  [aTriangleStripActor GetProperty] BackfaceCullingOn
  [aTriangleStripActor GetProperty] SetRepresentationToWireframe

vtkFloatPoints linePoints
  linePoints SetNumberOfPoints 2
  linePoints InsertPoint 0 0 0 0
  linePoints InsertPoint 1 1 1 0

vtkFloatScalars lineScalars
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

vtkContourFilter lineContours
  lineContours SetInput aLineGrid
  lineContours SetValue 0 .5

vtkDataSetMapper aLineContourMapper
  aLineContourMapper SetInput [lineContours GetOutput]
  aLineContourMapper ScalarVisibilityOff

vtkActor aLineContourActor
  aLineContourActor SetMapper aLineContourMapper
  aLineContourActor AddPosition 0 0 4
  aLineContourActor AddPosition 0 2 0

vtkDataSetMapper aLineMapper
  aLineMapper SetInput aLineGrid
  aLineMapper ScalarVisibilityOff

vtkActor aLineActor
  aLineActor SetMapper aLineMapper
  aLineActor AddPosition 0 0 4
  [aLineActor GetProperty] BackfaceCullingOn
  [aLineActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints polyLinePoints
  polyLinePoints SetNumberOfPoints 3
  polyLinePoints InsertPoint 0 0 0 0
  polyLinePoints InsertPoint 1 1 1 0
  polyLinePoints InsertPoint 2 1 0 0

vtkFloatScalars polyLineScalars
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

vtkContourFilter polyLineContours
  polyLineContours SetInput aPolyLineGrid
  polyLineContours SetValue 0 .5

vtkDataSetMapper aPolyLineContourMapper
  aPolyLineContourMapper SetInput [polyLineContours GetOutput]
  aPolyLineContourMapper ScalarVisibilityOff

vtkActor aPolyLineContourActor
  aPolyLineContourActor SetMapper aPolyLineContourMapper
  aPolyLineContourActor AddPosition 2 0 4
  aPolyLineContourActor AddPosition 0 2 0

vtkDataSetMapper aPolyLineMapper
  aPolyLineMapper SetInput aPolyLineGrid
  aPolyLineMapper ScalarVisibilityOff

vtkActor aPolyLineActor
  aPolyLineActor SetMapper aPolyLineMapper
  aPolyLineActor AddPosition 2 0 4
  [aPolyLineActor GetProperty] BackfaceCullingOn
  [aPolyLineActor GetProperty] SetRepresentationToWireframe


vtkFloatPoints vertexPoints
  vertexPoints SetNumberOfPoints 1
  vertexPoints InsertPoint 0 0 0 0

vtkVertex aVertex
  [aVertex GetPointIds] SetId 0 0

vtkUnstructuredGrid aVertexGrid
  aVertexGrid Allocate 1 1
  aVertexGrid InsertNextCell [aVertex GetCellType] [aVertex GetPointIds]
  aVertexGrid SetPoints vertexPoints

vtkDataSetMapper aVertexMapper
  aVertexMapper SetInput aVertexGrid

vtkActor aVertexActor
  aVertexActor SetMapper aVertexMapper
  aVertexActor AddPosition 0 0 6
  [aVertexActor GetProperty] BackfaceCullingOn


vtkFloatPoints polyVertexPoints
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
  aPolyVertexMapper SetInput aPolyVertexGrid

vtkActor aPolyVertexActor
  aPolyVertexActor SetMapper aPolyVertexMapper
  aPolyVertexActor AddPosition 2 0 6
  [aPolyVertexActor GetProperty] BackfaceCullingOn


ren1 SetBackground .1 .2 .3
renWin SetSize 600 300

ren1 AddActor aVoxelActor; [aVoxelActor GetProperty] SetDiffuseColor 1 0 0
ren1 AddActor aVoxelContourActor; [aVoxelContourActor GetProperty] SetDiffuseColor 1 0 0

ren1 AddActor aHexahedronActor; [aHexahedronActor GetProperty] SetDiffuseColor 1 1 0
ren1 AddActor aHexahedronContourActor; [aHexahedronContourActor GetProperty] SetDiffuseColor 1 1 0

ren1 AddActor aTetraActor; [aTetraActor GetProperty] SetDiffuseColor 0 1 0
ren1 AddActor aTetraContourActor; [aTetraContourActor GetProperty] SetDiffuseColor 0 1 0

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

ren1 AddActor aPolyVertexActor; [aPolyVertexActor GetProperty] SetDiffuseColor 1 1 1

source backdrop.tcl
BuildBackdrop -1 11 -1 4 -1 7 .1
ren1 AddActor base
[base GetProperty] SetDiffuseColor .4 .4 .4
ren1 AddActor left
[left GetProperty] SetDiffuseColor .4 .4 .4
ren1 AddActor back
[left GetProperty] SetDiffuseColor .4 .4 .4

[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
[ren1 GetActiveCamera] Dolly 3
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .

#renWin SetFileName "contourCells.tcl.ppm"
#renWin SaveImageAsPPM

