#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPCellSizeFilter.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

int PCellSizeFilter(int argc, char* argv[])
{
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);
  contr->SetGlobalController(contr);
  contr->CreateOutputWindow();

  vtkNew<vtkUnstructuredGridReader> reader;
  vtkNew<vtkCellSizeFilter> filter;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.vtk");

  reader->SetFileName(fname);
  delete[] fname;
  filter->SetInputConnection(reader->GetOutputPort());
  filter->ComputeSumOn();
  filter->Update();

  vtkDoubleArray* vertexCount = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("VertexCount"));
  vtkDoubleArray* length = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Length"));
  vtkDoubleArray* area = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Area"));
  vtkDoubleArray* volume = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("Volume"));
  vtkDoubleArray* arrays[4] = { vertexCount, length, area, volume };

  // types are hex, hex, tet, tet, polygon, triangle-strip, quad, triangle,
  // triangle, line, line, vertex
  double correctValues[12] = { 1, 1, .16667, .16667, 2, 2, 1, .5, .5, 1, 1, 1 };
  int dimensions[12] = { 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 0 };
  for (int i = 0; i < 4; i++)
  {
    if (!arrays[i])
    {
      vtkGenericWarningMacro(
        "Cannot find expected array output for dimension " << i << " from vtkCellSizeFilter");
      return EXIT_FAILURE;
    }
    for (vtkIdType j = 0; j < arrays[i]->GetNumberOfTuples(); j++)
    {
      if (dimensions[j] == i && fabs(arrays[i]->GetValue(j) - correctValues[j]) > .0001)
      {
        vtkGenericWarningMacro("Wrong size for cell " << j);
        return EXIT_FAILURE;
      }
    }
  }
  double correctSumValues[4] = { correctValues[11], correctValues[10] + correctValues[9],
    correctValues[8] + correctValues[7] + correctValues[6] + correctValues[5] + correctValues[4],
    correctValues[3] + correctValues[2] + correctValues[1] + correctValues[0] };

  vertexCount = vtkDoubleArray::SafeDownCast(vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())
                                               ->GetFieldData()
                                               ->GetArray("VertexCount"));
  if (fabs(vertexCount->GetValue(0) - correctSumValues[0]) > .0001)
  {
    vtkGenericWarningMacro("Wrong size sum for dimension 0");
    return EXIT_FAILURE;
  }
  length = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Length"));
  if (fabs(length->GetValue(0) - correctSumValues[1]) > .0001)
  {
    vtkGenericWarningMacro("Wrong size sum for dimension 1");
    return EXIT_FAILURE;
  }
  area = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Area"));
  if (fabs(area->GetValue(0) - correctSumValues[2]) > .0001)
  {
    vtkGenericWarningMacro("Wrong size sum for dimension 2");
    return EXIT_FAILURE;
  }
  volume = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Volume"));
  if (fabs(volume->GetValue(0) - correctSumValues[3]) > .0001)
  {
    vtkGenericWarningMacro("Wrong size sum for dimension 3");
    return EXIT_FAILURE;
  }

  contr->Finalize();
  contr->Delete();

  return EXIT_SUCCESS;
}
