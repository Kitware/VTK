/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProbeFilter.h"
#include "vtkLineSource.h"
#include "vtkArrayCalculator.h"
#include "vtkNew.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"

// Gets the number of points the probe filter counted as valid.
// The parameter should be the output of the probe filter
int GetNumberOfValidPoints(vtkDataSet* pd)
{
  vtkDataArray* data = pd->GetPointData()->GetScalars("vtkValidPointMask");
  int numValid = 0;
  for (int i = 0; i < data->GetNumberOfTuples(); ++i)
  {
    if (data->GetVariantValue(i).ToDouble() == 1)
    {
      ++numValid;
    }
  }
  return numValid;
}

// Tests the CompteThreshold and Threshold parameters on the vtkProbeFilter
int TestProbeFilterThreshold()
{
  vtkNew< vtkLineSource > line1;
  line1->SetPoint1(-1,0,0);
  line1->SetPoint2(10,0,0);
  line1->SetResolution(11);

  vtkNew< vtkLineSource > line2;
  line2->SetPoint1(-0.499962,-0.00872654,0);
  line2->SetPoint2(10.4996,0.0872654,0);
  line2->SetResolution(11);

  vtkNew< vtkArrayCalculator > calc;
  calc->SetInputConnection(line1->GetOutputPort());
  calc->AddCoordinateScalarVariable("coordsX");
  calc->SetFunction("sin(coordsX)");

  vtkNew< vtkProbeFilter > probe;
  probe->SetInputConnection(calc->GetOutputPort());
  probe->SetSourceConnection(line2->GetOutputPort());
  probe->Update();

  int validDefault = GetNumberOfValidPoints(probe->GetOutput());
  if (validDefault != 2)
  {
    return 1;
  }
  // turn off computing tolerance and set it to 11 times what is was.
  // 11 is magic number to get all the points within line1 selected.
  probe->SetComputeTolerance(false);
  probe->SetTolerance(11*probe->GetTolerance());
  probe->Update();

  int validNext = GetNumberOfValidPoints(probe->GetOutput());

  if (validNext != 11)
  {
    return 1;
  }
  // threshold is still set high, but we tell it to ignore it
  probe->SetComputeTolerance(true);
  probe->Update();

  int validIgnore = GetNumberOfValidPoints(probe->GetOutput());
  return (validIgnore == 2) ? 0 : 1;
}

// Currently only tests the ComputeThreshold and Threshold.  Other tests
// should be added
int TestProbeFilter(int, char*[])
{
  return TestProbeFilterThreshold();
}
