/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistributedPointCloudFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDistributedPointCloudFilter.h"
#include "vtkDoubleArray.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTrivialProducer.h"

int TestDistributedPointCloudFilter(int argc, char* argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (controller != nullptr));
  vtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();
  const int numberOfProcessors = controller->GetNumberOfProcesses();
  assert("pre: NumberOfProcessors >= 1" && (numberOfProcessors >= 1));
  assert("pre: Rank is out-of-bounds" && (rank >= 0));

  const int finalNumberOfPointsPerRank = 4;
  const int totalNumberOfPoints = numberOfProcessors * finalNumberOfPointsPerRank;
  const int initialNumberOfPoints = totalNumberOfPoints / 2 ;
  vtkNew<vtkPolyData> inputPoly;
  // create points with x=y=z
  if (rank == 0 || rank == 1)
  {
    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(initialNumberOfPoints);
    vtkNew<vtkDoubleArray> data;
    data->SetNumberOfValues(initialNumberOfPoints);
    data->SetName("ReverseOrder");
    for (vtkIdType i = 0; i < initialNumberOfPoints; i++)
    {
      double coord = 10 * (i + rank * initialNumberOfPoints);
      points->SetPoint(i, coord, coord, coord);
      data->SetValue(i, totalNumberOfPoints - i);
    }
    inputPoly->SetPoints(points.Get());
    inputPoly->GetPointData()->AddArray(data);
  }

  vtkNew<vtkDistributedPointCloudFilter> filter;
  filter->SetInputData(inputPoly.Get());
  filter->Update();
  vtkPointSet* outputPoly = filter->GetOutput();

  bool error = false;
  int nbOfLocallyReceivedPoints = outputPoly->GetNumberOfPoints();
  if (nbOfLocallyReceivedPoints != finalNumberOfPointsPerRank)
  {
    cout << "No point on the node " << rank << "\n";
    // do not exit here so MPI can end correctly
    error = true;
  }

  if (outputPoly->GetPointData()->GetNumberOfArrays() != 1)
  {
    cout << "No point data on the node " << rank<< "\n";
    error = true;
  }

  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  int* nbOfReceivedPoints = new int[numberOfProcessors];
  com->AllGatherVoidArray(&nbOfLocallyReceivedPoints, nbOfReceivedPoints, 1, VTK_INT);

  int totalNumberOfReceivedPoints = 0;
  for (vtkIdType i = 0; i < numberOfProcessors; i++)
  {
    totalNumberOfReceivedPoints += nbOfReceivedPoints[i];
  }

  if (totalNumberOfReceivedPoints != totalNumberOfPoints)
  {
    cout << "Wrong total of points: " << totalNumberOfReceivedPoints << " instead of "
         << totalNumberOfPoints << "\n";
    cout << "Rank " << rank << ":";
    for (int i = 0; i < nbOfLocallyReceivedPoints; i++)
    {
      cout << " " << outputPoly->GetPoints()->GetPoint(i)[0];
    }
    cout << endl;
    error = true;
  }

  delete[] nbOfReceivedPoints;

  controller->Finalize();

  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
