/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLReaderChangingBlocksOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

int TestXMLReaderChangingBlocksOverTime(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  const std::string dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  std::string filename(dataRoot);
  filename += "/Data/ChangingBlocksOverTime/wavelet_0.vtm";

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(filename.c_str());
  reader->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  filename = dataRoot + "/Data/ChangingBlocksOverTime/wavelet_1.vtm";
  reader->SetFileName(filename.c_str());
  reader->UpdatePiece(contr->GetLocalProcessId(), contr->GetNumberOfProcesses(), 0);

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return EXIT_SUCCESS;
}
