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

#include "vtkTecplotReader.h"
#include "vtkDebugLeaks.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkDirectory.h"
#include "vtkStringArray.h"
#include "vtkArrayIterator.h"
#include "vtkArrayIteratorTemplate.h"
#include <vtksys/SystemTools.hxx>
#include "vtkCallbackCommand.h"
#include <string>

using namespace std;

class vtkErrorObserver
{

public:
  static void Reset()
  {
    HasError = false;
    ErrorMessage.clear();
  }

  static void OnError(vtkObject* vtkNotUsed(caller), unsigned long int vtkNotUsed(eventId), void* vtkNotUsed(clientData), void* callData)
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

int TestTecplotReader2( int argc, char *argv[] )
{
  char* dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  string tecplotDir = string(dataRoot) + "/Data/TecPlot/";

  vtkNew<vtkDirectory> dir;
  if (1 != dir->Open(tecplotDir.c_str()))
  {
    cerr << "Unable to list files in " << tecplotDir << endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkCallbackCommand> cmd;
  cmd->SetCallback(&(vtkErrorObserver::OnError));

  int nErrors(0);
  for (int i = 0; i < dir->GetNumberOfFiles(); ++i)
  {
    string filename = tecplotDir;
    filename += dir->GetFile(i);
    string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);
    if (ext != ".dat")
    {
      continue;
    }

    vtkErrorObserver::Reset();
    vtkNew<vtkTecplotReader> r;
    r->AddObserver("ErrorEvent", cmd);
    r->SetFileName(filename.c_str());
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
      nErrors++;
      cerr << "Failed to read from " << filename << endl;
      if (!vtkErrorObserver::ErrorMessage.empty())
      {
        cerr << "Error message: " << vtkErrorObserver::ErrorMessage << endl;
      }
    }
  }

  if (nErrors > 0)
  {
    cerr << nErrors << "/" << dir->GetNumberOfFiles() << " files could not be loaded" << endl;
  }
  else
  {
    cout << "All files were loaded without errors." << endl;
  }

  delete[] dataRoot;
  return nErrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
