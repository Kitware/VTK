/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFlashReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRFlashReader.h"
#include "vtkSetGet.h"

int TestFlashReader( int vtkNotUsed(argc), char *vtkNotUsed(argv)[] )
{
  int rc = 0;

  vtkAMRFlashReader *flashReader = vtkAMRFlashReader::New();

  flashReader->Delete();
  return( rc );
}
