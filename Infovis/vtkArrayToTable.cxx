/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayToTable.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayData.h"
#include "vtkArrayToTable.h"
#include "vtkDenseArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkTable.h"

#include <vtksys/ios/sstream>

// ----------------------------------------------------------------------

vtkCxxRevisionMacro(vtkArrayToTable, "1.1");
vtkStandardNewMacro(vtkArrayToTable);

// ----------------------------------------------------------------------

vtkArrayToTable::vtkArrayToTable()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkArrayToTable::~vtkArrayToTable()
{
}

// ----------------------------------------------------------------------

void vtkArrayToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkArrayToTable::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------

int vtkArrayToTable::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  vtkTable* const output = vtkTable::GetData(outputVector);

  vtkTypedArray<double>* const array = vtkTypedArray<double>::SafeDownCast(input->GetArray());
  if(!array)
    {
    vtkErrorMacro(<< "vtkArrayToTable requires vtkTypedArray<double> as input");
    return 0;
    }

  if(array->GetExtents().GetDimensions() == 1)
    {
    const vtkArrayExtents extents = array->GetExtents();

    vtkDoubleArray* const column = vtkDoubleArray::New();
    column->SetNumberOfTuples(extents[0]);
    column->SetName("0");
    output->AddColumn(column);
    column->Delete();

    for(vtkIdType i = 0; i != extents[0]; ++i)
      {
      column->SetValue(i, array->GetValue(vtkArrayCoordinates(i)));
      }
    }
  else if(array->GetExtents().GetDimensions() == 2)
    {
    const vtkArrayExtents extents = array->GetExtents();

    for(vtkIdType j = 0; j != extents[1]; ++j)
      {
      vtkstd::ostringstream column_name;
      column_name << j;
        
      vtkDoubleArray* const column = vtkDoubleArray::New();
      column->SetNumberOfTuples(extents[0]);
      column->SetName(column_name.str().c_str());

      for(vtkIdType i = 0; i != extents[0]; ++i)
        {
        column->SetValue(i, array->GetValue(vtkArrayCoordinates(i, j)));
        }

      output->AddColumn(column);
      column->Delete();
      }
    }
  else
    {
    vtkErrorMacro(<< "vtkArrayToTable require an input array with 1 or 2 dimensions.");
    return 0;
    }

  return 1;
}

