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

#include <viskores/cont/internal/FieldCollection.h>

namespace viskores
{
namespace cont
{
namespace internal
{

VISKORES_CONT void FieldCollection::AddField(const Field& field)
{
  if (this->ValidAssoc.find(field.GetAssociation()) == this->ValidAssoc.end())
  {
    throw viskores::cont::ErrorBadValue("Invalid association for field: " + field.GetName());
  }

  this->Fields[{ field.GetName(), field.GetAssociation() }] = field;
}

const viskores::cont::Field& FieldCollection::GetField(viskores::Id index) const
{
  VISKORES_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  auto it = this->Fields.cbegin();
  std::advance(it, index);
  return it->second;
}

viskores::cont::Field& FieldCollection::GetField(viskores::Id index)
{
  VISKORES_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  auto it = this->Fields.begin();
  std::advance(it, index);
  return it->second;
}

viskores::Id FieldCollection::GetFieldIndex(const std::string& name,
                                            viskores::cont::Field::Association assoc) const
{
  // Find the field with the given name and association. If the association is
  // `viskores::cont::Field::Association::Any`, then the `Fields` object has a
  // special comparator that will match the field to any association.
  const auto it = this->Fields.find({ name, assoc });
  if (it != this->Fields.end())
  {
    return static_cast<viskores::Id>(std::distance(this->Fields.begin(), it));
  }
  return -1;
}

const viskores::cont::Field& FieldCollection::GetField(
  const std::string& name,
  viskores::cont::Field::Association assoc) const
{
  auto idx = this->GetFieldIndex(name, assoc);
  if (idx == -1)
  {
    throw viskores::cont::ErrorBadValue("No field with requested name: " + name);
  }

  return this->GetField(idx);
}

viskores::cont::Field& FieldCollection::GetField(const std::string& name,
                                                 viskores::cont::Field::Association assoc)
{
  auto idx = this->GetFieldIndex(name, assoc);
  if (idx == -1)
  {
    throw viskores::cont::ErrorBadValue("No field with requested name: " + name);
  }

  return this->GetField(idx);
}

}
}
} //viskores::cont::internal
