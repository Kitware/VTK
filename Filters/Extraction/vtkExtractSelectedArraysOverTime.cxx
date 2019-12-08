/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedArraysOverTime.h"

#include "vtkDataSetAttributes.h"
#include "vtkExtractDataArraysOverTime.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>

//****************************************************************************
vtkStandardNewMacro(vtkExtractSelectedArraysOverTime);
//----------------------------------------------------------------------------
vtkExtractSelectedArraysOverTime::vtkExtractSelectedArraysOverTime()
  : NumberOfTimeSteps(0)
  , FieldType(vtkSelectionNode::CELL)
  , ContentType(-1)
  , ReportStatisticsOnly(false)
  , Error(vtkExtractSelectedArraysOverTime::NoError)
  , SelectionExtractor(nullptr)
  , IsExecuting(false)
{
  this->SetNumberOfInputPorts(2);
  this->ArraysExtractor = vtkSmartPointer<vtkExtractDataArraysOverTime>::New();
  this->SelectionExtractor = vtkSmartPointer<vtkExtractSelection>::New();
}

//----------------------------------------------------------------------------
vtkExtractSelectedArraysOverTime::~vtkExtractSelectedArraysOverTime()
{
  this->SetSelectionExtractor(nullptr);
}

//----------------------------------------------------------------------------
void vtkExtractSelectedArraysOverTime::SetSelectionExtractor(vtkExtractSelection* extractor)
{
  if (this->SelectionExtractor != extractor)
  {
    this->SelectionExtractor = extractor;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkExtractSelection* vtkExtractSelectedArraysOverTime::GetSelectionExtractor()
{
  return this->SelectionExtractor;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "SelectionExtractor: " << this->SelectionExtractor.Get() << endl;
  os << indent << "ReportStatisticsOnly: " << (this->ReportStatisticsOnly ? "ON" : "OFF") << endl;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedArraysOverTime::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    // We can handle composite datasets.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedArraysOverTime::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  assert(this->ArraysExtractor != nullptr);
  return this->ArraysExtractor->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedArraysOverTime::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  assert(this->ArraysExtractor != nullptr);
  return this->ArraysExtractor->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedArraysOverTime::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->ArraysExtractor->GetNumberOfTimeSteps() <= 0)
  {
    vtkErrorMacro("No time steps in input data!");
    return 0;
  }

  // get the output data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // is this the first request
  if (!this->IsExecuting)
  {
    vtkSelection* selection = vtkSelection::GetData(inputVector[1], 0);
    if (!selection)
    {
      return 1;
    }

    if (!this->DetermineSelectionType(selection))
    {
      return 0;
    }

    bool reportStats = this->ReportStatisticsOnly;
    switch (this->ContentType)
    {
      // for the following types were the number of elements selected may
      // change over time, we can only track summaries.
      case vtkSelectionNode::QUERY:
        reportStats = true;
        break;
      default:
        break;
    }

    this->ArraysExtractor->SetReportStatisticsOnly(reportStats);
    const int association = vtkSelectionNode::ConvertSelectionFieldToAttributeType(this->FieldType);
    this->ArraysExtractor->SetFieldAssociation(association);
    switch (association)
    {
      case vtkDataObject::POINT:
        this->ArraysExtractor->SetInputArrayToProcess(0, 0, 0, association, "vtkOriginalPointIds");
        break;
      case vtkDataObject::CELL:
        this->ArraysExtractor->SetInputArrayToProcess(0, 0, 0, association, "vtkOriginalCellIds");
        break;
      case vtkDataObject::ROW:
        this->ArraysExtractor->SetInputArrayToProcess(0, 0, 0, association, "vtkOriginalRowIds");
        break;
      default:
        this->ArraysExtractor->SetInputArrayToProcess(
          0, 0, 0, association, vtkDataSetAttributes::GLOBALIDS);
        break;
    }
    this->IsExecuting = true;
  }

  auto extractedData = this->Extract(inputVector, outInfo);

  vtkSmartPointer<vtkDataObject> oldData = vtkDataObject::GetData(inputVector[0], 0);
  inputVector[0]->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), extractedData);
  const int status = this->ArraysExtractor->ProcessRequest(request, inputVector, outputVector);
  inputVector[0]->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), oldData);

  if (!status)
  {
    this->IsExecuting = false;
    return 0;
  }

  if (this->IsExecuting &&
    (!request->Has(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()) ||
      request->Get(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()) == 0))
  {
    this->PostExecute(request, inputVector, outputVector);
    this->IsExecuting = false;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedArraysOverTime::PostExecute(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // nothing to do.
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkExtractSelectedArraysOverTime::Extract(
  vtkInformationVector** inputVector, vtkInformation* outInfo)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* selInput = vtkSelection::GetData(inputVector[1], 0);
  vtkSmartPointer<vtkExtractSelection> filter = this->SelectionExtractor;
  if (filter == nullptr)
  {
    return input;
  }
  filter->SetPreserveTopology(0);
  filter->SetInputData(0, input);
  filter->SetInputData(1, selInput);

  vtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  // pass all required information to the helper filter
  int piece = 0;
  int npieces = 1;
  int* uExtent = nullptr;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    uExtent = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  }
  filter->UpdatePiece(piece, npieces, 0, uExtent);

  vtkSmartPointer<vtkDataObject> extractedData;
  extractedData.TakeReference(filter->GetOutputDataObject(0)->NewInstance());
  extractedData->ShallowCopy(filter->GetOutputDataObject(0));

  double dtime = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  extractedData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), dtime);
  return extractedData;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedArraysOverTime::DetermineSelectionType(vtkSelection* sel)
{
  int contentType = -1;
  int fieldType = -1;
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node)
    {
      int nodeFieldType = node->GetFieldType();
      int nodeContentType = node->GetContentType();
      if ((fieldType != -1 && fieldType != nodeFieldType) ||
        (contentType != -1 && contentType != nodeContentType))
      {
        vtkErrorMacro("All vtkSelectionNode instances within a vtkSelection"
                      " must have the same ContentType and FieldType.");
        return 0;
      }
      fieldType = nodeFieldType;
      contentType = nodeContentType;
    }
  }
  this->ContentType = contentType;
  this->FieldType = fieldType;
  switch (this->ContentType)
  {
    case vtkSelectionNode::BLOCKS:
      // if selection blocks, assume we're extract cells.
      this->FieldType = vtkSelectionNode::CELL;
      break;
    default:
      break;
  }
  return 1;
}
