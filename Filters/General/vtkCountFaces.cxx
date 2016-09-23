/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCountFaces.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCountFaces.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCountFaces)

//------------------------------------------------------------------------------
void vtkCountFaces::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputArrayName: "
     << (this->OutputArrayName ? this->OutputArrayName : "(NULL)") << "\n";
}

//------------------------------------------------------------------------------
vtkCountFaces::vtkCountFaces()
  : OutputArrayName(NULL)
{
  this->SetOutputArrayName("Face Count");
}

//------------------------------------------------------------------------------
vtkCountFaces::~vtkCountFaces()
{
  this->SetOutputArrayName(NULL);
}

//------------------------------------------------------------------------------
int vtkCountFaces::RequestData(vtkInformation *,
                               vtkInformationVector **inInfoVec,
                               vtkInformationVector *outInfoVec)
{
  // get the info objects
  vtkInformation *inInfo = inInfoVec[0]->GetInformationObject(0);
  vtkInformation *outInfo = outInfoVec->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));

  assert(input && output);

  output->ShallowCopy(input);

  vtkNew<vtkIdTypeArray> faceCount;
  faceCount->Allocate(input->GetNumberOfCells());
  faceCount->SetName(this->OutputArrayName);
  output->GetCellData()->AddArray(faceCount.Get());

  vtkCellIterator *it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    faceCount->InsertNextValue(it->GetNumberOfFaces());
  }
  it->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int vtkCountFaces::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCountFaces::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
