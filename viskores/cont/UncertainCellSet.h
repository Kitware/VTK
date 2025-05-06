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
#ifndef viskores_cont_UncertainCellSet_h
#define viskores_cont_UncertainCellSet_h

#include <viskores/cont/UnknownCellSet.h>

namespace viskores
{
namespace cont
{

/// \brief A `CellSet` of an uncertain type.
///
/// `UncertainCellSet` holds a `CellSet` object using runtime polymorphism to
/// manage different types. It behaves like its superclass, `UnknownCellSet`,
/// except that it also contains a template parameter that provides a
/// `viskores::List` of potential cell set types.
///
/// These potental types come into play when the `CastAndCall` method is called
/// (or the `UncertainCellSet` is used in the `viskores::cont::CastAndCall` function).
/// In this case, the `CastAndCall` will search for `CellSet`s of types that match
/// this list.
///
/// Both `UncertainCellSet` and `UnknownCellSet` have a method named
/// `ResetCellSetList` that redefines the list of potential cell sets by returning
/// a new `UncertainCellSet` containing the same `CellSet` but with the new cell
/// set type list.
///
template <typename CellSetList>
class VISKORES_ALWAYS_EXPORT UncertainCellSet : public viskores::cont::UnknownCellSet
{
  VISKORES_IS_LIST(CellSetList);

  VISKORES_STATIC_ASSERT_MSG((!std::is_same<CellSetList, viskores::ListUniversal>::value),
                             "Cannot use viskores::ListUniversal with UncertainCellSet.");

  using Superclass = UnknownCellSet;
  using Thisclass = UncertainCellSet<CellSetList>;

public:
  VISKORES_CONT UncertainCellSet() = default;

  template <typename CellSetType>
  VISKORES_CONT UncertainCellSet(const CellSetType& cellSet)
    : Superclass(cellSet)
  {
  }

  explicit VISKORES_CONT UncertainCellSet(const viskores::cont::UnknownCellSet& src)
    : Superclass(src)
  {
  }

  template <typename OtherCellSetList>
  explicit VISKORES_CONT UncertainCellSet(const UncertainCellSet<OtherCellSetList>& src)
    : Superclass(src)
  {
  }

  /// \brief Create a new cell set of the same type as this.
  ///
  /// This method creates a new cell set that is the same type as this one and
  /// returns a new `UncertainCellSet` for it.
  ///
  VISKORES_CONT Thisclass NewInstance() const { return Thisclass(this->Superclass::NewInstance()); }

  /// \brief Call a functor using the underlying cell set type.
  ///
  /// `CastAndCall` attempts to cast the held cell set to a specific type,
  /// and then calls the given functor with the cast cell set.
  ///
  template <typename Functor, typename... Args>
  VISKORES_CONT void CastAndCall(Functor&& functor, Args&&... args) const
  {
    this->CastAndCallForTypes<CellSetList>(std::forward<Functor>(functor),
                                           std::forward<Args>(args)...);
  }
};

// Defined here to avoid circular dependencies between UnknownCellSet and UncertainCellSet.
template <typename NewCellSetList>
VISKORES_CONT viskores::cont::UncertainCellSet<NewCellSetList> UnknownCellSet::ResetCellSetList(
  NewCellSetList) const
{
  return viskores::cont::UncertainCellSet<NewCellSetList>(*this);
}
template <typename NewCellSetList>
VISKORES_CONT viskores::cont::UncertainCellSet<NewCellSetList> UnknownCellSet::ResetCellSetList()
  const
{
  return viskores::cont::UncertainCellSet<NewCellSetList>(*this);
}

namespace internal
{

template <typename CellSetList>
struct DynamicTransformTraits<viskores::cont::UncertainCellSet<CellSetList>>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal

} // namespace viskores::cont
} // namespace viskores

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace viskores
{
namespace cont
{

template <typename CellSetList>
struct SerializableTypeString<viskores::cont::UncertainCellSet<CellSetList>>
{
  static VISKORES_CONT std::string Get()
  {
    return SerializableTypeString<viskores::cont::UnknownCellSet>::Get();
  }
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

namespace internal
{

struct UncertainCellSetSerializeFunctor
{
  template <typename CellSetType>
  void operator()(const CellSetType& cs, BinaryBuffer& bb) const
  {
    viskoresdiy::save(bb, viskores::cont::SerializableTypeString<CellSetType>::Get());
    viskoresdiy::save(bb, cs);
  }
};

struct UncertainCellSetDeserializeFunctor
{
  template <typename CellSetType>
  void operator()(CellSetType,
                  viskores::cont::UnknownCellSet& unknownCellSet,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    if (!success && (typeString == viskores::cont::SerializableTypeString<CellSetType>::Get()))
    {
      CellSetType knownCellSet;
      viskoresdiy::load(bb, knownCellSet);
      unknownCellSet = knownCellSet;
      success = true;
    }
  }
};

} // internal

template <typename CellSetList>
struct Serialization<viskores::cont::UncertainCellSet<CellSetList>>
{
  using Type = viskores::cont::UncertainCellSet<CellSetList>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(internal::UncertainCellSetSerializeFunctor{}, bb);
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    viskoresdiy::load(bb, typeString);

    bool success = false;
    viskores::ListForEach(
      internal::UncertainCellSetDeserializeFunctor{}, CellSetList{}, obj, typeString, success, bb);

    if (!success)
    {
      throw viskores::cont::ErrorBadType(
        "Error deserializing Unknown/UncertainCellSet. Message TypeString: " + typeString);
    }
  }
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

#endif //viskores_cont_UncertainCellSet_h
