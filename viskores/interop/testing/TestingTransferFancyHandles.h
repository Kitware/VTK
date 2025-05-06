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
#ifndef viskores_cont_testing_TestingFancyArrayHandles_h
#define viskores_cont_testing_TestingFancyArrayHandles_h

#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/interop/TransferToOpenGL.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace viskores
{
namespace interop
{
namespace testing
{

namespace
{
template <typename T>
viskores::cont::ArrayHandle<T> makeArray(viskores::Id length, T)
{
  viskores::cont::ArrayHandle<T> data;
  data.Allocate(length);

  auto portal = data.WritePortal();
  for (viskores::Id i = 0; i != data.GetNumberOfValues(); ++i)
  {
    portal.Set(i, TestValue(i, T()));
  }
  return data;
}

//bring the data back from openGL and into a std vector. Will bind the
//passed in handle to the default buffer type for the type T
template <typename T>
std::vector<T> CopyGLBuffer(GLuint& handle, T t)
{
  //get the type we used for this buffer.
  GLenum type = viskores::interop::internal::BufferTypePicker(t);

  //bind the buffer to the guessed buffer type, this way
  //we can call CopyGLBuffer no matter what it the active buffer
  glBindBuffer(type, handle);

  //get the size of the buffer
  int bytesInBuffer = 0;
  glGetBufferParameteriv(type, GL_BUFFER_SIZE, &bytesInBuffer);
  const std::size_t size = (static_cast<std::size_t>(bytesInBuffer) / sizeof(T));

  //get the buffer contents and place it into a vector
  std::vector<T> data;
  data.resize(size);
  glGetBufferSubData(type, 0, bytesInBuffer, &data[0]);

  return data;
}

template <typename T, typename U>
void validate(viskores::cont::ArrayHandle<T, U> handle, viskores::interop::BufferState& state)
{
  GLboolean is_buffer;
  is_buffer = glIsBuffer(*state.GetHandle());
  VISKORES_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");
  std::vector<T> returnedValues = CopyGLBuffer(*state.GetHandle(), T());

  viskores::Int64 retSize = static_cast<viskores::Int64>(returnedValues.size());

  //since BufferState allows for re-use of a GL buffer that is slightly
  //larger than the current array size, we should only check that the
  //buffer is not smaller than the array.
  //This GL buffer size is done to improve performance when transferring
  //arrays to GL whose size changes on a per frame basis
  VISKORES_TEST_ASSERT(retSize >= handle.GetNumberOfValues(),
                       "OpenGL buffer not large enough size");

  //validate that retsize matches the bufferstate capacity which returns
  //the amount of total GL buffer space, not the size we are using
  const viskores::Int64 capacity = (state.GetCapacity() / static_cast<viskores::Int64>(sizeof(T)));
  VISKORES_TEST_ASSERT(retSize == capacity, "OpenGL buffer size doesn't match BufferState");

  //validate that the capacity and the SMPTransferResource have the same size
  viskores::interop::internal::SMPTransferResource* resource =
    dynamic_cast<viskores::interop::internal::SMPTransferResource*>(state.GetResource());

  VISKORES_TEST_ASSERT(resource->Size == capacity,
                       "buffer state internal resource doesn't match BufferState capacity");

  auto portal = handle.ReadPortal();
  auto iter = returnedValues.cbegin();
  for (viskores::Id i = 0; i != handle.GetNumberOfValues(); ++i, ++iter)
  {
    VISKORES_TEST_ASSERT(portal.Get(i) == *iter, "incorrect value returned from OpenGL buffer");
  }
}

void test_ArrayHandleCartesianProduct()
{
  viskores::cont::ArrayHandle<viskores::Float32> x = makeArray(10, viskores::Float32());
  viskores::cont::ArrayHandle<viskores::Float32> y = makeArray(10, viskores::Float32());
  viskores::cont::ArrayHandle<viskores::Float32> z = makeArray(10, viskores::Float32());

  auto cartesian = viskores::cont::make_ArrayHandleCartesianProduct(x, y, z);

  viskores::interop::BufferState state;
  viskores::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);
  viskores::interop::TransferToOpenGL(cartesian, state); //make sure we can do multiple trasfers
  validate(cartesian, state);

  //resize up
  x = makeArray(100, viskores::Float32());
  y = makeArray(100, viskores::Float32());
  z = makeArray(100, viskores::Float32());
  cartesian = viskores::cont::make_ArrayHandleCartesianProduct(x, y, z);
  viskores::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);

  //resize down but instead capacity threshold
  x = makeArray(99, viskores::Float32());
  y = makeArray(99, viskores::Float32());
  z = makeArray(99, viskores::Float32());
  cartesian = viskores::cont::make_ArrayHandleCartesianProduct(x, y, z);
  viskores::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);

  //resize down
  x = makeArray(10, viskores::Float32());
  y = makeArray(10, viskores::Float32());
  z = makeArray(10, viskores::Float32());
  cartesian = viskores::cont::make_ArrayHandleCartesianProduct(x, y, z);
  viskores::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);
}

void test_ArrayHandleCast()
{
  viskores::cont::ArrayHandle<viskores::Vec3f_64> handle = makeArray(100000, viskores::Vec3f_64());
  auto castArray = viskores::cont::make_ArrayHandleCast(handle, viskores::Vec3f_32());

  viskores::interop::BufferState state;
  viskores::interop::TransferToOpenGL(castArray, state);
  validate(castArray, state);
  viskores::interop::TransferToOpenGL(castArray, state); //make sure we can do multiple trasfers
  validate(castArray, state);

  //resize down
  handle = makeArray(1000, viskores::Vec3f_64());
  castArray = viskores::cont::make_ArrayHandleCast(handle, viskores::Vec3f_32());
  viskores::interop::TransferToOpenGL(castArray, state);
  validate(castArray, state);
}

void test_ArrayHandleCounting()
{
  auto counting1 =
    viskores::cont::make_ArrayHandleCounting(viskores::Id(0), viskores::Id(1), viskores::Id(10000));
  auto counting2 =
    viskores::cont::make_ArrayHandleCounting(viskores::Id(0), viskores::Id(4), viskores::Id(10000));
  auto counting3 =
    viskores::cont::make_ArrayHandleCounting(viskores::Id(0), viskores::Id(0), viskores::Id(10000));

  //use the same state with different counting handles
  viskores::interop::BufferState state;
  viskores::interop::TransferToOpenGL(counting1, state);
  validate(counting1, state);
  viskores::interop::TransferToOpenGL(counting2, state);
  validate(counting2, state);
  viskores::interop::TransferToOpenGL(counting3, state);
  validate(counting3, state);
}

void test_ArrayHandleConcatenate()
{
  viskores::cont::ArrayHandle<viskores::Float32> a = makeArray(5000, viskores::Float32());
  viskores::cont::ArrayHandle<viskores::Float32> b = makeArray(25000, viskores::Float32());

  auto concatenate = viskores::cont::make_ArrayHandleConcatenate(a, b);

  viskores::interop::BufferState state;
  viskores::interop::TransferToOpenGL(concatenate, state);
  validate(concatenate, state);
  viskores::interop::TransferToOpenGL(concatenate, state); //make sure we can do multiple trasfers
  validate(concatenate, state);

  //resize down
  b = makeArray(1000, viskores::Float32());
  concatenate = viskores::cont::make_ArrayHandleConcatenate(a, b);
  viskores::interop::TransferToOpenGL(concatenate, state);
  validate(concatenate, state);
}

void test_ArrayHandleCompositeVector()
{
  viskores::cont::ArrayHandle<viskores::Float32> x = makeArray(10000, viskores::Float32());
  viskores::cont::ArrayHandle<viskores::Float32> y = makeArray(10000, viskores::Float32());
  viskores::cont::ArrayHandle<viskores::Float32> z = makeArray(10000, viskores::Float32());

  auto composite = viskores::cont::make_ArrayHandleCompositeVector(x, 0, y, 0, z, 0);

  viskores::interop::BufferState state;
  viskores::interop::TransferToOpenGL(composite, state);
  validate(composite, state);
}
}

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with viskores::interop::TransferToOpenGL
///
struct TestingTransferFancyHandles
{
public:
  /// Run a suite of tests to check to see if a viskores::interop::TransferToOpenGL
  /// properly supports all the fancy array handles that viskores supports. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  struct TestAll
  {
    void operator()() const
    {
      std::cout << "Doing FancyArrayHandle TransferToOpenGL Tests" << std::endl;

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCartesianProduct" << std::endl;
      test_ArrayHandleCartesianProduct();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCast" << std::endl;
      test_ArrayHandleCast();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCounting" << std::endl;
      test_ArrayHandleCounting();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConcatenate" << std::endl;
      test_ArrayHandleConcatenate();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConcatenate" << std::endl;
      test_ArrayHandleCompositeVector();
    }
  };

  static int Run(int argc, char* argv[])
  {
    return viskores::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace viskores::cont::testing

#endif //viskores_cont_testing_TestingFancyArrayHandles_h
