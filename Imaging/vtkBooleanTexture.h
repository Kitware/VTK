/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanTexture.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include "vtkStructuredPointsSource.h"

class VTK_IMAGING_EXPORT vtkBooleanTexture : public vtkStructuredPointsSource
{
public:
  static vtkBooleanTexture *New();

  vtkTypeMacro(vtkBooleanTexture,vtkStructuredPointsSource);
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
  vtkBooleanTexture(const vtkBooleanTexture&);
  void operator=(const vtkBooleanTexture&);

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


