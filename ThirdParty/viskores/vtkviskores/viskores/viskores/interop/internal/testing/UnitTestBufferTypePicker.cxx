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
#include <viskores/cont/testing/Testing.h>

#include <GL/glew.h>
#include <viskores/interop/internal/BufferTypePicker.h>

namespace
{
void TestBufferTypePicker()
{
  //just verify that certain types match
  GLenum type;
  using viskoresUint = unsigned int;
  using T = viskores::FloatDefault;

  type = viskores::interop::internal::BufferTypePicker(viskores::Id());
  VISKORES_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = viskores::interop::internal::BufferTypePicker(int());
  VISKORES_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = viskores::interop::internal::BufferTypePicker(viskoresUint());
  VISKORES_TEST_ASSERT(type == GL_ELEMENT_ARRAY_BUFFER, "Bad OpenGL Buffer Type");

  type = viskores::interop::internal::BufferTypePicker(viskores::Vec<T, 4>());
  VISKORES_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = viskores::interop::internal::BufferTypePicker(viskores::Vec<T, 3>());
  VISKORES_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = viskores::interop::internal::BufferTypePicker(viskores::FloatDefault());
  VISKORES_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = viskores::interop::internal::BufferTypePicker(float());
  VISKORES_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
  type = viskores::interop::internal::BufferTypePicker(double());
  VISKORES_TEST_ASSERT(type == GL_ARRAY_BUFFER, "Bad OpenGL Buffer Type");
}
}

int UnitTestBufferTypePicker(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestBufferTypePicker, argc, argv);
}
