/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTCoords.h
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
// .NAME vtkTriangularTCoords - 2D texture coordinates based for triangles.
// .SECTION Description
// vtkTriangularTCoords is a filter that generates texture coordinates
// for triangles. Texture coordinates for each triangle are:
// (0,0), (1,0) and (.5,sqrt(3)/2). This filter assumes that the triangle
// texture map is symmetric about the center of the triangle. Thus the order
// Of the texture coordinates is not important. The procedural texture
// in vtkTriangularTexture is designed with this symmetry. For more information
// see the paper "Opacity-modulating Triangular Textures for Irregular 
// Surfaces,"  by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
// .SECTION See Also
// vtkTriangularTexture vtkThresholdPoints vtkTextureMapToPlane 
// vtkTextureMapToSphere vtkTextureMapToCylinder vtkTextureMapToBox

#ifndef __vtkTriangularTCoords_h
#define __vtkTriangularTCoords_h

#include "vtkPolyToPolyFilter.h"

class VTK_EXPORT vtkTriangularTCoords : public vtkPolyToPolyFilter
{
public:
  char *GetClassName() {return "vtkTriangularTCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  // Usual data generation method
  void Execute();
};

#endif


