/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test vtkDataSetAttributes API.
#include "vtkAoSDataArrayTemplate.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkSoADataArrayTemplate.h"
#include "vtkStructuredData.h"
#include "vtkTimerLog.h"

template <class InArrayT, class OutArrayT>
bool Test(int numComps, const int inExt[6], const int outExt[6])
{
  vtkNew<InArrayT> inArray;

  //vtkTimerLog::MarkStartEvent("Initialize Arrays");
  int inDims[3], outDims[3];
  vtkStructuredData::GetDimensionsFromExtent(const_cast<int*>(inExt), inDims);
  vtkStructuredData::GetDimensionsFromExtent(const_cast<int*>(outExt), outDims);
  inArray->SetNumberOfComponents(numComps);
  inArray->SetNumberOfTuples(inDims[0] * inDims[1] * inDims[2]);
  inArray->SetName("Array0");

  //for (vtkIdType cc=0; cc < inArray->GetNumberOfTuples(); ++cc)
  //  {
  //  for (int kk=0; kk < numComps; ++kk)
  //    {
  //    //inArray->SetComponent(cc, kk,
  //    //  static_cast<typename InArrayT::ScalarType>(cc));
  //    }
  //  }
  //vtkTimerLog::MarkEndEvent("Initialize Arrays");
  vtkNew<vtkDataSetAttributes> inDSA;
  inDSA->AddArray(inArray.GetPointer());


  /*
  vtkNew<OutArrayT> outArray;
  outArray->SetNumberOfComponents(numComps);
  outArray->SetNumberOfTuples(outDims[0] * outDims[1] * outDims[2]);
  for (vtkIdType cc=0; cc < outArray->GetNumberOfTuples(); ++cc)
    {
    for (int kk=0; kk < numComps; ++kk)
      {
      inArray->SetComponentValue(cc, kk, static_cast<typename InArrayT::ScalarType>(0));
      }
    }

  */

  vtkNew<vtkDataSetAttributes> outDSA;
  //outDSA->AddArray(outArray.GetPointer());
  outDSA->CopyAllocate(inDSA.GetPointer());

  vtkTimerLog::MarkStartEvent("CopyStructuredData");
  outDSA->CopyStructuredData(inDSA.GetPointer(), const_cast<int*>(inExt), const_cast<int*>(outExt));
  vtkTimerLog::MarkEndEvent("CopyStructuredData");

  return true;
}


int TestDataSetAttributes(int, char*[])
{
  int inExt[] = {0, 511, 0, 511, 0, 511};
  int outExt[] = {100, 500, 100, 500, 0, 511};

  for (int kk=0; kk < 2; kk++)
    {
    for (int cc=0; cc < 6; cc++)
      {
      inExt[cc] *= kk+1;
      outExt[cc] *= kk+1;
      outExt[cc] = inExt[cc];
      }

    vtkTimerLog::MarkStartEvent("vtkSoADataArrayTemplate");
    if (!Test<vtkSoADataArrayTemplate<double>, vtkSoADataArrayTemplate<double> >(3, inExt, outExt))
      {
      return EXIT_FAILURE;
      }
    vtkTimerLog::MarkEndEvent("vtkSoADataArrayTemplate");

    vtkTimerLog::MarkStartEvent("vtkAoSDataArrayTemplate");
    if (!Test<vtkAoSDataArrayTemplate<double>, vtkAoSDataArrayTemplate<double> >(3, inExt, outExt))
      {
      return EXIT_FAILURE;
      }
    vtkTimerLog::MarkEndEvent("vtkAoSDataArrayTemplate");

    vtkTimerLog::MarkStartEvent("vtkDoubleArray");
    if (!Test<vtkDoubleArray, vtkDoubleArray>(3, inExt, outExt))
      {
      return EXIT_FAILURE;
      }
    vtkTimerLog::MarkEndEvent("vtkDoubleArray");
    }

  vtkTimerLog::DumpLogWithIndents(&cout, 0);
  return EXIT_SUCCESS;
}
