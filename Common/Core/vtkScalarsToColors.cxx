/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarsToColors.h"

#include "vtkAbstractArray.h"
#include "vtkCharArray.h"
#include "vtkStringArray.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"
#include "vtkObjectFactory.h"

#include <map>

#include <cmath>

// A helper map for quick lookups of annotated values.
class vtkScalarsToColors::vtkInternalAnnotatedValueMap :
  public std::map<vtkVariant,vtkIdType>
{
};

vtkStandardNewMacro(vtkScalarsToColors);

//----------------------------------------------------------------------------
vtkScalarsToColors::vtkScalarsToColors()
{
  this->Alpha = 1.0;
  this->VectorComponent = 0;
  this->VectorSize = -1;
  this->VectorMode = vtkScalarsToColors::COMPONENT;

  // only used in this class, not used in subclasses
  this->InputRange[0] = 0.0;
  this->InputRange[1] = 255.0;

  // Annotated values, their annotations, and whether colors
  // should be indexed by annotated value.
  this->AnnotatedValues = 0;
  this->Annotations = 0;
  this->AnnotatedValueMap = new vtkInternalAnnotatedValueMap;
  this->IndexedLookup = 0;

  // obsolete, kept for backwards compatibility
  this->UseMagnitude = 0;
}

//----------------------------------------------------------------------------
vtkScalarsToColors::~vtkScalarsToColors()
{
  if (this->AnnotatedValues)
  {
    this->AnnotatedValues->UnRegister(this);
  }
  if (this->Annotations)
  {
    this->Annotations->UnRegister(this);
  }
  delete this->AnnotatedValueMap;
}

//----------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1. Default implementation return true.
int vtkScalarsToColors::IsOpaque()
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToComponent()
{
  this->SetVectorMode(vtkScalarsToColors::COMPONENT);
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToMagnitude()
{
  this->SetVectorMode(vtkScalarsToColors::MAGNITUDE);
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToRGBColors()
{
  this->SetVectorMode(vtkScalarsToColors::RGBCOLORS);
}

//----------------------------------------------------------------------------
// do not use SetMacro() because we do not want the table to rebuild.
void vtkScalarsToColors::SetAlpha(double alpha)
{
  this->Alpha = (alpha < 0.0 ? 0.0 : (alpha > 1.0 ? 1.0 : alpha));
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetRange(double minval, double maxval)
{
  if (this->InputRange[0] != minval ||
      this->InputRange[1] != maxval)
  {
    this->InputRange[0] = minval;
    this->InputRange[1] = maxval;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double *vtkScalarsToColors::GetRange()
{
  return this->InputRange;
}

//----------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::GetNumberOfAvailableColors()
{
  // return total possible RGB colors
  return 256*256*256;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::DeepCopy(vtkScalarsToColors *obj)
{
  if (obj)
  {
    this->Alpha = obj->Alpha;
    this->VectorMode = obj->VectorMode;
    this->VectorComponent = obj->VectorComponent;
    this->VectorSize = obj->VectorSize;
    this->InputRange[0] = obj->InputRange[0];
    this->InputRange[1] = obj->InputRange[1];
    this->IndexedLookup = obj->IndexedLookup;
    if (obj->AnnotatedValues && obj->Annotations)
    {
      vtkAbstractArray* annValues = vtkAbstractArray::CreateArray(
        obj->AnnotatedValues->GetDataType());
      vtkStringArray* annotations = vtkStringArray::New();
      annValues->DeepCopy(obj->AnnotatedValues);
      annotations->DeepCopy(obj->Annotations);
      this->SetAnnotations(annValues, annotations);
      annValues->Delete();
      annotations->Delete();
    }
    else
    {
      this->SetAnnotations(0, 0);
    }
  }
}

//----------------------------------------------------------------------------
inline void vtkScalarsToColorsComputeShiftScale(
  vtkScalarsToColors *self, double &shift, double &scale)
{
  static const double minscale = -1e17;
  static const double maxscale = 1e17;

  const double *range = self->GetRange();
  shift = -range[0];
  scale = range[1] - range[0];
  if (scale*scale > 1e-30)
  {
    scale = 1.0/scale;
  }
  else
  {
    scale = (scale < 0.0 ? minscale : maxscale);
  }
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::GetColor(double v, double rgb[3])
{
  static double minval = 0.0;
  static double maxval = 1.0;

  double shift, scale;
  vtkScalarsToColorsComputeShiftScale(this, shift, scale);

  double val = (v + shift)*scale;
  val = (val > minval ? val : minval);
  val = (val < maxval ? val : maxval);

  rgb[0] = val;
  rgb[1] = val;
  rgb[2] = val;
}

//----------------------------------------------------------------------------
double vtkScalarsToColors::GetOpacity(double vtkNotUsed(v))
{
  return 1.0;
}

//----------------------------------------------------------------------------
unsigned char *vtkScalarsToColors::MapValue(double v)
{
  double rgb[3];

  this->GetColor(v, rgb);
  double alpha = this->GetOpacity(v);

  this->RGBABytes[0] = ColorToUChar(rgb[0]);
  this->RGBABytes[1] = ColorToUChar(rgb[1]);
  this->RGBABytes[2] = ColorToUChar(rgb[2]);
  this->RGBABytes[3] = ColorToUChar(alpha);

  return this->RGBABytes;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *vtkScalarsToColors::MapScalars(vtkDataArray *scalars,
                                                     int colorMode, int component)
{
  return this->MapScalars(static_cast<vtkAbstractArray*>(scalars), colorMode, component);
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *vtkScalarsToColors::MapScalars(vtkAbstractArray *scalars,
                                                     int colorMode, int component)
{
  int numberOfComponents = scalars->GetNumberOfComponents();
  vtkUnsignedCharArray *newColors;

  vtkDataArray *dataArray = vtkArrayDownCast<vtkDataArray>(scalars);

  // map scalars through lookup table only if needed
  if ((colorMode == VTK_COLOR_MODE_DEFAULT &&
       vtkArrayDownCast<vtkUnsignedCharArray>(dataArray) != NULL) ||
      (colorMode == VTK_COLOR_MODE_DIRECT_SCALARS && dataArray))
  {
    newColors = this->
      ConvertToRGBA(dataArray, scalars->GetNumberOfComponents(),
                    dataArray->GetNumberOfTuples());
  }
  else
  {
    newColors = vtkUnsignedCharArray::New();
    newColors->SetNumberOfComponents(4);
    newColors->SetNumberOfTuples(scalars->GetNumberOfTuples());

    // If mapper did not specify a component, use the VectorMode
    if (component < 0 && numberOfComponents > 1)
    {
      this->MapVectorsThroughTable(scalars->GetVoidPointer(0),
                                   newColors->GetPointer(0),
                                   scalars->GetDataType(),
                                   scalars->GetNumberOfTuples(),
                                   scalars->GetNumberOfComponents(),
                                   VTK_RGBA);
    }
    else
    {
      if (component < 0)
      {
        component = 0;
      }
      if (component >= numberOfComponents)
      {
        component = numberOfComponents - 1;
      }

      // Map the scalars to colors
      this->MapScalarsThroughTable(scalars->GetVoidPointer(component),
                                   newColors->GetPointer(0),
                                   scalars->GetDataType(),
                                   scalars->GetNumberOfTuples(),
                                   scalars->GetNumberOfComponents(),
                                   VTK_RGBA);
    }
  }

  return newColors;
}

//----------------------------------------------------------------------------
// Map a set of vector values through the table
void vtkScalarsToColors::MapVectorsThroughTable(
  void *input, unsigned char *output, int scalarType,
  int numValues, int inComponents, int outputFormat,
  int vectorComponent, int vectorSize)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapVectorsThroughTable: unrecognized color format");
    return;
  }

  int vectorMode = this->GetVectorMode();
  if (vectorMode == vtkScalarsToColors::COMPONENT)
  {
    // make sure vectorComponent is within allowed range
    if (vectorComponent == -1)
    {
      // if set to -1, use default value provided by table
      vectorComponent = this->GetVectorComponent();
    }
    if (vectorComponent < 0)
    {
      vectorComponent = 0;
    }
    if (vectorComponent >= inComponents)
    {
      vectorComponent = inComponents - 1;
    }
  }
  else
  {
    // make sure vectorSize is within allowed range
    if (vectorSize == -1)
    {
      // if set to -1, use default value provided by table
      vectorSize = this->GetVectorSize();
    }
    if (vectorSize <= 0)
    {
      vectorComponent = 0;
      vectorSize = inComponents;
    }
    else
    {
      if (vectorComponent < 0)
      {
        vectorComponent = 0;
      }
      if (vectorComponent >= inComponents)
      {
        vectorComponent = inComponents - 1;
      }
      if (vectorComponent + vectorSize > inComponents)
      {
        vectorSize = inComponents - vectorComponent;
      }
    }

    if (vectorMode == vtkScalarsToColors::MAGNITUDE &&
        (inComponents == 1 || vectorSize == 1))
    {
      vectorMode = vtkScalarsToColors::COMPONENT;
    }
  }

  // increment input pointer to the first component to map
  if (vectorComponent > 0)
  {
    int scalarSize = vtkDataArray::GetDataTypeSize(scalarType);
    input = static_cast<unsigned char *>(input) + vectorComponent*scalarSize;
  }

  // map according to the current vector mode
  switch (vectorMode)
  {
    case vtkScalarsToColors::COMPONENT:
    {
      this->MapScalarsThroughTable(
        input, output, scalarType, numValues, inComponents, outputFormat);
    }
      break;

    case vtkScalarsToColors::MAGNITUDE:
    {
      // convert to magnitude in blocks of 300 values
      int inInc = vtkDataArray::GetDataTypeSize(scalarType)*inComponents;
      double magValues[300];
      int blockSize = 300;
      int numBlocks = (numValues + blockSize - 1)/blockSize;
      int lastBlockSize = numValues - blockSize*(numBlocks - 1);

      for (int i = 0; i < numBlocks; i++)
      {
        int numMagValues = ((i < numBlocks-1) ? blockSize : lastBlockSize);
        this->MapVectorsToMagnitude(
          input, magValues, scalarType, numMagValues, inComponents,
          vectorSize);
        this->MapScalarsThroughTable(
          magValues, output, VTK_DOUBLE, numMagValues, 1, outputFormat);
        input = static_cast<char *>(input) + numMagValues*inInc;
        output += numMagValues*outputFormat;
      }
    }
      break;

    case vtkScalarsToColors::RGBCOLORS:
    {
      this->MapColorsToColors(
        input, output, scalarType, numValues, inComponents, vectorSize,
        outputFormat);
    }
      break;
  }
}

//----------------------------------------------------------------------------
// Map a set of scalar values through the table
void vtkScalarsToColors::MapScalarsThroughTable(vtkDataArray *scalars,
                                                unsigned char *output,
                                                int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapScalarsThroughTable: unrecognized color format");
    return;
  }

  this->MapScalarsThroughTable(scalars->GetVoidPointer(0),
                               output,
                               scalars->GetDataType(),
                               scalars->GetNumberOfTuples(),
                               scalars->GetNumberOfComponents(),
                               outputFormat);
}

//----------------------------------------------------------------------------
// Color type converters in anonymous namespace
namespace
{

#define vtkScalarsToColorsLuminance(r, g, b) \
    ((r)*0.30 + (g)*0.59 + (b)*0.11)

void vtkScalarsToColorsLuminanceToLuminance(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
  {
    *outPtr++ = *inPtr;
    inPtr += numComponents;
  }
  while (--count);
}

void vtkScalarsToColorsLuminanceToRGB(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
  {
    unsigned char l = *inPtr;
    outPtr[0] = l;
    outPtr[1] = l;
    outPtr[2] = l;
    inPtr += numComponents;
    outPtr += 3;
  }
  while (--count);
}

void vtkScalarsToColorsRGBToLuminance(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
  {
    unsigned char r = inPtr[0];
    unsigned char g = inPtr[1];
    unsigned char b = inPtr[2];
    *outPtr++ = static_cast<unsigned char>(
                  vtkScalarsToColorsLuminance(r, g, b) + 0.5);
    inPtr += numComponents;
  }
  while (--count);
}

void vtkScalarsToColorsRGBToRGB(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
  {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    inPtr += numComponents;
    outPtr += 3;
  }
  while (--count);
}

void vtkScalarsToColorsLuminanceToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);

  do
  {
    outPtr[0] = inPtr[0];
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}


template<typename T>
void vtkScalarsToColorsLuminanceToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);

  do
  {
    unsigned char l = vtkScalarsToColors::ColorToUChar(inPtr[0]);
    outPtr[0] = l;
    outPtr[1] = l;
    outPtr[2] = l;
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
  }
  while (--count);
}

void vtkScalarsToColorsRGBToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);

  do
  {
    unsigned char r = inPtr[0];
    unsigned char g = inPtr[1];
    unsigned char b = inPtr[2];
    outPtr[0] = static_cast<unsigned char>(
                  vtkScalarsToColorsLuminance(r, g, b) + 0.5);
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}

template<typename T>
void vtkScalarsToColorsRGBToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);

  do
  {
    outPtr[0] = vtkScalarsToColors::ColorToUChar(inPtr[0]);
    outPtr[1] = vtkScalarsToColors::ColorToUChar(inPtr[1]);
    outPtr[2] = vtkScalarsToColors::ColorToUChar(inPtr[2]);
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
  }
  while (--count);
}


void vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  if (alpha >= 1)
  {
    do
    {
      outPtr[0] = inPtr[0];
      outPtr[1] = inPtr[1];
      inPtr += numComponents;
      outPtr += 2;
    }
    while (--count);
  }
  else
  {
    do
    {
      outPtr[0] = inPtr[0];
      outPtr[1] = static_cast<unsigned char>(inPtr[1]*alpha + 0.5);
      inPtr += numComponents;
      outPtr += 2;
    }
    while (--count);
  }
}

template<typename T>
void vtkScalarsToColorsLuminanceAlphaToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  if (alpha >= 1)
  {
    do
    {
      unsigned char l = vtkScalarsToColors::ColorToUChar(inPtr[0]);
      unsigned char a = vtkScalarsToColors::ColorToUChar(inPtr[1]);
      outPtr[0] = l;
      outPtr[1] = l;
      outPtr[2] = l;
      outPtr[3] = a;
      inPtr += numComponents;
      outPtr += 4;
    }
    while (--count);
  }
  else
  {
    do
    {
      unsigned char l = vtkScalarsToColors::ColorToUChar(inPtr[0]);
      unsigned char a = vtkScalarsToColors::ColorToUChar(inPtr[1]);
      outPtr[0] = l;
      outPtr[1] = l;
      outPtr[2] = l;
      outPtr[3] = static_cast<unsigned char>(a*alpha + 0.5);
      inPtr += numComponents;
      outPtr += 4;
    }
    while (--count);
  }
}

void vtkScalarsToColorsRGBAToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  do
  {
    unsigned char r = inPtr[0];
    unsigned char g = inPtr[1];
    unsigned char b = inPtr[2];
    unsigned char a = inPtr[3];
    outPtr[0] = static_cast<unsigned char>(
                  vtkScalarsToColorsLuminance(r, g, b) + 0.5);
    outPtr[1] = static_cast<unsigned char>(a*alpha + 0.5);
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}

template<typename T>
void vtkScalarsToColorsRGBAToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double alpha)
{
  if (alpha >= 1)
  {
    do
    {
      outPtr[0] = vtkScalarsToColors::ColorToUChar(inPtr[0]);
      outPtr[1] = vtkScalarsToColors::ColorToUChar(inPtr[1]);
      outPtr[2] = vtkScalarsToColors::ColorToUChar(inPtr[2]);
      outPtr[3] = vtkScalarsToColors::ColorToUChar(inPtr[3]);
      inPtr += numComponents;
      outPtr += 4;
    }
    while (--count);
  }
  else
  {
    do
    {
      outPtr[0] = vtkScalarsToColors::ColorToUChar(inPtr[0]);
      outPtr[1] = vtkScalarsToColors::ColorToUChar(inPtr[1]);
      outPtr[2] = vtkScalarsToColors::ColorToUChar(inPtr[2]);
      outPtr[3] = static_cast<unsigned char>(inPtr[3]*alpha + 0.5);
      inPtr += numComponents;
      outPtr += 4;
    }
    while (--count);
  }
}

//----------------------------------------------------------------------------

template<class T>
void vtkScalarsToColorsLuminanceToLuminance(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double l = inPtr[0];
    l = (l + shift)*scale;
    l = (l > minval ? l : minval);
    l = (l < maxval ? l : maxval);
    l += 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    inPtr += numComponents;
    outPtr += 1;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsLuminanceToRGB(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double l = inPtr[0];
    l = (l + shift)*scale;
    l = (l > minval ? l : minval);
    l = (l < maxval ? l : maxval);
    unsigned char lc = static_cast<unsigned char>(l + 0.5);
    outPtr[0] = lc;
    outPtr[1] = lc;
    outPtr[2] = lc;
    inPtr += numComponents;
    outPtr += 3;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsRGBToLuminance(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double r = inPtr[0];
    double g = inPtr[1];
    double b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    r = (r > minval ? r : minval);
    r = (r < maxval ? r : maxval);
    g = (g > minval ? g : minval);
    g = (g < maxval ? g : maxval);
    b = (b > minval ? b : minval);
    b = (b < maxval ? b : maxval);
    double l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    inPtr += numComponents;
    outPtr += 1;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsRGBToRGB(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double r = inPtr[0];
    double g = inPtr[1];
    double b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    r = (r > minval ? r : minval);
    r = (r < maxval ? r : maxval);
    g = (g > minval ? g : minval);
    g = (g < maxval ? g : maxval);
    b = (b > minval ? b : minval);
    b = (b < maxval ? b : maxval);
    r += 0.5;
    g += 0.5;
    b += 0.5;
    outPtr[0] = static_cast<unsigned char>(r);
    outPtr[1] = static_cast<unsigned char>(g);
    outPtr[2] = static_cast<unsigned char>(b);
    inPtr += numComponents;
    outPtr += 3;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsLuminanceToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double l = inPtr[0];
    l = (l + shift)*scale;
    l = (l > minval ? l : minval);
    l = (l < maxval ? l : maxval);
    l += 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsLuminanceToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double l = inPtr[0];
    l = (l + shift)*scale;
    l = (l > minval ? l : minval);
    l = (l < maxval ? l : maxval);
    unsigned char lc = static_cast<unsigned char>(l + 0.5);
    outPtr[0] = lc;
    outPtr[1] = lc;
    outPtr[2] = lc;
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsRGBToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double r = inPtr[0];
    double g = inPtr[1];
    double b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    r = (r > minval ? r : minval);
    r = (r < maxval ? r : maxval);
    g = (g > minval ? g : minval);
    g = (g < maxval ? g : maxval);
    b = (b > minval ? b : minval);
    b = (b < maxval ? b : maxval);
    double l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsRGBToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double r = inPtr[0];
    double g = inPtr[1];
    double b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    r = (r > minval ? r : minval);
    r = (r < maxval ? r : maxval);
    g = (g > minval ? g : minval);
    g = (g < maxval ? g : maxval);
    b = (b > minval ? b : minval);
    b = (b < maxval ? b : maxval);
    r += 0.5;
    g += 0.5;
    b += 0.5;
    outPtr[0] = static_cast<unsigned char>(r);
    outPtr[1] = static_cast<unsigned char>(g);
    outPtr[2] = static_cast<unsigned char>(b);
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
  }
  while (--count);
}


template<class T>
void vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double l = inPtr[0];
    double a = inPtr[1];
    l = (l + shift)*scale;
    a = (a + shift)*scale;
    l = (l > minval ? l : minval);
    l = (l < maxval ? l : maxval);
    a = (a > minval ? a : minval);
    a = (a < maxval ? a : maxval);
    l += 0.5;
    a = a*alpha + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsLuminanceAlphaToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double l = inPtr[0];
    double a = inPtr[1];
    l = (l + shift)*scale;
    a = (a + shift)*scale;
    l = (l > minval ? l : minval);
    l = (l < maxval ? l : maxval);
    a = (a > minval ? a : minval);
    a = (a < maxval ? a : maxval);
    unsigned char lc = static_cast<unsigned char>(l + 0.5);
    a = a*alpha + 0.5;
    outPtr[0] = lc;
    outPtr[1] = lc;
    outPtr[2] = lc;
    outPtr[3] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 4;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsRGBAToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double r = inPtr[0];
    double g = inPtr[1];
    double b = inPtr[2];
    double a = inPtr[3];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    a = (a + shift)*scale;
    r = (r > minval ? r : minval);
    r = (r < maxval ? r : maxval);
    g = (g > minval ? g : minval);
    g = (g < maxval ? g : maxval);
    b = (b > minval ? b : minval);
    b = (b < maxval ? b : maxval);
    a = (a > minval ? a : minval);
    a = (a < maxval ? a : maxval);
    a = a*alpha + 0.5;
    double l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 2;
  }
  while (--count);
}

template<class T>
void vtkScalarsToColorsRGBAToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, double shift, double scale, double alpha)
{
  static double minval = 0;
  static double maxval = 255.0;

  do
  {
    double r = inPtr[0];
    double g = inPtr[1];
    double b = inPtr[2];
    double a = inPtr[3];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    a = (a + shift)*scale;
    r = (r > minval ? r : minval);
    r = (r < maxval ? r : maxval);
    g = (g > minval ? g : minval);
    g = (g < maxval ? g : maxval);
    b = (b > minval ? b : minval);
    b = (b < maxval ? b : maxval);
    a = (a > minval ? a : minval);
    a = (a < maxval ? a : maxval);
    r += 0.5;
    g += 0.5;
    b += 0.5;
    a = a*alpha + 0.5;
    outPtr[0] = static_cast<unsigned char>(r);
    outPtr[1] = static_cast<unsigned char>(g);
    outPtr[2] = static_cast<unsigned char>(b);
    outPtr[3] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 4;
  }
  while (--count);
}

//----------------------------------------------------------------------------
unsigned char *vtkScalarsToColorsUnpackBits(void *inPtr, vtkIdType numValues)
{
  vtkIdType n = (numValues + 7) % 8;
  unsigned char *newPtr = new unsigned char [n];

  unsigned char *tmpPtr = newPtr;
  unsigned char *bitdata = static_cast<unsigned char *>(inPtr);
  for (vtkIdType i = 0; i < n; i += 8)
  {
    unsigned char b = *bitdata++;
    int j = 8;
    do
    {
      *tmpPtr++ = ((b >> (--j)) & 0x01);
    }
    while (j);
  }

  return newPtr;
}

// end anonymous namespace
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::MapColorsToColors(
  void *inPtr, unsigned char *outPtr, int inputDataType,
  int numberOfTuples, int numberOfComponents, int inputFormat,
  int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapScalarsToColors: unrecognized color format");
    return;
  }

  if (numberOfTuples <= 0)
  {
    return;
  }

  unsigned char *newPtr = 0;
  if (inputDataType == VTK_BIT)
  {
    newPtr = vtkScalarsToColorsUnpackBits(
      inPtr, numberOfTuples*numberOfComponents);
    inPtr = newPtr;
    inputDataType = VTK_UNSIGNED_CHAR;
  }

  if (inputFormat <= 0 || inputFormat > numberOfComponents)
  {
    inputFormat = numberOfComponents;
  }

  double shift, scale;
  vtkScalarsToColorsComputeShiftScale(this, shift, scale);
  scale *= 255.0;

  double alpha = this->Alpha;
  if (alpha < 0) { alpha = 0; }
  if (alpha > 1) { alpha = 1; }

  if (inputDataType == VTK_UNSIGNED_CHAR &&
      static_cast<int>(shift*scale + 0.5) == 0 &&
      static_cast<int>((255 + shift)*scale + 0.5) == 255)
  {
    if (outputFormat == VTK_RGBA)
    {
      if (inputFormat == VTK_LUMINANCE)
      {
        vtkScalarsToColorsLuminanceToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
      else if (inputFormat == VTK_LUMINANCE_ALPHA)
      {
        vtkScalarsToColorsLuminanceAlphaToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
      else if (inputFormat == VTK_RGB)
      {
        vtkScalarsToColorsRGBToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
      else
      {
        vtkScalarsToColorsRGBAToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      if (inputFormat < VTK_RGB)
      {
        vtkScalarsToColorsLuminanceToRGB(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
      }
      else
      {
        vtkScalarsToColorsRGBToRGB(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      if (inputFormat == VTK_LUMINANCE)
      {
        vtkScalarsToColorsLuminanceToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
      else if (inputFormat == VTK_LUMINANCE_ALPHA)
      {
        vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
      else if (inputFormat == VTK_RGB)
      {
        vtkScalarsToColorsRGBToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
      else
      {
        vtkScalarsToColorsRGBAToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      if (inputFormat < VTK_RGB)
      {
        vtkScalarsToColorsLuminanceToLuminance(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
      }
      else
      {
        vtkScalarsToColorsRGBToLuminance(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
      }
    }
  }
  else
  {
    // must apply shift scale and/or do type conversion
    if (outputFormat == VTK_RGBA)
    {
      if (inputFormat == VTK_LUMINANCE)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsLuminanceToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
      else if (inputFormat == VTK_LUMINANCE_ALPHA)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsLuminanceAlphaToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
      else if (inputFormat == VTK_RGB)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsRGBToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
      else
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsRGBAToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      if (inputFormat < VTK_RGB)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsLuminanceToRGB(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
        }
      }
      else
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsRGBToRGB(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      if (inputFormat == VTK_LUMINANCE)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsLuminanceToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
      else if (inputFormat == VTK_LUMINANCE_ALPHA)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
      else if (inputFormat == VTK_RGB)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsRGBToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
      else
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsRGBAToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      if (inputFormat < VTK_RGB)
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsLuminanceToLuminance(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
        }
      }
      else
      {
        switch (inputDataType)
        {
          vtkTemplateAliasMacro(
            vtkScalarsToColorsRGBToLuminance(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
        }
      }
    }
  }

  delete [] newPtr;
}

//----------------------------------------------------------------------------
template<class T>
void vtkScalarsToColorsMapVectorsToMagnitude(
  const T *inPtr, double *outPtr, int numTuples, int vectorSize, int inInc)
{
  do
  {
    int n = vectorSize;
    double v = 0.0;
    do
    {
      double u = static_cast<double>(*inPtr++);
      v += u*u;
    }
    while (--n);
    *outPtr++ = sqrt(v);
    inPtr += inInc;
  }
  while (--numTuples);
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::MapVectorsToMagnitude(
  void *inPtr, double *outPtr, int inputDataType,
  int numberOfTuples, int numberOfComponents, int vectorSize)
{
  if (numberOfTuples <= 0)
  {
    return;
  }

  unsigned char *newPtr = 0;
  if (inputDataType == VTK_BIT)
  {
    newPtr = vtkScalarsToColorsUnpackBits(
      inPtr, numberOfTuples*numberOfComponents);
    inPtr = newPtr;
    inputDataType = VTK_UNSIGNED_CHAR;
  }

  if (vectorSize <= 0 || vectorSize > numberOfComponents)
  {
    vectorSize = numberOfComponents;
  }
  int inInc = numberOfComponents - vectorSize;

  switch (inputDataType)
  {
    vtkTemplateAliasMacro(
      vtkScalarsToColorsMapVectorsToMagnitude(
        static_cast<VTK_TT*>(inPtr), outPtr,
        numberOfTuples, vectorSize, inInc));
  }

  delete [] newPtr;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::MapScalarsThroughTable2(
  void *inPtr, unsigned char *outPtr, int inputDataType,
  int numberOfTuples, int numberOfComponents, int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapScalarsThroughTable2: unrecognized color format");
    return;
  }

  if (numberOfTuples <= 0)
  {
    return;
  }

  unsigned char *newPtr = 0;
  if (inputDataType == VTK_BIT)
  {
    newPtr = vtkScalarsToColorsUnpackBits(
      inPtr, numberOfTuples*numberOfComponents);
    inPtr = newPtr;
    inputDataType = VTK_UNSIGNED_CHAR;
  }

  double shift, scale;
  vtkScalarsToColorsComputeShiftScale(this, shift, scale);
  scale *= 255.0;

  double alpha = this->Alpha;
  if (alpha < 0) { alpha = 0; }
  if (alpha > 1) { alpha = 1; }

  if (inputDataType == VTK_UNSIGNED_CHAR &&
      static_cast<int>(shift*scale + 0.5) == 0 &&
      static_cast<int>((255 + shift)*scale + 0.5) == 255)
  {
    if (outputFormat == VTK_RGBA)
    {
      vtkScalarsToColorsLuminanceToRGBA(
        static_cast<unsigned char*>(inPtr), outPtr,
        numberOfTuples, numberOfComponents, alpha);
    }
    else if (outputFormat == VTK_RGB)
    {
      vtkScalarsToColorsLuminanceToRGB(
        static_cast<unsigned char*>(inPtr), outPtr,
        numberOfTuples, numberOfComponents);
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      vtkScalarsToColorsLuminanceToLuminanceAlpha(
        static_cast<unsigned char*>(inPtr), outPtr,
        numberOfTuples, numberOfComponents, alpha);
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      vtkScalarsToColorsLuminanceToLuminance(
        static_cast<unsigned char*>(inPtr), outPtr,
        numberOfTuples, numberOfComponents);
    }
  }
  else
  {
    // must apply shift scale and/or do type conversion
    if (outputFormat == VTK_RGBA)
    {
      switch (inputDataType)
      {
        vtkTemplateAliasMacro(
          vtkScalarsToColorsLuminanceToRGBA(
            static_cast<VTK_TT*>(inPtr), outPtr,
            numberOfTuples, numberOfComponents, shift, scale, alpha));

        default:
          vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input data type");
          break;
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      switch (inputDataType)
      {
        vtkTemplateAliasMacro(
          vtkScalarsToColorsLuminanceToRGB(
            static_cast<VTK_TT*>(inPtr), outPtr,
            numberOfTuples, numberOfComponents, shift, scale));

        default:
          vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input data type");
          break;
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      switch (inputDataType)
      {
        vtkTemplateAliasMacro(
          vtkScalarsToColorsLuminanceToLuminanceAlpha(
            static_cast<VTK_TT*>(inPtr), outPtr,
            numberOfTuples, numberOfComponents, shift, scale, alpha));

        default:
          vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input data type");
          break;
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      switch (inputDataType)
      {
        vtkTemplateAliasMacro(
          vtkScalarsToColorsLuminanceToLuminance(
            static_cast<VTK_TT*>(inPtr), outPtr,
            numberOfTuples, numberOfComponents, shift, scale));

        default:
          vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input data type");
          break;
      }
    }
  }

  delete [] newPtr;
}

// The callForAnyType is used to write generic code that works with any
// vtkDataArray derived types.
//
// This  macro calls a template function (on the data type stored in the
// array).  Example usage:
//   callForAnyType(array, myFunc(static_cast<VTK_TT*>(data), arg2));
// where 'array' is a vtkDataArray and
//       'data' could be: array->GetVoidPointer(0)
#define callForAnyType(array, call)                      \
  switch(array->GetDataType())                           \
  {                                                    \
    vtkTemplateMacro(call);                              \
  }

//----------------------------------------------------------------------------
vtkUnsignedCharArray *vtkScalarsToColors::ConvertToRGBA(
  vtkDataArray *colors, int numComp, int numTuples)
{
  if (vtkArrayDownCast<vtkCharArray>(colors) != NULL)
  {
    vtkErrorMacro(<<"char type does not have enough values to hold a color");
    return NULL;
  }

  if (numComp == 4 && this->Alpha >= 1.0 &&
      vtkArrayDownCast<vtkUnsignedCharArray>(colors) != NULL)
  {
    vtkUnsignedCharArray* c = vtkArrayDownCast<vtkUnsignedCharArray>(colors);
    c->Register(this);
    return c;
  }

  vtkUnsignedCharArray *newColors = vtkUnsignedCharArray::New();
  newColors->SetNumberOfComponents(4);
  newColors->SetNumberOfTuples(numTuples);
  unsigned char *nptr = newColors->GetPointer(0);
  double alpha = this->Alpha;
  alpha = (alpha > 0 ? alpha : 0);
  alpha = (alpha < 1 ? alpha : 1);

  if (numTuples <= 0)
  {
    return newColors;
  }

  switch (numComp)
  {
    case 1:
      callForAnyType(
        colors, vtkScalarsToColorsLuminanceToRGBA(
          static_cast<VTK_TT*>(colors->GetVoidPointer(0)),
          nptr, numTuples, numComp, alpha));
      break;

    case 2:
      callForAnyType(
        colors, vtkScalarsToColorsLuminanceAlphaToRGBA(
          static_cast<VTK_TT*>(colors->GetVoidPointer(0)),
          nptr, numTuples, numComp, alpha));
      break;

    case 3:
      callForAnyType(
        colors, vtkScalarsToColorsRGBToRGBA(
          static_cast<VTK_TT*>(colors->GetVoidPointer(0)),
          nptr, numTuples, numComp, alpha));
      break;

    case 4:
      callForAnyType(
        colors, vtkScalarsToColorsRGBAToRGBA(
          static_cast<VTK_TT*>(colors->GetVoidPointer(0)),
          nptr, numTuples, numComp, alpha));
      break;

    default:
      vtkErrorMacro(<<"Cannot convert colors");
      return NULL;
  }

  return newColors;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  if (this->VectorMode == vtkScalarsToColors::MAGNITUDE)
  {
    os << indent << "VectorMode: Magnitude\n";
  }
  else if (this->VectorMode == vtkScalarsToColors::RGBCOLORS)
  {
    os << indent << "VectorMode: RGBColors\n";
  }
  else
  {
    os << indent << "VectorMode: Component\n";
  }
  os << indent << "VectorComponent: " << this->VectorComponent << "\n";
  os << indent << "VectorSize: " << this->VectorSize << "\n";
  os << indent << "IndexedLookup: "
    << (this->IndexedLookup ? "ON" : "OFF") << "\n";
  vtkIdType nv = this->GetNumberOfAnnotatedValues();
  os << indent << "AnnotatedValues: "
    << nv << (nv > 0 ? " entries:\n" : " entries.\n");
  vtkIndent i2(indent.GetNextIndent());
  for (vtkIdType i = 0; i < nv; ++ i)
  {
    os
      << i2 << i << ": value: " << this->GetAnnotatedValue(i).ToString()
      << " note: \"" << this->GetAnnotation(i) << "\"\n";
  }
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetAnnotations(
  vtkAbstractArray* values, vtkStringArray* annotations)
{
  if (
    (values && !annotations) ||
    (!values && annotations))
    return;

  if (values && annotations &&
    values->GetNumberOfTuples() != annotations->GetNumberOfTuples())
  {
    vtkErrorMacro(
      << "Values and annotations do not have the same number of tuples ("
      << values->GetNumberOfTuples() << " and "
      << annotations->GetNumberOfTuples() << ", respectively. Ignoring.");
    return;
  }

  if (this->AnnotatedValues && !values)
  {
    this->AnnotatedValues->Delete();
    this->AnnotatedValues = 0;
  }
  else if (values)
  { // Ensure arrays are of the same type before copying.
    if (this->AnnotatedValues)
    {
      if (this->AnnotatedValues->GetDataType() != values->GetDataType())
      {
        this->AnnotatedValues->Delete();
        this->AnnotatedValues = 0;
      }
    }
    if (!this->AnnotatedValues)
    {
      this->AnnotatedValues =
        vtkAbstractArray::CreateArray(
          values->GetDataType());
    }
  }
  bool sameVals = (values == this->AnnotatedValues);
  if (!sameVals && values)
  {
    this->AnnotatedValues->DeepCopy(values);
  }

  if (this->Annotations && !annotations)
  {
    this->Annotations->Delete();
    this->Annotations = 0;
  }
  else if (!this->Annotations && annotations)
  {
    this->Annotations = vtkStringArray::New();
  }
  bool sameText = (annotations == this->Annotations);
  if (!sameText)
  {
    this->Annotations->DeepCopy(annotations);
  }
  this->UpdateAnnotatedValueMap();
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::SetAnnotation(
  vtkVariant value, vtkStdString annotation)
{
  vtkIdType i = this->CheckForAnnotatedValue(value);
  bool modified = false;
  if (i >= 0)
  {
    if (this->Annotations->GetValue(i) != annotation)
    {
      this->Annotations->SetValue(i, annotation);
      modified = true;
    }
  }
  else
  {
    i = this->Annotations->InsertNextValue(annotation);
    this->AnnotatedValues->InsertVariantValue(i, value);
    modified = true;
  }
  if (modified)
  {
    this->UpdateAnnotatedValueMap();
    this->Modified();
  }
  return i;
}

//----------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::SetAnnotation(
  vtkStdString value, vtkStdString annotation)
{
  bool valid;
  vtkVariant val(value);
  double x;
  x = val.ToDouble(&valid);
  if (valid)
  {
    return this->SetAnnotation(x, annotation);
  }
  return this->SetAnnotation(val, annotation);
}

//----------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::GetNumberOfAnnotatedValues()
{
  return
    this->AnnotatedValues ? this->AnnotatedValues->GetNumberOfTuples() : 0;
}

//----------------------------------------------------------------------------
vtkVariant vtkScalarsToColors::GetAnnotatedValue(vtkIdType idx)
{
  if (!this->AnnotatedValues ||
    idx < 0 || idx >= this->AnnotatedValues->GetNumberOfTuples())
  {
    vtkVariant invalid;
    return invalid;
  }
  return this->AnnotatedValues->GetVariantValue(idx);
}

//----------------------------------------------------------------------------
vtkStdString vtkScalarsToColors::GetAnnotation(vtkIdType idx)
{
  if (!this->Annotations)
    /* Don't check idx as Annotations->GetValue() does:
     * || idx < 0 || idx >= this->Annotations->GetNumberOfTuples())
     */
  {
    vtkStdString empty;
    return empty;
  }
  return this->Annotations->GetValue(idx);
}

//----------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::GetAnnotatedValueIndex(vtkVariant val)
{
  return (this->AnnotatedValues ? this->CheckForAnnotatedValue(val) : -1);
}

//----------------------------------------------------------------------------
bool vtkScalarsToColors::RemoveAnnotation(vtkVariant value)
{
  vtkIdType i = this->CheckForAnnotatedValue(value);
  bool needToRemove = (i >= 0);
  if (needToRemove)
  {
    // Note that this is the number of values minus 1:
    vtkIdType na = this->AnnotatedValues->GetMaxId();
    for (; i < na; ++ i)
    {
      this->AnnotatedValues->SetVariantValue(i,
        this->AnnotatedValues->GetVariantValue(i + 1));
      this->Annotations->SetValue(i, this->Annotations->GetValue(i + 1));
    }
    this->AnnotatedValues->Resize(na);
    this->Annotations->Resize(na);
    this->UpdateAnnotatedValueMap();
    this->Modified();
  }
  return needToRemove;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::ResetAnnotations()
{
  if (!this->Annotations)
  {
    vtkVariantArray* va = vtkVariantArray::New();
    vtkStringArray* sa = vtkStringArray::New();
    this->SetAnnotations(va, sa);
    va->Delete();
    sa->Delete();
  }
  this->AnnotatedValues->Reset();
  this->Annotations->Reset();
  this->AnnotatedValueMap->clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::GetAnnotationColor(const vtkVariant& val, double rgba[4])
{
  if (this->IndexedLookup)
  {
    vtkIdType i = this->GetAnnotatedValueIndex(val);
    this->GetIndexedColor(i, rgba);
  }
  else
  {
    this->GetColor(val.ToDouble(), rgba);
    rgba[3] = 1.;
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::CheckForAnnotatedValue(vtkVariant value)
{
  if (!this->Annotations)
  {
    vtkVariantArray* va = vtkVariantArray::New();
    vtkStringArray* sa = vtkStringArray::New();
    this->SetAnnotations(va, sa);
    va->FastDelete();
    sa->FastDelete();
  }
  return this->GetAnnotatedValueIndexInternal(value);
}

//----------------------------------------------------------------------------
// An unsafe version of vtkScalarsToColors::CheckForAnnotatedValue for
// internal use (no pointer checks performed)
vtkIdType vtkScalarsToColors::GetAnnotatedValueIndexInternal(vtkVariant& value)
{
  vtkInternalAnnotatedValueMap::iterator it =
    this->AnnotatedValueMap->find(value);
  vtkIdType nv = this->GetNumberOfAvailableColors();
  vtkIdType i = (it == this->AnnotatedValueMap->end() ?
    -1 : (nv ? it->second % nv : it->second));
  return i;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::GetIndexedColor(vtkIdType, double rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::UpdateAnnotatedValueMap()
{
  this->AnnotatedValueMap->clear();

  vtkIdType na =
    this->AnnotatedValues ? this->AnnotatedValues->GetMaxId() + 1 : 0;
  for (vtkIdType i = 0; i < na; ++ i)
  {
    (*this->AnnotatedValueMap)[this->AnnotatedValues->GetVariantValue(i)] = i;
  }
}
