/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgePoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkEdgePoints - generate points on isosurface
// .SECTION Description
// vtkEdgePoints is a filter that takes as input any dataset and 
// generates for output a set of points that lie on an isosurface. The 
// points are created by interpolation along cells edges whose end-points are 
// below and above the contour value.
// .SECTION Caveats
// vtkEdgePoints can be considered a "poor man's" dividing cubes algorithm
// (see vtkDividingCubes). Points are generated only on the edges of cells, 
// not in the interior, and at lower density than dividing cubes. However, it 
// is more general than dividing cubes since it treats any type of dataset.

#ifndef __vtkEdgePoints_h
#define __vtkEdgePoints_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkMergePoints.h"

class VTK_EXPORT vtkEdgePoints : public vtkDataSetToPolyDataFilter
{
public:
  vtkEdgePoints();
  static vtkEdgePoints *New() {return new vtkEdgePoints;};
  char *GetClassName() {return "vtkEdgePoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the contour value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

protected:
  void Execute();

  float Value;
  vtkMergePoints Locator;
};

#endif


