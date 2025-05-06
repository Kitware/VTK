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

#include <viskores/filter/FieldSelection.h>

#include <map>

namespace
{

struct FieldDescription
{
  std::string Name;
  viskores::cont::Field::Association Association;
  FieldDescription() = default;
  FieldDescription(const std::string& name, viskores::cont::Field::Association assoc)
    : Name(name)
    , Association(assoc)
  {
  }

  FieldDescription(const FieldDescription&) = default;
  FieldDescription& operator=(const FieldDescription&) = default;

  bool operator<(const FieldDescription& other) const
  {
    return (this->Association == other.Association) ? (this->Name < other.Name)
                                                    : (this->Association < other.Association);
  }
};

} // anonymous namespace

namespace viskores
{
namespace filter
{

struct FieldSelection::InternalStruct
{
  Mode ModeType;

  std::map<FieldDescription, Mode> Fields;
};

FieldSelection::FieldSelection(Mode mode)
  : Internals(new InternalStruct)
{
  this->SetMode(mode);
}

FieldSelection::FieldSelection(const std::string& field, Mode mode)
  : FieldSelection(mode)
{
  this->AddField(field, viskores::cont::Field::Association::Any);
}

FieldSelection::FieldSelection(const char* field, Mode mode)
  : FieldSelection(mode)
{
  this->AddField(field, viskores::cont::Field::Association::Any);
}

FieldSelection::FieldSelection(const std::string& field,
                               viskores::cont::Field::Association association,
                               Mode mode)
  : FieldSelection(mode)
{
  this->AddField(field, association);
}

FieldSelection::FieldSelection(std::initializer_list<std::string> fields, Mode mode)
  : FieldSelection(mode)
{
  for (const std::string& afield : fields)
  {
    this->AddField(afield, viskores::cont::Field::Association::Any);
  }
}

FieldSelection::FieldSelection(
  std::initializer_list<std::pair<std::string, viskores::cont::Field::Association>> fields,
  Mode mode)
  : FieldSelection(mode)
{
  for (const auto& item : fields)
  {
    this->AddField(item.first, item.second);
  }
}

FieldSelection::FieldSelection(
  std::initializer_list<viskores::Pair<std::string, viskores::cont::Field::Association>> fields,
  Mode mode)
  : FieldSelection(mode)
{
  for (const auto& item : fields)
  {
    this->AddField(item.first, item.second);
  }
}

FieldSelection::FieldSelection(const FieldSelection& src)
  : Internals(new InternalStruct(*src.Internals))
{
}

FieldSelection::FieldSelection(FieldSelection&&) = default;

FieldSelection& FieldSelection::operator=(const FieldSelection& src)
{
  *this->Internals = *src.Internals;
  return *this;
}

FieldSelection& FieldSelection::operator=(FieldSelection&&) = default;

FieldSelection::~FieldSelection() = default;

bool FieldSelection::IsFieldSelected(const std::string& name,
                                     viskores::cont::Field::Association association) const
{
  switch (this->GetFieldMode(name, association))
  {
    case Mode::Select:
      return true;
    case Mode::Exclude:
      return false;
    default:
      switch (this->GetMode())
      {
        case Mode::None:
        case Mode::Select:
          // Fields are not selected unless explicitly set
          return false;
        case Mode::All:
        case Mode::Exclude:
          // Fields are selected unless explicitly excluded
          return true;
      }
  }
  VISKORES_ASSERT(false && "Internal error. Unexpected mode");
  return false;
}

void FieldSelection::AddField(const std::string& fieldName,
                              viskores::cont::Field::Association association,
                              Mode mode)
{
  this->Internals->Fields[FieldDescription(fieldName, association)] = mode;
}

FieldSelection::Mode FieldSelection::GetFieldMode(
  const std::string& fieldName,
  viskores::cont::Field::Association association) const
{
  auto iter = this->Internals->Fields.find(FieldDescription(fieldName, association));
  if (iter != this->Internals->Fields.end())
  {
    return iter->second;
  }

  // if not exact match, let's lookup for Association::Any.
  for (const auto& aField : this->Internals->Fields)
  {
    if (aField.first.Name == fieldName)
    {
      if (aField.first.Association == viskores::cont::Field::Association::Any ||
          association == viskores::cont::Field::Association::Any)
      {
        return aField.second;
      }
    }
  }

  return Mode::None;
}

void FieldSelection::ClearFields()
{
  this->Internals->Fields.clear();
}

FieldSelection::Mode FieldSelection::GetMode() const
{
  return this->Internals->ModeType;
}

void FieldSelection::SetMode(Mode val)
{
  switch (val)
  {
    case Mode::None:
      this->ClearFields();
      this->Internals->ModeType = Mode::Select;
      break;
    case Mode::All:
      this->ClearFields();
      this->Internals->ModeType = Mode::Exclude;
      break;
    case Mode::Select:
    case Mode::Exclude:
      this->Internals->ModeType = val;
      break;
  }
}

}
} // namespace viskores::filter
