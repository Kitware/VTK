/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPoints.h
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
// .NAME vtkMaskPoints - selectively filter points
// .SECTION Description
// vtkMaskPoints is a filter that passes through points and point attributes 
// from input dataset. (Other geometry is not passed through.) It is 
// possible to mask every nth point, and to specify an initial offset
// to begin masking from. A special random mode feature enables random 
// selection of points.

#ifndef __vtkMaskPoints_h
#define __vtkMaskPoints_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_EXPORT vtkMaskPoints : public vtkDataSetToPolyDataFilter
{
public:
  vtkMaskPoints();
  static vtkMaskPoints *New() {return new vtkMaskPoints;};
  const char *GetClassName() {return "vtkMaskPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth point.
  vtkSetClampMacro(OnRatio,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Limit the number of points that can be passed through.
  vtkSetClampMacro(MaximumNumberOfPoints,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumNumberOfPoints,int);

  // Description:
  // Start with this point.
  vtkSetClampMacro(Offset,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(Offset,int);

  // Description:
  // Special flag causes randomization of point selection. If this mode is on,
  // statically every nth point (i.e., OnRatio) will be displayed.
  vtkSetMacro(RandomMode,int);
  vtkGetMacro(RandomMode,int);
  vtkBooleanMacro(RandomMode,int);

protected:
  void Execute();

  int OnRatio;     // every OnRatio point is on; all others are off.
  int Offset;      // offset (or starting point id)
  int RandomMode;  // turn on/off randomization
  int MaximumNumberOfPoints;
};

#endif


