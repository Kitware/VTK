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

#include <viskores/cont/RuntimeDeviceInformation.h>

#include <viskores/cont/DeviceAdapterTag.h>

#include <viskores/cont/testing/Testing.h>

#include <cctype> //for tolower

namespace
{

template <typename Tag>
void TestName(const std::string& name, Tag tag, viskores::cont::DeviceAdapterId id)
{
  viskores::cont::RuntimeDeviceInformation info;

  VISKORES_TEST_ASSERT(id.GetName() == name, "Id::GetName() failed.");
  VISKORES_TEST_ASSERT(tag.GetName() == name, "Tag::GetName() failed.");
  VISKORES_TEST_ASSERT(viskores::cont::make_DeviceAdapterId(id.GetValue()) == id,
                       "make_DeviceAdapterId(int8) failed");

  VISKORES_TEST_ASSERT(info.GetName(id) == name, "RDeviceInfo::GetName(Id) failed.");
  VISKORES_TEST_ASSERT(info.GetName(tag) == name, "RDeviceInfo::GetName(Tag) failed.");
  VISKORES_TEST_ASSERT(info.GetId(name) == id, "RDeviceInfo::GetId(name) failed.");

  //check going from name to device id
  auto lowerCaseFunc = [](char c)
  { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };

  auto upperCaseFunc = [](char c)
  { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); };

  if (id.IsValueValid())
  { //only test make_DeviceAdapterId with valid device ids
    VISKORES_TEST_ASSERT(
      viskores::cont::make_DeviceAdapterId(name) == id, "make_DeviceAdapterId(", name, ") failed");

    std::string casedName = name;
    std::transform(casedName.begin(), casedName.end(), casedName.begin(), lowerCaseFunc);
    VISKORES_TEST_ASSERT(viskores::cont::make_DeviceAdapterId(casedName) == id,
                         "make_DeviceAdapterId(",
                         name,
                         ") failed");

    std::transform(casedName.begin(), casedName.end(), casedName.begin(), upperCaseFunc);
    VISKORES_TEST_ASSERT(viskores::cont::make_DeviceAdapterId(casedName) == id,
                         "make_DeviceAdapterId(",
                         name,
                         ") failed");
  }
}

void TestNames()
{
  viskores::cont::DeviceAdapterTagUndefined undefinedTag;
  viskores::cont::DeviceAdapterTagSerial serialTag;
  viskores::cont::DeviceAdapterTagTBB tbbTag;
  viskores::cont::DeviceAdapterTagOpenMP openmpTag;
  viskores::cont::DeviceAdapterTagCuda cudaTag;
  viskores::cont::DeviceAdapterTagKokkos kokkosTag;

  TestName("Undefined", undefinedTag, undefinedTag);
  TestName("Serial", serialTag, serialTag);
  TestName("TBB", tbbTag, tbbTag);
  TestName("OpenMP", openmpTag, openmpTag);
  TestName("Cuda", cudaTag, cudaTag);
  TestName("Kokkos", kokkosTag, kokkosTag);
}

} // end anon namespace

int UnitTestRuntimeDeviceNames(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestNames, argc, argv);
}
