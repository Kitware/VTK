package require vtk
package require vtkinteraction

# Demonstrates all cell types
#
# NOTE: the use of NewInstance/DeepCopy is included to increase
# regression coverage.  It is not required in most applications.

vtkRenderer ren1
# turn off all cullers
[ren1 GetCullers] RemoveAllItems 

vtkRenderWindow renWin
  renWin AddRenderer ren1
  renWin SetSize 300 150
vtkRenderWindowInteractor iren
  iren SetRenderWindow renWin

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

vtkVoxel aVoxel
  [aVoxel GetPointIds] SetId 0 0
  [aVoxel GetPointIds] SetId 1 1
  [aVoxel GetPointIds] SetId 2 2
  [aVoxel GetPointIds] SetId 3 3
  [aVoxel GetPointIds] SetId 4 4
  [aVoxel GetPointIds] SetId 5 5
  [aVoxel GetPointIds] SetId 6 6
  [aVoxel GetPointIds] SetId 7 7

set bVoxel [aVoxel NewInstance]
$bVoxel DeepCopy aVoxel

vtkUnstructuredGrid aVoxelGrid
  aVoxelGrid Allocate 1 1
  aVoxelGrid InsertNextCell [aVoxel GetCellType] [aVoxel GetPointIds]
  aVoxelGrid SetPoints voxelPoints

vtkDataSetMapper aVoxelMapper
aVoxelMapper SetInput aVoxelGrid

vtkActor aVoxelActor
  aVoxelActor SetMapper aVoxelMapper
  [aVoxelActor GetProperty] BackfaceCullingOn

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
  
set bHexahedron [aHexahedron NewInstance]
$bHexahedron DeepCopy aHexahedron
  
vtkUnstructuredGrid aHexahedronGrid
  aHexahedronGrid Allocate 1 1
  aHexahedronGrid InsertNextCell [aHexahedron GetCellType] [aHexahedron GetPointIds]
  aHexahedronGrid SetPoints hexahedronPoints

vtkDataSetMapper aHexahedronMapper
  aHexahedronMapper SetInput aHexahedronGrid

vtkActor aHexahedronActor
  aHexahedronActor SetMapper aHexahedronMapper
  aHexahedronActor AddPosition 2 0 0
  [aHexahedronActor GetProperty] BackfaceCullingOn


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

set bTetra [aTetra NewInstance]
$bTetra DeepCopy aTetra

vtkUnstructuredGrid aTetraGrid
  aTetraGrid Allocate 1 1
  aTetraGrid InsertNextCell [aTetra GetCellType] [aTetra GetPointIds]
  aTetraGrid SetPoints tetraPoints

vtkUnstructuredGrid aTetraCopy
  aTetraCopy ShallowCopy aTetraGrid

vtkDataSetMapper aTetraMapper
  aTetraMapper SetInput aTetraCopy

vtkActor aTetraActor
  aTetraActor SetMapper aTetraMapper
  aTetraActor AddPosition 4 0 0
  [aTetraActor GetProperty] BackfaceCullingOn


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

set bWedge [aWedge NewInstance]
$bWedge DeepCopy aWedge

vtkUnstructuredGrid aWedgeGrid
  aWedgeGrid Allocate 1 1
  aWedgeGrid InsertNextCell [aWedge GetCellType] [aWedge GetPointIds]
  aWedgeGrid SetPoints wedgePoints

vtkUnstructuredGrid aWedgeCopy
  aWedgeCopy DeepCopy aWedgeGrid

vtkDataSetMapper aWedgeMapper
  aWedgeMapper SetInput aWedgeCopy

vtkActor aWedgeActor
  aWedgeActor SetMapper aWedgeMapper
  aWedgeActor AddPosition 6 0 0
  [aWedgeActor GetProperty] BackfaceCullingOn


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

set bPyramid [aPyramid NewInstance]
$bPyramid DeepCopy aPyramid

vtkUnstructuredGrid aPyramidGrid
  aPyramidGrid Allocate 1 1
  aPyramidGrid InsertNextCell [aPyramid GetCellType] [aPyramid GetPointIds]
  aPyramidGrid SetPoints pyramidPoints

vtkDataSetMapper aPyramidMapper
  aPyramidMapper SetInput aPyramidGrid

vtkActor aPyramidActor
  aPyramidActor SetMapper aPyramidMapper
  aPyramidActor AddPosition 8 0 0
  [aPyramidActor GetProperty] BackfaceCullingOn


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

set bPixel [aPixel NewInstance]
$bPixel DeepCopy aPixel

vtkUnstructuredGrid aPixelGrid
  aPixelGrid Allocate 1 1
  aPixelGrid InsertNextCell [aPixel GetCellType] [aPixel GetPointIds]
  aPixelGrid SetPoints pixelPoints

vtkDataSetMapper aPixelMapper
  aPixelMapper SetInput aPixelGrid

vtkActor aPixelActor
  aPixelActor SetMapper aPixelMapper
  aPixelActor AddPosition 0 0 2
  [aPixelActor GetProperty] BackfaceCullingOn


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

set bQuad [aQuad NewInstance]
$bQuad DeepCopy aQuad

vtkUnstructuredGrid aQuadGrid
  aQuadGrid Allocate 1 1
  aQuadGrid InsertNextCell [aQuad GetCellType] [aQuad GetPointIds]
  aQuadGrid SetPoints quadPoints

vtkDataSetMapper aQuadMapper
  aQuadMapper SetInput aQuadGrid

vtkActor aQuadActor
  aQuadActor SetMapper aQuadMapper
  aQuadActor AddPosition 2 0 2
  [aQuadActor GetProperty] BackfaceCullingOn


vtkPoints trianglePoints
  trianglePoints SetNumberOfPoints 3
  trianglePoints InsertPoint 0 0 0 0
  trianglePoints InsertPoint 1 1 0 0
  trianglePoints InsertPoint 2 .5 .5 0

vtkFloatArray triangleTCoords
  triangleTCoords SetNumberOfComponents 2
  triangleTCoords SetNumberOfTuples 3
  triangleTCoords InsertTuple2 0 1 1 
  triangleTCoords InsertTuple2 1 2 2 
  triangleTCoords InsertTuple2 2 3 3 

vtkTriangle aTriangle
  [aTriangle GetPointIds] SetId 0 0
  [aTriangle GetPointIds] SetId 1 1
  [aTriangle GetPointIds] SetId 2 2

set bTriangle [aTriangle NewInstance]
$bTriangle DeepCopy aTriangle

vtkUnstructuredGrid aTriangleGrid
  aTriangleGrid Allocate 1 1
  aTriangleGrid InsertNextCell [aTriangle GetCellType] [aTriangle GetPointIds]
  aTriangleGrid SetPoints trianglePoints
  [aTriangleGrid GetPointData] SetTCoords triangleTCoords

vtkDataSetMapper aTriangleMapper
  aTriangleMapper SetInput aTriangleGrid

vtkActor aTriangleActor
  aTriangleActor SetMapper aTriangleMapper
  aTriangleActor AddPosition 4 0 2
  [aTriangleActor GetProperty] BackfaceCullingOn


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

set bPolygon [aPolygon NewInstance]
$bPolygon DeepCopy aPolygon

vtkUnstructuredGrid aPolygonGrid
  aPolygonGrid Allocate 1 1
  aPolygonGrid InsertNextCell [aPolygon GetCellType] [aPolygon GetPointIds]
  aPolygonGrid SetPoints polygonPoints

vtkDataSetMapper aPolygonMapper
  aPolygonMapper SetInput aPolygonGrid

vtkActor aPolygonActor
  aPolygonActor SetMapper aPolygonMapper
  aPolygonActor AddPosition 6 0 2
  [aPolygonActor GetProperty] BackfaceCullingOn


vtkPoints triangleStripPoints
  triangleStripPoints SetNumberOfPoints 5
  triangleStripPoints InsertPoint 0 0 1 0
  triangleStripPoints InsertPoint 1 0 0 0
  triangleStripPoints InsertPoint 2 1 1 0
  triangleStripPoints InsertPoint 3 1 0 0
  triangleStripPoints InsertPoint 4 2 1 0

vtkFloatArray triangleStripTCoords
  triangleStripTCoords SetNumberOfComponents 2
  triangleStripTCoords SetNumberOfTuples 3
  triangleStripTCoords InsertTuple2 0 1 1 
  triangleStripTCoords InsertTuple2 1 2 2 
  triangleStripTCoords InsertTuple2 2 3 3 
  triangleStripTCoords InsertTuple2 3 4 4 
  triangleStripTCoords InsertTuple2 4 5 5 

vtkTriangleStrip aTriangleStrip
  [aTriangleStrip GetPointIds] SetNumberOfIds 5
  [aTriangleStrip GetPointIds] SetId 0 0
  [aTriangleStrip GetPointIds] SetId 1 1
  [aTriangleStrip GetPointIds] SetId 2 2
  [aTriangleStrip GetPointIds] SetId 3 3
  [aTriangleStrip GetPointIds] SetId 4 4

set bTriangleStrip [aTriangleStrip NewInstance]
$bTriangleStrip DeepCopy aTriangleStrip

vtkUnstructuredGrid aTriangleStripGrid
  aTriangleStripGrid Allocate 1 1
  aTriangleStripGrid InsertNextCell [aTriangleStrip GetCellType] [aTriangleStrip GetPointIds]
  aTriangleStripGrid SetPoints triangleStripPoints
  [aTriangleStripGrid GetPointData] SetTCoords triangleStripTCoords

vtkDataSetMapper aTriangleStripMapper
  aTriangleStripMapper SetInput aTriangleStripGrid

vtkActor aTriangleStripActor
  aTriangleStripActor SetMapper aTriangleStripMapper
  aTriangleStripActor AddPosition 8 0 2
  [aTriangleStripActor GetProperty] BackfaceCullingOn


vtkPoints linePoints
  linePoints SetNumberOfPoints 2
  linePoints InsertPoint 0 0 0 0
  linePoints InsertPoint 1 1 1 0

vtkLine aLine
  [aLine GetPointIds] SetId 0 0
  [aLine GetPointIds] SetId 1 1

set bLine [aLine NewInstance]
$bLine DeepCopy aLine

vtkUnstructuredGrid aLineGrid
  aLineGrid Allocate 1 1
  aLineGrid InsertNextCell [aLine GetCellType] [aLine GetPointIds]
  aLineGrid SetPoints linePoints

vtkDataSetMapper aLineMapper
  aLineMapper SetInput aLineGrid

vtkActor aLineActor
  aLineActor SetMapper aLineMapper
  aLineActor AddPosition 0 0 4
  [aLineActor GetProperty] BackfaceCullingOn


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

set bPolyLine [aPolyLine NewInstance]
$bPolyLine DeepCopy aPolyLine

vtkUnstructuredGrid aPolyLineGrid
  aPolyLineGrid Allocate 1 1
  aPolyLineGrid InsertNextCell [aPolyLine GetCellType] [aPolyLine GetPointIds]
  aPolyLineGrid SetPoints polyLinePoints

vtkDataSetMapper aPolyLineMapper
  aPolyLineMapper SetInput aPolyLineGrid

vtkActor aPolyLineActor
  aPolyLineActor SetMapper aPolyLineMapper
  aPolyLineActor AddPosition 2 0 4
  [aPolyLineActor GetProperty] BackfaceCullingOn


vtkPoints vertexPoints
  vertexPoints SetNumberOfPoints 1
  vertexPoints InsertPoint 0 0 0 0

vtkVertex aVertex
  [aVertex GetPointIds] SetId 0 0

set bVertex [aVertex NewInstance]
$bVertex DeepCopy aVertex

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

set bPolyVertex [aPolyVertex NewInstance]
$bPolyVertex DeepCopy aPolyVertex

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


if { [info command vtkRIBProperty] != "" } {
vtkRIBProperty aProperty
  aProperty SetVariable Km float
  aProperty SetSurfaceShader LGVeinedmarble
  aProperty SetVariable veinfreq float
  aProperty AddVariable warpfreq float
  aProperty AddVariable veincolor color
  aProperty AddParameter veinfreq 2
  aProperty AddParameter veincolor "1.0000 1.0000 0.9412"
vtkRIBProperty bProperty
  bProperty SetVariable Km float
  bProperty SetParameter Km 1.0
  bProperty SetDisplacementShader dented
  bProperty SetSurfaceShader plastic
} else {
  vtkProperty aProperty
  vtkProperty bProperty
}
aTriangleActor SetProperty aProperty
aTriangleStripActor SetProperty bProperty

ren1 SetBackground .1 .2 .4

ren1 AddActor aVoxelActor; [aVoxelActor GetProperty] SetDiffuseColor 1 0 0
ren1 AddActor aHexahedronActor; [aHexahedronActor GetProperty] SetDiffuseColor 1 1 0
ren1 AddActor aTetraActor; [aTetraActor GetProperty] SetDiffuseColor 0 1 0
ren1 AddActor aWedgeActor; [aWedgeActor GetProperty] SetDiffuseColor 0 1 1
ren1 AddActor aPyramidActor; [aPyramidActor GetProperty] SetDiffuseColor 1 0 1
ren1 AddActor aPixelActor; [aPixelActor GetProperty] SetDiffuseColor 0 1 1
ren1 AddActor aQuadActor; [aQuadActor GetProperty] SetDiffuseColor 1 0 1
ren1 AddActor aTriangleActor; [aTriangleActor GetProperty] SetDiffuseColor .3 1 .5
ren1 AddActor aPolygonActor; [aPolygonActor GetProperty] SetDiffuseColor 1 .4 .5
ren1 AddActor aTriangleStripActor; [aTriangleStripActor GetProperty] SetDiffuseColor .3 .7 1
ren1 AddActor aLineActor; [aLineActor GetProperty] SetDiffuseColor .2 1 1
ren1 AddActor aPolyLineActor; [aPolyLineActor GetProperty] SetDiffuseColor 1 1 1
ren1 AddActor aVertexActor; [aVertexActor GetProperty] SetDiffuseColor 1 1 1
ren1 AddActor aPolyVertexActor; [aPolyVertexActor GetProperty] SetDiffuseColor 1 1 1

if { [info command vtkRIBLight] != "" } {
    vtkRIBLight aLight
    aLight ShadowsOn
} else {
    vtkLight aLight
}
aLight PositionalOn
aLight SetConeAngle 15

ren1 AddLight aLight

[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
[ren1 GetActiveCamera] Dolly 2.8
ren1 ResetCameraClippingRange

eval aLight SetFocalPoint [[ren1 GetActiveCamera] GetFocalPoint]
eval aLight SetPosition [[ren1 GetActiveCamera] GetPosition]
renWin Render

if { [info command vtkRIBExporter] != "" } {
  vtkTexture atext
  vtkBMPReader pnmReader
    pnmReader SetFileName "$VTK_DATA_ROOT/Data/masonry.bmp"
  atext SetInput [pnmReader GetOutput]
  atext InterpolateOff
  aTriangleActor SetTexture atext
  vtkRIBExporter rib
    rib SetInput renWin
    rib SetFilePrefix cells
    rib SetTexturePrefix cells
}

vtkIVExporter iv
  iv SetInput renWin
  iv SetFileName cells.iv

vtkOBJExporter obj
  obj SetInput renWin
  obj SetFilePrefix cells

vtkVRMLExporter vrml
  vrml SetInput renWin
  vrml SetStartWrite {vrml SetFileName cells.wrl}
  vrml SetEndWrite {vrml SetFileName /a/acells.wrl}
  vrml SetSpeed 5.5

vtkOOGLExporter oogl
  oogl SetInput renWin
  oogl SetFileName cells.oogl

#
# If the current directory is writable, then test the witers
#

if {[catch {set channel [open test.tmp w]}] == 0 } {
   close $channel
   file delete -force test.tmp

   iv Write
   file delete -force cells.iv
   obj Write
   file delete -force cells.obj
   file delete -force cells.mtl
   vrml Write
   file delete -force cells.wrl
   #oogl Write
   #file delete -force cells.oogl
   
   if { [info command vtkRIBExporter] != "" } {
      rib Write
      file delete -force cells.rib
      eval file delete -force [glob -nocomplain cells_*_*.tif]
   }
}

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize
wm withdraw .


# the UnRegister calls are because make object is the same as New,
# and causes memory leaks. (Tcl does not treat NewInstance the same as New).
proc DeleteCopies {} {
  global bVoxel bHexahedron bTetra bPixel bQuad bTriangle bPolygon
  global bTriangleStrip bLine bPolyLine bVertex bPolyVertex
  global bWedge bPyramid
   $bVoxel UnRegister {}
   $bHexahedron UnRegister {}
   $bTetra UnRegister {}
   $bWedge UnRegister {}
   $bPyramid UnRegister {}
   $bPixel UnRegister {}
   $bQuad UnRegister {}
   $bTriangle UnRegister {}
   $bPolygon UnRegister {}
   $bTriangleStrip UnRegister {}
   $bLine UnRegister {}
   $bPolyLine UnRegister {}
   $bVertex UnRegister {}
   $bPolyVertex UnRegister {}
}

DeleteCopies

# for testing
set threshold 20

