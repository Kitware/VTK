#include <vtkContourFilter.h>
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkTetra.h>
#include <vtkUnstructuredGrid.h>

int TestContourConstantScalarUG(int, char*[])
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0, 1);

  vtkNew<vtkTetra> tetra;
  tetra->GetPointIds()->SetId(0, 0);
  tetra->GetPointIds()->SetId(1, 1);
  tetra->GetPointIds()->SetId(2, 2);
  tetra->GetPointIds()->SetId(3, 3);

  vtkNew<vtkUnstructuredGrid> ug;
  ug->SetPoints(points);
  ug->InsertNextCell(tetra->GetCellType(), tetra->GetPointIds());

  vtkIdType npts = ug->GetNumberOfPoints();

  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("Constant");
  scalars->SetNumberOfTuples(npts);
  for (vtkIdType i = 0; i < npts; ++i)
  {
    scalars->SetValue(i, 1000.0);
  }
  ug->GetPointData()->SetScalars(scalars);

  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(ug);
  contour->SetValue(0, 1000.0);
  contour->UseScalarTreeOn();

  contour->Update();

  vtkPolyData* output = contour->GetOutput();
  if (!output)
  {
    return EXIT_FAILURE;
  }

  if (output->GetNumberOfPoints() != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
