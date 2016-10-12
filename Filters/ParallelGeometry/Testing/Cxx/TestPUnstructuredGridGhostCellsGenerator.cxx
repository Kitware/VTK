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
#include "vtkMPIUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPUnstructuredGridGhostCellsGenerator.h"
#include "vtkRTAnalyticSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

#include <sstream>
#include <string>

// An RTAnalyticSource that generates GlobalNodeIds
class vtkRTAnalyticSource2 : public vtkRTAnalyticSource
{
public:
  static vtkRTAnalyticSource2 *New();
  vtkTypeMacro(vtkRTAnalyticSource2, vtkRTAnalyticSource);

protected:
  vtkRTAnalyticSource2() {}

  virtual void ExecuteDataWithInformation(vtkDataObject *output, vtkInformation *outInfo)
  {
    Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    vtkImageData *data = vtkImageData::GetData(outInfo);
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
    data->GetPointData()->SetGlobalIds(ids.Get());

    vtkIdType cnt = 0;
    for (int idxZ = 0; idxZ < maxZ; idxZ++)
    {
      for (int idxY = 0; idxY < maxY; idxY++)
      {
        for (int idxX = 0; idxX < maxX; idxX++, cnt++)
        {
          ids->SetValue(cnt, (idxX + outExt[0]) +
            (idxY + outExt[2]) * dX + (idxZ + outExt[4]) * (dX*dY));
        }
      }
    }
  }

private:
  vtkRTAnalyticSource2(const vtkRTAnalyticSource2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRTAnalyticSource2&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkRTAnalyticSource2);


//------------------------------------------------------------------------------
// Program main
int TestPUnstructuredGridGhostCellsGenerator(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;
  // Initialize the MPI controller
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller.Get());
  int rankId = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create the pipeline to produce the initial grid
  vtkNew<vtkRTAnalyticSource2> wavelet;
  const int gridSize = 50;
  wavelet->SetWholeExtent(0, gridSize, 0, gridSize, 0, gridSize);
  vtkNew<vtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(wavelet->GetOutputPort());
  tetrahedralize->UpdatePiece(rankId, nbRanks, 0);

  vtkUnstructuredGrid* initialGrid = tetrahedralize->GetOutput();

  // Prepare the ghost cells generator
  vtkNew<vtkPUnstructuredGridGhostCellsGenerator> ghostGenerator;
  ghostGenerator->SetInputData(initialGrid);
  ghostGenerator->SetController(controller.Get());
  ghostGenerator->UseGlobalPointIdsOn();

  // Check BuildIfRequired option
  ghostGenerator->BuildIfRequiredOff();
  ghostGenerator->UpdatePiece(rankId, nbRanks, 0);

  if (ghostGenerator->GetOutput()->GetCellGhostArray() == NULL)
  {
    vtkMPIUtilities::Printf(controller.Get(),
      "Ghost were not generated but were explicitely requested!\n");
    ret = EXIT_FAILURE;
  }

  ghostGenerator->BuildIfRequiredOn();
  ghostGenerator->UpdatePiece(rankId, nbRanks, 0);

  if (ghostGenerator->GetOutput()->GetCellGhostArray())
  {
    vtkMPIUtilities::Printf(controller.Get(),
      "Ghost were generated but were not requested!\n");
    ret = EXIT_FAILURE;
  }

  // Check if algorithm works with empty input on all nodes except first one
  vtkNew<vtkUnstructuredGrid> emptyGrid;
  ghostGenerator->SetInputData(rankId == 0 ? initialGrid : emptyGrid.Get());
  ghostGenerator->UpdatePiece(rankId, nbRanks, 1);
  ghostGenerator->SetInputData(initialGrid);
  ghostGenerator->Modified();

  // Check ghost cells generated with and without the global point ids
  // for several ghost layer levels
  int maxGhostLevel = 2;
  vtkUnstructuredGrid* outGrids[2];
  for(int ghostLevel = 1; ghostLevel <= maxGhostLevel; ++ghostLevel)
  {
    for(int step = 0; step < 2; ++step)
    {
      ghostGenerator->SetUseGlobalPointIds(step == 0 ? 1 : 0);

      vtkNew<vtkTimerLog> timer;
      timer->StartTimer();
      ghostGenerator->UpdatePiece(rankId, nbRanks, ghostLevel);
      timer->StopTimer();

      // Save the grid for further analysis
      outGrids[step] = ghostGenerator->GetOutput();
      outGrids[step]->Register(0);

      double elapsed = timer->GetElapsedTime();

      // get some performance statistics
      double minGhostUpdateTime = 0.0;
      double maxGhostUpdateTime = 0.0;
      double avgGhostUpdateTime = 0.0;
      controller->Reduce(&elapsed, &minGhostUpdateTime, 1, vtkCommunicator::MIN_OP, 0);
      controller->Reduce(&elapsed, &maxGhostUpdateTime, 1, vtkCommunicator::MAX_OP, 0);
      controller->Reduce(&elapsed, &avgGhostUpdateTime, 1, vtkCommunicator::SUM_OP, 0);
      avgGhostUpdateTime /= static_cast<double>(nbRanks);
      vtkMPIUtilities::Printf(controller.Get(),
        "-- Ghost Level: %i Elapsed Time: min=%f, avg=%f, max=%f\n",
        ghostLevel, minGhostUpdateTime, avgGhostUpdateTime, maxGhostUpdateTime);
    }

    vtkIdType initialNbOfCells = initialGrid->GetNumberOfCells();
    if (outGrids[0]->GetNumberOfCells() != outGrids[1]->GetNumberOfCells())
    {
      vtkMPIUtilities::Printf(controller.Get(),
        "Grids obtained with and without global ids for ghost level %i do not have the same number of cells!\n",
        ghostLevel);
      ret = EXIT_FAILURE;
    }

    for (int step = 0; step < 2; ++step)
    {
      vtkUnsignedCharArray* ghosts = vtkArrayDownCast<vtkUnsignedCharArray>(
        outGrids[step]->GetCellGhostArray());
      if (initialNbOfCells >= outGrids[step]->GetNumberOfCells())
      {
        vtkMPIUtilities::Printf(controller.Get(),
          "Obtained grids for ghost level %i has less or as many cells as the input grid!\n",
          ghostLevel);
        ret = EXIT_FAILURE;
      }
      if (!ghosts)
      {
        vtkMPIUtilities::Printf(controller.Get(),
          "Ghost cells array not found at ghost level %i, step %d!\n",
          ghostLevel, step);
        ret = EXIT_FAILURE;
        continue;
      }

      for (vtkIdType i = 0; i < ghosts->GetNumberOfTuples(); ++i)
      {
        unsigned char val = ghosts->GetValue(i);
        if (i < initialNbOfCells && val != 0)
        {
          vtkMPIUtilities::Printf(controller.Get(),
            "Ghost Level %i Cell %d is not supposed to be a ghost cell but it is!\n",
            ghostLevel, i);
          ret = EXIT_FAILURE;
          break;
        }
        if (i >= initialNbOfCells && val != 1)
        {
          vtkMPIUtilities::Printf(controller.Get(),
            "Ghost Level %i Cell %d is supposed to be a ghost cell but it's not!\n",
            ghostLevel, i);
          ret = EXIT_FAILURE;
          break;
        }
      }
    }

    outGrids[0]->Delete();
    outGrids[1]->Delete();
  }


  controller->Finalize();
  return ret;
}
