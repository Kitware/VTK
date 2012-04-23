/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBooleanTexture - generate 2D texture map based on combinations of inside, outside, and on region boundary

// .SECTION Description
// vtkBooleanTexture is a filter to generate a 2D texture map based on
// combinations of inside, outside, and on region boundary. The "region" is
// implicitly represented via 2D texture coordinates. These texture
// coordinates are normally generated using a filter like
// vtkImplicitTextureCoords, which generates the texture coordinates for
// any implicit function.
//
// vtkBooleanTexture generates the map according to the s-t texture
// coordinates plus the notion of being in, on, or outside of a
// region. An in region is when the texture coordinate is between
// (0,0.5-thickness/2).  An out region is where the texture coordinate
// is (0.5+thickness/2). An on region is between
// (0.5-thickness/2,0.5+thickness/2). The combination in, on, and out
// for each of the s-t texture coordinates results in 16 possible
// combinations (see text). For each combination, a different value of
// intensity and transparency can be assigned. To assign maximum intensity
// and/or opacity use the value 255. A minimum value of 0 results in
// a black region (for intensity) and a fully transparent region (for
// transparency).

// .SECTION See Also
// vtkImplicitTextureCoords vtkThresholdTextureCoords

#ifndef __vtkBooleanTexture_h
#define __vtkBooleanTexture_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGHYBRID_EXPORT vtkBooleanTexture : public vtkImageAlgorithm
{
public:
  static vtkBooleanTexture *New();

  vtkTypeMacro(vtkBooleanTexture,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the X texture map dimension.
  vtkSetMacro(XSize,int);
  vtkGetMacro(XSize,int);

  // Description:
  // Set the Y texture map dimension.
  vtkSetMacro(YSize,int);
  vtkGetMacro(YSize,int);

  // Description:
  // Set the thickness of the "on" region.
  vtkSetMacro(Thickness,int);
  vtkGetMacro(Thickness,int);

  // Description:
  // Specify intensity/transparency for "in/in" region.
  vtkSetVector2Macro(InIn,unsigned char);
  vtkGetVectorMacro(InIn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "in/out" region.
  vtkSetVector2Macro(InOut,unsigned char);
  vtkGetVectorMacro(InOut,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "out/in" region.
  vtkSetVector2Macro(OutIn,unsigned char);
  vtkGetVectorMacro(OutIn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "out/out" region.
  vtkSetVector2Macro(OutOut,unsigned char);
  vtkGetVectorMacro(OutOut,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "on/on" region.
  vtkSetVector2Macro(OnOn,unsigned char);
  vtkGetVectorMacro(OnOn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "on/in" region.
  vtkSetVector2Macro(OnIn,unsigned char);
  vtkGetVectorMacro(OnIn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "on/out" region.
  vtkSetVector2Macro(OnOut,unsigned char);
  vtkGetVectorMacro(OnOut,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "in/on" region.
  vtkSetVector2Macro(InOn,unsigned char);
  vtkGetVectorMacro(InOn,unsigned char,2);

  // Description:
  // Specify intensity/transparency for "out/on" region.
  vtkSetVector2Macro(OutOn,unsigned char);
  vtkGetVectorMacro(OutOn,unsigned char,2);

protected:
  vtkBooleanTexture();
  ~vtkBooleanTexture() {};

  virtual int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *);
  virtual void ExecuteDataWithInformation(vtkDataObject *data, vtkInformation* outInfo);

  int XSize;
  int YSize;

  int Thickness;
  unsigned char InIn[2];
  unsigned char InOut[2];
  unsigned char OutIn[2];
  unsigned char OutOut[2];
  unsigned char OnOn[2];
  unsigned char OnIn[2];
  unsigned char OnOut[2];
  unsigned char InOn[2];
  unsigned char OutOn[2];

private:
  vtkBooleanTexture(const vtkBooleanTexture&);  // Not implemented.
  void operator=(const vtkBooleanTexture&);  // Not implemented.
};

#endif


