/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataSet.h"

#include "vtkDataSet.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkMultiGroupDataIterator.h"
#include "vtkMultiGroupDataSetInternal.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationIntegerKey.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiGroupDataSet, "1.2");
vtkStandardNewMacro(vtkMultiGroupDataSet);

vtkCxxSetObjectMacro(vtkMultiGroupDataSet,MultiGroupDataInformation,vtkMultiGroupDataInformation);

vtkInformationKeyMacro(vtkMultiGroupDataSet,GROUP,Integer);

//----------------------------------------------------------------------------
vtkMultiGroupDataSet::vtkMultiGroupDataSet()
{
  this->Internal = new vtkMultiGroupDataSetInternal;
  this->MultiGroupDataInformation = vtkMultiGroupDataInformation::New();
}

//----------------------------------------------------------------------------
vtkMultiGroupDataSet::~vtkMultiGroupDataSet()
{
  this->InitializeDataSets();
  delete this->Internal;
  this->SetMultiGroupDataInformation(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkMultiGroupDataSet::NewIterator()
{
  vtkMultiGroupDataIterator* iter = vtkMultiGroupDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
vtkMGDSNode* vtkMultiGroupDataSet::NewNode()
{
  return new vtkMGDSNode;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::InitializeDataSets()
{
  this->Internal->DataSets.clear();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::Initialize()
{
  this->Superclass::Initialize();
  this->InitializeDataSets();
  this->SetMultiGroupDataInformation(0);
  this->MultiGroupDataInformation = vtkMultiGroupDataInformation::New();
}

//----------------------------------------------------------------------------
unsigned int vtkMultiGroupDataSet::GetNumberOfGroups()
{
  return this->Internal->DataSets.size();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::SetNumberOfGroups(unsigned int numGroups)
{
  this->MultiGroupDataInformation->SetNumberOfGroups(numGroups);
  if (numGroups == this->GetNumberOfGroups())
    {
    return;
    }
  this->Internal->DataSets.resize(numGroups);
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkMultiGroupDataSet::GetNumberOfDataSets(unsigned int group)
{
  if (this->Internal->DataSets.size() <= group)
    {
    return 0;
    }

  vtkMultiGroupDataSetInternal::GroupDataSetsType& ldataSets = 
    this->Internal->DataSets[group];

  return ldataSets.size();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::SetNumberOfDataSets(unsigned int group, 
                                                 unsigned int numDataSets)
{
  this->MultiGroupDataInformation->SetNumberOfDataSets(group, numDataSets);
  if (numDataSets == this->GetNumberOfDataSets(group))
    {
    return;
    }
  // Make sure that there is a vector allocated for this group
  if (this->Internal->DataSets.size() <= group)
    {
    this->SetNumberOfGroups(group+1);
    }

  vtkMultiGroupDataSetInternal::GroupDataSetsType& ldataSets = 
    this->Internal->DataSets[group];
  
  // We need to delete all extra nodes since we manage memory for them.
  unsigned int curNumDataSets = ldataSets.size();
  ldataSets.resize(numDataSets);

  // Assign NULL to all new pointers. We use this later to figure out
  // whether a node allocated for a particular entry.
  if (curNumDataSets < numDataSets)
    {
    for (unsigned int i=curNumDataSets; i<numDataSets; i++)
      {
      ldataSets[i] = 0;
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::InitializeNode(unsigned int group, 
                                            unsigned int id)
{

  // Make sure that there is a vector allocated for this group
  if (this->Internal->DataSets.size() <= group)
    {
    this->SetNumberOfGroups(group+1);
    }

  vtkMultiGroupDataSetInternal::GroupDataSetsType& ldataSets = 
    this->Internal->DataSets[group];
  
  // Make sure that the size of the vector for this group is big enough.
  if (ldataSets.size() <= id)
    {
    this->SetNumberOfDataSets(group, id+1);
    }

  ldataSets[id] = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::SetDataSet(unsigned int group, 
                                        unsigned int id, 
                                        vtkDataObject* ds)
{
  // Make sure that there is a vector allocated for this group
  if (this->Internal->DataSets.size() <= group)
    {
    this->SetNumberOfGroups(group+1);
    }

  vtkMultiGroupDataSetInternal::GroupDataSetsType& ldataSets = 
    this->Internal->DataSets[group];
  
  // Make sure that the size of the vector for this group is big enough.
  if (ldataSets.size() <= id)
    {
    this->SetNumberOfDataSets(group, id+1);
    }
  
  ldataSets[id] = ds;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::AddDataSet(vtkInformation* index, vtkDataObject* dobj)
{
  if (index->Has(INDEX()) && index->Has(GROUP()))
    {
    this->SetDataSet(index->Get(GROUP()), index->Get(INDEX()), dobj);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiGroupDataSet::GetDataSet(unsigned int group, 
                                                  unsigned int id)
{
  if (this->Internal->DataSets.size() <= group)
    {
    return 0;
    }

  vtkMultiGroupDataSetInternal::GroupDataSetsType& ldataSets = 
    this->Internal->DataSets[group];
  if (ldataSets.size() <= id)
    {
    return 0;
    }

  if (!ldataSets[id])
    {
    return 0;
    }

  return ldataSets[id];
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiGroupDataSet::GetDataSet(vtkInformation* index)
{
  if (index->Has(INDEX()) && index->Has(GROUP()))
    {
    return this->GetDataSet(index->Get(GROUP()), index->Get(INDEX()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::ShallowCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Superclass::ShallowCopy(src);

  vtkMultiGroupDataSet* from = vtkMultiGroupDataSet::SafeDownCast(src);
  if (from)
    {
    this->SetMultiGroupDataInformation(from->MultiGroupDataInformation);
    unsigned int numGroups = from->GetNumberOfGroups();
    this->SetNumberOfGroups(numGroups);
    for (unsigned int i=0; i<numGroups; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        this->SetDataSet(i, j, from->GetDataSet(i,j));
        }
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::DeepCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Superclass::ShallowCopy(src);
  this->SetMultiGroupDataInformation(0);
  this->MultiGroupDataInformation = vtkMultiGroupDataInformation::New();

  vtkMultiGroupDataSet* from = vtkMultiGroupDataSet::SafeDownCast(src);
  if (from)
    {
    this->MultiGroupDataInformation->DeepCopy(
      from->MultiGroupDataInformation);
    unsigned int numGroups = from->GetNumberOfGroups();
    this->SetNumberOfGroups(numGroups);
    for (unsigned int i=0; i<numGroups; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkDataObject* ds = from->GetDataSet(i,j);
        if (ds)
          {
          vtkDataObject* copy = ds->NewInstance();
          copy->DeepCopy(ds);
          this->SetDataSet(i, j, copy);
          }
        }
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkMultiGroupDataSet::GetNumberOfPoints()
{
  vtkIdType numPts = 0;

  vtkCompositeDataIterator* iterator = this->NewIterator();
  iterator->InitTraversal();
  while (!iterator->IsDoneWithTraversal())
    {
    vtkDataObject* dObj = iterator->GetCurrentDataObject();
    if (dObj)
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(dObj);
      if (ds)
        {
        numPts += ds->GetNumberOfPoints();
        }
      else
        {
        vtkMultiGroupDataSet* hds = 
          vtkMultiGroupDataSet::SafeDownCast(dObj);
        if (hds)
          {
          numPts += hds->GetNumberOfPoints();
          }
        }
      }
    iterator->GoToNextItem();
    }
  iterator->Delete();

  return numPts;
}

//----------------------------------------------------------------------------
vtkMultiGroupDataSet* vtkMultiGroupDataSet::GetData(vtkInformation* info)
{
  return
    info? vtkMultiGroupDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkMultiGroupDataSet* vtkMultiGroupDataSet::GetData(vtkInformationVector* v,
                                                    int i)
{
  return vtkMultiGroupDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MultiGroupDataInformation: ";
  if (this->MultiGroupDataInformation)
    {
    os << endl;
    this->MultiGroupDataInformation->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

