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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CastAndCall.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

#include <viskores/TypeList.h>
#include <viskores/TypeTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

// Likely to contain both supported and unsupported types.
using TypesToTry = viskores::
  List<viskores::FloatDefault, viskores::UInt32, VISKORES_UNUSED_INT_TYPE, viskores::Int8>;

constexpr viskores::Id DIM_SIZE = 4;
constexpr viskores::Id ARRAY_SIZE = DIM_SIZE * DIM_SIZE * DIM_SIZE;

template <typename T>
viskores::cont::ArrayHandle<viskores::Vec<T, 3>> MakeCoordinates()
{
  viskores::cont::ArrayHandleUniformPointCoordinates coordArray{ viskores::Id(DIM_SIZE) };
  VISKORES_TEST_ASSERT(coordArray.GetNumberOfValues() == ARRAY_SIZE);
  viskores::cont::ArrayHandle<viskores::Vec<T, 3>> castArray;
  viskores::cont::ArrayCopy(coordArray, castArray);
  return castArray;
}

template <typename T>
viskores::cont::ArrayHandle<T> MakeField()
{
  viskores::cont::ArrayHandle<T> fieldArray;
  fieldArray.Allocate(ARRAY_SIZE);
  SetPortal(fieldArray.WritePortal());
  return fieldArray;
}

template <typename T>
viskores::cont::ArrayHandle<viskores::Vec<T, 3>> MakeVecField()
{
  return MakeField<viskores::Vec<T, 3>>();
}

template <typename FieldType>
viskores::cont::DataSet MakeDataSet()
{
  VISKORES_STATIC_ASSERT((std::is_same<typename viskores::TypeTraits<FieldType>::DimensionalityTag,
                                       viskores::TypeTraitsScalarTag>::value));

  viskores::cont::DataSet dataset;

  viskores::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(viskores::Id3(DIM_SIZE));
  dataset.SetCellSet(cellSet);

  dataset.AddCoordinateSystem(
    viskores::cont::CoordinateSystem("coords", MakeCoordinates<FieldType>()));
  dataset.AddPointField("scalars", MakeField<FieldType>());
  dataset.AddPointField("vectors", MakeVecField<FieldType>());

  VISKORES_TEST_ASSERT(dataset.GetNumberOfPoints() == ARRAY_SIZE);

  return dataset;
}

struct CheckCoords
{
  template <typename ArrayType>
  void operator()(const ArrayType& array) const
  {
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(array, MakeCoordinates<viskores::FloatDefault>()));
  }
};

template <typename T>
struct CheckField
{
  template <typename ArrayType>
  void operator()(const ArrayType& array) const
  {
    auto expectedArray = MakeField<T>();
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(array, expectedArray));
  }
};

struct TryType
{
  template <typename FieldType>
  void operator()(FieldType) const
  {
    using VecType = viskores::Vec<FieldType, 3>;

    std::cout << "Creating data." << std::endl;
    viskores::cont::DataSet data = MakeDataSet<FieldType>();

    std::cout << "Check original data." << std::endl;
    CheckCoords{}(
      data.GetCoordinateSystem().GetData().AsArrayHandle<viskores::cont::ArrayHandle<VecType>>());
    CheckField<FieldType>{}(data.GetPointField("scalars")
                              .GetData()
                              .AsArrayHandle<viskores::cont::ArrayHandle<FieldType>>());
    CheckField<VecType>{}(data.GetPointField("vectors")
                            .GetData()
                            .AsArrayHandle<viskores::cont::ArrayHandle<VecType>>());

    VISKORES_TEST_ASSERT((data.GetCoordinateSystem().IsSupportedType() ==
                          viskores::ListHas<VISKORES_DEFAULT_TYPE_LIST, VecType>::value));
    VISKORES_TEST_ASSERT((data.GetField("scalars").IsSupportedType() ==
                          viskores::ListHas<VISKORES_DEFAULT_TYPE_LIST, FieldType>::value));
    VISKORES_TEST_ASSERT((data.GetField("vectors").IsSupportedType() ==
                          viskores::ListHas<VISKORES_DEFAULT_TYPE_LIST, VecType>::value));

    std::cout << "Check as float default." << std::endl;
    CheckCoords{}(data.GetCoordinateSystem()
                    .GetDataAsDefaultFloat()
                    .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>());
    CheckField<FieldType>{}(
      data.GetPointField("scalars")
        .GetDataAsDefaultFloat()
        .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>());
    CheckField<VecType>{}(data.GetPointField("vectors")
                            .GetDataAsDefaultFloat()
                            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>());

    std::cout << "Check as expected type." << std::endl;
    viskores::cont::CastAndCall(data.GetCoordinateSystem().GetDataWithExpectedTypes(),
                                CheckCoords{});
    viskores::cont::CastAndCall(data.GetPointField("scalars").GetDataWithExpectedTypes(),
                                CheckField<FieldType>{});
    viskores::cont::CastAndCall(data.GetPointField("vectors").GetDataWithExpectedTypes(),
                                CheckField<VecType>{});

    std::cout << "Convert to expected and check." << std::endl;
    data.ConvertToExpected();
    viskores::cont::CastAndCall(data.GetCoordinateSystem(), CheckCoords{});
    viskores::cont::CastAndCall(data.GetPointField("scalars"), CheckField<FieldType>{});
    viskores::cont::CastAndCall(data.GetPointField("vectors"), CheckField<VecType>{});
  }
};

void Run()
{
  VISKORES_TEST_ASSERT(
    viskores::ListHas<VISKORES_DEFAULT_TYPE_LIST, viskores::FloatDefault>::value,
    "This test assumes that VISKORES_DEFAULT_TYPE_LIST has viskores::FloatDefault. "
    "If there is a reason for this condition, then a special condition needs "
    "to be added to skip this test.");
  VISKORES_TEST_ASSERT(viskores::ListHas<VISKORES_DEFAULT_TYPE_LIST, viskores::Vec3f>::value,
                       "This test assumes that VISKORES_DEFAULT_TYPE_LIST has viskores::Vec3f. "
                       "If there is a reason for this condition, then a special condition needs "
                       "to be added to skip this test.");

  viskores::testing::Testing::TryTypes(TryType{}, TypesToTry{});
}

} // anonymous namespace

int UnitTestDataSetConvertToExpected(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
