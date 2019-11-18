/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositeDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <iostream>
#include <vector>

//------------------------------------------------------------------------------
int TestCompositeDataSets(int, char*[])
{
  int errors = 0;

  return (errors);
}
