/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeSelectedLocations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeSelectedLocations.h"

#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkProbeFilter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkProbeSelectedLocations);
//----------------------------------------------------------------------------
vtkProbeSelectedLocations::vtkProbeSelectedLocations()
{
}

//----------------------------------------------------------------------------
vtkProbeSelectedLocations::~vtkProbeSelectedLocations()
{
}


//----------------------------------------------------------------------------
int vtkProbeSelectedLocations::RequestDataObject(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (this->PreserveTopology)
  {
    vtkWarningMacro("This filter does not support PreserveTopology.");
    this->PreserveTopology = 0;
  }
  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkProbeSelectedLocations::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector ** inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (!selInfo)
  {
    // When selection is not provided, quietly select nothing.
    return 1;
  }

  vtkSelection* selInput = vtkSelection::GetData(selInfo);
  vtkDataSet* dataInput = vtkDataSet::GetData(inInfo);
  vtkDataSet* output = vtkDataSet::GetData(outInfo);

  vtkSelectionNode* node = 0;
  if (selInput->GetNumberOfNodes() == 1)
  {
    node = selInput->GetNode(0);
  }
  if (!node)
  {
    vtkErrorMacro("Selection must have a single node.");
    return 0;
  }

  if (node->GetContentType() != vtkSelectionNode::LOCATIONS)
  {
    vtkErrorMacro("Missing or incompatible CONTENT_TYPE. "
      "vtkSelection::LOCATIONS required.");
    return 0;
  }


  // From the indicates locations in the selInput, create a unstructured grid to
  // probe with.
  vtkUnstructuredGrid* tempInput = vtkUnstructuredGrid::New();
  vtkPoints* points = vtkPoints::New();
  tempInput->SetPoints(points);
  points->Delete();

  vtkDataArray* dA = vtkArrayDownCast<vtkDataArray>(
    node->GetSelectionList());
  if (!dA)
  {
    // no locations to probe, quietly quit.
    return 1;
  }

  if (dA->GetNumberOfComponents() != 3)
  {
    vtkErrorMacro("SelectionList must be a 3 component list with point locations.");
    return 0;
  }

  vtkIdType numTuples = dA->GetNumberOfTuples();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numTuples);

  for (vtkIdType cc=0; cc < numTuples; cc++)
  {
    points->SetPoint(cc, dA->GetTuple(cc));
  }


  vtkDataSet* inputClone = dataInput->NewInstance();
  inputClone->ShallowCopy(dataInput);

  vtkProbeFilter* subFilter = vtkProbeFilter::New();
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(inputClone);
  subFilter->SetInputConnection(1, tp->GetOutputPort());
  inputClone->Delete();
  tp->Delete();

  tp = vtkTrivialProducer::New();
  tp->SetOutput(tempInput);
  subFilter->SetInputConnection(0, tp->GetOutputPort());
  tempInput->Delete();
  tp->Delete();
  tp = 0;

  vtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  //pass all required information to the helper filter
  int piece = 0;
  int npieces = 1;
  int *uExtent=0;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    uExtent = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  }

  subFilter->UpdatePiece(piece, npieces, 0, uExtent);
  output->ShallowCopy(subFilter->GetOutput());
  subFilter->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkProbeSelectedLocations::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

