/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.h
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
// .NAME vtkCellDataToPointData - map cell data to point data
// .SECTION Description
// vtkCellDataToPointData is a filter that transforms cell data (i.e., data
// specified per cell) into point data (i.e., data specified at cell
// points). The method of transformation is based on averaging the data
// values of all cells using a particular point. Optionally, the input cell
// data can be passed through to the output as well.

// .SECTION Caveats
// This filter is an abstract filter, that is, the output is an abstract type
// (i.e., vtkDataSet). Use the convenience methods (e.g.,
// vtkGetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
// of output you want.

// .SECTION See Also
// vtkDataSetToDataSetFilter vtkPointData vtkCellData vtkPointDataToCellData


#ifndef __vtkCellDataToPointData_h
#define __vtkCellDataToPointData_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkCellDataToPointData : public vtkDataSetToDataSetFilter
{
public:
  vtkCellDataToPointData();
  static vtkCellDataToPointData *New() {return new vtkCellDataToPointData;};
  const char *GetClassName() {return "vtkCellDataToPointData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Control whether the input cell data is to be passed to the output. If
  // on, then the input cell data is passed through to the output; otherwise,
  // only generated point data is placed into the output.
  vtkSetMacro(PassCellData,int);
  vtkGetMacro(PassCellData,int);
  vtkBooleanMacro(PassCellData,int);

protected:
  void Execute();

  int PassCellData;
};

#endif


