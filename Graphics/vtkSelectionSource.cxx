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
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

#include "vtkstd/vector"
#include "vtkstd/set"

vtkCxxRevisionMacro(vtkSelectionSource, "1.15");
vtkStandardNewMacro(vtkSelectionSource);

class vtkSelectionSourceInternals
{
public:
  typedef vtkstd::set<vtkIdType> IDSetType;
  typedef vtkstd::vector<IDSetType> IDsType;
  IDsType IDs;
  
  vtkstd::vector<double> Thresholds;
  vtkstd::vector<double> Locations;
  double Frustum[32];
};

//----------------------------------------------------------------------------
vtkSelectionSource::vtkSelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->Internal = new vtkSelectionSourceInternals;
  
  this->ContentType = vtkSelection::INDICES;
  this->FieldType = vtkSelection::CELL;
  this->ContainingCells = 1;
  this->PreserveTopology = 0;
  this->Inverse = 0;
  this->ShowBounds = 0;
  this->ArrayName = NULL;
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

  os << indent << "ContainingCells: ";
  os << (this->ContainingCells?"CELLS":"POINTS") << endl;
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
  os << indent << "Inverse: " << this->Inverse << endl;
  os << indent << "ShowBounds: " << this->ShowBounds << endl;
  os << indent << "ArrayName: " << (this->ArrayName?this->ArrayName:"NULL") << endl;
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

  if (
    (this->ContentType == vtkSelection::GLOBALIDS) ||
    (this->ContentType == vtkSelection::INDICES))
    {    
    output->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), 
      this->ContentType);
    output->GetProperties()->Set(vtkSelection::FIELD_TYPE(),
      this->FieldType);

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
    }
  
  if (this->ContentType == vtkSelection::LOCATIONS)
    {
    output->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), 
                                 this->ContentType);
    output->GetProperties()->Set(vtkSelection::FIELD_TYPE(),
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

  if (this->ContentType == vtkSelection::THRESHOLDS)
    {
    output->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), 
                                 this->ContentType);
    output->GetProperties()->Set(vtkSelection::FIELD_TYPE(),
                                 this->FieldType);
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

  if (this->ContentType == vtkSelection::FRUSTUM)
    {
    output->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), 
                                 this->ContentType);
    output->GetProperties()->Set(vtkSelection::FIELD_TYPE(),
                                 this->FieldType);
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

  output->GetProperties()->Set(vtkSelection::CONTAINING_CELLS(),
                               this->ContainingCells);  

  output->GetProperties()->Set(vtkSelection::PRESERVE_TOPOLOGY(),
                               this->PreserveTopology);

  output->GetProperties()->Set(vtkSelection::INVERSE(),
                               this->Inverse);

  output->GetProperties()->Set(vtkSelection::ARRAY_NAME(),
                               this->ArrayName);

  output->GetProperties()->Set(vtkSelection::SHOW_BOUNDS(),
                               this->ShowBounds);
  return 1;
}

