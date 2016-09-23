/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageDataToUniformGrid.h"

#include "vtkCellData.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkImageDataToUniformGrid);

//----------------------------------------------------------------------------
vtkImageDataToUniformGrid::vtkImageDataToUniformGrid()
{
  this->Reverse = 0;
}

//----------------------------------------------------------------------------
vtkImageDataToUniformGrid::~vtkImageDataToUniformGrid()
{
}

//----------------------------------------------------------------------------
void vtkImageDataToUniformGrid::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Reverse: " << this->Reverse << "\n";
}

//----------------------------------------------------------------------------
int vtkImageDataToUniformGrid::RequestDataObject(vtkInformation *,
                                                 vtkInformationVector **inV,
                                                 vtkInformationVector *outV)
{
  vtkInformation* inInfo = inV[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return VTK_ERROR;
  }

  vtkInformation* outInfo = outV->GetInformationObject(0);

  if(vtkDataObjectTree* input = vtkDataObjectTree::GetData(inInfo) )
  { // multiblock data sets
    vtkDataObjectTree* output = vtkDataObjectTree::GetData(outInfo);
    if (!output)
    {
      output = input->NewInstance();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return VTK_OK;
  }
  if(vtkImageData::GetData(inInfo) != NULL)
  {
    vtkUniformGrid* output = vtkUniformGrid::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();

    return VTK_OK;
  }

  vtkErrorMacro("Don't know how to handle input of type " <<
                vtkDataObject::GetData(inInfo)->GetClassName());
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
int vtkImageDataToUniformGrid::RequestData(vtkInformation *,
                                           vtkInformationVector **,
                                           vtkInformationVector *outV)
{
  vtkDataObject* input = this->GetInput();
  vtkInformation *outInfo = outV->GetInformationObject(0);
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation *inArrayInfo = this->GetInputArrayInformation(0);
  if (!inArrayInfo)
  {
    vtkErrorMacro("Problem getting array to process.");
    return 0;
  }
  int association = -1;
  if (!inArrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
  {
    vtkErrorMacro("Unable to query field association for the scalar.");
    return 0;
  }
  association = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  const char* arrayName = inArrayInfo->Get(vtkDataObject::FIELD_NAME());
  if (!arrayName)
  {
    vtkErrorMacro("Problem getting array name to process.");
    return 0;
  }

  if(vtkImageData* inImageData = vtkImageData::SafeDownCast(input))
  {
    return this->Process(inImageData, association, arrayName,
                         vtkUniformGrid::SafeDownCast(output));
  }
  vtkDataObjectTree* inMB = vtkDataObjectTree::SafeDownCast(input);
  vtkDataObjectTree* outMB = vtkDataObjectTree::SafeDownCast(output);
  outMB->CopyStructure(inMB);
  vtkDataObjectTreeIterator* iter = inMB->NewTreeIterator();
  iter->VisitOnlyLeavesOn();
  iter->TraverseSubTreeOn();
  for(iter->GoToFirstItem();!iter->IsDoneWithTraversal();iter->GoToNextItem())
  {
    if(vtkImageData* inImageData =
       vtkImageData::SafeDownCast(iter->GetCurrentDataObject()))
    {
      vtkNew<vtkUniformGrid> outUniformGrid;
      if(this->Process(inImageData, association, arrayName, outUniformGrid.GetPointer()) != VTK_OK)
      {
        iter->Delete();
        return VTK_ERROR;
      }
      outMB->SetDataSetFrom(iter, outUniformGrid.GetPointer());
    }
    else
    { // not a uniform grid so we just shallow copy from input to output
      outMB->SetDataSetFrom(iter, iter->GetCurrentDataObject());
    }
  }
  iter->Delete();

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkImageDataToUniformGrid::FillInputPortInformation(int port,
                                                        vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  // According to the documentation this is the way to append additional
  // input data set type since VTK 5.2.
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
    "vtkDataObjectTree");
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkImageDataToUniformGrid::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkImageDataToUniformGrid::Process(
  vtkImageData* input, int association,
  const char* arrayName, vtkUniformGrid* output)
{
  if(vtkUniformGrid* uniformGrid = vtkUniformGrid::SafeDownCast(input))
  {
    output->ShallowCopy(uniformGrid);
  }
  else
  {
    output->ShallowCopy(input);
  }

  vtkDataArray* inScalars = NULL;
  if(association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    inScalars = input->GetPointData()->GetArray(arrayName);
  }
  else if(association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    inScalars = input->GetCellData()->GetArray(arrayName);
  }
  else
  {
    vtkErrorMacro("Wrong assocation type: " << association);
    return VTK_ERROR;
  }

  if (!inScalars)
  {
    vtkErrorMacro("No scalar data to use for blanking.");
    return VTK_ERROR;
  }
  else if(inScalars->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Scalar data must be a single component array.");
    return VTK_ERROR;
  }

  vtkNew<vtkUnsignedCharArray> blankingArray;
  blankingArray->DeepCopy(inScalars);
  blankingArray->SetName(vtkDataSetAttributes::GhostArrayName());


  unsigned char value1;
  unsigned char value2;
  if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    if (this->Reverse)
    {
      value1 = 0;
      value2 = vtkDataSetAttributes::HIDDENCELL;
    }
    else
    {
      value1 = vtkDataSetAttributes::HIDDENCELL;
      value2 = 0;
    }
  }
  else
  {
    if (this->Reverse)
    {
      value1 = 0;
      value2 = vtkDataSetAttributes::HIDDENPOINT;
    }
    else
    {
      value1 = vtkDataSetAttributes::HIDDENPOINT;
      value2 = 0;
    }
  }
  for(vtkIdType i=0;i<blankingArray->GetNumberOfTuples();i++)
  {
    char value = blankingArray->GetValue(i) == 0 ? value1 : value2;
    blankingArray->SetValue(i, value);
  }

  if(association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    output->GetPointData()->AddArray(blankingArray.GetPointer());
  }
  else
  {
    output->GetCellData()->AddArray(blankingArray.GetPointer());
  }

  return VTK_OK;
}
