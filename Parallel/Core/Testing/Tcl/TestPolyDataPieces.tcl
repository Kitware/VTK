package require vtk
package require vtkinteraction

vtkMath math
math RandomSeed 22

vtkSphereSource sphere
sphere SetPhiResolution 32
sphere SetThetaResolution 32

vtkExtractPolyDataPiece extract
extract SetInputConnection [sphere GetOutputPort]

vtkPolyDataNormals normals
normals SetInputConnection [extract GetOutputPort]

vtkPieceScalars ps
ps SetInputConnection [normals GetOutputPort]

vtkPolyDataMapper mapper
mapper SetInputConnection [ps GetOutputPort]
mapper SetNumberOfPieces 2

vtkActor actor
actor SetMapper mapper

vtkSphereSource sphere2
sphere2 SetPhiResolution 32
sphere2 SetThetaResolution 32

vtkExtractPolyDataPiece extract2
extract2 SetInputConnection [sphere2 GetOutputPort]

vtkPolyDataMapper mapper2
mapper2 SetInputConnection [extract2 GetOutputPort]
mapper2 SetNumberOfPieces 2
mapper2 SetPiece 1
mapper2 SetScalarRange 0 4
mapper2 SetScalarModeToUseCellFieldData
mapper2 SetColorModeToMapScalars
mapper2 ColorByArrayComponent "vtkGhostLevels" 0
mapper2 SetGhostLevel 4

# check the pipeline size
extract2 UpdateInformation
vtkPipelineSize psize
if {[psize GetEstimatedSize extract2 0 0] > 100} {
   puts stderr "ERROR: Pipeline Size increased"
}
if {[psize GetNumberOfSubPieces 10 mapper2] != 2} {
   puts stderr "ERROR: Number of sub pieces changed"
}

vtkActor actor2
actor2 SetMapper mapper2
actor2 SetPosition 1.5 0 0

vtkSphereSource sphere3
sphere3 SetPhiResolution 32
sphere3 SetThetaResolution 32

vtkExtractPolyDataPiece extract3
extract3 SetInputConnection [sphere3 GetOutputPort]

vtkPieceScalars ps3
ps3 SetInputConnection [extract3 GetOutputPort]

vtkPolyDataMapper mapper3
mapper3 SetInputConnection [ps3 GetOutputPort]
mapper3 SetNumberOfSubPieces 8
mapper3 SetScalarRange 0 8

vtkActor actor3
actor3 SetMapper mapper3
actor3 SetPosition 0 -1.5 0

vtkSphereSource sphere4
sphere4 SetPhiResolution 32
sphere4 SetThetaResolution 32

vtkExtractPolyDataPiece extract4
extract4 SetInputConnection [sphere4 GetOutputPort]

vtkPieceScalars ps4
ps4 RandomModeOn
ps4 SetScalarModeToCellData
ps4 SetInputConnection [extract4 GetOutputPort]

vtkPolyDataMapper mapper4
mapper4 SetInputConnection [ps4 GetOutputPort]
mapper4 SetNumberOfSubPieces 8
mapper4 SetScalarRange 0 8

vtkActor actor4
actor4 SetMapper mapper4
actor4 SetPosition 1.5 -1.5 0

vtkRenderer ren
ren AddActor actor
ren AddActor actor2
ren AddActor actor3
ren AddActor actor4

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

wm withdraw .

