/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPicker.h
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
// .NAME vtkPointPicker - select a point by shooting a ray into a graphics window
// .SECTION Description
// vtkPointPicker is used to select a point by shooting a ray into a graphics
// window and intersecting with actor's defining geometry - specifically 
// its points. Beside returning coordinates, actor, and mapper, vtkPointPicker
// returns the id of the closest point within the tolerance along the pick ray.
// .SECTION See Also
// For quick picking, see vtkPicker. To uniquely pick actors, see vtkCellPicker.

#ifndef __vtkPointPicker_h
#define __vtkPointPicker_h

#include "vtkPicker.h"

class VTK_EXPORT vtkPointPicker : public vtkPicker
{
public:
  vtkPointPicker();
  vtkPointPicker *New() {return new vtkPointPicker;};
  char *GetClassName() {return "vtkPointPicker";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the id of the picked point. If PointId = -1, nothing was picked.
  vtkGetMacro(PointId,int);

protected:
  int PointId; //picked point

  void IntersectWithLine(float p1[3], float p2[3], float tol, 
                         vtkActor *assem, vtkActor *a, vtkMapper *m);
  void Initialize();

};

#endif


