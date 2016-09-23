/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelection.h"

#include "vtkDataSet.h"
#include "vtkExtractSelectedBlock.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkExtractSelectedIds.h"
#include "vtkExtractSelectedLocations.h"
#include "vtkExtractSelectedRows.h"
#include "vtkExtractSelectedThresholds.h"
#include "vtkGraph.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkProbeSelectedLocations.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTrivialProducer.h"

vtkStandardNewMacro(vtkExtractSelection);

//----------------------------------------------------------------------------
vtkExtractSelection::vtkExtractSelection()
{
  this->BlockFilter = vtkExtractSelectedBlock::New();
  this->IdsFilter = vtkExtractSelectedIds::New();
  this->FrustumFilter = vtkExtractSelectedFrustum::New();
  this->LocationsFilter = vtkExtractSelectedLocations::New();
  this->ThresholdsFilter = vtkExtractSelectedThresholds::New();
  this->ProbeFilter = vtkProbeSelectedLocations::New();
  this->RowsFilter = vtkExtractSelectedRows::New();
  this->RowsFilter->AddOriginalRowIdsArrayOn();
  this->ShowBounds = 0;
  this->UseProbeForLocations = 0;
}

//----------------------------------------------------------------------------
vtkExtractSelection::~vtkExtractSelection()
{
  this->BlockFilter->Delete();
  this->IdsFilter->Delete();
  this->FrustumFilter->Delete();
  this->LocationsFilter->Delete();
  this->ThresholdsFilter->Delete();
  this->ProbeFilter->Delete();
  this->RowsFilter->Delete();
}

//----------------------------------------------------------------------------
int vtkExtractSelection::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
  {
    // Can work with composite datasets.
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
int vtkExtractSelection::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // verify the input, selection and output
  vtkDataObject *input = vtkDataObject::GetData(inInfo);
  if ( ! input )
  {
    vtkErrorMacro(<<"No input specified");
    return 0;
  }

  // If the input is a graph, don't try to handle it
  if ( vtkGraph::SafeDownCast(input) )
  {
    return 1;
  }

  if ( ! selInfo )
  {
    //When not given a selection, quietly select nothing.
    return 1;
  }

  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataObject *output = vtkDataObject::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int selType = -1;
  if (sel->GetNumberOfNodes() > 0)
  {
    selType = sel->GetNode(0)->GetContentType();
  }
  // if selType == vtkSelectionNode::BLOCKS, we don;t need to iterate over nodes in
  // the composite dataset, the extract block filter can handle composite datasets.
  if (input->IsA("vtkCompositeDataSet") && selType != vtkSelectionNode::BLOCKS)
  {
    vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input);
    vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::SafeDownCast(output);
    cdOutput->CopyStructure(cdInput);

    vtkCompositeDataIterator* iter = cdInput->NewIterator();
    vtkHierarchicalBoxDataIterator* hbIter =
      vtkHierarchicalBoxDataIterator::SafeDownCast(iter);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
      iter->GoToNextItem())
    {
      vtkDataObject* subOutput = 0;
      if (hbIter)
      {
        subOutput = this->RequestDataInternal(
          iter->GetCurrentFlatIndex(),
          hbIter->GetCurrentLevel(),
          hbIter->GetCurrentIndex(),
          iter->GetCurrentDataObject(),
          sel, outInfo);
      }
      else
      {
        subOutput = this->RequestDataInternal(iter->GetCurrentFlatIndex(),
          iter->GetCurrentDataObject(), sel, outInfo);
      }

      if (subOutput)
      {
        vtkDataSet* subDS = vtkDataSet::SafeDownCast(subOutput);
        vtkTable* subTable = vtkTable::SafeDownCast(subOutput);
        // purge empty datasets/tables from the output.
        if ((subDS  && subDS->GetNumberOfPoints() > 0) ||
          (subTable && subTable->GetNumberOfRows() > 0) ||
          (!subDS && !subTable))
        {
          cdOutput->SetDataSet(iter, subOutput);
        }
        subOutput->Delete();
      }
    }
    iter->Delete();
  }
  else
  {
    vtkSelectionNode* node = 0;
    if (sel->GetNumberOfNodes() > 0)
    {
      node = sel->GetNode(0);
    }
    if (!node)
    {
      vtkErrorMacro("Selection must have a single node.");
      return 0;
    }
    vtkDataObject* ecOutput =
      this->RequestDataFromBlock(input, node, outInfo);
    if (ecOutput)
    {
      output->ShallowCopy(ecOutput);
      ecOutput->Delete();
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractSelection::RequestDataObject(
  vtkInformation* req,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkCompositeDataSet *input = vtkCompositeDataSet::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
  {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
    return 1;
  }

  return this->Superclass::RequestDataObject(req, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkExtractSelection::RequestDataInternal(
  unsigned int composite_index,
  vtkDataObject* non_composite_input, vtkSelection* sel,
  vtkInformation* outInfo)
{
  if (non_composite_input->IsA("vtkCompositeDataSet"))
  {
    vtkErrorMacro(
      "RequestDataInternal cannot be called with a composite data input.");
    return NULL;
  }

  if (!non_composite_input || !sel)
  {
    return NULL;
  }

  for (unsigned int n = 0; n < sel->GetNumberOfNodes(); ++n)
  {
    vtkSelectionNode* node = sel->GetNode(n);
    vtkInformation* nodeProperties = node->GetProperties();
    if (nodeProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
      unsigned int cur_index = static_cast<unsigned int>(
          nodeProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()));
      if (cur_index == composite_index)
      {
        return this->RequestDataFromBlock(non_composite_input, node, outInfo);
      }
    }
    else
    {
      return this->RequestDataFromBlock(non_composite_input, node, outInfo);
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkExtractSelection::RequestDataInternal(
  unsigned int composite_index,
  unsigned int level,
  unsigned int index,
  vtkDataObject* non_composite_input, vtkSelection* sel,
  vtkInformation* outInfo)
{
  if (non_composite_input->IsA("vtkCompositeDataSet"))
  {
    vtkErrorMacro(
      "RequestDataInternal cannot be called with a composite data input.");
    return NULL;
  }


  // Here either COMPOSITE_INDEX() is present or
  // (HIERARCHICAL_LEVEL(), HIERARCHICAL_INDEX()) is present.

  if (!non_composite_input || !sel)
  {
    return NULL;
  }

  for (unsigned int n = 0; n < sel->GetNumberOfNodes(); n++)
  {
    vtkSelectionNode* node = sel->GetNode(n);
    vtkInformation* nodeProperties = node->GetProperties();
    if (nodeProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
      unsigned int cur_index = static_cast<unsigned int>(
          nodeProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()));
      if (cur_index == composite_index)
      {
        return this->RequestDataFromBlock(non_composite_input, node, outInfo);
      }
    }
    else if (nodeProperties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
        nodeProperties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()))
    {
      unsigned int cur_level = static_cast<unsigned int>(
          nodeProperties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL()));
      unsigned int cur_index = static_cast<unsigned int>(
          nodeProperties->Get(vtkSelectionNode::HIERARCHICAL_INDEX()));
      if (cur_level == level && cur_index == index)
      {
        return this->RequestDataFromBlock(non_composite_input, node, outInfo);
      }
    }
    else
    {
      return this->RequestDataFromBlock(non_composite_input, node, outInfo);
    }
  }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkExtractSelection::RequestDataFromBlock(
  vtkDataObject* input, vtkSelectionNode* sel, vtkInformation* outInfo)
{
  vtkAlgorithm *subFilter = NULL;
  int seltype = sel->GetContentType();
  switch (seltype)
  {
  case vtkSelectionNode::GLOBALIDS:
  case vtkSelectionNode::PEDIGREEIDS:
  case vtkSelectionNode::VALUES:
  case vtkSelectionNode::INDICES:
    if (input->IsA("vtkTable"))
    {
      subFilter = this->RowsFilter;
    }
    else
    {
      subFilter = this->IdsFilter;
    }
    break;

  case vtkSelectionNode::FRUSTUM:
    subFilter = this->FrustumFilter;
    this->FrustumFilter->SetShowBounds(this->ShowBounds);
    break;

  case vtkSelectionNode::LOCATIONS:
    subFilter = this->UseProbeForLocations?
      static_cast<vtkExtractSelectionBase*>(this->ProbeFilter) :
      static_cast<vtkExtractSelectionBase*>(this->LocationsFilter);
    break;

  case vtkSelectionNode::THRESHOLDS:
    subFilter = this->ThresholdsFilter;
    break;

  case vtkSelectionNode::BLOCKS:
    subFilter = this->BlockFilter;
    break;

  default:
      vtkErrorMacro("Unrecognized CONTENT_TYPE: " << seltype);
      return NULL;
  }

  if (vtkExtractSelectionBase* esb =
    vtkExtractSelectionBase::SafeDownCast(subFilter))
  {
    // Pass flags to the subFilter.
    esb->SetPreserveTopology(this->PreserveTopology);
  }

  vtkSmartPointer<vtkSelection> tempSel =
    vtkSmartPointer<vtkSelection>::New();
  tempSel->AddNode(sel);
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(tempSel);
  subFilter->SetInputConnection(1, tp->GetOutputPort());
  tp->Delete();

  vtkDataObject* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  tp = vtkTrivialProducer::New();
  tp->SetOutput(inputCopy);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    tp->SetWholeExtent(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  }
  subFilter->SetInputConnection(0, tp->GetOutputPort());
  tp->Delete();

  subFilter->UpdateInformation();

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

  vtkDataObject* ecOutput = subFilter->GetOutputDataObject(0);
  vtkDataObject* output = ecOutput->NewInstance();
  output->ShallowCopy(ecOutput);

  //make sure everything is deallocated
  inputCopy->Delete();
  ecOutput->Initialize();

  subFilter->SetInputConnection(0, NULL);
  subFilter->SetInputConnection(1, NULL);
  return output;
}

//----------------------------------------------------------------------------
void vtkExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "UseProbeForLocations: " << this->UseProbeForLocations << endl;
  os << indent << "ShowBounds: " << this->ShowBounds << endl;
}
