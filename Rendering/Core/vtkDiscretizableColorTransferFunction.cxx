/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscretizableColorTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDiscretizableColorTransferFunction.h"

#include "vtkCommand.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkTuple.h"

#include <vector>

class vtkDiscretizableColorTransferFunction::vtkInternals
{
public:
  std::vector<vtkTuple<double, 4> > IndexedColors;
};

vtkStandardNewMacro(vtkDiscretizableColorTransferFunction);
vtkCxxSetObjectMacro(
  vtkDiscretizableColorTransferFunction, ScalarOpacityFunction, vtkPiecewiseFunction);
//-----------------------------------------------------------------------------
vtkDiscretizableColorTransferFunction::vtkDiscretizableColorTransferFunction()
  : Internals(new vtkInternals())
{
  this->LookupTable = vtkLookupTable::New();

  this->Discretize = 0;
  this->NumberOfValues = 256;

  this->UseLogScale = 0;

  this->ScalarOpacityFunction = nullptr;
  this->EnableOpacityMapping = false;
}

//-----------------------------------------------------------------------------
vtkDiscretizableColorTransferFunction::~vtkDiscretizableColorTransferFunction()
{
  // this removes any observer we may have setup for the
  // ScalarOpacityFunction.
  this->SetScalarOpacityFunction(nullptr);
  this->LookupTable->Delete();

  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkDiscretizableColorTransferFunction::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  if (this->ScalarOpacityFunction)
  {
    vtkMTimeType somtime = this->ScalarOpacityFunction->GetMTime();
    mtime = somtime > mtime ? somtime : mtime;
  }
  if (this->LookupTable)
  {
    vtkMTimeType ltmtime = this->LookupTable->GetMTime();
    mtime = ltmtime > mtime ? ltmtime : mtime;
  }

  return mtime;
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetNumberOfIndexedColors(unsigned int count)
{
  if (static_cast<unsigned int>(this->Internals->IndexedColors.size()) != count)
  {
    this->Internals->IndexedColors.resize(count, vtkTuple<double, 4>(0.0));
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
unsigned int vtkDiscretizableColorTransferFunction::GetNumberOfIndexedColors()
{
  return static_cast<unsigned int>(this->Internals->IndexedColors.size());
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetIndexedColor(
  unsigned int index, double r, double g, double b, double a)
{
  if (static_cast<unsigned int>(this->Internals->IndexedColors.size()) <= index)
  {
    // resize and fill all new colors with the same color as specified.
    size_t old_size = this->Internals->IndexedColors.size();
    size_t new_size = static_cast<size_t>(index + 1);
    this->Internals->IndexedColors.resize(new_size);

    for (size_t cc = old_size; cc < new_size; cc++)
    {
      double* data = this->Internals->IndexedColors[cc].GetData();
      data[0] = r;
      data[1] = g;
      data[2] = b;
      data[3] = a;
    }

    this->Modified();
  }
  else if (this->Internals->IndexedColors[index].GetData()[0] != r ||
    this->Internals->IndexedColors[index].GetData()[1] != g ||
    this->Internals->IndexedColors[index].GetData()[2] != b ||
    this->Internals->IndexedColors[index].GetData()[3] != a)
  {
    // color has changed, change it.
    double* data = this->Internals->IndexedColors[index].GetData();
    data[0] = r;
    data[1] = g;
    data[2] = b;
    data[3] = a;

    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::GetIndexedColor(vtkIdType i, double rgba[4])
{
  if (this->IndexedLookup || this->Discretize)
  {
    this->LookupTable->GetIndexedColor(i, rgba);
  }
  else
  {
    this->Superclass::GetIndexedColor(i, rgba);
  }
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetUseLogScale(int useLogScale)
{
  if (this->UseLogScale != useLogScale)
  {
    this->UseLogScale = useLogScale;
    if (this->UseLogScale)
    {
      this->LookupTable->SetScaleToLog10();
      this->SetScaleToLog10();
    }
    else
    {
      this->LookupTable->SetScaleToLinear();
      this->SetScaleToLinear();
    }

    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkDiscretizableColorTransferFunction::IsOpaque()
{
  return !this->EnableOpacityMapping;
}

int vtkDiscretizableColorTransferFunction::IsOpaque(
  vtkAbstractArray* scalars, int colorMode, int component)
{
  // use superclass logic?
  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(scalars);
  if ((colorMode == VTK_COLOR_MODE_DEFAULT &&
        vtkArrayDownCast<vtkUnsignedCharArray>(dataArray) != nullptr) ||
    (colorMode == VTK_COLOR_MODE_DIRECT_SCALARS && dataArray))
  {
    return this->Superclass::IsOpaque(scalars, colorMode, component);
  }
  // otherwise look at our basic approach
  return this->IsOpaque();
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::Build()
{
  this->Superclass::Build();

  if (this->LookupTableUpdateTime > this->GetMTime())
  {
    // no need to rebuild anything.
    return;
  }

  this->LookupTable->SetVectorMode(this->VectorMode);
  this->LookupTable->SetVectorComponent(this->VectorComponent);
  this->LookupTable->SetIndexedLookup(this->IndexedLookup);
  this->LookupTable->SetUseBelowRangeColor(this->UseBelowRangeColor);
  this->LookupTable->SetUseAboveRangeColor(this->UseAboveRangeColor);

  double rgba[4];
  this->GetBelowRangeColor(rgba);
  rgba[3] = 1.0;
  this->LookupTable->SetBelowRangeColor(rgba);

  this->GetAboveRangeColor(rgba);
  rgba[3] = 1.0;
  this->LookupTable->SetAboveRangeColor(rgba);

  // this is essential since other the LookupTable doesn't update the
  // annotations map. That's a bug in the implementation of
  // vtkScalarsToColors::SetAnnotations(..,..);
  this->LookupTable->SetAnnotations(nullptr, nullptr);
  this->LookupTable->SetAnnotations(this->AnnotatedValues, this->Annotations);

  if (this->IndexedLookup)
  {
    if (this->GetNumberOfIndexedColors() > 0)
    {
      // Use the specified indexed-colors.
      vtkIdType count = this->GetNumberOfAnnotatedValues();
      this->LookupTable->SetNumberOfTableValues(count);
      for (size_t cc = 0;
           cc < this->Internals->IndexedColors.size() && cc < static_cast<size_t>(count); cc++)
      {
        rgba[0] = this->Internals->IndexedColors[cc].GetData()[0];
        rgba[1] = this->Internals->IndexedColors[cc].GetData()[1];
        rgba[2] = this->Internals->IndexedColors[cc].GetData()[2];
        rgba[3] = this->Internals->IndexedColors[cc].GetData()[3];
        this->LookupTable->SetTableValue(static_cast<int>(cc), rgba);
      }
    }
    else
    {
      // old logic for backwards compatibility.
      int nv = this->GetSize();
      this->LookupTable->SetNumberOfTableValues(nv);
      double nodeVal[6];
      for (int i = 0; i < nv; ++i)
      {
        this->GetNodeValue(i, nodeVal);
        nodeVal[4] = 1.;
        this->LookupTable->SetTableValue(i, &nodeVal[1]);
      }
    }
  }
  else if (this->Discretize)
  {
    // Do not omit the LookupTable->SetNumberOfTableValues call:
    // WritePointer does not update the NumberOfColors ivar.
    this->LookupTable->SetNumberOfTableValues(this->NumberOfValues);
    unsigned char* lut_ptr = this->LookupTable->WritePointer(0, this->NumberOfValues);
    double* table = new double[this->NumberOfValues * 3];
    double range[2];
    this->GetRange(range);
    bool logRangeValid = true;
    if (this->UseLogScale)
    {
      logRangeValid = range[0] > 0.0 || range[1] < 0.0;
      if (!logRangeValid && this->LookupTable->GetScale() == VTK_SCALE_LOG10)
      {
        this->LookupTable->SetScaleToLinear();
      }
    }

    this->LookupTable->SetRange(range);
    if (this->UseLogScale && logRangeValid && this->LookupTable->GetScale() == VTK_SCALE_LINEAR)
    {
      this->LookupTable->SetScaleToLog10();
    }

    this->GetTable(range[0], range[1], this->NumberOfValues, table);
    // Now, convert double to unsigned chars and fill the LUT.
    for (int cc = 0; cc < this->NumberOfValues; cc++)
    {
      lut_ptr[4 * cc] = (unsigned char)(255.0 * table[3 * cc] + 0.5);
      lut_ptr[4 * cc + 1] = (unsigned char)(255.0 * table[3 * cc + 1] + 0.5);
      lut_ptr[4 * cc + 2] = (unsigned char)(255.0 * table[3 * cc + 2] + 0.5);
      lut_ptr[4 * cc + 3] = 255;
    }
    delete[] table;
  }

  this->LookupTable->BuildSpecialColors();

  this->LookupTableUpdateTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetAlpha(double alpha)
{
  this->LookupTable->SetAlpha(alpha);
  this->Superclass::SetAlpha(alpha);
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetNanColor(double r, double g, double b)
{
  this->LookupTable->SetNanColor(r, g, b, this->GetNanOpacity());
  this->Superclass::SetNanColor(r, g, b);
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::SetNanOpacity(double a)
{
  double color[3];
  this->GetNanColor(color);
  this->LookupTable->SetNanColor(color[0], color[1], color[2], a);
  this->Superclass::SetNanOpacity(a);
}

//-----------------------------------------------------------------------------
const unsigned char* vtkDiscretizableColorTransferFunction::MapValue(double v)
{
  this->Build();
  if (this->Discretize || this->IndexedLookup)
  {
    return this->LookupTable->MapValue(v);
  }

  return this->Superclass::MapValue(v);
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::GetColor(double v, double rgb[3])
{
  this->Build();
  if (this->Discretize || this->IndexedLookup)
  {
    this->LookupTable->GetColor(v, rgb);
  }
  else
  {
    this->Superclass::GetColor(v, rgb);
  }
}

//-----------------------------------------------------------------------------
double vtkDiscretizableColorTransferFunction::GetOpacity(double v)
{
  if (this->IndexedLookup || !this->EnableOpacityMapping || !this->ScalarOpacityFunction)
  {
    return this->Superclass::GetOpacity(v);
  }
  return this->ScalarOpacityFunction->GetValue(v);
}

//----------------------------------------------------------------------------
// Internal mapping of the opacity value through the lookup table
template <class T>
static void vtkDiscretizableColorTransferFunctionMapOpacity(
  vtkDiscretizableColorTransferFunction* self, T* input, unsigned char* output, int length,
  int inIncr, int outFormat)
{
  double x;
  int i = length;
  unsigned char* optr = output;
  T* iptr = input;

  if (self->GetScalarOpacityFunction()->GetSize() == 0)
  {
    vtkGenericWarningMacro("Transfer Function Has No Points!");
    return;
  }

  if (outFormat != VTK_RGBA && outFormat != VTK_LUMINANCE_ALPHA)
  {
    return;
  }

  // opacity component stride
  unsigned int stride = (outFormat == VTK_RGBA ? 4 : 2);

  optr += stride - 1; // Move to first alpha value
  // Iterate through color components
  while (--i >= 0)
  {
    x = static_cast<double>(*iptr);
    double alpha = self->GetScalarOpacityFunction()->GetValue(x);
    *(optr) = static_cast<unsigned char>(alpha * 255.0 + 0.5);
    optr += stride;
    iptr += inIncr;
  }
}

//----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::MapScalarsThroughTable2(void* input,
  unsigned char* output, int inputDataType, int numberOfValues, int inputIncrement,
  int outputFormat)
{
  // Calculate RGB values
  if (this->Discretize || this->IndexedLookup)
  {
    this->LookupTable->MapScalarsThroughTable2(
      input, output, inputDataType, numberOfValues, inputIncrement, outputFormat);
  }
  else
  {
    this->Superclass::MapScalarsThroughTable2(
      input, output, inputDataType, numberOfValues, inputIncrement, outputFormat);
  }

  // Calculate alpha values
  if (this->IndexedLookup == false && // don't change alpha for IndexedLookup.
    this->EnableOpacityMapping == true && this->ScalarOpacityFunction.GetPointer() != nullptr)
  {
    switch (inputDataType)
    {
      vtkTemplateMacro(vtkDiscretizableColorTransferFunctionMapOpacity(
        this, static_cast<VTK_TT*>(input), output, numberOfValues, inputIncrement, outputFormat));
      default:
        vtkErrorMacro(<< "MapImageThroughTable: Unknown input ScalarType");
        return;
    }
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkDiscretizableColorTransferFunction::GetNumberOfAvailableColors()
{
  if (this->Discretize == false)
  {
    return 16777216; // 2^24
  }
  return this->NumberOfValues;
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkDiscretizableColorTransferFunction::GetScalarOpacityFunction() const
{
  return this->ScalarOpacityFunction;
}

//-----------------------------------------------------------------------------
void vtkDiscretizableColorTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Discretize: " << this->Discretize << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;
  os << indent << "UseLogScale: " << this->UseLogScale << endl;
  os << indent << "EnableOpacityMapping: " << this->EnableOpacityMapping << endl;
  os << indent << "ScalarOpacityFunction: " << this->ScalarOpacityFunction << endl;
}
