# @par Thanks:
# This class was written by Jacques-Bernard Lekien, 2018-19
# This work was supported by Commissariat a l'Energie Atomique
# CEA, DAM, DIF, F-91297 Arpajon, France.


import vtk

import math

TARGET_LEVEL = 4

class Circle:
  _center = [0.5, 0.]
  _rayon = 1.
  _rayon2 = _rayon*_rayon

  def test(self, x, y, z):
    d2 = (x - self._center[0])**2 + (y - self._center[1])**2
    if d2 > self._rayon2:
      return -1
    return 1

  def value(self, geoCursor):
    bounds = range(6)
    geoCursor.GetBounds( bounds )
    return self.test(
      bounds[0]+(bounds[1]-bounds[0])/2,
      bounds[2]+(bounds[3]-bounds[2])/2,
      bounds[4]+(bounds[5]-bounds[4])/2)

  def shouldRefine(self, geoCursor):
    if geoCursor.GetLevel() >= TARGET_LEVEL:
        return False
    bounds = range(6)
    geoCursor.GetBounds( bounds )
    v0 = self.test(bounds[0],bounds[2],bounds[4])
    v1 = self.test(bounds[1],bounds[2],bounds[4])
    if v0 == v1:
      v2 = self.test(bounds[0],bounds[3],bounds[4])
      if v0 == v2:
        v3 = self.test(bounds[1],bounds[3],bounds[4])
        if v0 == v3:
          return False
    return True

  def handleNode(self, geoCursor, levelArray, scalarArray):
    # Add value in fields
    idx = geoCursor.GetGlobalNodeIndex()
    scalarArray.InsertTuple1(idx, self.value(geoCursor))
    levelArray.InsertTuple1(idx, geoCursor.GetLevel())

    if geoCursor.IsLeaf():
      if self.shouldRefine(geoCursor):
        geoCursor.SubdivideLeaf()
        self.handleNode(geoCursor, levelArray, scalarArray)
    else:
      for ichild in range( geoCursor.GetNumberOfChildren() ):
        geoCursor.ToChild(ichild)
        self.handleNode(geoCursor, levelArray, scalarArray)
        geoCursor.ToParent()

#--------------------------------

#Generate a HyperTree Grid
htg = vtk.vtkHyperTreeGrid()
htg.Initialize()
htg.SetDimensions([4, 3, 1]) #GridPoints
htg.SetBranchFactor(2)

#Scalar Level
levelArray = vtk.vtkUnsignedCharArray()
levelArray.SetName('level')
levelArray.SetNumberOfValues(0)
htg.GetPointData().AddArray(levelArray)

scalarArray = vtk.vtkDoubleArray()
scalarArray.SetName('scalar')
scalarArray.SetNumberOfValues(0)
htg.GetPointData().AddArray(scalarArray)

xValues = vtk.vtkDoubleArray() #x
xValues.SetNumberOfValues(4)
xValues.SetValue(0, -1)
xValues.SetValue(1, 0)
xValues.SetValue(2, 1)
xValues.SetValue(3, 2)
htg.SetXCoordinates(xValues)

yValues = vtk.vtkDoubleArray() #y
yValues.SetNumberOfValues(3)
yValues.SetValue(0, -1)
yValues.SetValue(1, 0)
yValues.SetValue(2, 1)
htg.SetYCoordinates(yValues)

zValues = vtk.vtkDoubleArray() #z
zValues.SetNumberOfValues(1)
zValues.SetValue(0, 0)
htg.SetZCoordinates(zValues)

#Create a cursor
geoCursor = vtk.vtkHyperTreeGridNonOrientedGeometryCursor()
#Implicit index global
crtIndex = 0

circle = Circle()

for treeId in range(htg.GetMaxNumberOfTrees()):
  htg.InitializeNonOrientedGeometryCursor(geoCursor, treeId, True)
  geoCursor.SetGlobalIndexStart(crtIndex)
  #
  circle.handleNode(geoCursor, levelArray, scalarArray)
  #
  crtIndex += geoCursor.GetTree().GetNumberOfVertices()

#Tables cannot be assigned before.
#Maybe because if we do it at the beginning their zero size cancels
#this step ?
htg.GetPointData().AddArray(levelArray)
htg.GetPointData().AddArray(scalarArray)

#print("# ", crtIndex)
scalar = htg.GetPointData().GetArray('scalar')
assert(scalar)

#Depth Limiter Filter
#The addition of this intermediate filter is intended to do
#nothing but to allow the installation of a purely Python filter
#(inheriting the vta.VTKAlgorithm). Indeed, the latter does not
#propose by the method SetInputData !
#print('With Depth Limiter Filter (HTG)')
depth = vtk.vtkHyperTreeGridDepthLimiter()
depth.SetInputData(htg)
depth.SetDepth(1024) #No depth limiter

#My Filter
from vtk.util import vtkAlgorithm as vta

class MyAlgorithm(vta.VTKAlgorithm):
  _selectValue = None

  def SetSelectValue(self, value):
    self._selectValue = value

  def FillInputPortInformation(self, vtkself, port, info):
    info.Set(vtk.vtkAlgorithm.INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid")
    return 1

  def FillOutputPortInformation(self, vtkself, port, info):
    info.Set(vtk.vtkDataObject.DATA_TYPE_NAME(), "vtkHyperTreeGrid")
    return 1

  def RecursiveProcess(self, cursor, scalar, outMask):
    if cursor.IsLeaf():
      ind = cursor.GetGlobalNodeIndex()
      if scalar.GetValue(ind) == self._selectValue:
        discard = False
      else:
        discard = True
      outMask.SetValue(ind, discard)
    else:
      discard = True
      for ichild in range(cursor.GetNumberOfChildren()):
        cursor.ToChild( ichild )
        if not self.RecursiveProcess( cursor, scalar, outMask ):
          discard = False
        cursor.ToParent()
      outMask.SetValue(cursor.GetGlobalNodeIndex(), discard)

  def RequestData(self, vtkself, request, inInfo, outInfo):
    inp = self.GetInputData(inInfo, 0, 0)
    out = self.GetOutputData(outInfo, 0)
    out.ShallowCopy(inp)

    scalar = inp.GetPointData().GetArray('scalar')
    assert(scalar)

    outMask = vtk.vtkBitArray()
    outMask.SetNumberOfTuples(out.GetNumberOfVertices())

    cursor = vtk.vtkHyperTreeGridNonOrientedCursor()
    for treeId in range(inp.GetMaxNumberOfTrees()):
      inp.InitializeNonOrientedCursor( cursor, treeId )
      self.RecursiveProcess( cursor, scalar, outMask )

    out.SetMask( outMask )
    return 1

myAlgo = MyAlgorithm()
myAlgo.SetSelectValue(1) # 1 or -1

ex = vtk.vtkPythonAlgorithm()
ex.SetPythonObject(myAlgo)
ex.SetInputConnection(depth.GetOutputPort())

ex.Update()

from vtk.util.misc import vtkGetTempDir
nrep = vtkGetTempDir()

withAscii = False
if withAscii:
  filename = nrep+'toto_1_ascii.htg'
else:
  filename = nrep+'toto_1_binary.htg'
#Avant writer default (1.0)
writer = vtk.vtkXMLHyperTreeGridWriter()
writer.SetInputConnection(ex.GetOutputPort())
writer.SetFileName(filename)
#The default is in appended, data at the end of the file
if withAscii:
  writer.SetDataModeToAscii()
writer.Write()
#print('Write Defaut(1.0) '+filename)
#
#print("Avant writer 0.?")
writer.SetDataSetMajorVersion(0)
writer.SetFileName(nrep+'toto_0.htg')
writer.Write()
#print('Write Forced(0.1)')
#
#print('Read')
ex = vtk.vtkXMLHyperTreeGridReader()
ex.SetFileName(nrep+'toto_0.htg')
#
#print('Read '+filename)
ex = vtk.vtkXMLHyperTreeGridReader()
ex.SetFileName(filename)
#example for load reduction
##selected level
#ex.SetFixedLevel(5)
##selected HTs in the BB
#ex.SetCoordinatesBoundingBox(-5.,0.5,-5.,5.,-5.,0.)
##selected ont by ont HT with or not FixedLevel
#ex.ClearAndAddSelectedHT(2,4)
#ex.AddSelectedHT(1, 5)
#ex.ClearAndAddSelectedHT(2,2)
##or
#ex.AddSelectedHT(1, 5)
#ex.AddSelectedHT(3)
#
ex.Update()
#print("Avant writer default (1.0)")
writer = vtk.vtkXMLHyperTreeGridWriter()
writer.SetInputConnection(ex.GetOutputPort())
if withAscii:
  filename = nrep+'toto_1_ascii_ascii.htg'
else:
  filename = nrep+'toto_1_binary_ascii.htg'
writer.SetFileName(filename)
# default c'est en appended, data en fin du fichier
writer.SetDataModeToAscii()
writer.Write()
#print('Write Defaut(1.0)')

ex.Update() #The format 1.0 (ascii or binary)

gc = vtk.vtkHyperTreeGridGhostCells()
gc.SetInputConnection(ex.GetOutputPort())

##htg = depth.GetOutput()
#htg = ex.GetOutput()
#print('htg:',htg.GetNumberOfVertices())
#pointData = htg.GetPointData()
#field = pointData.GetArray('level')
#print('Field: level')
#print('> nb: ', field.GetNumberOfTuples())
##print(field)
#print('>range: ', field.GetRange())
#field = pointData.GetArray('scalar')
#print('Field: scalar')
#print('> nb:', field.GetNumberOfTuples())
##print(field)
#print('>range: ', field.GetRange())

#Warning The range is not goog if we fixed the max level.

#Generate a polygonal representation of a hypertree grid
geometry = vtk.vtkHyperTreeGridGeometry()
geometry.SetInputConnection(gc.GetOutputPort())

#Shrink this mesh
if True:
  shrink = vtk.vtkShrinkFilter()
  shrink.SetInputConnection(geometry.GetOutputPort())
  shrink.SetShrinkFactor(.8)
else:
  shrink = geometry

#Create a new window
renWin = vtk.vtkRenderWindow()
renWin.SetWindowName( "Generator HTG" )
renWin.SetSize( 800, 400 )

for nameField in ['level', 'scalar']:
  shrink.Update()
  dataRange = shrink.GetOutput().GetCellData().GetArray(nameField).GetRange()
  #print('> dataRange:',dataRange)

  # LookupTable
  lut = vtk.vtkLookupTable()
  lut.SetHueRange(0.66, 0)
  lut.Build()

  #Create a mapper
  mapper = vtk.vtkDataSetMapper()
  mapper.SetInputConnection( shrink.GetOutputPort() )

  mapper.SetLookupTable(lut)
  mapper.SetColorModeToMapScalars()
  mapper.SetScalarModeToUseCellFieldData()
  mapper.SelectColorArray(nameField)
  mapper.SetScalarRange(dataRange[0], dataRange[1])

  #Connect the mapper to an actor
  actor = vtk.vtkActor()
  actor.SetMapper( mapper )

  #Create a renderer and add the actor to ir
  renderer = vtk.vtkRenderer()
  renderer.SetBackground( 0., 0., 0. )
  renderer.AddActor( actor )

  if nameField == 'level':
    renderer.SetViewport( 0., 0., 0.5, 1. ) #xmin,ymin,xmax,ymax
  else:
    renderer.SetViewport( 0.5, 0., 1., 1. ) #xmin,ymin,xmax,ymax

  renWin.AddRenderer( renderer )

iren = None
if False: #True, if with interactor
  #Create an interactor
  iren = vtk.vtkRenderWindowInteractor()
  iren.SetRenderWindow( renWin )

#Initialize the interaction and start the rendering loop
if iren is not None:
  iren.Initialize()

renWin.Render()

if iren is not None:
  iren.Start()

if False:
  # screenshot code:
  w2if = vtk.vtkWindowToImageFilter()
  w2if.SetInput(renWin)
  # not exist w2if.SetMagnification(3) # set the resolution of the output image (3 times the current resolution of vtk render window)
  w2if.SetInputBufferTypeToRGBA() # also record the alpha (transparency) channel
  w2if.ReadFrontBufferOff(); # read from the back buffer
  w2if.Update()

  writer = vtk.vtkPNGWriter()
  writer.SetFileName("screenshot.png")
  writer.SetInputConnection(w2if.GetOutputPort())
  writer.Write()
