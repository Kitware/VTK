/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfaceLIC.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestSurfaceLIC.h"

#include <vtksys/SystemTools.hxx>
//#define VTK_CREATE_NEW(var, class) vtkSmartPointer<class> var = vtkSmartPointer<class>::New();

int TestSurfaceLIC(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref_surface.vtp");
  std::string filename = fname;
  filename = "--data=" + filename;
  std::string vectors = "--vectors=V";
  delete []fname;

  char** new_argv = new char*[argc+2];
  for (int cc=0; cc < argc; cc++)
    {
    new_argv[cc] = vtksys::SystemTools::DuplicateString(argv[cc]);
    }
  new_argv[argc++] = vtksys::SystemTools::DuplicateString(filename.c_str());
  new_argv[argc++] = vtksys::SystemTools::DuplicateString(vectors.c_str());

  RenderingMode = SURFACE_LIC_TEST;
  int status = SurfaceLIC(argc, new_argv);

  for (int kk=0; kk < argc; kk++)
    {
    delete [] new_argv[kk];
    }
  delete [] new_argv;
  return status;
}
