package require vtk
package require vtkinteraction

set file0 idFile0.vti
set file1 idFile1.vti
set file2 idFile2.vti

# read in some image data
vtkImageReader imageReader
  imageReader SetDataByteOrderToLittleEndian
  imageReader SetDataExtent 0 63 0 63 1 93
  imageReader SetDataSpacing 3.2 3.2 1.5
  imageReader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  imageReader Update

# extract to reduce extents of grid
vtkExtractVOI extract
  extract SetInputConnection [imageReader GetOutputPort]
  extract SetVOI 0 63 0 63 0 45
  extract Update

# write just a piece (extracted piece) as well as the whole thing  
vtkXMLImageDataWriter idWriter
  idWriter SetFileName $file0
  idWriter SetDataModeToAscii
  idWriter SetInputConnection [extract GetOutputPort]
  idWriter Write
  
  idWriter SetFileName $file1
  idWriter SetDataModeToAppended
  idWriter SetInputConnection [imageReader GetOutputPort]
  idWriter SetNumberOfPieces 2
  idWriter Write

  idWriter SetFileName $file2
  idWriter SetDataModeToBinary
  idWriter SetWriteExtent 1 31 4 63 12 92
  idWriter Write

# read the extracted grid
vtkXMLImageDataReader reader
  reader SetFileName $file0
  reader WholeSlicesOff
  reader Update

vtkImageData id0
  id0 DeepCopy [reader GetOutput]

vtkContourFilter cF0
  cF0 SetInput id0
  cF0 SetValue 0 500
  
vtkPolyDataMapper mapper0
  mapper0 SetInputConnection [cF0 GetOutputPort]
  mapper0 ScalarVisibilityOff

vtkActor actor0
  actor0 SetMapper mapper0
  actor0 SetPosition 180 -60 0


# read the whole image
reader SetFileName $file1
reader WholeSlicesOn
reader Update

vtkImageData id1
  id1 DeepCopy [reader GetOutput]

vtkContourFilter cF1
  cF1 SetInput id1
  cF1 SetValue 0 500

vtkPolyDataMapper mapper1
  mapper1 SetInputConnection [cF1 GetOutputPort]
  mapper1 ScalarVisibilityOff

vtkActor actor1
  actor1 SetMapper mapper1
  actor1 SetOrientation 90 0 0


# read the paritally written image
reader SetFileName $file2
reader Update

vtkContourFilter cF2
  cF2 SetInputConnection [reader GetOutputPort]
  cF2 SetValue 0 500

vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [cF2 GetOutputPort]
  mapper2 ScalarVisibilityOff

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 SetOrientation 0 -90 0
  actor2 SetPosition 180 -30 0

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor0
ren1 AddActor actor1
ren1 AddActor actor2

renWin SetSize 300 300
iren Initialize
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


file delete -force $file0
file delete -force $file1
file delete -force $file2
