/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToSparseArray.cxx

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

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLongLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTableToSparseArray.h"

#include <algorithm>

class vtkTableToSparseArray::implementation
{
public:
  std::vector<vtkStdString> Coordinates;
  vtkStdString Values;
  vtkArrayExtents OutputExtents;
  bool ExplicitOutputExtents;
};

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkTableToSparseArray);

// ----------------------------------------------------------------------

vtkTableToSparseArray::vtkTableToSparseArray() :
  Implementation(new implementation())
{
  this->Implementation->ExplicitOutputExtents = false;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkTableToSparseArray::~vtkTableToSparseArray()
{
  delete this->Implementation;
}

// ----------------------------------------------------------------------

void vtkTableToSparseArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for(size_t i = 0; i != this->Implementation->Coordinates.size(); ++i)
    os << indent << "CoordinateColumn: " << this->Implementation->Coordinates[i] << endl;
  os << indent << "ValueColumn: " << this->Implementation->Values << endl;
  os << indent << "OutputExtents: ";
  if(this->Implementation->ExplicitOutputExtents)
    os << this->Implementation->OutputExtents << endl;
  else
    os << "<none>" << endl;
}

void vtkTableToSparseArray::ClearCoordinateColumns()
{
  this->Implementation->Coordinates.clear();
  this->Modified();
}

void vtkTableToSparseArray::AddCoordinateColumn(const char* name)
{
  if(!name)
  {
    vtkErrorMacro(<< "cannot add coordinate column with NULL name");
    return;
  }

  this->Implementation->Coordinates.push_back(name);
  this->Modified();
}

void vtkTableToSparseArray::SetValueColumn(const char* name)
{
  if(!name)
  {
    vtkErrorMacro(<< "cannot set value column with NULL name");
    return;
  }

  this->Implementation->Values = name;
  this->Modified();
}

const char* vtkTableToSparseArray::GetValueColumn()
{
  return this->Implementation->Values.c_str();
}

void vtkTableToSparseArray::ClearOutputExtents()
{
  this->Implementation->ExplicitOutputExtents = false;
  this->Modified();
}

void vtkTableToSparseArray::SetOutputExtents(const vtkArrayExtents& extents)
{
  this->Implementation->ExplicitOutputExtents = true;
  this->Implementation->OutputExtents = extents;
  this->Modified();
}

int vtkTableToSparseArray::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
      return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------

int vtkTableToSparseArray::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkTable* const table = vtkTable::GetData(inputVector[0]);

  std::vector<vtkAbstractArray*> coordinates(this->Implementation->Coordinates.size());
  for(size_t i = 0; i != this->Implementation->Coordinates.size(); ++i)
  {
    coordinates[i] = table->GetColumnByName(this->Implementation->Coordinates[i].c_str());
    if(!coordinates[i])
    {
      vtkErrorMacro(<< "missing coordinate array: " << this->Implementation->Coordinates[i].c_str());
    }
  }
// See http://developers.sun.com/solaris/articles/cmp_stlport_libCstd.html
// Language Feature: Partial Specializations
// Workaround
  int n=0;
#ifdef _RWSTD_NO_CLASS_PARTIAL_SPEC
  std::count(coordinates.begin(), coordinates.end(), static_cast<vtkAbstractArray*>(0),n);
#else
  n=std::count(coordinates.begin(), coordinates.end(), static_cast<vtkAbstractArray*>(0));
#endif
  if(n!=0)
  {
    return 0;
  }

  vtkAbstractArray* const values = table->GetColumnByName(this->Implementation->Values.c_str());
  if(!values)
  {
    vtkErrorMacro(<< "missing value array: " << this->Implementation->Values.c_str());
    return 0;
  }

  vtkSparseArray<double>* const array = vtkSparseArray<double>::New();
  array->Resize(vtkArrayExtents::Uniform(coordinates.size(), 0));

  for(size_t i = 0; i != coordinates.size(); ++i)
    array->SetDimensionLabel(i, coordinates[i]->GetName());

  vtkArrayCoordinates output_coordinates;
  output_coordinates.SetDimensions(coordinates.size());
  for(vtkIdType i = 0; i != table->GetNumberOfRows(); ++i)
  {
    for(size_t j = 0; j != coordinates.size(); ++j)
    {
      output_coordinates[j] = coordinates[j]->GetVariantValue(i).ToInt();
    }
    array->AddValue(output_coordinates, values->GetVariantValue(i).ToDouble());
  }

  if(this->Implementation->ExplicitOutputExtents)
  {
    array->SetExtents(this->Implementation->OutputExtents);
  }
  else
  {
    array->SetExtentsFromContents();
  }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}

