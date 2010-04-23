/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitTextureCoords.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkDataSetAlgorithm.h"

class vtkImplicitFunction;

class VTK_GRAPHICS_EXPORT vtkImplicitTextureCoords : public vtkDataSetAlgorithm 
{
public:
  vtkTypeMacro(vtkImplicitTextureCoords,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create object with texture dimension=2 and no r-s-t implicit functions
  // defined and FlipTexture turned off.
  static vtkImplicitTextureCoords *New();
  
  // Description:
  // Specify an implicit function to compute the r texture coordinate.
  virtual void SetRFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(RFunction,vtkImplicitFunction);

  // Description:
  // Specify an implicit function to compute the s texture coordinate.
  virtual void SetSFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(SFunction,vtkImplicitFunction);

  // Description:
  // Specify an implicit function to compute the t texture coordinate.
  virtual void SetTFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(TFunction,vtkImplicitFunction);

  // Description:
  // If enabled, this will flip the sense of inside and outside the implicit
  // function (i.e., a rotation around the r-s-t=0.5 axis).
  vtkSetMacro(FlipTexture,int);
  vtkGetMacro(FlipTexture,int);
  vtkBooleanMacro(FlipTexture,int);  
  
protected:
  vtkImplicitTextureCoords();
  ~vtkImplicitTextureCoords();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkImplicitFunction *RFunction;
  vtkImplicitFunction *SFunction;
  vtkImplicitFunction *TFunction;
  int FlipTexture;
private:
  vtkImplicitTextureCoords(const vtkImplicitTextureCoords&);  // Not implemented.
  void operator=(const vtkImplicitTextureCoords&);  // Not implemented.
};

#endif


