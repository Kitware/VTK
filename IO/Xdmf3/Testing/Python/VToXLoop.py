"""
This test verifies that vtk's Xdmf reader and writer work in general.
It generates a variety of small data sets, writing each one
to and reading it from an xdmf file and compares the read in
generated result with the read in result and passes if each
closely matches.
"""

from __future__ import print_function

import os
import sys
import vtk

hasresource = True
try:
  import resource
except ImportError:
  hasresource = False

try:
  import argparse
except ImportError:
  from vtk.util import _argparse as argparse


CleanUpGood = True
LightDataLimit = 10000
OutputDir = ""

timer = vtk.vtkTimerLog()

testObjects = [
    "ID1",
    "ID2",
    "UF1",
    "RG1",
    "SG1",
    "PD1",
    "PD2",
    "UG1",
    "UG2",
    "UG3",
    "UG4",
    "MB{}",
    "MB{ID1}",
    "MB{UF1}",
    "MB{RG1}",
    "MB{SG1}",
    "MB{PD1}",
    "MB{UG1}",
    "MB{ ID1 UF1 RG1 SG1 PD1 UG1 }"
    ]

def MemUsage(point):
  if hasresource:
    usage=resource.getrusage(resource.RUSAGE_SELF)
    return '''%s: usertime=%s systime=%s mem=%s mb\
      '''%(point,usage[0],usage[1],\
          (usage[2]*resource.getpagesize())/1000000.0 )
  else:
    return '''%s:[cpu stats unavailable]'''%point

def raiseErrorAndExit(message):
  raise Exception(message)
  sys.exit(vtk.VTK_ERROR)

def DoFilesExist(xdmfFile, hdf5File, vtkFile, deleteIfSo):
  global CleanUpGood
  xexists = os.path.exists(xdmfFile)
  xlenOK = os.path.getsize(xdmfFile) > 0
  if hdf5File:
    hexists = os.path.exists(hdf5File)
    hlenOK = True #os.path.getsize(hdf5File) > 0
  if vtkFile:
    vexists = os.path.exists(vtkFile)
    vlenOK = os.path.getsize(vtkFile) > 0

  theyDo = xexists and xlenOK
  #if hdf5File:
  #  theyDo = theyDo and hexists and hlenOK
  if vtkFile:
    theyDo = theyDo and vexists and vlenOK

  if theyDo and deleteIfSo and CleanUpGood:
    os.remove(xdmfFile)
    if hexists:
      os.remove(hdf5File)
    if vtkFile:
      os.remove(vtkFile)

  return theyDo

def DoDataObjectsDiffer(dobj1, dobj2):
  class1 = dobj1.GetClassName()
  class2 = dobj2.GetClassName()
  if class1 == class2:
    pass
  else:
    if (((class1 == "vtkImageData") or
         (class1 == "vtkUniformGrid") or
         (class1 == "vtkStructuredPoints")) and
        ((class2 == "vtkImageData") or
         (class2 == "vtkUniformGrid") or
         (class2 == "vtkStructuredPoints"))):
      pass
    else:
      if (((class1 == "vtkPolyData") or
           (class1 == "vtkUnstructuredGrid")) and
          ((class2 == "vtkPolyData") or
           (class2 == "vtkUnstructuredGrid"))):
        pass
      else:
        if (((class1 == "vtkDirectedGraph") or
             (class1 == "vtkMutableDirectedGraph")) and
            ((class2 == "vtkDirectedGraph") or
             (class2 == "vtkMutableDirectedGraph"))):
          pass
        else:
          message = "Error: dobj1 is a " + class1 + " and dobj2 is a " + class2
          raiseErrorAndExit(message)

  if dobj1.GetFieldData().GetNumberOfArrays() !=\
     dobj2.GetFieldData().GetNumberOfArrays():
    raiseErrorAndExit("Number of field arrays test failed")

  if not dobj1.IsA("vtkPolyData") and\
     not dobj2.IsA("vtkMultiBlockDataSet") and\
     dobj1.GetActualMemorySize() != dobj2.GetActualMemorySize():
    message = "Memory size test failed"
    message += " M1 = " + str(dobj1.GetActualMemorySize())
    message += " M2 = " + str(dobj2.GetActualMemorySize())
    raiseErrorAndExit(message)

  ds1 = vtk.vtkDataSet.SafeDownCast(dobj1)
  ds2 = vtk.vtkDataSet.SafeDownCast(dobj2)
  if ds1 and ds2:
    if (ds1.GetNumberOfCells() != ds2.GetNumberOfCells()) or\
       (ds1.GetNumberOfPoints() != ds2.GetNumberOfPoints()):
      message = "Number of Cells/Points test Failed."
      message += " C1 = " + str(ds1.GetNumberOfCells())
      message += " C2 = " + str(ds2.GetNumberOfCells())
      message += " P1 = " + str(ds1.GetNumberOfPoints())
      message += " P2 = " + str(ds2.GetNumberOfPoints())
      raiseErrorAndExit(message)
    bds1 = ds1.GetBounds()
    bds2 = ds2.GetBounds()
    if (bds1[0]!=bds2[0]) or\
       (bds1[1]!=bds2[1]) or\
       (bds1[2]!=bds2[2]) or\
       (bds1[3]!=bds2[3]) or\
       (bds1[4]!=bds2[4]) or\
       (bds1[5]!=bds2[5]):
      for x in range(0,6):
        print("%f %f," % (bds1[x], bds2[x]))
      print()
      raiseErrorAndExit("Bounds test failed")

    if (ds1.GetPointData().GetNumberOfArrays() !=\
       ds2.GetPointData().GetNumberOfArrays()) or\
       (ds1.GetCellData().GetNumberOfArrays() !=\
       ds2.GetCellData().GetNumberOfArrays()):
      message = "Number of data arrays test Failed."
      message += " CD1 = " + str(ds1.GetCellData().GetNumberOfArrays())
      message += " CD2 = " + str(ds2.GetCellData().GetNumberOfArrays())
      message += " PD1 = " + str(ds1.GetPointData().GetNumberOfArrays())
      message += " PD2 = " + str(ds2.GetPointData().GetNumberOfArrays())
      raiseErrorAndExit(message)
  else:
    g1 = vtk.vtkGraph.SafeDownCast(dobj1)
    g2 = vtk.vtkGraph.SafeDownCast(dobj2)
    if g1 and g2:
      if (g1.GetNumberOfEdges() != g2.GetNumberOfEdges()) or\
          (g1.GetNumberOfVertices() != g2.GetNumberOfVertices()):
        message = "Number of Verts/Edges test failed"
        message += " E1 = " + str(g1.GetNumberOfEdges())
        message += " E2 = " + str(g2.GetNumberOfEdges())
        message += " V1 = " + str(g1.GetNumberOfVertices())
        message += " V2 = " + str(g2.GetNumberOfVertices())
        raiseErrorAndExit(message)

      if (g1.GetVertexData().GetNumberOfArrays() !=\
          g2.GetVertexData().GetNumberOfArrays()) or\
          (g1.GetEdgeData().GetNumberOfArrays() !=\
          (g2.GetEdgeData().GetNumberOfArrays()-1)): #xdmf added edge weights
        message = "Number of data arrays test Failed."
        message += " ED1 = " + str(g1.GetEdgeData().GetNumberOfArrays())
        message += " ED2 = " + str(g2.GetEdgeData().GetNumberOfArrays()-1)
        message += " VD1 = " + str(g1.GetVertexData().GetNumberOfArrays())
        message += " VD2 = " + str(g2.GetVertexData().GetNumberOfArrays())
        raiseErrorAndExit(message)

  return False

def TestXdmfConversion(dataInput, fileName):
  global CleanUpGood, timer
  fileName = OutputDir + fileName
  xdmfFile = fileName + ".xmf"
  hdf5File = fileName + ".h5"
  vtkFile = fileName + ".vtk"

  xWriter = vtk.vtkXdmf3Writer()
  xWriter.SetLightDataLimit(LightDataLimit)
  xWriter.WriteAllTimeStepsOn()
  xWriter.SetFileName(xdmfFile)
  xWriter.SetInputData(dataInput)
  timer.StartTimer()
  xWriter.Write()
  timer.StopTimer()
  print("vtkXdmf3Writer took %f seconds to write %s" % (timer.GetElapsedTime(), xdmfFile))

  ds = vtk.vtkDataSet.SafeDownCast(dataInput)
  if ds:
    dsw = vtk.vtkDataSetWriter()
    dsw.SetFileName(vtkFile)
    dsw.SetInputData(ds)
    dsw.Write()

  if not DoFilesExist(xdmfFile, None, None, False):
    message = "Writer did not create " + xdmfFile
    raiseErrorAndExit(message)

  xReader = vtk.vtkXdmf3Reader()
  xReader.SetFileName(xdmfFile)
  timer.StartTimer()
  xReader.Update()
  timer.StopTimer()
  print("vtkXdmf3Reader took %f seconds to read %s" % (timer.GetElapsedTime(), xdmfFile))

  rOutput = xReader.GetOutputDataObject(0)

  fail = DoDataObjectsDiffer(dataInput, rOutput)

  if fail:
    raiseErrorAndExit("Xdmf conversion test failed")
  else:
    if ds:
      DoFilesExist(xdmfFile, hdf5File, vtkFile, CleanUpGood)
    else:
      DoFilesExist(xdmfFile, hdf5File, None, CleanUpGood)

def RunTest():
  fail = False

  print("TEST SET 1 - verify reader/writer work for range of canonical datasets")
  print(MemUsage("Before starting TEST SET 1"))
  dog = vtk.vtkDataObjectGenerator()
  i = 0
  for testObject in testObjects:
    fileName = "xdmfIOtest_" + str(i)
    print("Test vtk object {}".format(testObject))
    dog.SetProgram(testObject)
    dog.Update()
    TestXdmfConversion(dog.GetOutput(), fileName)
    i += 1

  print("TEST SET 2 - verify reader/writer work for Graphs")
  print(MemUsage("Before starting TEST SET 2"))
  print("Test Graph data")
  gsrc = vtk.vtkRandomGraphSource()
  gsrc.DirectedOn()
  gsrc.Update()
  gFilePrefix = "xdmfIOtest_Graph"
  gFileName = OutputDir + gFilePrefix + ".xdmf"
  ghFileName = OutputDir + gFilePrefix + ".h5"
  xWriter = vtk.vtkXdmf3Writer()
  xWriter.SetLightDataLimit(LightDataLimit)
  xWriter.SetFileName(gFileName)
  xWriter.SetInputConnection(0, gsrc.GetOutputPort(0))
  timer.StartTimer()
  xWriter.Write()
  timer.StopTimer()
  print("vtkXdmf3Writer took %f seconds to write %s" % (timer.GetElapsedTime(), gFileName))
  xReader = vtk.vtkXdmf3Reader()
  xReader.SetFileName(gFileName)
  xReader.Update()
  rOutput = xReader.GetOutputDataObject(0)
  fail = DoDataObjectsDiffer(gsrc.GetOutputDataObject(0), xReader.GetOutputDataObject(0))
  if fail:
    raiseErrorAndExit("Failed graph conversion test")
  if not DoFilesExist(gFileName, ghFileName, None, CleanUpGood):
    raiseErrorAndExit("Failed to write Graph file")

  print("TEST SET 3 - verify reader/writer handle time varying data")
  print(MemUsage("Before starting TEST SET 3"))
  print("Test temporal data")
  tsrc = vtk.vtkTimeSourceExample()
  tsrc.GrowingOn()
  tsrc.SetXAmplitude(2.0)
  tFilePrefix = "xdmfIOTest_Temporal"
  tFileName = OutputDir + tFilePrefix + ".xdmf"
  thFileName = OutputDir + tFilePrefix + ".h5"
  xWriter = vtk.vtkXdmf3Writer()
  xWriter.SetLightDataLimit(LightDataLimit)
  xWriter.WriteAllTimeStepsOn()
  xWriter.SetFileName(tFileName)
  xWriter.SetInputConnection(0, tsrc.GetOutputPort(0))
  timer.StartTimer()
  xWriter.Write()
  timer.StopTimer()
  print("vtkXdmf3Writer took %f seconds to write %s" % (timer.GetElapsedTime(), tFileName))
  xReader = vtk.vtkXdmf3Reader()
  xReader.SetFileName(tFileName)
  xReader.UpdateInformation()
  oi = xReader.GetOutputInformation(0)
  timerange = oi.Get(vtk.vtkCompositeDataPipeline.TIME_STEPS())
  ii = tsrc.GetOutputInformation(0)
  correcttimes = ii.Get(vtk.vtkCompositeDataPipeline.TIME_STEPS())

  #compare number of and values for temporal range
  if len(timerange) != len(correcttimes):
    print("timesteps failed")
    print("{} != {}".format(timerange, correcttimes))
    raiseErrorAndExit("Failed to get same times")
  for i in range(0, len(correcttimes)):
    if abs(abs(timerange[i])-abs(correcttimes[i])) > 0.000001:
      print("time result failed")
      print("{} != {}".format(timerange, correcttimes))
      raiseErrorAndExit("Failed to get same times")

  #exercise temporal processing and compare geometric bounds at each tstep
  indices = [x for x in range(0,len(timerange))] + [x for x in range(len(timerange)-2,-1,-1)]
  for x in indices:
      xReader.UpdateTimeStep(timerange[x])
      obds = xReader.GetOutputDataObject(0).GetBounds()
      tsrc.UpdateTimeStep(timerange[x]+0.0001) #workaround a precision bug in TSE
      ibds = tsrc.GetOutputDataObject(0).GetBounds()
      print("{} {}".format( timerange[x], obds))
      for i in (0,1,2,3,4,5):
        if abs(abs(obds[i])-abs(ibds[i])) > 0.000001:
          print("time result failed")
          print("{} != {}".format(obds, ibds))
          raiseErrorAndExit("Failed to get same data for this timestep")
  fail = DoFilesExist(tFileName, thFileName, None, CleanUpGood)
  if not fail:
    raiseErrorAndExit("Failed Temporal Test")

  print(MemUsage("End of Testing"))

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Test Xdmf3 IO")
  parser.add_argument("-dc", "--dont-clean", dest="dontClean",\
                      help="Do not delete files created during test\
                      after testing complete (Default = False)",\
                      action="store_true")
  parser.add_argument("-T", nargs=1, dest="outputDir")
  args = parser.parse_args()
  if args.dontClean:
    CleanUpGood = False
  if args.outputDir:
    OutputDir = args.outputDir[0] + "/"

  print("Test XML Arrays")
  LightDataLimit = 10000
  RunTest()
  print("Test HDF5 Arrays")
  LightDataLimit = 0
  RunTest()
  print("That's all folks")
