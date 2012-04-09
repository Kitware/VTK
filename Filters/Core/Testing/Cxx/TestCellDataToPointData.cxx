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

#include <vtkCellDataToPointData.h>
#include <vtkDataArray.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkThreshold.h>
#include <vtkTestUtilities.h>

#define vsp(type, name) \
        vtkSmartPointer<vtk##type> name = vtkSmartPointer<vtk##type>::New()

int TestCellDataToPointData (int, char*[])
{
  char const name [] = "RTData";
  vsp(RTAnalyticSource, wavelet);
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

  vsp(PointDataToCellData, p2c);
    p2c->SetInputConnection(wavelet->GetOutputPort());
    p2c->PassPointDataOff();

  vsp(CellDataToPointData, sc2p);
    sc2p->SetInputConnection(p2c->GetOutputPort());
    sc2p->PassCellDataOff();
    sc2p->Update();

  vsp(DataSetTriangleFilter, c2g);
    c2g->SetInputConnection(p2c->GetOutputPort());

  vsp(CellDataToPointData, uc2p);
    uc2p->SetInputConnection(c2g->GetOutputPort());
    uc2p->Update();

  vtkDataArray* const x = sc2p->GetOutput()->GetPointData()->GetArray(name);
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

  bool const ok = fabs(mean) < 1e-4 && fabs(variance) < 1e-4;
  return !ok; // zero indicates test succeed
}

