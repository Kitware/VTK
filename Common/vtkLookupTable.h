/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTable.h
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
// .NAME vtkLookupTable - map scalar values into colors via a lookup table
// .SECTION Description
// vtkLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The color table can be created by direct 
// insertion of color values, or by specifying  hue, saturation, value, and 
// alpha range and generating a table.
//
// .SECTION See Also
// vtkLogLookupTable vtkWindowLevelLookupTable

#ifndef __vtkLookupTable_h
#define __vtkLookupTable_h

#include "vtkScalarsToColors.h"

#define VTK_RAMP_LINEAR 0
#define VTK_RAMP_SCURVE 1
#define VTK_SCALE_LINEAR 0
#define VTK_SCALE_LOG10 1

class VTK_COMMON_EXPORT vtkLookupTable : public vtkScalarsToColors
{
public:
  // Description:
  // Construct with range=[0,1]; and hsv ranges set up for rainbow color table 
  // (from red to blue).
  static vtkLookupTable *New();
  
  vtkTypeMacro(vtkLookupTable,vtkScalarsToColors);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate a color table of specified size.
  int Allocate(int sz=256, int ext=256);
  
  // Description:
  // Generate lookup table from hue, saturation, value, alpha min/max values. 
  // Table is built from linear ramp of each value.
  virtual void Build();

  // Description:
  // Set the shape of the table ramp to either linear or S-curve.
  // The default is S-curve, which tails off gradually at either end.  
  // The equation used for the S-curve is y = (sin((x - 1/2)*pi) + 1)/2,
  // while the equation for the linear ramp is simply y = x.  For an
  // S-curve greyscale ramp, you should set NumberOfTableValues to 402 
  // (which is 256*pi/2) to provide room for the tails of the ramp.
  vtkSetMacro(Ramp,int);
  void SetRampToLinear() { this->SetRamp(VTK_RAMP_LINEAR); };
  void SetRampToSCurve() { this->SetRamp(VTK_RAMP_SCURVE); };
  vtkGetMacro(Ramp,int);

  // Description:
  // Set the type of scale to use, linear or logarithmic.  The default
  // is linear.  If the scale is logarithmic, then the TableRange must not
  // cross the value zero.
  void SetScale(int scale);
  void SetScaleToLinear() { this->SetScale(VTK_SCALE_LINEAR); };
  void SetScaleToLog10() { this->SetScale(VTK_SCALE_LOG10); };
  vtkGetMacro(Scale,int);

  // Description:
  // Set/Get the minimum/maximum scalar values for scalar mapping. Scalar
  // values less than minimum range value are clamped to minimum range value.
  // Scalar values greater than maximum range value are clamped to maximum
  // range value.
  void SetTableRange(float r[2]); 
  virtual void SetTableRange(float min, float max);
  vtkGetVectorMacro(TableRange,float,2);

  // Description:
  // Set the range in hue (using automatic generation). Hue ranges 
  // between [0,1].
  vtkSetVector2Macro(HueRange,float);
  vtkGetVector2Macro(HueRange,float);

  // Description:
  // Set the range in saturation (using automatic generation). Saturation 
  // ranges between [0,1].
  vtkSetVector2Macro(SaturationRange,float);
  vtkGetVector2Macro(SaturationRange,float);

  // Description:
  // Set the range in value (using automatic generation). Value ranges 
  // between [0,1].
  vtkSetVector2Macro(ValueRange,float);
  vtkGetVector2Macro(ValueRange,float);

  // Description:
  // Set the range in alpha (using automatic generation). Alpha ranges from 
  // [0,1].
  vtkSetVector2Macro(AlphaRange,float);
  vtkGetVector2Macro(AlphaRange,float);

  // Description:
  // Map one value through the lookup table.
  unsigned char *MapValue(float v);

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of floats between 0 and 1.
  float *GetColor(float x) { return vtkScalarsToColors::GetColor(x); }
  void GetColor(float x, float rgb[3]);

  // Description:
  // Map one value through the lookup table and return the alpha value
  // (the opacity) as a float between 0 and 1.
  float GetOpacity(float v);  

  // Description:
  // Specify the number of values (i.e., colors) in the lookup
  // table.
  void SetNumberOfTableValues(int number);
  int GetNumberOfTableValues() { return this->NumberOfColors; };

  // Description:
  // Directly load color into lookup table. Use [0,1] float values for color
  // component specification. Make sure that you've either used the
  // Build() method or used SetNumberOfTableValues() prior to using this
  // method.
  void SetTableValue(int indx, float rgba[4]);

  // Description:
  // Directly load color into lookup table. Use [0,1] float values for color 
  // component specification.
  void SetTableValue(int indx, float r, float g, float b, float a=1.0);

  // Description:
  // Return a rgba color value for the given index into the lookup table. Color
  // components are expressed as [0,1] float values.
  float *GetTableValue(int id);

  // Description:
  // Return a rgba color value for the given index into the lookup table. Color
  // components are expressed as [0,1] float values.
  void GetTableValue(int id, float rgba[4]);

  // Description:
  // Get pointer to color table data. Format is array of unsigned char
  // r-g-b-a-r-g-b-a...
  unsigned char *GetPointer(const int id) {
    return this->Table->GetPointer(4*id); };

  // Description:
  // Get pointer to data. Useful for direct writes into object. MaxId is bumped
  // by number (and memory allocated if necessary). Id is the location you 
  // wish to write into; number is the number of rgba values to write.
  unsigned char *WritePointer(const int id, const int number);

  // Description:
  // Sets/Gets the range of scalars which will be mapped.  This is a duplicate
  // of Get/SetTableRange.
  float *GetRange() { return this->GetTableRange(); };
  void SetRange(float min, float max) { this->SetTableRange(min, max); };
  void SetRange(float rng[2]) { this->SetRange(rng[0], rng[1]); };

  // Description:
  // Set the number of colors in the lookup table.  Use
  // SetNumberOfTableValues() instead, it can be used both before and
  // after the table has been built whereas SetNumberOfColors() has no
  // effect after the table has been built.
  vtkSetClampMacro(NumberOfColors,int,2,65535);
  vtkGetMacro(NumberOfColors,int);

  // Description:
  // map a set of scalars through the lookup table
  void MapScalarsThroughTable2(void *input, unsigned char *output,
			       int inputDataType, int numberOfValues,
			       int inputIncrement, int outputIncrement);

protected:
  vtkLookupTable(int sze=256, int ext=256);
  ~vtkLookupTable();

  int NumberOfColors;
  vtkUnsignedCharArray *Table;
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  float AlphaRange[2];
  int Scale;
  int Ramp;
  vtkTimeStamp InsertTime;
  vtkTimeStamp BuildTime;
  float RGBA[4]; //used during conversion process
private:
  vtkLookupTable(const vtkLookupTable&);  // Not implemented.
  void operator=(const vtkLookupTable&);  // Not implemented.
};

inline unsigned char *vtkLookupTable::WritePointer(const int id, 
						   const int number)
{
 return this->Table->WritePointer(4*id,4*number);
}

#endif



