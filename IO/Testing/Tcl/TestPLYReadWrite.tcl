package require vtk
package require vtkinteraction

vtkSphereSource ss
   ss SetPhiResolution 10
   ss SetThetaResolution 20
vtkSimpleElevationFilter ele
   ele SetInput [ss GetOutput]
vtkPointDataToCellData pd2cd
   pd2cd SetInput [ele GetPolyDataOutput]

# First way or writing
vtkPLYWriter w
   w SetInput [pd2cd GetPolyDataOutput]
   w SetFileName plyWriter.ply
   w SetFileTypeToBinary
   w SetDataByteOrderToLittleEndian
   w SetColorModeToUniformCellColor
   w SetColor 255 0 0 
   w Write
vtkPLYReader r
   r SetFileName plyWriter.ply
   r Update
file delete -force plyWriter.ply
vtkPolyDataMapper plyMapper
   plyMapper SetInput [r GetOutput]
vtkActor plyActor
   plyActor SetMapper plyMapper

# Second way or writing - it will map through a lookup table
vtkLookupTable lut
   lut Build
vtkPLYWriter w2
   w2 SetInput [pd2cd GetPolyDataOutput]
   w2 SetFileName plyWriter.ply
   w2 SetFileTypeToBinary
   w2 SetDataByteOrderToLittleEndian
   w2 SetColorModeToDefault
   w2 SetLookupTable lut
   w2 SetArrayName Elevation
   w2 SetComponent 0
   w2 Write
vtkPLYReader r2
   r2 SetFileName plyWriter.ply
   r2 Update
vtkPolyDataMapper plyMapper2
   plyMapper2 SetInput [r2 GetOutput]
vtkActor plyActor2
   plyActor2 SetMapper plyMapper2
   plyActor2 AddPosition 1 0 0

# Third way or writing - it will read the previous file with rgb cell color
vtkPLYReader r3
   r3 SetFileName plyWriter.ply
   r3 Update
vtkPLYWriter w3
   w3 SetInput [r3 GetOutput]
   w3 SetFileName plyWriter.ply
   w3 SetFileTypeToBinary
   w3 SetDataByteOrderToLittleEndian
   w3 SetColorModeToDefault
   w3 SetArrayName RGB
   w3 SetComponent 0
   w3 Write
vtkPLYReader r4
   r4 SetFileName plyWriter.ply
   r4 Update
vtkPolyDataMapper plyMapper3
   plyMapper3 SetInput [r4 GetOutput]
vtkActor plyActor3
   plyActor3 SetMapper plyMapper3
   plyActor3 AddPosition 2 0 0

file delete -force plyWriter.ply

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor plyActor
ren1 AddActor plyActor2
ren1 AddActor plyActor3

renWin SetSize 325 125 
iren Initialize
renWin Render
[ren1 GetActiveCamera] Zoom 3.0

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
