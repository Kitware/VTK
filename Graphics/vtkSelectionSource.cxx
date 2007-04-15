/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionSource.h"

#include "vtkCommand.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

#include "vtkstd/vector"
#include "vtkstd/set"

vtkCxxRevisionMacro(vtkSelectionSource, "1.7");
vtkStandardNewMacro(vtkSelectionSource);

struct vtkSelectionSourceInternals
{
  typedef vtkstd::set<vtkIdType> IDSetType;
  typedef vtkstd::vector<IDSetType> IDsType;
  IDsType IDs;
};

//----------------------------------------------------------------------------
vtkSelectionSource::vtkSelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->Internal = new vtkSelectionSourceInternals;
  this->ContentType = vtkSelection::INDICES;
  this->FieldType = vtkSelection::CELL;
}

//----------------------------------------------------------------------------
vtkSelectionSource::~vtkSelectionSource()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllIDs()
{
  this->Internal->IDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::AddID(vtkIdType proc, vtkIdType id)
{
  // proc == -1 means all processes. All other are stored at index proc+1
  proc++;

  if (proc >= (vtkIdType)this->Internal->IDs.size())
    {
    this->Internal->IDs.resize(proc+1);
    }
  vtkSelectionSourceInternals::IDSetType& idSet = this->Internal->IDs[proc];
  idSet.insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ContentType: " ;
  switch (this->ContentType)
    {
  case vtkSelection::SELECTIONS:
    os << "SELECTIONS";
    break;
  case vtkSelection::COMPOSITE_SELECTIONS:
    os << "COMPOSITE_SELECTIONS";
    break;
  case vtkSelection::GLOBALIDS:
    os << "GLOBALIDS";
    break;
  case vtkSelection::VALUES:
    os << "VALUES";
    break;
  case vtkSelection::INDICES:
    os << "INDICES";
    break;
  case vtkSelection::FRUSTUM:
    os << "FRUSTUM";
    break;
  case vtkSelection::LOCATIONS:
    os << "LOCATIONS";
    break;
  case vtkSelection::THRESHOLDS:
    os << "THRESHOLDS";
    break;
  default:
    os << "UNKNOWN";
    }
  os << endl;

  os << indent << "FieldType: " ;
  switch (this->FieldType)
    {
  case vtkSelection::CELL:
    os << "CELL";
    break;
  case vtkSelection::POINT:
    os << "POINT";
    break;
  default:
    os << "UNKNOWN";
    }
  os << endl;
}

//----------------------------------------------------------------------------
int vtkSelectionSource::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // We can handle multiple piece request.
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectionSource::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector )
{
  vtkSelection* output = vtkSelection::GetData(outputVector);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = 0;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  // Number of selected items common to all pieces
  vtkIdType numCommonElems = 0;
  if (!this->Internal->IDs.empty())
    {
    numCommonElems = this->Internal->IDs[0].size();
    }
  if (piece+1 >= (int)this->Internal->IDs.size() &&
      numCommonElems == 0)
    {
    vtkDebugMacro("No selection for piece: " << piece);
    return 1;
    }

  // idx == 0 is the list for all pieces
  // idx == piece+1 is the list for the current piece
  size_t pids[2] = {0, piece+1};
  for(int i=0; i<2; i++)
    {
    size_t idx = pids[i];
    if (idx >= this->Internal->IDs.size())
      {
      continue;
      }
    vtkSelectionSourceInternals::IDSetType& selSet =
      this->Internal->IDs[idx];
    
    if (selSet.size() > 0)
      {
      output->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), 
                                   this->ContentType);
      output->GetProperties()->Set(vtkSelection::FIELD_TYPE(),
                                   this->FieldType);
      // Create the selection list
      vtkIdTypeArray* selectionList = vtkIdTypeArray::New();
      selectionList->SetNumberOfTuples(selSet.size());
      // iterate over ids and insert to the selection list
      vtkSelectionSourceInternals::IDSetType::iterator iter =
        selSet.begin();
      for (vtkIdType idx2=0; iter != selSet.end(); iter++, idx2++)
        {
        selectionList->SetValue(idx2, *iter);
        }
      output->SetSelectionList(selectionList);
      selectionList->Delete();
      }
    }
  
  return 1;
}

