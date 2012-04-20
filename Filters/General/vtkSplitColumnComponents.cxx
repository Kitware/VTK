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
#include "vtkAbstractArray.h"
#include "vtkTable.h"

#include "vtksys/ios/sstream"
#include "math.h"

vtkStandardNewMacro(vtkSplitColumnComponents);
//---------------------------------------------------------------------------
vtkSplitColumnComponents::vtkSplitColumnComponents()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->CalculateMagnitudes = true;
}

//---------------------------------------------------------------------------
vtkSplitColumnComponents::~vtkSplitColumnComponents()
{
}

//---------------------------------------------------------------------------
// Templated function in an anonymous namespace to copy the data from the
// specified component in one column to a single component column
namespace {

template<typename T>
void CopyArrayData(T* source, T* destination, int components, int c,
                   unsigned int length)
{
  for (unsigned int i = 0; i < length; ++i)
    {
    destination[i] = source[i*components + c];
    }
}

template<typename T>
void CalculateMagnitude(T* source, T* destination, int components,
                        unsigned int length)
{
  for (unsigned int i = 0; i < length; ++i)
    {
    double tmp = 0.0;
    for (int j = 0; j < components; ++j)
      {
      tmp += static_cast<double>(source[i*components + j]
                                 * source[i*components + j]);
      }
    destination[i] = static_cast<T>(sqrt(tmp));
    }
}

} // End of anonymous namespace

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
      output->AddColumn(col);
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
        newCol->SetName(ostr.str().c_str());
        newCol->SetNumberOfTuples(colSize);
        // Now copy the components into their new columns
        switch(col->GetDataType())
          {
          vtkExtraExtendedTemplateMacro(
              CopyArrayData(static_cast<VTK_TT*>(col->GetVoidPointer(0)),
                            static_cast<VTK_TT*>(newCol->GetVoidPointer(0)),
                            components, j, colSize));
          }

        output->AddColumn(newCol);
        newCol->Delete();
        }
      // Add a magnitude column and calculate values if requested
      if (this->CalculateMagnitudes && col->IsA("vtkDataArray"))
        {
        vtksys_ios::ostringstream ostr;
        ostr << name << " (Magnitude)";
        vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
        newCol->SetName(ostr.str().c_str());
        newCol->SetNumberOfTuples(colSize);
        // Now calculate the magnitude column
        switch(col->GetDataType())
          {
          vtkTemplateMacro(
              CalculateMagnitude(static_cast<VTK_TT*>(col->GetVoidPointer(0)),
                                 static_cast<VTK_TT*>(newCol->GetVoidPointer(0)),
                                 components, colSize));
          }

        output->AddColumn(newCol);
        newCol->Delete();
        }
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSplitColumnComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CalculateMagnitudes: " << this->CalculateMagnitudes << endl;
}
