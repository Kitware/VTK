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
#include "vtkDataArrayRange.h"
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
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* originalList)
  {
    // created with NewInstance from the originalList, we know it is the same type
    auto fixedList = SelectionListArrayType::FastDownCast(this->FixedArray);
    assert(originalList->GetNumberOfComponents() == 1);
    assert(fixedList->GetNumberOfComponents() == 2);

    const auto orig = vtk::DataArrayValueRange<1>(originalList);
    auto fixed = vtk::DataArrayValueRange<2>(fixedList);
    std::copy(orig.cbegin(), orig.cend(), fixed.begin());
  }
};

//----------------------------------------------------------------------------
// This is used for the cases where the SelectionList is a 1-component array,
// implying that the values are exact matches.
struct ArrayValueMatchFunctor
{
  vtkSignedCharArray* InsidednessArray;
  int ComponentNo;

  ArrayValueMatchFunctor(vtkSignedCharArray* insidednessArray, int comp)
    : InsidednessArray(insidednessArray)
    , ComponentNo(comp)
  {
  }

  // this is used for selecting entries where the field array has matching
  // values.
  template <typename InputArrayType, typename SelectionListArrayType>
  void operator()(InputArrayType* fArray, SelectionListArrayType* selList)
  {
    static_assert(std::is_same<vtk::GetAPIType<InputArrayType>,
                    vtk::GetAPIType<SelectionListArrayType> >::value,
      "value types mismatched!");
    VTK_ASSUME(selList->GetNumberOfComponents() == 1);
    VTK_ASSUME(fArray->GetNumberOfComponents() > this->ComponentNo);

    using ValueType = vtk::GetAPIType<SelectionListArrayType>;

    const ValueType* haystack_begin = selList->GetPointer(0);
    const ValueType* haystack_end = haystack_begin + selList->GetNumberOfValues();
    const int comp = fArray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;

    vtkSignedCharArray* insidednessArray = this->InsidednessArray;
    VTK_ASSUME(insidednessArray->GetNumberOfTuples() == fArray->GetNumberOfTuples());
    if (comp >= 0)
    {
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        const auto fRange = vtk::DataArrayTupleRange(fArray, begin, end);
        auto insideRange = vtk::DataArrayValueRange<1>(insidednessArray, begin, end);
        auto insideIter = insideRange.begin();
        for (auto i = fRange.cbegin(); i != fRange.cend(); ++i, ++insideIter)
        {
          auto result = std::binary_search(haystack_begin, haystack_end, (*i)[comp]);
          *insideIter = result ? 1 : 0;
        }
      });
    }
    else
    {
      // compare vector magnitude.
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        const auto fRange = vtk::DataArrayTupleRange(fArray, begin, end);
        auto insideRange = vtk::DataArrayValueRange<1>(insidednessArray, begin, end);
        using FTupleCRefType = typename decltype(fRange)::ConstTupleReferenceType;
        std::transform(fRange.cbegin(), fRange.cend(), insideRange.begin(),
          [&](FTupleCRefType fTuple) -> signed char {
            ValueType val = ValueType(0);
            for (const ValueType fComp : fTuple)
            {
              val += fComp * fComp;
            }
            const auto mag = static_cast<ValueType>(std::sqrt(val));
            return std::binary_search(haystack_begin, haystack_end, mag) ? 1 : 0;
          });
      });
    }
  }

  // this is used to select indices
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* selList)
  {
    using T = vtk::GetAPIType<SelectionListArrayType>;
    assert(selList->GetNumberOfComponents() == 1);

    const vtkIdType numDataValues = this->InsidednessArray->GetNumberOfTuples();
    const auto selRange = vtk::DataArrayValueRange<1>(selList);

    this->InsidednessArray->FillValue(0);
    std::for_each(selRange.cbegin(), selRange.cend(), [&](const T selVal) {
      const auto cid = static_cast<vtkIdType>(selVal);
      if (cid >= 0 && cid < numDataValues)
      {
        this->InsidednessArray->SetValue(cid, 1);
      }
    });
  }
};

//----------------------------------------------------------------------------
// This is used for the cases where the SelectionList is a 2-component array,
// implying that the values are ranges.
struct ArrayValueRangeFunctor
{
  vtkSignedCharArray* InsidednessArray;
  int ComponentNo;

  ArrayValueRangeFunctor(vtkSignedCharArray* insidednessArray, int comp)
    : InsidednessArray(insidednessArray)
    , ComponentNo(comp)
  {
  }

  // for selecting using field array values
  template <typename InputArrayType, typename SelectionListArrayType>
  void operator()(InputArrayType* fArray, SelectionListArrayType* selList)
  {
    static_assert(std::is_same<vtk::GetAPIType<InputArrayType>,
                    vtk::GetAPIType<SelectionListArrayType> >::value,
      "value types mismatched!");

    using ValueType = vtk::GetAPIType<SelectionListArrayType>;

    VTK_ASSUME(selList->GetNumberOfComponents() == 2);
    VTK_ASSUME(fArray->GetNumberOfComponents() > this->ComponentNo);
    VTK_ASSUME(this->InsidednessArray->GetNumberOfTuples() == fArray->GetNumberOfTuples());

    const int comp = fArray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;

    if (comp >= 0)
    {
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        const auto fRange = vtk::DataArrayTupleRange(fArray, begin, end);
        const auto selRange = vtk::DataArrayTupleRange<2>(selList);
        auto insideRange = vtk::DataArrayValueRange<1>(this->InsidednessArray, begin, end);

        using FTupleCRefType = typename decltype(fRange)::ConstTupleReferenceType;
        using STupleCRefType = typename decltype(selRange)::ConstTupleReferenceType;

        auto insideIter = insideRange.begin();
        for (FTupleCRefType fTuple : fRange)
        {
          const ValueType val = fTuple[comp];
          auto matchIter = std::find_if(selRange.cbegin(), selRange.cend(),
            [&](STupleCRefType range) -> bool { return val >= range[0] && val <= range[1]; });
          *insideIter++ = matchIter != selRange.cend() ? 1 : 0;
        }
      });
    }
    else
    {
      // compare vector magnitude.
      vtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
        const auto fRange = vtk::DataArrayTupleRange(fArray, begin, end);
        const auto selRange = vtk::DataArrayTupleRange<2>(selList);
        auto insideRange = vtk::DataArrayValueRange<1>(this->InsidednessArray, begin, end);

        using FTupleCRefType = typename decltype(fRange)::ConstTupleReferenceType;
        using STupleCRefType = typename decltype(selRange)::ConstTupleReferenceType;

        auto insideIter = insideRange.begin();
        for (FTupleCRefType fTuple : fRange)
        {
          ValueType val{ 0 };
          for (const ValueType fComp : fTuple)
          {
            val += fComp * fComp;
          }
          const auto mag = static_cast<ValueType>(std::sqrt(val));
          auto matchIter = std::find_if(selRange.cbegin(), selRange.cend(),
            [&](STupleCRefType range) -> bool { return mag >= range[0] && mag <= range[1]; });
          *insideIter++ = matchIter != selRange.cend() ? 1 : 0;
        }
      });
    }
  }

  // this is used to select indices
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* selList)
  {
    assert(selList->GetNumberOfComponents() == 2);

    const auto selRange = vtk::DataArrayTupleRange<2>(selList);
    using SelRangeCRefT = typename decltype(selRange)::ConstTupleReferenceType;

    const vtkIdType numValues = this->InsidednessArray->GetNumberOfTuples();

    this->InsidednessArray->FillValue(0);
    for (SelRangeCRefT range : selRange)
    {
      const vtkIdType start = std::min(static_cast<vtkIdType>(range[0]), numValues - 1);
      const vtkIdType last = std::min(static_cast<vtkIdType>(range[1]), numValues - 1);
      if (start >= 0 && last >= start)
      {
        auto inside = vtk::DataArrayValueRange<1>(this->InsidednessArray, start, last + 1);
        std::fill(inside.begin(), inside.end(), 1);
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
  vtkInternals(vtkAbstractArray* selectionList, int fieldAssociation, const std::string& fieldName,
    int component)
    : vtkInternals(selectionList, fieldName, fieldAssociation, -1, component)
  {
  }

  // use this constructor when selection is specified as (assoc, attribute type)
  vtkInternals(
    vtkAbstractArray* selectionList, int fieldAssociation, int attributeType, int component)
    : vtkInternals(selectionList, "", fieldAssociation, attributeType, component)
  {
    if (attributeType < 0 || attributeType >= vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
      throw std::runtime_error("unsupported attribute type");
    }
  }

  // use this constructor when selection is for ids of element type = assoc.
  vtkInternals(vtkAbstractArray* selectionList, int fieldAssociation)
    : vtkInternals(selectionList, "", fieldAssociation, -1, 0)
  {
  }

  // returns false on any failure or unhandled case.
  bool Execute(vtkDataObject* dobj, vtkSignedCharArray* darray);

private:
  vtkInternals(vtkAbstractArray* selectionList, const std::string& fieldName, int fieldAssociation,
    int attributeType, int component)
    : SelectionList(selectionList)
    , FieldName(fieldName)
    , FieldAssociation(fieldAssociation)
    , FieldAttributeType(attributeType)
    , ComponentNo(component)
  {
    if (fieldAssociation < 0 || fieldAssociation >= vtkDataObject::NUMBER_OF_ASSOCIATIONS ||
      fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      throw std::runtime_error("unsupported field association");
    }

    if (selectionList->GetNumberOfComponents() != 1 && selectionList->GetNumberOfComponents() != 2)
    {
      // 1-component == exact value match
      // 2-component == values in range specified by each tuple.
      throw std::runtime_error("Currently, selecting multi-components arrays is not supported.");
    }

    if (selectionList->GetNumberOfComponents() == 1)
    {
      // we sort the selection list to speed up extraction later.
      this->SelectionList.TakeReference(selectionList->NewInstance());
      this->SelectionList->DeepCopy(selectionList);
      vtkSortDataArray::Sort(this->SelectionList);
    }
    else
    {
      // don't bother sorting.
      this->SelectionList = selectionList;
    }
  }

  bool Execute(vtkAbstractArray* darray, vtkSignedCharArray* insidednessArray)
  {
    if (auto dataArray = vtkDataArray::SafeDownCast(darray))
    {
      return this->Execute(dataArray, insidednessArray);
    }
    else if (darray)
    {
      // classes like vtkStringArray may be added later, if needed.
      vtkGenericWarningMacro(<< darray->GetClassName() << " not supported by vtkValueSelector.");
      return false;
    }
    else
    {
      // missing array, this can happen.
      return false;
    }
  }

  bool Execute(vtkDataArray* darray, vtkSignedCharArray* insidednessArray)
  {
    assert(vtkDataArray::SafeDownCast(this->SelectionList));
    if (darray->GetNumberOfComponents() < this->ComponentNo)
    {
      // array doesn't have request components. nothing to select.
      return false;
    }

    if (this->SelectionList->GetNumberOfComponents() == 1)
    {
      ArrayValueMatchFunctor worker(insidednessArray, this->ComponentNo);
      if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(
            darray, vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro("Type mismatch in selection list ("
          << this->SelectionList->GetClassName() << ") and field array (" << darray->GetClassName()
          << ").");
        return false;
      }
    }
    else
    {
      ArrayValueRangeFunctor worker(insidednessArray, this->ComponentNo);
      if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(
            darray, vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // Use the slow data array API for threshold-based selection. Thresholds
        // are assumed to be stored in a vtkDoubleArray, which may very well not be the
        // same as the data array type.
        vtkDataArray* selList = vtkDataArray::SafeDownCast(this->SelectionList);
        const int comp = darray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;
        const vtkIdType numRanges = selList->GetNumberOfTuples();

        if (comp >= 0)
        {
          vtkSMPTools::For(0, darray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
            for (vtkIdType cc = begin; cc < end; ++cc)
            {
              const auto val = darray->GetComponent(cc, comp);
              bool match = false;
              for (vtkIdType r = 0; r < numRanges && !match; ++r)
              {
                match = (val >= selList->GetComponent(r, 0) && val <= selList->GetComponent(r, 1));
              }
              insidednessArray->SetValue(cc, match ? 1 : 0);
            }
          });
        }
        else
        {
          const int num_components = darray->GetNumberOfComponents();

          // compare vector magnitude.
          vtkSMPTools::For(0, darray->GetNumberOfTuples(), [=](vtkIdType begin, vtkIdType end) {
            for (vtkIdType cc = begin; cc < end; ++cc)
            {
              double val = double(0);
              for (int kk = 0; kk < num_components; ++kk)
              {
                const auto valKK = darray->GetComponent(cc, comp);
                val += valKK * valKK;
              }
              const auto magnitude = std::sqrt(val);
              bool match = false;
              for (vtkIdType r = 0; r < numRanges && !match; ++r)
              {
                match = (magnitude >= selList->GetComponent(r, 0) &&
                  magnitude <= selList->GetComponent(r, 1));
              }
              insidednessArray->SetValue(cc, match ? 1 : 0);
            }
          });
        }
      }
    }

    insidednessArray->Modified();
    return true;
  }

  // this is used for when selecting elements by ids
  bool Execute(vtkSignedCharArray* insidednessArray)
  {
    assert(vtkDataArray::SafeDownCast(this->SelectionList));

    if (this->SelectionList->GetNumberOfComponents() == 1)
    {
      ArrayValueMatchFunctor worker(insidednessArray, 0);
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro(
          "Unsupported selection list array type (" << this->SelectionList->GetClassName() << ").");
        return false;
      }
    }
    else
    {
      ArrayValueRangeFunctor worker(insidednessArray, 0);
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            vtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        vtkGenericWarningMacro(
          "Unsupported selection list array type (" << this->SelectionList->GetClassName() << ").");
        return false;
      }
    }
    insidednessArray->Modified();
    return true;
  }
};

//----------------------------------------------------------------------------
bool vtkValueSelector::vtkInternals::Execute(
  vtkDataObject* dobj, vtkSignedCharArray* insidednessArray)
{
  if (this->FieldAssociation != -1 && !this->FieldName.empty())
  {
    auto* dsa = dobj->GetAttributesAsFieldData(this->FieldAssociation);
    return dsa ? this->Execute(dsa->GetAbstractArray(this->FieldName.c_str()), insidednessArray)
               : false;
  }
  else if (this->FieldAssociation != -1 && this->FieldAttributeType != -1)
  {
    auto* dsa = dobj->GetAttributes(this->FieldAssociation);
    return dsa
      ? this->Execute(dsa->GetAbstractAttribute(this->FieldAttributeType), insidednessArray)
      : false;
  }
  else if (this->FieldAssociation != -1)
  {
    return this->Execute(insidednessArray);
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
vtkValueSelector::~vtkValueSelector() {}

//----------------------------------------------------------------------------
void vtkValueSelector::Initialize(vtkSelectionNode* node)
{
  assert(node);

  this->Superclass::Initialize(node);

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
          vtkWarningMacro(
            "Warning: range selections should use two-component arrays to specify the"
            " range.  Using single component arrays with a tuple for the low and high ends of the"
            " range is legacy behavior and may be removed in future releases.");
#endif
          auto selList = vtkDataArray::SafeDownCast(selectionList.GetPointer());
          if (selList)
          {
            selectionList = vtkSmartPointer<vtkAbstractArray>::NewInstance(selList);
            selectionList->SetNumberOfComponents(2);
            selectionList->SetNumberOfTuples(selList->GetNumberOfTuples() / 2);
            selectionList->SetName(selList->GetName());

            ThresholdSelectionListReshaper reshaper(vtkDataArray::SafeDownCast(selectionList));

            if (!vtkArrayDispatch::Dispatch::Execute(selList, reshaper))
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
          this->Internals.reset(
            new vtkInternals(selectionList, assoc, vtkDataSetAttributes::SCALARS, component_no));
        }
        else
        {
          this->Internals.reset(
            new vtkInternals(selectionList, assoc, selectionList->GetName(), component_no));
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
  vtkDataObject* input, vtkSignedCharArray* insidednessArray)
{
  assert(input != nullptr && insidednessArray != nullptr);
  return this->Internals ? this->Internals->Execute(input, insidednessArray) : false;
}

//----------------------------------------------------------------------------
void vtkValueSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
