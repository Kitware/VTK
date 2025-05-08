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

#include <viskores/worklet/MaskIndices.h>
#include <viskores/worklet/ScatterUniform.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/worklet/WorkletPointNeighborhood.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/Math.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

using FieldType = viskores::Float32;
#define FieldNull viskores::Nan32()
constexpr viskores::IdComponent IdNull = -2;

struct FieldWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(WholeCellSetIn<>, // Placeholder for interface consistency
                                FieldIn inputField,
                                FieldInOut fieldCopy,
                                FieldInOut visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _2;

  using ScatterType = viskores::worklet::ScatterUniform<2>;
  using MaskType = viskores::worklet::MaskIndices;

  VISKORES_EXEC void operator()(FieldType inField,
                                viskores::IdComponent visitIndex,
                                FieldType& fieldCopy,
                                viskores::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

struct TopologyWorklet : viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint inputField,
                                FieldInOutPoint fieldCopy,
                                FieldInOutPoint visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterUniform<2>;
  using MaskType = viskores::worklet::MaskIndices;

  VISKORES_EXEC void operator()(FieldType inField,
                                viskores::IdComponent visitIndex,
                                FieldType& fieldCopy,
                                viskores::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

struct NeighborhoodWorklet : viskores::worklet::WorkletPointNeighborhood
{
  using ControlSignature = void(CellSetIn,
                                FieldIn inputField,
                                FieldInOut fieldCopy,
                                FieldInOut visitCopy);
  using ExecutionSignature = void(_2, VisitIndex, _3, _4);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterUniform<2>;
  using MaskType = viskores::worklet::MaskIndices;

  VISKORES_EXEC void operator()(FieldType inField,
                                viskores::IdComponent visitIndex,
                                FieldType& fieldCopy,
                                viskores::IdComponent& visitCopy) const
  {
    fieldCopy = inField;
    visitCopy = visitIndex;
  }
};

template <typename DispatcherType>
void TestMapWorklet()
{
  viskores::cont::testing::MakeTestDataSet builder;
  viskores::cont::DataSet data = builder.Make3DUniformDataSet1();

  viskores::cont::CellSetStructured<3> cellSet =
    data.GetCellSet().AsCellSet<viskores::cont::CellSetStructured<3>>();
  viskores::Id numPoints = cellSet.GetNumberOfPoints();

  viskores::cont::ArrayHandle<FieldType> inField;
  inField.Allocate(numPoints);
  SetPortal(inField.WritePortal());

  viskores::cont::ArrayHandle<FieldType> fieldCopy;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant(FieldNull, numPoints * 2),
                            fieldCopy);

  viskores::cont::ArrayHandle<viskores::IdComponent> visitCopy;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant(IdNull, numPoints * 2),
                            visitCopy);

  // The scatter is hardcoded to create 2 outputs for every input.
  // Set up the mask to select a range of values in the middle.
  viskores::Id maskStart = numPoints / 2;
  viskores::Id maskEnd = (numPoints * 2) / 3;
  viskores::worklet::MaskIndices mask(
    viskores::cont::make_ArrayHandleCounting(maskStart, viskores::Id(1), maskEnd - maskStart));

  DispatcherType dispatcher(mask);
  dispatcher.Invoke(cellSet, inField, fieldCopy, visitCopy);

  // Check outputs
  auto fieldCopyPortal = fieldCopy.ReadPortal();
  auto visitCopyPortal = visitCopy.ReadPortal();
  for (viskores::Id outputIndex = 0; outputIndex < numPoints * 2; ++outputIndex)
  {
    FieldType fieldValue = fieldCopyPortal.Get(outputIndex);
    viskores::IdComponent visitValue = visitCopyPortal.Get(outputIndex);
    if ((outputIndex >= maskStart) && (outputIndex < maskEnd))
    {
      viskores::Id inputIndex = outputIndex / 2;
      FieldType expectedField = TestValue(inputIndex, FieldType());
      VISKORES_TEST_ASSERT(fieldValue == expectedField,
                           outputIndex,
                           ": expected ",
                           expectedField,
                           ", got ",
                           fieldValue);

      viskores::IdComponent expectedVisit = static_cast<viskores::IdComponent>(outputIndex % 2);
      VISKORES_TEST_ASSERT(visitValue == expectedVisit,
                           outputIndex,
                           ": expected ",
                           expectedVisit,
                           ", got ",
                           visitValue);
    }
    else
    {
      VISKORES_TEST_ASSERT(
        viskores::IsNan(fieldValue), outputIndex, ": expected NaN, got ", fieldValue);
      VISKORES_TEST_ASSERT(
        visitValue == IdNull, outputIndex, ": expected ", IdNull, ", got ", visitValue);
    }
  }
}

void Test()
{
  std::cout << "Try on WorkletMapField" << std::endl;
  TestMapWorklet<viskores::worklet::DispatcherMapField<FieldWorklet>>();

  std::cout << "Try on WorkletMapCellToPoint" << std::endl;
  TestMapWorklet<viskores::worklet::DispatcherMapTopology<TopologyWorklet>>();

  std::cout << "Try on WorkletPointNeighborhood" << std::endl;
  TestMapWorklet<viskores::worklet::DispatcherPointNeighborhood<NeighborhoodWorklet>>();
}

} // anonymous namespace

int UnitTestScatterAndMask(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
