/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStructuredGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetTriangleFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPUnstructuredGridGhostCellsGenerator.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include <sstream>
#include <string>

namespace
{
// An RTAnalyticSource that generates GlobalNodeIds
class vtkRTAnalyticSource2 : public vtkRTAnalyticSource
{
public:
  static vtkRTAnalyticSource2* New();
  vtkTypeMacro(vtkRTAnalyticSource2, vtkRTAnalyticSource);

protected:
  vtkRTAnalyticSource2() {}

  virtual void ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo) override
  {
    Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    vtkImageData* data = vtkImageData::GetData(outInfo);
    int* outExt = data->GetExtent();
    int* whlExt = this->GetWholeExtent();

    // find the region to loop over
    int maxX = (outExt[1] - outExt[0]) + 1;
    int maxY = (outExt[3] - outExt[2]) + 1;
    int maxZ = (outExt[5] - outExt[4]) + 1;

    int dX = (whlExt[1] - whlExt[0]) + 1;
    int dY = (whlExt[3] - whlExt[2]) + 1;

    vtkNew<vtkIdTypeArray> ids;
    ids->SetName("GlobalNodeIds");
    ids->SetNumberOfValues(maxX * maxY * maxZ);
    data->GetPointData()->SetGlobalIds(ids);

    vtkIdType cnt = 0;
    for (int idxZ = 0; idxZ < maxZ; idxZ++)
    {
      for (int idxY = 0; idxY < maxY; idxY++)
      {
        for (int idxX = 0; idxX < maxX; idxX++, cnt++)
        {
          ids->SetValue(
            cnt, (idxX + outExt[0]) + (idxY + outExt[2]) * dX + (idxZ + outExt[4]) * (dX * dY));
        }
      }
    }
  }

private:
  vtkRTAnalyticSource2(const vtkRTAnalyticSource2&) = delete;
  void operator=(const vtkRTAnalyticSource2&) = delete;
};

vtkStandardNewMacro(vtkRTAnalyticSource2);

bool CheckFieldData(vtkFieldData* fd)
{
  vtkUnsignedCharArray* fdArray = vtkUnsignedCharArray::SafeDownCast(fd->GetArray("FieldData"));
  if (!fdArray || fdArray->GetValue(0) != 2)
  {
    cerr << "Field data array value is not the same as the input" << endl;
    return false;
  }

  return true;
}

} // anonymous namespace

//------------------------------------------------------------------------------
// Program main
int TestPUnstructuredGridGhostCellsGenerator(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;
  // Initialize the MPI controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create the pipeline to produce the initial grid
  vtkNew<vtkRTAnalyticSource2> wavelet;
  const int gridSize = 50;
  wavelet->SetWholeExtent(0, gridSize, 0, gridSize, 0, gridSize);
  vtkNew<vtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(wavelet->GetOutputPort());
  tetrahedralize->UpdatePiece(myRank, nbRanks, 0);

  vtkNew<vtkUnstructuredGrid> initialGrid;
  initialGrid->ShallowCopy(tetrahedralize->GetOutput());

  // Add field data
  vtkNew<vtkUnsignedCharArray> fdArray;
  fdArray->SetNumberOfTuples(1);
  fdArray->SetName("FieldData");
  fdArray->SetValue(0, 2);
  vtkNew<vtkFieldData> fd;
  fd->AddArray(fdArray);
  initialGrid->SetFieldData(fd);

  // Prepare the ghost cells generator
  vtkNew<vtkPUnstructuredGridGhostCellsGenerator> ghostGenerator;
  ghostGenerator->SetInputData(initialGrid);
  ghostGenerator->SetController(controller);
  ghostGenerator->UseGlobalPointIdsOn();

  // Check BuildIfRequired option
  ghostGenerator->BuildIfRequiredOff();
  ghostGenerator->UpdatePiece(myRank, nbRanks, 0);

  if (ghostGenerator->GetOutput()->GetCellGhostArray() == nullptr)
  {
    cerr << "Ghost were not generated but were explicitly requested on process "
         << controller->GetLocalProcessId() << endl;
    ret = EXIT_FAILURE;
  }

  ghostGenerator->BuildIfRequiredOn();
  ghostGenerator->UpdatePiece(myRank, nbRanks, 0);

  if (ghostGenerator->GetOutput()->GetCellGhostArray())
  {
    cerr << "Ghost were generated but were not requested on process "
         << controller->GetLocalProcessId() << endl;
    ret = EXIT_FAILURE;
  }

  // Check that field data is copied
  ghostGenerator->Update();
  if (!CheckFieldData(ghostGenerator->GetOutput()->GetFieldData()))
  {
    cerr << "Field data was not copied correctly" << std::endl;
    ret = EXIT_FAILURE;
  }

  // Check if algorithm works with empty input on all nodes except first one
  vtkNew<vtkUnstructuredGrid> emptyGrid;
  ghostGenerator->SetInputData(myRank == 0 ? initialGrid : emptyGrid);
  for (int step = 0; step < 2; ++step)
  {
    ghostGenerator->SetUseGlobalPointIds(step == 0 ? 1 : 0);
    ghostGenerator->UpdatePiece(myRank, nbRanks, 1);
  }
  ghostGenerator->SetInputData(initialGrid);
  ghostGenerator->Modified();

  // Check ghost cells generated with and without the global point ids
  // for several ghost layer levels
  int maxGhostLevel = 2;
  vtkSmartPointer<vtkUnstructuredGrid> outGrids[2];
  for (int ghostLevel = 1; ghostLevel <= maxGhostLevel; ++ghostLevel)
  {
    for (int step = 0; step < 2; ++step)
    {
      ghostGenerator->SetUseGlobalPointIds(step == 0 ? 1 : 0);
      ghostGenerator->Modified();
      vtkNew<vtkTimerLog> timer;
      timer->StartTimer();
      ghostGenerator->UpdatePiece(myRank, nbRanks, ghostLevel);
      timer->StopTimer();

      // Save the grid for further analysis
      outGrids[step] = ghostGenerator->GetOutput();

      if (!CheckFieldData(outGrids[step]->GetFieldData()))
      {
        cerr << "Field data was not copied" << std::endl;
        ret = EXIT_FAILURE;
      }

      double elapsed = timer->GetElapsedTime();

      // get some performance statistics
      double minGhostUpdateTime = 0.0;
      double maxGhostUpdateTime = 0.0;
      double avgGhostUpdateTime = 0.0;
      controller->Reduce(&elapsed, &minGhostUpdateTime, 1, vtkCommunicator::MIN_OP, 0);
      controller->Reduce(&elapsed, &maxGhostUpdateTime, 1, vtkCommunicator::MAX_OP, 0);
      controller->Reduce(&elapsed, &avgGhostUpdateTime, 1, vtkCommunicator::SUM_OP, 0);
      avgGhostUpdateTime /= static_cast<double>(nbRanks);
      if (controller->GetLocalProcessId() == 0)
      {
        cerr << "-- Ghost Level: " << ghostLevel
             << " UseGlobalPointIds: " << ghostGenerator->GetUseGlobalPointIds()
             << " Elapsed Time: min=" << minGhostUpdateTime << ", avg=" << avgGhostUpdateTime
             << ", max=" << maxGhostUpdateTime << endl;
      }
    }

    vtkIdType initialNbOfCells = initialGrid->GetNumberOfCells();

    // quantitative correct values for runs with 4 MPI processes
    // components are for [ghostlevel][procid][bounds]
    vtkIdType correctCellCounts[2] = { 675800 / 4, 728800 / 4 };
    double correctBounds[2][4][6] = {
      {
        { 0.000000, 50.000000, 0.000000, 26.000000, 0.000000, 26.000000 },
        { 0.000000, 50.000000, 24.000000, 50.000000, 0.000000, 26.000000 },
        { 0.000000, 50.000000, 0.000000, 26.000000, 24.000000, 50.000000 },
        { 0.000000, 50.000000, 24.000000, 50.000000, 24.000000, 50.000000 },
      },
      { { 0.000000, 50.000000, 0.000000, 27.000000, 0.000000, 27.000000 },
        { 0.000000, 50.000000, 23.000000, 50.000000, 0.000000, 27.000000 },
        { 0.000000, 50.000000, 0.000000, 27.000000, 23.000000, 50.000000 },
        { 0.000000, 50.000000, 23.000000, 50.000000, 23.000000, 50.000000 } }
    };
    for (int step = 0; step < 2; ++step)
    {
      if (nbRanks == 4)
      {
        if (outGrids[step]->GetNumberOfCells() != correctCellCounts[ghostLevel - 1])
        {
          cerr << "Wrong number of cells on process " << myRank << " for " << ghostLevel
               << " ghost levels!\n";
          ret = EXIT_FAILURE;
        }
        double bounds[6];
        outGrids[step]->GetBounds(bounds);
        for (int i = 0; i < 6; i++)
        {
          if (std::abs(bounds[i] - correctBounds[ghostLevel - 1][myRank][i]) > .001)
          {
            cerr << "Wrong bounds for " << ghostLevel << " ghost levels!\n";
            ret = EXIT_FAILURE;
          }
        }
      }

      vtkUnsignedCharArray* ghosts =
        vtkArrayDownCast<vtkUnsignedCharArray>(outGrids[step]->GetCellGhostArray());
      if (initialNbOfCells >= outGrids[step]->GetNumberOfCells())
      {
        cerr << "Obtained grids for ghost level " << ghostLevel
             << " has less or as many cells as the input grid!\n";
        ret = EXIT_FAILURE;
      }
      if (!ghosts)
      {
        cerr << "Ghost cells array not found at ghost level " << ghostLevel << ", step " << step
             << "!\n";
        ret = EXIT_FAILURE;
        continue;
      }

      for (vtkIdType i = 0; i < ghosts->GetNumberOfTuples(); ++i)
      {
        unsigned char val = ghosts->GetValue(i);
        if (i < initialNbOfCells && val != 0)
        {
          cerr << "Ghost Level " << ghostLevel << " Cell " << i
               << " is not supposed to be a ghost cell but it is!\n";
          ret = EXIT_FAILURE;
          break;
        }
        if (i >= initialNbOfCells && val != 1)
        {
          cerr << "Ghost Level " << ghostLevel << " Cell " << i
               << " is supposed to be a ghost cell but it's not!\n";
          ret = EXIT_FAILURE;
          break;
        }
      }
    }
  }

  controller->Finalize();
  return ret;
}
