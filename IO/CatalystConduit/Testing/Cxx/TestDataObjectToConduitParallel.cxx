#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataObjectToConduit.h"
#include "vtkDoubleArray.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <catalyst_conduit.hpp>
#include <cstdlib>

namespace
{
//----------------------------------------------------------------------------
void FillShapeMap(conduit_cpp::Node& shapeMap)
{
  shapeMap["hex"] = VTK_HEXAHEDRON;
  shapeMap["tet"] = VTK_TETRA;
  shapeMap["polygonal"] = VTK_POLYGON;
  shapeMap["quad"] = VTK_QUAD;
  shapeMap["tri"] = VTK_TRIANGLE;
  shapeMap["line"] = VTK_HEXAHEDRON;
  shapeMap["point"] = VTK_VERTEX;
  shapeMap["line"] = VTK_LINE;
  shapeMap["pyramid"] = VTK_PYRAMID;
  shapeMap["wedge"] = VTK_WEDGE;
}

//----------------------------------------------------------------------------
bool TestParallelUG()
{
  int rank = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();

  vtkNew<vtkUnstructuredGrid> ug;
  const std::array<std::vector<vtkIdType>, 2> connectivities{ std::vector<vtkIdType>{ 0, 1, 2 },
    { 1, 2, 3, 4 } };
  const std::array<std::array<double, 3>, 4> point_pos{ std::array<double, 3>{ { 0.0, 1.0, 2.0 } },
    { { 3.0, 4.0, 5.0 } }, { { 6.0, 7.0, 8.0 } }, { { 9.0, 10.0, 11.0 } } };
  std::vector<double> pdVals{ 0.5, 0.2, 1.4, 2.5 };
  std::vector<unsigned char> cdVals{ 3, 4 };

  if (rank == 0)
  {
    // Only add data to rank 0
    vtkNew<vtkPoints> points;

    for (size_t i = 0; i < point_pos.size(); i++)
    {
      points->InsertPoint(i, point_pos[i].data());
    }
    ug->SetPoints(points);

    ug->Allocate(2);

    ug->InsertNextCell(VTK_TRIANGLE, 3, connectivities.at(0).data());
    ug->InsertNextCell(VTK_QUAD, 4, connectivities.at(1).data());

    vtkNew<vtkDoubleArray> pd;
    pd->SetArray(pdVals.data(), 4, 1);
    pd->SetName("PointD");
    ug->GetPointData()->AddArray(pd);

    vtkNew<vtkUnsignedCharArray> cd;
    cd->SetArray(cdVals.data(), 2, 1);
    cd->SetName("CellD");
    ug->GetCellData()->AddArray(cd);
  }

  conduit_cpp::Node node;

  if (!vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(ug), node))
  {
    std::cerr << "FillConduitNode failed for TestMixedShapedUnstructuredGrid" << std::endl;
    return false;
  }

  std::vector<conduit_uint8> shapes = { VTK_TRIANGLE, VTK_QUAD };
  std::vector<conduit_int64> offsets = { 0, 3 };
  std::vector<conduit_int64> sizes = { 3, 4 };
  std::vector<conduit_int64> connectivity{ 0, 1, 2, 1, 2, 3, 4 };
  std::vector<float> pt_x = { 0.0, 3.0, 6.0, 9.0 };
  std::vector<float> pt_y = { 1.0, 4.0, 7.0, 10.0 };
  std::vector<float> pt_z = { 2.0, 5.0, 8.0, 11.0 };

  if (rank == 1)
  {
    shapes.clear();
    sizes.clear();
    offsets.clear();
    connectivity.clear();
    pdVals.clear();
    cdVals.clear();
    pt_x.clear();
    pt_y.clear();
    pt_z.clear();
  }

  conduit_cpp::Node expectedNode;
  auto coordsNode = expectedNode["coordsets/coords"];
  coordsNode["type"] = "explicit";
  coordsNode["values/x"] = pt_x;
  coordsNode["values/y"] = pt_y;
  coordsNode["values/z"] = pt_z;

  auto topologiesNode = expectedNode["topologies/mesh"];
  topologiesNode["type"] = "unstructured";
  topologiesNode["coordset"] = "coords";
  topologiesNode["elements/shape"] = "mixed";

  auto shape_map = topologiesNode["elements/shape_map"];
  ::FillShapeMap(shape_map);

  auto fieldsNode = expectedNode["fields"];
  fieldsNode["PointD/association"] = "vertex";
  fieldsNode["PointD/topology"] = "mesh";
  fieldsNode["PointD/volume_dependent"] = "false";
  fieldsNode["PointD/values"] = pdVals;

  fieldsNode["CellD/association"] = "element";
  fieldsNode["CellD/topology"] = "mesh";
  fieldsNode["CellD/volume_dependent"] = "false";
  fieldsNode["CellD/values"] = cdVals;

  topologiesNode["elements/shapes"] = shapes;

  if (ug->GetCells()->IsStorage64Bit())
  {
    topologiesNode["elements/shapes"] = shapes;
    topologiesNode["elements/sizes"] = sizes;
    topologiesNode["elements/offsets"] = offsets;
    topologiesNode["elements/connectivity"] = connectivity;
  }
  else
  {
    topologiesNode["elements/shapes"] = std::vector<conduit_int32>(shapes.begin(), shapes.end());
    topologiesNode["elements/offsets"] = std::vector<conduit_int32>(offsets.begin(), offsets.end());
    topologiesNode["elements/sizes"] = std::vector<conduit_int32>(sizes.begin(), sizes.end());
    topologiesNode["elements/connectivity"] =
      std::vector<conduit_int32>{ connectivity.begin(), connectivity.end() };
  }

  conduit_cpp::Node diff_info;
  bool areNodesDifferent = node.diff(expectedNode, diff_info, 1e-6);
  if (areNodesDifferent)
  {
    diff_info.print();
  }

  return !areNodesDifferent;
}
}

int TestDataObjectToConduitParallel(int argc, char** argv)
{
  /**
  When using MPI, make sure that nodes on all ranks have the same structure.
  This is critical for some applications such as AdiosCatalyst that assume that the node structure
  is the exact same between all ranks.
  */

  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  bool res = ::TestParallelUG();

  controller->Finalize();

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
