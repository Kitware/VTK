/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonAppInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* Minimal main program -- everything is loaded from the library */

#include "vtkPython.h"
#include "vtkPythonCompatibility.h"

#ifdef VTK_COMPILED_USING_MPI
# include <mpi.h>
# include "vtkMPIController.h"
#endif // VTK_COMPILED_USING_MPI

#include "vtkOutputWindow.h"
#include "vtkPythonInterpreter.h"
#include "vtkVersion.h"
#include "vtkpythonmodules.h"
#include <sys/stat.h>

#include <string>

#ifdef VTK_COMPILED_USING_MPI
class vtkMPICleanup {
public:
  vtkMPICleanup()
  {
      this->Controller = nullptr;
  }
  void Initialize(int* argc, char ***argv)
  {
      MPI_Init(argc, argv);
      this->Controller = vtkMPIController::New();
      this->Controller->Initialize(argc, argv, 1);
      vtkMultiProcessController::SetGlobalController(this->Controller);
  }
  void Cleanup()
  {
    if ( this->Controller )
    {
      this->Controller->Finalize();
      this->Controller->Delete();
      this->Controller = nullptr;
      vtkMultiProcessController::SetGlobalController(nullptr);
    }
  }
  ~vtkMPICleanup()
  {
    this->Cleanup();
  }

private:
  vtkMPIController *Controller;
};

static vtkMPICleanup VTKMPICleanup;
// AtExitCallback is needed to finalize the MPI controller if the python script
// calls sys.exit() directly.
static void AtExitCallback()
{
  VTKMPICleanup.Cleanup();
}
#endif // VTK_COMPILED_USING_MPI

int main(int argc, char **argv)
{
#ifdef VTK_COMPILED_USING_MPI
  VTKMPICleanup.Initialize(&argc, &argv);
  Py_AtExit(::AtExitCallback);
#endif // VTK_COMPILED_USING_MPI

  /**
   * This function is generated and exposed in vtkpythonmodules.h.
   * This registers any Python modules for VTK for static builds.
   */
  CMakeLoadAllPythonModules();

  // Setup the output window to be vtkOutputWindow, rather than platform
  // specific one. This avoids creating vtkWin32OutputWindow on Windows, for
  // example, which puts all Python errors in a window rather than the terminal
  // as one would expect.
  auto opwindow = vtkOutputWindow::New();;
  vtkOutputWindow::SetInstance(opwindow);
  opwindow->Delete();

  return vtkPythonInterpreter::PyMain(argc, argv);
}
