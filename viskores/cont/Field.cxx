//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Field.h>

#include <viskores/cont/Invoker.h>
#include <viskores/cont/Logging.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/TypeList.h>

#include <viskores/cont/ArrayRangeCompute.h>

namespace viskores
{
namespace cont
{

/// constructors for points / whole mesh
VISKORES_CONT
Field::Field(std::string name,
             Association association,
             const viskores::cont::UnknownArrayHandle& data)
  : Name(name)
  , FieldAssociation(association)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
}

VISKORES_CONT
Field::Field(const viskores::cont::Field& src)
  : Name(src.Name)
  , FieldAssociation(src.FieldAssociation)
  , Data(src.Data)
  , Range(src.Range)
  , ModifiedFlag(src.ModifiedFlag)
{
}

VISKORES_CONT
Field::Field(viskores::cont::Field&& src) noexcept
  : Name(std::move(src.Name))
  , FieldAssociation(std::move(src.FieldAssociation))
  , Data(std::move(src.Data))
  , Range(std::move(src.Range))
  , ModifiedFlag(std::move(src.ModifiedFlag))
{
}

VISKORES_CONT
Field& Field::operator=(const viskores::cont::Field& src)
{
  this->Name = src.Name;
  this->FieldAssociation = src.FieldAssociation;
  this->Data = src.Data;
  this->Range = src.Range;
  this->ModifiedFlag = src.ModifiedFlag;
  return *this;
}

VISKORES_CONT
Field& Field::operator=(viskores::cont::Field&& src) noexcept
{
  this->Name = std::move(src.Name);
  this->FieldAssociation = std::move(src.FieldAssociation);
  this->Data = std::move(src.Data);
  this->Range = std::move(src.Range);
  this->ModifiedFlag = std::move(src.ModifiedFlag);
  return *this;
}


VISKORES_CONT
void Field::PrintSummary(std::ostream& out, bool full) const
{
  out << "   " << this->Name;
  out << " assoc= ";
  switch (this->GetAssociation())
  {
    case Association::Any:
      out << "Any ";
      break;
    case Association::WholeDataSet:
      out << "WholeDataSet ";
      break;
    case Association::Points:
      out << "Points ";
      break;
    case Association::Cells:
      out << "Cells ";
      break;
    case Association::Partitions:
      out << "Partitions ";
      break;
    case Association::Global:
      out << "Global ";
      break;
  }
  this->Data.PrintSummary(out, full);
}

VISKORES_CONT
Field::~Field() {}


VISKORES_CONT
const viskores::cont::UnknownArrayHandle& Field::GetData() const
{
  return this->Data;
}

VISKORES_CONT
viskores::cont::UnknownArrayHandle& Field::GetData()
{
  this->ModifiedFlag = true;
  return this->Data;
}

VISKORES_CONT const viskores::cont::ArrayHandle<viskores::Range>& Field::GetRange() const
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "Field::GetRange");

  if (this->ModifiedFlag)
  {
    this->Range = viskores::cont::ArrayRangeCompute(this->Data);
    this->ModifiedFlag = false;
  }

  return this->Range;
}

VISKORES_CONT void Field::GetRange(viskores::Range* range) const
{
  this->GetRange();
  const viskores::Id length = this->Range.GetNumberOfValues();
  auto portal = this->Range.ReadPortal();
  for (viskores::Id i = 0; i < length; ++i)
  {
    range[i] = portal.Get(i);
  }
}

VISKORES_CONT void Field::SetData(const viskores::cont::UnknownArrayHandle& newdata)
{
  this->Data = newdata;
  this->ModifiedFlag = true;
}

namespace
{

struct CheckArrayType
{
  template <typename T, typename S>
  void operator()(viskores::List<T, S>,
                  const viskores::cont::UnknownArrayHandle& data,
                  bool& found) const
  {
    if (data.CanConvert<viskores::cont::ArrayHandle<T, S>>())
    {
      found = true;
    }
  }
};

} // anonymous namespace

bool Field::IsSupportedType() const
{
  bool found = false;
  viskores::ListForEach(
    CheckArrayType{},
    viskores::cont::internal::ListAllArrayTypes<VISKORES_DEFAULT_TYPE_LIST,
                                                VISKORES_DEFAULT_STORAGE_LIST>{},
    this->Data,
    found);
  return found;
}

namespace
{

struct CheckStorageType
{
  template <typename S>
  void operator()(S, const viskores::cont::UnknownArrayHandle& data, bool& found) const
  {
    if (data.IsStorageType<S>())
    {
      found = true;
    }
  }
};

// This worklet is used in lieu of ArrayCopy because the use of ArrayHandleRecombineVec
// can throw off the casting in implementations of ArrayCopy.
struct CopyWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InType, typename OutType>
  VISKORES_EXEC_CONT void operator()(const InType& in, OutType& out) const
  {
    VISKORES_ASSERT(in.GetNumberOfComponents() == out.GetNumberOfComponents());
    for (viskores::IdComponent cIndex = 0; cIndex < in.GetNumberOfComponents(); ++cIndex)
    {
      out[cIndex] = static_cast<viskores::FloatDefault>(in[cIndex]);
    }
  }
};

struct CopyToFloatArray
{
  template <typename ArrayType>
  void operator()(const ArrayType& inArray, viskores::cont::UnknownArrayHandle& outArray) const
  {
    viskores::cont::Invoker invoke;
    invoke(CopyWorklet{}, inArray, outArray.ExtractArrayFromComponents<viskores::FloatDefault>());
  }
};

} // anonymous namespace

viskores::cont::UnknownArrayHandle Field::GetDataAsDefaultFloat() const
{
  if (this->Data.IsBaseComponentType<viskores::FloatDefault>())
  {
    bool supportedStorage = false;
    viskores::ListForEach(
      CheckStorageType{}, VISKORES_DEFAULT_STORAGE_LIST{}, this->Data, supportedStorage);
    if (supportedStorage)
    {
      // Array is already float default and supported storage. No better conversion can be done.
      return this->Data;
    }
  }

  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Info,
                     "Converting field '%s' to default floating point.",
                     this->GetName().c_str());
  viskores::cont::UnknownArrayHandle outArray = this->Data.NewInstanceFloatBasic();
  outArray.Allocate(this->Data.GetNumberOfValues());
  this->Data.CastAndCallWithExtractedArray(CopyToFloatArray{}, outArray);
  return outArray;
}

viskores::cont::UnknownArrayHandle Field::GetDataWithExpectedTypes() const
{
  if (this->IsSupportedType())
  {
    return this->Data;
  }
  else
  {
    return this->GetDataAsDefaultFloat();
  }
}

void Field::ConvertToExpected()
{
  this->SetData(this->GetDataWithExpectedTypes());
}

}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

void Serialization<viskores::cont::Field>::save(BinaryBuffer& bb,
                                                const viskores::cont::Field& field)
{
  viskoresdiy::save(bb, field.GetName());
  viskoresdiy::save(bb, static_cast<int>(field.GetAssociation()));
  viskoresdiy::save(bb, field.GetData());
}

void Serialization<viskores::cont::Field>::load(BinaryBuffer& bb, viskores::cont::Field& field)
{
  std::string name;
  viskoresdiy::load(bb, name);
  int assocVal = 0;
  viskoresdiy::load(bb, assocVal);

  auto assoc = static_cast<viskores::cont::Field::Association>(assocVal);
  viskores::cont::UnknownArrayHandle data;
  viskoresdiy::load(bb, data);
  field = viskores::cont::Field(name, assoc, data);
}


} // namespace diy
