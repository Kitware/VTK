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
  angularPeriodicDataArray->GetTupleValue(0, pTmp);
  float pTmp2[3];
  angularPeriodicDataArray2->GetTupleValue(0, pTmp2);

  double dEpsilon = std::numeric_limits<double>::epsilon() * 20.0;
  float fEpsilon = std::numeric_limits<float>::epsilon() * 20.f;

  double range[6];
  angularPeriodicDataArray->GetRange(range, 0);
  angularPeriodicDataArray->GetRange(range + 2, 1);
  angularPeriodicDataArray->GetRange(range + 4, 2);

  if (std::abs(pTmp[0] - 7.77777777777) >= dEpsilon ||
      std::abs(pTmp[1] - 9.134443205595016) >= dEpsilon ||
      std::abs(pTmp[2] - 8.291829985380172) >= dEpsilon ||
      std::abs(pTmp2[0] - 5.180415) >= fEpsilon ||
      std::abs(pTmp2[1] - 12.3) >= fEpsilon ||
      std::abs(pTmp2[2] - -5.878743) >= fEpsilon ||
      std::abs(range[0] - 7.77777777777) >= dEpsilon ||
      std::abs(range[2] - 9.134443205595016) >= dEpsilon ||
      std::abs(range[4] - 8.291829985380172) >= dEpsilon
      )
    {
    cerr << "Error in vtkAngularPeriodicDataArray : " << endl
         << "Double Array : " << endl << std::abs(pTmp[0] - 7.77777777777) << " "
         << std::abs(pTmp[1] - 9.134443205595016) << " "
         << std::abs(pTmp[2] - 8.291829985380172) << endl << "Float Array : "
         << std::abs(pTmp2[0] - 5.180415) << " "<< std::abs(pTmp2[1] - 12.3)
         << " " << std::abs(pTmp2[2] - -5.878743) << endl;
    return 1;
    }


  tmp[0] = 1.;
  tmp[1] = 1.;
  tmp[2] = 1.;
  angularPeriodicDataArray2->SetCenter(tmp);
  angularPeriodicDataArray2->GetTupleValue(0, pTmp2);

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
  tensorPArray->GetTupleValue(0, pTmp3);
  if (std::abs(pTmp3[0] - 2.0096596603995191543) >= dEpsilon ||
      std::abs(pTmp3[1] - 13.555918386315806146) >= dEpsilon ||
      std::abs(pTmp3[2] - -8.669310802221298573) >= dEpsilon ||
      std::abs(pTmp3[3] - -3.0690815099231580909) >= dEpsilon ||
      std::abs(pTmp3[4] - 5.7678880688287419432) >= dEpsilon ||
      std::abs(pTmp3[5] - 15.666523169279097161) >= dEpsilon ||
      std::abs(pTmp3[6] - 3417.474919319152832) >= dEpsilon ||
      std::abs(pTmp3[7] - 2136.7724573373793646) >= dEpsilon ||
      std::abs(pTmp3[8] - 19.19191919) >= dEpsilon)
    {
      cerr << "Error while rotating tensor : " << std::abs(pTmp3[0] - 2.0096596603995191543) << " " <<
      std::abs(pTmp3[1] - 13.555918386315806146) << " " <<
      std::abs(pTmp3[2] - -8.669310802221298573) << " " <<
      std::abs(pTmp3[3] - -3.0690815099231580909) << " " <<
      std::abs(pTmp3[4] - 5.7678880688287419432) << " " <<
      std::abs(pTmp3[5] - 15.666523169279097161) << " " <<
      std::abs(pTmp3[6] - 3417.474919319152832) << " " <<
      std::abs(pTmp3[7] - 2136.7724573373793646) << " " <<
      std::abs(pTmp3[8] - 19.19191919) << " " << dEpsilon << endl;
    }
  return 0;
}
