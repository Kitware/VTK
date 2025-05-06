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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/contour/MIRFilter.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/worklet/WorkletMapField.h>

#include <stdio.h>

void ConnectionHelperHex(std::vector<viskores::Id>& conn,
                         int x,
                         int y,
                         int z,
                         int mx,
                         int my,
                         int mz)
{
  (void)mz;
  conn.push_back(mx * (my * z + y) + x);
  conn.push_back(mx * (my * z + y) + x + 1);
  conn.push_back(mx * (my * z + y + 1) + x + 1);
  conn.push_back(mx * (my * z + y + 1) + x);
  conn.push_back(mx * (my * (z + 1) + y) + x);
  conn.push_back(mx * (my * (z + 1) + y) + x + 1);
  conn.push_back(mx * (my * (z + 1) + y + 1) + x + 1);
  conn.push_back(mx * (my * (z + 1) + y + 1) + x);
}

viskores::cont::DataSet GetTestDataSet()
{
  viskores::cont::DataSetBuilderExplicit dsb;

  int mx = 3, my = 3, mz = 3;


  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::Id> connections;
  std::vector<viskores::IdComponent> numberofInd;
  std::vector<viskores::Vec3f> points;

  for (int z = 0; z < mz - 1; z++)
  {
    for (int y = 0; y < my - 1; y++)
    {
      for (int x = 0; x < mx - 1; x++)
      {
        ConnectionHelperHex(connections, x, y, z, mx, my, mz);
      }
    }
  }

  std::vector<viskores::Id> idAR{ 1, 2, 2, 1, 2, 1, 1, 2 };
  std::vector<viskores::Id> lnAR{ 1, 1, 1, 1, 1, 1, 1, 1 };
  std::vector<viskores::Id> ofAR{ 0, 1, 2, 3, 4, 5, 6, 7 };
  viskores::cont::ArrayHandle<viskores::Id> offsets =
    viskores::cont::make_ArrayHandle(ofAR, viskores::CopyFlag::On);
  viskores::cont::ArrayHandle<viskores::Id> lengths =
    viskores::cont::make_ArrayHandle(lnAR, viskores::CopyFlag::On);
  viskores::cont::ArrayHandle<viskores::Id> ids =
    viskores::cont::make_ArrayHandle(idAR, viskores::CopyFlag::On);
  std::vector<viskores::FloatDefault> vfAR{ 1, 1, 1, 1, 1, 1, 1, 1 };
  viskores::cont::ArrayHandle<viskores::FloatDefault> vfs =
    viskores::cont::make_ArrayHandle(vfAR, viskores::CopyFlag::On);

  shapes.reserve((mx - 1) * (my - 1) * (mz - 1));
  numberofInd.reserve((mx - 1) * (my - 1) * (mz - 1));
  for (int i = 0; i < (mx - 1) * (my - 1) * (mz - 1); i++)
  {
    shapes.push_back(viskores::CELL_SHAPE_HEXAHEDRON);
    numberofInd.push_back(8);
  }

  points.reserve(mz * my * mx);
  for (int z = 0; z < mz; z++)
  {
    for (int y = 0; y < my; y++)
    {
      for (int x = 0; x < mx; x++)
      {
        viskores::Vec3f_32 point(static_cast<viskores::Float32>(x),
                                 static_cast<viskores::Float32>(y),
                                 static_cast<viskores::Float32>(z));
        points.push_back(point);
      }
    }
  }
  viskores::cont::DataSet ds = dsb.Create(points, shapes, numberofInd, connections);
  ds.AddField(
    viskores::cont::Field("scatter_pos", viskores::cont::Field::Association::Cells, offsets));
  ds.AddField(
    viskores::cont::Field("scatter_len", viskores::cont::Field::Association::Cells, lengths));
  ds.AddField(
    viskores::cont::Field("scatter_ids", viskores::cont::Field::Association::WholeDataSet, ids));
  ds.AddField(
    viskores::cont::Field("scatter_vfs", viskores::cont::Field::Association::WholeDataSet, vfs));

  return ds;
}

class MetaDataLength : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  VISKORES_EXEC
  void operator()(const viskores::FloatDefault& background,
                  const viskores::FloatDefault& circle_a,
                  const viskores::FloatDefault& circle_b,
                  const viskores::FloatDefault& circle_c,
                  viskores::Id& length) const
  {
    length = 0;
    if (background > viskores::FloatDefault(0.0))
      length++;
    if (circle_a > viskores::FloatDefault(0.0))
      length++;
    if (circle_b > viskores::FloatDefault(0.0))
      length++;
    if (circle_c > viskores::FloatDefault(0.0))
      length++;
  }
};

class MetaDataPopulate : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayOut, WholeArrayOut);

  template <typename IdArray, typename DataArray>
  VISKORES_EXEC void operator()(const viskores::Id& offset,
                                const viskores::FloatDefault& background,
                                const viskores::FloatDefault& circle_a,
                                const viskores::FloatDefault& circle_b,
                                const viskores::FloatDefault& circle_c,
                                IdArray& matIds,
                                DataArray& matVFs) const
  {
    viskores::Id index = offset;
    if (background > viskores::FloatDefault(0.0))
    {
      matIds.Set(index, 1);
      matVFs.Set(index, background);
      index++;
    }
    if (circle_a > viskores::FloatDefault(0.0))
    {
      matIds.Set(index, 2);
      matVFs.Set(index, circle_a);
      index++;
    }
    if (circle_b > viskores::FloatDefault(0.0))
    {
      matIds.Set(index, 3);
      matVFs.Set(index, circle_b);
      index++;
    }
    if (circle_c > viskores::FloatDefault(0.0))
    {
      matIds.Set(index, 4);
      matVFs.Set(index, circle_c);
      index++;
    }
  }
};

void TestMIRVenn250()
{
  using IdArray = viskores::cont::ArrayHandle<viskores::Id>;
  using DataArray = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  viskores::cont::Invoker invoker;

  std::string vennFile = viskores::cont::testing::Testing::DataPath("uniform/venn250.vtk");
  viskores::io::VTKDataSetReader reader(vennFile);
  viskores::cont::DataSet data = reader.ReadDataSet();

  DataArray backArr;
  data.GetField("mesh_topo/background").GetDataAsDefaultFloat().AsArrayHandle(backArr);
  DataArray cirAArr;
  data.GetField("mesh_topo/circle_a").GetDataAsDefaultFloat().AsArrayHandle(cirAArr);
  DataArray cirBArr;
  data.GetField("mesh_topo/circle_b").GetDataAsDefaultFloat().AsArrayHandle(cirBArr);
  DataArray cirCArr;
  data.GetField("mesh_topo/circle_c").GetDataAsDefaultFloat().AsArrayHandle(cirCArr);

  IdArray length;
  IdArray offset;
  IdArray matIds;
  DataArray matVFs;
  invoker(MetaDataLength{}, backArr, cirAArr, cirBArr, cirCArr, length);
  viskores::cont::Algorithm::ScanExclusive(length, offset);

  viskores::Id total = viskores::cont::Algorithm::Reduce(length, viskores::Id(0));
  matIds.Allocate(total);
  matVFs.Allocate(total);

  invoker(MetaDataPopulate{}, offset, backArr, cirAArr, cirBArr, cirCArr, matIds, matVFs);

  data.AddField(
    viskores::cont::Field("scatter_pos", viskores::cont::Field::Association::Cells, offset));
  data.AddField(
    viskores::cont::Field("scatter_len", viskores::cont::Field::Association::Cells, length));
  data.AddField(
    viskores::cont::Field("scatter_ids", viskores::cont::Field::Association::WholeDataSet, matIds));
  data.AddField(
    viskores::cont::Field("scatter_vfs", viskores::cont::Field::Association::WholeDataSet, matVFs));

  viskores::filter::contour::MIRFilter mir;
  mir.SetIDWholeSetName("scatter_ids");
  mir.SetPositionCellSetName("scatter_pos");
  mir.SetLengthCellSetName("scatter_len");
  mir.SetVFWholeSetName("scatter_vfs");
  mir.SetErrorScaling(viskores::Float64(0.2));
  mir.SetScalingDecay(viskores::Float64(1.0));
  mir.SetMaxIterations(viskores::IdComponent(0)); // =0 -> No iterations..
  // Only useful for iterations >= 1, will stop iterating if total % error for entire mesh is less than this value
  // Note it is mathematically impossible to obtain 0% error outside of VERY special cases (neglecting float error)
  mir.SetMaxPercentError(viskores::Float64(0.00001));

  VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "Before executing filter w/ Venn data");

  viskores::cont::DataSet fromMIR = mir.Execute(data);

  VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "After executing filter w/ Venn data");

  VISKORES_TEST_ASSERT(fromMIR.GetNumberOfCells() == 66086, "Wrong number of output cells");
}

void TestMIRSynthetic()
{
  viskores::cont::DataSet ds = GetTestDataSet();

  viskores::filter::contour::MIRFilter mir;
  mir.SetIDWholeSetName("scatter_ids");
  mir.SetPositionCellSetName("scatter_pos");
  mir.SetLengthCellSetName("scatter_len");
  mir.SetVFWholeSetName("scatter_vfs");

  mir.SetErrorScaling(viskores::Float64(0.2));
  mir.SetScalingDecay(viskores::Float64(1.0));
  mir.SetMaxIterations(viskores::IdComponent(0)); // =0 -> No iterations..
  mir.SetMaxPercentError(viskores::Float64(
    0.00001)); // Only useful for iterations >= 1, will stop iterating if total % error for entire mesh is less than this value
  // Note it is mathematically impossible to obtain 0% error outside of VERY special cases (neglecting float error)
  VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "Before executing filter");

  viskores::cont::DataSet ds_from_mir = mir.Execute(ds);

  VISKORES_LOG_S(viskores::cont::LogLevel::Warn, "After executing filter");

  // Test if ds_from_mir has 40 cells
  VISKORES_TEST_ASSERT(ds_from_mir.GetNumberOfCells() == 40, "Wrong number of output cells");
}

void TestMIR()
{
  TestMIRSynthetic();
  TestMIRVenn250();
}

int UnitTestMIRFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestMIR, argc, argv);
}
