/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataObjectToTable.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkDataObjectToTable, "1.1");
vtkStandardNewMacro(vtkDataObjectToTable);
//---------------------------------------------------------------------------
vtkDataObjectToTable::vtkDataObjectToTable()
{
  this->FieldType = POINT_DATA;
}

//---------------------------------------------------------------------------
vtkDataObjectToTable::~vtkDataObjectToTable()
{
}

//---------------------------------------------------------------------------
int vtkDataObjectToTable::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//---------------------------------------------------------------------------
int vtkDataObjectToTable::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get input data
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inputInfo->Get(vtkDataObject::DATA_OBJECT());

  // Get output table
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkFieldData* data = vtkFieldData::New();
  if (this->FieldType == FIELD_DATA)
    {
    data->ShallowCopy(input->GetFieldData());
    }
  else
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
    if (!ds)
      {
      vtkErrorMacro("Input must be vtkDataSet to extract point or cell data.");
      }
    if (this->FieldType == POINT_DATA)
      {
      data->ShallowCopy(ds->GetPointData());
      }
    else if (this->FieldType == CELL_DATA)
      {
      data->ShallowCopy(ds->GetCellData());
      }
    }
  output->SetFieldData(data);
  data->Delete();
  return 1;
}

//---------------------------------------------------------------------------
void vtkDataObjectToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
}
