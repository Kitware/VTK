/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributesFieldList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetAttributesFieldList.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace detail
{
/**
 * FieldInfo is used to store metadata about a field.
 */
struct FieldInfo
{
  //@{
  /**
   * These attributes are used to compare two fields. If they match,
   * then the fields can be treated as similar, hence can be merged.
   */
  std::string Name;
  int Type;
  int NumberOfComponents;
  //@}

  //@{
  /**
   * These store metadata that may be present on any input field.
   * These are passed to the output in `CopyAllocate`
   */
  vtkSmartPointer<vtkLookupTable> LUT;
  vtkSmartPointer<vtkInformation> Information;
  std::vector<std::string> ComponentNames;
  //@}

  /**
   * An array where `AttributeTypes[j][i]==true` if this field is marked
   * as the i'th attribute type on the j'th input idx.
   */
  std::vector<std::array<bool, vtkDataSetAttributes::NUM_ATTRIBUTES> > AttributeTypes;

  /**
   * Location of this field in the input vtkDataSetAttributes instance at the
   * specific index, or -1 if not present in that input.
   */
  std::vector<int> Location;

  /**
   * This is set in `CopyAllocate` to indicate the location of this field in the
   * output vtkDataSetAttributes.
   */
  mutable int OutputLocation;

  FieldInfo()
    : Name()
    , Type(VTK_VOID)
    , NumberOfComponents(0)
    , LUT(nullptr)
    , Information(nullptr)
    , ComponentNames{}
    , Location{}
    , OutputLocation(-1)
  {
  }

  void PrintSelf(ostream& os, vtkIndent indent) const
  {
    if (this->IsEmpty())
    {
      os << indent << "FieldInfo (" << this << "): Empty" << endl;
    }
    else
    {
      os << indent << "FieldInfo (" << this << ")\n";
      os << indent.GetNextIndent() << "Name: " << this->Name << endl;
      os << indent.GetNextIndent() << "Type: " << this->Type << endl;
      os << indent.GetNextIndent() << "NumberOfComponents: " << this->NumberOfComponents << endl;
      os << indent.GetNextIndent() << "LUT: " << this->LUT << endl;
      os << indent.GetNextIndent() << "Information: " << this->Information << endl;
      os << indent.GetNextIndent() << "Location: [ ";
      for (const int& loc : this->Location)
      {
        os << loc << " ";
      }
      os << "]" << endl;
      os << indent.GetNextIndent() << "OutputLocation: " << this->OutputLocation << endl;
    }
  }

  bool IsEmpty() const { return this->Type == VTK_VOID; }

  static FieldInfo Create(vtkAbstractArray* array, int loc)
  {
    FieldInfo info;
    if (array)
    {
      info.Name = array->GetName() ? std::string(array->GetName()) : std::string();
      info.Type = array->GetDataType();
      info.NumberOfComponents = array->GetNumberOfComponents();
      if (auto da = vtkDataArray::SafeDownCast(array))
      {
        info.LUT = da->GetLookupTable();
      }
      info.Information = array->GetInformation();

      info.ComponentNames.resize(info.NumberOfComponents);
      for (int cc = 0; cc < info.NumberOfComponents; ++cc)
      {
        if (auto name = array->GetComponentName(cc))
        {
          info.ComponentNames[cc] = name;
        }
      }
      info.Location.push_back(loc);
    }
    return info;
  }

  void InitializeArray(vtkAbstractArray* array, vtkIdType sz, vtkIdType ext) const
  {
    if (array)
    {
      array->SetName(this->Name.empty() ? nullptr : this->Name.c_str());
      array->SetNumberOfComponents(this->NumberOfComponents);
      int cc = 0;
      for (const auto& cname : this->ComponentNames)
      {
        if (!cname.empty())
        {
          array->SetComponentName(cc, cname.c_str());
        }
        ++cc;
      }

      if (this->Information)
      {
        array->CopyInformation(this->Information, /*deep=*/1);
      }

      if (auto darray = vtkDataArray::SafeDownCast(array))
      {
        darray->SetLookupTable(this->LUT);
      }
      array->Allocate(sz, ext);
    }
  }

  bool IsSimilar(const FieldInfo& other) const
  {
    return (this->Name == other.Name && this->Type == other.Type &&
      this->NumberOfComponents == other.NumberOfComponents);
  }

  /**
   * This method merges `this` and `other` to return a new FieldInfo.
   */
  FieldInfo operator+(const FieldInfo& other) const
  {
    if (!this->IsEmpty() && this->IsSimilar(other))
    {
      FieldInfo result;
      result = *this;
      result.LUT = result.LUT ? result.LUT : other.LUT;
      result.Information = result.Information ? result.Information : other.Information;

      // merge component names.
      assert(result.ComponentNames.size() == other.ComponentNames.size());
      std::transform(result.ComponentNames.begin(), result.ComponentNames.end(),
        other.ComponentNames.begin(), result.ComponentNames.begin(),
        [](const std::string& in0, const std::string& in1) { return in0.empty() ? in1 : in0; });

      assert(other.Location.size() == 1);
      result.Location.insert(result.Location.end(), other.Location.begin(), other.Location.end());

      result.AttributeTypes.insert(
        result.AttributeTypes.end(), other.AttributeTypes.begin(), other.AttributeTypes.end());
      return result;
    }
    else
    {
      return FieldInfo();
    }
  }

  //@{
  /**
   * These methods are used by `UnionFieldList` to pad a FieldInfo instance.
   * Calling these methods clears `AttributeTypes` since it indicates that this
   * field is missing either in the inputs seen so far or in the current
   * input and hence cannot be flagged as an attribute.
   */
  void ExtendForUnion()
  {
    this->Location.push_back(-1);

    std::array<bool, vtkDataSetAttributes::NUM_ATTRIBUTES> curattrs;
    std::fill(curattrs.begin(), curattrs.end(), false);
    this->AttributeTypes.push_back(std::move(curattrs));
  }

  void PreExtendForUnion(int count)
  {
    this->Location.insert(this->Location.begin(), count, -1);

    std::array<bool, vtkDataSetAttributes::NUM_ATTRIBUTES> curattrs;
    std::fill(curattrs.begin(), curattrs.end(), false);
    this->AttributeTypes.insert(this->AttributeTypes.begin(), count, curattrs);
  }
  //@}
};

std::multimap<std::string, FieldInfo> GetFields(vtkDataSetAttributes* dsa)
{
  std::array<int, vtkDataSetAttributes::NUM_ATTRIBUTES> attribute_indices;
  dsa->GetAttributeIndices(attribute_indices.data());

  std::multimap<std::string, FieldInfo> fields;
  const auto num_of_arrays = dsa->GetNumberOfArrays();
  for (int cc = 0; cc < num_of_arrays; ++cc)
  {
    auto finfo = FieldInfo::Create(dsa->GetAbstractArray(cc), cc);

    // setup attributes info.
    std::array<bool, vtkDataSetAttributes::NUM_ATTRIBUTES> curattrs;
    std::transform(attribute_indices.begin(), attribute_indices.end(), curattrs.begin(),
      [cc](int idx) { return idx == cc; });

    finfo.AttributeTypes.push_back(std::move(curattrs));

    fields.insert(std::make_pair(finfo.Name, std::move(finfo)));
  }
  return fields;
}

/**
 * returns a vector of FieldInfo* where the index is the attribute type and
 * value is the FieldInfo that will be flagged as that attribute type.
 * To determine this, we look at the AttributeTypes information accumulated for
 * inputs and mark an attribute as such only if its tagged as an attribute on
 * all inputs consistently.
 */
std::array<const detail::FieldInfo*, vtkDataSetAttributes::NUM_ATTRIBUTES> GetAttributes(
  const std::multimap<std::string, FieldInfo>& mmap)
{
  std::array<const detail::FieldInfo*, vtkDataSetAttributes::NUM_ATTRIBUTES> attrs;
  std::fill(attrs.begin(), attrs.end(), nullptr);
  for (auto& pair : mmap)
  {
    const FieldInfo* finfo = &pair.second;

    // check if this field is consistently marked as an attribute in all inputs.
    std::array<bool, vtkDataSetAttributes::NUM_ATTRIBUTES> accumulated_attrs;
    std::fill(accumulated_attrs.begin(), accumulated_attrs.end(), true);
    for (const auto& inattrs : finfo->AttributeTypes)
    {
      std::transform(accumulated_attrs.begin(), accumulated_attrs.end(), inattrs.begin(),
        accumulated_attrs.begin(), std::logical_and<bool>());
    }

    std::transform(attrs.begin(), attrs.end(), accumulated_attrs.begin(), attrs.begin(),
      [&](const detail::FieldInfo* prev, bool isattr) {
        return isattr && prev == nullptr ? finfo : prev;
      });
  }
  return attrs;
}

template <typename Container, typename ForwardIt, typename UnaryPredicate>
void remove_if(Container& cont, ForwardIt first, ForwardIt second, UnaryPredicate p)
{
  for (auto iter = first; iter != second;)
  {
    if (p(*iter))
    {
      iter = cont.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}
}

class vtkDataSetAttributesFieldList::vtkInternals
{
public:
  enum FieldListMode
  {
    NONE,
    INTERSECTION,
    UNION
  };

  std::multimap<std::string, detail::FieldInfo> Fields;
  vtkIdType NumberOfTuples;
  int NumberOfInputs; //< tracks the number of inputs seen so far.
  FieldListMode Mode;

  vtkInternals()
    : NumberOfTuples(0)
    , NumberOfInputs(-1)
    , Mode(NONE)
  {
  }

  void Reset()
  {
    this->Fields.clear();
    this->NumberOfTuples = 0;
    this->NumberOfInputs = -1;
    this->Mode = NONE;
  }

  void Prune()
  {
    detail::remove_if(this->Fields, this->Fields.begin(), this->Fields.end(),
      [](const std::pair<std::string, detail::FieldInfo>& pair) { return pair.second.IsEmpty(); });
  }

  const detail::FieldInfo* GetLegacyFieldForIndex(int i) const
  {
    if (i >= 0 && i < vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
      auto attrs = detail::GetAttributes(this->Fields);
      auto finfo = attrs[i];
      return finfo;
    }
    else if (i >= vtkDataSetAttributes::NUM_ATTRIBUTES &&
      i < vtkDataSetAttributes::NUM_ATTRIBUTES + static_cast<int>(this->Fields.size()))
    {
      const auto attrs_ptrs = detail::GetAttributes(this->Fields);

      auto iter = this->Fields.begin();
      std::advance(iter, i - vtkDataSetAttributes::NUM_ATTRIBUTES);
      if (std::find(attrs_ptrs.begin(), attrs_ptrs.end(), &iter->second) != attrs_ptrs.end())
      {
        // i is beyond available attribute types and this field was already
        // reported as an attribute, don't report it again.
        return nullptr;
      }
      return &iter->second;
    }

    return nullptr;
  }
};

//----------------------------------------------------------------------------
vtkDataSetAttributesFieldList::vtkDataSetAttributesFieldList(int vtkNotUsed(number_of_inputs))
  : Internals(new vtkDataSetAttributesFieldList::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkDataSetAttributesFieldList::~vtkDataSetAttributesFieldList() {}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::Reset()
{
  this->Internals->Reset();
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::InitializeFieldList(vtkDataSetAttributes* dsa)
{
  this->Internals->Reset();
  this->Internals->Fields = detail::GetFields(dsa);
  this->Internals->NumberOfTuples += dsa->GetNumberOfTuples();
  this->Internals->NumberOfInputs++;

  // initialize OutputLocation to match the input location for 0th input. This
  // is to support legacy use-cases where FieldList was used without
  // calling CopyAllocate.
  for (auto& pair : this->Internals->Fields)
  {
    auto& fieldInfo = pair.second;
    fieldInfo.OutputLocation = fieldInfo.Location.front();
  }
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::IntersectFieldList(vtkDataSetAttributes* dsa)
{
  auto& internals = *this->Internals;
  if (internals.NumberOfInputs == -1)
  {
    // called without calling InitializeFieldList, just call it.
    this->InitializeFieldList(dsa);
    internals.Mode = vtkInternals::INTERSECTION;
    return;
  }

  if (internals.Mode == vtkInternals::UNION)
  {
    vtkGenericWarningMacro("Mixing of `IntersectFieldList` and `UnionFieldList` "
                           "calls is not supported!");
    return;
  }
  internals.Mode = vtkInternals::INTERSECTION;
  internals.NumberOfTuples += dsa->GetNumberOfTuples();

  const auto curfields = detail::GetFields(dsa);
  auto& accfields = internals.Fields;

  // first, find the array names in the intersection set.
  // we build set of keys for the accumulated fields (accfields) and current
  // fields (curfields).
  std::set<std::string> acckeys;
  for (const auto& pair : accfields)
  {
    acckeys.insert(pair.first);
  }

  std::set<std::string> curkeys;
  for (const auto& pair : curfields)
  {
    curkeys.insert(pair.first);
  }

  std::set<std::string> rkeys;
  std::set_intersection(acckeys.begin(), acckeys.end(), curkeys.begin(), curkeys.end(),
    std::inserter(rkeys, rkeys.end()));

  // second, remove fields from accumulate collection with names not in the
  // intersection set.
  detail::remove_if(accfields, accfields.begin(), accfields.end(),
    [&](const std::pair<std::string, detail::FieldInfo>& pair) {
      return rkeys.find(pair.first) == rkeys.end();
    });

  // now, since multiple fields can have same name (including empty names),
  // we do second intersection for fields with same names (or no names).
  for (const auto& fname : rkeys)
  {
    decltype(accfields.begin()) acciter, accend;
    std::tie(acciter, accend) = accfields.equal_range(fname);

    decltype(curfields.begin()) niter, nend;
    std::tie(niter, nend) = curfields.equal_range(fname);

    for (; acciter != accend && niter != nend; ++acciter, ++niter)
    {
      acciter->second = acciter->second + niter->second;
    }

    // any extra fields in the accumulated set for the current name
    // are pruned.
    accfields.erase(acciter, accend);
  }

  internals.NumberOfInputs++;
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::UnionFieldList(vtkDataSetAttributes* dsa)
{
  auto& internals = *this->Internals;
  if (internals.NumberOfInputs == -1)
  {
    // called without calling InitializeFieldList, just call it.
    this->InitializeFieldList(dsa);
    internals.Mode = vtkInternals::UNION;
    return;
  }

  if (internals.Mode == vtkInternals::INTERSECTION)
  {
    vtkGenericWarningMacro("Mixing of `IntersectFieldList` and `UnionFieldList` "
                           "calls is not supported!");
    return;
  }
  internals.Mode = vtkInternals::UNION;
  internals.NumberOfTuples += dsa->GetNumberOfTuples();

  auto curfields = detail::GetFields(dsa);
  auto& accfields = internals.Fields;

  std::set<const detail::FieldInfo*> updated_finfos;

  // iterate over curfields to find matching fields in those accumulated so far
  // and merge them if found.
  for (auto& curpair : curfields)
  {
    const std::string& fname = curpair.first;
    detail::FieldInfo& finfo = curpair.second;

    // for the incoming array, find an unused best-match,
    decltype(accfields.begin()) acciter, accend;
    std::tie(acciter, accend) = accfields.equal_range(fname);
    for (; acciter != accend; ++acciter)
    {
      if (acciter->second.IsSimilar(finfo) && updated_finfos.count(&acciter->second) == 0)
      {
        // found a match, combine them.
        acciter->second = acciter->second + finfo;
        updated_finfos.insert(&acciter->second);
        finfo = detail::FieldInfo();
        break;
      }
    }
  }

  // for all FieldInfo instances in accumulated fields
  // not in updated_finfos, pad them with a extra location
  // for the current input with `-1`. That is indicate that the field is missing
  // in the current input.
  for (auto& accpair : accfields)
  {
    if (updated_finfos.find(&accpair.second) == updated_finfos.end())
    {
      accpair.second.ExtendForUnion();
    }
  }

  // for all non-empty FieldInfo in curfields, add them to the accumulation set
  // after padding the location to indicate that the field is missing is inputs
  // seen for far.
  for (auto& curpair : curfields)
  {
    if (!curpair.second.IsEmpty())
    {
      curpair.second.PreExtendForUnion(internals.NumberOfInputs);
      accfields.insert(curpair);
    }
  }

  internals.NumberOfInputs++;
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::CopyAllocate(
  vtkDataSetAttributes* output, int ctype, vtkIdType sz, vtkIdType ext) const
{
  auto& internals = *this->Internals;
  // lets remove empty items to make iteration easier.
  internals.Prune();

  sz = sz > 0 ? sz : internals.NumberOfTuples;

  // these are pointers to fields to be tagged as attributes.
  const auto attribute_ptrs = detail::GetAttributes(internals.Fields);

  for (auto& pair : internals.Fields)
  {
    const auto& name = pair.first;
    const auto& fieldInfo = pair.second;
    fieldInfo.OutputLocation = -1;

    assert(!fieldInfo.IsEmpty());

    bool skip_field = false;
    bool is_attribute = false;

    // lets determine if the field is to be copied over (rather skipped) using attribute flags
    // if the field is marked as any of the attribute types
    for (int attrType = 0; attrType < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attrType)
    {
      if (attribute_ptrs[attrType] == &fieldInfo &&
        output->CopyAttributeFlags[ctype][attrType] == 0)
      {
        skip_field = true;
      }

      is_attribute = is_attribute | (attribute_ptrs[attrType] == &fieldInfo);
    }

    if (skip_field)
    {
      continue;
    }

    if (!is_attribute)
    {
      // if the field it not an attribute, check if it's to copied using array rules.
      // (this is directly copied over from vtkDataSetAttributes::FieldList,
      // the intent is a little unclear to me).
      const int flag = output->GetFlag(name.c_str());
      const bool copy = ((flag != 0) && !(output->DoCopyAllOff && (flag != 1)));
      if (!copy)
      {
        continue;
      }
    }

    auto array = this->CreateArray(fieldInfo.Type);
    if (array)
    {
      fieldInfo.InitializeArray(array, sz, ext);
      int index = output->AddArray(array);
      fieldInfo.OutputLocation = index;

      // flag as appropriate attribute.
      for (int attrType = 0;
           is_attribute && attrType < vtkDataSetAttributes::NUM_ATTRIBUTES && index != -1;
           ++attrType)
      {
        if (attribute_ptrs[attrType] == &fieldInfo)
        {
          output->SetActiveAttribute(index, attrType);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::CopyData(int inputIndex, vtkDataSetAttributes* input,
  vtkIdType fromId, vtkDataSetAttributes* output, vtkIdType toId) const
{
  auto& internals = *this->Internals;
  for (auto& pair : internals.Fields)
  {
    auto& fieldInfo = pair.second;
    if (inputIndex < 0 || inputIndex > static_cast<int>(fieldInfo.Location.size()))
    {
      vtkGenericWarningMacro("Incorrect/unknown inputIndex specified : " << inputIndex);
      return;
    }
    else if (fieldInfo.OutputLocation != -1 && fieldInfo.Location[inputIndex] != -1)
    {
      output->CopyTuple(input->GetAbstractArray(fieldInfo.Location[inputIndex]),
        output->GetAbstractArray(fieldInfo.OutputLocation), fromId, toId);
    }
  }
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::CopyData(int inputIndex, vtkDataSetAttributes* input,
  vtkIdType inputStart, vtkIdType numValues, vtkDataSetAttributes* output, vtkIdType outStart) const
{
  auto& internals = *this->Internals;
  for (auto& pair : internals.Fields)
  {
    auto& fieldInfo = pair.second;
    if (inputIndex < 0 || inputIndex > static_cast<int>(fieldInfo.Location.size()))
    {
      vtkGenericWarningMacro("Incorrect/unknown inputIndex specified : " << inputIndex);
      return;
    }
    else if (fieldInfo.OutputLocation != -1 && fieldInfo.Location[inputIndex] != -1)
    {
      output->CopyTuples(input->GetAbstractArray(fieldInfo.Location[inputIndex]),
        output->GetAbstractArray(fieldInfo.OutputLocation), outStart, numValues, inputStart);
    }
  }
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::InterpolatePoint(int inputIndex, vtkDataSetAttributes* input,
  vtkIdList* inputIds, double* weights, vtkDataSetAttributes* output, vtkIdType toId) const
{
  auto& internals = *this->Internals;
  for (auto& pair : internals.Fields)
  {
    auto& fieldInfo = pair.second;
    if (inputIndex < 0 || inputIndex > static_cast<int>(fieldInfo.Location.size()))
    {
      vtkGenericWarningMacro("Incorrect/unknown inputIndex specified : " << inputIndex);
      return;
    }
    else if (fieldInfo.OutputLocation != -1 && fieldInfo.Location[inputIndex] != -1)
    {
      auto fromArray = input->GetAbstractArray(fieldInfo.Location[inputIndex]);
      auto toArray = output->GetAbstractArray(fieldInfo.OutputLocation);

      // check if the destination array needs nearest neighbor interpolation.
      int attrIndex = input->IsArrayAnAttribute(fieldInfo.Location[inputIndex]);
      if (attrIndex != -1 &&
        output->GetCopyAttribute(attrIndex, vtkDataSetAttributes::INTERPOLATE) == 2)
      {
        vtkIdType numIds = inputIds->GetNumberOfIds();
        vtkIdType maxId = inputIds->GetId(0);
        vtkIdType maxWeight = 0.;
        for (int j = 0; j < numIds; j++)
        {
          if (weights[j] > maxWeight)
          {
            maxWeight = weights[j];
            maxId = inputIds->GetId(j);
          }
        }
        toArray->InsertTuple(toId, maxId, fromArray);
      }
      else
      {
        toArray->InterpolateTuple(toId, inputIds, fromArray, weights);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::TransformData(int inputIndex, vtkDataSetAttributes* input,
  vtkDataSetAttributes* output, std::function<void(vtkAbstractArray*, vtkAbstractArray*)> op) const
{
  auto& internals = *this->Internals;
  for (auto& pair : internals.Fields)
  {
    auto& fieldInfo = pair.second;
    if (inputIndex < 0 || inputIndex > static_cast<int>(fieldInfo.Location.size()))
    {
      vtkGenericWarningMacro("Incorrect/unknown inputIndex specified : " << inputIndex);
      return;
    }
    else if (fieldInfo.OutputLocation != -1 && fieldInfo.Location[inputIndex] != -1)
    {
      op(input->GetAbstractArray(fieldInfo.Location[inputIndex]),
        output->GetAbstractArray(fieldInfo.OutputLocation));
    }
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkAbstractArray> vtkDataSetAttributesFieldList::CreateArray(int type) const
{
  return vtkSmartPointer<vtkAbstractArray>::Take(vtkAbstractArray::CreateArray(type));
}

//----------------------------------------------------------------------------
void vtkDataSetAttributesFieldList::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkDataSetAttributesFieldList (" << this << ")\n";
  auto& internals = *this->Internals;
  for (auto& pair : internals.Fields)
  {
    pair.second.PrintSelf(os, indent.GetNextIndent());
  }
}
