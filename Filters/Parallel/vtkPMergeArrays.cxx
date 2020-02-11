/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMergeArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMergeArrays.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPMergeArrays);

//----------------------------------------------------------------------------
vtkPMergeArrays::vtkPMergeArrays() {}

//----------------------------------------------------------------------------
void vtkPMergeArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPMergeArrays::MergeDataObjectFields(vtkDataObject* input, int idx, vtkDataObject* output)
{
  int checks[vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
  {
    checks[attr] = output->GetNumberOfElements(attr) == input->GetNumberOfElements(attr) ? 0 : 1;
  }
  int globalChecks[vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller == nullptr)
  {
    for (int i = 0; i < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++i)
    {
      globalChecks[i] = checks[i];
    }
  }
  else
  {
    controller->AllReduce(
      checks, globalChecks, vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES, vtkCommunicator::MAX_OP);
  }

  for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
  {
    if (globalChecks[attr] == 0)
    {
      // only merge arrays when the number of elements in the input and output are the same
      this->MergeArrays(
        idx, input->GetAttributesAsFieldData(attr), output->GetAttributesAsFieldData(attr));
    }
  }

  return 1;
}
