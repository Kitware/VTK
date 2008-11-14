/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBoxUtilities.hxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description:
//
// This is a collection of utilitiy functions that operate on
// data definied on regions given by vtkAMRBox.
//
#ifndef __vtkAMRBoxUtilities_hxx
#define __vtkAMRBoxUtilities_hxx

#include "vtkAMRBox.h"
#include "vtkType.h"

//*****************************************************************************
// Description:
// Fill the region of "pArray" enclosed by "destRegion" with "fillValue"
// "pArray" is defined on "arrayRegion".
template <typename T>
void FillRegion(
        T *pArray,
        const vtkAMRBox &arrayRegion,
        const vtkAMRBox &destRegion,
        T fillValue)
{
  // Convert regions to array index space. VTK arrays
  // always start with 0,0,0.
  int ofs[3];
  arrayRegion.GetLoCorner(ofs);
  ofs[0]=-ofs[0];
  ofs[1]=-ofs[1];
  ofs[2]=-ofs[2];
  vtkAMRBox arrayDims(arrayRegion);
  arrayDims.Shift(ofs);
  vtkAMRBox destDims(destRegion);
  destDims.Shift(ofs);
  // Quick sanity check.
  if (!arrayRegion.Contains(destRegion))
    {
    cerr << "ERROR: Array must enclose the destination region. "
         << "Aborting the fill." << endl;
    }
  // Get the bounds of the indices we fill.
  int destLo[3];
  destDims.GetLoCorner(destLo);
  int destHi[3];
  destDims.GetHiCorner(destHi);
  // Get the array dimensions.
  int arrayHi[3];
  arrayDims.GetNumberOfCells(arrayHi);
  // Fill.
  for (int k=destLo[2]; k<=destHi[2]; ++k) 
    {
    vtkIdType kOfs=k*arrayHi[0]*arrayHi[1];
    for (int j=destLo[1]; j<=destHi[1]; ++j) 
      {
      vtkIdType idx=kOfs+j*arrayHi[0]+destLo[0];
      for (int i=destLo[0]; i<=destHi[0]; ++i)
        {
        pArray[idx]=fillValue;
        ++idx;
        }
      }
    }
}

#endif
