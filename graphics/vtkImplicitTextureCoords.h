/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitTextureCoords.h
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
// .NAME vtkImplicitTextureCoords - generate 1D, 2D, or 3D texture coordinates based on implicit function(s)
// .SECTION Description
// vtkImplicitTextureCoords is a filter to generate 1D, 2D, or 3D texture 
// coordinates from one, two, or three implicit functions, respectively. 
// In combinations with a vtkBooleanTexture map (or another texture map of
// your own creation), the texture coordinates can be used to highlight
//(via color or intensity) or cut (via transparency) dataset geometry without
// any complex geometric processing. (Note: the texture coordinates are 
// referred to as r-s-t coordinates.)
//
// The texture coordinates are automatically normalized to lie between (0,1). 
// Thus, no matter what the implicit functions evaluate to, the resulting 
// texture coordinates lie between (0,1), with the zero implicit function 
// value mapped to the 0.5 texture coordinates value. Depending upon the 
// maximum negative/positive implicit function values, the full (0,1) range 
// may not be occupied (i.e., the positive/negative ranges are mapped using 
// the same scale factor).
//
// A boolean variable InvertTexture is available to flip the texture 
// coordinates around 0.5 (value 1.0 becomes 0.0, 0.25->0.75). This is 
// equivalent to flipping the texture map (but a whole lot easier).

// .SECTION Caveats
// You can use the transformation capabilities of vtkImplicitFunction to
// orient, translate, and scale the implicit functions. Also, the dimension of 
// the texture coordinates is implicitly defined by the number of implicit 
// functions defined.

// .SECTION See Also
// vtkImplicitFunction vtkTexture vtkBooleanTexture vtkTransformTexture

#ifndef __vtkImplicitTextureCoords_h
#define __vtkImplicitTextureCoords_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkImplicitFunction.h"

class vtkImplicitTextureCoords : public vtkDataSetToDataSetFilter 
{
public:
  vtkImplicitTextureCoords();
  char *GetClassName() {return "vtkImplicitTextureCoords";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an implicit function to compute the r texture coordinate.
  vtkSetObjectMacro(RFunction,vtkImplicitFunction);
  vtkGetObjectMacro(RFunction,vtkImplicitFunction);

  // Description:
  // Specify an implicit function to compute the s texture coordinate.
  vtkSetObjectMacro(SFunction,vtkImplicitFunction);
  vtkGetObjectMacro(SFunction,vtkImplicitFunction);

  // Description:
  // Specify an implicit function to compute the t texture coordinate.
  vtkSetObjectMacro(TFunction,vtkImplicitFunction);
  vtkGetObjectMacro(TFunction,vtkImplicitFunction);

  // Description:
  // If enabled, this will flip the sense of inside and outside the implicit
  // function (i.e., a rotation around the r-s-t=0.5 axis).
  vtkSetMacro(FlipTexture,int);
  vtkGetMacro(FlipTexture,int);
  vtkBooleanMacro(FlipTexture,int);

protected:
  void Execute();

  vtkImplicitFunction *RFunction;
  vtkImplicitFunction *SFunction;
  vtkImplicitFunction *TFunction;
  int FlipTexture;
};

#endif


