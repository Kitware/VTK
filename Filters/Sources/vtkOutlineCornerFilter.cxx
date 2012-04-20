/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineCornerFilter.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineCornerSource.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkOutlineCornerFilter);

vtkOutlineCornerFilter::vtkOutlineCornerFilter ()
{
  this->CornerFactor = 0.2;
  this->OutlineCornerSource = vtkOutlineCornerSource::New();
}

vtkOutlineCornerFilter::~vtkOutlineCornerFilter ()
{
  if (this->OutlineCornerSource != NULL)
    {
    this->OutlineCornerSource->Delete ();
    this->OutlineCornerSource = NULL;
    }
}

int vtkOutlineCornerFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineCornerSource do all the work
  //
  this->OutlineCornerSource->SetBounds(input->GetBounds());
  this->OutlineCornerSource->SetCornerFactor(this->GetCornerFactor());
  this->OutlineCornerSource->Update();

  output->CopyStructure(this->OutlineCornerSource->GetOutput());

  return 1;
}

int vtkOutlineCornerFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkOutlineCornerFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
}
