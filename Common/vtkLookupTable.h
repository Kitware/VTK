/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLookupTable - map scalar values into colors via a lookup table
// .SECTION Description
// vtkLookupTable is an object that is used by mapper objects to map scalar 
// values into rgba (red-green-blue-alpha transparency) color specification, 
// or rgba into scalar values. The color table can be created by direct 
// insertion of color values, or by specifying  hue, saturation, value, and 
// alpha range and generating a table.
//
// .SECTION Caveats
// You need to explicitly call Build() when constructing the LUT by hand.
//
// .SECTION See Also
// vtkLogLookupTable vtkWindowLevelLookupTable

#ifndef __vtkLookupTable_h
#define __vtkLookupTable_h

#include "vtkScalarsToColors.h"

#include "vtkUnsignedCharArray.h" // Needed for inline method

#define VTK_RAMP_LINEAR 0
#define VTK_RAMP_SCURVE 1
#define VTK_RAMP_SQRT 2
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
  // Return true if all of the values defining the mapping have an opacity
  // equal to 1. Default implementation return true.
  virtual int IsOpaque();
  
  // Description:
  // Allocate a color table of specified size.
  int Allocate(int sz=256, int ext=256);
  
  // Description:
  // Generate lookup table from hue, saturation, value, alpha min/max values. 
  // Table is built from linear ramp of each value.
  virtual void Build();

  // Description:
  // Force the lookup table to regenerate from hue, saturation, value,
  // and alpha min/max values.  Table is built from a linear ramp of
  // each value.  ForceBuild() is useful if a lookup table has been
  // defined manually (using SetTableValue) and then an application
  // decides to rebuild the lookup table using the implicit process.
  virtual void ForceBuild();

  // Description:
  // Set the shape of the table ramp to either linear or S-curve.
  // The default is S-curve, which tails off gradually at either end.  
  // The equation used for the S-curve is y = (sin((x - 1/2)*pi) + 1)/2,
  // while the equation for the linear ramp is simply y = x.  For an
  // S-curve greyscale ramp, you should set NumberOfTableValues to 402 
  // (which is 256*pi/2) to provide room for the tails of the ramp.
  // The equation for the SQRT is y = sqrt(x).  
  vtkSetMacro(Ramp,int);
  void SetRampToLinear() { this->SetRamp(VTK_RAMP_LINEAR); };
  void SetRampToSCurve() { this->SetRamp(VTK_RAMP_SCURVE); };
  void SetRampToSQRT() { this->SetRamp(VTK_RAMP_SQRT); };
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
  void SetTableRange(double r[2]); 
  virtual void SetTableRange(double min, double max);
  vtkGetVectorMacro(TableRange,double,2);

  // Description:
  // Set the range in hue (using automatic generation). Hue ranges 
  // between [0,1].
  vtkSetVector2Macro(HueRange,double);
  vtkGetVector2Macro(HueRange,double);

  // Description:
  // Set the range in saturation (using automatic generation). Saturation 
  // ranges between [0,1].
  vtkSetVector2Macro(SaturationRange,double);
  vtkGetVector2Macro(SaturationRange,double);

  // Description:
  // Set the range in value (using automatic generation). Value ranges 
  // between [0,1].
  vtkSetVector2Macro(ValueRange,double);
  vtkGetVector2Macro(ValueRange,double);

  // Description:
  // Set the range in alpha (using automatic generation). Alpha ranges from 
  // [0,1].
  vtkSetVector2Macro(AlphaRange,double);
  vtkGetVector2Macro(AlphaRange,double);

  // Description:
  // Set the color to use when a NaN (not a number) is encountered.  This is an
  // RGBA 4-tuple color of doubles in the range [0,1].
  vtkSetVector4Macro(NanColor, double);
  vtkGetVector4Macro(NanColor, double);

  // Description:
  // Map one value through the lookup table.
  unsigned char *MapValue(double v);

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of doubles between 0 and 1.
  void GetColor(double x, double rgb[3]);

  // Description:
  // Map one value through the lookup table and return the alpha value
  // (the opacity) as a double between 0 and 1.
  double GetOpacity(double v);

  // Description:
  // Return the table index associated with a particular value.
  virtual vtkIdType GetIndex(double v);

  // Description:
  // Specify the number of values (i.e., colors) in the lookup
  // table.
  void SetNumberOfTableValues(vtkIdType number);
  vtkIdType GetNumberOfTableValues() { return this->NumberOfColors; };

  // Description:
  // Directly load color into lookup table. Use [0,1] double values for color
  // component specification. Make sure that you've either used the
  // Build() method or used SetNumberOfTableValues() prior to using this
  // method.
  void SetTableValue(vtkIdType indx, double rgba[4]);

  // Description:
  // Directly load color into lookup table. Use [0,1] double values for color 
  // component specification.
  void SetTableValue(vtkIdType indx, double r, double g, double b, double a=1.0);

  // Description:
  // Return a rgba color value for the given index into the lookup table. Color
  // components are expressed as [0,1] double values.
  double *GetTableValue(vtkIdType id);

  // Description:
  // Return a rgba color value for the given index into the lookup table. Color
  // components are expressed as [0,1] double values.
  void GetTableValue(vtkIdType id, double rgba[4]);

  // Description:
  // Get pointer to color table data. Format is array of unsigned char
  // r-g-b-a-r-g-b-a...
  unsigned char *GetPointer(const vtkIdType id) {
    return this->Table->GetPointer(4*id); };

  // Description:
  // Get pointer to data. Useful for direct writes into object. MaxId is bumped
  // by number (and memory allocated if necessary). Id is the location you 
  // wish to write into; number is the number of rgba values to write.
  unsigned char *WritePointer(const vtkIdType id, const int number);

  // Description:
  // Sets/Gets the range of scalars which will be mapped.  This is a duplicate
  // of Get/SetTableRange.
  double *GetRange() { return this->GetTableRange(); };
  void SetRange(double min, double max) { this->SetTableRange(min, max); };
  void SetRange(double rng[2]) { this->SetRange(rng[0], rng[1]); };

  //BTX
  // Description:
  // Returns the log of \c range in \c log_range.
  // There is a little more to this than simply taking the log10 of the
  // two range values: we do conversion of negative ranges to positive
  // ranges, and conversion of zero to a 'very small number'.
  static void GetLogRange(const double range[2], double log_range[2]);

  // Description:
  // Apply log to value, with appropriate constraints.
  static double ApplyLogScale(double v, const double range[2],
    const double log_range[2]);
  //ETX

  // Description:
  // Set the number of colors in the lookup table.  Use
  // SetNumberOfTableValues() instead, it can be used both before and
  // after the table has been built whereas SetNumberOfColors() has no
  // effect after the table has been built.
  vtkSetClampMacro(NumberOfColors,vtkIdType,2,VTK_LARGE_ID);
  vtkGetMacro(NumberOfColors,vtkIdType);

  // Description:
  // Set/Get the internal table array that is used to map the scalars
  // to colors.  The table array is an unsigned char array with 4
  // components representing RGBA.
  void SetTable(vtkUnsignedCharArray *);
  vtkGetObjectMacro(Table,vtkUnsignedCharArray);

  // Description:
  // map a set of scalars through the lookup table
  void MapScalarsThroughTable2(void *input, unsigned char *output,
                               int inputDataType, int numberOfValues,
                               int inputIncrement, int outputIncrement);

  // Description:
  // Copy the contents from another LookupTable
  void DeepCopy(vtkScalarsToColors *lut);

  // Description:
  // This should return 1 is the subclass is using log scale for mapping scalars
  // to colors. Returns 1 is scale == VTK_SCALE_LOG10.
  virtual int UsingLogScale()
    {
    return (this->GetScale() == VTK_SCALE_LOG10)? 1 : 0;
    }

  // Description:
  // Get the number of available colors for mapping to.
  virtual vtkIdType GetNumberOfAvailableColors();

protected:
  vtkLookupTable(int sze=256, int ext=256);
  ~vtkLookupTable();

  vtkIdType NumberOfColors;
  vtkUnsignedCharArray *Table;
  double TableRange[2];
  double HueRange[2];
  double SaturationRange[2];
  double ValueRange[2];
  double AlphaRange[2];
  double NanColor[4];
  int Scale;
  int Ramp;
  vtkTimeStamp InsertTime;
  vtkTimeStamp BuildTime;
  double RGBA[4]; //used during conversion process

  int OpaqueFlag;
  vtkTimeStamp OpaqueFlagBuildTime;
  
private:
  vtkLookupTable(const vtkLookupTable&);  // Not implemented.
  void operator=(const vtkLookupTable&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline unsigned char *vtkLookupTable::WritePointer(const vtkIdType id, 
                                                   const int number)
{
  this->InsertTime.Modified();
  return this->Table->WritePointer(4*id,4*number);
}

#endif



