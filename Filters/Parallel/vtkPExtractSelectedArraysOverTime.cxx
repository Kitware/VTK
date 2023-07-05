// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPExtractSelectedArraysOverTime.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPExtractDataArraysOverTime.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPExtractSelectedArraysOverTime);
//------------------------------------------------------------------------------
vtkPExtractSelectedArraysOverTime::vtkPExtractSelectedArraysOverTime()
{
  this->ArraysExtractor = vtkSmartPointer<vtkPExtractDataArraysOverTime>::New();
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPExtractSelectedArraysOverTime::~vtkPExtractSelectedArraysOverTime()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPExtractSelectedArraysOverTime::SetController(vtkMultiProcessController* controller)
{
  auto extractor = vtkPExtractDataArraysOverTime::SafeDownCast(this->ArraysExtractor);
  if (extractor && extractor->GetController() != controller)
  {
    extractor->SetController(controller);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkPExtractSelectedArraysOverTime::GetController()
{
  auto extractor = vtkPExtractDataArraysOverTime::SafeDownCast(this->ArraysExtractor);
  return (extractor ? extractor->GetController() : nullptr);
}

//------------------------------------------------------------------------------
void vtkPExtractSelectedArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->GetController() << endl;
}
VTK_ABI_NAMESPACE_END
