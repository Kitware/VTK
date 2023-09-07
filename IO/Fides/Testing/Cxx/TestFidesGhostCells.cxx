// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObject.h"
#include "vtkFidesReader.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTestUtilities.h"
#include "vtkTuple.h"
#include "vtkVariant.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include <iostream>
#include <sstream>

int TestFidesGhostCells(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* contr = vtkMPIController::New();
#else
  vtkDummyController* contr = vtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  std::string bpFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/gs.bp");

  int me = contr->GetLocalProcessId();
  int numProcs = contr->GetNumberOfProcesses();

  vtkNew<vtkFidesReader> reader;
  reader->SetFileName(bpFile);
  reader->ConvertToVTKOn();

  using vtkTuple6i = vtkTuple<int, 6>;
  // These valid extents were recorded for 2 processes and the gray-scott data file.
  vtkTuple6i validExtents[3][2][2]; // 3 ghostLevels, 2 numProceses, 2 numPartsPerProcess
  validExtents[0][0][0] = vtkTuple6i({ 0, 31, 0, 31, 0, 63 });
  validExtents[0][0][1] = vtkTuple6i({ 0, 31, 31, 63, 0, 63 });
  validExtents[0][1][0] = vtkTuple6i({ 31, 63, 0, 31, 0, 63 });
  validExtents[0][1][1] = vtkTuple6i({ 31, 63, 31, 63, 0, 63 });
  validExtents[1][0][0] = vtkTuple6i({ 0, 32, 0, 32, 0, 63 });
  validExtents[1][0][1] = vtkTuple6i({ 0, 32, 30, 63, 0, 63 });
  validExtents[1][1][0] = vtkTuple6i({ 30, 63, 0, 32, 0, 63 });
  validExtents[1][1][1] = vtkTuple6i({ 30, 63, 30, 63, 0, 63 });
  validExtents[2][0][0] = vtkTuple6i({ 0, 33, 0, 33, 0, 63 });
  validExtents[2][0][1] = vtkTuple6i({ 0, 33, 29, 63, 0, 63 });
  validExtents[2][1][0] = vtkTuple6i({ 29, 63, 0, 33, 0, 63 });
  validExtents[2][1][1] = vtkTuple6i({ 29, 63, 29, 63, 0, 63 });

  for (int ghostLevels = 0; ghostLevels < 3; ++ghostLevels)
  {
    reader->UpdatePiece(me, numProcs, ghostLevels);
    auto output = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
    auto pds = output->GetPartitionedDataSet(0);
    std::cout << "\nNumberOfGhostLevels: " << ghostLevels << "\n\t" << pds->GetNumberOfPartitions()
              << " partition (s) on process " << me << "\n";
    for (unsigned int partIdx = 0; partIdx < pds->GetNumberOfPartitions(); ++partIdx)
    {
      auto ds = vtkImageData::SafeDownCast(pds->GetPartition(partIdx));
      vtkTuple<int, 6> extent;
      const auto& validExtent = validExtents[ghostLevels][me][partIdx];
      ds->GetExtent(extent.GetData());
      std::cout << "\t\tpart " << partIdx << " " << extent << '\n';
      if (extent != validExtent)
      {
        std::cerr << "GhostLevels: " << ghostLevels << " rank: " << me << " partIdx: " << partIdx
                  << " got " << extent << "but valid extent is " << validExtent << "\n";
        return 1;
      }
    }
  }

  contr->Finalize();
  contr->Delete();
  return 0;
}
