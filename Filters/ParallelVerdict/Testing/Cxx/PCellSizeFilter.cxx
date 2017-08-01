#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPCellSizeFilter.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

int PCellSizeFilter( int argc, char* argv[] )
{
  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize(&argc, &argv);
  contr->SetGlobalController(contr);
  contr->CreateOutputWindow();

  vtkNew<vtkUnstructuredGridReader> reader;
  vtkNew<vtkPCellSizeFilter> filter;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/uGridEx.vtk");

  reader->SetFileName( fname );
  delete [] fname;
  filter->SetInputConnection( reader->GetOutputPort() );
  filter->ComputeSumOn();
  filter->Update();

  vtkDoubleArray* sizes = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("size"));

  if (!sizes)
  {
    vtkGenericWarningMacro("Cannot find expected array output ('size') from vtkPCellSizeFilter");
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
  double correctSumValues[4] = {
    correctValues[11], correctValues[10]+correctValues[9],
    correctValues[8]+correctValues[7]+correctValues[6]+correctValues[5]+correctValues[4],
    correctValues[3]+correctValues[2]+correctValues[1]+correctValues[0] };
  // each process is reading in so we multiply the serial values by the number of processes for the sum
  int numProcs = contr->GetNumberOfProcesses();
  for (int i=0;i<4;i++)
  {
    correctSumValues[i] *= numProcs;
  }

  sizes = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("size"));
  for (vtkIdType i=0;i<sizes->GetNumberOfTuples();i++)
  {
    if (fabs(sizes->GetValue(i)-correctSumValues[i]) > .0001)
    {
      vtkGenericWarningMacro("Wrong size sum for dimension " << i);
      return EXIT_FAILURE;
    }
  }

  // only compute for the highest dimension cells (e.g. 3D cells)
  filter->ComputeSumOff();
  filter->ComputeHighestDimensionOn();
  filter->Update();
  sizes = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray("size"));

  for (int i=4;i<12;i++)
  {
    correctValues[i] = 0.;
  }
  for (vtkIdType i=0;i<sizes->GetNumberOfTuples();i++)
  {
    if (fabs(sizes->GetValue(i)-correctValues[i]) > .0001)
    {
      if (i<4)
      {
      vtkGenericWarningMacro("Wrong size for volumetric cell " << i);
      }
      else
      {
        vtkGenericWarningMacro("Should be skipping size computation for non-3D cell "
                               << i << " but did not");
      }
      return EXIT_FAILURE;
    }
  }
  if (vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetFieldData()->GetArray("size"))
  {
    vtkGenericWarningMacro("Should not be computing sum of sizes but it is being done");
    return EXIT_FAILURE;
  }

  const char name[] = "mysize";
  filter->SetArrayName(name);
  filter->ComputeHighestDimensionOff();
  filter->ComputePointOff();
  filter->ComputeLengthOff();
  filter->ComputeAreaOff();
  filter->ComputeVolumeOff();
  filter->Update();
  sizes = vtkDoubleArray::SafeDownCast(
    vtkUnstructuredGrid::SafeDownCast(filter->GetOutput())->GetCellData()->GetArray(name));

  for (vtkIdType i=0;i<sizes->GetNumberOfTuples();i++)
  {
    if (sizes->GetValue(i) )
    {
      vtkGenericWarningMacro("Should be skipping size computation for cell "<<i<<" but did not");
      return EXIT_FAILURE;
    }
  }

  contr->Finalize();
  contr->Delete();

  return EXIT_SUCCESS;
}
