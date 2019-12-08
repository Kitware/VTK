#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellTypeSource.h>
#include <vtkCellValidator.h>
#include <vtkDataArrayRange.h>
#include <vtkIdList.h>
#include <vtkShortArray.h>
#include <vtkUnstructuredGrid.h>

#include <array>

int TestPolyhedronConvexityMultipleCells(int, char*[])
{
  // create hexahedron cells
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_HEXAHEDRON);
  source->SetBlocksDimensions(2, 2, 2);
  source->Update();

  auto output = source->GetOutput();

  // create polyhedron cells
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(output->GetPoints());

  // explicit definition of the 6 hexahedron faces based on the local point ids
  // order within hexahedron cell arrays
  std::array<std::array<vtkIdType, 4>, 6> baseFaces = { { { { 0, 3, 2, 1 } }, { { 0, 4, 7, 3 } },
    { { 4, 5, 6, 7 } }, { { 5, 1, 2, 6 } }, { { 0, 1, 5, 4 } }, { { 2, 3, 7, 6 } } } };

  vtkIdType nCells = output->GetNumberOfCells();
  vtkNew<vtkIdTypeArray> cells;
  output->GetCells()->ExportLegacyFormat(cells);
  int z = 0;

  // this loop converts each hexahedron cell into an equivalent polyhedron cell
  // using the basefaces defined above.
  // polyhedron cells use a special cell array format to describe their cells:
  // (#faces, #face0_points, id0_0, ..., id0_N, ..., #faceN_points, idN_0, ..., idN_N)
  for (int i = 0; i < nCells; ++i)
  {
    vtkNew<vtkIdList> faces;
    auto size = cells->GetValue(z);
    auto cell = cells->GetPointer(z + 1);

    faces->InsertNextId(static_cast<vtkIdType>(baseFaces.size()));
    for (auto& baseFace : baseFaces)
    {
      faces->InsertNextId(static_cast<vtkIdType>(baseFace.size()));
      for (auto& f : baseFace)
      {
        faces->InsertNextId(cell[f]);
      }
    }

    z += size + 1;
    grid->InsertNextCell(VTK_POLYHEDRON, faces);
  }

  // validate cells
  vtkNew<vtkCellValidator> validator;
  validator->SetInputData(grid);
  validator->Update();

  auto states = validator->GetOutput()->GetCellData()->GetArray("ValidityState");
  for (auto state : vtk::DataArrayValueRange<1>(states))
  {
    if (state != vtkCellValidator::State::Valid)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
