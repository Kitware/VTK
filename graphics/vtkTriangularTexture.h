/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTexture.h
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
// .NAME vtkTriangularTexture - generate 2D triangular texture map
// .SECTION Description
// vtkTriangularTexture is a filter that generates a 2D texture map based on 
// the paper "Opacity-modulating Triangular Textures for Irregular Surfaces,"
// by Penny Rheingans, IEEE Visualization '96, pp. 219-225.
// The textures assume texture coordinates of (0,0), (1.0) and
// (.5, sqrt(23)/2). The sequence of texture values is the same along each
// edge of the triangular texture map. So, the assignment order of texture
// coordinates is arbitrary.

// .SECTION See Also
// vtkTriangularTextureCoords

#ifndef __vtkTriangularTexture_h
#define __vtkTriangularTexture_h

#include "vtkStructuredPointsSource.h"

class vtkTriangularTexture : public vtkStructuredPointsSource
{
public:
  vtkTriangularTexture();
  char *GetClassName() {return "vtkTriangularTexture";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set a Scale Factor.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Set the X texture map dimension. Default is 64.
  vtkSetMacro(XSize,int);
  vtkGetMacro(XSize,int);

  // Description:
  // Set the Y texture map dimension. Default is 64.
  vtkSetMacro(YSize,int);
  vtkGetMacro(YSize,int);

  // Description:
  // Set the texture pattern.
  //    1 = opaque at centroid (default)
  //    2 = opaque at vertices
  //    3 = opaque in rings around vertices
  vtkSetClampMacro(TexturePattern,int,1,3);
  vtkGetMacro(TexturePattern,int);

protected:
  void Execute();

  int XSize;
  int YSize;
  float ScaleFactor;

  int TexturePattern;
};

#endif


