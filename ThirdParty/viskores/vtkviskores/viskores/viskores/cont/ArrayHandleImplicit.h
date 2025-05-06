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
#ifndef viskores_cont_ArrayHandleImplicit_h
#define viskores_cont_ArrayHandleImplicit_h

#include <viskores/cont/ArrayHandle.h>

#include <viskoresstd/is_trivial.h>

namespace viskores
{

namespace internal
{

/// \brief An array portal that returns the result of a functor
///
/// This array portal is similar to an implicit array i.e an array that is
/// defined functionally rather than actually stored in memory. The array
/// comprises a functor that is called for each index.
///
/// The \c ArrayPortalImplicit is used in an ArrayHandle with an
/// \c StorageImplicit container.
///
template <class FunctorType_>
class VISKORES_ALWAYS_EXPORT ArrayPortalImplicit
{
public:
  using FunctorType = FunctorType_;
  using ValueType = decltype(FunctorType{}(viskores::Id{}));

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalImplicit()
    : Functor()
    , NumberOfValues(0)
  {
  }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ArrayPortalImplicit(FunctorType f, viskores::Id numValues)
    : Functor(f)
    , NumberOfValues(numValues)
  {
  }

  VISKORES_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VISKORES_SUPPRESS_EXEC_WARNINGS
  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return this->Functor(index); }

private:
  FunctorType Functor;
  viskores::Id NumberOfValues;
};

} // namespace internal

namespace cont
{

/// \brief An implementation for read-only implicit arrays.
///
/// It is sometimes the case that you want Viskores to operate on an array of
/// implicit values. That is, rather than store the data in an actual array, it
/// is gerenated on the fly by a function. This is handled in Viskores by creating
/// an ArrayHandle in Viskores with a StorageTagImplicit type of \c Storage. This
/// tag itself is templated to specify an ArrayPortal that generates the
/// desired values. An ArrayHandle created with this tag will raise an error on
/// any operation that tries to modify it.
///
template <class ArrayPortalType>
struct VISKORES_ALWAYS_EXPORT StorageTagImplicit
{
  using PortalType = ArrayPortalType;
};

namespace internal
{

/// Given an array portal, returns the buffers for the `ArrayHandle` with a storage that
/// is (or is compatible with) a storage tag of `StorageTagImplicit<PortalType>`.
template <typename PortalType>
VISKORES_CONT inline std::vector<viskores::cont::internal::Buffer>
PortalToArrayHandleImplicitBuffers(const PortalType& portal)
{
  std::vector<viskores::cont::internal::Buffer> buffers(1);
  buffers[0].SetMetaData(portal);
  return buffers;
}

/// Given a functor and the number of values, returns the buffers for the `ArrayHandleImplicit`
/// for the given functor.
template <typename FunctorType>
VISKORES_CONT inline std::vector<viskores::cont::internal::Buffer>
FunctorToArrayHandleImplicitBuffers(const FunctorType& functor, viskores::Id numValues)
{
  return PortalToArrayHandleImplicitBuffers(
    viskores::internal::ArrayPortalImplicit<FunctorType>(functor, numValues));
}

template <class ArrayPortalType>
struct VISKORES_ALWAYS_EXPORT
  Storage<typename ArrayPortalType::ValueType, StorageTagImplicit<ArrayPortalType>>
{
  VISKORES_IS_TRIVIALLY_COPYABLE(ArrayPortalType);

  VISKORES_STORAGE_NO_RESIZE;
  VISKORES_STORAGE_NO_WRITE_PORTAL;

  using ReadPortalType = ArrayPortalType;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return viskores::cont::internal::PortalToArrayHandleImplicitBuffers(ArrayPortalType{});
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<typename ArrayPortalType::ValueType>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<ArrayPortalType>().GetNumberOfValues();
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId,
    viskores::cont::Token&)
  {
    return buffers[0].GetMetaData<ArrayPortalType>();
  }
};

} // namespace internal

namespace detail
{

/// A convenience class that provides a typedef to the appropriate tag for
/// a implicit array container.
template <typename FunctorType>
struct ArrayHandleImplicitTraits
{
  using ValueType = decltype(FunctorType{}(viskores::Id{}));
  using PortalType = viskores::internal::ArrayPortalImplicit<FunctorType>;
  using StorageTag = viskores::cont::StorageTagImplicit<PortalType>;
  using Superclass = viskores::cont::ArrayHandle<ValueType, StorageTag>;
};

} // namespace detail

/// \brief An \c ArrayHandle that computes values on the fly.
///
/// \c ArrayHandleImplicit is a specialization of ArrayHandle.
/// It takes a user defined functor which is called with a given index value.
/// The functor returns the result of the functor as the value of this
/// array at that position.
///
template <class FunctorType>
class VISKORES_ALWAYS_EXPORT ArrayHandleImplicit
  : public detail::ArrayHandleImplicitTraits<FunctorType>::Superclass
{
private:
  using ArrayTraits = typename detail::ArrayHandleImplicitTraits<FunctorType>;
  using PortalType = typename ArrayTraits::PortalType;

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleImplicit,
                                 (ArrayHandleImplicit<FunctorType>),
                                 (typename ArrayTraits::Superclass));

  VISKORES_CONT
  ArrayHandleImplicit(FunctorType functor, viskores::Id length)
    : Superclass(internal::PortalToArrayHandleImplicitBuffers(PortalType(functor, length)))
  {
  }
};

/// make_ArrayHandleImplicit is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// arry.

template <typename FunctorType>
VISKORES_CONT viskores::cont::ArrayHandleImplicit<FunctorType> make_ArrayHandleImplicit(
  FunctorType functor,
  viskores::Id length)
{
  return ArrayHandleImplicit<FunctorType>(functor, length);
}
}
} // namespace viskores::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores
{
namespace cont
{

template <typename Functor>
struct SerializableTypeString<viskores::cont::ArrayHandleImplicit<Functor>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Implicit<" + SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename Functor>
struct SerializableTypeString<viskores::cont::ArrayHandle<
  typename viskores::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  viskores::cont::StorageTagImplicit<viskores::internal::ArrayPortalImplicit<Functor>>>>
  : SerializableTypeString<viskores::cont::ArrayHandleImplicit<Functor>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename Functor>
struct Serialization<viskores::cont::ArrayHandleImplicit<Functor>>
{
private:
  using Type = viskores::cont::ArrayHandleImplicit<Functor>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    viskoresdiy::save(bb, obj.GetNumberOfValues());
    viskoresdiy::save(bb, obj.ReadPortal().GetFunctor());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    viskores::Id count = 0;
    viskoresdiy::load(bb, count);

    Functor functor;
    viskoresdiy::load(bb, functor);

    obj = viskores::cont::make_ArrayHandleImplicit(functor, count);
  }
};

template <typename Functor>
struct Serialization<viskores::cont::ArrayHandle<
  typename viskores::cont::detail::ArrayHandleImplicitTraits<Functor>::ValueType,
  viskores::cont::StorageTagImplicit<viskores::internal::ArrayPortalImplicit<Functor>>>>
  : Serialization<viskores::cont::ArrayHandleImplicit<Functor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleImplicit_h
