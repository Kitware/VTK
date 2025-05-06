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

#include <viskores/cont/internal/ArrayPortalFromIterators.h>
#include <viskores/cont/internal/Buffer.h>

#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

using T = viskores::FloatDefault;
constexpr viskores::Id ARRAY_SIZE = 20;

using PortalType = viskores::cont::internal::ArrayPortalFromIterators<T*>;
using PortalTypeConst = viskores::cont::internal::ArrayPortalFromIterators<const T*>;

struct TestMetaData
{
  viskores::Id Value = 0;
};

constexpr viskores::Id METADATA_VALUE = 42;

bool CheckMetaData(const viskores::cont::internal::Buffer& buffer)
{
  return buffer.GetMetaData<TestMetaData>().Value == METADATA_VALUE;
}

PortalType MakePortal(void* buffer, viskores::Id numValues)
{
  return PortalType(static_cast<T*>(buffer),
                    static_cast<T*>(buffer) + static_cast<std::size_t>(numValues));
};

PortalTypeConst MakePortal(const void* buffer, viskores::Id numValues)
{
  return PortalTypeConst(static_cast<const T*>(buffer),
                         static_cast<const T*>(buffer) + static_cast<std::size_t>(numValues));
};

void VectorDeleter(void* container)
{
  std::vector<T>* v = reinterpret_cast<std::vector<T>*>(container);
  delete v;
}

void VectorReallocator(void*& memory,
                       void*& container,
                       viskores::BufferSizeType oldSize,
                       viskores::BufferSizeType newSize)
{
  std::vector<T>* v = reinterpret_cast<std::vector<T>*>(container);
  VISKORES_TEST_ASSERT(v->size() == static_cast<std::size_t>(oldSize));
  VISKORES_TEST_ASSERT(v->empty() || (memory == v->data()));

  v->resize(static_cast<std::size_t>(newSize));
  memory = v->data();
}
struct VectorDeleter
{
  std::shared_ptr<std::vector<T>> Data;

  VectorDeleter(viskores::Id numValues)
    : Data(new std::vector<T>(static_cast<size_t>(numValues)))
  {
  }

  template <typename U>
  void operator()(U* p)
  {
    if (this->Data)
    {
      VISKORES_TEST_ASSERT(reinterpret_cast<T*>(p) == this->Data->data());
      this->Data.reset();
    }
  }
};


void DoTest()
{
  constexpr viskores::Id BUFFER_SIZE = ARRAY_SIZE * static_cast<viskores::Id>(sizeof(T));
  constexpr viskores::cont::DeviceAdapterTagSerial device;

  viskores::cont::internal::Buffer buffer;

  {
    TestMetaData metadata;
    metadata.Value = METADATA_VALUE;
    buffer.SetMetaData(metadata);
    VISKORES_TEST_ASSERT(CheckMetaData(buffer));
  }

  std::cout << "Copy uninitialized buffer" << std::endl;
  {
    viskores::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VISKORES_TEST_ASSERT(copy.GetNumberOfBytes() == 0);
    VISKORES_TEST_ASSERT(CheckMetaData(copy));
  }

  std::cout << "Initialize buffer" << std::endl;
  {
    viskores::cont::Token token;
    buffer.SetNumberOfBytes(BUFFER_SIZE, viskores::CopyFlag::Off, token);
  }

  VISKORES_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE);

  std::cout << "Copy sized but uninitialized buffer" << std::endl;
  {
    viskores::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VISKORES_TEST_ASSERT(copy.GetNumberOfBytes() == BUFFER_SIZE);
    VISKORES_TEST_ASSERT(CheckMetaData(copy));
    VISKORES_TEST_ASSERT(!copy.IsAllocatedOnHost());
    VISKORES_TEST_ASSERT(!copy.IsAllocatedOnDevice(device));
  }

  std::cout << "Fill up values on host" << std::endl;
  {
    viskores::cont::Token token;
    SetPortal(MakePortal(buffer.WritePointerHost(token), ARRAY_SIZE));
  }
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VISKORES_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Check values on host" << std::endl;
  {
    viskores::cont::Token token;
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
  }
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VISKORES_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Copy buffer with host data" << std::endl;
  {
    viskores::cont::Token token;
    viskores::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VISKORES_TEST_ASSERT(copy.GetNumberOfBytes() == BUFFER_SIZE);
    VISKORES_TEST_ASSERT(CheckMetaData(copy));
    VISKORES_TEST_ASSERT(copy.IsAllocatedOnHost());
    VISKORES_TEST_ASSERT(!copy.IsAllocatedOnDevice(device));
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
  }

  std::cout << "Check values on device" << std::endl;
  {
    viskores::cont::Token token;
    CheckPortal(MakePortal(buffer.ReadPointerDevice(device, token), ARRAY_SIZE));
  }
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Resize array and access write on device" << std::endl;
  {
    viskores::cont::Token token;
    buffer.SetNumberOfBytes(BUFFER_SIZE / 2, viskores::CopyFlag::On, token);
    VISKORES_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE / 2);
    CheckPortal(MakePortal(buffer.WritePointerDevice(device, token), ARRAY_SIZE / 2));
  }
  VISKORES_TEST_ASSERT(!buffer.IsAllocatedOnHost());
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Resize array and access write on host" << std::endl;
  // Note that this is a weird corner case where the array was resized while saving the data
  // and then requested on another device.
  {
    viskores::cont::Token token;
    buffer.SetNumberOfBytes(BUFFER_SIZE * 2, viskores::CopyFlag::On, token);
    VISKORES_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE * 2);
    // Although the array is twice ARRAY_SIZE, the valid values are only ARRAY_SIZE/2
    CheckPortal(MakePortal(buffer.WritePointerHost(token), ARRAY_SIZE / 2));
  }
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnHost());
  VISKORES_TEST_ASSERT(!buffer.IsAllocatedOnDevice(device));

  std::cout << "Fill buffer" << std::endl;
  {
    constexpr viskores::BufferSizeType fillValueSize =
      static_cast<viskores::BufferSizeType>(sizeof(T));
    viskores::cont::Token token;
    T fillValue1 = 1.234f;
    T fillValue2 = 5.678f;
    buffer.Fill(&fillValue1, fillValueSize, 0, BUFFER_SIZE * 2, token);
    buffer.Fill(&fillValue2, fillValueSize, BUFFER_SIZE / 2, BUFFER_SIZE, token);
    const T* array = reinterpret_cast<const T*>(buffer.ReadPointerHost(token));
    for (viskores::Id index = 0; index < ARRAY_SIZE / 2; ++index)
    {
      VISKORES_TEST_ASSERT(array[index] == fillValue1);
    }
    for (viskores::Id index = ARRAY_SIZE / 2; index < ARRAY_SIZE; ++index)
    {
      VISKORES_TEST_ASSERT(array[index] == fillValue2);
    }
    for (viskores::Id index = ARRAY_SIZE; index < ARRAY_SIZE * 2; ++index)
    {
      VISKORES_TEST_ASSERT(array[index] == fillValue1);
    }
  }

  std::cout << "Reset with device data" << std::endl;
  std::vector<T> v(ARRAY_SIZE);
  void* devicePointer = v.data();
  SetPortal(MakePortal(devicePointer, ARRAY_SIZE));
  buffer.Reset(viskores::cont::internal::BufferInfo(device,
                                                    devicePointer,
                                                    new std::vector<T>(std::move(v)),
                                                    BUFFER_SIZE,
                                                    VectorDeleter,
                                                    VectorReallocator));
  VISKORES_TEST_ASSERT(buffer.GetNumberOfBytes() == BUFFER_SIZE);
  VISKORES_TEST_ASSERT(!buffer.IsAllocatedOnHost());
  VISKORES_TEST_ASSERT(buffer.IsAllocatedOnDevice(device));

  std::cout << "Make sure device pointer is as expected" << std::endl;
  {
    viskores::cont::Token token;
    VISKORES_TEST_ASSERT(buffer.WritePointerDevice(device, token) == devicePointer);
  }

  std::cout << "Copy buffer with device data" << std::endl;
  {
    viskores::cont::Token token;
    viskores::cont::internal::Buffer copy;
    copy.DeepCopyFrom(buffer);
    VISKORES_TEST_ASSERT(copy.GetNumberOfBytes() == BUFFER_SIZE);
    VISKORES_TEST_ASSERT(CheckMetaData(copy));
    VISKORES_TEST_ASSERT(!copy.IsAllocatedOnHost());
    VISKORES_TEST_ASSERT(copy.IsAllocatedOnDevice(device));
    CheckPortal(MakePortal(buffer.ReadPointerDevice(device, token), ARRAY_SIZE));
  }

  std::cout << "Pull data to host" << std::endl;
  {
    viskores::cont::Token token;
    CheckPortal(MakePortal(buffer.ReadPointerHost(token), ARRAY_SIZE));
  }
}

} // anonymous namespace

int UnitTestBuffer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
