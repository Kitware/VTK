// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkConduitToDataObject.h"
#include "vtkDataObjectToConduit.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>

namespace
{

//----------------------------------------------------------------------------
bool TestGhostCellsAndGhostPoints()
{
  conduit_cpp::Node node;

  vtkNew<vtkImageData> im;
  im->SetDimensions(3, 3, 2);
  im->SetSpacing(10, 20, 30);
  im->SetOrigin(-1, -2, -3);
  im->AllocateScalars(VTK_INT, 1);
  int* dims = im->GetDimensions();

  for (int z = 0; z < dims[2]; z++)
  {
    for (int y = 0; y < dims[1]; y++)
    {
      for (int x = 0; x < dims[0]; x++)
      {
        im->SetScalarComponentFromFloat(x, y, z, 0, 2);
      }
    }
  }

  vtkNew<vtkUnsignedCharArray> ghostCells;
  ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
  ghostCells->SetNumberOfValues(im->GetNumberOfCells());
  ghostCells->SetValue(2, vtkDataSetAttributes::DUPLICATECELL);
  ghostCells->SetValue(3, vtkDataSetAttributes::DUPLICATECELL);
  im->GetCellData()->AddArray(ghostCells);

  vtkNew<vtkUnsignedCharArray> ghostPoints;
  ghostPoints->SetName(vtkDataSetAttributes::GhostArrayName());
  ghostPoints->SetNumberOfValues(im->GetNumberOfPoints());
  for (int i : { 6, 7, 8, 15, 16, 17 })
  {
    ghostPoints->SetValue(i, vtkDataSetAttributes::DUPLICATEPOINT);
  }
  im->GetPointData()->AddArray(ghostPoints);

  if (!vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(im), node))
  {
    std::cerr << "vtkDataObjectToConduit::FillConduitNode failed for TestGhostCells" << std::endl;
    return false;
  }

  vtkNew<vtkPartitionedDataSet> dataset;
  if (!vtkConduitToDataObject::FillPartitionedDataSet(dataset, node))
  {
    std::cerr << "vtkConduitToDataObject::FillPartitionedDataSet failed for TestGhostCells"
              << std::endl;
    return false;
  }

  auto* ghostPointArray =
    dataset->GetPartition(0)->GetGhostArray(vtkDataObject::AttributeTypes::POINT);
  if (!ghostPointArray || ghostPointArray->GetSize() != ghostPoints->GetSize())
  {
    std::cerr << "No ghost point array found in:\n" << node.to_string() << std::endl;
    return false;
  }
  auto* ghostCellArray =
    dataset->GetPartition(0)->GetGhostArray(vtkDataObject::AttributeTypes::CELL);
  if (!ghostCellArray || ghostCellArray->GetSize() != ghostCells->GetSize())
  {
    std::cerr << "No ghost cell array found in:\n" << node.to_string() << std::endl;
    return false;
  }

  return true;
}

}

//----------------------------------------------------------------------------
int TestDataObjectToConduitToDataObject(int argc, char* argv[])
{
  bool ret = true;

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif

  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);

  ret &= ::TestGhostCellsAndGhostPoints();

  controller->Finalize();

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
