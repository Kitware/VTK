/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApproximatingSubdivisionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    This work was supported bt PHS Research Grant No. 1 P41 RR13218-01
             from the National Center for Research Resources


Copyright (c) 1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkApproximatingSubdivisionFilter - generate a subdivision surface using an Approximating Scheme
// .SECTION Description
// vtkApproximatingSubdivisionFilter is an abstract class that defines
// the protocol for Approximating subdivision surface filters.

#ifndef __vtkApproximatingSubdivisionFilter_h
#define __vtkApproximatingSubdivisionFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkCellArray.h"

class VTK_EXPORT vtkApproximatingSubdivisionFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkApproximatingSubdivisionFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the number of subdivisions.
  vtkSetMacro(NumberOfSubdivisions,int);
  vtkGetMacro(NumberOfSubdivisions,int);

protected:
  vtkApproximatingSubdivisionFilter();
  ~vtkApproximatingSubdivisionFilter() {};
  vtkApproximatingSubdivisionFilter(const vtkApproximatingSubdivisionFilter&) {};
  void operator=(const vtkApproximatingSubdivisionFilter&) {};

  void Execute();
  virtual void GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD) = 0;
  void GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys, vtkCellData *outputCD);
  int FindEdge (vtkPolyData *mesh, int cellId, int p1, int p2,
		vtkIntArray *edgeData, vtkIdList *cellIds);
  int InterpolatePosition (vtkPoints *inputPts, vtkPoints *outputPts, vtkIdList *stencil, float *weights);
  int NumberOfSubdivisions;
};

#endif


