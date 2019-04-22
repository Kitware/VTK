/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridOutlineFilter.h"

#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkHyperTreeGridOutlineFilter);

//----------------------------------------------------------------------------
vtkHyperTreeGridOutlineFilter::vtkHyperTreeGridOutlineFilter()
{
  this->OutlineSource = vtkOutlineSource::New();

  this->GenerateFaces = 0;
}

//----------------------------------------------------------------------------
vtkHyperTreeGridOutlineFilter::~vtkHyperTreeGridOutlineFilter()
{
  if (this->OutlineSource != nullptr)
  {
    this->OutlineSource->Delete();
    this->OutlineSource = nullptr;
  }
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridOutlineFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkHyperTreeGrid* input =
    vtkHyperTreeGrid::SafeDownCast(inInfo->Get(vtkHyperTreeGrid::DATA_OBJECT()));
  if (!input)
  {
    vtkErrorMacro(
      "Incorrect type of input: " << inInfo->Get(vtkHyperTreeGrid::DATA_OBJECT())->GetClassName());
    return 0;
  }

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    vtkErrorMacro(
      "Incorrect type of output: " << outInfo->Get(vtkDataObject::DATA_OBJECT())->GetClassName());
    return 0;
  }

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
int vtkHyperTreeGridOutlineFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridOutlineFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
// JBL Pour moi, c'est un defaut de design de vtkHyperTreeGridAlgorithm
int vtkHyperTreeGridOutlineFilter::ProcessTrees(
  vtkHyperTreeGrid* vtkNotUsed(input), vtkDataObject* vtkNotUsed(outputDO))
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridOutlineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Faces: " << (this->GenerateFaces ? "On\n" : "Off\n");
}
