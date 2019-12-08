/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTecPlotReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkTecplotReader
// .SECTION Description
//

#include "vtkDebugLeaks.h"
#include "vtkTecplotReader.h"

#include "vtkActor.h"
#include "vtkArrayIterator.h"
#include "vtkArrayIteratorTemplate.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDirectory.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"
#include <string>
#include <vtksys/SystemTools.hxx>

using namespace std;

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
  static string ErrorMessage;
};

bool vtkErrorObserver::HasError = false;
string vtkErrorObserver::ErrorMessage = string();

int TestTecplotReader2(int argc, char* argv[])
{
  char* dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  const string tecplotDir = string(dataRoot) + "/Data/TecPlot/";

  if (argc < 2)
  {
    return EXIT_SUCCESS;
  }

  const char* filename = argv[1];

  vtkNew<vtkCallbackCommand> cmd;
  cmd->SetCallback(&(vtkErrorObserver::OnError));

  const string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
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
    cerr << "Failed to read data set from " << filename << endl;
    return EXIT_FAILURE;
  }
  if (vtkErrorObserver::HasError)
  {
    cerr << "Failed to read from " << filename << endl;
    if (!vtkErrorObserver::ErrorMessage.empty())
    {
      cerr << "Error message: " << vtkErrorObserver::ErrorMessage << endl;
    }
    return EXIT_FAILURE;
  }

  cout << filename << " was read without errors." << endl;
  delete[] dataRoot;
  return EXIT_SUCCESS;
}
