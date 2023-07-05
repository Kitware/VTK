// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBoundingBox.h"
#include "vtkDistributedPointCloudFilter.h"
#include "vtkDoubleArray.h"
#include "vtkGenerateProcessIds.h"
#include "vtkIdFilter.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"

#ifdef DEBUG_ON
#include "vtkXMLPPolyDataWriter.h"
#endif

#include <sstream>

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

  const int finalNumberOfPointsPerRank = 40;
  const int totalNumberOfPoints = numberOfProcessors * finalNumberOfPointsPerRank;
  const int initialNumberOfPoints = totalNumberOfPoints / (numberOfProcessors > 1 ? 2 : 1);
  vtkNew<vtkPolyData> inputPoly;
  // Create random set of points on the two first ranks only
  if (rank == 0 || rank == 1)
  {
    vtkNew<vtkMinimalStandardRandomSequence> random;
    random->Initialize(rank);

    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(initialNumberOfPoints);
    inputPoly->SetPoints(points);

    vtkNew<vtkDoubleArray> data;
    data->SetNumberOfValues(initialNumberOfPoints);
    data->SetName("ReverseOrder");
    inputPoly->GetPointData()->AddArray(data);

    vtkNew<vtkStringArray> sdata;
    sdata->SetNumberOfValues(initialNumberOfPoints);
    sdata->SetName("RankString");
    inputPoly->GetPointData()->AddArray(sdata);

    std::stringstream ss;
    ss << "Rank_" << rank;

    for (vtkIdType i = 0; i < initialNumberOfPoints; i++)
    {
      double coords[3];
      coords[0] = random->GetValue();
      random->Next();
      coords[1] = random->GetValue();
      random->Next();
      coords[2] = random->GetValue();
      random->Next();
      points->SetPoint(i, coords);
      data->SetValue(i, totalNumberOfPoints - i - 1);
      sdata->SetValue(i, ss.str());
    }
  }

  // attach initial ids and process ids
  vtkNew<vtkIdFilter> idFilter;
  idFilter->SetInputData(inputPoly);
  idFilter->SetPointIdsArrayName("OriginalId");
  idFilter->SetCellIdsArrayName("OriginalId");
  vtkNew<vtkGenerateProcessIds> procIdScalars;
  procIdScalars->SetInputConnection(idFilter->GetOutputPort());
  procIdScalars->Update();
  procIdScalars->GetOutput()->GetPointData()->GetProcessIds()->SetName("OriginalProcessIds");
  procIdScalars->GetOutput()->GetPointData()->SetActiveAttribute(
    -1, vtkDataSetAttributes::PROCESSIDS);

  // distribute the points over the processors
  vtkNew<vtkDistributedPointCloudFilter> filter;
  filter->SetInputConnection(procIdScalars->GetOutputPort());

  // attach new process ids
  vtkNew<vtkGenerateProcessIds> outProcIdScalars;
  outProcIdScalars->SetInputConnection(filter->GetOutputPort());
  outProcIdScalars->Update();
  vtkPolyData* outputPoly = vtkPolyData::SafeDownCast(outProcIdScalars->GetOutput());

  bool error = false;
  int nbOfLocallyReceivedPoints = outputPoly->GetNumberOfPoints();
  if (nbOfLocallyReceivedPoints != finalNumberOfPointsPerRank)
  {
    cerr << "No point on the node " << rank << "\n";
    // do not exit here so MPI can end correctly
    error = true;
  }

  if (outputPoly->GetPointData()->GetNumberOfArrays() != 5)
  {
    cerr << "Incorrect number of point data arrays on rank " << rank << "\n";
    error = true;
  }

  double bounds[6];
  outputPoly->GetBounds(bounds);
  vtkBoundingBox bbox(bounds);
  if (!bbox.IsValid() || bbox.GetLength(0) == 0. || bbox.GetLength(1) == 0. ||
    bbox.GetLength(2) == 0.)
  {
    cerr << "Incorrect bounding box of output points on rank " << rank << "\n";
    error = true;
  }

  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  std::vector<int> nbOfReceivedPoints(numberOfProcessors);
  com->AllGather(&nbOfLocallyReceivedPoints, nbOfReceivedPoints.data(), 1);

  int totalNumberOfReceivedPoints = 0;
  for (vtkIdType i = 0; i < numberOfProcessors; i++)
  {
    totalNumberOfReceivedPoints += nbOfReceivedPoints[i];
  }

  if (totalNumberOfReceivedPoints != totalNumberOfPoints)
  {
    cerr << "Wrong total of points: " << totalNumberOfReceivedPoints << " instead of "
         << totalNumberOfPoints << "\n";
    cerr << "Rank " << rank << ":";
    for (int i = 0; i < nbOfLocallyReceivedPoints; i++)
    {
      cout << " " << outputPoly->GetPoints()->GetPoint(i)[0];
    }
    cerr << endl;
    error = true;
  }

  controller->Finalize();

  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
