/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanTexture.h
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

#include "vtkStructuredPointsSource.hh"

class vtkBooleanTexture : public vtkStructuredPointsSource
{
public:
  vtkBooleanTexture();
  char *GetClassName() {return "vtkBooleanTexture";};
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
  void Execute();

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

};

#endif


