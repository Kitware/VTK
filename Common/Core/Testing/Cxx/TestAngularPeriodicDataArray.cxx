/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkAngularPeriodicDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"

#include <limits>

int TestAngularPeriodicDataArray(int, char * [])
{
  vtkNew<vtkDoubleArray> array;
  vtkNew<vtkFloatArray> floatArray;
  vtkNew<vtkAngularPeriodicDataArray<double> > angularPeriodicDataArray;
  vtkNew<vtkAngularPeriodicDataArray<float> > angularPeriodicDataArray2;
  array->SetNumberOfComponents(3);
  floatArray->SetNumberOfComponents(3);

  double tmp[3];
  tmp[0] = 7.77777777777;
  tmp[1] = 12.3;
  tmp[2] = 0.95;

  array->InsertNextTuple(tmp);
  floatArray->InsertNextTuple(tmp);

  angularPeriodicDataArray->InitializeArray(array.Get());
  angularPeriodicDataArray->SetAngle(37.8152145);

  angularPeriodicDataArray2->InitializeArray(floatArray.Get());
  angularPeriodicDataArray2->SetAngle(55.5769061);
  angularPeriodicDataArray2->SetAxis(VTK_PERIODIC_ARRAY_AXIS_Y);

  double pTmp[3];
  angularPeriodicDataArray->GetTypedTuple(0, pTmp);
  float pTmp2[3];
  angularPeriodicDataArray2->GetTypedTuple(0, pTmp2);

  double dEpsilon = std::numeric_limits<double>::epsilon() * 20.0;
  float fEpsilon = std::numeric_limits<float>::epsilon() * 20.f;

  double range[6];
  angularPeriodicDataArray->GetRange(range, 0);
  angularPeriodicDataArray->GetRange(range + 2, 1);
  angularPeriodicDataArray->GetRange(range + 4, 2);

  if (std::abs(pTmp[0] - 7.77777777777) >= dEpsilon ||
      std::abs(pTmp[1] - 9.1344434349507945825) >= dEpsilon ||
      std::abs(pTmp[2] - 8.29182990260197883) >= dEpsilon ||
      std::abs(pTmp2[0] - 5.18041563034058) >= fEpsilon ||
      std::abs(pTmp2[1] - 12.3) >= fEpsilon ||
      std::abs(pTmp2[2] - -5.87874317169189) >= fEpsilon ||
      std::abs(range[0] - 7.77777777777) >= dEpsilon ||
      std::abs(range[2] - 9.1344434349507945825) >= dEpsilon ||
      std::abs(range[4] - 8.29182990260197883) >= dEpsilon
      )
  {
    cerr.precision(20);
    cerr << "Error in vtkAngularPeriodicDataArray : " << endl
         << "Double Array : " << endl << std::abs(pTmp[0] - 7.77777777777) << " "
         << std::abs(pTmp[1] - 9.13444343495079) << " "
         << std::abs(pTmp[2] - 8.29182990260198) << endl << "Float Array : "
         << std::abs(pTmp2[0] - 5.180415) << " "<< std::abs(pTmp2[1] - 12.3)
         << " " << std::abs(pTmp2[2] - -5.878743) << endl
         << "Range : " << endl << std::abs(range[0] - 7.77777777777)
         << std::abs(range[2] - 9.13444343495079) << " "
         << std::abs(range[4] - 8.29182990260198) << endl
         << "Epsilon : " << fEpsilon << " " << dEpsilon << endl;
    return 1;
  }


  tmp[0] = 1.;
  tmp[1] = 1.;
  tmp[2] = 1.;
  angularPeriodicDataArray2->SetCenter(tmp);
  angularPeriodicDataArray2->GetTypedTuple(0, pTmp2);

  if (std::abs(pTmp2[0] - 4.7902297) >= fEpsilon ||
      std::abs(pTmp2[1] - 12.3) >= fEpsilon ||
      std::abs(pTmp2[2] - -4.6191568) >= fEpsilon
     )
  {
    cerr << "Error in vtkAngularPeriodicDataArray : " << endl
         << "Non Zero origin rotation : " << endl << std::abs(pTmp2[0] - 4.7902297) << " "
         << std::abs(pTmp2[1] - 12.3) << " "
         << std::abs(pTmp2[2] - -4.6191568) << endl;
    return 1;
  }

  vtkNew<vtkDoubleArray> tensorArray;
  vtkNew<vtkAngularPeriodicDataArray<double> > tensorPArray;

  tensorArray->SetNumberOfComponents(9);

  double tmp3[9];
  tmp3[0] = 7.77777777777;
  tmp3[1] = 12.3;
  tmp3[2] = 0.95;
  tmp3[3] = -4.325;
  tmp3[4] = -0.00023;
  tmp3[5] = 17.88;
  tmp3[6] = 4030.5;
  tmp3[7] = 1.1;
  tmp3[8] = 19.19191919;

  tensorArray->InsertNextTuple(tmp3);
  tensorPArray->InitializeArray(tensorArray.Get());
  tensorPArray->SetAngle(32);
  tensorPArray->SetAxis(VTK_PERIODIC_ARRAY_AXIS_Z);
  tensorPArray->SetCenter(tmp);

  double pTmp3[9];
  tensorPArray->GetTypedTuple(0, pTmp3);
  if (std::abs(pTmp3[0] - 2.0096597239047708783) >= dEpsilon ||
      std::abs(pTmp3[1] - 13.555918489185591724) >= dEpsilon ||
      std::abs(pTmp3[2] - -8.6693107531410973365) >= dEpsilon ||
      std::abs(pTmp3[3] - -3.0690815108144087198) >= dEpsilon ||
      std::abs(pTmp3[4] - 5.7678880538652288479) >= dEpsilon ||
      std::abs(pTmp3[5] - 15.666523260298440334) >= dEpsilon ||
      std::abs(pTmp3[6] - 3417.4749403678183626) >= dEpsilon ||
      std::abs(pTmp3[7] - 2136.7724473977045818) >= dEpsilon ||
      std::abs(pTmp3[8] - 19.19191919) >= dEpsilon)
  {
      cerr.precision(20);
      cerr << "Error while rotating tensor : " << std::abs(pTmp3[0] - 2.0096597239047708783) << " " <<
      std::abs(pTmp3[1] - 13.555918489185591724) << " " <<
      std::abs(pTmp3[2] - -8.6693107531410973365) << " " <<
      std::abs(pTmp3[3] - -3.0690815108144087198) << " " <<
      std::abs(pTmp3[4] - 5.7678880538652288479) << " " <<
      std::abs(pTmp3[5] - 15.666523260298440334) << " " <<
      std::abs(pTmp3[6] - 3417.4749403678183626) << " " <<
      std::abs(pTmp3[7] - 2136.7724473977045818) << " " <<
      std::abs(pTmp3[8] - 19.19191919) << " " << dEpsilon << endl;
  }
  return 0;
}
