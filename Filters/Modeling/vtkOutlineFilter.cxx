/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineFilter.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkOutlineFilter);

//----------------------------------------------------------------------------
vtkOutlineFilter::vtkOutlineFilter ()
{
  this->OutlineSource = vtkOutlineSource::New();

  this->GenerateFaces = 0;
}

//----------------------------------------------------------------------------
vtkOutlineFilter::~vtkOutlineFilter ()
{
  if (this->OutlineSource != NULL)
  {
    this->OutlineSource->Delete ();
    this->OutlineSource = NULL;
  }
}

//----------------------------------------------------------------------------
int vtkOutlineFilter::RequestData(
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
  // Let OutlineSource do all the work
  //

  this->OutlineSource->SetBounds(input->GetBounds());
  this->OutlineSource->SetGenerateFaces(this->GenerateFaces);
  this->OutlineSource->Update();

  output->CopyStructure(this->OutlineSource->GetOutput());

  return 1;
}

//----------------------------------------------------------------------------
int vtkOutlineFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Generate Faces: "
     << (this->GenerateFaces ? "On\n" : "Off\n");
}

