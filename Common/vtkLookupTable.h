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
// .NAME vtkLookupTable - map scalar values into colors or colors to scalars; generate color table
// .SECTION Description
// vtkLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The color table can be created by direct 
// insertion of color values, or by specifying  hue, saturation, value, and 
// alpha range and generating a table.
//
// This class is designed as a base class for derivation by other classes. 
// The Build(), MapValue(), MapValues(), and SetTableRange() methods are 
// virtual and may require overloading in subclasses.
//
// .SECTION Caveats
// vtkLookupTable is a reference counted object. Therefore, you should 
// always use operator "new" to construct new objects. This procedure will
// avoid memory problems (see text).
//
// .SECTION See Also
// vtkLogLookupTable vtkWindowLevelLookupTable

#ifndef __vtkLookupTable_h
#define __vtkLookupTable_h

#include "vtkScalarsToColors.h"

class vtkScalars;

class VTK_EXPORT vtkLookupTable : public vtkScalarsToColors
{
public:
  // Description:
  // Construct with range=(0,1); and hsv ranges set up for rainbow color table 
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
  // Set the number of colors in the lookup table. Use this method before
  // building the lookup table. Use SetNumberOfTableValues() to change the
  // table size after the lookup table has been built.
  vtkSetClampMacro(NumberOfColors,int,2, 65536);
  vtkGetMacro(NumberOfColors,int);

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
  // between (0,1).
  vtkSetVector2Macro(HueRange,float);
  vtkGetVectorMacro(HueRange,float,2);

  // Description:
  // Set the range in saturation (using automatic generation). Saturation 
  // ranges between (0,1).
  vtkSetVector2Macro(SaturationRange,float);
  vtkGetVectorMacro(SaturationRange,float,2);

  // Description:
  // Set the range in value (using automatic generation). Value ranges 
  // between (0,1).
  vtkSetVector2Macro(ValueRange,float);
  vtkGetVectorMacro(ValueRange,float,2);

  // Description:
  // Set the range in alpha (using automatic generation). Alpha ranges from 
  // (0,1).
  vtkSetVector2Macro(AlphaRange,float);
  vtkGetVectorMacro(AlphaRange,float,2);

  // Description:
  // Map one value through the lookup table.
  unsigned char *MapValue(float v);

  // Description:
  // map a set of scalars through the lookup table
  void MapScalarsThroughTable2(void *input, unsigned char *output,
			       int inputDataType, int numberOfValues,
			       int inputIncrement, int outputIncrement);
    

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of floats between 0 and 1.
  float *GetColor(float x) { 
    return vtkScalarsToColors::GetColor(x); }
  void GetColor(float x, float rgb[3]);

  // Description:
  // Map one value through the lookup table and return the alpha value
  // (the opacity) as a float between 0 and 1.
  float GetOpacity(float v);  

  // Description:
  // Specify the number of values (i.e., colors) in the lookup
  // table. This method simply allocates memory and prepares the table
  // for use with SetTableValue(). It differs from Build() method in
  // that the allocated memory is not initialized according to HSVA ramps.
  void SetNumberOfTableValues(int number);
  
  // Description:
  // Directly load color into lookup table. Use [0,1] float values for color
  // component specification. Make sure that you've either used the
  // Build() method or used SetNumberOfTableValues() prior to using this
  // method.
  void SetTableValue (int indx, float rgba[4]);

  // Description:
  // Directly load color into lookup table. Use [0,1] float values for color 
  // component specification.
  void SetTableValue (int indx, float r, float g, float b, float a=1.0);

  // Description:
  // Return a rgba color value for the given index into the lookup table. Color
  // components are expressed as [0,1] float values.
  float *GetTableValue (int id);

  // Description:
  // Return a rgba color value for the given index into the lookup table. Color
  // components are expressed as [0,1] float values.
  void GetTableValue (int id, float rgba[4]);

  // Description:
  // Get pointer to color table data. Format is array of unsigned char
  // r-g-b-a-r-g-b-a...
  unsigned char *GetPointer(const int id){return this->Table->GetPointer(4*id);};

  // Description:
  // Get pointer to data. Useful for direct writes into object. MaxId is bumped
  // by number (and memory allocated if necessary). Id is the location you 
  // wish to write into; number is the number of rgba values to write.
  unsigned char *WritePointer(const int id, const int number);

  // Description:
  // Sets/Gets the range of scalars which will be mapped.
  float *GetRange() {return this->GetTableRange();};
  void SetRange(float min, float max) {this->SetTableRange(min,max);};
  void SetRange(float rng[2]) {this->SetRange(rng[0],rng[1]);};

protected:
  vtkLookupTable(int sze=256, int ext=256);
  ~vtkLookupTable();
  vtkLookupTable(const vtkLookupTable&) {};
  void operator=(const vtkLookupTable&) {};

  int NumberOfColors;
  vtkUnsignedCharArray *Table;
  float TableRange[2];
  float HueRange[2];
  float SaturationRange[2];
  float ValueRange[2];
  float AlphaRange[2];
  vtkTimeStamp InsertTime;
  vtkTimeStamp BuildTime;
  float RGBA[4]; //used during conversion process
};

inline unsigned char *vtkLookupTable::WritePointer(const int id, 
						   const int number)
{
 return this->Table->WritePointer(4*id,4*number);
}

#endif



