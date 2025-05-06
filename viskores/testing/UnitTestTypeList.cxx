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

#include <viskores/TypeList.h>

#include <viskores/Types.h>

#include <viskores/testing/Testing.h>

#include <set>
#include <string>

namespace
{

class TypeSet
{
  using NameSetType = std::set<std::string>;
  NameSetType NameSet;

public:
  template <typename T>
  void AddExpected(T)
  {
    this->NameSet.insert(viskores::testing::TypeName<T>::Name());
  }

  template <typename T>
  void Found(T)
  {
    std::string name = viskores::testing::TypeName<T>::Name();
    //std::cout << "  found " << name << std::endl;
    NameSetType::iterator typeLocation = this->NameSet.find(name);
    if (typeLocation != this->NameSet.end())
    {
      // This type is expected. Remove it to mark it found.
      this->NameSet.erase(typeLocation);
    }
    else
    {
      std::cout << "**** Did not expect to get type " << name << std::endl;
      VISKORES_TEST_FAIL("Got unexpected type.");
    }
  }

  void CheckFound()
  {
    for (NameSetType::iterator typeP = this->NameSet.begin(); typeP != this->NameSet.end(); typeP++)
    {
      std::cout << "**** Failed to find " << *typeP << std::endl;
    }
    VISKORES_TEST_ASSERT(this->NameSet.empty(), "List did not call functor on all expected types.");
  }
};

struct TestFunctor
{
  TypeSet ExpectedTypes;

  TestFunctor(const TypeSet& expectedTypes)
    : ExpectedTypes(expectedTypes)
  {
  }

  template <typename T>
  VISKORES_CONT void operator()(T)
  {
    this->ExpectedTypes.Found(T());
  }
};

template <typename List>
void TryList(const TypeSet& expected, List)
{
  TestFunctor functor(expected);
  viskores::ListForEach(functor, List());
  functor.ExpectedTypes.CheckFound();
}

void TestLists()
{
  std::cout << "TypeListId" << std::endl;
  TypeSet id;
  id.AddExpected(viskores::Id());
  TryList(id, viskores::TypeListId());

  std::cout << "TypeListId2" << std::endl;
  TypeSet id2;
  id2.AddExpected(viskores::Id2());
  TryList(id2, viskores::TypeListId2());

  std::cout << "TypeListId3" << std::endl;
  TypeSet id3;
  id3.AddExpected(viskores::Id3());
  TryList(id3, viskores::TypeListId3());

  std::cout << "TypeListId4" << std::endl;
  TypeSet id4;
  id4.AddExpected(viskores::Id4());
  TryList(id4, viskores::TypeListId4());

  std::cout << "TypeListIndex" << std::endl;
  TypeSet index;
  index.AddExpected(viskores::Id());
  index.AddExpected(viskores::Id2());
  index.AddExpected(viskores::Id3());
  TryList(index, viskores::TypeListIndex());

  std::cout << "TypeListFieldScalar" << std::endl;
  TypeSet scalar;
  scalar.AddExpected(viskores::Float32());
  scalar.AddExpected(viskores::Float64());
  TryList(scalar, viskores::TypeListFieldScalar());

  std::cout << "TypeListFieldVec2" << std::endl;
  TypeSet vec2;
  vec2.AddExpected(viskores::Vec2f_32());
  vec2.AddExpected(viskores::Vec2f_64());
  TryList(vec2, viskores::TypeListFieldVec2());

  std::cout << "TypeListFieldVec3" << std::endl;
  TypeSet vec3;
  vec3.AddExpected(viskores::Vec3f_32());
  vec3.AddExpected(viskores::Vec3f_64());
  TryList(vec3, viskores::TypeListFieldVec3());

  std::cout << "TypeListFieldVec4" << std::endl;
  TypeSet vec4;
  vec4.AddExpected(viskores::Vec4f_32());
  vec4.AddExpected(viskores::Vec4f_64());
  TryList(vec4, viskores::TypeListFieldVec4());

  std::cout << "TypeListField" << std::endl;
  TypeSet field;
  field.AddExpected(viskores::Float32());
  field.AddExpected(viskores::Float64());
  field.AddExpected(viskores::Vec2f_32());
  field.AddExpected(viskores::Vec2f_64());
  field.AddExpected(viskores::Vec3f_32());
  field.AddExpected(viskores::Vec3f_64());
  field.AddExpected(viskores::Vec4f_32());
  field.AddExpected(viskores::Vec4f_64());
  TryList(field, viskores::TypeListField());

  std::cout << "TypeListCommon" << std::endl;
  TypeSet common;
  common.AddExpected(viskores::Float32());
  common.AddExpected(viskores::Float64());
  common.AddExpected(viskores::UInt8());
  common.AddExpected(viskores::Int32());
  common.AddExpected(viskores::Int64());
  common.AddExpected(viskores::Vec3f_32());
  common.AddExpected(viskores::Vec3f_64());
  TryList(common, viskores::TypeListCommon());

  std::cout << "TypeListScalarAll" << std::endl;
  TypeSet scalarsAll;
  scalarsAll.AddExpected(viskores::Float32());
  scalarsAll.AddExpected(viskores::Float64());
  scalarsAll.AddExpected(viskores::Int8());
  scalarsAll.AddExpected(viskores::UInt8());
  scalarsAll.AddExpected(viskores::Int16());
  scalarsAll.AddExpected(viskores::UInt16());
  scalarsAll.AddExpected(viskores::Int32());
  scalarsAll.AddExpected(viskores::UInt32());
  scalarsAll.AddExpected(viskores::Int64());
  scalarsAll.AddExpected(viskores::UInt64());
  TryList(scalarsAll, viskores::TypeListScalarAll());

  std::cout << "TypeListBaseC" << std::endl;
  TypeSet baseC;
  baseC.AddExpected(viskores::Float32());
  baseC.AddExpected(viskores::Float64());
  baseC.AddExpected(viskores::Int8());
  baseC.AddExpected(viskores::UInt8());
  baseC.AddExpected(viskores::Int16());
  baseC.AddExpected(viskores::UInt16());
  baseC.AddExpected(viskores::Int32());
  baseC.AddExpected(viskores::UInt32());
  baseC.AddExpected(viskores::Int64());
  baseC.AddExpected(viskores::UInt64());
  // Extra types with same layout as above but considered different by C
  baseC.AddExpected(bool());
  baseC.AddExpected(char());
  baseC.AddExpected((signed int)0);
  baseC.AddExpected((unsigned int)0);
  baseC.AddExpected((signed long)0);
  baseC.AddExpected((unsigned long)0);
  baseC.AddExpected((signed long long)0);
  baseC.AddExpected((unsigned long long)0);
  TryList(baseC, viskores::TypeListBaseC());

  std::cout << "TypeListVecCommon" << std::endl;
  TypeSet vecCommon;
  vecCommon.AddExpected(viskores::Vec2f_32());
  vecCommon.AddExpected(viskores::Vec2f_64());
  vecCommon.AddExpected(viskores::Vec2ui_8());
  vecCommon.AddExpected(viskores::Vec2i_32());
  vecCommon.AddExpected(viskores::Vec2i_64());
  vecCommon.AddExpected(viskores::Vec3f_32());
  vecCommon.AddExpected(viskores::Vec3f_64());
  vecCommon.AddExpected(viskores::Vec3ui_8());
  vecCommon.AddExpected(viskores::Vec3i_32());
  vecCommon.AddExpected(viskores::Vec3i_64());
  vecCommon.AddExpected(viskores::Vec4f_32());
  vecCommon.AddExpected(viskores::Vec4f_64());
  vecCommon.AddExpected(viskores::Vec4ui_8());
  vecCommon.AddExpected(viskores::Vec4i_32());
  vecCommon.AddExpected(viskores::Vec4i_64());
  TryList(vecCommon, viskores::TypeListVecCommon());

  std::cout << "TypeListVecAll" << std::endl;
  TypeSet vecAll;
  vecAll.AddExpected(viskores::Vec2f_32());
  vecAll.AddExpected(viskores::Vec2f_64());
  vecAll.AddExpected(viskores::Vec2i_8());
  vecAll.AddExpected(viskores::Vec2i_16());
  vecAll.AddExpected(viskores::Vec2i_32());
  vecAll.AddExpected(viskores::Vec2i_64());
  vecAll.AddExpected(viskores::Vec2ui_8());
  vecAll.AddExpected(viskores::Vec2ui_16());
  vecAll.AddExpected(viskores::Vec2ui_32());
  vecAll.AddExpected(viskores::Vec2ui_64());
  vecAll.AddExpected(viskores::Vec3f_32());
  vecAll.AddExpected(viskores::Vec3f_64());
  vecAll.AddExpected(viskores::Vec3i_8());
  vecAll.AddExpected(viskores::Vec3i_16());
  vecAll.AddExpected(viskores::Vec3i_32());
  vecAll.AddExpected(viskores::Vec3i_64());
  vecAll.AddExpected(viskores::Vec3ui_8());
  vecAll.AddExpected(viskores::Vec3ui_16());
  vecAll.AddExpected(viskores::Vec3ui_32());
  vecAll.AddExpected(viskores::Vec3ui_64());
  vecAll.AddExpected(viskores::Vec4f_32());
  vecAll.AddExpected(viskores::Vec4f_64());
  vecAll.AddExpected(viskores::Vec4i_8());
  vecAll.AddExpected(viskores::Vec4i_16());
  vecAll.AddExpected(viskores::Vec4i_32());
  vecAll.AddExpected(viskores::Vec4i_64());
  vecAll.AddExpected(viskores::Vec4ui_8());
  vecAll.AddExpected(viskores::Vec4ui_16());
  vecAll.AddExpected(viskores::Vec4ui_32());
  vecAll.AddExpected(viskores::Vec4ui_64());
  TryList(vecAll, viskores::TypeListVecAll());

  std::cout << "TypeListAll" << std::endl;
  TypeSet all;
  all.AddExpected(viskores::Float32());
  all.AddExpected(viskores::Float64());
  all.AddExpected(viskores::Int8());
  all.AddExpected(viskores::UInt8());
  all.AddExpected(viskores::Int16());
  all.AddExpected(viskores::UInt16());
  all.AddExpected(viskores::Int32());
  all.AddExpected(viskores::UInt32());
  all.AddExpected(viskores::Int64());
  all.AddExpected(viskores::UInt64());
  all.AddExpected(viskores::Vec2f_32());
  all.AddExpected(viskores::Vec2f_64());
  all.AddExpected(viskores::Vec2i_8());
  all.AddExpected(viskores::Vec2i_16());
  all.AddExpected(viskores::Vec2i_32());
  all.AddExpected(viskores::Vec2i_64());
  all.AddExpected(viskores::Vec2ui_8());
  all.AddExpected(viskores::Vec2ui_16());
  all.AddExpected(viskores::Vec2ui_32());
  all.AddExpected(viskores::Vec2ui_64());
  all.AddExpected(viskores::Vec3f_32());
  all.AddExpected(viskores::Vec3f_64());
  all.AddExpected(viskores::Vec3i_8());
  all.AddExpected(viskores::Vec3i_16());
  all.AddExpected(viskores::Vec3i_32());
  all.AddExpected(viskores::Vec3i_64());
  all.AddExpected(viskores::Vec3ui_8());
  all.AddExpected(viskores::Vec3ui_16());
  all.AddExpected(viskores::Vec3ui_32());
  all.AddExpected(viskores::Vec3ui_64());
  all.AddExpected(viskores::Vec4f_32());
  all.AddExpected(viskores::Vec4f_64());
  all.AddExpected(viskores::Vec4i_8());
  all.AddExpected(viskores::Vec4i_16());
  all.AddExpected(viskores::Vec4i_32());
  all.AddExpected(viskores::Vec4i_64());
  all.AddExpected(viskores::Vec4ui_8());
  all.AddExpected(viskores::Vec4ui_16());
  all.AddExpected(viskores::Vec4ui_32());
  all.AddExpected(viskores::Vec4ui_64());
  TryList(all, viskores::TypeListAll());
}

} // anonymous namespace

int UnitTestTypeList(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestLists, argc, argv);
}
