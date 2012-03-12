/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractImageInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractImageInterpolator.h"
#include "vtkImageInterpolatorInternals.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkDataArray.h"
#include "vtkTypeTraits.h"


//----------------------------------------------------------------------------
// default do-nothing interpolation functions
namespace {

template<class F>
struct vtkInterpolateNOP
{
  static void InterpolationFunc(
    vtkInterpolationInfo *info, const F point[3], F *outPtr);

  static void RowInterpolationFunc(
    vtkInterpolationWeights *weights, int idX, int idY, int idZ,
    F *outPtr, int n);
};

template<class F>
void vtkInterpolateNOP<F>::InterpolationFunc(
  vtkInterpolationInfo *, const F [3], F *)
{
}

template<class F>
void vtkInterpolateNOP<F>::RowInterpolationFunc(
    vtkInterpolationWeights *, int, int, int, F *, int)
{
}

} // end anonymous namespace

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator::vtkAbstractImageInterpolator()
{
  this->Scalars = NULL;
  this->BorderMode = VTK_IMAGE_BORDER_CLAMP;

  for (int i = 0; i < 6; i++)
    {
    this->StructuredBoundsDouble[i] = 0.0;
    this->StructuredBoundsFloat[i] = 0.0f;
    }

  for (int j = 0; j < 3; j++)
    {
    this->Extent[2*j] = 0;
    this->Extent[2*j+1] = -1;
    this->WholeExtent[2*j] = 0;
    this->WholeExtent[2*j+1] = -1;
    this->Spacing[j] = 1.0;
    this->Origin[j] = 0.0;
    }

  this->OutValue = 0.0;
  this->Tolerance = 7.62939453125e-06;
  this->ComponentOffset = 0;
  this->ComponentCount = -1;

  this->InterpolationInfo = new vtkInterpolationInfo();
  this->InterpolationInfo->Pointer = NULL;
  this->InterpolationInfo->NumberOfComponents = 1;

  this->InterpolationFuncDouble =
    &(vtkInterpolateNOP<double>::InterpolationFunc);
  this->InterpolationFuncFloat =
    &(vtkInterpolateNOP<float>::InterpolationFunc);
  this->RowInterpolationFuncDouble =
    &(vtkInterpolateNOP<double>::RowInterpolationFunc);
  this->RowInterpolationFuncFloat =
    &(vtkInterpolateNOP<float>::RowInterpolationFunc);
}

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator::~vtkAbstractImageInterpolator()
{
  if (this->Scalars)
    {
    this->Scalars->Delete();
    }
  if (this->InterpolationInfo)
    {
    delete this->InterpolationInfo;
    }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::DeepCopy(vtkAbstractImageInterpolator *obj)
{
  this->SetTolerance(obj->Tolerance);
  this->SetOutValue(obj->OutValue);
  this->SetComponentOffset(obj->ComponentOffset);
  this->SetComponentCount(obj->ComponentCount);
  this->SetBorderMode(obj->BorderMode);
  obj->GetExtent(this->Extent);
  obj->GetWholeExtent(this->WholeExtent);
  obj->GetOrigin(this->Origin);
  obj->GetSpacing(this->Spacing);
  if (this->Scalars)
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
  if (obj->Scalars)
    {
    this->Scalars = obj->Scalars;
    this->Scalars->Register(this);
    }
  *this->InterpolationInfo = *obj->InterpolationInfo;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "ComponentOffset: " << this->ComponentOffset << "\n";
  os << indent << "ComponentCount: " << this->ComponentCount << "\n";
  os << indent << "BorderMode: " << this->GetBorderModeAsString() << "\n";
  os << indent << "Extent: " << this->Extent[0] << " " << this->Extent[1]
     << " " << this->Extent[2] << " " << this->Extent[3]
     << " " << this->Extent[4] << " " << this->Extent[5] << "\n";
  os << indent << "WholeExtent: "
     << this->WholeExtent[0] << " " << this->WholeExtent[1] << " "
     << this->WholeExtent[2] << " " << this->WholeExtent[3] << " "
     << this->WholeExtent[4] << " " << this->WholeExtent[5] << "\n";
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1]
     << " " << this->Origin[2] << "\n";
  os << indent << "Spacing: " << this->Spacing[0] << " " << this->Spacing[1]
     << " " << this->Spacing[2] << "\n";
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetBorderMode(int mode)
{
  static int minmode = VTK_IMAGE_BORDER_CLAMP;
  static int maxmode = VTK_IMAGE_BORDER_MIRROR;
  mode = ((mode > minmode) ? mode : minmode);
  mode = ((mode < maxmode) ? mode : maxmode);
  if (this->BorderMode != mode)
    {
    this->BorderMode = mode;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
const char *vtkAbstractImageInterpolator::GetBorderModeAsString()
{
  switch (this->BorderMode)
    {
    case VTK_IMAGE_BORDER_CLAMP:
      return "Clamp";
    case VTK_IMAGE_BORDER_REPEAT:
      return "Repeat";
    case VTK_IMAGE_BORDER_MIRROR:
      return "Mirror";
    }
  return "";
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetComponentOffset(int offset)
{
  if (this->ComponentOffset != offset)
    {
    this->ComponentOffset = offset;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetComponentCount(int count)
{
  if (this->ComponentCount != count)
    {
    this->ComponentCount = count;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkAbstractImageInterpolator::ComputeNumberOfComponents(int inputCount)
{
  // validate the component range to extract
  int component = this->ComponentOffset;
  int count = this->ComponentCount;

  component = ((component > 0) ? component : 0);
  component = ((component < inputCount) ? component : inputCount-1);
  count = ((count < (inputCount-component)) ? count : (inputCount-component));
  count = ((count > 0) ? count : (inputCount-component));

  return count;
}

//----------------------------------------------------------------------------
int vtkAbstractImageInterpolator::GetNumberOfComponents()
{
  return this->InterpolationInfo->NumberOfComponents;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetOutValue(double value)
{
  if (this->OutValue != value)
    {
    this->OutValue = value;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::SetTolerance(double value)
{
  if (this->Tolerance != value)
    {
    this->Tolerance = value;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::Initialize(vtkDataObject *o)
{
  // free any previous scalars
  this->ReleaseData();

  // check for valid data
  vtkImageData *data = vtkImageData::SafeDownCast(o);
  vtkDataArray *scalars = NULL;
  if (data)
    {
    scalars = data->GetPointData()->GetScalars();
    }

  if (data == NULL || scalars == NULL)
    {
    vtkErrorMacro("Initialize(): no image data to interpolate!");
    return;
    }

  // claim the scalars
  scalars->Register(this);
  this->Scalars = scalars;

  // get the image information
  data->GetSpacing(this->Spacing);
  data->GetOrigin(this->Origin);
  data->GetExtent(this->WholeExtent);
  data->GetExtent(this->Extent);

  // call update
  this->Update();
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::ReleaseData()
{
  if (this->Scalars)
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::Update()
{
  vtkDataArray *scalars = this->Scalars;

  // check for scalars
  if (!scalars)
    {
    this->InterpolationInfo->Pointer = NULL;
    this->InterpolationInfo->NumberOfComponents = 1;

    this->InterpolationFuncDouble =
      &(vtkInterpolateNOP<double>::InterpolationFunc);
    this->InterpolationFuncFloat =
      &(vtkInterpolateNOP<float>::InterpolationFunc);
    this->RowInterpolationFuncDouble =
      &(vtkInterpolateNOP<double>::RowInterpolationFunc);
    this->RowInterpolationFuncFloat =
      &(vtkInterpolateNOP<float>::RowInterpolationFunc);

    return;
    }

  // set the InterpolationInfo object
  vtkInterpolationInfo *info = this->InterpolationInfo;
  vtkIdType *inc = info->Increments;
  int *extent = info->Extent;
  extent[0] = this->Extent[0];
  extent[1] = this->Extent[1];
  extent[2] = this->Extent[2];
  extent[3] = this->Extent[3];
  extent[4] = this->Extent[4];
  extent[5] = this->Extent[5];

  // use the WholeExtent and Tolerance to set the bounds
  double *bounds = this->StructuredBoundsDouble;
  float *fbounds = this->StructuredBoundsFloat;
  int *wholeExtent = this->WholeExtent;
  double tol = this->Tolerance;
  // always restrict the bounds to the limits of int
  int supportSize[3];
  this->ComputeSupportSize(NULL, supportSize);
  // use the max of the three support size values
  int kernelSize = supportSize[0];
  kernelSize = ((supportSize[1] < kernelSize) ? kernelSize : supportSize[1]);
  kernelSize = ((supportSize[2] < kernelSize) ? kernelSize : supportSize[2]);
  double minbound = VTK_INT_MIN + kernelSize/2;
  double maxbound = VTK_INT_MAX - kernelSize/2;

  for (int i = 0; i < 3; i++)
    {
    // use min tolerance of 0.5 if just one slice thick
    double newtol = 0.5*(wholeExtent[2*i] == wholeExtent[2*i+1]);
    newtol = ((newtol > tol) ? newtol : tol);

    double bound = wholeExtent[2*i] - newtol;
    bound = ((bound > minbound) ? bound : minbound);
    fbounds[2*i] = bounds[2*i] = bound;
    bound = wholeExtent[2*i+1] + newtol;
    bound = ((bound < maxbound) ? bound : maxbound);
    fbounds[2*i+1] = bounds[2*i+1] = bound;
    }

  // generate the increments
  int xdim = extent[1] - extent[0] + 1;
  int ydim = extent[3] - extent[2] + 1;

  int ncomp = scalars->GetNumberOfComponents();
  inc[0] = ncomp;
  inc[1] = inc[0] * xdim;
  inc[2] = inc[1] * ydim;

  // compute first component and adjust data pointer
  int component = this->ComponentOffset;
  component = ((component > 0) ? component : 0);
  component = ((component < ncomp) ? component : ncomp-1);

  int dataSize = scalars->GetDataTypeSize();
  void *inPtr = scalars->GetVoidPointer(0);
  info->Pointer = static_cast<char *>(inPtr) + component*dataSize;

  // set all other elements of the InterpolationInfo
  info->ScalarType = scalars->GetDataType();
  info->NumberOfComponents = this->ComputeNumberOfComponents(ncomp);
  info->InterpolationMode = 0;
  info->BorderMode = this->BorderMode;
  info->ExtraInfo = NULL;

  // subclass-specific update
  this->InternalUpdate();

  // get the functions that will perform the interpolation
  this->GetInterpolationFunc(&this->InterpolationFuncDouble);
  this->GetInterpolationFunc(&this->InterpolationFuncFloat);
  this->GetRowInterpolationFunc(&this->RowInterpolationFuncDouble);
  this->GetRowInterpolationFunc(&this->RowInterpolationFuncFloat);
}

//----------------------------------------------------------------------------
bool vtkAbstractImageInterpolator::Interpolate(
  const double point[3], double *value)
{
  double p[3];
  p[0] = (point[0] - this->Origin[0])/this->Spacing[0];
  p[1] = (point[1] - this->Origin[1])/this->Spacing[1];
  p[2] = (point[2] - this->Origin[2])/this->Spacing[2];

  if (this->CheckBoundsIJK(p))
    {
    this->InterpolationFuncDouble(this->InterpolationInfo, p, value);
    return true;
    }

  int n = this->InterpolationInfo->NumberOfComponents;
  for (int i = 0; i < n; i++)
    {
    value[i] = this->OutValue;
    }
  return false;
}

//----------------------------------------------------------------------------
double vtkAbstractImageInterpolator::Interpolate(
  double x, double y, double z, int component)
{
  double value = this->OutValue;
  double point[3];
  point[0] = x;
  point[1] = y;
  point[2] = z;

  double p[3];
  p[0] = (point[0] - this->Origin[0])/this->Spacing[0];
  p[1] = (point[1] - this->Origin[1])/this->Spacing[1];
  p[2] = (point[2] - this->Origin[2])/this->Spacing[2];

  if (this->CheckBoundsIJK(p))
    {
    vtkInterpolationInfo iinfo(*this->InterpolationInfo);
    int ncomp = static_cast<int>(iinfo.Increments[0]);
    ncomp -= this->ComponentOffset;
    int size = vtkAbstractArray::GetDataTypeSize(iinfo.ScalarType);

    component = ((component > 0) ? component : 0);
    component = ((component < ncomp) ? component : ncomp-1);

    iinfo.Pointer = (static_cast<const char *>(iinfo.Pointer) +
                     size*component);
    iinfo.NumberOfComponents = 1;

    this->InterpolationFuncDouble(&iinfo, p, &value);
    }

  return value;
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetInterpolationFunc(
  void (**)(vtkInterpolationInfo *, const double [3], double *))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetInterpolationFunc(
  void (**)(vtkInterpolationInfo *, const float [3], float *))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetRowInterpolationFunc(
  void (**)(vtkInterpolationWeights *, int, int, int, double *, int))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::GetRowInterpolationFunc(
  void (**)(vtkInterpolationWeights *, int, int, int, float *, int))
{
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::PrecomputeWeightsForExtent(
  const double [16], const int [6], int [6], vtkInterpolationWeights *&)
{
  vtkErrorMacro("PrecomputeWeights not supported for this interpolator");
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::PrecomputeWeightsForExtent(
  const float [16], const int [6], int [6], vtkInterpolationWeights *&)
{
  vtkErrorMacro("PrecomputeWeights not supported for this interpolator");
}

//----------------------------------------------------------------------------
void vtkAbstractImageInterpolator::FreePrecomputedWeights(
  vtkInterpolationWeights *&weights)
{
  int *extent = weights->WeightExtent;

  for (int k = 0; k < 3; k++)
    {
    int step = weights->KernelSize[k];
    weights->Positions[k] += step*extent[2*k];
    delete [] weights->Positions[k];
    if (weights->Weights[k])
      {
      if (weights->WeightType == VTK_FLOAT)
        {
        float *constants = static_cast<float *>(weights->Weights[k]);
        constants += step*extent[2*k];
        delete [] constants;
        }
      else
        {
        double *constants = static_cast<double *>(weights->Weights[k]);
        constants += step*extent[2*k];
        delete [] constants;
        }
      }
    }

  delete weights;

  weights = NULL;
}
