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

#include <viskores/cont/arg/TransportTagCellSetIn.h>

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename CellSetInType>
struct TestKernel : public viskores::exec::FunctorBase
{
  CellSetInType CellSet;

  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    if (this->CellSet.GetNumberOfElements() != 2)
    {
      this->RaiseError("Got bad number of shapes in exec cellset object.");
    }

    if (this->CellSet.GetIndices(0).GetNumberOfComponents() != 3 ||
        this->CellSet.GetIndices(1).GetNumberOfComponents() != 4)
    {
      this->RaiseError("Got bad number of Indices in exec cellset object.");
    }

    if (this->CellSet.GetCellShape(0).Id != 5 || this->CellSet.GetCellShape(1).Id != 9)
    {
      this->RaiseError("Got bad cell shape in exec cellset object.");
    }
  }
};

template <typename Device>
bool TransportWholeCellSetIn(Device device)
{
  std::cout << "Trying CellSetIn transport with " << device.GetName() << std::endl;

  //build a fake cell set
  const int nVerts = 5;
  viskores::cont::CellSetExplicit<> contObject;
  contObject.PrepareToAddCells(2, 7);
  contObject.AddCell(viskores::CELL_SHAPE_TRIANGLE, 3, viskores::make_Vec<viskores::Id>(0, 1, 2));
  contObject.AddCell(viskores::CELL_SHAPE_QUAD, 4, viskores::make_Vec<viskores::Id>(2, 1, 3, 4));
  contObject.CompleteAddingCells(nVerts);

  using IncidentTopology = viskores::TopologyElementTagPoint;
  using VisitTopology = viskores::TopologyElementTagCell;

  using ExecObjectType =
    typename viskores::cont::CellSetExplicit<>::template ExecConnectivityType<VisitTopology,
                                                                              IncidentTopology>;

  viskores::cont::arg::Transport<
    viskores::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>,
    viskores::cont::CellSetExplicit<>,
    Device>
    transport;

  viskores::cont::Token token;

  TestKernel<ExecObjectType> kernel;
  kernel.CellSet = transport(contObject, nullptr, 1, 1, token);

  viskores::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);

  return true;
}

void UnitTestCellSetIn()
{
  VISKORES_TEST_ASSERT(
    viskores::cont::TryExecute([](auto device) { return TransportWholeCellSetIn(device); }));
}

} // Anonymous namespace

int UnitTestTransportCellSetIn(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(UnitTestCellSetIn, argc, argv);
}
