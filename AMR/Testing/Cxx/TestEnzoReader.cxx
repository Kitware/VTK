/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEnzoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMREnzoReader.h"
#include "vtkSetGet.h"

int TestEnzoReader( int vtkNotUsed(argc), char *vtkNotUsed(argv)[] )
{
  int rc = 0;

  vtkAMREnzoReader *myEnzoReader = vtkAMREnzoReader::New();

  myEnzoReader->Delete();
  return( rc );
}
