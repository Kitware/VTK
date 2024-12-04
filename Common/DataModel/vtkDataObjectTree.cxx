// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataObjectTree.h"

#include "vtkCellGrid.h"
#include "vtkDataObjectTreeInternals.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkLegacy.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRDataIterator.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkDataObjectTree::vtkDataObjectTree()
{
  this->Internals = new vtkDataObjectTreeInternals;
}

//------------------------------------------------------------------------------
vtkDataObjectTree::~vtkDataObjectTree()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkDataObjectTree::GetData(vtkInformation* info)
{
  return info ? vtkDataObjectTree::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkDataObjectTree::GetData(vtkInformationVector* v, int i)
{
  return vtkDataObjectTree::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::SetNumberOfChildren(unsigned int num)
{
  this->Internals->Children.resize(num);
  this->Modified();
}

//------------------------------------------------------------------------------
unsigned int vtkDataObjectTree::GetNumberOfChildren()
{
  return static_cast<unsigned int>(this->Internals->Children.size());
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::SetChild(unsigned int index, vtkDataObject* dobj)
{
  if (this->Internals->Children.size() <= index)
  {
    this->SetNumberOfChildren(index + 1);
  }

  vtkDataObjectTreeItem& item = this->Internals->Children[index];
  if (item.DataObject != dobj)
  {
    item.DataObject = dobj;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::RemoveChild(unsigned int index)
{
  if (this->Internals->Children.size() <= index)
  {
    vtkErrorMacro("The input index is out of range.");
    return;
  }

  vtkDataObjectTreeItem& item = this->Internals->Children[index];
  item.DataObject = nullptr;
  this->Internals->Children.erase(this->Internals->Children.begin() + index);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTree::GetChild(unsigned int index)
{
  if (index < this->Internals->Children.size())
  {
    return this->Internals->Children[index].DataObject;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
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
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::SetChildMetaData(unsigned int index, vtkInformation* info)
{
  if (this->Internals->Children.size() <= index)
  {
    this->SetNumberOfChildren(index + 1);
  }

  vtkDataObjectTreeItem& item = this->Internals->Children[index];
  item.MetaData = info;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkDataObjectTree::HasChildMetaData(unsigned int index)
{
  if (index < this->Internals->Children.size())
  {
    vtkDataObjectTreeItem& item = this->Internals->Children[index];
    return (item.MetaData != nullptr) ? 1 : 0;
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::CopyStructure(vtkCompositeDataSet* compositeSource)
{
  if (!compositeSource)
  {
    return;
  }
  auto dObjTree = vtkDataObjectTree::SafeDownCast(compositeSource);
  if (dObjTree == this)
  {
    return;
  }

  this->Superclass::CopyStructure(compositeSource);
  this->Internals->Children.clear();

  if (dObjTree)
  {
    this->Internals->Children.resize(dObjTree->Internals->Children.size());

    for (auto dObjTreeIter = dObjTree->Internals->Children.begin(),
              thisIter = this->Internals->Children.begin();
         dObjTreeIter != dObjTree->Internals->Children.end(); ++dObjTreeIter, ++thisIter)
    {
      vtkDataObjectTree* subDObjTree = vtkDataObjectTree::SafeDownCast(dObjTreeIter->DataObject);
      if (subDObjTree)
      {
        if (vtkDataObjectTree* copy = this->CreateForCopyStructure(subDObjTree))
        {
          thisIter->DataObject.TakeReference(copy);
          copy->CopyStructure(subDObjTree);
        }
        else
        {
          vtkErrorMacro("CopyStructure has encountered an error and will fail!");
        }
      }

      // shallow copy meta data.
      if (dObjTreeIter->MetaData)
      {
        vtkNew<vtkInformation> info;
        info->Copy(dObjTreeIter->MetaData, /*deep=*/0);
        thisIter->MetaData = info;
      }
    }
  }
  else if (auto amr = vtkUniformGridAMR::SafeDownCast(compositeSource))
  {
    this->SetNumberOfChildren(amr->GetNumberOfLevels());
    vtkNew<vtkPartitionedDataSet> tempPds;
    for (unsigned int level = 0; level < amr->GetNumberOfLevels(); level++)
    {
      auto child = vtk::TakeSmartPointer(this->CreateForCopyStructure(tempPds));
      child->SetNumberOfChildren(amr->GetNumberOfDataSets(level));
      this->SetChild(level, child);

      vtkNew<vtkInformation> info;
      info->Set(vtkCompositeDataSet::NAME(), "Level " + std::to_string(level));
      this->SetChildMetaData(level, info);
    }
  }
  else
  {
    // WARNING:
    // If we copy the structure from a non-tree composite data set
    // we create a special structure of two levels, the first level
    // is just a single partitioned dataSet and the second level are all the data sets.
    // This is likely to change in the future!
    vtkNew<vtkPartitionedDataSet> tempPds;
    auto pds = vtk::TakeSmartPointer(this->CreateForCopyStructure(tempPds));
    this->SetChild(0, pds);

    vtkNew<vtkInformation> info;
    info->Set(vtkCompositeDataSet::NAME(), "All Blocks");
    this->SetChildMetaData(0, info);

    int totalNumBlocks = 0;
    auto iter = vtk::TakeSmartPointer(compositeSource->NewIterator());
    iter->SkipEmptyNodesOff();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      ++totalNumBlocks;
    }

    pds->SetNumberOfChildren(totalNumBlocks);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkDataObjectTree::CreateForCopyStructure(vtkDataObjectTree* other)
{
  return other ? other->NewInstance() : nullptr;
}

//------------------------------------------------------------------------------
vtkDataObjectTreeIterator* vtkDataObjectTree::NewTreeIterator()
{
  vtkDataObjectTreeIterator* dObjTreeIter = vtkDataObjectTreeIterator::New();
  dObjTreeIter->SetDataSet(this);
  return dObjTreeIter;
}

//------------------------------------------------------------------------------
vtkCompositeDataIterator* vtkDataObjectTree::NewIterator()
{
  return this->NewTreeIterator();
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj)
{
  if (auto dObjTreeIter = vtkDataObjectTreeIterator::SafeDownCast(iter))
  {
    this->SetDataSetFrom(dObjTreeIter, dataObj);
    return;
  }

  if (!iter || iter->IsDoneWithTraversal())
  {
    vtkErrorMacro("Invalid iterator location.");
    return;
  }

  auto uniformGridAMRIter = vtkUniformGridAMRDataIterator::SafeDownCast(iter);
  if (uniformGridAMRIter)
  {
    if (uniformGridAMRIter->GetCurrentLevel() < this->GetNumberOfChildren())
    {
      auto parent =
        vtkDataObjectTree::SafeDownCast(this->GetChild(uniformGridAMRIter->GetCurrentLevel()));
      if (parent)
      {
        parent->SetChild(uniformGridAMRIter->GetCurrentIndex(), dataObj);
      }
      else
      {
        vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      }
    }
    else
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
    }
  }
  else
  {
    // WARNING: We are doing something special here. See comments
    // in CopyStructure()
    unsigned int index = iter->GetCurrentFlatIndex();
    if (this->GetNumberOfChildren() != 1)
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return;
    }
    auto parent = vtkPartitionedDataSet::SafeDownCast(this->GetChild(0));
    if (!parent)
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return;
    }
    parent->SetChild(index, dataObj);
  }
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::SetDataSetFrom(
  vtkDataObjectTreeIterator* dObjTreeIter, vtkDataObject* dataObj)
{
  if (!dObjTreeIter || dObjTreeIter->IsDoneWithTraversal())
  {
    vtkErrorMacro("Invalid iterator location.");
    return;
  }

  vtkDataObjectTreeIndex index = dObjTreeIter->GetCurrentIndex();

  if (index.empty())
  {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return;
  }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc = 0; cc < numIndices - 1; cc++)
  {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return;
    }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
  }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
  {
    vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
    return;
  }

  parent->SetChild(index.back(), dataObj);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkDataObjectTree::GetDataSet(vtkCompositeDataIterator* compositeIter)
{
  if (!compositeIter || compositeIter->IsDoneWithTraversal())
  {
    vtkErrorMacro("Invalid iterator location.");
    return nullptr;
  }

  if (auto dObjTreeIter = vtkDataObjectTreeIterator::SafeDownCast(compositeIter))
  {
    vtkDataObjectTreeIndex index = dObjTreeIter->GetCurrentIndex();

    if (index.empty())
    {
      // Sanity check.
      vtkErrorMacro("Invalid index returned by iterator.");
      return nullptr;
    }

    vtkDataObjectTree* parent = this;
    int numIndices = static_cast<int>(index.size());
    for (int cc = 0; cc < numIndices - 1; cc++)
    {
      if (!parent || parent->GetNumberOfChildren() <= index[cc])
      {
        vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
        return nullptr;
      }
      parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
    }

    if (!parent || parent->GetNumberOfChildren() <= index.back())
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return nullptr;
    }

    return parent->GetChild(index.back());
  }
  else if (auto amrIter = vtkUniformGridAMRDataIterator::SafeDownCast(compositeIter))
  {
    if (amrIter->GetCurrentLevel() < this->GetNumberOfChildren())
    {
      auto parent = vtkDataObjectTree::SafeDownCast(this->GetChild(amrIter->GetCurrentLevel()));
      if (parent)
      {
        return parent->GetChild(amrIter->GetCurrentIndex());
      }
      else
      {
        vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
        return nullptr;
      }
    }
    else
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return nullptr;
    }
  }
  else
  {
    // WARNING: We are doing something special here. See comments
    // in CopyStructure()
    // To do: More clear check of structures here. At least something like this->Depth()==1
    unsigned int currentFlatIndex = compositeIter->GetCurrentFlatIndex();

    if (this->GetNumberOfChildren() != 1)
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return nullptr;
    }
    auto parent = vtkPartitionedDataSet::SafeDownCast(this->GetChild(0));
    if (!parent)
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return nullptr;
    }

    if (currentFlatIndex < parent->GetNumberOfChildren())
    {
      return parent->GetChild(currentFlatIndex);
    }
    else
    {
      return nullptr;
    }
  }
}

//------------------------------------------------------------------------------
vtkInformation* vtkDataObjectTree::GetMetaData(vtkCompositeDataIterator* compositeIter)
{
  auto dObjTreeIter = vtkDataObjectTreeIterator::SafeDownCast(compositeIter);
  if (!dObjTreeIter || dObjTreeIter->IsDoneWithTraversal())
  {
    vtkErrorMacro("Invalid iterator location.");
    return nullptr;
  }

  vtkDataObjectTreeIndex index = dObjTreeIter->GetCurrentIndex();

  if (index.empty())
  {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return nullptr;
  }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc = 0; cc < numIndices - 1; cc++)
  {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return nullptr;
    }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
  }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
  {
    vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
    return nullptr;
  }

  return parent->GetChildMetaData(index.back());
}

//------------------------------------------------------------------------------
vtkTypeBool vtkDataObjectTree::HasMetaData(vtkCompositeDataIterator* compositeIter)
{
  auto dObjTreeIter = vtkDataObjectTreeIterator::SafeDownCast(compositeIter);
  if (!dObjTreeIter || dObjTreeIter->IsDoneWithTraversal())
  {
    vtkErrorMacro("Invalid iterator location.");
    return 0;
  }

  vtkDataObjectTreeIndex index = dObjTreeIter->GetCurrentIndex();

  if (index.empty())
  {
    // Sanity check.
    vtkErrorMacro("Invalid index returned by iterator.");
    return 0;
  }

  vtkDataObjectTree* parent = this;
  int numIndices = static_cast<int>(index.size());
  for (int cc = 0; cc < numIndices - 1; cc++)
  {
    if (!parent || parent->GetNumberOfChildren() <= index[cc])
    {
      vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
      return 0;
    }
    parent = vtkDataObjectTree::SafeDownCast(parent->GetChild(index[cc]));
  }

  if (!parent || parent->GetNumberOfChildren() <= index.back())
  {
    vtkErrorMacro("Structure is not expected. Did you forget to use copy structure?");
    return 0;
  }

  return parent->HasChildMetaData(index.back());
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::CompositeShallowCopy(vtkCompositeDataSet* src)
{
  if (src == this)
  {
    return;
  }

  this->Internals->Children.clear();
  this->Superclass::CompositeShallowCopy(src);

  if (auto from = vtkDataObjectTree::SafeDownCast(src))
  {
    unsigned int numChildren = from->GetNumberOfChildren();
    this->SetNumberOfChildren(numChildren);
    for (unsigned int cc = 0; cc < numChildren; cc++)
    {
      if (auto child = from->GetChild(cc))
      {
        if (auto childTree = vtkDataObjectTree::SafeDownCast(child))
        {
          vtkDataObjectTree* clone = childTree->NewInstance();
          clone->CompositeShallowCopy(childTree);
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
        toInfo->Copy(from->GetChildMetaData(cc), 0);
      }
    }
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::DeepCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }

  this->Internals->Children.clear();
  this->Superclass::DeepCopy(src);

  if (auto from = vtkDataObjectTree::SafeDownCast(src))
  {
    unsigned int numChildren = from->GetNumberOfChildren();
    this->SetNumberOfChildren(numChildren);
    for (unsigned int cc = 0; cc < numChildren; cc++)
    {
      if (auto fromChild = from->GetChild(cc))
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

//------------------------------------------------------------------------------
void vtkDataObjectTree::ShallowCopy(vtkDataObject* src)
{
  if (src == this)
  {
    return;
  }

  this->Internals->Children.clear();
  this->Superclass::ShallowCopy(src);

  if (auto from = vtkDataObjectTree::SafeDownCast(src))
  {
    unsigned int numChildren = from->GetNumberOfChildren();
    this->SetNumberOfChildren(numChildren);
    for (unsigned int cc = 0; cc < numChildren; cc++)
    {
      if (auto child = from->GetChild(cc))
      {
        auto clone = child->NewInstance();
        clone->ShallowCopy(child);
        this->SetChild(cc, clone);
        clone->FastDelete();
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

//------------------------------------------------------------------------------
void vtkDataObjectTree::Initialize()
{
  this->Internals->Children.clear();
  this->Superclass::Initialize();
}

//------------------------------------------------------------------------------
vtkIdType vtkDataObjectTree::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  auto dObjTreeIter = vtk::TakeSmartPointer(this->NewIterator());
  for (dObjTreeIter->InitTraversal(); !dObjTreeIter->IsDoneWithTraversal();
       dObjTreeIter->GoToNextItem())
  {
    if (auto ds = vtkDataSet::SafeDownCast(dObjTreeIter->GetCurrentDataObject()))
    {
      numPts += ds->GetNumberOfPoints();
    }
  }
  return numPts;
}

//------------------------------------------------------------------------------
vtkIdType vtkDataObjectTree::GetNumberOfCells()
{
  vtkIdType numCells = 0;
  auto dObjTreeIter = vtk::TakeSmartPointer(this->NewIterator());
  for (dObjTreeIter->InitTraversal(); !dObjTreeIter->IsDoneWithTraversal();
       dObjTreeIter->GoToNextItem())
  {
    if (auto ds = vtkDataSet::SafeDownCast(dObjTreeIter->GetCurrentDataObject()))
    {
      numCells += ds->GetNumberOfCells();
    }
    else if (auto cg = vtkCellGrid::SafeDownCast(dObjTreeIter->GetCurrentDataObject()))
    {
      numCells += cg->GetNumberOfCells();
    }
  }
  return numCells;
}

//------------------------------------------------------------------------------
unsigned long vtkDataObjectTree::GetActualMemorySize()
{
  unsigned long memSize = 0;
  auto dObjTreeIter = vtk::TakeSmartPointer(this->NewIterator());
  for (dObjTreeIter->InitTraversal(); !dObjTreeIter->IsDoneWithTraversal();
       dObjTreeIter->GoToNextItem())
  {
    vtkDataObject* dobj = dObjTreeIter->GetCurrentDataObject();
    memSize += dobj->GetActualMemorySize();
  }
  return memSize;
}

//------------------------------------------------------------------------------
void vtkDataObjectTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number Of Children: " << this->GetNumberOfChildren() << endl;
  for (unsigned int cc = 0; cc < this->GetNumberOfChildren(); cc++)
  {
    const char* name = (this->HasChildMetaData(cc) && this->GetChildMetaData(cc)->Has(NAME()))
      ? this->GetChildMetaData(cc)->Get(NAME())
      : nullptr;

    vtkDataObject* child = this->GetChild(cc);
    if (child)
    {
      os << indent << "Child " << cc << ": " << child->GetClassName() << endl;
      os << indent << "Name: " << (name ? name : "(nullptr)") << endl;
      child->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << indent << "Child " << cc << ": nullptr" << endl;
      os << indent << "Name: " << (name ? name : "(nullptr)") << endl;
    }
  }
}
VTK_ABI_NAMESPACE_END
