// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCallbackCommand.h"
#include "vtkExecutive.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataWriter.h"

#include <iostream>

int TestXMLWriteError(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkXMLPolyDataWriter> writer;
  writer->SetInputData(pd);
  writer->SetFileName("this/path/does/not/exist/file.vtp");

  vtkNew<vtkCallbackCommand> nullCallback;
  nullCallback->SetCallback([](vtkObject*, unsigned long, void*, void*) {});
  writer->AddObserver(vtkCommand::ErrorEvent, nullCallback);
  writer->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, nullCallback);

  if (writer->Write() != 0)
  {
    std::cerr << "Unexpected success write to an invalid path!" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
