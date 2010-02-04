/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataLIC2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "TestImageDataLIC2D.h"
#include <vtksys/SystemTools.hxx>

int TestImageDataLIC2D(int argc, char* argv[])
{
  char* fname =  
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SurfaceVectors.vtk");
  vtkstd::string filename = fname;
  filename = "--data=" + filename;

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/noise.png");
  vtkstd::string noise = fname;
  noise = "--noise=" + noise;
  
  char** new_argv = new char*[argc+10];
  for (int cc=0; cc < argc; cc++)
    {
    new_argv[cc] = vtksys::SystemTools::DuplicateString(argv[cc]);
    }
  new_argv[argc++] = vtksys::SystemTools::DuplicateString(filename.c_str());
  new_argv[argc++] = vtksys::SystemTools::DuplicateString(noise.c_str());
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--mag=5");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--partitions=5");
  int status = ImageDataLIC2D(argc, new_argv);
  for (int kk=0; kk < argc; kk++)
    {
    delete [] new_argv[kk];
    }
  delete [] new_argv;
  return status;
}
