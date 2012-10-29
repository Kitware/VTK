#!/usr/bin/env python

# Simple volume rendering example.
reader = vtk.vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(2,2,1)
reader.SetDataScalarTypeToUnsignedShort()
reader.Update()
changeFilter = vtk.vtkImageChangeInformation()
changeFilter.SetInputConnection(reader.GetOutputPort())
changeFilter.SetOutputOrigin(-63,-63,-46)
# Create transfer functions for opacity and color
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(600,0.0)
opacityTransferFunction.AddPoint(2000,1.0)
colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.ClampingOff()
colorTransferFunction.AddHSVPoint(0.0,0.01,1.0,1.0)
colorTransferFunction.AddHSVPoint(1000.0,0.50,1.0,1.0)
colorTransferFunction.AddHSVPoint(2000.0,0.99,1.0,1.0)
colorTransferFunction.SetColorSpaceToHSV()
# Create properties, mappers, volume actors, and ray cast function
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeMapper = vtk.vtkVolumeTextureMapper3D()
volumeMapper.SetInputConnection(changeFilter.GetOutputPort())
volumeMapper.SetSampleDistance(0.25)
volume = vtk.vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)
# Create geometric sphere
sphereSource = vtk.vtkSphereSource()
sphereSource.SetRadius(65)
sphereSource.SetThetaResolution(20)
sphereSource.SetPhiResolution(40)
# Compute random scalars (colors) for each cell
randomColors = vtk.vtkProgrammableAttributeDataFilter()
randomColors.SetInputConnection(sphereSource.GetOutputPort())

def colorCells (__vtk__temp0=0,__vtk__temp1=0):
    randomColorGenerator = vtk.vtkMath()
    input = randomColors.GetInput()
    output = randomColors.GetOutput()
    numCells = input.GetNumberOfCells()
    colors = vtk.vtkFloatArray()
    colors.SetNumberOfTuples(numCells)
    i = 0
    while i < numCells:
        colors.SetValue(i,randomColorGenerator.Random(0,1))
        i = i + 1

    output.GetCellData().CopyScalarsOff()
    output.GetCellData().PassData(input.GetCellData())
    output.GetCellData().SetScalars(colors)
    del colors
    #reference counting - it's ok
    del randomColorGenerator

randomColors.SetExecuteMethod(colorCells)
# This does not need a hierarchical mapper, but hierarchical
# mapper could use a test that has clipping so we use it here
sphereMapper = vtk.vtkHierarchicalPolyDataMapper()
sphereMapper.SetInputConnection(randomColors.GetOutputPort(0))
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
# Set up the planes
plane1 = vtk.vtkPlane()
plane1.SetOrigin(0,0,-10)
plane1.SetNormal(0,0,1)
plane2 = vtk.vtkPlane()
plane2.SetOrigin(0,0,10)
plane2.SetNormal(0,0,-1)
plane3 = vtk.vtkPlane()
plane3.SetOrigin(-10,0,0)
plane3.SetNormal(1,0,0)
plane4 = vtk.vtkPlane()
plane4.SetOrigin(10,0,0)
plane4.SetNormal(-1,0,0)
sphereMapper.AddClippingPlane(plane1)
sphereMapper.AddClippingPlane(plane2)
volumeMapper.AddClippingPlane(plane3)
volumeMapper.AddClippingPlane(plane4)
# Okay now the graphics stuff
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.ReportGraphicErrorsOn()
renWin.AddRenderer(ren1)
renWin.SetSize(256,256)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.GetCullers().InitTraversal()
culler = ren1.GetCullers().GetNextItem()
culler.SetSortingStyleToBackToFront()
ren1.AddViewProp(sphereActor)
ren1.SetBackground(0.1,0.2,0.4)
renWin.Render()
ren1.GetActiveCamera().Azimuth(45)
ren1.GetActiveCamera().Elevation(15)
ren1.GetActiveCamera().Roll(45)
ren1.GetActiveCamera().Zoom(2.0)
valid = volumeMapper.IsRenderSupported(volumeProperty,ren1)
ren1.AddViewProp(volume)
if ():
    ren1.RemoveAllViewProps()
    t = vtk.vtkTextActor()
    t.SetInput("Required Extensions Not Supported")
    t.SetDisplayPosition(128,128)
    t.GetTextProperty().SetJustificationToCentered()
    ren1.AddViewProp(t)
    pass
iren.Initialize()
i = 0
while i < 5:
    volume.RotateY(17)
    volume.RotateZ(13)
    sphereActor.RotateX(13)
    sphereActor.RotateY(17)
    renWin.Render()
    i = i + 1

# --- end of script --
