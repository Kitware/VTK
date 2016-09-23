/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThrough.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPassThrough.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPassThrough);

//----------------------------------------------------------------------------
vtkPassThrough::vtkPassThrough()
{
  this->DeepCopyInput = 0;
}

//----------------------------------------------------------------------------
vtkPassThrough::~vtkPassThrough()
{
}

//----------------------------------------------------------------------------
void vtkPassThrough::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DeepCopyInput: "
     << (this->DeepCopyInput ? "on" : "off") << endl;
}

//----------------------------------------------------------------------------
int vtkPassThrough::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(this->DeepCopyInput)
  {
    output->DeepCopy(input);
  }
  else
  {
    output->ShallowCopy(input);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPassThrough::FillInputPortInformation(int port, vtkInformation* info)
{
    if (port == 0)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        return 1;
    }
    return 0;
}
