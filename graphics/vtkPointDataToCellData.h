/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkPointDataToCellData - map point data to cell data
// .SECTION Description
// vtkPointDataToCellData is a filter that transforms point data (i.e., data
// specified per point) into point data (i.e., data specified per cell).
// The method of transformation is based on averaging the data
// values of all points defining a particular cell. Optionally, the input point
// data can be passed through to the output as well.

// .SECTION Caveats
// This filter is an abstract filter, that is, the output is an abstract type
// (i.e., vtkDataSet). Use the convenience methods (e.g.,
// vtkGetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
// of output you want.

// .SECTION See Also
// vtkDataSetToDataSetFilter vtkPointData vtkCellData vtkCellDataToPointData


#ifndef __vtkPointDataToCellData_h
#define __vtkPointDataToCellData_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkPointDataToCellData : public vtkDataSetToDataSetFilter
{
public:

// Description:
// Instantiate object so that point data is not passed to output.
  vtkPointDataToCellData();

  static vtkPointDataToCellData *New() {return new vtkPointDataToCellData;};
  const char *GetClassName() {return "vtkPointDataToCellData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Control whether the input point data is to be passed to the output. If
  // on, then the input point data is passed through to the output; otherwise,
  // only generated point data is placed into the output.
  vtkSetMacro(PassPointData,int);
  vtkGetMacro(PassPointData,int);
  vtkBooleanMacro(PassPointData,int);

protected:
  void Execute();

  int PassPointData;
};

#endif


