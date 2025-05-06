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
#ifndef viskores_cont_UnknownCellSet_h
#define viskores_cont_UnknownCellSet_h

#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/CellSet.h>
#include <viskores/cont/DefaultTypes.h>

#include <viskores/cont/viskores_cont_export.h>

#include <memory>

namespace viskores
{
namespace cont
{

// Forward declaration.
template <typename CellSetList>
class UncertainCellSet;

/// \brief A CellSet of an unknown type.
///
/// `UnknownCellSet` holds a `CellSet` object using runtime polymorphism to manage
/// the dynamic type rather than compile-time templates. This adds a programming
/// convenience that helps avoid a proliferation of templates.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in Viskores, `UnknownCellSet` contains a method named `CastAndCallForTypes` that
/// determines the correct type from some known list of types. This mechanism is
/// used internally by Viskores's worklet invocation mechanism to determine the type
/// when running algorithms.
///
/// If the `UnknownCellSet` is used in a context where the possible cell set types
/// can be whittled down to a finite list, you can specify lists of cell set types
/// using the `ResetCellSetList` method. This will convert this object to an
/// `UncertainCellSet` of the given types. In cases where a finite set of types
/// are needed but there is no subset, `VISKORES_DEFAULT_CELL_SET_LIST`
///
class VISKORES_CONT_EXPORT UnknownCellSet
{
  std::shared_ptr<viskores::cont::CellSet> Container;

  void InitializeKnownOrUnknownCellSet(const UnknownCellSet& cellSet,
                                       std::true_type viskoresNotUsed(isUnknownCellSet))
  {
    *this = cellSet;
  }

  template <typename CellSetType>
  void InitializeKnownOrUnknownCellSet(const CellSetType& cellSet,
                                       std::false_type viskoresNotUsed(isUnknownCellSet))
  {
    VISKORES_IS_CELL_SET(CellSetType);
    this->Container = std::shared_ptr<viskores::cont::CellSet>(new CellSetType(cellSet));
  }

public:
  VISKORES_CONT UnknownCellSet() = default;

  template <typename CellSetType>
  VISKORES_CONT UnknownCellSet(const CellSetType& cellSet)
  {
    this->InitializeKnownOrUnknownCellSet(
      cellSet, typename std::is_base_of<UnknownCellSet, CellSetType>::type{});
  }

  /// \brief Returns whether a cell set is stored in this `UnknownCellSet`.
  ///
  /// If the `UnknownCellSet` is constructed without a `CellSet`, it will not
  /// have an underlying type, and therefore the operations will be invalid.
  ///
  VISKORES_CONT bool IsValid() const { return static_cast<bool>(this->Container); }

  /// \brief Returns a pointer to the `CellSet` base class.
  ///
  VISKORES_CONT viskores::cont::CellSet* GetCellSetBase() { return this->Container.get(); }
  VISKORES_CONT const viskores::cont::CellSet* GetCellSetBase() const
  {
    return this->Container.get();
  }

  /// \brief Create a new cell set of the same type as this cell set.
  ///
  /// This method creates a new cell set that is the same type as this one
  /// and returns a new `UnknownCellSet` for it. This method is convenient
  /// when creating output cell sets that should be the same type as the
  /// input cell set.
  ///
  VISKORES_CONT UnknownCellSet NewInstance() const;

  /// \brief Returns the name of the cell set type stored in this class.
  ///
  /// Returns an empty string if no cell set is stored.
  ///
  VISKORES_CONT std::string GetCellSetName() const;

  /// \brief Returns true if this cell set matches the `CellSetType` template argument.
  ///
  template <typename CellSetType>
  VISKORES_CONT bool IsType() const
  {
    return (dynamic_cast<const CellSetType*>(this->Container.get()) != nullptr);
  }

  VISKORES_CONT viskores::Id GetNumberOfCells() const
  {
    return this->Container ? this->Container->GetNumberOfCells() : 0;
  }
  VISKORES_CONT viskores::Id GetNumberOfFaces() const
  {
    return this->Container ? this->Container->GetNumberOfFaces() : 0;
  }
  VISKORES_CONT viskores::Id GetNumberOfEdges() const
  {
    return this->Container ? this->Container->GetNumberOfEdges() : 0;
  }
  VISKORES_CONT viskores::Id GetNumberOfPoints() const
  {
    return this->Container ? this->Container->GetNumberOfPoints() : 0;
  }

  VISKORES_CONT viskores::UInt8 GetCellShape(viskores::Id id) const
  {
    return this->GetCellSetBase()->GetCellShape(id);
  }
  VISKORES_CONT viskores::IdComponent GetNumberOfPointsInCell(viskores::Id id) const
  {
    return this->GetCellSetBase()->GetNumberOfPointsInCell(id);
  }
  VISKORES_CONT void GetCellPointIds(viskores::Id id, viskores::Id* ptids) const
  {
    return this->GetCellSetBase()->GetCellPointIds(id, ptids);
  }

  VISKORES_CONT void DeepCopyFrom(const CellSet* src) { this->GetCellSetBase()->DeepCopy(src); }

  VISKORES_CONT void PrintSummary(std::ostream& os) const;

  VISKORES_CONT void ReleaseResourcesExecution()
  {
    if (this->Container)
    {
      this->Container->ReleaseResourcesExecution();
    }
  }

  /// \brief Returns true if this cell set can be retrieved as the given type.
  ///
  /// This method will return true if calling `AsCellSet` of the given type will
  /// succeed. This result is similar to `IsType`, and if `IsType` returns true,
  /// then this will return true. However, this method will also return true for
  /// other types where automatic conversions are made.
  ///
  template <typename CellSetType>
  VISKORES_CONT bool CanConvert() const
  {
    // TODO: Currently, these are the same. But in the future we expect to support
    // special CellSet types that can convert back and forth such as multiplexed
    // cell sets or a cell set that can hold structured grids of any dimension.
    return this->IsType<CellSetType>();
  }

  ///@{
  /// \brief Get the cell set as a known type.
  ///
  /// Returns this cell set cast appropriately and stored in the given `CellSet`
  /// type. Throws an `ErrorBadType` if the stored cell set cannot be stored in
  /// the given cell set type. Use the `CanConvert` method to determine if the
  /// cell set can be returned with the given type.
  ///
  template <typename CellSetType>
  VISKORES_CONT void AsCellSet(CellSetType& cellSet) const
  {
    VISKORES_IS_CELL_SET(CellSetType);
    CellSetType* cellSetPointer = dynamic_cast<CellSetType*>(this->Container.get());
    if (cellSetPointer == nullptr)
    {
      VISKORES_LOG_CAST_FAIL(*this, CellSetType);
      throwFailedDynamicCast(this->GetCellSetName(), viskores::cont::TypeToString(cellSet));
    }
    VISKORES_LOG_CAST_SUCC(*this, *cellSetPointer);
    cellSet = *cellSetPointer;
  }

  template <typename CellSetType>
  VISKORES_CONT CellSetType AsCellSet() const
  {
    CellSetType cellSet;
    this->AsCellSet(cellSet);
    return cellSet;
  }
  ///@}

  /// \brief Assigns potential cell set types.
  ///
  /// Calling this method will return an `UncertainCellSet` with the provided
  /// cell set list. The returned object will hold the same `CellSet`, but
  /// `CastAndCall`'s on the returned object will be constrained to the given
  /// types.
  ///
  // Defined in UncertainCellSet.h
  template <typename CellSetList>
  VISKORES_CONT viskores::cont::UncertainCellSet<CellSetList> ResetCellSetList(CellSetList) const;
  template <typename CellSetList>
  VISKORES_CONT viskores::cont::UncertainCellSet<CellSetList> ResetCellSetList() const;

  /// \brief Call a functor using the underlying cell set type.
  ///
  /// `CastAndCallForTypes` attemts to cast the held cell set to a specific type
  /// and then calls the given functor with the cast cell set. You must specify
  /// the `CellSetList` (in a `viskores::List`) as a template argument.
  ///
  /// After the functor argument, you may add any number of arguments that will
  /// be passed to the functor after the converted cell set.
  ///
  template <typename CellSetList, typename Functor, typename... Args>
  VISKORES_CONT void CastAndCallForTypes(Functor&& functor, Args&&... args) const;
};

//=============================================================================
// Free function casting helpers
// (Not sure if these should be deprecated.)

/// Returns true if `unknownCellSet` matches the type of `CellSetType`.
///
template <typename CellSetType>
VISKORES_CONT inline bool IsType(const viskores::cont::UnknownCellSet& unknownCellSet)
{
  return unknownCellSet.IsType<CellSetType>();
}

/// Returns `unknownCellSet` cast to the given `CellSet` type. Throws
/// `ErrorBadType` if the cast does not work. Use `IsType`
/// to check if the cast can happen.
///
template <typename CellSetType>
VISKORES_CONT inline CellSetType Cast(const viskores::cont::UnknownCellSet& unknownCellSet)
{
  return unknownCellSet.AsCellSet<CellSetType>();
}

namespace internal
{

VISKORES_CONT_EXPORT void ThrowCastAndCallException(const viskores::cont::UnknownCellSet&,
                                                    const std::type_info&);

template <>
struct DynamicTransformTraits<viskores::cont::UnknownCellSet>
{
  using DynamicTag = viskores::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal

template <typename CellSetList, typename Functor, typename... Args>
VISKORES_CONT void UnknownCellSet::CastAndCallForTypes(Functor&& functor, Args&&... args) const
{
  VISKORES_IS_LIST(CellSetList);
  bool called = false;
  viskores::ListForEach(
    [&](auto cellSet)
    {
      if (!called && this->CanConvert<decltype(cellSet)>())
      {
        called = true;
        this->AsCellSet(cellSet);
        VISKORES_LOG_CAST_SUCC(*this, cellSet);

        // If you get a compile error here, it means that you have called CastAndCall for a
        // viskores::cont::UnknownCellSet and the arguments of the functor do not match those
        // being passed. This is often because it is calling the functor with a CellSet
        // type that was not expected. Either add overloads to the functor to accept all
        // possible cell set types or constrain the types tried for the CastAndCall.
        functor(cellSet, args...);
      }
    },
    CellSetList{});

  if (!called)
  {
    VISKORES_LOG_CAST_FAIL(*this, CellSetList);
    internal::ThrowCastAndCallException(*this, typeid(CellSetList));
  }
}

/// A specialization of `CastAndCall` for `UnknownCellSet`.
/// Since we have no hints on the types, use `VISKORES_DEFAULT_CELL_SET_LIST`.
template <typename Functor, typename... Args>
void CastAndCall(const viskores::cont::UnknownCellSet& cellSet, Functor&& f, Args&&... args)
{
  cellSet.CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST>(std::forward<Functor>(f),
                                                              std::forward<Args>(args)...);
}

namespace internal
{

/// Checks to see if the given object is an unknown (or uncertain) cell set. It
/// resolves to either `std::true_type` or `std::false_type`.
///
template <typename T>
using UnknownCellSetCheck = typename std::is_base_of<viskores::cont::UnknownCellSet, T>::type;

#define VISKORES_IS_UNKNOWN_CELL_SET(T) \
  VISKORES_STATIC_ASSERT(::viskores::cont::internal::UnknownCellSetCheck<T>::value)

#define VISKORES_IS_KNOWN_OR_UNKNOWN_CELL_SET(T)                                     \
  VISKORES_STATIC_ASSERT(::viskores::cont::internal::CellSetCheck<T>::type::value || \
                         ::viskores::cont::internal::UnknownCellSetCheck<T>::value)

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

template <>
struct VISKORES_CONT_EXPORT SerializableTypeString<viskores::cont::UnknownCellSet>
{
  static VISKORES_CONT std::string Get();
};
}
} // namespace viskores::cont

namespace mangled_diy_namespace
{

template <>
struct VISKORES_CONT_EXPORT Serialization<viskores::cont::UnknownCellSet>
{
public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const viskores::cont::UnknownCellSet& obj);
  static VISKORES_CONT void load(BinaryBuffer& bb, viskores::cont::UnknownCellSet& obj);
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

// Include the implementation of UncertainCellSet. This should be included because there
// are methods in UnknownCellSet that produce objects of this type. It has to be included
// at the end to resolve the circular dependency.
#include <viskores/cont/UncertainCellSet.h>

#endif //viskores_cont_UnknownCellSet_h
