/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.hh
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
// .NAME vtkTransformFilter - transform points and associated normals and vectors
// .SECTION Description
// vtkTransformFilter is a filter to transform point coordinates, and 
// associated point normals and vectors. Other point data is passed
// through the filter.
//
// An alternative method of transformation is to use vtkActor's methods
// to scale, rotate, and translate objects. The difference between the
// two methods is that vtkActor's transformation simply effects where
// objects are rendered (via the graphics pipeline), whereas
// vtkTransformFilter actually modifies point coordinates in the 
// visualization pipeline. This is necessary for some objects 
// (e.g., vtkProbeFilter) that require point coordinates as input.
// .SECTION See Also
// vtkTransform vtkTransformPolyFilter vtkActor
// .EXAMPLE XFormSph.cc

#ifndef __vtkTransformFilter_h
#define __vtkTransformFilter_h

#include "vtkPointSetToPointSetFilter.hh"
#include "vtkTransform.hh"

class vtkTransformFilter : public vtkPointSetToPointSetFilter
{
public:
  vtkTransformFilter() : Transform(NULL) {};
  char *GetClassName() {return "vtkTransformFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  // Description:
  // Specify the transform object used to transform points.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

protected:
  void Execute();
  vtkTransform *Transform;
};

#endif


