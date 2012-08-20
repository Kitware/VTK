/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScalarsToColors - Superclass for mapping scalar values to colors
// .SECTION Description
// vtkScalarsToColors is a general purpose superclass for objects that
// convert scalars to colors. This include vtkLookupTable classes and
// color transfer functions.  By itself, this class will simply rescale
// the scalars
//
// The scalars to color mapping can be augmented with an additional
// uniform alpha blend. This is used, for example, to blend a vtkActor's
// opacity with the lookup table values.
//
// Specific scalar values may be annotated with text strings that will
// be included in color legends using \a SetAnnotations, \a SetAnnotation,
// \a GetNumberOfAnnotatedValues, \a GetAnnotatedValue, \a GetAnnotation,
// \a RemoveAnnotation, and \a ResetAnnotations.
//
// This class also has a method for indicating that the set of annotated values
// form a categorical color map;
// by setting \a IndexedLookup to true, you indicate that the annotated
// values are the only valid values for which entries in the color table
// should be returned.
// In this mode, subclasses should then assign colors
// to annotated values by taking the modulus of an annotated value's index in the list
// of annotations with the number of colors in the table.
//
// .SECTION See Also
// vtkLookupTable vtkColorTransferFunction

#ifndef __vtkScalarsToColors_h
#define __vtkScalarsToColors_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkVariant.h" // Set/get annotation methods require variants.
#include "vtkObject.h"

class vtkDataArray;
class vtkUnsignedCharArray;
class vtkAbstractArray;
class vtkStringArray;


class VTKCOMMONCORE_EXPORT vtkScalarsToColors : public vtkObject
{
public:
  vtkTypeMacro(vtkScalarsToColors,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkScalarsToColors *New();

  // Description:
  // Return true if all of the values defining the mapping have an opacity
  // equal to 1. Default implementation return true.
  virtual int IsOpaque();

  // Description:
  // Perform any processing required (if any) before processing
  // scalars.
  virtual void Build() {};

  // Description:
  // Sets/Gets the range of scalars which will be mapped.
  virtual double *GetRange();
  virtual void SetRange(double min, double max);
  void SetRange(double rng[2])
    {this->SetRange(rng[0],rng[1]);}

  // Description:
  // Map one value through the lookup table and return a color defined
  // as a RGBA unsigned char tuple (4 bytes).
  virtual unsigned char *MapValue(double v);

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of doubles between 0 and 1.
  virtual void GetColor(double v, double rgb[3]);

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of doubles between 0 and 1.
  double *GetColor(double v)
    {this->GetColor(v,this->RGB); return this->RGB;}

  // Description:
  // Map one value through the lookup table and return the alpha value
  // (the opacity) as a double between 0 and 1.
  virtual double GetOpacity(double v);

  // Description:
  // Map one value through the lookup table and return the luminance
  // 0.3*red + 0.59*green + 0.11*blue as a double between 0 and 1.
  // Returns the luminance value for the specified scalar value.
  double GetLuminance(double x)
    {double rgb[3]; this->GetColor(x,rgb);
    return static_cast<double>(rgb[0]*0.30 + rgb[1]*0.59 + rgb[2]*0.11);}

  // Description:
  // Specify an additional opacity (alpha) value to blend with. Values
  // != 1 modify the resulting color consistent with the requested
  // form of the output. This is typically used by an actor in order to
  // blend its opacity.
  virtual void SetAlpha(double alpha);
  vtkGetMacro(Alpha,double);

  // Description:
  // An internal method maps a data array into a 4-component, unsigned char
  // RGBA array. The color mode determines the behavior of mapping. If
  // VTK_COLOR_MODE_DEFAULT is set, then unsigned char data arrays are
  // treated as colors (and converted to RGBA if necessary); otherwise,
  // the data is mapped through this instance of ScalarsToColors. The offset
  // is used for data arrays with more than one component; it indicates
  // which component to use to do the blending.
  // When the component argument is -1, then the this object uses its
  // own selected technique to change a vector into a scalar to map.
  virtual vtkUnsignedCharArray *MapScalars(vtkDataArray *scalars, int colorMode,
                                   int component);

  // Description:
  // Change mode that maps vectors by magnitude vs. component.
  // If the mode is "RGBColors", then the vectors components are
  // scaled to the range and passed directly as the colors.
  vtkSetMacro(VectorMode, int);
  vtkGetMacro(VectorMode, int);
  void SetVectorModeToMagnitude();
  void SetVectorModeToComponent();
  void SetVectorModeToRGBColors();

//BTX
  enum VectorModes {
    MAGNITUDE=0,
    COMPONENT=1,
    RGBCOLORS=2
  };
//ETX


  // Description:
  // If the mapper does not select which component of a vector
  // to map to colors, you can specify it here.
  vtkSetMacro(VectorComponent, int);
  vtkGetMacro(VectorComponent, int);

  // Description:
  // When mapping vectors, consider only the number of components selected
  // by VectorSize to be part of the vector, and ignore any other
  // components.  Set to -1 to map all components.  If this is not set
  // to -1, then you can use SetVectorComponent to set which scalar
  // component will be the first component in the vector to be mapped.
  vtkSetMacro(VectorSize, int);
  vtkGetMacro(VectorSize, int);

  // Description:
  // Map vectors through the lookup table.  Unlike MapScalarsThroughTable,
  // this method will use the VectorMode to decide how to map vectors.
  // The output format can be set to VTK_RGBA (4 components),
  // VTK_RGB (3 components), VTK_LUMINANCE (1 component, greyscale),
  // or VTK_LUMINANCE_ALPHA (2 components)
  void MapVectorsThroughTable(void *input, unsigned char *output,
                              int inputDataType, int numberOfValues,
                              int inputIncrement, int outputFormat,
                              int vectorComponent, int vectorSize);
  void MapVectorsThroughTable(void *input, unsigned char *output,
                              int inputDataType, int numberOfValues,
                              int inputIncrement, int outputFormat)
    { this->MapVectorsThroughTable(input, output, inputDataType, numberOfValues,
                                   inputIncrement, outputFormat, -1, -1); }

  // Description:
  // Map a set of scalars through the lookup table in a single operation.
  // This method ignores the VectorMode and the VectorComponent.
  // The output format can be set to VTK_RGBA (4 components),
  // VTK_RGB (3 components), VTK_LUMINANCE (1 component, greyscale),
  // or VTK_LUMINANCE_ALPHA (2 components)
  // If not supplied, the output format defaults to RGBA.
  void MapScalarsThroughTable(vtkDataArray *scalars,
                              unsigned char *output,
                              int outputFormat);
  void MapScalarsThroughTable(vtkDataArray *scalars,
                              unsigned char *output)
    {this->MapScalarsThroughTable(scalars,output,VTK_RGBA);}
  void MapScalarsThroughTable(void *input, unsigned char *output,
                              int inputDataType, int numberOfValues,
                              int inputIncrement,
                              int outputFormat)
    {this->MapScalarsThroughTable2(input, output, inputDataType,
       numberOfValues, inputIncrement, outputFormat);}

  // Description:
  // An internal method typically not used in applications.  This should
  // be a protected function, but it must be kept public for backwards
  // compatibility.  Never call this method directly.
  virtual void MapScalarsThroughTable2(void *input, unsigned char *output,
                                       int inputDataType, int numberOfValues,
                                       int inputIncrement,
                                       int outputFormat);

  // Description:
  // An internal method used to convert a color array to RGBA. The
  // method instantiates a vtkUnsignedCharArray and returns it. The user is
  // responsible for managing the memory.
  virtual vtkUnsignedCharArray *ConvertUnsignedCharToRGBA(
    vtkUnsignedCharArray *colors, int numComp, int numTuples);

  // Description:
  // Copy the contents from another object.
  virtual void DeepCopy(vtkScalarsToColors *o);

  // Description:
  // This should return 1 is the subclass is using log scale for mapping scalars
  // to colors. Default implementation returns 0.
  virtual int UsingLogScale()
    { return 0; }

  // Description:
  // Get the number of available colors for mapping to.
  virtual vtkIdType GetNumberOfAvailableColors();

  // Description:
  // Set a list of discrete values, either
  // as a categorical set of values (when IndexedLookup is true) or
  // as a set of annotations to add to a scalar array (when IndexedLookup is false).
  // The two arrays must both either be NULL or of the same length or
  // the call will be ignored.
  virtual void SetAnnotations( vtkAbstractArray* values, vtkStringArray* annotations );
  vtkGetObjectMacro(AnnotatedValues,vtkAbstractArray);
  vtkGetObjectMacro(Annotations,vtkStringArray);

  /**\brief Add a new entry (or change an existing entry) to the list of annotated values.
    *
    * Returns the index of \a value in the list of annotations.
    */
  virtual vtkIdType SetAnnotation( vtkVariant value, vtkStdString annotation );

  /// This variant of \a SetAnnotation accepts the value as a string so ParaView can treat annotations as string vector arrays.
  virtual vtkIdType SetAnnotation( vtkStdString value, vtkStdString annotation );

  /// Return the annotated value at a particular index in the list of annotations.
  vtkIdType GetNumberOfAnnotatedValues();

  /// Return the annotated value at a particular index in the list of annotations.
  vtkVariant GetAnnotatedValue( vtkIdType idx );

  /// Return the annotation at a particular index in the list of annotations.
  vtkStdString GetAnnotation( vtkIdType idx );

  /// Return the index of the given value in the list of annotated values (or -1 if not present).
  vtkIdType GetAnnotatedValueIndex( vtkVariant val );

  /// Look up an index into the array of annotations given a value. Does no pointer checks. Returns -1 when \a val not present.
  vtkIdType GetAnnotatedValueIndexInternal( vtkVariant& val );

  /**\brief Remove an existing entry from the list of annotated values.
    *
    * True is returned when the entry was actually removed (i.e., it existed before the call).
    * Otherwise, false is returned.
    */
  virtual bool RemoveAnnotation( vtkVariant value );

  /// Remove all existing values and their annotations.
  virtual void ResetAnnotations();

  // Description:
  // Set/get whether the lookup table is for categorical or ordinal data.
  // The default is ordinal data and values not present in the lookup table
  // will be assigned an interpolated color.
  // When categorical data is present, only values in the lookup table will be
  // considered valid; all other values will be assigned \a NanColor.
  vtkSetMacro(IndexedLookup,int);
  vtkGetMacro(IndexedLookup,int);
  vtkBooleanMacro(IndexedLookup,int);

protected:
  vtkScalarsToColors();
  ~vtkScalarsToColors();

  // Description:
  // An internal method that assumes that the input already has the right
  // colors, and only remaps the range to [0,255] and pads to the desired
  // output format.  If the input has 1 or 2 components, the first component
  // will duplicated if the output format is RGB or RGBA.  If the input
  // has 2 or 4 components, the last component will be used for the alpha
  // if the output format is RGBA or LuminanceAlpha.  If the input has
  // 3 or 4 components but the output is Luminance or LuminanceAlpha,
  // then the components will be combined to compute the luminance.
  // Any components past the fourth component will be ignored.
  void MapColorsToColors(void *input, unsigned char *output,
                         int inputDataType, int numberOfValues,
                         int numberOfComponents, int vectorSize,
                         int outputFormat);

  // Description:
  // An internal method for converting vectors to magnitudes, used as
  // a preliminary step before doing magnitude mapping.
  void MapVectorsToMagnitude(void *input, double *output,
                             int inputDataType, int numberOfValues,
                             int numberOfComponents, int vectorSize);

  /// Allocate annotation arrays if needed, then return the index of the given \a value or -1 if not present.
  virtual vtkIdType CheckForAnnotatedValue( vtkVariant value );
  /// Update the map from annotated values to indices in the array of annotations.
  virtual void UpdateAnnotatedValueMap();

  // Annotations of specific values.
  class vtkInternalAnnotatedValueMap;
  vtkAbstractArray* AnnotatedValues;
  vtkStringArray* Annotations;
  vtkInternalAnnotatedValueMap* AnnotatedValueMap;
  int IndexedLookup;

  double Alpha;

  // How to map arrays with multiple components.
  int VectorMode;
  int VectorComponent;
  int VectorSize;

  // Obsolete, kept so subclasses will still compile
  int UseMagnitude;

private:
  double RGB[3];
  unsigned char RGBABytes[4];
  double InputRange[2];

  vtkScalarsToColors(const vtkScalarsToColors&);  // Not implemented.
  void operator=(const vtkScalarsToColors&);  // Not implemented.
};

#endif



