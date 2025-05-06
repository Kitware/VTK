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

#include <viskores/cont/UnknownCellSet.h>

#include <viskores/cont/UncertainCellSet.h>

#include <sstream>

namespace
{

// Could potentially precompile more cell sets to serialze if that is useful.
using UnknownSerializationCellSets = VISKORES_DEFAULT_CELL_SET_LIST;

}

namespace viskores
{
namespace cont
{

viskores::cont::UnknownCellSet UnknownCellSet::NewInstance() const
{
  UnknownCellSet newCellSet;
  if (this->Container)
  {
    newCellSet.Container = this->Container->NewInstance();
  }
  return newCellSet;
}

std::string UnknownCellSet::GetCellSetName() const
{
  if (this->Container)
  {
    return viskores::cont::TypeToString(typeid(this->Container.get()));
  }
  else
  {
    return "";
  }
}

void UnknownCellSet::PrintSummary(std::ostream& os) const
{
  if (this->Container)
  {
    this->Container->PrintSummary(os);
  }
  else
  {
    os << " UnknownCellSet = nullptr\n";
  }
}

namespace internal
{

void ThrowCastAndCallException(const viskores::cont::UnknownCellSet& ref,
                               const std::type_info& type)
{
  std::ostringstream out;
  out << "Could not find appropriate cast for cell set in CastAndCall.\n"
         "CellSet: ";
  ref.PrintSummary(out);
  out << "TypeList: " << viskores::cont::TypeToString(type) << "\n";
  throw viskores::cont::ErrorBadType(out.str());
}

} // namespace internal

} // namespace viskores::cont
} // namespace viskores

//=============================================================================
// Specializations of serialization related classes

namespace viskores
{
namespace cont
{

std::string SerializableTypeString<viskores::cont::UnknownCellSet>::Get()
{
  return "UnknownCS";
}
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

void Serialization<viskores::cont::UnknownCellSet>::save(BinaryBuffer& bb,
                                                         const viskores::cont::UnknownCellSet& obj)
{
  viskoresdiy::save(bb, obj.ResetCellSetList<UnknownSerializationCellSets>());
}

void Serialization<viskores::cont::UnknownCellSet>::load(BinaryBuffer& bb,
                                                         viskores::cont::UnknownCellSet& obj)
{
  viskores::cont::UncertainCellSet<UnknownSerializationCellSets> uncertainCellSet;
  viskoresdiy::load(bb, uncertainCellSet);
  obj = uncertainCellSet;
}

} // namespace mangled_diy_namespace
