#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKContribPython import *

# Demonstrates the 3D Studio Importer and all exporters


ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)


# create a scene with one of each cell type

voxelPoints = vtkPoints()
voxelPoints.SetNumberOfPoints(8)
voxelPoints.InsertPoint(0,0,0,0)
voxelPoints.InsertPoint(1,1,0,0)
voxelPoints.InsertPoint(2,0,1,0)
voxelPoints.InsertPoint(3,1,1,0)
voxelPoints.InsertPoint(4,0,0,1)
voxelPoints.InsertPoint(5,1,0,1)
voxelPoints.InsertPoint(6,0,1,1)
voxelPoints.InsertPoint(7,1,1,1)

aVoxel = vtkVoxel()
aVoxel.GetPointIds().SetId(0,0)
aVoxel.GetPointIds().SetId(1,1)
aVoxel.GetPointIds().SetId(2,2)
aVoxel.GetPointIds().SetId(3,3)
aVoxel.GetPointIds().SetId(4,4)
aVoxel.GetPointIds().SetId(5,5)
aVoxel.GetPointIds().SetId(6,6)
aVoxel.GetPointIds().SetId(7,7)


aVoxelGrid = vtkUnstructuredGrid()
aVoxelGrid.Allocate(1,1)
aVoxelGrid.InsertNextCell(aVoxel.GetCellType(), aVoxel.GetPointIds())
aVoxelGrid.SetPoints(voxelPoints)

aVoxelMapper = vtkDataSetMapper()
aVoxelMapper.SetInput(aVoxelGrid)

aVoxelActor = vtkActor()
aVoxelActor.SetMapper(aVoxelMapper)
aVoxelActor.GetProperty().BackfaceCullingOn()

hexahedronPoints = vtkPoints()
hexahedronPoints.SetNumberOfPoints(8)
hexahedronPoints.InsertPoint(0,0,0,0)
hexahedronPoints.InsertPoint(1,1,0,0)
hexahedronPoints.InsertPoint(2,1,1,0)
hexahedronPoints.InsertPoint(3,0,1,0)
hexahedronPoints.InsertPoint(4,0,0,1)
hexahedronPoints.InsertPoint(5,1,0,1)
hexahedronPoints.InsertPoint(6,1,1,1)
hexahedronPoints.InsertPoint(7,0,1,1)
  
aHexahedron = vtkHexahedron()
aHexahedron.GetPointIds().SetId(0,0)
aHexahedron.GetPointIds().SetId(1,1)
aHexahedron.GetPointIds().SetId(2,2)
aHexahedron.GetPointIds().SetId(3,3)
aHexahedron.GetPointIds().SetId(4,4)
aHexahedron.GetPointIds().SetId(5,5)
aHexahedron.GetPointIds().SetId(6,6)
aHexahedron.GetPointIds().SetId(7,7)
  
  
aHexahedronGrid = vtkUnstructuredGrid()
aHexahedronGrid.Allocate(1,1)
aHexahedronGrid.InsertNextCell(aHexahedron.GetCellType(), aHexahedron.GetPointIds())
aHexahedronGrid.SetPoints(hexahedronPoints)

aHexahedronMapper = vtkDataSetMapper()
aHexahedronMapper.SetInput(aHexahedronGrid)

aHexahedronActor = vtkActor()
aHexahedronActor.SetMapper(aHexahedronMapper)
aHexahedronActor.AddPosition(2,0,0)
aHexahedronActor.GetProperty().BackfaceCullingOn()


tetraPoints = vtkPoints()
tetraPoints.SetNumberOfPoints(4)
tetraPoints.InsertPoint(0,0,0,0)
tetraPoints.InsertPoint(1,1,0,0)
tetraPoints.InsertPoint(2,.5,1,0)
tetraPoints.InsertPoint(3,.5,.5,1)

aTetra = vtkTetra()
aTetra.GetPointIds().SetId(0,0)
aTetra.GetPointIds().SetId(1,1)
aTetra.GetPointIds().SetId(2,2)
aTetra.GetPointIds().SetId(3,3)

aTetraGrid = vtkUnstructuredGrid()
aTetraGrid.Allocate(1,1)
aTetraGrid.InsertNextCell(aTetra.GetCellType(), aTetra.GetPointIds())
aTetraGrid.SetPoints(tetraPoints)

aTetraMapper = vtkDataSetMapper()
aTetraMapper.SetInput(aTetraGrid)

aTetraActor = vtkActor()
aTetraActor.SetMapper(aTetraMapper)
aTetraActor.AddPosition(4,0,0)
aTetraActor.GetProperty().BackfaceCullingOn()


pixelPoints = vtkPoints()
pixelPoints.SetNumberOfPoints(4)
pixelPoints.InsertPoint(0,0,0,0)
pixelPoints.InsertPoint(1,1,0,0)
pixelPoints.InsertPoint(2,0,1,0)
pixelPoints.InsertPoint(3,1,1,0)

aPixel = vtkPixel()
aPixel.GetPointIds().SetId(0,0)
aPixel.GetPointIds().SetId(1,1)
aPixel.GetPointIds().SetId(2,2)
aPixel.GetPointIds().SetId(3,3)

aPixelGrid = vtkUnstructuredGrid()
aPixelGrid.Allocate(1,1)
aPixelGrid.InsertNextCell(aPixel.GetCellType(), aPixel.GetPointIds())
aPixelGrid.SetPoints(pixelPoints)

aPixelMapper = vtkDataSetMapper()
aPixelMapper.SetInput(aPixelGrid)

aPixelActor = vtkActor()
aPixelActor.SetMapper(aPixelMapper)
aPixelActor.AddPosition(0,0,2)
aPixelActor.GetProperty().BackfaceCullingOn()


quadPoints = vtkPoints()
quadPoints.SetNumberOfPoints(4)
quadPoints.InsertPoint(0,0,0,0)
quadPoints.InsertPoint(1,1,0,0)
quadPoints.InsertPoint(2,1,1,0)
quadPoints.InsertPoint(3,0,1,0)

aQuad = vtkQuad()
aQuad.GetPointIds().SetId(0,0)
aQuad.GetPointIds().SetId(1,1)
aQuad.GetPointIds().SetId(2,2)
aQuad.GetPointIds().SetId(3,3)

aQuadGrid = vtkUnstructuredGrid()
aQuadGrid.Allocate(1,1)
aQuadGrid.InsertNextCell(aQuad.GetCellType(), aQuad.GetPointIds())
aQuadGrid.SetPoints(quadPoints)

aQuadMapper = vtkDataSetMapper()
aQuadMapper.SetInput(aQuadGrid)

aQuadActor = vtkActor()
aQuadActor.SetMapper(aQuadMapper)
aQuadActor.AddPosition(2,0,2)
aQuadActor.GetProperty().BackfaceCullingOn()


trianglePoints = vtkPoints()
trianglePoints.SetNumberOfPoints(3)
trianglePoints.InsertPoint(0,0,0,0)
trianglePoints.InsertPoint(1,1,0,0)
trianglePoints.InsertPoint(2,.5,.5,0)

aTriangle = vtkTriangle()
aTriangle.GetPointIds().SetId(0,0)
aTriangle.GetPointIds().SetId(1,1)
aTriangle.GetPointIds().SetId(2,2)

aTriangleGrid = vtkUnstructuredGrid()
aTriangleGrid.Allocate(1,1)
aTriangleGrid.InsertNextCell(aTriangle.GetCellType(), aTriangle.GetPointIds())
aTriangleGrid.SetPoints(trianglePoints)

aTriangleMapper = vtkDataSetMapper()
aTriangleMapper.SetInput(aTriangleGrid)

aTriangleActor = vtkActor()
aTriangleActor.SetMapper(aTriangleMapper)
aTriangleActor.AddPosition(4,0,2)
aTriangleActor.GetProperty().BackfaceCullingOn()


polygonPoints = vtkPoints()
polygonPoints.SetNumberOfPoints(4)
polygonPoints.InsertPoint(0,0,0,0)
polygonPoints.InsertPoint(1,1,0,0)
polygonPoints.InsertPoint(2,1,1,0)
polygonPoints.InsertPoint(3,0,1,0)

aPolygon = vtkPolygon()
aPolygon.GetPointIds().SetNumberOfIds(4)
aPolygon.GetPointIds().SetId(0,0)
aPolygon.GetPointIds().SetId(1,1)
aPolygon.GetPointIds().SetId(2,2)
aPolygon.GetPointIds().SetId(3,3)

aPolygonGrid = vtkUnstructuredGrid()
aPolygonGrid.Allocate(1,1)
aPolygonGrid.InsertNextCell(aPolygon.GetCellType(), aPolygon.GetPointIds())
aPolygonGrid.SetPoints(polygonPoints)

aPolygonMapper = vtkDataSetMapper()
aPolygonMapper.SetInput(aPolygonGrid)

aPolygonActor = vtkActor()
aPolygonActor.SetMapper(aPolygonMapper)
aPolygonActor.AddPosition(6,0,2)
aPolygonActor.GetProperty().BackfaceCullingOn()


triangleStripPoints = vtkPoints()
triangleStripPoints.SetNumberOfPoints(5)
triangleStripPoints.InsertPoint(0,0,1,0)
triangleStripPoints.InsertPoint(1,0,0,0)
triangleStripPoints.InsertPoint(2,1,1,0)
triangleStripPoints.InsertPoint(3,1,0,0)
triangleStripPoints.InsertPoint(4,2,1,0)

aTriangleStrip = vtkTriangleStrip()
aTriangleStrip.GetPointIds().SetNumberOfIds(5)
aTriangleStrip.GetPointIds().SetId(0,0)
aTriangleStrip.GetPointIds().SetId(1,1)
aTriangleStrip.GetPointIds().SetId(2,2)
aTriangleStrip.GetPointIds().SetId(3,3)
aTriangleStrip.GetPointIds().SetId(4,4)

aTriangleStripGrid = vtkUnstructuredGrid()
aTriangleStripGrid.Allocate(1,1)
aTriangleStripGrid.InsertNextCell(aTriangleStrip.GetCellType(), aTriangleStrip.GetPointIds())
aTriangleStripGrid.SetPoints(triangleStripPoints)

aTriangleStripMapper = vtkDataSetMapper()
aTriangleStripMapper.SetInput(aTriangleStripGrid)

aTriangleStripActor = vtkActor()
aTriangleStripActor.SetMapper(aTriangleStripMapper)
aTriangleStripActor.AddPosition(8,0,2)
aTriangleStripActor.GetProperty().BackfaceCullingOn()


linePoints = vtkPoints()
linePoints.SetNumberOfPoints(2)
linePoints.InsertPoint(0,0,0,0)
linePoints.InsertPoint(1,1,1,0)

aLine = vtkLine()
aLine.GetPointIds().SetId(0,0)
aLine.GetPointIds().SetId(1,1)

aLineGrid = vtkUnstructuredGrid()
aLineGrid.Allocate(1,1)
aLineGrid.InsertNextCell(aLine.GetCellType(), aLine.GetPointIds())
aLineGrid.SetPoints(linePoints)

aLineMapper = vtkDataSetMapper()
aLineMapper.SetInput(aLineGrid)

aLineActor = vtkActor()
aLineActor.SetMapper(aLineMapper)
aLineActor.AddPosition(0,0,4)
aLineActor.GetProperty().BackfaceCullingOn()


polyLinePoints = vtkPoints()
polyLinePoints.SetNumberOfPoints(3)
polyLinePoints.InsertPoint(0,0,0,0)
polyLinePoints.InsertPoint(1,1,1,0)
polyLinePoints.InsertPoint(2,1,0,0)

aPolyLine = vtkPolyLine()
aPolyLine.GetPointIds().SetNumberOfIds(3)
aPolyLine.GetPointIds().SetId(0,0)
aPolyLine.GetPointIds().SetId(1,1)
aPolyLine.GetPointIds().SetId(2,2)

aPolyLineGrid = vtkUnstructuredGrid()
aPolyLineGrid.Allocate(1,1)
aPolyLineGrid.InsertNextCell(aPolyLine.GetCellType(), aPolyLine.GetPointIds())
aPolyLineGrid.SetPoints(polyLinePoints)

aPolyLineMapper = vtkDataSetMapper()
aPolyLineMapper.SetInput(aPolyLineGrid)

aPolyLineActor = vtkActor()
aPolyLineActor.SetMapper(aPolyLineMapper)
aPolyLineActor.AddPosition(2,0,4)
aPolyLineActor.GetProperty().BackfaceCullingOn()


vertexPoints = vtkPoints()
vertexPoints.SetNumberOfPoints(1)
vertexPoints.InsertPoint(0,0,0,0)

aVertex = vtkVertex()
aVertex.GetPointIds().SetId(0,0)

aVertexGrid = vtkUnstructuredGrid()
aVertexGrid.Allocate(1,1)
aVertexGrid.InsertNextCell(aVertex.GetCellType(), aVertex.GetPointIds())
aVertexGrid.SetPoints(vertexPoints)

aVertexMapper = vtkDataSetMapper()
aVertexMapper.SetInput(aVertexGrid)

aVertexActor = vtkActor()
aVertexActor.SetMapper(aVertexMapper)
aVertexActor.AddPosition(0,0,6)
aVertexActor.GetProperty().BackfaceCullingOn()


polyVertexPoints = vtkPoints()
polyVertexPoints.SetNumberOfPoints(3)
polyVertexPoints.InsertPoint(0,0,0,0)
polyVertexPoints.InsertPoint(1,1,0,0)
polyVertexPoints.InsertPoint(2,1,1,0)

aPolyVertex = vtkPolyVertex()
aPolyVertex.GetPointIds().SetNumberOfIds(3)
aPolyVertex.GetPointIds().SetId(0,0)
aPolyVertex.GetPointIds().SetId(1,1)
aPolyVertex.GetPointIds().SetId(2,2)

aPolyVertexGrid = vtkUnstructuredGrid()
aPolyVertexGrid.Allocate(1,1)
aPolyVertexGrid.InsertNextCell(aPolyVertex.GetCellType(), aPolyVertex.GetPointIds())
aPolyVertexGrid.SetPoints(polyVertexPoints)

aPolyVertexMapper = vtkDataSetMapper()
aPolyVertexMapper.SetInput(aPolyVertexGrid)

aPolyVertexActor = vtkActor()
aPolyVertexActor.SetMapper(aPolyVertexMapper)
aPolyVertexActor.AddPosition(2,0,6)
aPolyVertexActor.GetProperty().BackfaceCullingOn()


ren.SetBackground(.1,.2,.4)

ren.AddActor(aVoxelActor)
aVoxelActor.GetProperty().SetDiffuseColor(1,0,0)
ren.AddActor(aHexahedronActor)
aHexahedronActor.GetProperty().SetDiffuseColor(1,1,0)
ren.AddActor(aTetraActor)
aTetraActor.GetProperty().SetDiffuseColor(0,1,0)
ren.AddActor(aPixelActor)
aPixelActor.GetProperty().SetDiffuseColor(0,1,1)
ren.AddActor(aQuadActor)
aQuadActor.GetProperty().SetDiffuseColor(1,0,1)
ren.AddActor(aTriangleActor)
aTriangleActor.GetProperty().SetDiffuseColor(.3,1,.5)
ren.AddActor(aPolygonActor)
aPolygonActor.GetProperty().SetDiffuseColor(1,.4,.5)
ren.AddActor(aTriangleStripActor)
aTriangleStripActor.GetProperty().SetDiffuseColor(.3,.7,1)
ren.AddActor(aLineActor)
aLineActor.GetProperty().SetDiffuseColor(.2,1,1)
ren.AddActor(aPolyLineActor)
aPolyLineActor.GetProperty().SetDiffuseColor(1,1,1)
ren.AddActor(aVertexActor)
aVertexActor.GetProperty().SetDiffuseColor(1,1,1)
ren.AddActor(aPolyVertexActor)
aPolyVertexActor.GetProperty().SetDiffuseColor(1,1,1)

ren.GetActiveCamera().Azimuth(30)
ren.GetActiveCamera().Elevation(20)
ren.GetActiveCamera().Dolly(1.25)
ren.ResetCameraClippingRange()
renWin.Render()

vrml = vtkVRMLExporter()
vrml.SetInput(renWin)
vrml.SetFileName("cells.wrl")
vrml.Write()

if globals().has_key("vtkRIBExporter"):
  print 'yep, globals has vtkRIBExporter'
  rib = vtkRIBExporter()
  rib.SetFilePrefix("cells")
  rib.SetRenderWindow(renWin)
  rib.BackgroundOn()
  rib.Write()
else:
  print 'nope, globals has no vtkRIBExporter'


iv = vtkIVExporter()
iv.SetInput(renWin)
iv.SetFileName("cells.iv")
iv.Write()

obj = vtkOBJExporter()
obj.SetInput(renWin)
obj.SetFilePrefix("cells")
obj.Write()


# render the image
#
iren.Initialize()


iren.Start()
