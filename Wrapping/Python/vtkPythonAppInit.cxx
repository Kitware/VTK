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

#ifdef VTK_COMPILED_USING_MPI
# include <mpi.h>
# include "vtkMPIController.h"
#endif // VTK_COMPILED_USING_MPI

#include "vtkVersion.h"
#include "Wrapping/Python/vtkPythonAppInitConfigure.h"

#if defined(CMAKE_INTDIR)
# define VTK_PYTHON_LIBRARY_DIR VTK_PYTHON_LIBRARY_DIR_BUILD "/" CMAKE_INTDIR
#else
# define VTK_PYTHON_LIBRARY_DIR VTK_PYTHON_LIBRARY_DIR_BUILD
#endif

#include <sys/stat.h>

/*
 * Make sure all the kits register their classes with vtkInstantiator.
 */
#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#ifdef VTK_USE_RENDERING
#include "vtkRenderingInstantiator.h"
#endif

#ifdef VTK_USE_VOLUMERENDERING
#include "vtkVolumeRenderingInstantiator.h"
#endif

#ifdef VTK_USE_HYBRID
#include "vtkHybridInstantiator.h"
#endif

#ifdef VTK_USE_PARALLEL
#include "vtkParallelInstantiator.h"
#endif

#ifdef VTK_COMPILED_USING_MPI
class vtkMPICleanup {
public:
  vtkMPICleanup()
    {
      this->Controller = 0;
    }
  void Initialize(int* argc, char ***argv)
    {
      MPI_Init(argc, argv);
      this->Controller = vtkMPIController::New();
      this->Controller->Initialize(argc, argv, 1);
      vtkMultiProcessController::SetGlobalController(this->Controller);      
    }
  ~vtkMPICleanup()
    {
      if ( this->Controller )
        {
        this->Controller->Finalize();
        this->Controller->Delete();
        }
    }
private:
  vtkMPIController *Controller;
};

static vtkMPICleanup VTKMPICleanup;

#endif // VTK_COMPILED_USING_MPI

extern "C" {
  extern DL_IMPORT(int) Py_Main(int, char **);
}

static void vtkPythonAppInitEnableMSVCDebugHook();
static int vtkPythonAppInitFileExists(const char* filename);

int main(int argc, char **argv)
{
  vtkPythonAppInitEnableMSVCDebugHook();
  
#ifdef VTK_COMPILED_USING_MPI
  VTKMPICleanup.Initialize(&argc, &argv);
#endif // VTK_COMPILED_USING_MPI

  int displayVersion = 0;
  if ( argc > 1 )
    {
    int cc;
    for ( cc = 1; cc < argc; cc ++ )
      {
      if ( strcmp(argv[cc], "-V") == 0 )
        {
        displayVersion = 1;
        break;
        }
      }
    }
  else
    {
    displayVersion = 1;
    }
  if ( displayVersion )
    {
    cout << vtkVersion::GetVTKSourceVersion() << endl;
    }

  // The following code will hack in the path for running VTK/Python
  // from the build tree. Do not try this at home. We are
  // professionals.

  // Set the program name, so that we can ask python to provide us
  // full path.
  Py_SetProgramName(argv[0]);

  // Initialize interpreter.
  Py_Initialize();

  // If the location of the library path and wrapping path exist, add
  // them to the list.
  
  // Get the pointer to path list object, append both paths, and
  // make sure to decrease reference counting for both path strings.
  char tmpPath[5];
  sprintf(tmpPath,"path");
  PyObject* path = PySys_GetObject(tmpPath);
  PyObject* newpath;
  if ( ::vtkPythonAppInitFileExists(VTK_PYTHON_LIBRARY_DIR) )
    {
    newpath = PyString_FromString(VTK_PYTHON_LIBRARY_DIR);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }
  if ( ::vtkPythonAppInitFileExists(VTK_PYTHON_PACKAGE_DIR) )
    {
    newpath = PyString_FromString(VTK_PYTHON_PACKAGE_DIR);
    PyList_Insert(path, 0, newpath);
    Py_DECREF(newpath);
    }

  // Ok, all done, now enter python main.
  return Py_Main(argc, argv);
}

int vtkPythonAppInitFileExists(const char* filename)
{
  // Return true if the file exists.
  struct stat fs;
  if (stat(filename, &fs) != 0) 
    {
    return 0;
    }
  return 1;
}

// For a DEBUG build on MSVC, add a hook to prevent error dialogs when
// being run from DART.
#if defined(_MSC_VER) && defined(_DEBUG)
# include <crtdbg.h>
static int vtkPythonAppInitDebugReport(int, char* message, int*)
{
  fprintf(stderr, message);
  exit(1);
  return 0;
}
void vtkPythonAppInitEnableMSVCDebugHook()
{
  if(getenv("DART_TEST_FROM_DART"))
    {
    _CrtSetReportHook(vtkPythonAppInitDebugReport);
    }
}
#else
void vtkPythonAppInitEnableMSVCDebugHook()
{
}
#endif
