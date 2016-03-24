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
                     vtkIdType numPts, int numComp, vtkDataArray *outArray, T nullValue)
{
  ArrayPair<T> *pair = new ArrayPair<T>(inData,outData,numPts,numComp,outArray,nullValue);
  list->Arrays.push_back(pair);
}

// Indicate arrays not to process
void ArrayList::
ExcludeArray(vtkDataArray *da)
{
  ExcludedArrays.push_back(da);
}

// Has the specified array been excluded?
bool ArrayList::
IsExcluded(vtkDataArray *da)
{
  return (std::find(ExcludedArrays.begin(), ExcludedArrays.end(), da) != ExcludedArrays.end());
}

// Add an array pair (input,output) using the name provided for the output.
void ArrayList::
AddArrayPair(vtkIdType numPts, vtkDataArray *inArray,
             vtkStdString &outArrayName, double nullValue)
{
  vtkDataArray *outArray = inArray->NewInstance();
  outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
  outArray->SetNumberOfTuples(inArray->GetNumberOfTuples());
  outArray->SetName(outArrayName);
  void *iD = inArray->GetVoidPointer(0);
  void *oD = outArray->GetVoidPointer(0);
  int iType = inArray->GetDataType();

  switch (iType)
    {
    vtkTemplateMacro(CreateArrayPair(this, static_cast<VTK_TT *>(iD),
                     static_cast<VTK_TT *>(oD),numPts,inArray->GetNumberOfComponents(),
                     outArray,static_cast<VTK_TT>(nullValue)));
    }//over all VTK types
}

// Add the arrays to interpolate here. This presumes that vtkDataSetAttributes::CopyData() or
// vtkDataSetAttributes::InterpolateData() has been called, and the input and output array
// names match.
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
    if ( oArray && ! this->IsExcluded(oArray) )
      {
      name = oArray->GetName();
      iArray = inPD->GetArray(name);
      if ( iArray && ! this->IsExcluded(iArray) )
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
                             oArray,static_cast<VTK_TT>(nullValue)));
            }//over all VTK types
          }//if matching types
        }//if matching input array
      }//if output array
    }//for each candidate array
}

#endif
