/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColors.h
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
// .NAME vtkScalarsToColors - map scalar values into colors
// .SECTION Description
// vtkScalarsToColors is a general purpose superclass for objects that
// convert scalars to colors. This include vtkLookupTable classes and
// color transfer functions.
//
// .SECTION See Also
// vtkLookupTable vtkColorTransferFunction

#ifndef __vtkScalarsToColors_h
#define __vtkScalarsToColors_h

#include "vtkObject.h"
#include "vtkUnsignedCharArray.h"

#define VTK_RGBA       4
#define VTK_RGB        3
#define VTK_LUMINANCE  1
#define VTK_LUMINANCE_ALPHA 2

class vtkScalars;

class VTK_EXPORT vtkScalarsToColors : public vtkObject
{
public:
  vtkTypeMacro(vtkScalarsToColors,vtkObject);
  
  // Description:
  // Perform any processing required (if any) before processing 
  // scalars.
  virtual void Build() {};
  
  // Description:
  // Sets/Gets the range of scalars which will be mapped.
  virtual float *GetRange() = 0;
  virtual void SetRange(float min, float max) = 0;
  void SetRange(float rng[2]) 
    {this->SetRange(rng[0],rng[1]);}
  
  // Description:
  // Map one value through the lookup table and return a color defined
  // as a RGBA unsigned char tuple (4 bytes).
  virtual unsigned char *MapValue(float v) = 0;

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of floats between 0 and 1.
  virtual void GetColor(float v, float rgb[3]) = 0;

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of floats between 0 and 1.
  float *GetColor(float v) 
    {this->GetColor(v,this->RGB); return this->RGB;}

  // Description:
  // Map one value through the lookup table and return the alpha value
  // (the opacity) as a float between 0 and 1.
  virtual float GetOpacity(float vtkNotUsed(v)) 
    {return 1.0;}

  // Description:
  // Map one value through the lookup table and return the luminance
  // 0.3*red + 0.59*green + 0.11*blue as a float between 0 and 1.
  // Returns the luminance value for the specified scalar value.
  float GetLuminance(float x) 
    {float rgb[3]; this->GetColor(x,rgb);
    return rgb[0]*0.30 + rgb[1]*0.59 + rgb[2]*0.11;}

  // Description:
  // Map a set of scalars through the lookup table in a single operation. 
  // The output format can be set to VTK_RGBA (4 components), 
  // VTK_RGB (3 components), VTK_LUMINANCE (1 component, greyscale),
  // or VTK_LUMINANCE_ALPHA (2 components)
  // If not supplied, the output format defaults to RGBA.
  void MapScalarsThroughTable(vtkScalars *scalars, 
                              unsigned char *output,
                              int outputFormat);
  void MapScalarsThroughTable(vtkScalars *scalars, 
                              unsigned char *output) 
    {this->MapScalarsThroughTable(scalars,output,VTK_RGBA);}


  // Description:
  // An internal method typically not used in applications.
  virtual void MapScalarsThroughTable2(void *input, unsigned char *output,
                                       int inputDataType, int numberOfValues,
                                       int inputIncrement, 
                                       int outputFormat) = 0;

protected:
  vtkScalarsToColors() {};
  ~vtkScalarsToColors() {};
  vtkScalarsToColors(const vtkScalarsToColors &);
  void operator=(const vtkScalarsToColors &);

private:
  float RGB[3];
};

#endif



