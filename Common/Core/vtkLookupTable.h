// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLookupTable
 * @brief   map scalar values into colors via a lookup table
 *
 * vtkLookupTable is an object that is used by mapper objects to map scalar
 * values into RGBA (red-green-blue-alpha) color specification,
 * or RGBA into scalar values. The color table can be created by direct
 * insertion of color values, or by specifying a hue, saturation, value, and
 * alpha range and generating a table.
 *
 * A special color for NaN values in the data can be specified via
 * SetNanColor(). In addition, a color for data values below the
 * lookup table range minimum can be specified with
 * SetBelowRangeColor(), and that color will be used for values below
 * the range minimum when UseBelowRangeColor is on.  Likewise, a color
 * for data values above the lookup table range maximum can be
 * specified with SetAboveRangeColor(), and it is used when
 * UseAboveRangeColor is on.
 *
 * This class behaves differently depending on how \a IndexedLookup is set.
 * When true, vtkLookupTable enters a mode for representing categorical color maps.
 * By setting \a IndexedLookup to true, you indicate that the annotated
 * values are the only valid values for which entries in the color table
 * should be returned. The colors in the lookup \a Table are assigned
 * to annotated values by taking the modulus of their index in the list
 * of annotations. \a IndexedLookup changes the behavior of \a GetIndex,
 * which in turn changes the way \a MapScalarsThroughTable2 behaves;
 * when \a IndexedLookup is true, \a MapScalarsThroughTable2 will search for
 * scalar values in \a AnnotatedValues and use the resulting index to
 * determine the color. If a scalar value is not present in \a AnnotatedValues,
 * then \a NanColor will be used.
 *
 * @warning
 * You need to explicitly call Build() when constructing the LUT by hand.
 *
 * @sa
 * vtkLogLookupTable vtkWindowLevelLookupTable
 */

#ifndef vtkLookupTable_h
#define vtkLookupTable_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkScalarsToColors.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include "vtkUnsignedCharArray.h" // Needed for inline method

#define VTK_RAMP_LINEAR 0
#define VTK_RAMP_SCURVE 1
#define VTK_RAMP_SQRT 2
#define VTK_SCALE_LINEAR 0
#define VTK_SCALE_LOG10 1

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT VTK_MARSHALAUTO vtkLookupTable : public vtkScalarsToColors
{
public:
  ///@{
  /**
   * Constants for offsets of special colors (e.g., NanColor, BelowRangeColor,
   * AboveRangeColor) from the maximum index in the lookup table.
   * These should be considered private and not be used by clients of this class.
   */
  static const vtkIdType REPEATED_LAST_COLOR_INDEX;
  static const vtkIdType BELOW_RANGE_COLOR_INDEX;
  static const vtkIdType ABOVE_RANGE_COLOR_INDEX;
  static const vtkIdType NAN_COLOR_INDEX;
  static const vtkIdType NUMBER_OF_SPECIAL_COLORS;
  ///@}

  /**
   * Construct with range=[0,1]; and hsv ranges set up for rainbow color table
   * (from red to blue).
   */
  static vtkLookupTable* New();

  vtkTypeMacro(vtkLookupTable, vtkScalarsToColors);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Return true if all of the values defining the mapping have an opacity
   * equal to 1.
   */
  vtkTypeBool IsOpaque() override;
  vtkTypeBool IsOpaque(vtkAbstractArray* scalars, int colorMode, int component) override;
  vtkTypeBool IsOpaque(vtkAbstractArray* scalars, int colorMode, int component,
    vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip = 0xff) override;
  ///@}

  /**
   * Allocate a color table of specified size.
   * Note that ext is no longer used.
   */
  int Allocate(int sz = 256, int ext = 256);

  /**
   * Generate lookup table from hue, saturation, value, alpha min/max values.
   * Table is built from linear ramp of each value.
   */
  void Build() override;

  /**
   * Force the lookup table to regenerate from hue, saturation, value,
   * and alpha min/max values.  Table is built from a linear ramp of
   * each value.  ForceBuild() is useful if a lookup table has been
   * defined manually (using SetTableValue) and then an application
   * decides to rebuild the lookup table using the implicit process.
   */
  virtual void ForceBuild();

  /**
   * Copies the "special" colors into the given table.
   */
  void BuildSpecialColors();

  ///@{
  /**
   * Set the shape of the table ramp to either S-curve, linear, or sqrt.
   * The default is S-curve, which tails off gradually at either end.
   *
   * The equation used for the S-curve is y = (sin((x - 1/2)*pi) + 1)/2,
   * For an S-curve greyscale ramp, you should set NumberOfTableValues
   * to 402 (which is 256*pi/2) to provide room for the tails of the ramp.
   *
   * The equation for the linear ramp is simply y = x.
   *
   * The equation for the SQRT is y = sqrt(x).
   */
  vtkSetMacro(Ramp, int);
  void SetRampToLinear() { this->SetRamp(VTK_RAMP_LINEAR); }
  void SetRampToSCurve() { this->SetRamp(VTK_RAMP_SCURVE); }
  void SetRampToSQRT() { this->SetRamp(VTK_RAMP_SQRT); }
  vtkGetMacro(Ramp, int);
  ///@}

  ///@{
  /**
   * Set the type of scale to use, linear or logarithmic.  The default
   * is linear.  If the scale is logarithmic, then the TableRange must not
   * cross the value zero.
   */
  void SetScale(int scale);
  void SetScaleToLinear() { this->SetScale(VTK_SCALE_LINEAR); }
  void SetScaleToLog10() { this->SetScale(VTK_SCALE_LOG10); }
  vtkGetMacro(Scale, int);
  ///@}

  ///@{
  /**
   * Set/Get the minimum/maximum scalar values for scalar mapping. Scalar
   * values less than minimum range value are clamped to minimum range value.
   * Scalar values greater than maximum range value are clamped to maximum
   * range value.

   * The \a TableRange values are only used when \a IndexedLookup is false.
   */
  virtual void SetTableRange(const double r[2]);
  virtual void SetTableRange(double min, double max);
  vtkGetVectorMacro(TableRange, double, 2);
  ///@}

  ///@{
  /**
   * Set the range in hue (using automatic generation). Hue ranges
   * between [0,1].
   */
  vtkSetVector2Macro(HueRange, double);
  vtkGetVector2Macro(HueRange, double);
  ///@}

  ///@{
  /**
   * Set the range in saturation (using automatic generation). Saturation
   * ranges between [0,1].
   */
  vtkSetVector2Macro(SaturationRange, double);
  vtkGetVector2Macro(SaturationRange, double);
  ///@}

  ///@{
  /**
   * Set the range in value (using automatic generation). Value ranges
   * between [0,1].
   */
  vtkSetVector2Macro(ValueRange, double);
  vtkGetVector2Macro(ValueRange, double);
  ///@}

  ///@{
  /**
   * Set the range in alpha (using automatic generation). Alpha ranges from
   * [0,1].
   */
  vtkSetVector2Macro(AlphaRange, double);
  vtkGetVector2Macro(AlphaRange, double);
  ///@}

  ///@{
  /**
   * Set the color to use when a NaN (not a number) is encountered.  This is an
   * RGBA 4-tuple of doubles in the range [0,1].
   */
  vtkSetVector4Macro(NanColor, double);
  vtkGetVector4Macro(NanColor, double);
  ///@}

  /**
   * Return the \a NanColor as a pointer to 4 unsigned chars. This
   * will overwrite any data returned by previous calls to MapValue.
   */
  unsigned char* GetNanColorAsUnsignedChars();

  /**
   * Given an RGBA[4] color in the [0,1] range, convert it to RGBA[4] in the [0,255] range.
   */
  static void GetColorAsUnsignedChars(const double colorIn[4], unsigned char colorOut[4]);

  ///@{
  /**
   * Set the color to use when a value below the range is
   * encountered. This is an RGBA 4-tuple of doubles in the range [0, 1].
   */
  vtkSetVector4Macro(BelowRangeColor, double);
  vtkGetVector4Macro(BelowRangeColor, double);
  ///@}

  ///@{
  /**
   * Set whether the below range color should be used.
   */
  vtkSetMacro(UseBelowRangeColor, vtkTypeBool);
  vtkGetMacro(UseBelowRangeColor, vtkTypeBool);
  vtkBooleanMacro(UseBelowRangeColor, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the color to use when a value above the range is
   * encountered. This is an RGBA 4-tuple of doubles in the range [0, 1].
   */
  vtkSetVector4Macro(AboveRangeColor, double);
  vtkGetVector4Macro(AboveRangeColor, double);
  ///@}

  ///@{
  /**
   * Set whether the above range color should be used.
   */
  vtkSetMacro(UseAboveRangeColor, vtkTypeBool);
  vtkGetMacro(UseAboveRangeColor, vtkTypeBool);
  vtkBooleanMacro(UseAboveRangeColor, vtkTypeBool);
  ///@}

  /**
   * Map one value through the lookup table, returning an RBGA[4] color.
   */
  const unsigned char* MapValue(double v) override;

  /**
   * Map one value through the lookup table and return the color as
   * an RGB[3] array of doubles between 0 and 1. Note lack of alpha.
   */
  void GetColor(double v, double rgb[3]) override;

  /**
   * Map one value through the lookup table and return the alpha value
   * (the opacity) as a double between 0 and 1.
   */
  double GetOpacity(double v) override;

  /**
   * Return the table index associated with a particular value.
   * Returns -1 if \a v is NaN.

   * Do not use this function when \a IndexedLookup is true:
   * in that case, the set of values \a v may take on is exactly the integers
   * from 0 to \a GetNumberOfTableValues() - 1;
   * and \a v serves directly as an index into \a TableValues.
   */
  virtual vtkIdType GetIndex(double v);

  ///@{
  /**
   * Specify the number of values (i.e., colors) in the lookup table.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  void SetNumberOfTableValues(vtkIdType number);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  vtkIdType GetNumberOfTableValues() { return this->NumberOfColors; }
  ///@}

  /**
   * Directly load color into lookup table. Use [0,1] double values for color
   * component specification. Make sure that you've either used the
   * Build() method or used SetNumberOfTableValues() prior to using this
   * method.
   */
  virtual void SetTableValue(vtkIdType indx, const double rgba[4]);

  /**
   * Directly load color into lookup table. Use [0,1] double values for color
   * component specification. Alpha defaults to 1 if unspecified.
   */
  virtual void SetTableValue(vtkIdType indx, double r, double g, double b, double a = 1.0);

  /**
   * Return an RGBA color value for the given index into the lookup table. Color
   * components are expressed as [0,1] double values.
   */
  double* GetTableValue(vtkIdType indx) VTK_SIZEHINT(4);

  /**
   * Return an RGBA color value for the given index into the lookup table. Color
   * components are expressed as [0,1] double values.
   */
  void GetTableValue(vtkIdType indx, double rgba[4]);

  /**
   * Get pointer to color table data. Format is array of unsigned char
   * R-G-B-A...R-G-B-A.
   */
  unsigned char* GetPointer(vtkIdType id) { return this->Table->GetPointer(4 * id); }

  /**
   * Get pointer to data. Useful for direct writes into object. MaxId is bumped
   * by number (and memory allocated if necessary). Id is the location you
   * wish to write into; number is the number of rgba values to write.

   * \warning If you modify the table data via the pointer returned by this
   * member function, you must call vtkLookupTable::BuildSpecialColors()
   * afterwards to ensure that the special colors (below/above range and NaN
   * value) are up-to-date.
   */
  unsigned char* WritePointer(vtkIdType id, int number);

  ///@{
  /**
   * Sets/Gets the range of scalars which will be mapped.  This is a duplicate
   * of Get/SetTableRange.
   */
  double* GetRange() VTK_SIZEHINT(2) override { return this->GetTableRange(); }
  void SetRange(double min, double max) override { this->SetTableRange(min, max); }
  void SetRange(const double rng[2]) override { this->SetRange(rng[0], rng[1]); }
  ///@}

  /**
   * Returns the log of \c range in \c log_range.
   * There is a little more to this than simply taking the log10 of the
   * two range values: we do conversion of negative ranges to positive
   * ranges, and conversion of zero to a 'very small number'.
   */
  static void GetLogRange(const double range[2], double log_range[2]);

  /**
   * Apply log to value, with appropriate constraints.
   */
  static double ApplyLogScale(double v, const double range[2], const double log_range[2]);

  ///@{
  /**
   * Set the number of colors in the lookup table.  Use
   * SetNumberOfTableValues() instead, it can be used both before and
   * after the table has been built whereas SetNumberOfColors() has no
   * effect after the table has been built.
   */
  vtkSetClampMacro(NumberOfColors, vtkIdType, 2, VTK_ID_MAX);
  vtkGetMacro(NumberOfColors, vtkIdType);
  ///@}

  ///@{
  /**
   * Set/Get the internal table array that is used to map the scalars
   * to colors.  The table array is an unsigned char array with 4
   * components representing RGBA.
   */
  void SetTable(vtkUnsignedCharArray*);
  vtkGetObjectMacro(Table, vtkUnsignedCharArray);
  ///@}

  /**
   * Map a set of scalars through the lookup table.

   * This member function is thread safe.
   */
  void MapScalarsThroughTable2(void* input, unsigned char* output, int inputDataType,
    int numberOfValues, int inputIncrement, int outputFormat) override;

  /**
   * Copy the contents from another LookupTable.
   */
  void DeepCopy(vtkScalarsToColors* obj) override;

  /**
   * This should return 1 if the subclass is using log scale for mapping scalars
   * to colors. Returns 1 is scale == VTK_SCALE_LOG10.
   */
  vtkTypeBool UsingLogScale() override { return (this->GetScale() == VTK_SCALE_LOG10) ? 1 : 0; }

  /**
   * Get the number of available colors for mapping to.
   */
  vtkIdType GetNumberOfAvailableColors() override;

  /**
   * Return a color given an integer index.

   * This is used to assign colors to annotations (given an offset into the
   * list of annotations).
   * If the table is empty or \a idx < 0, then NanColor is returned.
   */
  void GetIndexedColor(vtkIdType idx, double rgba[4]) override;

protected:
  vtkLookupTable(int sze = 256, int ext = 256);
  ~vtkLookupTable() override;

  vtkIdType NumberOfColors;
  vtkUnsignedCharArray* Table;
  double TableRange[2];
  double HueRange[2];
  double SaturationRange[2];
  double ValueRange[2];
  double AlphaRange[2];
  double NanColor[4];
  double BelowRangeColor[4];
  vtkTypeBool UseBelowRangeColor;
  double AboveRangeColor[4];
  vtkTypeBool UseAboveRangeColor;

  int Scale;
  int Ramp;
  vtkTimeStamp InsertTime;
  vtkTimeStamp BuildTime;
  double RGBA[4]; // used during conversion process
  unsigned char NanColorChar[4];

  vtkTypeBool OpaqueFlag;
  vtkTimeStamp OpaqueFlagBuildTime;
  vtkTimeStamp SpecialColorsBuildTime;

  /**
   * Resize the LookupTable to have enough room for the out-of-range colors
   */
  void ResizeTableForSpecialColors();

private:
  vtkLookupTable(const vtkLookupTable&) = delete;
  void operator=(const vtkLookupTable&) = delete;
};

//----------------------------------------------------------------------------
inline unsigned char* vtkLookupTable::WritePointer(vtkIdType id, int number)
{
  this->InsertTime.Modified();
  return this->Table->WritePointer(4 * id, 4 * number);
}

VTK_ABI_NAMESPACE_END
#endif
