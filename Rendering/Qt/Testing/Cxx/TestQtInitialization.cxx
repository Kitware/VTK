/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtInitialization.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// Tests vtkQtInitialization.
// Thanks to Tim Shead from Sandia National Laboratories for writing this test.

#include "vtkQtInitialization.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#include <QCoreApplication>

int TestQtInitialization(int , char* [])
{
  int error_count = 0;

  // Because we share the same process with other tests, verify that
  // an instance of QCoreApplication hasn't already been created.  This
  // ensures that we don't introduce false-positives in case some other test
  // has an instance of QCoreApplication floating-around ...

  if(QCoreApplication::instance())
   {
   cerr << "Internal test error ... QCoreApplication already exists" << endl;
   ++error_count;
   }

  vtkSmartPointer<vtkQtInitialization> initialization = vtkSmartPointer<vtkQtInitialization>::New();

  if(!QCoreApplication::instance())
   {
   cerr << "QCoreApplication not initialized" << endl;
   ++error_count;
   }

  return error_count;
}
