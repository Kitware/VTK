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
#ifndef viskores_cont_ArrayHandleTransform_h
#define viskores_cont_ArrayHandleTransform_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorInternal.h>
#include <viskores/cont/ExecutionAndControlObjectBase.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/internal/ArrayPortalHelpers.h>

#include <viskores/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>

namespace viskores
{
namespace internal
{

/// Tag used in place of an inverse functor.
struct NullFunctorType
{
};

/// \brief An array portal that transforms a value from another portal.
///
template <typename ValueType_,
          typename PortalType_,
          typename FunctorType_,
          typename InverseFunctorType_ = NullFunctorType>
class VISKORES_ALWAYS_EXPORT ArrayPortalTransform;

template <typename ValueType_, typename PortalType_, typename FunctorType_>
class VISKORES_ALWAYS_EXPORT
  ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>
{
public:
  using PortalType = PortalType_;
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;

  VISKORES_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType())
    : Portal(portal)
    , Functor(functor)
  {
  }

  /// Copy constructor for any other ArrayPortalTransform with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV, class OtherP, class OtherF>
  VISKORES_EXEC_CONT ArrayPortalTransform(const ArrayPortalTransform<OtherV, OtherP, OtherF>& src)
    : Portal(src.GetPortal())
    , Functor(src.GetFunctor())
  {
  }

  VISKORES_EXEC_CONT
  viskores::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VISKORES_EXEC_CONT
  ValueType Get(viskores::Id index) const { return this->Functor(this->Portal.Get(index)); }

  VISKORES_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

  VISKORES_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

protected:
  PortalType Portal;
  FunctorType Functor;
};

template <typename ValueType_,
          typename PortalType_,
          typename FunctorType_,
          typename InverseFunctorType_>
class VISKORES_ALWAYS_EXPORT ArrayPortalTransform
  : public ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>
{
  using Writable = viskores::internal::PortalSupportsSets<PortalType_>;

public:
  using Superclass = ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>;
  using PortalType = PortalType_;
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;
  using InverseFunctorType = InverseFunctorType_;

  VISKORES_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(portal, functor)
    , InverseFunctor(inverseFunctor)
  {
  }

  template <class OtherV, class OtherP, class OtherF, class OtherInvF>
  VISKORES_EXEC_CONT ArrayPortalTransform(
    const ArrayPortalTransform<OtherV, OtherP, OtherF, OtherInvF>& src)
    : Superclass(src)
    , InverseFunctor(src.GetInverseFunctor())
  {
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VISKORES_EXEC_CONT void Set(viskores::Id index, const ValueType& value) const
  {
    this->Portal.Set(index, this->InverseFunctor(value));
  }

  VISKORES_EXEC_CONT
  const InverseFunctorType& GetInverseFunctor() const { return this->InverseFunctor; }

private:
  InverseFunctorType InverseFunctor;
};
}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

namespace internal
{

using NullFunctorType = viskores::internal::NullFunctorType;

template <typename ProvidedFunctorType, typename FunctorIsExecContObject>
struct TransformFunctorManagerImpl;

template <typename ProvidedFunctorType>
struct TransformFunctorManagerImpl<ProvidedFunctorType, std::false_type>
{
  VISKORES_STATIC_ASSERT_MSG(
    !viskores::cont::internal::IsExecutionObjectBase<ProvidedFunctorType>::value,
    "Must use an ExecutionAndControlObject instead of an ExecutionObject.");

  ProvidedFunctorType Functor;
  using FunctorType = ProvidedFunctorType;

  TransformFunctorManagerImpl() = default;

  VISKORES_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  VISKORES_CONT
  ProvidedFunctorType PrepareForControl() const { return this->Functor; }

  VISKORES_CONT ProvidedFunctorType PrepareForExecution(viskores::cont::DeviceAdapterId,
                                                        viskores::cont::Token&) const
  {
    return this->Functor;
  }
};

template <typename ProvidedFunctorType>
struct TransformFunctorManagerImpl<ProvidedFunctorType, std::true_type>
{
  VISKORES_IS_EXECUTION_AND_CONTROL_OBJECT(ProvidedFunctorType);

  ProvidedFunctorType Functor;
  //  using FunctorType = decltype(std::declval<ProvidedFunctorType>().PrepareForControl());
  //  using FunctorType = decltype(Functor.PrepareForControl());
  using FunctorType = viskores::cont::internal::ControlObjectType<ProvidedFunctorType>;

  TransformFunctorManagerImpl() = default;

  VISKORES_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  VISKORES_CONT
  auto PrepareForControl() const
    -> decltype(viskores::cont::internal::CallPrepareForControl(this->Functor))
  {
    return viskores::cont::internal::CallPrepareForControl(this->Functor);
  }

  VISKORES_CONT auto PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const
    -> decltype(viskores::cont::internal::CallPrepareForExecution(this->Functor, device, token))
  {
    return viskores::cont::internal::CallPrepareForExecution(this->Functor, device, token);
  }
};

template <typename ProvidedFunctorType>
struct TransformFunctorManager
  : TransformFunctorManagerImpl<
      ProvidedFunctorType,
      typename viskores::cont::internal::IsExecutionAndControlObjectBase<ProvidedFunctorType>::type>
{
  using Superclass = TransformFunctorManagerImpl<
    ProvidedFunctorType,
    typename viskores::cont::internal::IsExecutionAndControlObjectBase<ProvidedFunctorType>::type>;
  using FunctorType = typename Superclass::FunctorType;

  VISKORES_CONT TransformFunctorManager() = default;

  VISKORES_CONT TransformFunctorManager(const TransformFunctorManager&) = default;

  VISKORES_CONT TransformFunctorManager(const ProvidedFunctorType& functor)
    : Superclass(functor)
  {
  }

  template <typename ValueType>
  using TransformedValueType = decltype(std::declval<FunctorType>()(std::declval<ValueType>()));
};

template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType = NullFunctorType>
struct VISKORES_ALWAYS_EXPORT StorageTagTransform
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using ValueType =
    typename FunctorManager::template TransformedValueType<typename ArrayHandleType::ValueType>;
};

template <typename ArrayHandleType, typename FunctorType>
class Storage<typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
              StorageTagTransform<ArrayHandleType, FunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;

  using SourceStorage = typename ArrayHandleType::StorageType;

  static std::vector<viskores::cont::internal::Buffer> SourceBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 1, buffers.end());
  }

public:
  VISKORES_STORAGE_NO_RESIZE;
  VISKORES_STORAGE_NO_WRITE_PORTAL;

  using ReadPortalType =
    viskores::internal::ArrayPortalTransform<ValueType,
                                             typename ArrayHandleType::ReadPortalType,
                                             typename FunctorManager::FunctorType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<ValueType>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(SourceBuffers(buffers));
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    if (device == viskores::cont::DeviceAdapterTagUndefined{})
    {
      return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                            buffers[0].GetMetaData<FunctorManager>().PrepareForControl());
    }
    else
    {
      return ReadPortalType(
        SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
        buffers[0].GetMetaData<FunctorManager>().PrepareForExecution(device, token));
    }
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const ArrayHandleType& handle = ArrayHandleType{},
    const FunctorType& functor = FunctorType())
  {
    return viskores::cont::internal::CreateBuffers(FunctorManager(functor), handle);
  }

  VISKORES_CONT static ArrayHandleType GetArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                       typename ArrayHandleType::StorageTag>(
      SourceBuffers(buffers));
  }

  VISKORES_CONT static FunctorType GetFunctor(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<FunctorManager>().Functor;
  }

  VISKORES_CONT static NullFunctorType GetInverseFunctor(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return NullFunctorType{};
  }
};

template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class Storage<
  typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType,
  StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using InverseFunctorManager = TransformFunctorManager<InverseFunctorType>;
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;

  using SourceStorage = typename ArrayHandleType::StorageType;

  static std::vector<viskores::cont::internal::Buffer> SourceBuffers(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return std::vector<viskores::cont::internal::Buffer>(buffers.begin() + 2, buffers.end());
  }

public:
  using ReadPortalType =
    viskores::internal::ArrayPortalTransform<ValueType,
                                             typename ArrayHandleType::ReadPortalType,
                                             typename FunctorManager::FunctorType,
                                             typename InverseFunctorManager::FunctorType>;
  using WritePortalType =
    viskores::internal::ArrayPortalTransform<ValueType,
                                             typename ArrayHandleType::WritePortalType,
                                             typename FunctorManager::FunctorType,
                                             typename InverseFunctorManager::FunctorType>;

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return viskores::VecFlat<ValueType>::NUM_COMPONENTS;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return SourceStorage::GetNumberOfValues(SourceBuffers(buffers));
  }

  VISKORES_CONT static void ResizeBuffers(
    viskores::Id numValues,
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::CopyFlag preserve,
    viskores::cont::Token& token)
  {
    std::vector<viskores::cont::internal::Buffer> sourceBuffers = SourceBuffers(buffers);
    SourceStorage::ResizeBuffers(numValues, sourceBuffers, preserve, token);
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    if (device == viskores::cont::DeviceAdapterTagUndefined{})
    {
      return ReadPortalType(SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
                            buffers[0].GetMetaData<FunctorManager>().PrepareForControl(),
                            buffers[1].GetMetaData<InverseFunctorManager>().PrepareForControl());
    }
    else
    {
      return ReadPortalType(
        SourceStorage::CreateReadPortal(SourceBuffers(buffers), device, token),
        buffers[0].GetMetaData<FunctorManager>().PrepareForExecution(device, token),
        buffers[1].GetMetaData<InverseFunctorManager>().PrepareForExecution(device, token));
    }
  }

  VISKORES_CONT static WritePortalType CreateWritePortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    return WritePortalType(
      SourceStorage::CreateWritePortal(SourceBuffers(buffers), device, token),
      buffers[0].GetMetaData<FunctorManager>().PrepareForExecution(device, token),
      buffers[1].GetMetaData<InverseFunctorManager>().PrepareForExecution(device, token));
  }

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers(
    const ArrayHandleType& handle = ArrayHandleType{},
    const FunctorType& functor = FunctorType(),
    const InverseFunctorType& inverseFunctor = InverseFunctorType())
  {
    return viskores::cont::internal::CreateBuffers(
      FunctorManager(functor), InverseFunctorManager(inverseFunctor), handle);
  }

  VISKORES_CONT static ArrayHandleType GetArray(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return viskores::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                       typename ArrayHandleType::StorageTag>(
      SourceBuffers(buffers));
  }

  VISKORES_CONT static FunctorType GetFunctor(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[0].GetMetaData<FunctorManager>().Functor;
  }

  VISKORES_CONT static InverseFunctorType GetInverseFunctor(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    return buffers[1].GetMetaData<InverseFunctorManager>().Functor;
  }
};

} // namespace internal

/// \brief Implicitly transform values of one array to another with a functor.
///
/// ArrayHandleTransforms is a specialization of ArrayHandle. It takes a
/// delegate array handle and makes a new handle that calls a given unary
/// functor with the element at a given index and returns the result of the
/// functor as the value of this array at that position. This transformation is
/// done on demand. That is, rather than make a new copy of the array with new
/// values, the transformation is done as values are read from the array. Thus,
/// the functor operator should work in both the control and execution
/// environments.
///
template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType = internal::NullFunctorType>
class ArrayHandleTransform;

template <typename ArrayHandleType, typename FunctorType>
class ArrayHandleTransform<ArrayHandleType, FunctorType, internal::NullFunctorType>
  : public viskores::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleTransform,
    (ArrayHandleTransform<ArrayHandleType, FunctorType>),
    (viskores::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType>>));

  VISKORES_CONT
  ArrayHandleTransform(const ArrayHandleType& handle,
                       const FunctorType& functor = FunctorType{},
                       internal::NullFunctorType = internal::NullFunctorType{})
    : Superclass(StorageType::CreateBuffers(handle, functor))
  {
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.
template <typename HandleType, typename FunctorType>
VISKORES_CONT viskores::cont::ArrayHandleTransform<HandleType, FunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor)
{
  return ArrayHandleTransform<HandleType, FunctorType>(handle, functor);
}

// ArrayHandleTransform with inverse functors enabled (no need to subclass from
// ArrayHandleTransform without inverse functors: nothing to inherit).
template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class ArrayHandleTransform
  : public viskores::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::
        ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleTransform,
    (ArrayHandleTransform<ArrayHandleType, FunctorType, InverseFunctorType>),
    (viskores::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::
        ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>));

  ArrayHandleTransform(const ArrayHandleType& handle,
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(StorageType::CreateBuffers(handle, functor, inverseFunctor))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleTransform() {}

  /// \brief Returns the `ArrayHandle` that is being transformed.
  ///
  ArrayHandleType GetTransformedArray() const { return StorageType::GetArray(this->GetBuffers()); }

  /// \brief Returns the functor transforming the `ArrayHandle`.
  ///
  FunctorType GetFunctor() const { return StorageType::GetFunctor(this->GetBuffers()); }

  /// \brief Returns the inverse functor transforming the `ArrayHandle`
  ///
  InverseFunctorType GetInverseFunctor() const
  {
    return StorageType::GetInverseFunctor(this->GetBuffers());
  }
};

template <typename HandleType, typename FunctorType, typename InverseFunctorType>
VISKORES_CONT viskores::cont::ArrayHandleTransform<HandleType, FunctorType, InverseFunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor, InverseFunctorType inverseFunctor)
{
  return ArrayHandleTransform<HandleType, FunctorType, InverseFunctorType>(
    handle, functor, inverseFunctor);
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

template <typename AH, typename Functor, typename InvFunctor>
struct SerializableTypeString<viskores::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Transform<" + SerializableTypeString<AH>::Get() + "," +
      SerializableTypeString<Functor>::Get() + "," + SerializableTypeString<InvFunctor>::Get() +
      ">";
    return name;
  }
};

template <typename AH, typename Functor>
struct SerializableTypeString<viskores::cont::ArrayHandleTransform<AH, Functor>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "AH_Transform<" + SerializableTypeString<AH>::Get() + "," +
      SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct SerializableTypeString<viskores::cont::ArrayHandle<
  typename viskores::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>::ValueType,
  viskores::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>>>
  : SerializableTypeString<viskores::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
};
}
} // viskores::cont

namespace mangled_diy_namespace
{

template <typename AH, typename Functor>
struct Serialization<viskores::cont::ArrayHandleTransform<AH, Functor>>
{
private:
  using Type = viskores::cont::ArrayHandleTransform<AH, Functor>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type transformedArray = obj;
    viskoresdiy::save(bb, obj.GetArray());
    viskoresdiy::save(bb, obj.GetFunctor());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    viskoresdiy::load(bb, array);
    Functor functor;
    viskoresdiy::load(bb, functor);
    obj = viskores::cont::make_ArrayHandleTransform(array, functor);
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct Serialization<viskores::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
private:
  using Type = viskores::cont::ArrayHandleTransform<AH, Functor, InvFunctor>;
  using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VISKORES_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type transformedArray = obj;
    viskoresdiy::save(bb, transformedArray.GetTransformedArray());
    viskoresdiy::save(bb, transformedArray.GetFunctor());
    viskoresdiy::save(bb, transformedArray.GetInverseFunctor());
  }

  static VISKORES_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    viskoresdiy::load(bb, array);
    Functor functor;
    viskoresdiy::load(bb, functor);
    InvFunctor invFunctor;
    viskoresdiy::load(bb, invFunctor);
    obj = viskores::cont::make_ArrayHandleTransform(array, functor, invFunctor);
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct Serialization<viskores::cont::ArrayHandle<
  typename viskores::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>::ValueType,
  viskores::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>>>
  : Serialization<viskores::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleTransform_h
