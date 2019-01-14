/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractSelectedArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtractSelectedArraysOverTime.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPExtractDataArraysOverTime.h"

vtkStandardNewMacro(vtkPExtractSelectedArraysOverTime);
//----------------------------------------------------------------------------
vtkPExtractSelectedArraysOverTime::vtkPExtractSelectedArraysOverTime()
{
  this->ArraysExtractor = vtkSmartPointer<vtkPExtractDataArraysOverTime>::New();
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPExtractSelectedArraysOverTime::~vtkPExtractSelectedArraysOverTime()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
void vtkPExtractSelectedArraysOverTime::SetController(vtkMultiProcessController* controller)
{
  auto extractor = vtkPExtractDataArraysOverTime::SafeDownCast(this->ArraysExtractor);
  if (extractor && extractor->GetController() != controller)
  {
    extractor->SetController(controller);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPExtractSelectedArraysOverTime::GetController()
{
  auto extractor = vtkPExtractDataArraysOverTime::SafeDownCast(this->ArraysExtractor);
  return (extractor ? extractor->GetController() : nullptr);
}

//----------------------------------------------------------------------------
void vtkPExtractSelectedArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->GetController() << endl;
}
