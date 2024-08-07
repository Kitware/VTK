// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCleanArrays.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessControllerHelper.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <set>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
inline bool vtkSkipAttributeType(int attr)
{
  return (attr == vtkDataObject::POINT_THEN_CELL);
}

inline void vtkShallowCopy(vtkDataObject* output, vtkDataObject* input)
{
  vtkCompositeDataSet* cdout = vtkCompositeDataSet::SafeDownCast(output);
  if (cdout == nullptr)
  {
    output->ShallowCopy(input);
    return;
  }

  // We can't use vtkCompositeDataSet::ShallowCopy() since that simply passes
  // the leaf datasets without actually shallowcopying them. That doesn't work
  // in our case since we will be modifying the datasets in the output.
  vtkCompositeDataSet* cdin = vtkCompositeDataSet::SafeDownCast(input);
  cdout->CopyStructure(cdin);
  vtkSmartPointer<vtkCompositeDataIterator> initer;
  initer.TakeReference(cdin->NewIterator());
  for (initer->InitTraversal(); !initer->IsDoneWithTraversal(); initer->GoToNextItem())
  {
    vtkDataObject* in = initer->GetCurrentDataObject();
    vtkDataObject* clone = in->NewInstance();
    clone->ShallowCopy(in);
    cdout->SetDataSet(initer, clone);
    clone->FastDelete();
  }
}

vtkStandardNewMacro(vtkCleanArrays);
vtkCxxSetObjectMacro(vtkCleanArrays, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkCleanArrays::vtkCleanArrays()
  : Controller(nullptr)
  , FillPartialArrays(false)
  , MarkFilledPartialArrays(false)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCleanArrays::~vtkCleanArrays()
{
  this->SetController(nullptr);
}

//****************************************************************************
class vtkCleanArrays::vtkArrayData
{
public:
  std::string Name;
  int NumberOfComponents;
  int Type;
  vtkArrayData()
  {
    this->NumberOfComponents = 0;
    this->Type = 0;
  }
  vtkArrayData(const vtkArrayData& other)
  {
    this->Name = other.Name;
    this->NumberOfComponents = other.NumberOfComponents;
    this->Type = other.Type;
  }

  bool operator<(const vtkArrayData& b) const
  {
    if (this->Name != b.Name)
    {
      return this->Name < b.Name;
    }
    if (this->NumberOfComponents != b.NumberOfComponents)
    {
      return this->NumberOfComponents < b.NumberOfComponents;
    }
    return this->Type < b.Type;
  }

  void Set(vtkAbstractArray* array)
  {
    this->Name = array->GetName();
    this->NumberOfComponents = array->GetNumberOfComponents();
    this->Type = array->GetDataType();
  }

  vtkAbstractArray* NewArray(vtkIdType numTuples) const
  {
    vtkAbstractArray* array = vtkAbstractArray::CreateArray(this->Type);
    if (array)
    {
      array->SetName(this->Name.c_str());
      array->SetNumberOfComponents(this->NumberOfComponents);
      array->SetNumberOfTuples(numTuples);
      vtkDataArray* data_array = vtkDataArray::SafeDownCast(array);
      for (int cc = 0; data_array && cc < this->NumberOfComponents; cc++)
      {
        data_array->FillComponent(cc, 0.0);
      }
    }
    return array;
  }
};

class vtkCleanArrays::vtkArraySet : public std::set<vtkCleanArrays::vtkArrayData>
{
  int Valid;

public:
  vtkArraySet()
    : Valid(0)
  {
  }
  bool IsValid() const { return this->Valid != 0; }
  void MarkValid() { this->Valid = 1; }

  void Intersection(const vtkArraySet& other)
  {
    if (this->Valid && other.Valid)
    {
      vtkCleanArrays::vtkArraySet setC;
      std::set_intersection(
        this->begin(), this->end(), other.begin(), other.end(), std::inserter(setC, setC.begin()));
      setC.MarkValid();
      this->swap(setC);
    }
    else if (other.Valid)
    {
      *this = other;
    }
  }
  void Union(const vtkArraySet& other)
  {
    if (this->Valid && other.Valid)
    {
      vtkCleanArrays::vtkArraySet setC;
      std::set_union(
        this->begin(), this->end(), other.begin(), other.end(), std::inserter(setC, setC.begin()));
      setC.MarkValid();
      this->swap(setC);
    }
    else if (other.Valid)
    {
      *this = other;
    }
  }

  // Fill up \c this with arrays from \c dsa
  void Initialize(vtkFieldData* dsa)
  {
    this->Valid = true;
    int numArrays = dsa->GetNumberOfArrays();
    if (dsa->GetNumberOfTuples() == 0)
    {
      numArrays = 0;
    }
    for (int cc = 0; cc < numArrays; cc++)
    {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      if (array && array->GetName())
      {
        vtkCleanArrays::vtkArrayData mda;
        mda.Set(array);
        this->insert(mda);
      }
    }
  }

  // Remove arrays from \c dsa not present in \c this.
  void UpdateFieldData(vtkFieldData* dsa, bool add_validity_array) const
  {
    if (this->Valid == 0)
    {
      return;
    }

    std::vector<std::pair<std::string, bool>> partial_flags;
    vtkArraySet myself = (*this);
    int numArrays = dsa->GetNumberOfArrays();
    for (int cc = numArrays - 1; cc >= 0; cc--)
    {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      if (array && array->GetName())
      {
        vtkCleanArrays::vtkArrayData mda;
        mda.Set(array);
        if (myself.find(mda) == myself.end())
        {
          // cout << "Removing: " << array->GetName() << endl;
          dsa->RemoveArray(array->GetName());
        }
        else
        {
          myself.erase(mda);
          partial_flags.emplace_back(std::string(array->GetName()), true);
        }
      }
    }
    // Now fill any missing arrays.
    for (iterator iter = myself.begin(); iter != myself.end(); ++iter)
    {
      vtkAbstractArray* array = iter->NewArray(dsa->GetNumberOfTuples());
      if (array)
      {
        dsa->AddArray(array);
        array->FastDelete();
        partial_flags.emplace_back(std::string(array->GetName()), false);
      }
    }

    // let's add validity arrays if requested.
    if (add_validity_array)
    {
      for (const auto& pair : partial_flags)
      {
        auto validArrayName = "__vtkValidMask__" + pair.first;
        if (dsa->GetAbstractArray(validArrayName.c_str()))
        {
          // a valid mask array may have already been added by an earlier
          // filter such as vtkAttributeDataToTableFilter.
          continue;
        }

        if (pair.first.find("__vtkValidMask__") == 0)
        {
          // don't add a validity mask for a validity mask array added by an
          // earlier filter such as vtkAttributeDataToTableFilter.
          continue;
        }

        auto validArray = vtkUnsignedCharArray::New();
        validArray->SetName(("__vtkValidMask__" + pair.first).c_str());
        validArray->SetNumberOfTuples(dsa->GetNumberOfTuples());
        validArray->FillValue(pair.second ? 1 : 0);
        dsa->AddArray(validArray);
        validArray->FastDelete();
      }
    }
  }

  void Save(vtkMultiProcessStream& stream)
  {
    stream.Reset();
    stream << this->Valid;
    stream << static_cast<unsigned int>(this->size());
    vtkCleanArrays::vtkArraySet::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
    {
      stream << iter->Name << iter->NumberOfComponents << iter->Type;
    }
  }

  void Load(vtkMultiProcessStream& stream)
  {
    this->clear();
    unsigned int numvalues;
    stream >> this->Valid;
    stream >> numvalues;
    for (unsigned int cc = 0; cc < numvalues; cc++)
    {
      vtkCleanArrays::vtkArrayData mda;
      stream >> mda.Name >> mda.NumberOfComponents >> mda.Type;
      this->insert(mda);
    }
  }
  void Print()
  {
    vtkCleanArrays::vtkArraySet::iterator iter;
    cout << "Valid: " << this->Valid << endl;
    for (iter = this->begin(); iter != this->end(); ++iter)
    {
      cout << iter->Name << ", " << iter->NumberOfComponents << ", " << iter->Type << endl;
    }
    cout << "-----------------------------------" << endl << endl;
  }
};

//----------------------------------------------------------------------------
static void IntersectStreams(vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
  vtkCleanArrays::vtkArraySet setA;
  vtkCleanArrays::vtkArraySet setB;
  setA.Load(A);
  setB.Load(B);
  setA.Intersection(setB);
  B.Reset();
  setA.Save(B);
}

//----------------------------------------------------------------------------
static void UnionStreams(vtkMultiProcessStream& A, vtkMultiProcessStream& B)
{
  vtkCleanArrays::vtkArraySet setA;
  vtkCleanArrays::vtkArraySet setB;
  setA.Load(A);
  setB.Load(B);
  setA.Union(setB);
  B.Reset();
  setA.Save(B);
}

//----------------------------------------------------------------------------
int vtkCleanArrays::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  vtkShallowCopy(outputDO, inputDO);
  vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(outputDO);

  vtkMultiProcessController* controller = this->Controller;
  if ((!controller || controller->GetNumberOfProcesses() <= 1) && outputCD == nullptr)
  {
    // Nothing to do since not running in parallel or on composite datasets.
    return 1;
  }

  // Build the array sets for all attribute types across all blocks (if any).
  vtkCleanArrays::vtkArraySet arraySets[vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  if (outputCD)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(outputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
      {
        if (vtkSkipAttributeType(attr))
        {
          continue;
        }
        if (dobj->GetNumberOfElements(attr) > 0)
        {
          vtkCleanArrays::vtkArraySet myset;
          myset.Initialize(dobj->GetAttributesAsFieldData(attr));
          if (this->FillPartialArrays)
          {
            arraySets[attr].Union(myset);
          }
          else
          {
            arraySets[attr].Intersection(myset);
          }
        }
      }
    }
  }
  else
  {
    for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
    {
      if (vtkSkipAttributeType(attr))
      {
        continue;
      }
      if (outputDO->GetNumberOfElements(attr) > 0)
      {
        arraySets[attr].Initialize(outputDO->GetAttributesAsFieldData(attr));
      }
    }
  }

  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
    {
      if (vtkSkipAttributeType(attr))
      {
        continue;
      }
      vtkMultiProcessStream mstream;
      arraySets[attr].Save(mstream);
      vtkMultiProcessControllerHelper::ReduceToAll(controller, mstream,
        this->FillPartialArrays ? ::UnionStreams : ::IntersectStreams, 1278392 + attr);
      arraySets[attr].Load(mstream);
    }
  }

  if (outputCD)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(outputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
      {
        if (vtkSkipAttributeType(attr))
        {
          continue;
        }
        arraySets[attr].UpdateFieldData(
          dobj->GetAttributesAsFieldData(attr), this->MarkFilledPartialArrays);
      }
    }
  }
  else
  {
    for (int attr = 0; attr < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
    {
      if (vtkSkipAttributeType(attr))
      {
        continue;
      }
      arraySets[attr].UpdateFieldData(
        outputDO->GetAttributesAsFieldData(attr), this->MarkFilledPartialArrays);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCleanArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FillPartialArrays: " << this->FillPartialArrays << endl;
  os << indent << "Controller: " << this->Controller << endl;
}
VTK_ABI_NAMESPACE_END
