/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorTopology.h
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
// .NAME vtkVectorTopology - mark points where the vector field vanishes (singularities exist).
// .SECTION Description
// vtkVectorTopology is a filter that marks points where the vector field 
// vanishes. At these points various important flow features are found, 
// including regions of circulation, separation, etc. The region around these
// areas are good places to start streamlines. (The vector field vanishes in 
// cells where the x-y-z vector components each pass through zero.)
//
// The output of this filter is a set of vertices. These vertices mark the 
// vector field singularities. You can use an object like vtkGlyph3D to place
// markers at these points, or use the vertices to initiate streamlines.
//
// The Distance instance variable controls the accuracy of placement of the
// vertices. Smaller values result in greater execution times.
//
// The input to this filter is any dataset type. The position of the 
// vertices is found by sampling the cell in parametric space. Sampling is
// repeated until the Distance criterion is satisfied.
// .SECTION See Also
// vtkGlyph3D vtkStreamLine

#ifndef __vtkVectorTopology_h
#define __vtkVectorTopology_h

#include "vtkDataSetToPolyFilter.h"

class vtkVectorTopology : public vtkDataSetToPolyFilter
{
public:
  vtkVectorTopology();
  char *GetClassName() {return "vtkVectorTopology";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify distance from singularity to generate point.
  vtkSetClampMacro(Distance,float,1.0e-06,VTK_LARGE_FLOAT);
  vtkGetMacro(Distance,float);

protected:
  void Execute();
  float Distance;
};

#endif


