#include "vtkExtractCells.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

namespace
{
vtkSmartPointer<vtkUnstructuredGrid> ReadData(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> r;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/voronoiMesh.vtu");
  r->SetFileName(fname);
  delete[] fname;

  r->Update();
  return r->GetOutput();
}
}

int TestExtractCells(int argc, char* argv[])
{
  auto ug = ReadData(argc, argv);

  vtkNew<vtkExtractCells> extractor;
  extractor->AddCellRange(0, 1); // it includes the end (don't ask!)
  extractor->SetInputDataObject(ug);
  extractor->Update();
  if (extractor->GetOutput()->GetNumberOfCells() != 2)
  {
    vtkLogF(ERROR, "ERROR: failed to extract polyhedral elements;");
    return EXIT_FAILURE;
  }

  vtkNew<vtkUnstructuredGrid> emptyUG;
  extractor->SetInputDataObject(emptyUG);
  extractor->Update();
  if (extractor->GetOutput()->GetNumberOfCells() != 0)
  {
    vtkLogF(ERROR, "ERROR: Unexpected output with empty input");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
