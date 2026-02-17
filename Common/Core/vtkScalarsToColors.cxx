// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_7_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkScalarsToColors.h"

#include "vtkAbstractArray.h"
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <algorithm>
#include <cmath>
#include <list>

// A helper list lookups of annotated values.
// Note you cannot use a map or sort etc as the
// comparison operator for vtkVariant is not suitable
// for strict sorting.
VTK_ABI_NAMESPACE_BEGIN
class vtkScalarsToColors::vtkInternalAnnotatedValueList : public std::list<vtkVariant>
{
};

vtkStandardNewMacro(vtkScalarsToColors);

//------------------------------------------------------------------------------
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
  this->AnnotatedValues = nullptr;
  this->Annotations = nullptr;
  this->AnnotatedValueList = new vtkInternalAnnotatedValueList;
  this->IndexedLookup = 0;
}

//------------------------------------------------------------------------------
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
  delete this->AnnotatedValueList;
}

//------------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1. Default implementation return true.
vtkTypeBool vtkScalarsToColors::IsOpaque()
{
  return 1;
}

//------------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1.
vtkTypeBool vtkScalarsToColors::IsOpaque(
  vtkAbstractArray* scalars, int colorMode, int vectorComponent)
{
  return this->IsOpaque(scalars, colorMode, vectorComponent, nullptr);
}

//------------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1.
vtkTypeBool vtkScalarsToColors::IsOpaque(vtkAbstractArray* scalars, int colorMode, int,
  vtkUnsignedCharArray* ghosts, unsigned char ghostsToSkip)
{
  if (!scalars)
  {
    return this->IsOpaque();
  }

  int numberOfComponents = scalars->GetNumberOfComponents();

  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(scalars);

  // map scalars through lookup table only if needed
  if ((colorMode == VTK_COLOR_MODE_DEFAULT &&
        vtkArrayDownCast<vtkUnsignedCharArray>(dataArray) != nullptr) ||
    (colorMode == VTK_COLOR_MODE_DIRECT_SCALARS && dataArray))
  {
    // we will be using the scalars directly, so look at the number of
    // components and the range
    if (numberOfComponents == 3 || numberOfComponents == 1)
    {
      return (this->Alpha >= 1.0 ? 1 : 0);
    }
    // otherwise look at the range of the alpha channel
    unsigned char opacity = 0;
    double range[2];
    dataArray->GetRange(
      range, numberOfComponents - 1, ghosts ? ghosts->GetPointer(0) : nullptr, ghostsToSkip);
    switch (scalars->GetDataType())
    {
      vtkTemplateMacro(vtkScalarsToColors::ColorToUChar(static_cast<VTK_TT>(range[0]), &opacity));
    }
    return ((opacity == 255) ? 1 : 0);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToComponent()
{
  this->SetVectorMode(vtkScalarsToColors::COMPONENT);
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToMagnitude()
{
  this->SetVectorMode(vtkScalarsToColors::MAGNITUDE);
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToRGBColors()
{
  this->SetVectorMode(vtkScalarsToColors::RGBCOLORS);
}

//------------------------------------------------------------------------------
// do not use SetMacro() because we do not want the table to rebuild.
void vtkScalarsToColors::SetAlpha(double alpha)
{
  this->Alpha = std::clamp(alpha, 0.0, 1.0);
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::SetRange(double minval, double maxval)
{
  if (this->InputRange[0] != minval || this->InputRange[1] != maxval)
  {
    this->InputRange[0] = minval;
    this->InputRange[1] = maxval;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
double* vtkScalarsToColors::GetRange()
{
  return this->InputRange;
}

//------------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::GetNumberOfAvailableColors()
{
  // return total possible RGB colors
  return 256 * 256 * 256;
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::DeepCopy(vtkScalarsToColors* obj)
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
      vtkAbstractArray* annValues =
        vtkAbstractArray::CreateArray(obj->AnnotatedValues->GetDataType());
      vtkStringArray* annotations = vtkStringArray::New();
      annValues->DeepCopy(obj->AnnotatedValues);
      annotations->DeepCopy(obj->Annotations);
      this->SetAnnotations(annValues, annotations);
      annValues->Delete();
      annotations->Delete();
    }
    else
    {
      this->SetAnnotations(nullptr, nullptr);
    }
  }
}

//------------------------------------------------------------------------------
inline void vtkScalarsToColorsComputeShiftScale(
  vtkScalarsToColors* self, double& shift, double& scale)
{
  constexpr double minscale = -1e17;
  constexpr double maxscale = 1e17;

  const double* range = self->GetRange();
  shift = -range[0];
  scale = range[1] - range[0];
  if (scale * scale > 1e-30)
  {
    scale = 1.0 / scale;
  }
  else
  {
    scale = (scale < 0.0 ? minscale : maxscale);
  }
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::GetColor(double v, double rgb[3])
{
  constexpr double minval = 0.0;
  constexpr double maxval = 1.0;

  double shift, scale;
  vtkScalarsToColorsComputeShiftScale(this, shift, scale);

  double val = (v + shift) * scale;
  val = (val > minval ? val : minval);
  val = (val < maxval ? val : maxval);

  rgb[0] = val;
  rgb[1] = val;
  rgb[2] = val;
}

//------------------------------------------------------------------------------
double vtkScalarsToColors::GetOpacity(double vtkNotUsed(v))
{
  return 1.0;
}

//------------------------------------------------------------------------------
const unsigned char* vtkScalarsToColors::MapValue(double v)
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

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkScalarsToColors::MapScalars(
  vtkDataArray* scalars, int colorMode, int vectorComponent, int outputFormat)
{
  return this->MapScalars(
    static_cast<vtkAbstractArray*>(scalars), colorMode, vectorComponent, outputFormat);
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkScalarsToColors::MapScalars(
  vtkAbstractArray* scalars, int colorMode, int vectorComponent, int outputFormat)
{
  int numberOfComponents = scalars->GetNumberOfComponents();
  vtkUnsignedCharArray* newColors;

  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(scalars);

  // map scalars through lookup table only if needed
  if ((colorMode == VTK_COLOR_MODE_DEFAULT &&
        vtkArrayDownCast<vtkUnsignedCharArray>(dataArray) != nullptr) ||
    (colorMode == VTK_COLOR_MODE_DIRECT_SCALARS && dataArray))
  {
    newColors = this->ConvertToRGBA(
      dataArray, scalars->GetNumberOfComponents(), dataArray->GetNumberOfTuples());
  }
  else
  {
    newColors = vtkUnsignedCharArray::New();
    newColors->SetNumberOfComponents(outputFormat);
    newColors->SetNumberOfTuples(scalars->GetNumberOfTuples());

    // If mapper did not specify a component, use the VectorMode
    if (vectorComponent < 0 && numberOfComponents > 1)
    {
      this->MapVectorsThroughTable(dataArray, newColors->GetPointer(0),
        scalars->GetNumberOfTuples(), scalars->GetNumberOfComponents(), outputFormat);
    }
    else
    {
      vectorComponent = std::clamp(vectorComponent, 0, numberOfComponents - 1);

      // Map the scalars to colors
      this->MapScalarsThroughTable(scalars, newColors->GetPointer(0), scalars->GetNumberOfTuples(),
        scalars->GetNumberOfComponents(), vectorComponent, outputFormat);
    }
  }

  return newColors;
}

//------------------------------------------------------------------------------
// Map a set of vector values through the table
void vtkScalarsToColors::MapVectorsThroughTable(vtkDataArray* input, unsigned char* output,
  int numberOfTuples, int numberOfComponents, int vectorComponent, int vectorSize, int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapVectorsThroughTable: unrecognized color format");
    return;
  }
  if (numberOfTuples <= 0)
  {
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
    vectorComponent = std::clamp(vectorComponent, 0, numberOfComponents - 1);
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
      vectorSize = numberOfComponents;
    }
    else
    {
      vectorComponent = std::clamp(vectorComponent, 0, numberOfComponents - 1);
      if (vectorComponent + vectorSize > numberOfComponents)
      {
        vectorSize = numberOfComponents - vectorComponent;
      }
    }

    if (vectorMode == vtkScalarsToColors::MAGNITUDE && (numberOfComponents == 1 || vectorSize == 1))
    {
      vectorMode = vtkScalarsToColors::COMPONENT;
    }
  }

  // map according to the current vector mode
  switch (vectorMode)
  {
    case vtkScalarsToColors::COMPONENT:
    {
      this->MapScalarsThroughTable(
        input, output, numberOfTuples, numberOfComponents, vectorComponent, outputFormat);
    }
    break;

    case vtkScalarsToColors::MAGNITUDE:
    {
      vtkNew<vtkDoubleArray> magArray;
      magArray->SetNumberOfComponents(1);
      magArray->SetNumberOfTuples(numberOfTuples);
      this->MapVectorsToMagnitude(input, magArray->GetPointer(0), numberOfTuples,
        numberOfComponents, vectorComponent, vectorSize);
      this->MapScalarsThroughTable(magArray, output, numberOfTuples, 1, 0, outputFormat);
    }
    break;

    case vtkScalarsToColors::RGBCOLORS:
    {
      this->MapColorsToColors(input, output, numberOfTuples, numberOfComponents, vectorComponent,
        vectorSize, outputFormat);
    }
    break;
  }
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapVectorsThroughTable(VTK_FUTURE_CONST void* inPtr, unsigned char* outPtr,
  int scalarType, int numberOfTuples, int numberOfComponents, int outputFormat, int vectorComponent,
  int vectorSize)
{
  auto input = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(scalarType));
  input->SetNumberOfComponents(numberOfComponents);
  input->SetVoidArray(inPtr, numberOfTuples * numberOfComponents, 1);
  this->MapVectorsThroughTable(
    input, outPtr, numberOfTuples, numberOfComponents, vectorComponent, vectorSize, outputFormat);
}

//------------------------------------------------------------------------------
// Map a set of scalar values through the table
void vtkScalarsToColors::MapScalarsThroughTable(
  vtkDataArray* scalars, unsigned char* output, int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapScalarsThroughTable: unrecognized color format");
    return;
  }

  this->MapScalarsThroughTable(scalars, output, scalars->GetNumberOfTuples(),
    scalars->GetNumberOfComponents(), 0, outputFormat);
}
VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
// Color type converters in anonymous namespace
namespace
{

#define vtkScalarsToColorsLuminance(r, g, b) ((r)*0.30 + (g)*0.59 + (b)*0.11)

//------------------------------------------------------------------------------
struct vtkScalarsToColorsLuminanceToLuminance
{
  template <typename TArray>
  void operator()(
    TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents, int vectorComponent)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    do
    {
      *outPtr++ = *inPtr;
      inPtr += numComponents;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double l = inPtr[0];
      l = (l + shift) * scale;
      l = (l > minval ? l : minval);
      l = (l < maxval ? l : maxval);
      l += 0.5;
      outPtr[0] = static_cast<unsigned char>(l);
      inPtr += numComponents;
      outPtr += 1;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsLuminanceToRGB
{
  template <typename TArray>
  void operator()(
    TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents, int vectorComponent)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    do
    {
      unsigned char l = *inPtr;
      outPtr[0] = l;
      outPtr[1] = l;
      outPtr[2] = l;
      inPtr += numComponents;
      outPtr += 3;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double l = inPtr[0];
      l = (l + shift) * scale;
      l = (l > minval ? l : minval);
      l = (l < maxval ? l : maxval);
      unsigned char lc = static_cast<unsigned char>(l + 0.5);
      outPtr[0] = lc;
      outPtr[1] = lc;
      outPtr[2] = lc;
      inPtr += numComponents;
      outPtr += 3;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsRGBToLuminance
{
  template <typename TArray>
  void operator()(
    TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents, int vectorComponent)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    do
    {
      unsigned char r = inPtr[0];
      unsigned char g = inPtr[1];
      unsigned char b = inPtr[2];
      *outPtr++ = static_cast<unsigned char>(vtkScalarsToColorsLuminance(r, g, b) + 0.5);
      inPtr += numComponents;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double r = inPtr[0];
      double g = inPtr[1];
      double b = inPtr[2];
      r = (r + shift) * scale;
      g = (g + shift) * scale;
      b = (b + shift) * scale;
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
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsRGBToRGB
{
  template <typename TArray>
  void operator()(
    TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents, int vectorComponent)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    do
    {
      outPtr[0] = inPtr[0];
      outPtr[1] = inPtr[1];
      outPtr[2] = inPtr[2];
      inPtr += numComponents;
      outPtr += 3;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double r = inPtr[0];
      double g = inPtr[1];
      double b = inPtr[2];
      r = (r + shift) * scale;
      g = (g + shift) * scale;
      b = (b + shift) * scale;
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
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsLuminanceToLuminanceAlpha
{
  template <typename TArray>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    do
    {
      outPtr[0] = inPtr[0];
      outPtr[1] = a;
      inPtr += numComponents;
      outPtr += 2;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double l = inPtr[0];
      l = (l + shift) * scale;
      l = (l > minval ? l : minval);
      l = (l < maxval ? l : maxval);
      l += 0.5;
      outPtr[0] = static_cast<unsigned char>(l);
      outPtr[1] = a;
      inPtr += numComponents;
      outPtr += 2;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsLuminanceToRGBA
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
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
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double l = inPtr[0];
      l = (l + shift) * scale;
      l = (l > minval ? l : minval);
      l = (l < maxval ? l : maxval);
      unsigned char lc = static_cast<unsigned char>(l + 0.5);
      outPtr[0] = lc;
      outPtr[1] = lc;
      outPtr[2] = lc;
      outPtr[3] = a;
      inPtr += numComponents;
      outPtr += 4;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsRGBToLuminanceAlpha
{
  template <typename TArray>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    do
    {
      unsigned char r = inPtr[0];
      unsigned char g = inPtr[1];
      unsigned char b = inPtr[2];
      outPtr[0] = static_cast<unsigned char>(vtkScalarsToColorsLuminance(r, g, b) + 0.5);
      outPtr[1] = a;
      inPtr += numComponents;
      outPtr += 2;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double r = inPtr[0];
      double g = inPtr[1];
      double b = inPtr[2];
      r = (r + shift) * scale;
      g = (g + shift) * scale;
      b = (b + shift) * scale;
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
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsRGBToRGBA
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    do
    {
      outPtr[0] = vtkScalarsToColors::ColorToUChar(inPtr[0]);
      outPtr[1] = vtkScalarsToColors::ColorToUChar(inPtr[1]);
      outPtr[2] = vtkScalarsToColors::ColorToUChar(inPtr[2]);
      outPtr[3] = a;
      inPtr += numComponents;
      outPtr += 4;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    unsigned char a = vtkScalarsToColors::ColorToUChar(alpha);
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double r = inPtr[0];
      double g = inPtr[1];
      double b = inPtr[2];
      r = (r + shift) * scale;
      g = (g + shift) * scale;
      b = (b + shift) * scale;
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
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha
{
  template <typename TArray>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    if (alpha >= 1)
    {
      do
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = inPtr[1];
        inPtr += numComponents;
        outPtr += 2;
      } while (--count);
    }
    else
    {
      do
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = static_cast<unsigned char>(inPtr[1] * alpha + 0.5);
        inPtr += numComponents;
        outPtr += 2;
      } while (--count);
    }
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double l = inPtr[0];
      double a = inPtr[1];
      l = (l + shift) * scale;
      a = (a + shift) * scale;
      l = (l > minval ? l : minval);
      l = (l < maxval ? l : maxval);
      a = (a > minval ? a : minval);
      a = (a < maxval ? a : maxval);
      l += 0.5;
      a = a * alpha + 0.5;
      outPtr[0] = static_cast<unsigned char>(l);
      outPtr[1] = static_cast<unsigned char>(a);
      inPtr += numComponents;
      outPtr += 2;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsLuminanceAlphaToRGBA
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
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
      } while (--count);
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
        outPtr[3] = static_cast<unsigned char>(a * alpha + 0.5);
        inPtr += numComponents;
        outPtr += 4;
      } while (--count);
    }
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double l = inPtr[0];
      double a = inPtr[1];
      l = (l + shift) * scale;
      a = (a + shift) * scale;
      l = (l > minval ? l : minval);
      l = (l < maxval ? l : maxval);
      a = (a > minval ? a : minval);
      a = (a < maxval ? a : maxval);
      unsigned char lc = static_cast<unsigned char>(l + 0.5);
      a = a * alpha + 0.5;
      outPtr[0] = lc;
      outPtr[1] = lc;
      outPtr[2] = lc;
      outPtr[3] = static_cast<unsigned char>(a);
      inPtr += numComponents;
      outPtr += 4;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsRGBAToLuminanceAlpha
{
  template <typename TArray>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, unsigned char>(input, vectorComponent)
        .begin();
    do
    {
      unsigned char r = inPtr[0];
      unsigned char g = inPtr[1];
      unsigned char b = inPtr[2];
      unsigned char a = inPtr[3];
      outPtr[0] = static_cast<unsigned char>(vtkScalarsToColorsLuminance(r, g, b) + 0.5);
      outPtr[1] = static_cast<unsigned char>(a * alpha + 0.5);
      inPtr += numComponents;
      outPtr += 2;
    } while (--count);
  }

  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double r = inPtr[0];
      double g = inPtr[1];
      double b = inPtr[2];
      double a = inPtr[3];
      r = (r + shift) * scale;
      g = (g + shift) * scale;
      b = (b + shift) * scale;
      a = (a + shift) * scale;
      r = (r > minval ? r : minval);
      r = (r < maxval ? r : maxval);
      g = (g > minval ? g : minval);
      g = (g < maxval ? g : maxval);
      b = (b > minval ? b : minval);
      b = (b < maxval ? b : maxval);
      a = (a > minval ? a : minval);
      a = (a < maxval ? a : maxval);
      a = a * alpha + 0.5;
      double l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
      outPtr[0] = static_cast<unsigned char>(l);
      outPtr[1] = static_cast<unsigned char>(a);
      inPtr += numComponents;
      outPtr += 2;
    } while (--count);
  }
};

//------------------------------------------------------------------------------
struct vtkScalarsToColorsRGBAToRGBA
{
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
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
      } while (--count);
    }
    else
    {
      do
      {
        outPtr[0] = vtkScalarsToColors::ColorToUChar(inPtr[0]);
        outPtr[1] = vtkScalarsToColors::ColorToUChar(inPtr[1]);
        outPtr[2] = vtkScalarsToColors::ColorToUChar(inPtr[2]);
        outPtr[3] = static_cast<unsigned char>(inPtr[3] * alpha + 0.5);
        inPtr += numComponents;
        outPtr += 4;
      } while (--count);
    }
  }

  //------------------------------------------------------------------------------
  template <typename TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(TArray* input, unsigned char* outPtr, vtkIdType count, int numComponents,
    int vectorComponent, double shift, double scale, double alpha)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input, vectorComponent).begin();
    constexpr double minval = 0.0;
    constexpr double maxval = 255.0;

    do
    {
      double r = inPtr[0];
      double g = inPtr[1];
      double b = inPtr[2];
      double a = inPtr[3];
      r = (r + shift) * scale;
      g = (g + shift) * scale;
      b = (b + shift) * scale;
      a = (a + shift) * scale;
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
      a = a * alpha + 0.5;
      outPtr[0] = static_cast<unsigned char>(r);
      outPtr[1] = static_cast<unsigned char>(g);
      outPtr[2] = static_cast<unsigned char>(b);
      outPtr[3] = static_cast<unsigned char>(a);
      inPtr += numComponents;
      outPtr += 4;
    } while (--count);
  }
};

// end anonymous namespace
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Unpack an array of bits into an array of `unsigned char`.
vtkSmartPointer<vtkUnsignedCharArray> vtkScalarsToColors::UnpackBits(
  vtkBitArray* input, int numComp, vtkIdType numTuples)
{
  const unsigned char* inPtr = input->GetPointer(0);
  auto output = vtkSmartPointer<vtkUnsignedCharArray>::New();
  output->SetNumberOfComponents(numComp);
  output->SetNumberOfTuples(numTuples);
  unsigned char* outPtr = output->GetPointer(0);
  const vtkIdType numValues = numTuples * numComp;
  for (vtkIdType i = 0; i < numValues; i += 8)
  {
    unsigned char b = *inPtr++;
    vtkIdType j = std::min(static_cast<vtkIdType>(8), numValues - i);
    do
    {
      *outPtr++ = ((b >> (--j)) & 0x01);
    } while (j);
  }

  return output;
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapColorsToColors(vtkDataArray* input, unsigned char* outPtr,
  int numberOfTuples, int numberOfComponents, int vectorComponent, int vectorSize, int outputFormat)
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

  vtkSmartPointer<vtkDataArray> realInput = input;
  if (input->GetDataType() == VTK_BIT)
  {
    auto bitArray = vtkBitArray::SafeDownCast(input);
    realInput = vtkScalarsToColors::UnpackBits(bitArray, numberOfComponents, numberOfTuples);
  }

  if (vectorSize <= 0 || vectorSize > numberOfComponents)
  {
    vectorSize = numberOfComponents;
  }

  double shift, scale;
  vtkScalarsToColorsComputeShiftScale(this, shift, scale);
  scale *= 255.0;

  double alpha = this->Alpha;
  alpha = std::clamp(alpha, 0.0, 1.0);

  if (input->GetDataType() == VTK_UNSIGNED_CHAR && static_cast<int>(shift * scale + 0.5) == 0 &&
    static_cast<int>((255 + shift) * scale + 0.5) == 255)
  {
    using Dispatcher = vtkArrayDispatch::DispatchByArrayAndValueType<vtkArrayDispatch::AllArrays,
      vtkTypeList::Create<unsigned char>>;
    if (outputFormat == VTK_RGBA)
    {
      if (vectorSize == VTK_LUMINANCE)
      {
        vtkScalarsToColorsLuminanceToRGBA worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
      else if (vectorSize == VTK_LUMINANCE_ALPHA)
      {
        vtkScalarsToColorsLuminanceAlphaToRGBA worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
      else if (vectorSize == VTK_RGB)
      {
        vtkScalarsToColorsRGBToRGBA worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
      else
      {
        vtkScalarsToColorsRGBAToRGBA worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      if (vectorSize < VTK_RGB)
      {
        vtkScalarsToColorsLuminanceToRGB worker;
        if (!Dispatcher::Execute(
              realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent))
        {
          worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent);
        }
      }
      else
      {
        vtkScalarsToColorsRGBToRGB worker;
        if (!Dispatcher::Execute(
              realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent))
        {
          worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent);
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      if (vectorSize == VTK_LUMINANCE)
      {
        vtkScalarsToColorsLuminanceToLuminanceAlpha worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
      else if (vectorSize == VTK_LUMINANCE_ALPHA)
      {
        vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
      else if (vectorSize == VTK_RGB)
      {
        vtkScalarsToColorsRGBToLuminanceAlpha worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
      else
      {
        vtkScalarsToColorsRGBAToLuminanceAlpha worker;
        if (!Dispatcher::Execute(realInput, worker, outPtr, numberOfTuples, numberOfComponents,
              vectorComponent, alpha))
        {
          worker(
            realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      if (vectorSize < VTK_RGB)
      {
        vtkScalarsToColorsLuminanceToLuminance worker;
        if (!Dispatcher::Execute(
              realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent))
        {
          worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent);
        }
      }
      else
      {
        vtkScalarsToColorsRGBToLuminance worker;
        if (!Dispatcher::Execute(
              realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent))
        {
          worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent);
        }
      }
    }
  }
  else
  {
    // must apply shift scale and/or do type conversion
    if (outputFormat == VTK_RGBA)
    {
      if (vectorSize == VTK_LUMINANCE)
      {
        vtkScalarsToColorsLuminanceToRGBA worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
      else if (vectorSize == VTK_LUMINANCE_ALPHA)
      {
        vtkScalarsToColorsLuminanceAlphaToRGBA worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
      else if (vectorSize == VTK_RGB)
      {
        vtkScalarsToColorsRGBToRGBA worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
      else
      {
        vtkScalarsToColorsRGBAToRGBA worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      if (vectorSize < VTK_RGB)
      {
        vtkScalarsToColorsLuminanceToRGB worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale)));
          }
        }
      }
      else
      {
        vtkScalarsToColorsRGBToRGB worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale)));
          }
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      if (vectorSize == VTK_LUMINANCE)
      {
        vtkScalarsToColorsLuminanceToLuminanceAlpha worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
      else if (vectorSize == VTK_LUMINANCE_ALPHA)
      {
        vtkScalarsToColorsLuminanceAlphaToLuminanceAlpha worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
      else if (vectorSize == VTK_RGB)
      {
        vtkScalarsToColorsRGBToLuminanceAlpha worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
      else
      {
        vtkScalarsToColorsRGBAToLuminanceAlpha worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale, alpha))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
          }
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      if (vectorSize < VTK_RGB)
      {
        vtkScalarsToColorsLuminanceToLuminance worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale)));
          }
        }
      }
      else
      {
        vtkScalarsToColorsRGBToLuminance worker;
        if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
              numberOfComponents, vectorComponent, shift, scale))
        {
          switch (realInput->GetDataType())
          {
            vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
              outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale)));
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapColorsToColors(VTK_FUTURE_CONST void* inPtr, unsigned char* outPtr,
  int inputDataType, int numberOfTuples, int numberOfComponents, int vectorSize, int outputFormat)
{
  auto input = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(inputDataType));
  input->SetNumberOfComponents(numberOfComponents);
  input->SetVoidArray(inPtr, numberOfTuples * numberOfComponents, 1);
  this->MapColorsToColors(
    input, outPtr, numberOfTuples, numberOfComponents, 0, vectorSize, outputFormat);
}

//------------------------------------------------------------------------------
struct vtkScalarsToColorsMapVectorsToMagnitude
{
  template <class TArray, typename T = vtk::GetAPIType<TArray>>
  void operator()(
    TArray* input, double* outPtr, int numTuples, int vectorComponent, int vectorSize, int inInc)
  {
    auto inPtr =
      vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(input).begin() + vectorComponent;
    do
    {
      int n = vectorSize;
      double v = 0.0;
      do
      {
        double u = static_cast<double>(*inPtr++);
        v += u * u;
      } while (--n);
      *outPtr++ = sqrt(v);
      inPtr += inInc;
    } while (--numTuples);
  }
};

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapVectorsToMagnitude(vtkDataArray* input, double* output,
  int numberOfTuples, int numberOfComponents, int vectorComponent, int vectorSize)
{
  if (numberOfTuples <= 0)
  {
    return;
  }
  vtkSmartPointer<vtkDataArray> realInput = input;
  if (input->GetDataType() == VTK_BIT)
  {
    auto bitArray = vtkBitArray::SafeDownCast(input);
    realInput = vtkScalarsToColors::UnpackBits(bitArray, numberOfComponents, numberOfTuples);
  }
  if (vectorSize <= 0 || vectorSize > numberOfComponents)
  {
    vectorSize = numberOfComponents;
  }
  int inInc = numberOfComponents - vectorSize;
  vtkScalarsToColorsMapVectorsToMagnitude worker;
  if (!vtkArrayDispatch::Dispatch::Execute(
        input, worker, output, numberOfTuples, vectorComponent, vectorSize, inInc))
  {
    switch (input->GetDataType())
    {
      vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(
        input, output, numberOfTuples, vectorComponent, vectorSize, inInc)));
    }
  }
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapVectorsToMagnitude(VTK_FUTURE_CONST void* inPtr, double* outPtr,
  int inputDataType, int numberOfTuples, int numberOfComponents, int vectorSize)
{
  auto input = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(inputDataType));
  input->SetNumberOfComponents(numberOfComponents);
  input->SetVoidArray(inPtr, numberOfTuples * numberOfComponents, 1);
  this->MapVectorsToMagnitude(input, outPtr, numberOfTuples, numberOfComponents, 0, vectorSize);
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapScalarsThroughTable(vtkAbstractArray* input, unsigned char* outPtr,
  int numberOfTuples, int numberOfComponents, int vectorComponent, int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
  {
    vtkErrorMacro(<< "MapScalarsThroughTable: unrecognized color format");
    return;
  }

  if (numberOfTuples <= 0)
  {
    return;
  }
  auto inputDA = vtkDataArray::SafeDownCast(input);
  if (!inputDA)
  {
    vtkErrorMacro(<< "MapScalarsThroughTable: Unknown input ScalarType "
                  << input->GetDataTypeAsString());
    return;
    ;
  }

  vtkSmartPointer<vtkDataArray> realInput = inputDA;
  if (inputDA->GetDataType() == VTK_BIT)
  {
    auto bitArray = vtkBitArray::SafeDownCast(inputDA);
    realInput = vtkScalarsToColors::UnpackBits(bitArray, numberOfComponents, numberOfTuples);
  }

  double shift, scale;
  vtkScalarsToColorsComputeShiftScale(this, shift, scale);
  scale *= 255.0;

  double alpha = this->Alpha;
  alpha = std::clamp(alpha, 0.0, 1.0);

  if (input->GetDataType() == VTK_UNSIGNED_CHAR && static_cast<int>(shift * scale + 0.5) == 0 &&
    static_cast<int>((255 + shift) * scale + 0.5) == 255)
  {
    using Dispatcher = vtkArrayDispatch::DispatchByArrayAndValueType<vtkArrayDispatch::AllArrays,
      vtkTypeList::Create<unsigned char>>;
    if (outputFormat == VTK_RGBA)
    {
      vtkScalarsToColorsLuminanceToRGBA worker;
      if (!Dispatcher::Execute(
            realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha))
      {
        worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      vtkScalarsToColorsLuminanceToRGB worker;
      if (!Dispatcher::Execute(
            realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent))
      {
        worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent);
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      vtkScalarsToColorsLuminanceToLuminanceAlpha worker;
      if (!Dispatcher::Execute(
            realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha))
      {
        worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent, alpha);
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      vtkScalarsToColorsLuminanceToLuminance worker;
      if (!Dispatcher::Execute(
            realInput, worker, outPtr, numberOfTuples, numberOfComponents, vectorComponent))
      {
        worker(realInput.Get(), outPtr, numberOfTuples, numberOfComponents, vectorComponent);
      }
    }
  }
  else
  {
    // must apply shift scale and/or do type conversion
    if (outputFormat == VTK_RGBA)
    {
      vtkScalarsToColorsLuminanceToRGBA worker;
      if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
            numberOfComponents, vectorComponent, shift, scale, alpha))
      {
        switch (realInput->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
            outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
        }
      }
    }
    else if (outputFormat == VTK_RGB)
    {
      vtkScalarsToColorsLuminanceToRGB worker;
      if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
            numberOfComponents, vectorComponent, shift, scale))
      {
        switch (realInput->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
            outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale)));
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
    {
      vtkScalarsToColorsLuminanceToLuminanceAlpha worker;
      if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
            numberOfComponents, vectorComponent, shift, scale, alpha))
      {
        switch (realInput->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
            outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale, alpha)));
        }
      }
    }
    else if (outputFormat == VTK_LUMINANCE)
    {
      vtkScalarsToColorsLuminanceToLuminance worker;
      if (!vtkArrayDispatch::Dispatch::Execute(realInput, worker, outPtr, numberOfTuples,
            numberOfComponents, vectorComponent, shift, scale))
      {
        switch (realInput->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(realInput.Get(),
            outPtr, numberOfTuples, numberOfComponents, vectorComponent, shift, scale)));
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapScalarsThroughTable(VTK_FUTURE_CONST void* inPtr, unsigned char* outPtr,
  int inputDataType, int numberOfTuples, int numberOfComponents, int outputFormat)
{
  auto input = vtk::TakeSmartPointer(vtkAbstractArray::CreateArray(inputDataType));
  input->SetNumberOfComponents(numberOfComponents);
  input->SetVoidArray(inPtr, numberOfTuples * numberOfComponents, 1);
  this->MapScalarsThroughTable(input, outPtr, numberOfTuples, numberOfComponents, 0, outputFormat);
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::MapScalarsThroughTable2(VTK_FUTURE_CONST void* inPtr,
  unsigned char* outPtr, int inputDataType, int numberOfTuples, int numberOfComponents,
  int outputFormat)
{
  this->MapScalarsThroughTable(
    inPtr, outPtr, inputDataType, numberOfTuples, numberOfComponents, outputFormat);
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkScalarsToColors::ConvertToRGBA(
  vtkDataArray* colors, int numComp, int numTuples)
{
  if (vtkArrayDownCast<vtkCharArray>(colors) != nullptr)
  {
    vtkErrorMacro(<< "char type does not have enough values to hold a color");
    return nullptr;
  }

  if (numComp == 4 && this->Alpha >= 1.0 &&
    vtkArrayDownCast<vtkUnsignedCharArray>(colors) != nullptr)
  {
    vtkUnsignedCharArray* c = vtkArrayDownCast<vtkUnsignedCharArray>(colors);
    c->Register(this);
    return c;
  }

  vtkUnsignedCharArray* newColors = vtkUnsignedCharArray::New();
  newColors->SetNumberOfComponents(4);
  newColors->SetNumberOfTuples(numTuples);
  unsigned char* nptr = newColors->GetPointer(0);
  double alpha = this->Alpha;
  alpha = (alpha > 0.0 ? alpha : 0.0);
  alpha = (alpha < 1.0 ? alpha : 1.0);

  if (numTuples <= 0)
  {
    return newColors;
  }

  switch (numComp)
  {
    case 1:
    {
      vtkScalarsToColorsLuminanceToRGBA worker;
      if (!vtkArrayDispatch::Dispatch::Execute(colors, worker, nptr, numTuples, numComp, 0, alpha))
      {
        switch (colors->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(
            colors, nptr, numTuples, numComp, 0, alpha)));
        }
      }
      break;
    }
    case 2:
    {
      vtkScalarsToColorsLuminanceAlphaToRGBA worker;
      if (!vtkArrayDispatch::Dispatch::Execute(colors, worker, nptr, numTuples, numComp, 0, alpha))
      {
        switch (colors->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(
            colors, nptr, numTuples, numComp, 0, alpha)));
        }
      }
      break;
    }
    case 3:
    {
      vtkScalarsToColorsRGBToRGBA worker;
      if (!vtkArrayDispatch::Dispatch::Execute(colors, worker, nptr, numTuples, numComp, 0, alpha))
      {
        switch (colors->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(
            colors, nptr, numTuples, numComp, 0, alpha)));
        }
      }
      break;
    }
    case 4:
    {
      vtkScalarsToColorsRGBAToRGBA worker;
      if (!vtkArrayDispatch::Dispatch::Execute(colors, worker, nptr, numTuples, numComp, 0, alpha))
      {
        switch (colors->GetDataType())
        {
          vtkTemplateMacro((worker.template operator()<vtkDataArray, VTK_TT>(
            colors, nptr, numTuples, numComp, 0, alpha)));
        }
      }
      break;
    }

    default:
      vtkErrorMacro(<< "Cannot convert colors");
      return nullptr;
  }

  return newColors;
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

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
  os << indent << "IndexedLookup: " << (this->IndexedLookup ? "ON" : "OFF") << "\n";
  vtkIdType nv = this->GetNumberOfAnnotatedValues();
  os << indent << "AnnotatedValues: " << nv << (nv > 0 ? " entries:\n" : " entries.\n");
  vtkIndent i2(indent.GetNextIndent());
  for (vtkIdType i = 0; i < nv; ++i)
  {
    os << i2 << i << ": value: " << this->GetAnnotatedValue(i).ToString() << " note: \""
       << this->GetAnnotation(i) << "\"\n";
  }
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::SetAnnotations(vtkAbstractArray* values, vtkStringArray* annotations)
{
  if ((values && !annotations) || (!values && annotations))
    return;

  if (values && annotations && values->GetNumberOfTuples() != annotations->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Values and annotations do not have the same number of tuples ("
                  << values->GetNumberOfTuples() << " and " << annotations->GetNumberOfTuples()
                  << ", respectively. Ignoring.");
    return;
  }

  if (this->AnnotatedValues && !values)
  {
    this->AnnotatedValues->Delete();
    this->AnnotatedValues = nullptr;
  }
  else if (values)
  { // Ensure arrays are of the same type before copying.
    if (this->AnnotatedValues)
    {
      if (this->AnnotatedValues->GetDataType() != values->GetDataType())
      {
        this->AnnotatedValues->Delete();
        this->AnnotatedValues = nullptr;
      }
    }
    if (!this->AnnotatedValues)
    {
      this->AnnotatedValues = vtkAbstractArray::CreateArray(values->GetDataType());
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
    this->Annotations = nullptr;
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

//------------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::SetAnnotation(vtkVariant value, vtkStdString annotation)
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

//------------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::SetAnnotation(vtkStdString value, vtkStdString annotation)
{
  bool valid;
  vtkVariant val(value);
  double x = val.ToDouble(&valid);
  if (valid)
  {
    return this->SetAnnotation(x, annotation);
  }
  return this->SetAnnotation(val, annotation);
}

//------------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::GetNumberOfAnnotatedValues()
{
  return this->AnnotatedValues ? this->AnnotatedValues->GetNumberOfTuples() : 0;
}

//------------------------------------------------------------------------------
vtkVariant vtkScalarsToColors::GetAnnotatedValue(vtkIdType idx)
{
  if (!this->AnnotatedValues || idx < 0 || idx >= this->AnnotatedValues->GetNumberOfTuples())
  {
    vtkVariant invalid;
    return invalid;
  }
  return this->AnnotatedValues->GetVariantValue(idx);
}

//------------------------------------------------------------------------------
vtkStdString vtkScalarsToColors::GetAnnotation(vtkIdType idx)
{
  if (!this->AnnotatedValues || idx < 0 || idx >= this->AnnotatedValues->GetNumberOfTuples())
  {
    return {};
  }
  return this->Annotations->GetValue(idx);
}

//------------------------------------------------------------------------------
vtkIdType vtkScalarsToColors::GetAnnotatedValueIndex(vtkVariant val)
{
  return (this->AnnotatedValues ? this->CheckForAnnotatedValue(val) : -1);
}

//------------------------------------------------------------------------------
bool vtkScalarsToColors::RemoveAnnotation(vtkVariant value)
{
  vtkIdType i = this->CheckForAnnotatedValue(value);
  bool needToRemove = (i >= 0);
  if (needToRemove)
  {
    // Note that this is the number of values minus 1:
    vtkIdType na = this->AnnotatedValues->GetMaxId();
    for (; i < na; ++i)
    {
      this->AnnotatedValues->SetVariantValue(i, this->AnnotatedValues->GetVariantValue(i + 1));
      this->Annotations->SetValue(i, this->Annotations->GetValue(i + 1));
    }
    this->AnnotatedValues->Resize(na);
    this->Annotations->Resize(na);
    this->UpdateAnnotatedValueMap();
    this->Modified();
  }
  return needToRemove;
}

//------------------------------------------------------------------------------
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
  this->AnnotatedValueList->clear();
  this->Modified();
}

//------------------------------------------------------------------------------
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
    rgba[3] = 1.0;
  }
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// An unsafe version of vtkScalarsToColors::CheckForAnnotatedValue for
// internal use (no pointer checks performed)
vtkIdType vtkScalarsToColors::GetAnnotatedValueIndexInternal(const vtkVariant& value)
{
  auto it = this->AnnotatedValueList->begin();
  size_t idx = 0;
  for (; idx < this->AnnotatedValueList->size(); ++idx, it++)
  {
    if (*it == value)
    {
      break;
    }
  }
  vtkIdType nv = this->GetNumberOfAvailableColors();
  vtkIdType result = static_cast<vtkIdType>(idx);

  // if not found return -1
  if (it == this->AnnotatedValueList->end())
  {
    result = -1;
  }
  else if (nv > 0)
  {
    result = result % nv;
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::GetIndexedColor(vtkIdType, double rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.0;
}

//------------------------------------------------------------------------------
void vtkScalarsToColors::UpdateAnnotatedValueMap()
{
  this->AnnotatedValueList->clear();

  vtkIdType na = this->AnnotatedValues ? this->AnnotatedValues->GetMaxId() + 1 : 0;
  for (vtkIdType i = 0; i < na; ++i)
  {
    this->AnnotatedValueList->push_back(this->AnnotatedValues->GetVariantValue(i));
  }
}
VTK_ABI_NAMESPACE_END
