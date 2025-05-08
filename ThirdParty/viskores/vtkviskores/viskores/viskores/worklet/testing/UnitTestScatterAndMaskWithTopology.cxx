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

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/MaskSelect.h>
#include <viskores/worklet/ScatterUniform.h>

namespace
{

class TestWorkletMapTopo : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn topology, FieldInVisit pointCoords);
  using ExecutionSignature = void(_2, WorkIndex, InputIndex, OutputIndex, VisitIndex);
};

class TestWorkletMapTopoIdentity : public TestWorkletMapTopo
{
public:
  using ScatterType = viskores::worklet::ScatterIdentity;

  VISKORES_EXEC void operator()(const viskores::Vec<int, 3>& viskoresNotUsed(coords),
                                const viskores::Id& workIndex,
                                const viskores::Id& inputIndex,
                                const viskores::Id& outputIndex,
                                const viskores::Id& visitIndex) const
  {
    if (workIndex != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if (visitIndex != 0)
    {
      this->RaiseError("Got wrong visit value.");
    }
  }
};

class TestWorkletMapTopoUniform : public TestWorkletMapTopo
{
public:
  using ScatterType = viskores::worklet::ScatterUniform<2>;

  VISKORES_EXEC void operator()(const viskores::Vec<int, 3>& viskoresNotUsed(coords),
                                const viskores::Id& workIndex,
                                const viskores::Id& inputIndex,
                                const viskores::Id& outputIndex,
                                const viskores::Id& visitIndex) const
  {
    if ((workIndex / 2) != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if ((workIndex % 2) != visitIndex)
    {
      this->RaiseError("Got wrong visit value.");
    }
  }
};

class TestWorkletMapTopoNone : public TestWorkletMapTopo
{
public:
  using MaskType = viskores::worklet::MaskNone;

  VISKORES_EXEC void operator()(const viskores::Vec<int, 3>& viskoresNotUsed(coords),
                                const viskores::Id& workIndex,
                                const viskores::Id& inputIndex,
                                const viskores::Id& outputIndex,
                                const viskores::Id& visitIndex) const
  {
    if (workIndex != inputIndex)
    {
      this->RaiseError("Got wrong input value.");
    }
    if (outputIndex != workIndex)
    {
      this->RaiseError("Got work and output index don't match.");
    }
    if (visitIndex != 0)
    {
      this->RaiseError("Got wrong visit value.");
    }
  }
};

class TestWorkletMapTopoSelect : public TestWorkletMapTopo
{
public:
  using MaskType = viskores::worklet::MaskSelect;

  VISKORES_EXEC void operator()(const viskores::Vec<int, 3>& viskoresNotUsed(coords),
                                const viskores::Id& viskoresNotUsed(workIndex),
                                const viskores::Id& viskoresNotUsed(inputIndex),
                                const viskores::Id& viskoresNotUsed(outputIndex),
                                const viskores::Id& viskoresNotUsed(visitIndex)) const
  {
    // This method should never be called
    this->RaiseError("An element was selected, this test selects none.");
  }
};

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::testing::MakeTestDataSet testDataSet;
    viskores::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();

    viskores::cont::CellSetStructured<3> cellSet =
      dataSet3D.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>();

    viskores::cont::Invoker invoker;
    invoker(WorkletType{}, cellSet, dataSet3D.GetCoordinateSystem());
  }
};

template <>
struct DoTestWorklet<TestWorkletMapTopoSelect>
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::testing::MakeTestDataSet testDataSet;
    viskores::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();

    // Start select array with an array of zeros
    auto selectArrayHandle =
      viskores::cont::make_ArrayHandleMove(std::vector<viskores::IdComponent>(
        static_cast<std::size_t>(dataSet3D.GetNumberOfPoints()), 0));

    viskores::cont::CellSetStructured<3> cellSet =
      dataSet3D.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>();

    viskores::cont::Invoker invoker;
    invoker(TestWorkletMapTopoSelect{},
            viskores::worklet::MaskSelect(selectArrayHandle),
            cellSet,
            dataSet3D.GetCoordinateSystem());
  }
};

void TestWorkletMapField3d(viskores::cont::DeviceAdapterId id)
{
  using HandleTypesToTest3D =
    viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;

  using HandleTypesToTest1D = viskores::List<viskores::Int32,
                                             viskores::Int64,
                                             viskores::UInt32,
                                             viskores::UInt64,
                                             viskores::Int8,
                                             viskores::UInt8,
                                             char>;

  std::cout << "Testing WorkletMapTopology with ScatterIdentity on device adapter: " << id.GetName()
            << std::endl;

  viskores::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoIdentity>(),
                                       HandleTypesToTest3D());

  std::cout << "Testing WorkletMapTopology with ScatterUniform on device adapter: " << id.GetName()
            << std::endl;

  viskores::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoUniform>(),
                                       HandleTypesToTest3D());

  std::cout << "Testing WorkletMapTopology with MaskNone on device adapter: " << id.GetName()
            << std::endl;

  viskores::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoNone>(),
                                       HandleTypesToTest3D());

  std::cout << "Testing WorkletMapTopology with MaskSelect on device adapter: " << id.GetName()
            << std::endl;

  viskores::testing::Testing::TryTypes(DoTestWorklet<TestWorkletMapTopoSelect>(),
                                       HandleTypesToTest1D());
}

} //  namespace

int UnitTestScatterAndMaskWithTopology(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(TestWorkletMapField3d, argc, argv);
}
