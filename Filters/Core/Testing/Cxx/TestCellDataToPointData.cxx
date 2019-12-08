/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkThreshold.h>
#include <vtkUnstructuredGrid.h>

int TestCellDataToPointData(int, char*[])
{
  char const name[] = "RTData";
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  wavelet->SetCenter(0, 0, 0);
  wavelet->SetMaximum(255);
  wavelet->SetStandardDeviation(.5);
  wavelet->SetXFreq(60);
  wavelet->SetYFreq(30);
  wavelet->SetZFreq(40);
  wavelet->SetXMag(10);
  wavelet->SetYMag(18);
  wavelet->SetZMag(5);
  wavelet->SetSubsampleRate(1);
  wavelet->Update();

  vtkNew<vtkDoubleArray> dist;
  dist->SetNumberOfComponents(1);
  dist->SetName("Dist");

  vtkImageData* original = wavelet->GetOutput();
  for (vtkIdType i = 0; i < original->GetNumberOfPoints(); ++i)
  {
    double p[3];
    original->GetPoint(i, p);
    dist->InsertNextValue(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
  }
  original->GetPointData()->AddArray(dist);

  vtkNew<vtkPointDataToCellData> p2c;
  p2c->SetInputData(original);
  p2c->PassPointDataOff();

  vtkNew<vtkCellDataToPointData> selectiveC2P;
  selectiveC2P->SetInputConnection(p2c->GetOutputPort());
  selectiveC2P->SetProcessAllArrays(false);
  selectiveC2P->AddCellDataArray(name);
  selectiveC2P->Update();

  vtkNew<vtkCellDataToPointData> sc2p;
  sc2p->SetInputConnection(p2c->GetOutputPort());
  sc2p->PassCellDataOff();
  sc2p->Update();

  vtkNew<vtkDataSetTriangleFilter> c2g;
  c2g->SetInputConnection(p2c->GetOutputPort());

  vtkNew<vtkCellDataToPointData> uc2p;
  uc2p->SetInputConnection(c2g->GetOutputPort());

  vtkDataArray* const x = sc2p->GetOutput()->GetPointData()->GetArray(name);

  // test if selective CellDataToPointData operates on the correct
  int outNumPArrays = selectiveC2P->GetOutput()->GetPointData()->GetNumberOfArrays(); // should be 1
  int outNumCArrays = selectiveC2P->GetOutput()->GetCellData()->GetNumberOfArrays();  // should be 0
  std::string pArrayName =
    selectiveC2P->GetOutput()->GetPointData()->GetArrayName(0); // should be RTData

  if (outNumPArrays != 1)
  {
    std::cerr << "Wrong number of PointData arrays." << std::endl;
    return EXIT_FAILURE;
  }

  if (outNumCArrays != 0)
  {
    std::cerr << "Wrong number of CellData arrays." << std::endl;
    return EXIT_FAILURE;
  }

  if (pArrayName != name)
  {
    std::cerr << "Array name not matching original name." << std::endl;
    return EXIT_FAILURE;
  }

  // iterate through the options for which cells contribute to the result
  // for the cell data to point data filter. since all cells are 3D the
  // result should be the same.
  for (int opt = 0; opt < 3; opt++)
  {
    uc2p->SetContributingCellOption(opt);
    uc2p->Update();

    vtkDataArray* const y = uc2p->GetOutput()->GetPointData()->GetArray(name);

    vtkIdType const nvalues = x->GetNumberOfTuples() * x->GetNumberOfComponents();
    double mean = 0, variance = 0;

    // mean
    for (vtkIdType i = 0; i < nvalues; ++i)
    {
      mean += x->GetTuple1(i) - y->GetTuple1(i);
    }
    mean /= nvalues;

    // variance
    for (vtkIdType i = 0; i < nvalues; ++i)
    {
      double z = x->GetTuple1(i) - y->GetTuple1(i);
      variance += z * z;
    }
    variance /= nvalues;

    if (!(fabs(mean) < 1e-4 && fabs(variance) < 1e-4))
    {
      cerr << "Failure on option " << opt << endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
