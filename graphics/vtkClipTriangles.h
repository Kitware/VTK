/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipTriangles.h
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
// .NAME vtkClipTriangles - clip triangles with user-specified implicit function
// .SECTION Description
// vtkClipTriangles is a filter that clips triangles to by a any subclass of 
// vtkImplicitFunction.
// .SECTION See Also
// vtkImplicitFunction vtkCutter

#ifndef __vtkClipTriangles_h
#define __vtkClipTriangles_h

#include "vtkDataSetToPolyFilter.h"
#include "vtkImplicitFunction.h"

class vtkClipTriangles : public vtkDataSetToPolyFilter
{
public:
  vtkClipTriangles(vtkImplicitFunction *cf=NULL);
  char *GetClassName() {return "vtkClipTriangles";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  // Description:
  // Set the clipping value of the implicit function. Default is 0.0.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);
  
  // Description:
  // Set/Get the InsideOut flag. When off, a vertex is considered inside
  // the implicit function if the scalar value at the vertex is <=
  // the iso value. If on, the vertex is inside if the scalar value is
  // > the iso value. InsideOut is off by default.
  // 
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description
  // Specify the implicit function to perform the Clipping.
  vtkSetObjectMacro(ClipFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);


  // Description:
  // Create default locator. Used to create one when none is specified. The locator
  // is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  void Execute();
  vtkImplicitFunction *ClipFunction;
  
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
  int InsideOut;
  float Value;

  void Clip(vtkCell *aCell, float value, vtkFloatScalars *cellScalars, 
	    vtkPointLocator *locator,
            vtkCellArray *verts, 
	    vtkCellArray *lines,
	    vtkCellArray *polys, 
	    vtkFloatScalars *scalars,
	    int InsideOut);
};

#endif
