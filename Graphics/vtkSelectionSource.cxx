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
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedIntArray.h"

#include "vtkstd/vector"
#include "vtkstd/set"

vtkStandardNewMacro(vtkSelectionSource);

class vtkSelectionSourceInternals
{
public:
  typedef vtkstd::set<vtkIdType> IDSetType;
  typedef vtkstd::vector<IDSetType> IDsType;
  IDsType IDs;
  
  typedef vtkstd::set<vtkStdString> StringIDSetType;
  typedef vtkstd::vector<StringIDSetType> StringIDsType;
  StringIDsType StringIDs;

  vtkstd::vector<double> Thresholds;
  vtkstd::vector<double> Locations;
  IDSetType Blocks;
  double Frustum[32];
};

//----------------------------------------------------------------------------
vtkSelectionSource::vtkSelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->Internal = new vtkSelectionSourceInternals;
  
  this->ContentType = vtkSelectionNode::INDICES;
  this->FieldType = vtkSelectionNode::CELL;
  this->ContainingCells = 1;
  this->Inverse = 0;
  this->ArrayName = NULL;
  this->ArrayComponent = 0;
  for (int cc=0; cc < 32; cc++)
    {
    this->Internal->Frustum[cc] = 0;
    }
  this->CompositeIndex = -1;
  this->HierarchicalLevel = -1;
  this->HierarchicalIndex = -1;
}

//----------------------------------------------------------------------------
vtkSelectionSource::~vtkSelectionSource()
{
  delete this->Internal;
  if (this->ArrayName)
    {
    delete[] this->ArrayName;
    }
}

//----------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllIDs()
{
  this->Internal->IDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllStringIDs()
{
  this->Internal->StringIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllLocations()
{
  this->Internal->Locations.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllThresholds()
{
  this->Internal->Thresholds.clear();
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
void vtkSelectionSource::AddStringID(vtkIdType proc, const char* id)
{
  // proc == -1 means all processes. All other are stored at index proc+1
  proc++;

  if (proc >= (vtkIdType)this->Internal->StringIDs.size())
    {
    this->Internal->StringIDs.resize(proc+1);
    }
  vtkSelectionSourceInternals::StringIDSetType& idSet = this->Internal->StringIDs[proc];
  idSet.insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::AddLocation(double x, double y, double z)
{
  this->Internal->Locations.push_back(x);
  this->Internal->Locations.push_back(y);
  this->Internal->Locations.push_back(z);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::AddThreshold(double min, double max)
{
  this->Internal->Thresholds.push_back(min);
  this->Internal->Thresholds.push_back(max);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::SetFrustum(double *vertices)
{
  for (int cc=0; cc < 32; cc++)
    {
    if (vertices[cc] != this->Internal->Frustum[cc])
      {
      memcpy(this->Internal->Frustum, vertices, 32*sizeof(double));
      this->Modified();
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSelectionSource::AddBlock(vtkIdType block)
{
  this->Internal->Blocks.insert(block);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::RemoveAllBlocks()
{
  this->Internal->Blocks.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ContentType: " ;
  switch (this->ContentType)
    {
  case vtkSelectionNode::SELECTIONS:
    os << "SELECTIONS";
    break;
  case vtkSelectionNode::GLOBALIDS:
    os << "GLOBALIDS";
    break;
  case vtkSelectionNode::VALUES:
    os << "VALUES";
    break;
  case vtkSelectionNode::INDICES:
    os << "INDICES";
    break;
  case vtkSelectionNode::FRUSTUM:
    os << "FRUSTUM";
    break;
  case vtkSelectionNode::LOCATIONS:
    os << "LOCATIONS";
    break;
  case vtkSelectionNode::THRESHOLDS:
    os << "THRESHOLDS";
    break;
  case vtkSelectionNode::BLOCKS:
    os << "BLOCKS";
    break;
  default:
    os << "UNKNOWN";
    }
  os << endl;

  os << indent << "FieldType: " ;
  switch (this->FieldType)
    {
    case vtkSelectionNode::CELL:
      os << "CELL";
      break;
    case vtkSelectionNode::POINT:
      os << "POINT";
      break;
    case vtkSelectionNode::FIELD:
      os << "FIELD";
      break;
    case vtkSelectionNode::VERTEX:
      os << "VERTEX";
      break;
    case vtkSelectionNode::EDGE:
      os << "EDGE";
      break;
    case vtkSelectionNode::ROW:
      os << "ROW";
      break;
    default:
      os << "UNKNOWN";
    }
  os << endl;

  os << indent << "ContainingCells: ";
  os << (this->ContainingCells?"CELLS":"POINTS") << endl;
  os << indent << "Inverse: " << this->Inverse << endl;
  os << indent << "ArrayName: " << (this->ArrayName?this->ArrayName:"NULL") << endl;
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
  os << indent << "CompositeIndex: " << this->CompositeIndex << endl;
  os << indent << "HierarchicalLevel: " << this->HierarchicalLevel << endl;
  os << indent << "HierarchicalIndex: " << this->HierarchicalIndex << endl;
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
  vtkSelection* outputSel = vtkSelection::GetData(outputVector);
  vtkSmartPointer<vtkSelectionNode> output =
    vtkSmartPointer<vtkSelectionNode>::New();
  outputSel->AddNode(output);
  vtkInformation* oProperties = output->GetProperties();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = 0;
  if (outInfo->Has(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
    {
    piece = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    }

  if (this->CompositeIndex >= 0)
    {
    oProperties->Set(vtkSelectionNode::COMPOSITE_INDEX(), this->CompositeIndex);
    }

  if (this->HierarchicalLevel >= 0 && this->HierarchicalIndex >= 0)
    {
    oProperties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(),
      this->HierarchicalLevel);
    oProperties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(),
      this->HierarchicalIndex);
    }

  // First look for string ids.
  if (
    ((this->ContentType == vtkSelectionNode::GLOBALIDS) ||
    (this->ContentType == vtkSelectionNode::PEDIGREEIDS) ||
    (this->ContentType == vtkSelectionNode::INDICES)) &&
    !this->Internal->StringIDs.empty())
    {    
    oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), 
      this->ContentType);
    oProperties->Set(vtkSelectionNode::FIELD_TYPE(),
      this->FieldType);

    vtkStringArray* selectionList = vtkStringArray::New();
    output->SetSelectionList(selectionList);
    selectionList->Delete();

    // Number of selected items common to all pieces
    vtkIdType numCommonElems = 0;
    if (!this->Internal->StringIDs.empty())
      {
      numCommonElems = this->Internal->StringIDs[0].size();
      }
    if (piece+1 >= (int)this->Internal->StringIDs.size() &&
        numCommonElems == 0)
      {
      vtkDebugMacro("No selection for piece: " << piece);
      }
    else
      {
      // idx == 0 is the list for all pieces
      // idx == piece+1 is the list for the current piece
      size_t pids[2] = {0, piece+1};
      for(int i=0; i<2; i++)
        {
        size_t idx = pids[i];
        if (idx >= this->Internal->StringIDs.size())
          {
          continue;
          }
        
        vtkSelectionSourceInternals::StringIDSetType& selSet =
          this->Internal->StringIDs[idx];
        
        if (selSet.size() > 0)
          {
          // Create the selection list
          selectionList->SetNumberOfTuples(selSet.size());
          // iterate over ids and insert to the selection list
          vtkSelectionSourceInternals::StringIDSetType::iterator iter =
            selSet.begin();
          for (vtkIdType idx2=0; iter != selSet.end(); iter++, idx2++)
            {
            selectionList->SetValue(idx2, *iter);
            } 
          }
        }
      }
    }

  // If no string ids, use integer ids.
  if (
    ((this->ContentType == vtkSelectionNode::GLOBALIDS) ||
    (this->ContentType == vtkSelectionNode::PEDIGREEIDS) ||
    (this->ContentType == vtkSelectionNode::INDICES)) &&
    this->Internal->StringIDs.empty())
    {    
    oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), 
      this->ContentType);
    oProperties->Set(vtkSelectionNode::FIELD_TYPE(),
      this->FieldType);

    vtkIdTypeArray* selectionList = vtkIdTypeArray::New();
    output->SetSelectionList(selectionList);
    selectionList->Delete();

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
      }
    else
      {
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
          // Create the selection list
          selectionList->SetNumberOfTuples(selSet.size());
          // iterate over ids and insert to the selection list
          vtkSelectionSourceInternals::IDSetType::iterator iter =
            selSet.begin();
          for (vtkIdType idx2=0; iter != selSet.end(); iter++, idx2++)
            {
            selectionList->SetValue(idx2, *iter);
            }  
          }
        }
      }
    }
  
  if (this->ContentType == vtkSelectionNode::LOCATIONS)
    {
    oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), 
                                 this->ContentType);
    oProperties->Set(vtkSelectionNode::FIELD_TYPE(),
                                 this->FieldType);
    // Create the selection list
    vtkDoubleArray* selectionList = vtkDoubleArray::New(); 
    selectionList->SetNumberOfComponents(3);
    selectionList->SetNumberOfValues(this->Internal->Locations.size());

    vtkstd::vector<double>::iterator iter =
      this->Internal->Locations.begin();
    for (vtkIdType cc=0;
      iter != this->Internal->Locations.end(); ++iter, ++cc)
      {
      selectionList->SetValue(cc, *iter);
      }

    output->SetSelectionList(selectionList);
    selectionList->Delete();    
    }

  if (this->ContentType == vtkSelectionNode::THRESHOLDS)
    {
    oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), 
                                 this->ContentType);
    oProperties->Set(vtkSelectionNode::FIELD_TYPE(),
                                 this->FieldType);
    oProperties->Set(vtkSelectionNode::COMPONENT_NUMBER(),
                     this->ArrayComponent);
    // Create the selection list
    vtkDoubleArray* selectionList = vtkDoubleArray::New(); 
    selectionList->SetNumberOfComponents(1);
    selectionList->SetNumberOfValues(this->Internal->Thresholds.size());

    vtkstd::vector<double>::iterator iter =
      this->Internal->Thresholds.begin();
    for (vtkIdType cc=0;
      iter != this->Internal->Thresholds.end(); ++iter, ++cc)
      {
      selectionList->SetValue(cc, *iter);
      }

    output->SetSelectionList(selectionList);
    selectionList->Delete();    
    }

  if (this->ContentType == vtkSelectionNode::FRUSTUM)
    {
    oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    oProperties->Set(vtkSelectionNode::FIELD_TYPE(), this->FieldType);
    // Create the selection list
    vtkDoubleArray* selectionList = vtkDoubleArray::New(); 
    selectionList->SetNumberOfComponents(4);
    selectionList->SetNumberOfTuples(8);
    for (vtkIdType cc=0; cc < 32; cc++)
      {
      selectionList->SetValue(cc, this->Internal->Frustum[cc]);
      }

    output->SetSelectionList(selectionList);
    selectionList->Delete();    
    }

  if (this->ContentType == vtkSelectionNode::BLOCKS)
    {
    oProperties->Set(vtkSelectionNode::CONTENT_TYPE(), this->ContentType);
    vtkUnsignedIntArray* selectionList = vtkUnsignedIntArray::New();
    selectionList->SetNumberOfComponents(1);
    selectionList->SetNumberOfTuples(this->Internal->Blocks.size());
    vtkSelectionSourceInternals::IDSetType::iterator iter;
    vtkIdType cc=0;
    for (iter = this->Internal->Blocks.begin();
      iter != this->Internal->Blocks.end(); ++iter, ++cc)
      {
      selectionList->SetValue(cc, *iter);
      }
    output->SetSelectionList(selectionList);
    selectionList->Delete();
    }

  oProperties->Set(vtkSelectionNode::CONTAINING_CELLS(),
                               this->ContainingCells);  

  oProperties->Set(vtkSelectionNode::INVERSE(),
                               this->Inverse);

  if (output->GetSelectionList())
    {
    output->GetSelectionList()->SetName(this->ArrayName);
    }

  return 1;
}

