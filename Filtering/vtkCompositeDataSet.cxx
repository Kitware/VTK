/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataSet.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSetInternals.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTrivialProducer.h"

vtkInformationKeyMacro(vtkCompositeDataSet, NAME, String);
//----------------------------------------------------------------------------
vtkCompositeDataSet::vtkCompositeDataSet()
{
  this->Internals = new vtkCompositeDataSetInternals;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet::~vtkCompositeDataSet()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkCompositeDataSet::GetProducerPort()
{  
  // Make sure there is an executive.
  if(!this->GetExecutive())
    {
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    vtkCompositeDataPipeline* exec = vtkCompositeDataPipeline::New();
    tp->SetExecutive(exec);
    vtkInformation* portInfo = 
      tp->GetOutputPortInformation(0);
    portInfo->Set(vtkDataObject::DATA_TYPE_NAME(), this->GetClassName());
    exec->Delete();
    tp->SetOutput(this);
    tp->Delete();
    }

  // Get the port from the executive.
  return this->GetExecutive()->GetProducerPort(this);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformation* info)
{
  return info? vtkCompositeDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkCompositeDataSet::GetData(vtkInformationVector* v,
                                                  int i)
{
  return vtkCompositeDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::SetNumberOfChildren(unsigned int num)
{
  this->Internals->Children.resize(num);
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkCompositeDataSet::GetNumberOfChildren()
{
  return static_cast<unsigned int>(this->Internals->Children.size());
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::SetChild(unsigned int index, vtkDataObject* dobj)
{
  if (this->Internals->Children.size() <= index)
    {
    this->SetNumberOfChildren(index+1);
    }

  vtkCompositeDataSetItem& item = this->Internals->Children[index];
  item.DataObject = dobj;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::RemoveChild(unsigned int index)
{
  if (this->Internals->Children.size() <= index)
    {
    vtkErrorMacro("The input index is out of range.");
    return;
    }

  vtkCompositeDataSetItem& item = this->Internals->Children[index];
  item.DataObject = NULL;
  this->Internals->Children.erase(this->Internals->Children.begin()+index);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeDataSet::GetChild(unsigned int index)
{
  if (index < this->Internals->Children.size())
    {
    return this->Internals->Children[index].DataObject;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkInformation* vtkCompositeDataSet::GetChildMetaData(unsigned int index)
{
  if (index < this->Internals->Children.size())
    {
    vtkCompositeDataSetItem& item = this->Internals->Children[index];
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
void vtkCompositeDataSet::SetChildMetaData(unsigned int index, vtkInformation* info)
{
  if (this->Internals->Children.size() <= index)
    {
    this->SetNumberOfChildren(index+1);
    }

  vtkCompositeDataSetItem& item = this->Internals->Children[index];
  item.MetaData = info;
}

//----------------------------------------------------------------------------
int vtkCompositeDataSet::HasChildMetaData(unsigned int index)
{
  if (index < this->Internals->Children.size())
    {
    vtkCompositeDataSetItem& item = this->Internals->Children[index];
    return (item.MetaData.GetPointer() != NULL)? 1 : 0;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::CopyStructure(vtkCompositeDataSet* source)
{
  if (source == this)
    {
    return;
    }

  this->Internals->Children.clear();
  if (!source)
    {
    return;
    }

  this->Internals->Children.resize(source->Internals->Children.size());
  
  vtkCompositeDataSetInternals::Iterator srcIter =
    source->Internals->Children.begin();
  vtkCompositeDataSetInternals::Iterator myIter =
    this->Internals->Children.begin();
  for (; srcIter != source->Internals->Children.end(); ++srcIter, myIter++)
    {
    vtkCompositeDataSet* compositeSrc = 
      vtkCompositeDataSet::SafeDownCast(srcIter->DataObject);
    if (compositeSrc)
      {
      vtkCompositeDataSet* copy = compositeSrc->NewInstance();
      myIter->DataObject.TakeReference(copy);
      copy->CopyStructure(compositeSrc);
      }

    // shallow copy meta data.
    if (srcIter->MetaData)
      {
      vtkInformation* info = vtkInformation::New();
      info->Copy(srcIter->MetaData, /*deep=*/0);
      myIter->MetaData = info;
      info->Delete();
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkCompositeDataSet::NewIterator()
{
  vtkCompositeDataIterator* iter = vtkCompositeDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::SetDataSet(vtkCompositeDataIterator* iter, 
  vtkDataObject* dataObj)
{
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return;
    }

  vtkCompositeDataSetIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return;
    }

  vtkCompositeDataSet* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return;
      }
    parent = vtkCompositeDataSet::SafeDownCast(parent->GetChild(index[cc]));
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
vtkDataObject* vtkCompositeDataSet::GetDataSet(vtkCompositeDataIterator* iter)
{
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
    }

  vtkCompositeDataSetIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
    }

  vtkCompositeDataSet* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return 0;
      }
    parent = vtkCompositeDataSet::SafeDownCast(parent->GetChild(index[cc]));
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
vtkInformation* vtkCompositeDataSet::GetMetaData(vtkCompositeDataIterator* iter)
{
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
    }

  vtkCompositeDataSetIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
    }

  vtkCompositeDataSet* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return 0;
      }
    parent = vtkCompositeDataSet::SafeDownCast(parent->GetChild(index[cc]));
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
int vtkCompositeDataSet::HasMetaData(vtkCompositeDataIterator* iter)
{
  if (!iter || iter->IsDoneWithTraversal())
    {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
    }

  vtkCompositeDataSetIndex index = iter->GetCurrentIndex();

  if (index.size() == 0)
    {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
    }

  vtkCompositeDataSet* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc=0; cc < numIndices-1; cc++)
    {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
      vtkErrorMacro("Structure does not match. "
        "You must use CopyStructure before calling this method.");
      return 0;
      }
    parent = vtkCompositeDataSet::SafeDownCast(parent->GetChild(index[cc]));
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
void vtkCompositeDataSet::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
    {
    return;
    }

  this->Internals->Children.clear();
  this->Superclass::ShallowCopy(src);

  vtkCompositeDataSet* from = vtkCompositeDataSet::SafeDownCast(src);
  if (from)
    {
    unsigned int numChildren = from->GetNumberOfChildren();
    this->SetNumberOfChildren(numChildren);
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkDataObject* child = from->GetChild(cc);
      if (child)
        {
        vtkDataObject* clone = child->NewInstance();
        clone->ShallowCopy(child);
        this->SetChild(cc, clone);
        clone->Delete();
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
void vtkCompositeDataSet::DeepCopy(vtkDataObject* src)
{
  if (src == this)
    {
    return;
    }

  this->Internals->Children.clear();
  this->Superclass::DeepCopy(src);

  vtkCompositeDataSet* from = vtkCompositeDataSet::SafeDownCast(src);
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
        toChild->Delete();
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
void vtkCompositeDataSet::Initialize()
{
  this->Internals->Children.clear();
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositeDataSet::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  vtkCompositeDataIterator* iter = this->NewIterator();
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
void vtkCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
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

