#include "vtkCellData.h"
#include "vtkCellSizeFilter.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

int CellSizeFilter( int argc, char* argv[] )
{
  vtkNew<vtkUnstructuredGridReader> reader;
  vtkNew<vtkCellSizeFilter> filter;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.vtk");

  reader->SetFileName( fname );
  delete [] fname;
  filter->SetInputConnection( reader->GetOutputPort() );
  filter->Update();

  vtkDoubleArray* sizes = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())
    ->GetCellData()->GetArray("size"));

  if (!sizes)
  {
    vtkGenericWarningMacro("Cannot find expected array output ('size') from vtkCellSizeFilter");
    return EXIT_FAILURE;
  }
  // types are hex, hex, tet, tet, polygon, triangle-strip, quad, triangle,
  // triangle, line, line, vertex
  double correctValues[12] = {1, 1, .16667, .16667, 2, 2, 1, .5, .5, 1, 1, 1};
  for (vtkIdType i=0;i<sizes->GetNumberOfTuples();i++)
  {
    if (fabs(sizes->GetValue(i)-correctValues[i]) > .0001)
    {
      vtkGenericWarningMacro("Wrong size for cell " << i);
      return EXIT_FAILURE;
    }
  }
  filter->ComputePointOff();
  filter->ComputeLengthOff();
  filter->ComputeAreaOff();
  filter->ComputeVolumeOff();
  filter->Update();
  sizes = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())
    ->GetCellData()->GetArray("size"));

  for (vtkIdType i=0;i<sizes->GetNumberOfTuples();i++)
  {
    if (sizes->GetValue(i) )
    {
      vtkGenericWarningMacro("Should be skipping size computation for cell "
                             << i << " but did not");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
