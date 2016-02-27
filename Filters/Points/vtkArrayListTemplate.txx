/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayListTemplate.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrayListTemplate.h"


#ifndef vtkArrayListTemplate_txx
#define vtkArrayListTemplate_txx

//----------------------------------------------------------------------------
// Sort of a little object factory (in conjunction w/ vtkTemplateMacro())
template <typename T>
void CreateArrayPair(ArrayList *list, T *inData, T *outData,
                     vtkIdType numPts, int numComp, T nullValue)
{
  ArrayPair<T> *pair = new ArrayPair<T>(inData,outData,numPts,numComp, nullValue);
  list->Arrays.push_back(pair);
}

// Add the arrays to interpolate here
void ArrayList::
AddArrays(vtkIdType numOutPts, vtkDataSetAttributes *inPD, vtkDataSetAttributes *outPD,
          double nullValue)
{
  // Build the vector of interpolation pairs. Note that InterpolateAllocate should have
  // been called at this point (output arrays created and allocated).
  char *name;
  vtkDataArray *iArray, *oArray;
  int iType, oType;
  void *iD, *oD;
  int iNumComp, oNumComp;
  int i, numArrays = outPD->GetNumberOfArrays();
  for (i=0; i < numArrays; ++i)
    {
    oArray = outPD->GetArray(i);
    if ( oArray )
      {
      name = oArray->GetName();
      iArray = inPD->GetArray(name);
      if ( iArray )
        {
        iType = iArray->GetDataType();
        oType = oArray->GetDataType();
        iNumComp = iArray->GetNumberOfComponents();
        oNumComp = oArray->GetNumberOfComponents();
        oArray->SetNumberOfTuples(numOutPts);

        if ( (iType == oType) && (iNumComp == oNumComp) ) //sanity check
          {
          iD = iArray->GetVoidPointer(0);
          oD = oArray->GetVoidPointer(0);
          switch (iType)
            {
            vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT *>(iD),
                             static_cast<VTK_TT *>(oD),numOutPts,oNumComp,
                                             static_cast<VTK_TT>(nullValue)));
            }//over all VTK types
          }//if matching types
        }//if matching input array
      }//if output array
    }//for each candidate array
}

#endif
