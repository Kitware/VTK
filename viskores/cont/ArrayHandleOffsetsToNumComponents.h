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
#ifndef viskores_cont_ArrayHandleOffsetsToNumComponents_h
#define viskores_cont_ArrayHandleOffsetsToNumComponents_h

#include <viskores/cont/ArrayHandle.h>

namespace viskores
{
namespace internal
{

// Note that `ArrayPortalOffsetsToNumComponents` requires a source portal with +1 entry
// to avoid branching. See `ArrayHandleOffsetsToNumComponents` for details.
template <typename OffsetsPortal>
class VISKORES_ALWAYS_EXPORT ArrayPortalOffsetsToNumComponents
{
  OffsetsPortal Portal;

public:
  ArrayPortalOffsetsToNumComponents() = default;

  ArrayPortalOffsetsToNumComponents(const OffsetsPortal& portal)
    : Portal(portal)
  {
  }

  using ValueType = viskores::IdComponent;

  VISKORES_EXEC_CONT viskores::Id GetNumberOfValues() const
  {
    return this->Portal.GetNumberOfValues() - 1;
  }

  VISKORES_EXEC_CONT viskores::IdComponent Get(viskores::Id index) const
  {
    return static_cast<viskores::IdComponent>(this->Portal.Get(index + 1) -
                                              this->Portal.Get(index));
  }
};

}
} // namespace viskores::internal

namespace viskores
{
namespace cont
{

template <typename OffsetsStorageTag>
struct VISKORES_ALWAYS_EXPORT StorageTagOffsetsToNumComponents
{
};

namespace internal
{

template <typename OffsetsStorageTag>
class VISKORES_ALWAYS_EXPORT
  Storage<viskores::IdComponent,
          viskores::cont::StorageTagOffsetsToNumComponents<OffsetsStorageTag>>
{
  using OffsetsStorage = viskores::cont::internal::Storage<viskores::Id, OffsetsStorageTag>;

public:
  VISKORES_STORAGE_NO_RESIZE;
  VISKORES_STORAGE_NO_WRITE_PORTAL;

  using ReadPortalType =
    viskores::internal::ArrayPortalOffsetsToNumComponents<typename OffsetsStorage::ReadPortalType>;

  VISKORES_CONT static std::vector<viskores::cont::internal::Buffer> CreateBuffers()
  {
    return OffsetsStorage::CreateBuffers();
  }

  VISKORES_CONT static viskores::IdComponent GetNumberOfComponentsFlat(
    const std::vector<viskores::cont::internal::Buffer>&)
  {
    return 1;
  }

  VISKORES_CONT static viskores::Id GetNumberOfValues(
    const std::vector<viskores::cont::internal::Buffer>& buffers)
  {
    viskores::Id numOffsets = OffsetsStorage::GetNumberOfValues(buffers);
    if (numOffsets < 1)
    {
      throw viskores::cont::ErrorBadValue(
        "ArrayHandleOffsetsToNumComponents requires an offsets array with at least one value.");
    }
    return numOffsets - 1;
  }

  VISKORES_CONT static ReadPortalType CreateReadPortal(
    const std::vector<viskores::cont::internal::Buffer>& buffers,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token)
  {
    VISKORES_ASSERT(OffsetsStorage::GetNumberOfValues(buffers) > 0);
    return ReadPortalType(OffsetsStorage::CreateReadPortal(buffers, device, token));
  }
};

} // namespace internal

/// \brief An `ArrayHandle` that converts an array of offsets to an array of `Vec` sizes.
///
/// It is common in Viskores to pack small vectors of variable sizes into a single contiguous
/// array. For example, cells in an explicit cell set can each have a different amount of
/// vertices (triangles = 3, quads = 4, tetra = 4, hexa = 8, etc.). Generally, to access
/// items in this list, you need an array of components in each entry and the offset for
/// each entry. However, if you have just the array of offsets in sorted order, you can
/// easily derive the number of components for each entry by subtracting adjacent entries.
/// This works best if the offsets array has a size that is one more than the number of
/// packed vectors with the first entry set to 0 and the last entry set to the total size
/// of the packed array (the offset to the end).
///
/// `ArrayHandleOffsetsToNumComponents` decorates an array in exactly this manner. It
/// takes an offsets array and makes it behave like an array of counts. Note that the
/// offsets array must conform to the conditions described above: the offsets are in
/// sorted order and there is one additional entry in the offsets (ending in an offset
/// pointing past the end of the array).
///
/// When packing data of this nature, it is common to start with an array that is the
/// number of components. You can convert that to an offsets array using the
/// `viskores::cont::ConvertNumComponentsToOffsets` function. This will create an offsets array
/// with one extra entry as previously described. You can then throw out the original
/// number of components array and use the offsets with `ArrayHandleOffsetsToNumComponents`
/// to represent both the offsets and num components while storing only one array.
///
template <class OffsetsArray>
class VISKORES_ALWAYS_EXPORT ArrayHandleOffsetsToNumComponents
  : public viskores::cont::ArrayHandle<
      viskores::IdComponent,
      viskores::cont::StorageTagOffsetsToNumComponents<typename OffsetsArray::StorageTag>>
{
  VISKORES_IS_ARRAY_HANDLE(OffsetsArray);
  VISKORES_STATIC_ASSERT_MSG((std::is_same<typename OffsetsArray::ValueType, viskores::Id>::value),
                             "Offsets array must have a value type of viskores::Id.");

public:
  VISKORES_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleOffsetsToNumComponents,
    (ArrayHandleOffsetsToNumComponents<OffsetsArray>),
    (viskores::cont::ArrayHandle<
      viskores::IdComponent,
      viskores::cont::StorageTagOffsetsToNumComponents<typename OffsetsArray::StorageTag>>));

  VISKORES_CONT ArrayHandleOffsetsToNumComponents(const OffsetsArray& array)
    : Superclass(array.GetBuffers())
  {
  }
};

template <typename OffsetsStorageTag>
VISKORES_CONT viskores::cont::ArrayHandleOffsetsToNumComponents<
  viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageTag>>
make_ArrayHandleOffsetsToNumComponents(
  const viskores::cont::ArrayHandle<viskores::Id, OffsetsStorageTag>& array)
{
  // Converts to correct type.
  return array;
}

}
} // namespace viskores::cont

#endif //viskores_cont_ArrayHandleOffsetsToNumComponents_h
