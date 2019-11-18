/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkmPolyDataNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkmPolyDataNormals.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/PolyDataConverter.h"

#include "vtkmFilterPolicy.h"

#include "vtkm/filter/SurfaceNormals.h"

vtkStandardNewMacro(vtkmPolyDataNormals);

//------------------------------------------------------------------------------
void vtkmPolyDataNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkmPolyDataNormals::vtkmPolyDataNormals()
{
  // change defaults from parent
  this->Splitting = 0;
  this->Consistency = 0;
  this->FlipNormals = 0;
  this->ComputePointNormals = 1;
  this->ComputeCellNormals = 0;
  this->AutoOrientNormals = 0;
}

//------------------------------------------------------------------------------
vtkmPolyDataNormals::~vtkmPolyDataNormals() = default;

//------------------------------------------------------------------------------
int vtkmPolyDataNormals::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  try
  {
    // convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::None);

    vtkm::cont::DataSet result;

    // check for flags that vtkm filter cannot handle
    bool unsupported = this->Splitting != 0;
    if (!unsupported)
    {
      vtkmInputFilterPolicy policy;
      vtkm::filter::SurfaceNormals filter;
      filter.SetGenerateCellNormals((this->ComputeCellNormals != 0));
      filter.SetCellNormalsName("Normals");
      filter.SetGeneratePointNormals((this->ComputePointNormals != 0));
      filter.SetPointNormalsName("Normals");
      filter.SetAutoOrientNormals(this->AutoOrientNormals != 0);
      filter.SetFlipNormals(this->FlipNormals != 0);
      filter.SetConsistency(this->Consistency != 0);
      result = filter.Execute(in, policy);
    }
    else
    {
      vtkWarningMacro(<< "Unsupported options\n"
                      << "Falling back to vtkPolyDataNormals.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    if (!fromvtkm::Convert(result, output, input))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkWarningMacro(<< "VTK-m error: " << e.GetMessage() << "Falling back to vtkPolyDataNormals");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkSmartPointer<vtkDataArray> pointNormals = output->GetPointData()->GetArray("Normals");
  vtkSmartPointer<vtkDataArray> cellNormals = output->GetCellData()->GetArray("Normals");

  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->PassData(input->GetPointData());

  if (pointNormals)
  {
    output->GetPointData()->SetNormals(pointNormals);
  }
  if (cellNormals)
  {
    output->GetCellData()->SetNormals(cellNormals);
  }

  return 1;
}
