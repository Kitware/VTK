/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.h
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
// .NAME vtkVectorDot - generate scalars from dot product of vectors and normals (e.g., show displacement plot)
// .SECTION Description
// vtkVectorDot is a filter to generate scalar values from a dataset.
// The scalar value at a point is created by computing the dot product 
// between the normal and vector at that point. Combined with the appropriate
// color map, this can show nodal lines/mode shapes of vibration, or a 
// displacement plot.

#ifndef __vtkVectorDot_h
#define __vtkVectorDot_h

#include "vtkDataSetToDataSetFilter.hh"

class vtkVectorDot : public vtkDataSetToDataSetFilter 
{
public:
  vtkVectorDot();
  char *GetClassName() {return "vtkVectorDot";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify range to map scalars into.
  vtkSetVector2Macro(ScalarRange,float);

  // Description:
  // Get the range that scalars map into.
  vtkGetVectorMacro(ScalarRange,float,2);

protected:
  void Execute();
  float ScalarRange[2];
};

#endif


