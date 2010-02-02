/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitColumnComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSplitColumnComponents.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkTable.h"

#include "vtkStdString.h"
#include "vtksys/ios/sstream"

vtkCxxRevisionMacro(vtkSplitColumnComponents, "1.1");
vtkStandardNewMacro(vtkSplitColumnComponents);
//---------------------------------------------------------------------------
vtkSplitColumnComponents::vtkSplitColumnComponents()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//---------------------------------------------------------------------------
vtkSplitColumnComponents::~vtkSplitColumnComponents()
{
}

//---------------------------------------------------------------------------
int vtkSplitColumnComponents::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get input tables
  vtkInformation* table1Info = inputVector[0]->GetInformationObject(0);
  vtkTable* table = vtkTable::SafeDownCast(
    table1Info->Get(vtkDataObject::DATA_OBJECT()));

  // Get output table
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Add columns from table, split multiple component columns as necessary
  for (int i = 0; i < table->GetNumberOfColumns(); ++i)
    {
    vtkAbstractArray* col = table->GetColumn(i);
    char* name = col->GetName();
    int components = col->GetNumberOfComponents();

    if (components == 1)
      {
      vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
      newCol->DeepCopy(col);
      newCol->SetName(name);
      output->AddColumn(newCol);
      newCol->Delete();
      }
    else if (components > 1)
      {
      // Split the multicomponent column up into individual columns
      int colSize = col->GetNumberOfTuples();
      for (int j = 0; j < components; ++j)
        {
        vtksys_ios::ostringstream ostr;
        ostr << name << " (" << j << ")";
        vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
        newCol->SetNumberOfTuples(col->GetNumberOfTuples());
        newCol->SetName(ostr.str().c_str());
        newCol->SetNumberOfTuples(colSize);
        // Now copy the components into their new columns
        for (vtkIdType row = 0; row < colSize; ++row)
          {
          newCol->InsertVariantValue(row,
                                     col->GetVariantValue(components*row + j));
          }

        output->AddColumn(newCol);
        newCol->Delete();
        }
      }
    }

  // Clean up pipeline information
  int piece = -1;
  int npieces = -1;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  output->GetInformation()->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), npieces);
  output->GetInformation()->Set(vtkDataObject::DATA_PIECE_NUMBER(), piece);

  return 1;
}

//---------------------------------------------------------------------------
void vtkSplitColumnComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
