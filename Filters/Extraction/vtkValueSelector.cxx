/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValueSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkValueSelector.h"

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSortDataArray.h"
#include "vtkStringArray.h"

#include <cassert>
#include <type_traits>
namespace
{

struct ThresholdSelectionListReshaper
{
protected:
  vtkDataArray* FixedArray;
public:
  ThresholdSelectionListReshaper(vtkDataArray* toFill)
    : FixedArray(toFill)
  {
  }
  // If the input selection list for a threshold has one component we need
  // to reshape it into an array with two component tuples (ranges) so it
  // is interpreted correctly later.
  template<typename SelectionListArrayType>
  void operator()(SelectionListArrayType* originalList)
  {
    // created with NewInstance from the originalList, we know it is the same type
    auto fixedList = SelectionListArrayType::FastDownCast(this->FixedArray);
    assert(originalList->GetNumberOfComponents() == 1);
    assert(fixedList->GetNumberOfComponents() == 2);

    vtkDataArrayAccessor<SelectionListArrayType> originalAccessor(originalList);
    vtkDataArrayAccessor<SelectionListArrayType> fixedAccessor(fixedList);

    for (vtkIdType i = 0; i < fixedList->GetNumberOfTuples(); ++i)
    {
      fixedAccessor.Set(i, 0, originalAccessor.Get(2 * i, 0));
      fixedAccessor.Set(i, 1, originalAccessor.Get(2 * i + 1, 0));
    }
  }
};

//----------------------------------------------------------------------------
// This is used for the cases where the SelectionList is a 1-component array,
// implying that the values are exact matches.
struct ArrayValueMatchFunctor
{
  vtkSignedCharArray* InsidenessArray;
  int ComponentNo;

  ArrayValueMatchFunctor(vtkSignedCharArray* insidenessArray, int comp)
    : InsidenessArray(insidenessArray)
    , ComponentNo(comp)
  {
  }

  // this is used for selecting entries where the field array has matching
  // values.
  template <typename InputArrayType, typename SelectionListArrayType>
  void operator()(InputArrayType* fArray, SelectionListArrayType* selList)
  {
    assert(selList->GetNumberOfComponents() == 1);
    assert(fArray->GetNumberOfComponents() > this->ComponentNo);

    using ValueType = typename vtkDataArrayAccessor<SelectionListArrayType>::APIType;
    vtkDataArrayAccessor<InputArrayType> faccessor(fArray);

    static_assert(std::is_same<ValueType,
                    typename vtkDataArrayAccessor<SelectionListArrayType>::APIType>::value,
      "value types mismatched!");

    const ValueType* haystack_begin = selList->GetPointer(0);
    const ValueType* haystack_end = haystack_begin + selList->GetNumberOfValues();
    const int comp = fArray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;

    vtkSignedCharArray* insidenessArray = this->InsidenessArray;
    assert(insidenessArray->GetNumberOfTuples() == fArray->GetNumberOfTuples());
    if (comp >= 0)
    {
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        for (vtkIdType cc = begin; cc < end; ++cc)
        {
          auto val = faccessor.Get(cc, comp);
          insidenessArray->SetValue(
            cc, std::binary_search(haystack_begin, haystack_end, val) ? 1 : 0);
        }
      });
    }
    else
    {
      const int num_components = fArray->GetNumberOfComponents();

      // compare vector magnitude.
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        for (vtkIdType cc = begin; cc < end; ++cc)
        {
          ValueType val = ValueType(0);
          for (int kk = 0; kk < num_components; ++kk)
          {
            const auto valKK = faccessor.Get(cc, comp);
            val += valKK * valKK;
          }
          const auto magnitude = static_cast<ValueType>(std::sqrt(val));
          insidenessArray->SetValue(
            cc, std::binary_search(haystack_begin, haystack_end, magnitude) ? 1 : 0);
        }
      });
    }
  }

  // this is used to select indices
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* selList)
  {
    assert(selList->GetNumberOfComponents() == 1);

    this->InsidenessArray->FillValue(0);

    const vtkIdType numDataValues = this->InsidenessArray->GetNumberOfTuples();
    const vtkIdType numSelList = selList->GetNumberOfTuples();
    vtkDataArrayAccessor<SelectionListArrayType> selListAccessor(selList);
    for (vtkIdType cc = 0; cc < numSelList; ++cc)
    {
      auto cid = static_cast<vtkIdType>(selListAccessor.Get(cc, 0));
      if (cid >= 0 && cid < numDataValues)
      {
        this->InsidenessArray->SetValue(cid, 1);
      }
    }
  }
};

//----------------------------------------------------------------------------
// This is used for the cases where the SelectionList is a 2-component array,
// implying that the values are ranges.
struct ArrayValueRangeFunctor
{
  vtkSignedCharArray* InsidenessArray;
  int ComponentNo;

  ArrayValueRangeFunctor(vtkSignedCharArray* insidenessArray, int comp)
    : InsidenessArray(insidenessArray)
    , ComponentNo(comp)
  {
  }

  // for selecting using field array values
  template <typename InputArrayType, typename SelectionListArrayType>
  void operator()(InputArrayType* fArray, SelectionListArrayType* selList)
  {
    assert(selList->GetNumberOfComponents() == 2);
    assert(fArray->GetNumberOfComponents() > this->ComponentNo);
    assert(this->InsidenessArray->GetNumberOfTuples() == fArray->GetNumberOfTuples());

    using ValueType = typename vtkDataArrayAccessor<SelectionListArrayType>::APIType;
    vtkDataArrayAccessor<InputArrayType> fAccessor(fArray);

    static_assert(std::is_same<ValueType,
                    typename vtkDataArrayAccessor<SelectionListArrayType>::APIType>::value,
      "value types mismatched!");

    vtkDataArrayAccessor<SelectionListArrayType> rangeAccessor(selList);
    const int comp = fArray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;
    const vtkIdType numRanges = selList->GetNumberOfTuples();

    if (comp >= 0)
    {
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        for (vtkIdType cc = begin; cc < end; ++cc)
        {
          const auto val = fAccessor.Get(cc, comp);
          bool match = false;
          for (vtkIdType r = 0; r < numRanges && !match; ++r)
          {
            match = (val >= rangeAccessor.Get(r, 0) && val <= rangeAccessor.Get(r, 1));
          }
          this->InsidenessArray->SetValue(cc, match ? 1 : 0);
        }
      });
    }
    else
    {
      const int num_components = fArray->GetNumberOfComponents();

      // compare vector magnitude.
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        for (vtkIdType cc = begin; cc < end; ++cc)
        {
          ValueType val = ValueType(0);
          for (int kk = 0; kk < num_components; ++kk)
          {
            const auto valKK = fAccessor.Get(cc, comp);
            val += valKK * valKK;
          }
          const auto magnitude = static_cast<ValueType>(std::sqrt(val));
          bool match = false;
          for (vtkIdType r = 0; r < numRanges && !match; ++r)
          {
            match = (magnitude >= rangeAccessor.Get(r, 0) && magnitude <= rangeAccessor.Get(r, 1));
          }
          this->InsidenessArray->SetValue(cc, match ? 1 : 0);
        }
      });
    }
  }

  // this is used to select indices
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* selList)
  {
    assert(selList->GetNumberOfComponents() == 2);

    vtkDataArrayAccessor<SelectionListArrayType> rangeAccessor(selList);

    const vtkIdType numValues = this->InsidenessArray->GetNumberOfTuples();
    const vtkIdType numRanges = selList->GetNumberOfTuples();

    this->InsidenessArray->FillValue(0);
    for (vtkIdType cc = 0; cc < numRanges; ++cc)
    {
      vtkIdType start = std::min(static_cast<vtkIdType>(rangeAccessor.Get(cc, 0)), numValues - 1);
      vtkIdType last = std::min(static_cast<vtkIdType>(rangeAccessor.Get(cc, 1)), numValues - 1);
      if (start >= 0 && last >= start)
      {
        std::fill_n(this->InsidenessArray->GetPointer(start), (start - last) + 1, 1);
      }
    }
  }
};
}

//----------------------------------------------------------------------------
class vtkValueSelector::vtkInternals
{
  vtkSmartPointer<vtkAbstractArray> SelectionList;
  std::string FieldName;
  int FieldAssociation;
  int FieldAttributeType;
  int ComponentNo;

public:
  // use this constructor when selection is specified as (assoc, name)
  vtkInternals(vtkAbstractArray* selList, int assoc, const std::string& aname, int comp)
    : vtkInternals(comp, selList)
  {
    this->FieldName = aname;
    this->FieldAssociation = assoc;

    if (assoc < 0 || assoc >= vtkDataObject::NUMBER_OF_ASSOCIATIONS ||
      assoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      throw std::runtime_error("unsupported association");
    }
  }

  // use this constructor when selection is specified as (assoc, attribute type)
  vtkInternals(vtkAbstractArray* selList, int assoc, int attr, int comp)
    : vtkInternals(comp, selList)
  {
    this->FieldAssociation = assoc;
    this->FieldAttributeType = attr;

    if (assoc < 0 || assoc >= vtkDataObject::NUMBER_OF_ASSOCIATIONS ||
      assoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      throw std::runtime_error("unsupported association");
    }

    if (attr < 0 || attr >= vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
      throw std::runtime_error("unsupported attribute type");
    }
  }

  // use this constructor when selection is for ids of element type = assoc.
  vtkInternals(vtkAbstractArray* selList, int assoc)
    : vtkInternals(0, selList)
  {
    this->FieldAssociation = assoc;

    if (assoc < 0 || assoc >= vtkDataObject::NUMBER_OF_ASSOCIATIONS ||
      assoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      throw std::runtime_error("unsupported association");
    }
  }

  // returns false on any failure or unhandled case.
  bool Execute(vtkDataObject* dobj, vtkSignedCharArray* darray);

private:
  vtkInternals(int comp, vtkAbstractArray* selList)
    : SelectionList()
    , FieldName()
    , FieldAssociation(-1)
    , FieldAttributeType(-1)
    , ComponentNo(comp)
  {
    if (selList->GetNumberOfComponents() != 1 && selList->GetNumberOfComponents() != 2)
    {
      // 1-component == exact value match
      // 2-component == values in range specified by each tuple.
      throw std::runtime_error("Currently, selecting multi-components arrays is not supported.");
    }

    if (selList->GetNumberOfComponents() == 1)
    {
      // we sort the selection list to speed up extraction later.
      this->SelectionList.TakeReference(selList->NewInstance());
      this->SelectionList->DeepCopy(selList);
      vtkSortDataArray::Sort(this->SelectionList);
    }
    else
    {
      // don't bother sorting.
      this->SelectionList = selList;
    }
  }

  bool Execute(vtkAbstractArray* darray, vtkSignedCharArray* insidenessArray)
  {
    if (vtkStringArray::SafeDownCast(darray))
    {
      // this will be added later, if needed.
      vtkGenericWarningMacro(<< darray->GetClassName()
                             << " currently not supported by vtkValueSelector.");
      return false;
    }
    else if (auto dataArray = vtkDataArray::SafeDownCast(darray))
    {
      return this->Execute(dataArray, insidenessArray);
    }
    else
    {
      vtkGenericWarningMacro(<< darray->GetClassName() << " not supported by vtkValueSelector.");
      return false;
    }
  }

  bool Execute(vtkDataArray* darray, vtkSignedCharArray* insidenessArray)
  {
    assert(vtkDataArray::SafeDownCast(this->SelectionList));
    if (darray->GetNumberOfComponents() < this->ComponentNo)
    {
      // array doesn't have request components. nothing to select.
      return false;
    }

    if (this->SelectionList->GetNumberOfComponents() == 1)
    {
      ArrayValueMatchFunctor worker(insidenessArray, this->ComponentNo);
      if (!vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>::Execute(
            darray, vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro("Mismatch in selection list and field array.");
        return false;
      }
    }
    else
    {
      ArrayValueRangeFunctor worker(insidenessArray, this->ComponentNo);
      if (!vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>::Execute(
            darray, vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro("Mismatch in selection list and field array.");
        return false;
      }
    }

    insidenessArray->Modified();
    return true;
  }

  // this is used for when selecting elements by ids
  bool Execute(vtkSignedCharArray* insidenessArray)
  {
    assert(vtkDataArray::SafeDownCast(this->SelectionList));

    if (this->SelectionList->GetNumberOfComponents() == 1)
    {
      ArrayValueMatchFunctor worker(insidenessArray, 0);
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro("Mismatch in selection list and field array.");
        return false;
      }
    }
    else
    {
      ArrayValueRangeFunctor worker(insidenessArray, 0);
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro("Mismatch in selection list and field array.");
        return false;
      }
    }
    insidenessArray->Modified();
    return true;
  }
};

//----------------------------------------------------------------------------
bool vtkValueSelector::vtkInternals::Execute(
  vtkDataObject* dobj, vtkSignedCharArray* insidenessArray)
{
  if (this->FieldAssociation != -1 && !this->FieldName.empty())
  {
    auto* dsa = dobj->GetAttributesAsFieldData(this->FieldAssociation);
    return dsa ? this->Execute(dsa->GetAbstractArray(this->FieldName.c_str()), insidenessArray)
               : false;
  }
  else if (this->FieldAssociation != -1 && this->FieldAttributeType != -1)
  {
    auto* dsa = dobj->GetAttributes(this->FieldAssociation);
    return dsa ? this->Execute(dsa->GetAbstractAttribute(this->FieldAttributeType), insidenessArray)
               : false;
  }
  else if (this->FieldAssociation != -1)
  {
    return this->Execute(insidenessArray);
  }
  return false;
}

//============================================================================
vtkStandardNewMacro(vtkValueSelector);
//----------------------------------------------------------------------------
vtkValueSelector::vtkValueSelector()
  : Internals(nullptr)
{
}

//----------------------------------------------------------------------------
vtkValueSelector::~vtkValueSelector()
{
}

//----------------------------------------------------------------------------
void vtkValueSelector::Initialize(vtkSelectionNode* node)
{
  assert(node);

  this->Internals.reset();

  try
  {
    vtkSmartPointer<vtkAbstractArray> selectionList = node->GetSelectionList();
    if (!selectionList || selectionList->GetNumberOfTuples() == 0)
    {
      // empty selection list, nothing to do.
      return;
    }

    auto properties = node->GetProperties();

    const int contentType = node->GetContentType();
    const int fieldType = node->GetFieldType();
    const int assoc = vtkSelectionNode::ConvertSelectionFieldToAttributeType(fieldType);
    const int component_no = properties->Has(vtkSelectionNode::COMPONENT_NUMBER())
      ? properties->Get(vtkSelectionNode::COMPONENT_NUMBER())
      : 0;

    switch (contentType)
    {
      case vtkSelectionNode::GLOBALIDS:
        this->Internals.reset(
          new vtkInternals(selectionList, assoc, vtkDataSetAttributes::GLOBALIDS, component_no));
        break;

      case vtkSelectionNode::PEDIGREEIDS:
        this->Internals.reset(
          new vtkInternals(selectionList, assoc, vtkDataSetAttributes::PEDIGREEIDS, component_no));
        break;

      case vtkSelectionNode::THRESHOLDS:
          if (selectionList->GetNumberOfComponents() == 1)
          {
#ifndef VTK_LEGACY_SILENT
            vtkWarningMacro("Warning: range selections should use two-component arrays to specify the"
                " range.  Using single component arrays with a tuple for the low and high ends of the"
                " range is legacy behavior and may be removed in future releases.");
#endif
            auto selList = vtkDataArray::SafeDownCast(selectionList.GetPointer());
            if (selList)
            {
              selectionList = vtkSmartPointer<vtkAbstractArray>::NewInstance(selList);
              selectionList->SetNumberOfComponents(2);
              selectionList->SetNumberOfTuples(selList->GetNumberOfTuples()/2);
              selectionList->SetName(selList->GetName());

              ThresholdSelectionListReshaper reshaper(vtkDataArray::SafeDownCast(selectionList));

              if (!vtkArrayDispatch::Dispatch::Execute(
                    selList, reshaper))
              {
                // should never happen, we create an array with the same type
                vtkErrorMacro("Mismatch in selection list fixup code");
                break;
              }
            }
          }
          VTK_FALLTHROUGH;
      case vtkSelectionNode::VALUES:
        if (selectionList->GetName() == nullptr || selectionList->GetName()[0] == '\0')
        {
          // if selectionList has no name, we're selected scalars (this is old
          // behavior, and we're preserving it).
          this->Internals.reset(new vtkInternals(selectionList, assoc, vtkDataSetAttributes::SCALARS, component_no));
        }
        else
        {
          this->Internals.reset(new vtkInternals(selectionList, assoc, selectionList->GetName(), component_no));
        }
        break;

      case vtkSelectionNode::INDICES:
        this->Internals.reset(new vtkInternals(selectionList, assoc));
        break;

      default:
        vtkErrorMacro("vtkValueSelector doesn't support content-type: " << contentType);
        break;
    };
  }
  catch (const std::runtime_error& e)
  {
    vtkErrorMacro(<< e.what());
  }
}

//----------------------------------------------------------------------------
void vtkValueSelector::Finalize()
{
  this->Internals.reset();
}

//----------------------------------------------------------------------------
bool vtkValueSelector::ComputeSelectedElements(
  vtkDataObject* input, vtkSignedCharArray* elementInside)
{
  assert(input != nullptr && elementInside != nullptr);
  return this->Internals ? this->Internals->Execute(input, elementInside) : false;
}

//----------------------------------------------------------------------------
void vtkValueSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
