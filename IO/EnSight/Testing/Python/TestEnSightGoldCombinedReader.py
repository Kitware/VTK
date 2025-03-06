#!/usr/bin/env python

import math
from vtkmodules.vtkCommonDataModel import vtkDataSetAttributes
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkFiltersCore import vtkAssignAttribute
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOEnSight import vtkEnSightGoldCombinedReader
from vtkmodules.vtkIOEnSight import vtkEnSightSOSGoldReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

from vtk.test import Testing

class TestEnSightGoldCombinedReader(Testing.vtkTest):
  def setupReader(self, casefile):
    if casefile.endswith(".sos"):
      reader = vtkEnSightSOSGoldReader()
    else:
      reader = vtkEnSightGoldCombinedReader()
    reader.SetCaseFileName(str(VTK_DATA_ROOT) + "/Data/EnSight/" + casefile)
    return reader


  def geomPipeline(self, reader):
    geomFilter = vtkGeometryFilter()
    geomFilter.SetInputConnection(reader.GetOutputPort())
    return geomFilter


  def setupMapper(self, filter, var = None, arrRange = None):
    mapper = vtkCompositePolyDataMapper()
    mapper.SetInputConnection(filter.GetOutputPort())
    if var is not None:
      mapper.ColorByArrayComponent(var, 0)
    if arrRange is not None:
      mapper.SetScalarRange(arrRange[0], arrRange[1])
    return mapper


  def renderAndCompare(self, mapper, img_file, pos=None):
    ren = vtkRenderer()
    #ren.SetBackground(0.1, 0.2, 0.4)
    renWin = vtkRenderWindow()
    renWin.AddRenderer(ren)
    renWin.SetSize(300,300)

    actor = vtkActor()
    actor.SetMapper(mapper)
    ren.AddActor(actor)

    ren.ResetCamera()
    if pos is not None:
      ren.GetActiveCamera().SetPosition(pos[0], pos[1], pos[2])

    iren = vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)
    renWin.Render()

    Testing.compareImage(iren.GetRenderWindow(), Testing.getAbsImagePath(img_file))
    Testing.interact()


  def basicPipeline(self, casefile, img_file, var = None, arrRange = None, pos = None):
    reader = self.setupReader(casefile)
    geom = self.geomPipeline(reader)
    mapper = self.setupMapper(geom, var, arrRange)
    self.renderAndCompare(mapper, img_file, pos)


  def aaPipeline(self, casefile, img_file, var, arrRange):
    reader = self.setupReader(casefile)
    geom = self.geomPipeline(reader)

    aa = vtkAssignAttribute()
    aa.SetInputConnection(geom.GetOutputPort())
    aa.Assign(var, vtkDataSetAttributes.SCALARS, vtkAssignAttribute.CELL_DATA)
    aa.Update()

    mapper = self.setupMapper(aa, var, arrRange)
    self.renderAndCompare(mapper, img_file)

  def contourPipeline(self, casefile, img_file, contourValues):
    reader = self.setupReader(casefile)
    contour = vtkContourFilter()
    contour.SetInputConnection(reader.GetOutputPort())
    contour.SetNumberOfContours(len(contourValues))
    for i in range(len(contourValues)):
      contour.SetValue(i, contourValues[i])
    contour.SetComputeScalars(1)

    mapper = self.setupMapper(contour)
    self.renderAndCompare(mapper, img_file)


  def streamlinePipeline(self, casefile, img_file, pos, forward):
    reader = self.setupReader(casefile)

    streamer = vtkStreamTracer()
    streamer.SetInputConnection(reader.GetOutputPort())
    streamer.SetStartPosition(pos)
    streamer.SetMaximumPropagation(500)
    streamer.SetInitialIntegrationStep(0.1)
    if forward == True:
      streamer.SetIntegrationDirectionToForward()
    else:
      streamer.SetIntegrationDirectionToBoth()

    mapper = self.setupMapper(streamer)
    self.renderAndCompare(mapper, img_file)


  def undefAndPartialCheck(self, casefile):
    reader = self.setupReader(casefile)
    reader.Update()
    data = reader.GetOutput()
    self.assertEqual(data.GetNumberOfPartitionedDataSets(), 1)

    pds = data.GetPartitionedDataSet(0)
    self.assertEqual(pds.GetNumberOfPartitions(), 1)
    part = pds.GetPartition(0)

    mass = part.GetCellData().GetArray("mass")
    self.assertIsNotNone(mass)
    self.assertEqual(mass.GetRange()[0], 0.0)
    self.assertEqual(mass.GetRange()[1], 3.0)
    self.assertTrue(math.isnan(mass.GetValue(2)))

    pres = part.GetPointData().GetArray("pres")
    self.assertIsNotNone(pres)
    self.assertEqual(pres.GetRange()[0], 4.0)
    self.assertEqual(pres.GetRange()[1], 6.0)
    self.assertEqual(pres.GetValue(2), 4)
    self.assertEqual(pres.GetValue(4), 6)

    for i in range(0, 10):
      if i == 2 or i == 4:
        continue
      self.assertTrue(math.isnan(pres.GetValue(i)))


  def checkParticles(self, casefile):
    reader = self.setupReader(casefile)
    # first check that we have all partitions loaded
    reader.Update()

    data = reader.GetOutput()
    self.assertEqual(data.GetNumberOfPartitionedDataSets(), 4)

    pds = data.GetPartitionedDataSet(3)
    self.assertEqual(pds.GetNumberOfPartitions(), 1)

    part = pds.GetPartition(0)
    arr = part.GetPointData().GetArray("test-scalar")
    self.assertIsNotNone(arr)
    self.assertEqual(arr.GetNumberOfComponents(), 1)
    arr = part.GetPointData().GetArray("test-vector")
    self.assertIsNotNone(arr)
    self.assertEqual(arr.GetNumberOfComponents(), 3)

    # now check for when we only load the particle data
    reader.UpdateInformation()
    reader.GetPartSelection().DisableAllArrays()
    reader.GetPartSelection().EnableArray("measured particles")
    reader.Update()

    data = reader.GetOutput()
    self.assertEqual(data.GetNumberOfPartitionedDataSets(), 1)

    pds = data.GetPartitionedDataSet(0)
    self.assertEqual(pds.GetNumberOfPartitions(), 1)

    part = pds.GetPartition(0)
    arr = part.GetPointData().GetArray("test-scalar")
    self.assertIsNotNone(arr)
    self.assertEqual(arr.GetNumberOfComponents(), 1)
    arr = part.GetPointData().GetArray("test-vector")
    self.assertIsNotNone(arr)
    self.assertEqual(arr.GetNumberOfComponents(), 3)


  def testAsymTensorASCII(self):
    self.aaPipeline("pitzDaily.case", "TestEnSightGoldCombinedReader_1.png", "UGrad", [0,100])

  def testAsymTensorBinary(self):
    self.aaPipeline("RESULT_FLUID_DOMAIN.case", "TestEnSightGoldCombinedReader_2.png", "R_ns", [0.001, 0.008])

  def testBlow1ASCII(self):
    self.basicPipeline("blow1_ascii.case", "TestEnSightGoldCombinedReader_3.png", var="displacement", pos=[99.3932,17.6571,-22.6071])

  def testBlow1Binary(self):
    self.basicPipeline("blow1_bin.case", "TestEnSightGoldCombinedReader_3.png", var="displacement", pos=[99.3932,17.6571,-22.6071])

  def testBlow2ASCII(self):
    self.basicPipeline("blow2_ascii.case", "TestEnSightGoldCombinedReader_3.png", var="displacement", pos=[99.3932,17.6571,-22.6071])

  def testBlow2Binary(self):
    self.basicPipeline("blow2_bin.case", "TestEnSightGoldCombinedReader_3.png", var="displacement", pos=[99.3932,17.6571,-22.6071])

  def testElementsASCII(self):
    self.basicPipeline("elements.case", "TestEnSightGoldCombinedReader_4.png", var="pointScalars", arrRange=[0, 112])

  def testElementsBinary(self):
    self.basicPipeline("elements-bin.case", "TestEnSightGoldCombinedReader_5.png", var="pointTensors", arrRange=[0,325])

  def testElementTypesTest(self):
    self.basicPipeline("elementTypesTest.case", "TestEnSightGoldCombinedReader_6.png")

  def testEmptyPartsBinary(self):
    # this dataset has two empty parts and two non-empty parts. just tests to make sure
    # we can access the parts and that they have the arrays. empty parts still get a
    # partitioned dataset
    reader = vtkEnSightGoldCombinedReader()
    reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/emptyParts_bin.case")
    reader.Update()
    data = reader.GetOutput()

    self.assertTrue(data.GetNumberOfPartitionedDataSets() == 4)

    part1 = data.GetPartitionedDataSet(0)
    part2 = data.GetPartitionedDataSet(1)
    part3 = data.GetPartitionedDataSet(2)
    part4 = data.GetPartitionedDataSet(3)

    self.assertTrue(part1.GetPartition(0).GetCellData().HasArray("scalar_per_element"))
    self.assertTrue(part1.GetPartition(0).GetCellData().HasArray("vector_per_element"))
    self.assertTrue(part1.GetPartition(0).GetPointData().HasArray("scalar_per_node"))
    self.assertTrue(part1.GetPartition(0).GetPointData().HasArray("vector_per_node"))

    self.assertTrue(part4.GetPartition(0).GetCellData().HasArray("scalar_per_element"))
    self.assertTrue(part4.GetPartition(0).GetCellData().HasArray("vector_per_element"))
    self.assertTrue(part4.GetPartition(0).GetPointData().HasArray("scalar_per_node"))
    self.assertTrue(part4.GetPartition(0).GetPointData().HasArray("vector_per_node"))

  def testIronProtASCII(self):
    self.contourPipeline("ironProt_ascii.case", "TestEnSightGoldCombinedReader_7.png", [200])

  def testIronProtBinary(self):
    self.contourPipeline("ironProt_bin.case", "TestEnSightGoldCombinedReader_7.png", [200])

  def testMandelbrot1(self):
    self.basicPipeline("mandelbrot1.case", "TestEnSightGoldCombinedReader_8.png", var="Iterations", arrRange=[1.9,100])

  def testMandelbrot2(self):
    self.basicPipeline("mandelbrot2.case", "TestEnSightGoldCombinedReader_9.png", var="Iterations", arrRange=[1.9,100])

  def testNacaBin(self):
    self.contourPipeline("naca.bin.case", "TestEnSightGoldCombinedReader_10.png", [0.602, 0.685, 0.767, 0.850, 0.933, 1.015, 1.098, 1.181, 1.264, 1.346])

  def testEnSightGoldASCII(self):
    self.basicPipeline("ensight-gold-test-ascii.case", "TestEnSightGoldCombinedReader_11.png")

  def testEnSightGoldBinary(self):
    self.basicPipeline("ensight-gold-test-bin.case", "TestEnSightGoldCombinedReader_11.png")

  def testOfficeASCII(self):
    self.streamlinePipeline("office_ascii.case", "TestEnSightGoldCombinedReader_12.png", [0.1, 2.1, 0.5], True)

  def testOfficeBinary(self):
    self.streamlinePipeline("office_bin.case", "TestEnSightGoldCombinedReader_12.png", [0.1, 2.1, 0.5], True)

  def testRectGridASCII(self):
    self.streamlinePipeline("RectGrid_ascii.case", "TestEnSightGoldCombinedReader_13.png", [0, -0.078125, 0.4], False)

  def testRectGridBinary(self):
    self.streamlinePipeline("RectGrid_bin.case", "TestEnSightGoldCombinedReader_13.png", [0, -0.078125, 0.4], False)

  def testNfacedBinary(self):
    self.basicPipeline("TEST_bin.case", "TestEnSightGoldCombinedReader_14.png", var="Pressure", arrRange=[0.121168,0.254608])

  def testNfacedASCII(self):
    self.basicPipeline("TEST.case", "TestEnSightGoldCombinedReader_14.png", var="Pressure", arrRange=[0.121168,0.254608])

  def testParticlesASCII(self):
    self.checkParticles("ensight-gold-test-ascii.case")

  def testParticlesBinary(self):
    self.checkParticles("ensight-gold-test-bin.case")

  def testSelectParts(self):
    reader = self.setupReader("ensight-gold-test-ascii.case")
    reader.UpdateInformation()
    reader.GetPartSelection().DisableAllArrays()
    reader.GetPartSelection().EnableArray("Part 1")
    reader.GetPartSelection().EnableArray("Part 3")

    geom = self.geomPipeline(reader)
    mapper = self.setupMapper(geom, "evect")
    self.renderAndCompare(mapper, "TestEnSightGoldCombinedReader_15.png", [26.4, 2.7, 1.4])

  def testSelectArrays(self):
    reader = self.setupReader("blow1_ascii.case")
    reader.UpdateInformation()
    reader.GetPointArraySelection().DisableAllArrays()
    reader.GetPointArraySelection().EnableArray("displacement")

    reader.GetCellArraySelection().DisableAllArrays()
    reader.GetCellArraySelection().EnableArray("thickness")
    reader.Update()

    data = reader.GetOutput()
    self.assertEqual(data.GetNumberOfPartitionedDataSets(), 1)

    pds = data.GetPartitionedDataSet(0)
    self.assertEqual(pds.GetNumberOfPartitions(), 1)

    part = pds.GetPartition(0)
    arr = part.GetPointData().GetArray("displacement")
    self.assertIsNotNone(arr)
    arr = part.GetPointData().GetArray("thickness")
    self.assertIsNone(arr)
    arr = part.GetCellData().GetArray("displacement")
    self.assertIsNone(arr)
    arr = part.GetCellData().GetArray("thickness")
    self.assertIsNotNone(arr)

  def testUndefAndPartialASCII(self):
    self.undefAndPartialCheck("UndefAndPartialAscii/grid.case")

  def testUndefAndPartialBin(self):
    self.undefAndPartialCheck("UndefAndPartial/grid_bin.case")

  def testVigaBinary(self):
    self.basicPipeline("viga.case", "TestEnSightGoldCombinedReader_17.png")


  def testRigidBody(self):
    reader = self.setupReader("veh.case")
    reader.UpdateInformation()
    outInfo = reader.GetOutputInformation(0)
    numSteps = outInfo.Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
    self.assertEqual(numSteps, 21)

    outInfo.Set(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 36.0)
    reader.Update()

    geom = self.geomPipeline(reader)
    mapper = self.setupMapper(geom, "evect")
    self.renderAndCompare(mapper, "TestEnSightGoldCombinedReader_16.png", [26.4, 2.7, 1.4])

  def testSOSReader(self):
    self.basicPipeline("mandelbrot.sos", "TestEnSightGoldCombinedReader_18.png", var="Iterations", arrRange=[1.9,100])

  def checkTimeStep(self, reader, time, img_file, var, pos):
    outInfo = reader.GetOutputInformation(0)
    outInfo.Set(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), time)
    reader.Update()

    geom = self.geomPipeline(reader)
    mapper = self.setupMapper(geom, var)
    self.renderAndCompare(mapper, img_file, pos)

  def testTimeWithTimeSets(self):
    reader = self.setupReader("blow2_ascii.case")
    reader.UpdateInformation()

    outInfo = reader.GetOutputInformation(0)
    numSteps = outInfo.Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
    self.assertEqual(numSteps, 2)

    self.checkTimeStep(reader, 0.0, "TestEnSightGoldCombinedReader_3.png", var="displacement", pos=[99.3932,17.6571,-22.6071])
    self.checkTimeStep(reader, 0.1, "TestEnSightGoldCombinedReader_19.png", var="displacement", pos=[99.3932,17.6571,-22.6071])

  def testTimeWithFileSets(self):
    reader = self.setupReader("blow1_ascii.case")
    reader.UpdateInformation()

    outInfo = reader.GetOutputInformation(0)
    numSteps = outInfo.Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
    self.assertEqual(numSteps, 2)

    self.checkTimeStep(reader, 0.0, "TestEnSightGoldCombinedReader_3.png", var="displacement", pos=[99.3932,17.6571,-22.6071])
    self.checkTimeStep(reader, 0.1, "TestEnSightGoldCombinedReader_19.png", var="displacement", pos=[99.3932,17.6571,-22.6071])


if __name__ == "__main__":
    Testing.main([(TestEnSightGoldCombinedReader, 'test')])
