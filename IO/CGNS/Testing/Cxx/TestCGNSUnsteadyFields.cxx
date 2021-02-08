/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCGNSUnsteadyFields.cxx

  Copyright (c) Jakub Benda, CFD support, Czech Republic
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;               \
    return EXIT_FAILURE;                                                                           \
  }

int TestField(vtkMultiBlockDataSet* mb, double value)
{
  const double eps = 1e-5;
  unsigned int nBlocks = mb->GetNumberOfBlocks();
  vtk_assert(nBlocks > 0);
  for (unsigned int i = 0; i < nBlocks; ++i)
  {
    cout << "Block #" << i << endl;
    vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(i));
    vtk_assert(mb2 != nullptr);
    for (unsigned int j = 0; j < mb2->GetNumberOfBlocks(); ++j)
    {
      cout << " - Sub-block #" << j << endl;
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(j));
      vtk_assert(ug != nullptr);
      vtkCellData* cd = ug->GetCellData();
      vtk_assert(cd != nullptr);
      vtkIdType na = cd->GetNumberOfArrays();
      cout << "    - number of arrays: " << na << endl;
      vtk_assert(na == 1);
      for (vtkIdType k = 0; k < na; ++k)
      {
        vtkDataArray* ar = cd->GetArray(k);
        vtk_assert(ar != nullptr);
        vtkIdType nt = ar->GetNumberOfTuples();
        vtkIdType nc = ar->GetNumberOfComponents();
        vtk_assert(nt == 1);
        vtk_assert(nc == 1);
        double x = ar->GetComponent(0, 0);
        cout << "    - field value: " << x << endl;
        vtk_assert(std::fabs(x - value) < eps);
      }
    }
  }
  return 0;
}

int TestCGNSUnsteadyFields(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_fields.cgns");
  std::string fields = fname ? fname : "";
  delete[] fname;

  cout << "Opening " << fields.c_str() << endl;
  vtkNew<vtkCGNSReader> fieldsReader;
  fieldsReader->SetFileName(fields.c_str());
  fieldsReader->Update();
  fieldsReader->EnableAllCellArrays();

  for (int timestep = 0; timestep < 6; ++timestep)
  {
    fieldsReader->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timestep);
    fieldsReader->Update();

    vtkMultiBlockDataSet* mb = fieldsReader->GetOutput();

    if (0 != TestField(mb, timestep))
    {
      return EXIT_FAILURE;
    }
  }

  cout << __FILE__ << " tests passed." << endl;
  return EXIT_SUCCESS;
}
