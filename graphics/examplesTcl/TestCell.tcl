catch {load vtktcl}
# user interface command widget
source ../../examplesTcl/vtkInt.tcl

# create a rendering window and renderer
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create an actor and give it cone geometry
vtkStructuredPoints spts
spts SetDimensions 3 3 1

vtkScalars scalars
scalars InsertNextScalar 1
scalars InsertNextScalar 3
scalars InsertNextScalar 2
scalars InsertNextScalar 4

vtkNormals normals
normals InsertNextNormal 1 0 0
normals InsertNextNormal .7  0 .7
normals InsertNextNormal 0 0 1
normals InsertNextNormal 0 .7 .7

[spts GetCellData] SetScalars scalars
[spts GetCellData] SetNormals normals

vtkDataSetMapper aMapper
  aMapper SetInput spts
#  aMapper SetInput pd
  aMapper SetScalarRange 1 4
vtkActor anActor
  anActor SetMapper aMapper

# assign our actor to the renderer
ren1 AddActor anActor

# enable user interface interactor
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

