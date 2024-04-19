// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPassThrough.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPassThrough);

//------------------------------------------------------------------------------
vtkPassThrough::vtkPassThrough()
  : DeepCopyInput(0)
  , AllowNullInput(false)
{
}

//------------------------------------------------------------------------------
vtkPassThrough::~vtkPassThrough() = default;

//------------------------------------------------------------------------------
int vtkPassThrough::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inVec, vtkInformationVector* outVec)
{
  if (this->AllowNullInput && this->GetNumberOfInputPorts() != 0 &&
    inVec[0]->GetInformationObject(0) == nullptr)
  {
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkPolyData* obj = vtkPolyData::New();
      outVec->GetInformationObject(i)->Set(vtkDataObject::DATA_OBJECT(), obj);
      obj->FastDelete();
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inVec, outVec);
  }
}

//------------------------------------------------------------------------------
void vtkPassThrough::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DeepCopyInput: " << (this->DeepCopyInput ? "on" : "off") << endl
     << indent << "AllowNullInput: " << (this->AllowNullInput ? "on" : "off") << endl;
}

//------------------------------------------------------------------------------
int vtkPassThrough::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!inInfo)
  {
    return this->AllowNullInput ? 1 : 0;
  }

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (this->DeepCopyInput)
  {
    output->DeepCopy(input);
  }
  else
  {
    output->ShallowCopy(input);
  }

  this->CheckAbort();

  return 1;
}

//------------------------------------------------------------------------------
int vtkPassThrough::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}
VTK_ABI_NAMESPACE_END
