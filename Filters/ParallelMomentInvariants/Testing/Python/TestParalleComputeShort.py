"""
    we cutout a random point from the given dataset,
    rotate it, scale it, multiply with a constant, add a constant
    and look for it.
    The test is considered successful if the original position is the global maximum of similarity.
    Inaccuracies come from the resolution of the dataset and the pattern as well as the coarse resolutions.
    I chose the numbers to perfectly match and the rotation angle to be a multiple of 90 degree. So that the sampling error is suppressed.
    """

import vtk
import numpy as np
import sys
import os, shutil
import math
from patternDetectionHelper import *

numberOfIntegrationSteps = 0
order = 2
numberOfMoments = 0
nameOfPointData = "values"
radius = 0.1

def compute(whole, split):

# prepare
  controller = vtk.vtkMPIController()
  vtk.vtkMultiProcessController.SetGlobalController(controller)
  rank = controller.GetLocalProcessId()
  nprocs = controller.GetNumberOfProcesses()
  #  print('rank={0}'.format(rank))
  #  print('nprocs={0}'.format(nprocs))

  # whole
  (wholeDataset, bounds, dimension) = readDataset(whole)
  if pow(2,dimension) != nprocs:
    print 'the number of procs must be 2^dimension'
    sys.exit()

  # coarse
  coarseData = vtk.vtkImageData()
  coarseData.CopyStructure(wholeDataset)
  coarseData.SetSpacing(0.1,0.1,0.1)
  if dimension == 2:
    coarseData.SetDimensions(11,11,1)
  else:
    coarseData.SetDimensions(11,11,11)
  #  print data
  #  print coarseData

  # compute moments
  momentsFilter = vtk.vtkComputeMoments()
  momentsFilter.SetFieldData(wholeDataset)
  momentsFilter.SetGridData(coarseData)
  momentsFilter.SetNameOfPointData(nameOfPointData)
  momentsFilter.SetUseFFT(0)
  momentsFilter.SetNumberOfIntegrationSteps(numberOfIntegrationSteps)
  momentsFilter.SetOrder(order)
  momentsFilter.SetRadiiArray([radius,0,0,0,0,0,0,0,0,0])
  momentsFilter.Update()
  #  if rank ==0: print momentsFilter



  # this is to make sure each proc only loads their piece
  ids_to_read = [rank]
  reqs = vtk.vtkInformation()
  reqs.Set(vtk.vtkCompositeDataPipeline.UPDATE_COMPOSITE_INDICES(), ids_to_read, 1)

  reader = vtk.vtkXMLMultiBlockDataReader()
  reader.SetFileName(split)
  reader.Update(reqs)
  #    print('reader={0}'.format(reader.GetOutput()))

  multiblock = vtk.vtkMultiBlockDataSet.SafeDownCast(reader.GetOutput())
  #    print('multiblock={0}'.format(multiblock))

  multipiece = vtk.vtkMultiPieceDataSet.SafeDownCast(multiblock.GetBlock(0))
  #    print('multipiece={0}'.format(multipiece))

  data = vtk.vtkImageData.SafeDownCast(multipiece.GetPiece(rank))
#  print('data={0}'.format(data))

  # coarse
  coarseData = vtk.vtkImageData()
  coarseData.CopyStructure(data)
  coarseData.SetSpacing(0.1,0.1,0.1)
  if dimension == 2:
    coarseData.SetDimensions(6,6,1)
  else:
    coarseData.SetDimensions(6,6,6)
  #  print data
  #  coarseData

  # parallel
  pMomentsFilter = vtk.vtkPComputeMoments()
  pMomentsFilter.SetFieldData(data)
  pMomentsFilter.SetGridData(coarseData)
  pMomentsFilter.SetNameOfPointData(nameOfPointData)
  pMomentsFilter.SetNumberOfIntegrationSteps(numberOfIntegrationSteps)
  pMomentsFilter.SetRadiiArray([radius, 0, 0, 0, 0, 0, 0, 0, 0, 0 ])
  pMomentsFilter.Update(reqs)
#  if rank ==0: print pMomentsFilter




  # check for difference except for the global boundary (the probe and parallel probe filters behave differently there. So, there is a difference if numberOfIntegrationSteps > 0 )
  diff = 0
  for i in xrange(pMomentsFilter.GetOutput().GetPointData().GetNumberOfArrays()):
    diffArray = vtk.vtkDoubleArray()
    diffArray.SetName( pMomentsFilter.GetOutput().GetPointData().GetArrayName(i) )
    diffArray.SetNumberOfComponents( 1 )
    diffArray.SetNumberOfTuples( pMomentsFilter.GetOutput().GetNumberOfPoints() )
    diffArray.Fill( 0.0 )

    momentsArray = momentsFilter.GetOutput().GetPointData().GetArray(i)
    pMomentsArray = pMomentsFilter.GetOutput().GetPointData().GetArray(i)

    for j in xrange(pMomentsFilter.GetOutput().GetNumberOfPoints()):
      diffArray.SetTuple1(j, abs(momentsArray.GetTuple1(momentsFilter.GetOutput().FindPoint(pMomentsFilter.GetOutput().GetPoint(j))) - pMomentsArray.GetTuple1(j)))
      if (dimension == 2 and pMomentsFilter.GetOutput().GetPoint(j)[0] > 0 and pMomentsFilter.GetOutput().GetPoint(j)[0] < 1 and pMomentsFilter.GetOutput().GetPoint(j)[1] > 0 and pMomentsFilter.GetOutput().GetPoint(j)[1] < 1) or (dimension == 3 and pMomentsFilter.GetOutput().GetPoint(j)[0] > 0 and pMomentsFilter.GetOutput().GetPoint(j)[0] < 1 and pMomentsFilter.GetOutput().GetPoint(j)[1] > 0 and pMomentsFilter.GetOutput().GetPoint(j)[1] < 1 and  pMomentsFilter.GetOutput().GetPoint(j)[2] > 0 and pMomentsFilter.GetOutput().GetPoint(j)[2] < 1):
#        if diffArray.GetTuple1(j) > 1e-10:
#          print rank, i, j, pMomentsFilter.GetOutput().GetPoint(j), diffArray.GetTuple(j)
        diff = max(diff, diffArray.GetTuple1(j))
  if diff > 1e-10:
    print "test failed, maxdiff =", diff
  else:
    print "test successful"



  if rank == 0:
    print 'all done!'

if __name__ == '__main__':
  if len(sys.argv) < 2:
    print 'usage: <file whole> <file split>'
  else:
      compute(sys.argv[1], sys.argv[2])
