// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkTecplotReader
// .SECTION Description
//

#include "vtkTecplotReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkTestUtilities.h"
#include <vtksys/SystemTools.hxx>

#include <iostream>
#include <string>

class vtkErrorObserver
{

public:
  static void Reset()
  {
    HasError = false;
    ErrorMessage.clear();
  }

  static void OnError(vtkObject* vtkNotUsed(caller), unsigned long int vtkNotUsed(eventId),
    void* vtkNotUsed(clientData), void* callData)
  {
    HasError = true;
    char* pString = (char*)callData;
    if (pString)
    {
      ErrorMessage = pString;
    }
  }

  static bool HasError;
  static std::string ErrorMessage;
};

bool vtkErrorObserver::HasError = false;
std::string vtkErrorObserver::ErrorMessage = std::string();

int TestTecplotReader2(int argc, char* argv[])
{
  char* dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  const std::string tecplotDir = std::string(dataRoot) + "/Data/TecPlot/";

  if (argc < 2)
  {
    return EXIT_SUCCESS;
  }

  const char* filename = argv[1];

  vtkNew<vtkCallbackCommand> cmd;
  cmd->SetCallback(&(vtkErrorObserver::OnError));

  const std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
  if (ext != ".dat")
  {
    return EXIT_FAILURE;
  }

  vtkErrorObserver::Reset();
  vtkNew<vtkTecplotReader> r;
  r->AddObserver("ErrorEvent", cmd);
  r->SetFileName((tecplotDir + filename).c_str());
  r->Update();
  r->RemoveAllObservers();

  vtkMultiBlockDataSet* ds = r->GetOutput();
  if (ds == nullptr)
  {
    std::cerr << "Failed to read data set from " << filename << std::endl;
    return EXIT_FAILURE;
  }
  if (vtkErrorObserver::HasError)
  {
    std::cerr << "Failed to read from " << filename << std::endl;
    if (!vtkErrorObserver::ErrorMessage.empty())
    {
      std::cerr << "Error message: " << vtkErrorObserver::ErrorMessage << std::endl;
    }
    return EXIT_FAILURE;
  }

  std::cout << filename << " was read without errors." << std::endl;
  delete[] dataRoot;
  return EXIT_SUCCESS;
}
