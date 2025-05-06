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
#ifndef viskores_cont_internal_FieldCollection_h
#define viskores_cont_internal_FieldCollection_h

#include <set>
#include <viskores/cont/Field.h>

namespace viskores
{
namespace cont
{
namespace internal
{

class VISKORES_CONT_EXPORT FieldCollection
{
public:
  VISKORES_CONT
  FieldCollection(std::initializer_list<viskores::cont::Field::Association> validAssoc)
  {
    auto it = this->ValidAssoc.begin();
    for (const auto& item : validAssoc)
      it = this->ValidAssoc.insert(it, item);
  }

  VISKORES_CONT
  FieldCollection(std::set<viskores::cont::Field::Association>&& validAssoc)
    : ValidAssoc(std::move(validAssoc))
  {
  }

  VISKORES_CONT
  void Clear() { this->Fields.clear(); }

  VISKORES_CONT
  viskores::IdComponent GetNumberOfFields() const
  {
    return static_cast<viskores::IdComponent>(this->Fields.size());
  }

  VISKORES_CONT void AddField(const Field& field);

  VISKORES_CONT
  const viskores::cont::Field& GetField(viskores::Id index) const;

  VISKORES_CONT
  viskores::cont::Field& GetField(viskores::Id index);

  VISKORES_CONT
  bool HasField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const
  {
    return (this->GetFieldIndex(name, assoc) != -1);
  }

  VISKORES_CONT
  viskores::Id GetFieldIndex(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const;

  VISKORES_CONT
  const viskores::cont::Field& GetField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any) const;

  VISKORES_CONT
  viskores::cont::Field& GetField(
    const std::string& name,
    viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any);

private:
  struct FieldCompare
  {
    using Key = std::pair<std::string, viskores::cont::Field::Association>;

    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
      if (a.first == b.first)
        return a.second < b.second && a.second != viskores::cont::Field::Association::Any &&
          b.second != viskores::cont::Field::Association::Any;

      return a.first < b.first;
    }
  };

  std::map<FieldCompare::Key, viskores::cont::Field, FieldCompare> Fields;
  std::set<viskores::cont::Field::Association> ValidAssoc;
};

}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_FieldCollection_h
