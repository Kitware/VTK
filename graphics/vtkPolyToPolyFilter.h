/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyToPolyFilter.h
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
// .NAME vtkPolyToPolyFilter - abstract filter class
// .SECTION Description
// vtkPolyToPolyFilter is an abstract filter class whose subclasses take
// as input polygonal data and generate polygonal data on output.

// .SECTION See Also
// vtkCleanPolyData vtkDecimate vtkFeatureEdges vtkFeatureVertices vtkMaskPolyData
// vtkPolyNormals vtkSmoothPolyFilter vtkStripper vtkTransformPolyFilter vtkTriangleFilter
// vtkTubeFilter vtkLinearExtrusionFilter vtkRibbonFilter vtkRotationalExtrusionFilter
// vtkShrinkPolyData

#ifndef __vtkPolyToPolyFilter_h
#define __vtkPolyToPolyFilter_h

#include "vtkPolyFilter.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkPolyToPolyFilter : public vtkPolyFilter
{
public:
  vtkPolyToPolyFilter();
  char *GetClassName() {return "vtkPolyToPolyFilter";};

  // Description:
  // Get the output of this filter.
  vtkPolyData *GetOutput() {return (vtkPolyData *)this->Output;};

};

#endif


