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

#include <viskores/cont/PointLocatorSparseGrid.h>

#include <viskores/cont/Invoker.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/worklet/WorkletMapField.h>

#include <random>

namespace
{

////brute force method /////
template <typename CoordiVecT, typename CoordiPortalT, typename CoordiT>
VISKORES_EXEC_CONT viskores::Id NNSVerify3D(CoordiVecT qc,
                                            CoordiPortalT coordiPortal,
                                            CoordiT& dis2)
{
  dis2 = std::numeric_limits<CoordiT>::max();
  viskores::Id nnpIdx = -1;

  for (viskores::Int32 i = 0; i < coordiPortal.GetNumberOfValues(); i++)
  {
    CoordiT splitX = coordiPortal.Get(i)[0];
    CoordiT splitY = coordiPortal.Get(i)[1];
    CoordiT splitZ = coordiPortal.Get(i)[2];
    CoordiT _dis2 = (splitX - qc[0]) * (splitX - qc[0]) + (splitY - qc[1]) * (splitY - qc[1]) +
      (splitZ - qc[2]) * (splitZ - qc[2]);
    if (_dis2 < dis2)
    {
      dis2 = _dis2;
      nnpIdx = i;
    }
  }
  return nnpIdx;
}

class NearestNeighborSearchBruteForce3DWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn qcIn,
                                WholeArrayIn treeCoordiIn,
                                FieldOut nnIdOut,
                                FieldOut nnDisOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  VISKORES_CONT
  NearestNeighborSearchBruteForce3DWorklet() {}

  template <typename CoordiVecType, typename CoordiPortalType, typename IdType, typename CoordiType>
  VISKORES_EXEC void operator()(const CoordiVecType& qc,
                                const CoordiPortalType& coordiPortal,
                                IdType& nnId,
                                CoordiType& nnDis) const
  {
    nnDis = std::numeric_limits<CoordiType>::max();

    nnId = NNSVerify3D(qc, coordiPortal, nnDis);
  }
};

class PointLocatorSparseGridWorklet : public viskores::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn qcIn,
                                ExecObject locator,
                                FieldOut nnIdOut,
                                FieldOut nnDistOut);

  typedef void ExecutionSignature(_1, _2, _3, _4);

  VISKORES_CONT
  PointLocatorSparseGridWorklet() {}

  template <typename CoordiVecType, typename Locator>
  VISKORES_EXEC void operator()(const CoordiVecType& qc,
                                const Locator& locator,
                                viskores::Id& nnIdOut,
                                viskores::FloatDefault& nnDis) const
  {
    locator.FindNearestNeighbor(qc, nnIdOut, nnDis);
  }
};

void TestTest()
{
  viskores::cont::Invoker invoke;

  viskores::Int32 nTrainingPoints = 5;
  viskores::Int32 nTestingPoint = 1;

  std::vector<viskores::Vec3f_32> coordi;

  ///// randomly generate training points/////
  std::default_random_engine dre;
  std::uniform_real_distribution<viskores::Float32> dr(0.0f, 10.0f);

  for (viskores::Int32 i = 0; i < nTrainingPoints; i++)
  {
    coordi.push_back(viskores::make_Vec(dr(dre), dr(dre), dr(dre)));
  }
  // Add a point to each corner to test the case where points might slip out
  // of the range by epsilon
  coordi.push_back(viskores::make_Vec(00.0f, 00.0f, 00.0f));
  coordi.push_back(viskores::make_Vec(00.0f, 10.0f, 00.0f));
  coordi.push_back(viskores::make_Vec(10.0f, 00.0f, 00.0f));
  coordi.push_back(viskores::make_Vec(10.0f, 10.0f, 00.0f));
  coordi.push_back(viskores::make_Vec(00.0f, 00.0f, 10.0f));
  coordi.push_back(viskores::make_Vec(00.0f, 10.0f, 10.0f));
  coordi.push_back(viskores::make_Vec(10.0f, 00.0f, 10.0f));
  coordi.push_back(viskores::make_Vec(10.0f, 10.0f, 10.0f));
  auto coordi_Handle = viskores::cont::make_ArrayHandle(coordi, viskores::CopyFlag::Off);

  viskores::cont::CoordinateSystem coord("points", coordi_Handle);

  viskores::cont::PointLocatorSparseGrid locator;
  locator.SetCoordinates(coord);
  locator.SetRange({ { 0.0, 10.0 } });
  locator.SetNumberOfBins({ 5, 5, 5 });

  locator.Update();

  ///// randomly generate testing points/////
  std::vector<viskores::Vec3f_32> qcVec;
  for (viskores::Int32 i = 0; i < nTestingPoint; i++)
  {
    qcVec.push_back(viskores::make_Vec(dr(dre), dr(dre), dr(dre)));
  }
  // Test near each corner to make sure that corner gets included
  qcVec.push_back(viskores::make_Vec(0.01f, 0.01f, 0.01f));
  qcVec.push_back(viskores::make_Vec(0.01f, 9.99f, 0.01f));
  qcVec.push_back(viskores::make_Vec(9.99f, 0.01f, 0.01f));
  qcVec.push_back(viskores::make_Vec(9.99f, 9.99f, 0.01f));
  qcVec.push_back(viskores::make_Vec(0.01f, 0.01f, 9.991f));
  qcVec.push_back(viskores::make_Vec(0.01f, 9.99f, 9.99f));
  qcVec.push_back(viskores::make_Vec(9.99f, 0.01f, 9.99f));
  qcVec.push_back(viskores::make_Vec(9.99f, 9.99f, 9.99f));
  auto qc_Handle = viskores::cont::make_ArrayHandle(qcVec, viskores::CopyFlag::Off);

  viskores::cont::ArrayHandle<viskores::Id> nnId_Handle;
  viskores::cont::ArrayHandle<viskores::FloatDefault> nnDis_Handle;

  invoke(PointLocatorSparseGridWorklet{}, qc_Handle, locator, nnId_Handle, nnDis_Handle);

  // brute force
  viskores::cont::ArrayHandle<viskores::Id> bfnnId_Handle;
  viskores::cont::ArrayHandle<viskores::Float32> bfnnDis_Handle;
  invoke(NearestNeighborSearchBruteForce3DWorklet{},
         qc_Handle,
         coordi_Handle,
         bfnnId_Handle,
         bfnnDis_Handle);

  ///// verify search result /////
  bool passTest = true;
  auto nnPortal = nnDis_Handle.ReadPortal();
  auto bfPortal = bfnnDis_Handle.ReadPortal();
  for (viskores::Int32 i = 0; i < nTestingPoint; i++)
  {
    viskores::Id workletIdx = nnId_Handle.WritePortal().Get(i);
    viskores::FloatDefault workletDis = nnPortal.Get(i);
    viskores::Id bfworkletIdx = bfnnId_Handle.WritePortal().Get(i);
    viskores::FloatDefault bfworkletDis = bfPortal.Get(i);

    if (workletIdx != bfworkletIdx)
    {
      std::cout << "bf index: " << bfworkletIdx << ", dis: " << bfworkletDis
                << ", grid: " << workletIdx << ", dis " << workletDis << std::endl;
      passTest = false;
    }
  }
  VISKORES_TEST_ASSERT(passTest, "Uniform Grid NN search result incorrect.");
}

} // anonymous namespace

int UnitTestPointLocatorSparseGrid(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestTest, argc, argv);
}
