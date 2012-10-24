/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectTree.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeInternals.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMultiPieceDataSet.h"


//----------------------------------------------------------------------------
vtkDataObjectTree::vtkDataObjectTree()
{
  this->Internals = new vtkDataObjectTreeInternals;
}

//----------------------------------------------------------------------------
vtkDataObjectTree::~vtkDataObjectTree()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkDataObjectTree* vtkDataObjectTree::GetData(vtkInformation* info)
{
  return info? vtkDataObjectTree::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkDataObjectTree* vtkDataObjectTree::GetData(vtkInformationVector* v,
                                                  int i)
{
  return vtkDataObjectTree::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::SetNumberOfChildren(unsigned int num)
{
  this->Internals->Children.resize(num);
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkDataObjectTree::GetNumberOfChildren()
{
  return static_cast<unsigned int>(this->Internals->Children.size());
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::SetChild(unsigned int index, vtkDataObject* dobj)
{
  if (this->Internals->Children.size() <= index)
    {
    this->SetNumberOfChildren(index+1);
    }

  vtkDataObjectTreeItem& item = this->Internals->Children[index];
  if(item.DataObject!=dobj)
    {
    item.DataObject = dobj;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::RemoveChild(unsigned int index)
{
  if (this->Internals->Children.size() <= index)
    {
    vtkErrorMacro("The input index is out of range.");
    return;
    }

  vtkDataObjectTreeItem& item = this->Internals->Children[index];
  item.DataObject = NULL;
  this->Internals->Children.erase(this->Internals->Children.begin()+index);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTree::GetChild(unsigned int index)
{
  if (index < this->Internals->Children.size())
    {
    return this->Internals->Children[index].DataObject;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkDataObjectTree::GetChildMetaData(unsigned int index)
{
  if (index < this->Internals->Children.size())
    {
    vtkDataObjectTreeItem& item = this->Internals->Children[index];
    if (!item.MetaData)
      {
      // New vtkInformation is allocated is none is already present.
      item.MetaData.TakeReference(vtkInformation::New());
      }
    return item.MetaData;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::SetChildMetaData(unsigned int index, vtkInformation* info)
{
  if (this->Internals->Children.size() <= index)
    {
    this->SetNumberOfChildren(index+1);
    }

  vtkDataObjectTreeItem& item = this->Internals->Children[index];
  item.MetaData = info;
}

//----------------------------------------------------------------------------
int vtkDataObjectTree::HasChildMetaData(unsigned int index)
{
  if (index < this->Internals->Children.size())
    {
    vtkDataObjectTreeItem& item = this->Internals->Children[index];
    return (item.MetaData.GetPointer() != NULL)? 1 : 0;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::CopyStructure(vtkCompositeDataSet* compositeSource)
{
  if(!compositeSource)
    {
    return;
    }
  vtkDataObjectTree* source = vtkDataObjectTree::SafeDownCast(compositeSource);
  if (source == this)
    {
    return;
    }

  this->Internals->Children.clear();
  if (!source)
    {
    //WARNING:
    //If we copy the structure of from a non-tree composite data set
    //we create a special structure of two levels, the first level
    //is just a single multipiece and the second level are all the data sets.
    //This is likely to change in the future!
    vtkMultiPieceDataSet* mds = vtkMultiPieceDataSet::New();
    this->SetChild(0, mds);
    mds->Delete();

    vtkInformation* info = vtkInformation::New();
    info->Set(vtkCompositeDataSet::NAME(),"All Blocks");
    this->SetChildMetaData(0, info);
    info->FastDelete();


    int totalNumBlocks=0;
    vtkCompositeDataIterator* iter = compositeSource->NewIterator();
    iter->SkipEmptyNodesOff();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      totalNumBlocks++;
      }
    iter->Delete();

    mds->SetNumberOfChildren(totalNumBlocks);
    return;
    }

  this->Internals->Children.resize(source->Internals->Children.size());

  vtkDataObjectTreeInternals::Iterator srcIter =
    source->Internals->Children.begin();
  vtkDataObjectTreeInternals::Iterator myIter =
    this->Internals->Children.begin();
  for (; srcIter != source->Internals->Children.end(); ++srcIter, myIter++)
    {
    vtkDataObjectTree* compositeSrc =
      vtkDataObjectTree::SafeDownCast(srcIter->DataObject);
    if (compositeSrc)
      {
      vtkDataObjectTree* copy = compositeSrc->NewInstance();
      myIter->DataObject.TakeReference(copy);
      copy->CopyStructure(compositeSrc);
      }

    // shallow copy meta data.
    if (srcIter->MetaData)
      {
      vtkInformation* info = vtkInformation::New();
      info->Copy(srcIter->MetaData, /*deep=*/0);
      myIter->MetaData = info;
      info->FastDelete();
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkDataObjectTreeIterator* vtkDataObjectTree::NewTreeIterator()
{
  vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkDataObjectTree::NewIterator()
{
  return this->NewTreeIterator();
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::SetDataSet(vtkCompositeDataIterator* iter,
  vtkDataObject* dataObj)
{
  vtkDataObjectTreeIterator* treeIter = vtkDataObjectTreeIterator::SafeDownCast(iter);
  if(treeIter)
    {
    this->SetDataSetFrom(treeIter,dataObj);
    return;
    }

  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return;
    }


  //WARNING: We are doing something special here. See comments
  // in CopyStructure()

  unsigned int index = iter->GetCurrentFlatIndex();
  if(this->GetNumberOfChildren()!=1)
    {
    vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
    return;
    }
  vtkMultiPieceDataSet* parent  = vtkMultiPieceDataSet::SafeDownCast(this->GetChild(0));
  if(!parent)
    {
    vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
    return;
    }
  parent->SetChild(index, dataObj);
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::SetDataSetFrom(vtkDataObjectTreeIterator* iter,
  vtkDataObject* dataObj)
{
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return;
    }

  vtkDataObjectTreeIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return;
    }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return;
      }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
    }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
    {
    vtkErrorMacro("Structure does not match. "
      "You must use CopyStructure before calling this method.");
    return;
    }

  parent->SetChild(index.back(), dataObj);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTree::GetDataSet(vtkCompositeDataIterator* compositeIter)
{
  if (!compositeIter || compositeIter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
    }

  vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(compositeIter);
  if (!iter)
    {
    //WARNING: We are doing something special here. See comments
    // in CopyStructure()
    //To do: More clear check of structures here. At least something like this->Depth()==1
    unsigned int currentFlatIndex = compositeIter->GetCurrentFlatIndex();

    if(this->GetNumberOfChildren()!=1)
      {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return NULL;
      }
    vtkMultiPieceDataSet* parent  = vtkMultiPieceDataSet::SafeDownCast(this->GetChild(0));
    if(!parent)
      {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return NULL;
      }

    if(currentFlatIndex < parent->GetNumberOfChildren())
      {
      return parent->GetChild(currentFlatIndex);
      }
    else
      {
      return NULL;
      }
    }

  vtkDataObjectTreeIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
    }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return 0;
      }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
    }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
    {
    vtkErrorMacro("Structure does not match. "
      "You must use CopyStructure before calling this method.");
    return 0;
    }

  return parent->GetChild(index.back());
}

//----------------------------------------------------------------------------
vtkInformation* vtkDataObjectTree::GetMetaData(vtkCompositeDataIterator* compositeIter)
{
  vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(compositeIter);
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
    }

  vtkDataObjectTreeIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
    }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return 0;
      }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
    }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
    {
    vtkErrorMacro("Structure does not match. "
      "You must use CopyStructure before calling this method.");
    return 0;
    }

  return parent->GetChildMetaData(index.back());
}

//----------------------------------------------------------------------------
int vtkDataObjectTree::HasMetaData(vtkCompositeDataIterator* compositeIter)
{
  vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(compositeIter);
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
    }

  vtkDataObjectTreeIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
    }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return 0;
      }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
    }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
    {
    vtkErrorMacro("Structure does not match. "
      "You must use CopyStructure before calling this method.");
    return 0;
    }

  return parent->HasChildMetaData(index.back());
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
    {
    return;
    }

  this->Internals->Children.clear();
  this->Superclass::ShallowCopy(src);

  vtkDataObjectTree* from = vtkDataObjectTree::SafeDownCast(src);
  if (from)
    {
    unsigned int numChildren = from->GetNumberOfChildren();
    this->SetNumberOfChildren(numChildren);
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkDataObject* child = from->GetChild(cc);
      if (child)
        {
        if (child->IsA("vtkDataObjectTree"))
          {
          vtkDataObject* clone = child->NewInstance();
          clone->ShallowCopy(child);
          this->SetChild(cc, clone);
          clone->FastDelete();
          }
        else
          {
          this->SetChild(cc, child);
          }
        }
      if (from->HasChildMetaData(cc))
        {
        vtkInformation* toInfo = this->GetChildMetaData(cc);
        toInfo->Copy(from->GetChildMetaData(cc), /*deep=*/0);
        }
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::DeepCopy(vtkDataObject* src)
{
  if (src == this)
    {
    return;
    }

  this->Internals->Children.clear();
  this->Superclass::DeepCopy(src);

  vtkDataObjectTree* from = vtkDataObjectTree::SafeDownCast(src);
  if (from)
    {
    unsigned int numChildren = from->GetNumberOfChildren();
    this->SetNumberOfChildren(numChildren);
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkDataObject* fromChild = from->GetChild(cc);
      if (fromChild)
        {
        vtkDataObject* toChild = fromChild->NewInstance();
        toChild->DeepCopy(fromChild);
        this->SetChild(cc, toChild);
        toChild->FastDelete();
        if (from->HasChildMetaData(cc))
          {
          vtkInformation* toInfo = this->GetChildMetaData(cc);
          toInfo->Copy(from->GetChildMetaData(cc), /*deep=*/1);
          }
        }
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::Initialize()
{
  this->Internals->Children.clear();
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
vtkIdType vtkDataObjectTree::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(this->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      numPts += ds->GetNumberOfPoints();
      }
    }
  iter->Delete();
  return numPts;
}


//----------------------------------------------------------------------------
unsigned long vtkDataObjectTree::GetActualMemorySize()
{
  unsigned long memSize = 0;
  vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(this->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    memSize += dobj->GetActualMemorySize();
    }
  iter->Delete();
  return memSize;
}

//----------------------------------------------------------------------------
void vtkDataObjectTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number Of Children: " << this->GetNumberOfChildren() << endl;
  for (unsigned int cc=0; cc < this->GetNumberOfChildren(); cc++)
    {
    vtkDataObject* child = this->GetChild(cc);
    if (child)
      {
      os << indent << "Child " << cc << ": " << child->GetClassName() << endl;
      child->PrintSelf(os, indent.GetNextIndent());
      }
    else
      {
      os << indent << "Child " << cc << ": NULL" << endl;
      }
    }
}
