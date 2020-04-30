#include "vtkCellData.h"
#include "vtkCellSizeFilter.h"
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

int CellSizeFilter2(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{

  const int NumberOf1DCellTypes = 5;
  const int OneDCellTypes[NumberOf1DCellTypes] = { VTK_LINE, VTK_QUADRATIC_EDGE, VTK_CUBIC_LINE,
    VTK_LAGRANGE_CURVE, VTK_BEZIER_CURVE };
  const int NumberOf2DCellTypes = 8;
  const int TwoDCellTypes[NumberOf2DCellTypes] = { VTK_TRIANGLE, VTK_QUAD, VTK_QUADRATIC_TRIANGLE,
    VTK_QUADRATIC_QUAD, VTK_LAGRANGE_TRIANGLE, VTK_LAGRANGE_QUADRILATERAL, VTK_BEZIER_TRIANGLE,
    VTK_BEZIER_QUADRILATERAL };
  const int NumberOf3DCellTypes = 16;
  const int ThreeDCellTypes[NumberOf3DCellTypes] = { VTK_TETRA, VTK_HEXAHEDRON, VTK_WEDGE,
    VTK_PYRAMID, VTK_PENTAGONAL_PRISM, VTK_HEXAGONAL_PRISM, VTK_QUADRATIC_TETRA,
    VTK_QUADRATIC_HEXAHEDRON, VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_PYRAMID, VTK_LAGRANGE_TETRAHEDRON,
    VTK_LAGRANGE_HEXAHEDRON, VTK_LAGRANGE_WEDGE, VTK_BEZIER_TETRAHEDRON, VTK_BEZIER_HEXAHEDRON,
    VTK_BEZIER_WEDGE };

  for (int i = 0; i < NumberOf1DCellTypes; i++)
  {

    vtkNew<vtkCellTypeSource> cellTypeSource;
    cellTypeSource->SetBlocksDimensions(1, 1, 1);
    cellTypeSource->SetCellOrder(2);
    cellTypeSource->SetCellType(OneDCellTypes[i]);
    vtkNew<vtkCellSizeFilter> filter;
    filter->SetInputConnection(cellTypeSource->GetOutputPort());
    filter->ComputeSumOn();
    filter->Update();

    vtkDoubleArray* length = vtkDoubleArray::SafeDownCast(
      vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Length"));

    if (fabs(length->GetValue(0) - 1.0) > .0001)
    {
      vtkGenericWarningMacro("Wrong length dimension for the cell source type "
        << OneDCellTypes[i] << " supposed to be 1.0 whereas it is " << length->GetValue(0));
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < NumberOf2DCellTypes; i++)
  {

    vtkNew<vtkCellTypeSource> cellTypeSource;
    cellTypeSource->SetBlocksDimensions(1, 1, 1);
    cellTypeSource->SetCellOrder(2);
    cellTypeSource->SetCellType(TwoDCellTypes[i]);
    vtkNew<vtkCellSizeFilter> filter;
    filter->SetInputConnection(cellTypeSource->GetOutputPort());
    filter->ComputeSumOn();
    filter->Update();

    vtkDoubleArray* area = vtkDoubleArray::SafeDownCast(
      vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Area"));

    if (fabs(area->GetValue(0) - 1.0) > .0001)
    {
      vtkGenericWarningMacro("Wrong area dimension for the cell source type "
        << TwoDCellTypes[i] << " supposed to be 1.0 whereas it is " << area->GetValue(0));
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < NumberOf3DCellTypes; i++)
  {

    vtkNew<vtkCellTypeSource> cellTypeSource;
    cellTypeSource->SetBlocksDimensions(1, 1, 1);
    cellTypeSource->SetCellOrder(3);
    cellTypeSource->SetCellType(ThreeDCellTypes[i]);
    vtkNew<vtkCellSizeFilter> filter;
    filter->SetInputConnection(cellTypeSource->GetOutputPort());
    filter->ComputeSumOn();
    filter->Update();

    vtkDoubleArray* volume = vtkDoubleArray::SafeDownCast(
      vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("Volume"));

    if (fabs(volume->GetValue(0) - 1.0) > .0001)
    {
      vtkGenericWarningMacro("Wrong volume dimension for the cell source type "
        << ThreeDCellTypes[i] << " supposed to be 1.0 whereas it is " << volume->GetValue(0));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
