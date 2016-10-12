/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSincInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageSincInterpolator.h"
#include "vtkImageInterpolatorInternals.h"
#include "vtkImageData.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

#include "vtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types, because
// they cannot be faithfully represented by doubles
# undef VTK_USE_INT64
# define VTK_USE_INT64 0
# undef VTK_USE_UINT64
# define VTK_USE_UINT64 0

// masks for storing window and size in a single integer
#define VTK_INTERPOLATION_WINDOW_MASK        0x0000007f
#define VTK_INTERPOLATION_WINDOW_XBLUR_MASK  0x00008000
#define VTK_INTERPOLATION_WINDOW_XSIZE_MASK  0x00007f00
#define VTK_INTERPOLATION_WINDOW_XSIZE_SHIFT 8
#define VTK_INTERPOLATION_WINDOW_YBLUR_MASK  0x00800000
#define VTK_INTERPOLATION_WINDOW_YSIZE_MASK  0x007f0000
#define VTK_INTERPOLATION_WINDOW_YSIZE_SHIFT 16
#define VTK_INTERPOLATION_WINDOW_ZBLUR_MASK  0x80000000
#define VTK_INTERPOLATION_WINDOW_ZSIZE_MASK  0x7f000000
#define VTK_INTERPOLATION_WINDOW_ZSIZE_SHIFT 24

// kernel lookup table size must be 256*n where n is kernel half-width
// in order to provide sufficient precision for 16-bit images
#define VTK_SINC_KERNEL_TABLE_DIVISIONS 256

vtkStandardNewMacro(vtkImageSincInterpolator);

//----------------------------------------------------------------------------
vtkImageSincInterpolator::vtkImageSincInterpolator()
{
  this->WindowFunction = VTK_LANCZOS_WINDOW;
  this->WindowHalfWidth = 3;
  this->KernelLookupTable[0] = NULL;
  this->KernelLookupTable[1] = NULL;
  this->KernelLookupTable[2] = NULL;
  this->KernelSize[0] = 6;
  this->KernelSize[1] = 6;
  this->KernelSize[2] = 6;
  this->Antialiasing = 0;
  this->Renormalization = 1;
  this->BlurFactors[0] = 1.0;
  this->BlurFactors[1] = 1.0;
  this->BlurFactors[2] = 1.0;
  this->LastBlurFactors[0] = 1.0;
  this->LastBlurFactors[1] = 1.0;
  this->LastBlurFactors[2] = 1.0;
  this->WindowParameter = 0.5;
  this->UseWindowParameter = 0;
}

//----------------------------------------------------------------------------
vtkImageSincInterpolator::~vtkImageSincInterpolator()
{
  if (this->KernelLookupTable[0])
  {
    this->FreeKernelLookupTable();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WindowFunction: "
     << this->GetWindowFunctionAsString() << "\n";
  os << indent << "WindowHalfWidth: " << this->WindowHalfWidth << "\n";
  os << indent << "UseWindowParameter: "
     << (this->UseWindowParameter ? "On\n" : "Off\n");
  os << indent << "WindowParameter: " << this->WindowParameter << "\n";
  os << indent << "BlurFactors: " << this->BlurFactors[0] << " "
     << this->BlurFactors[1] << " " << this->BlurFactors[2] << "\n";
  os << indent << "Antialiasing: "
     << (this->Antialiasing ? "On\n" : "Off\n");
  os << indent << "Renormalization: "
     << (this->Renormalization ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::ComputeSupportSize(
  const double matrix[16], int size[3])
{
  // compute the default support size for when matrix is null
  if (this->Antialiasing)
  {
    size[0] = VTK_SINC_KERNEL_SIZE_MAX;
    size[1] = VTK_SINC_KERNEL_SIZE_MAX;
    size[2] = VTK_SINC_KERNEL_SIZE_MAX;
  }
  else
  {
    for (int i = 0; i < 3; i++)
    {
      // use blur factors to compute support size
      size[i] = 2*this->WindowHalfWidth;
      double rowscale = this->BlurFactors[i];
      if (rowscale > (1.0 + VTK_INTERPOLATE_FLOOR_TOL))
      {
        size[i] = 2*static_cast<int>(
        rowscale*this->WindowHalfWidth + 1.0 - VTK_INTERPOLATE_FLOOR_TOL);
      }
    }
  }

  if (matrix == NULL)
  {
    return;
  }

  if (this->Antialiasing)
  {
    // if antialiasing is on, initialize blur factors to 1
    this->BlurFactors[0] = 1.0;
    this->BlurFactors[1] = 1.0;
    this->BlurFactors[2] = 1.0;
    this->KernelSize[0] = 2*this->WindowHalfWidth;
    this->KernelSize[1] = 2*this->WindowHalfWidth;
    this->KernelSize[2] = 2*this->WindowHalfWidth;
  }
  else
  {
    // keep blur factors, use kernel size computed from blur factors
    this->KernelSize[0] = size[0];
    this->KernelSize[1] = size[1];
    this->KernelSize[2] = size[2];
  }

  // if matrix does perspective, use the defaults just computed
  if (matrix[12] != 0 || matrix[13] != 0 || matrix[14] != 0 ||
      matrix[15] != 1.0)
  {
    return;
  }

  // use matrix to compute blur factors and kernel size
  for (int i = 0; i < 3; i++)
  {
    int integerRow = 1;
    double rowscale = 0.0;
    for (int j = 0; j < 3; j++)
    {
      // compute the scale from a row of the matrix
      double x = matrix[4*i + j];
      rowscale += x*x;

      // verify that the element is an integer:
      // check fraction that remains after floor operation
      double f;
      vtkInterpolationMath::Floor(x, f);
      integerRow &= (f == 0);
    }

    if (this->Antialiasing)
    {
      // rowscale is the subsampling factor in a particular direction
      rowscale = sqrt(rowscale);
    }
    else
    {
      // ignore computed value, use factor provided by SetBlurFactors()
      rowscale = this->BlurFactors[i];
    }

    // if scale is greater than one, expand kernel size
    if (rowscale > (1.0 + VTK_INTERPOLATE_FLOOR_TOL))
    {
      // need extra suport for antialiasing
      this->BlurFactors[i] = rowscale;
      int s = 2*static_cast<int>(
       rowscale*this->WindowHalfWidth + 1.0 - VTK_INTERPOLATE_FLOOR_TOL);
      size[i] = s;
      this->KernelSize[i] = s;
    }
    else if (integerRow)
    {
      // if no blurring and if ints map to ints, no interpolation is needed
      size[i] = 1;
    }
  }

  // rebuild the kernel lookup tables
  this->InternalUpdate();
}

//----------------------------------------------------------------------------
bool vtkImageSincInterpolator::IsSeparable()
{
  return true;
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetWindowHalfWidth(int size)
{
  static int minsize = 1;
  static int maxsize = VTK_SINC_KERNEL_SIZE_MAX/2;
  size = ((size > minsize) ? size : minsize);
  size = ((size < maxsize) ? size : maxsize);
  if (this->WindowHalfWidth != size)
  {
    this->WindowHalfWidth = size;
    this->KernelSize[0] = 2*size;
    this->KernelSize[1] = 2*size;
    this->KernelSize[2] = 2*size;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetWindowFunction(int mode)
{
  static int minmode = VTK_LANCZOS_WINDOW;
  static int maxmode = VTK_BLACKMAN_NUTTALL4;
  mode = ((mode > minmode) ? mode : minmode);
  mode = ((mode < maxmode) ? mode : maxmode);
  if (this->WindowFunction != mode)
  {
    this->WindowFunction = mode;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char *vtkImageSincInterpolator::GetWindowFunctionAsString()
{
  const char *result = "";

  switch (this->WindowFunction)
  {
    case VTK_LANCZOS_WINDOW:
      result = "Lanczos";
      break;
    case VTK_KAISER_WINDOW:
      result = "Kaiser";
      break;
    case VTK_COSINE_WINDOW:
      result = "Cosine";
      break;
    case VTK_HANN_WINDOW:
      result = "Hann";
      break;
    case VTK_HAMMING_WINDOW:
      result = "Hamming";
      break;
    case VTK_BLACKMAN_WINDOW:
      result = "Blackman";
      break;
    case VTK_BLACKMAN_HARRIS3:
      result = "BlackmanHarris3";
      break;
    case VTK_BLACKMAN_HARRIS4:
      result = "BlackmanHarris4";
      break;
    case VTK_NUTTALL_WINDOW:
      result = "Nuttall";
      break;
    case VTK_BLACKMAN_NUTTALL3:
      result = "BlackmanNuttall3";
      break;
    case VTK_BLACKMAN_NUTTALL4:
      result = "BlackmanNuttall4";
      break;
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetUseWindowParameter(int val)
{
  val = (val != 0);
  if (this->UseWindowParameter != val)
  {
    this->UseWindowParameter = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetWindowParameter(double val)
{
  if (this->WindowParameter != val)
  {
    this->WindowParameter = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetBlurFactors(double x, double y, double z)
{
  if (this->BlurFactors[0] != x ||
      this->BlurFactors[1] != y ||
      this->BlurFactors[2] != z)
  {
    this->BlurFactors[0] = x;
    this->BlurFactors[1] = y;
    this->BlurFactors[2] = z;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetAntialiasing(int val)
{
  val = (val != 0);
  if (this->Antialiasing != val)
  {
    this->Antialiasing = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::SetRenormalization(int val)
{
  val = (val != 0);
  if (this->Renormalization != val)
  {
    this->Renormalization = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::InternalDeepCopy(
  vtkAbstractImageInterpolator *a)
{
  vtkImageSincInterpolator *obj = vtkImageSincInterpolator::SafeDownCast(a);
  if (obj)
  {
    this->SetWindowFunction(obj->WindowFunction);
    this->SetWindowHalfWidth(obj->WindowHalfWidth);
    this->SetUseWindowParameter(obj->UseWindowParameter);
    this->SetWindowParameter(obj->WindowParameter);
    this->SetAntialiasing(obj->Antialiasing);
    if (this->Antialiasing)
    {
      // if blur factors were computed, then don't call "modified"
      obj->GetBlurFactors(this->BlurFactors);
    }
    else
    {
      this->SetBlurFactors(obj->BlurFactors);
    }
  }

  this->KernelSize[0] = 6;
  this->KernelSize[1] = 6;
  this->KernelSize[2] = 6;

  if (this->KernelLookupTable[0])
  {
    this->FreeKernelLookupTable();
  }
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::InternalUpdate()
{
  bool blurchange = false;
  int mode = this->WindowFunction;
  int hsize[3];
  for (int i = 0; i < 3; i++)
  {
    static int minsize = 1;
    static int maxsize = VTK_SINC_KERNEL_SIZE_MAX/2;
    int size = this->KernelSize[i]/2;
    size = ((size > minsize) ? size : minsize);
    size = ((size < maxsize) ? size : maxsize);
    hsize[i] = size;
    blurchange |= (fabs(this->BlurFactors[i] - this->LastBlurFactors[i]) >=
                   VTK_INTERPOLATE_FLOOR_TOL);
  }

  if (this->BlurFactors[0] > 1.0 + VTK_INTERPOLATE_FLOOR_TOL)
  {
    mode |= VTK_INTERPOLATION_WINDOW_XBLUR_MASK;
  }
  if (this->BlurFactors[1] > 1.0 + VTK_INTERPOLATE_FLOOR_TOL)
  {
    mode |= VTK_INTERPOLATION_WINDOW_YBLUR_MASK;
  }
  if (this->BlurFactors[2] > 1.0 + VTK_INTERPOLATE_FLOOR_TOL)
  {
    mode |= VTK_INTERPOLATION_WINDOW_ZBLUR_MASK;
  }

  mode |= (hsize[0] << VTK_INTERPOLATION_WINDOW_XSIZE_SHIFT);
  mode |= (hsize[1] << VTK_INTERPOLATION_WINDOW_YSIZE_SHIFT);
  mode |= (hsize[2] << VTK_INTERPOLATION_WINDOW_ZSIZE_SHIFT);

  if (this->InterpolationInfo->InterpolationMode != mode ||
      blurchange ||
      this->KernelLookupTable[0] == NULL)
  {
    this->BuildKernelLookupTable();
  }

  this->InterpolationInfo->InterpolationMode = mode;
  this->InterpolationInfo->ExtraInfo = this->KernelLookupTable;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  Interpolation subroutines and associated code
//----------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------
// Special functions

// Compute the sinc function (leave undefined at x=0 for efficiency,
// the code that uses it never evaluates it at x=0)
inline double vtkSincPi(double x)
{
  x *= vtkMath::Pi();

  return sin(x)/x;
}

// Compute the modified bessel function I0
inline double vtkBesselI0(double x)
{
  int m = 0;
  double x2 = 0.25*x*x;
  double p = 1;
  double b = 1;
  do
  {
    m++;
    p *= x2/(m*m);
    b += p;
  }
  while (p > b*VTK_DBL_EPSILON);

  return b;
}

//----------------------------------------------------------------------------
// Sinc window functions

struct vtkSincWindow
{
  static double Lanczos(double x);
  static double Kaiser(double x, double a);
  static double Cosine(double x);
  template<int N>
  static double Hamming(double x, const double *a);
};

double vtkSincWindow::Lanczos(double x)
{
  return vtkSincPi(x);
}

double vtkSincWindow::Kaiser(double x, double a)
{
  double api = a*vtkMath::Pi();
  double y = 1 - x*x;
  y *= (y > 0); // if less than zero, set to zero

  return vtkBesselI0(api*sqrt(y))/vtkBesselI0(api);
}

double vtkSincWindow::Cosine(double x)
{
  double halfpi = 0.5*vtkMath::Pi();

  return cos(x*halfpi);
}

template<int N>
double vtkSincWindow::Hamming(double x, const double *a)
{
  double q = 0;
  double y = a[0];
  x *= vtkMath::Pi();
  for (int i = 1; i < N; i++)
  {
    q += x;
    y += a[i]*cos(q);
  }
  return y;
}

//----------------------------------------------------------------------------
// Sinc kernel computation: compute half of the interpolation kernel,
// including n sinc lobes, to make a lookup table of size "size".
// In the table, x=0.0 corresponds to index position zero, and
// x = 1.0 corresponds to index position "size", which is just
// beyond the end of the table and holds an implicit value of zero.

struct vtkSincKernel
{
  template<class F>
  static void Lanczos(F *kernel, int size, int n, double p);
  template<class F>
  static void Kaiser(F *kernel, int size, int n, double p, double a);
  template<class F>
  static void Cosine(F *kernel, int size, int n, double p);
  template<class F, int N>
  static void Hamming(F *kernel, int size, int n, double p, const double *a);
};

template<class F>
void vtkSincKernel::Lanczos(F *kernel, int size, int n, double p)
{
  double q = n*p;
  double x = p;
  double y = q;
  *kernel++ = 1.0;
  --size;
  do
  {
    int inbounds = (x < 1.0);
    *kernel++ = vtkSincWindow::Lanczos(x)*vtkSincPi(y)*inbounds;
    x += p;
    y += q;
  }
  while (--size);
}

template<class F>
void vtkSincKernel::Kaiser(F *kernel, int size, int n, double p, double a)
{
  // The Kaiser window has a tunable parameter "alpha", where
  // a smaller alpha increases sharpness (and ringing) while a
  // larger alpha increases blurring.  Setting alpha equal to n
  // closely approximates the optimal alpha values shown in
  // Helwig Hauser, Eduard Groller, Thomas Theussl,
  // "Mastering Windows: Improving Reconstruction,"
  // IEEE Symposium on Volume Visualization and Graphics (VV 2000),
  // pp. 101-108, 2000
  a = ((a >= 0) ? a : n);
  double q = n*p;
  double x = p;
  double y = q;
  *kernel++ = 1.0;
  --size;
  do
  {
    int inbounds = (x < 1.0);
    *kernel++ = vtkSincWindow::Kaiser(x, a)*vtkSincPi(y)*inbounds;
    x += p;
    y += q;
  }
  while (--size);
}

template<class F>
void vtkSincKernel::Cosine(F *kernel, int size, int n, double p)
{
  double q = n*p;
  double x = p;
  double y = q;
  *kernel++ = 1;
  --size;
  do
  {
    int inbounds = (x < 1.0);
    *kernel++ = vtkSincWindow::Cosine(x)*vtkSincPi(y)*inbounds;
    x += p;
    y += q;
  }
  while (--size);
}

template<class F, int N>
void vtkSincKernel::Hamming(F *kernel, int size, int n, double p, const double *a)
{
  double q = n*p;
  double x = p;
  double y = q;
  *kernel++ = 1.0;
  --size;
  do
  {
    int inbounds = (x < 1.0);
    *kernel++ = vtkSincWindow::Hamming<N>(x, a)*vtkSincPi(y)*inbounds;
    x += p;
    y += q;
  }
  while (--size);
}

//----------------------------------------------------------------------------
template<class T, class F>
void vtkSincInterpWeights(T *kernel, F *fX, F fx, int m)
{
  // table bins per unit
  int p = VTK_SINC_KERNEL_TABLE_DIVISIONS;

  // compute table interpolation info
  F f = fx*p;
  int offset = static_cast<int>(f);
  f -= offset;
  F r = 1 - f;

  // interpolate the table, partially unrolled loop
  int n = (m >> 1);
  int i = (1 - n)*p - offset;
  do
  {
    int i0 = i;
    int i1 = i + 1;
    int ni = -i0;
    i0 = ((i0 >= 0) ? i0 : ni);
    ni = -i1;
    i1 = ((i1 >= 0) ? i1 : ni);
    F y = r*kernel[i0] + f*kernel[i1];
    fX[0] = y;
    i += p;
    i0 = i;
    i1 = i + 1;
    ni = -i0;
    i0 = ((i0 >= 0) ? i0 : ni);
    ni = -i1;
    i1 = ((i1 >= 0) ? i1 : ni);
    y = r*kernel[i0] + f*kernel[i1];
    fX[1] = y;
    i += p;
    fX += 2;
  }
  while (--n);
}

//----------------------------------------------------------------------------
// Ensure that the set of n coefficients extracted from the kernel table
// will always sum to unity.  This renormalization is needed to ensure that
// the interpolation will not have a DC offset.  For the rationale, see e.g.
//  NA Thacker, A Jackson, D Moriarty, E Vokurka, "Improved quality of
//  re-sliced MR images usng re-normalized sinc interpolation," Journal of
//  Magnetic Resonance Imaging 10:582-588, 1999.
// Parameters:
//  kernel = table containing half of a symmetric kernel
//  m = table offset between lookup positions for adjacent weights
//  n = number of kernel weights (i.e. size of discrete kernel size)
//  The kernel table size must be (n*m+1)/2
template<class T>
void vtkRenormalizeKernel(T *kernel, int m, int n)
{
  // the fact that we only have half the kernel makes the weight
  // lookup more complex: there will be kn direct lookups and km
  // mirror lookups.
  int kn = (n + 1)/2;
  int km = n - kn;

  if (m == 0 || km == 0)
  {
    return;
  }

  // get sum of weights for zero offset
  T w = - (*kernel)*0.5;
  T *kernel2 = kernel;
  int k = kn;
  do
  {
    w += *kernel2;
    kernel2 += m;
  }
  while (--k);

  // divide weights by their sum to renormalize
  w *= 2;
  kernel2 = kernel;
  k = kn;
  do
  {
    *kernel2 /= w;
    kernel2 += m;
  }
  while (--k);

  // need the opposite end of the kernel array
  kernel2 = kernel + km*m;

  int j = (m - 1)/2;
  if (j) do
  {
    // move to next offset
    kernel++;
    kernel2--;

    // get the sum of the weights at this offset
    w = 0;
    T *kernel1 = kernel2;
    k = km;
    do
    {
      w += *kernel1;
      kernel1 -= m;
    }
    while (--k);
    kernel1 = kernel;
    k = kn;
    do
    {
      w += *kernel1;
      kernel1 += m;
    }
    while (--k);

    // divide the weights by their sum to renormalize
    kernel1 = kernel2;
    k = km;
    do
    {
      *kernel1 /= w;
      kernel1 -= m;
    }
    while (--k);
    kernel1 = kernel;
    k = kn;
    do
    {
      *kernel1 /= w;
      kernel1 += m;
    }
    while (--k);
  }
  while (--j);

  // get sum of weights for offset of 0.5 (only applies when m is even)
  if ((m & 1) == 0)
  {
    w = 0;
    kernel++;
    kernel2 = kernel;
    k = km;
    do
    {
      w += *kernel2;
      kernel2 += m;
    }
    while (--k);

    // divide weights by their sum to renormalize
    w *= 2;
    kernel2 = kernel;
    k = km;
    do
    {
      *kernel2 /= w;
      kernel2 += m;
    }
    while (--k);
  }
}

//----------------------------------------------------------------------------
template<class F, class T>
struct vtkImageSincInterpolate
{
  static void General(
    vtkInterpolationInfo *info, const F point[3], F *outPtr);
};

//----------------------------------------------------------------------------
template <class F, class T>
void vtkImageSincInterpolate<F, T>::General(
  vtkInterpolationInfo *info, const F point[3], F *outPtr)
{
  const T *inPtr = static_cast<const T *>(info->Pointer);
  int *inExt = info->Extent;
  vtkIdType *inInc = info->Increments;
  int numscalars = info->NumberOfComponents;

  // kernel lookup table
  float **kernel = static_cast<float **>(info->ExtraInfo);

  // size of kernel
  int mode = info->InterpolationMode;
  int xm = 2*((mode & VTK_INTERPOLATION_WINDOW_XSIZE_MASK)
              >> VTK_INTERPOLATION_WINDOW_XSIZE_SHIFT);
  int ym = 2*((mode & VTK_INTERPOLATION_WINDOW_YSIZE_MASK)
              >> VTK_INTERPOLATION_WINDOW_YSIZE_SHIFT);
  int zm = 2*((mode & VTK_INTERPOLATION_WINDOW_ZSIZE_MASK)
              >> VTK_INTERPOLATION_WINDOW_ZSIZE_SHIFT);

  // index to kernel midpoint position
  int xm2 = ((xm - 1) >> 1);
  int ym2 = ((ym - 1) >> 1);
  int zm2 = ((zm - 1) >> 1);

  F fx, fy, fz;
  int inIdX0 = vtkInterpolationMath::Floor(point[0], fx);
  int inIdY0 = vtkInterpolationMath::Floor(point[1], fy);
  int inIdZ0 = vtkInterpolationMath::Floor(point[2], fz);

  // change arrays into locals
  vtkIdType inIncX = inInc[0];
  vtkIdType inIncY = inInc[1];
  vtkIdType inIncZ = inInc[2];

  int minX = inExt[0];
  int maxX = inExt[1];
  int minY = inExt[2];
  int maxY = inExt[3];
  int minZ = inExt[4];
  int maxZ = inExt[5];

  // the memory offsets
  vtkIdType factX[VTK_SINC_KERNEL_SIZE_MAX];
  vtkIdType factY[VTK_SINC_KERNEL_SIZE_MAX];
  vtkIdType factZ[VTK_SINC_KERNEL_SIZE_MAX];

  // handle the borders
  int xi = inIdX0 - xm2;
  int yi = inIdY0 - ym2;
  int zi = inIdZ0 - zm2;
  int mm = xm;
  mm = ((mm >= ym) ? mm : ym);
  mm = ((mm >= zm) ? mm : zm);

  switch (info->BorderMode)
  {
    case VTK_IMAGE_BORDER_REPEAT:
    {
      int l = 0;
      do
      {
        factX[l] = vtkInterpolationMath::Wrap(xi, minX, maxX)*inIncX;
        factY[l] = vtkInterpolationMath::Wrap(yi, minY, maxY)*inIncY;
        factZ[l] = vtkInterpolationMath::Wrap(zi, minZ, maxZ)*inIncZ;
        l++; xi++; yi++; zi++;
      }
      while (--mm);
    }
      break;

    case VTK_IMAGE_BORDER_MIRROR:
    {
      int l = 0;
      do
      {
        factX[l] = vtkInterpolationMath::Mirror(xi, minX, maxX)*inIncX;
        factY[l] = vtkInterpolationMath::Mirror(yi, minY, maxY)*inIncY;
        factZ[l] = vtkInterpolationMath::Mirror(zi, minZ, maxZ)*inIncZ;
        l++; xi++; yi++; zi++;
      }
      while (--mm);
    }
      break;

    default:
    {
      int l = 0;
      do
      {
        factX[l] = vtkInterpolationMath::Clamp(xi, minX, maxX)*inIncX;
        factY[l] = vtkInterpolationMath::Clamp(yi, minY, maxY)*inIncY;
        factZ[l] = vtkInterpolationMath::Clamp(zi, minZ, maxZ)*inIncZ;
        l++; xi++; yi++; zi++;
      }
      while (--mm);
    }
      break;
  }

  // compute the kernel weights
  F fX[VTK_SINC_KERNEL_SIZE_MAX];
  F fY[VTK_SINC_KERNEL_SIZE_MAX];
  F fZ[VTK_SINC_KERNEL_SIZE_MAX];

  vtkSincInterpWeights(kernel[0], fX, fx, xm);
  vtkSincInterpWeights(kernel[1], fY, fy, ym);
  vtkSincInterpWeights(kernel[2], fZ, fz, zm);

  // check if only one slice in a particular direction
  int multipleY = (minY != maxY);
  int multipleZ = (minZ != maxZ);

  // the limits to use when doing the interpolation
  int k1 = zm2*(1 - multipleZ);
  int k2 = (zm2 + 1)*(multipleZ + 1) - 1;
  int j1 = ym2*(1 - multipleY);
  int j2 = (ym2 + 1)*(multipleY + 1) - 1;

  do // loop over components
  {
    F val = 0;
    int k = k1;
    do // loop over z
    {
      F ifz = fZ[k];
      vtkIdType factz = factZ[k];
      int j = j1;
      do // loop over y
      {
        F ify = fY[j];
        F fzy = ifz*ify;
        vtkIdType factzy = factz + factY[j];
        // loop over x
        const T *tmpPtr = inPtr + factzy;
        const F *tmpfX = fX;
        const vtkIdType *tmpfactX = factX;
        F tmpval = 0;
        int l = (xm >> 1);
        do
        {
          tmpval += tmpfX[0]*tmpPtr[tmpfactX[0]];
          tmpval += tmpfX[1]*tmpPtr[tmpfactX[1]];
          tmpfX += 2;
          tmpfactX += 2;
        }
        while (--l);
        val += fzy*tmpval;
      }
      while (++j <= j2);
    }
    while (++k <= k2);

    *outPtr++ = val;
    inPtr++;
  }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Get the interpolation function for the specified data types
template<class F>
void vtkImageSincInterpolatorGetInterpolationFunc(
  void (**interpolate)(vtkInterpolationInfo *, const F [3], F *),
  int dataType, int vtkNotUsed(interpolationMode))
{
  switch (dataType)
  {
    vtkTemplateAliasMacro(
      *interpolate =
        &(vtkImageSincInterpolate<F, VTK_TT>::General)
      );
    default:
      *interpolate = 0;
  }
}

//----------------------------------------------------------------------------
// Interpolation for precomputed weights

template <class F, class T>
struct vtkImageSincRowInterpolate
{
  static void General(
    vtkInterpolationWeights *weights, int idX, int idY, int idZ,
    F *outPtr, int n);
};


//--------------------------------------------------------------------------
// helper function for high-order interpolation
template<class F, class T>
void vtkImageSincRowInterpolate<F, T>::General(
  vtkInterpolationWeights *weights, int idX, int idY, int idZ,
  F *outPtr, int n)
{
  int stepX = weights->KernelSize[0];
  int stepY = weights->KernelSize[1];
  int stepZ = weights->KernelSize[2];
  idX *= stepX;
  idY *= stepY;
  idZ *= stepZ;
  const F *fX = static_cast<F *>(weights->Weights[0]) + idX;
  const F *fY = static_cast<F *>(weights->Weights[1]) + idY;
  const F *fZ = static_cast<F *>(weights->Weights[2]) + idZ;
  const vtkIdType *factX = weights->Positions[0] + idX;
  const vtkIdType *factY = weights->Positions[1] + idY;
  const vtkIdType *factZ = weights->Positions[2] + idZ;
  const T *inPtr = static_cast<const T *>(weights->Pointer);

  int numscalars = weights->NumberOfComponents;
  for (int i = n; i > 0; --i)
  {
    const T *inPtr0 = inPtr;
    int c = numscalars;
    do // loop over components
    {
      F val = 0;
      int k = 0;
      do // loop over z
      {
        F ifz = fZ[k];
        vtkIdType factz = factZ[k];
        int j = 0;
        do // loop over y
        {
          F ify = fY[j];
          F fzy = ifz*ify;
          vtkIdType factzy = factz + factY[j];
          // loop over x
          const T *tmpPtr = inPtr0 + factzy;
          const F *tmpfX = fX;
          const vtkIdType *tmpfactX = factX;
          F tmpval = 0;
          int l = stepX;
          do
          {
            tmpval += tmpfX[0]*tmpPtr[tmpfactX[0]];
            tmpfX++;
            tmpfactX++;
          }
          while (--l);
          val += fzy*tmpval;
        }
        while (++j < stepY);
      }
      while (++k < stepZ);

      *outPtr++ = val;
      inPtr0++;
    }
    while (--c);

    factX += stepX;
    fX += stepX;
  }
}

//----------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
template<class F>
void vtkImageSincInterpolatorGetRowInterpolationFunc(
  void (**summation)(vtkInterpolationWeights *weights, int idX, int idY,
                     int idZ, F *outPtr, int n),
  int scalarType, int vtkNotUsed(interpolationMode))
{
  switch (scalarType)
  {
    vtkTemplateAliasMacro(
      *summation = &(vtkImageSincRowInterpolate<F,VTK_TT>::General)
      );
    default:
      *summation = 0;
  }
}

//----------------------------------------------------------------------------
template<class F>
void vtkImageSincInterpolatorPrecomputeWeights(
  const F newmat[16], const int outExt[6], int clipExt[6],
  const F bounds[6], vtkInterpolationWeights *weights)
{
  float **kernel = static_cast<float **>(weights->ExtraInfo);
  weights->WeightType = vtkTypeTraits<F>::VTKTypeID();
  int sizes[3];
  bool blur[3];
  int mode = weights->InterpolationMode;
  sizes[0] = 2*((mode & VTK_INTERPOLATION_WINDOW_XSIZE_MASK)
                >> VTK_INTERPOLATION_WINDOW_XSIZE_SHIFT);
  sizes[1] = 2*((mode & VTK_INTERPOLATION_WINDOW_YSIZE_MASK)
                >> VTK_INTERPOLATION_WINDOW_YSIZE_SHIFT);
  sizes[2] = 2*((mode & VTK_INTERPOLATION_WINDOW_ZSIZE_MASK)
                >> VTK_INTERPOLATION_WINDOW_ZSIZE_SHIFT);
  blur[0] = ((mode & VTK_INTERPOLATION_WINDOW_XBLUR_MASK) != 0);
  blur[1] = ((mode & VTK_INTERPOLATION_WINDOW_YBLUR_MASK) != 0);
  blur[2] = ((mode & VTK_INTERPOLATION_WINDOW_ZBLUR_MASK) != 0);

  // set up input positions table for interpolation
  bool validClip = true;
  for (int j = 0; j < 3; j++)
  {
    // set k to the row for which the element in column j is nonzero,
    // and set matrow to the elements of that row
    int k = 0;
    const F *matrow = newmat;
    while (k < 3 && matrow[j] == 0)
    {
      k++;
      matrow += 4;
    }

    // get the extents
    clipExt[2*j] = outExt[2*j];
    clipExt[2*j + 1] = outExt[2*j + 1];
    int minExt = weights->Extent[2*k];
    int maxExt = weights->Extent[2*k + 1];
    F minBounds = bounds[2*k];
    F maxBounds = bounds[2*k + 1];

    // the kernel size should not exceed the input dimension
    int m = sizes[j];
    int m2 = ((m - 1) >> 1);
    int step = m;
    int inCount = maxExt - minExt + 1;
    step = ((step < inCount) ? step : inCount);

    // if output pixels lie exactly on top of the input pixels
    F f1, f2;
    vtkInterpolationMath::Floor(matrow[j], f1);
    vtkInterpolationMath::Floor(matrow[3], f2);
    if (f1 == 0 && f2 == 0 && !blur[j])
    {
      step = 1;
    }

    // allocate space for the weights
    vtkIdType size = step*(outExt[2*j+1] - outExt[2*j] + 1);
    vtkIdType *positions = new vtkIdType[size];
    positions -= step*outExt[2*j];
    F *constants = new F[size];
    constants -= step*outExt[2*j];

    weights->KernelSize[j] = step;
    weights->Positions[j] = positions;
    weights->Weights[j] = constants;
    weights->WeightExtent[2*j] = outExt[2*j];
    weights->WeightExtent[2*j+1] = outExt[2*j+1];

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
    {
      F point = matrow[3] + i*matrow[j];

      F f = 0;
      int idx = vtkInterpolationMath::Floor(point, f);
      int lmax = 1;
      if (step > 1)
      {
        idx -= m2;
        lmax = m;
      }

      int inId[VTK_SINC_KERNEL_SIZE_MAX];

      int l = 0;
      switch (weights->BorderMode)
      {
        case VTK_IMAGE_BORDER_REPEAT:
          do
          {
            inId[l] = vtkInterpolationMath::Wrap(idx++, minExt, maxExt);
          }
          while (++l < lmax);
          break;

        case VTK_IMAGE_BORDER_MIRROR:
          do
          {
            inId[l] = vtkInterpolationMath::Mirror(idx++, minExt, maxExt);
          }
          while (++l < lmax);
          break;

        default:
          do
          {
            inId[l] = vtkInterpolationMath::Clamp(idx++, minExt, maxExt);
          }
          while (++l < lmax);
          break;
      }

      // compute the weights and offsets
      vtkIdType inInc = weights->Increments[k];
      if (step == 1)
      {
        positions[step*i] = inId[0]*inInc;
        constants[step*i] = static_cast<F>(1);
      }
      else
      {
        F g[VTK_SINC_KERNEL_SIZE_MAX];
        vtkSincInterpWeights(kernel[j], g, f, m);

        if (step == m)
        {
          int ll = 0;
          do
          {
            positions[step*i + ll] = inId[ll]*inInc;
            constants[step*i + ll] = g[ll];
          }
          while (++ll < step);
        }
        else
        {
          // it gets tricky if the data is thinner than the kernel
          F gg[VTK_SINC_KERNEL_SIZE_MAX];
          int ll = 0;
          do { gg[ll] = 0; } while (++ll < m);
          ll = 0;
          do
          {
            int rIdx = inId[ll];
            gg[rIdx] += g[ll];
          }
          while (++ll < m);
          ll = 0;
          do
          {
            positions[step*i + ll] = ll*inInc;
            constants[step*i + ll] = gg[ll];
          }
          while (++ll < step);
        }
      }

      if (point >= minBounds && point <= maxBounds)
      {
        if (region == 0)
        { // entering the input extent
          region = 1;
          clipExt[2*j] = i;
        }
      }
      else
      {
        if (region == 1)
        { // leaving the input extent
          region = 2;
          clipExt[2*j+1] = i - 1;
        }
      }
    }

    if (region == 0 || clipExt[2*j] > clipExt[2*j+1])
    { // never entered input extent!
      validClip = false;
    }
  }

  if (!validClip)
  {
    // output extent doesn't itersect input extent
    for (int j = 0; j < 3; j++)
    {
      clipExt[2*j] = outExt[2*j];
      clipExt[2*j + 1] = outExt[2*j] - 1;
    }
  }
}


//----------------------------------------------------------------------------
} // ends anonymous namespace

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::GetInterpolationFunc(
  void (**func)(vtkInterpolationInfo *, const double [3], double *))
{
  vtkImageSincInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->WindowFunction);
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::GetInterpolationFunc(
  void (**func)(vtkInterpolationInfo *, const float [3], float *))
{
  vtkImageSincInterpolatorGetInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->WindowFunction);
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::GetRowInterpolationFunc(
  void (**func)(vtkInterpolationWeights *, int, int, int, double *, int))
{
  vtkImageSincInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->WindowFunction);
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::GetRowInterpolationFunc(
  void (**func)(vtkInterpolationWeights *, int, int, int, float *, int))
{
  vtkImageSincInterpolatorGetRowInterpolationFunc(
    func, this->InterpolationInfo->ScalarType, this->WindowFunction);
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::PrecomputeWeightsForExtent(
  const double matrix[16], const int extent[6], int newExtent[6],
  vtkInterpolationWeights *&weights)
{
  weights = new vtkInterpolationWeights(*this->InterpolationInfo);

  vtkImageSincInterpolatorPrecomputeWeights(
    matrix, extent, newExtent, this->StructuredBoundsDouble, weights);
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::PrecomputeWeightsForExtent(
  const float matrix[16], const int extent[6], int newExtent[6],
  vtkInterpolationWeights *&weights)
{
  weights = new vtkInterpolationWeights(*this->InterpolationInfo);

  vtkImageSincInterpolatorPrecomputeWeights(
    matrix, extent, newExtent, this->StructuredBoundsFloat, weights);
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::FreePrecomputedWeights(
  vtkInterpolationWeights *&weights)
{
  this->Superclass::FreePrecomputedWeights(weights);
}

//----------------------------------------------------------------------------
// build any tables required for the interpolation
void vtkImageSincInterpolator::BuildKernelLookupTable()
{
  if (this->KernelLookupTable[0])
  {
    this->FreeKernelLookupTable();
  }

  float *kernel[3];
  kernel[0] = 0;
  kernel[1] = 0;
  kernel[2] = 0;

  for (int i = 0; i < 3; i++)
  {
    // reuse the X kernel lookup table if possible
    if (i > 0 && this->KernelSize[i] == this->KernelSize[0] &&
        fabs(this->BlurFactors[i] - this->BlurFactors[0]) <
          VTK_INTERPOLATE_FLOOR_TOL)
    {
      kernel[i] = kernel[0];
      continue;
    }

    // kernel parameters
    int n = this->WindowHalfWidth;
    int m = this->KernelSize[i];
    double b = this->BlurFactors[i];

    // reduce lobe count until kernel is within size limit
    while (n > 1 && 2*n*b > static_cast<double>(VTK_SINC_KERNEL_SIZE_MAX))
    {
      --n;
      m = VTK_SINC_KERNEL_SIZE_MAX;
    }

    // blur factor must be restricted to half the max kernel size
    if (b > 0.5*VTK_SINC_KERNEL_SIZE_MAX)
    {
      b = 0.5*VTK_SINC_KERNEL_SIZE_MAX;
    }

    // compute lookup table size and step size
    int size = m/2*VTK_SINC_KERNEL_TABLE_DIVISIONS;
    double p = 1.0/(b*n*VTK_SINC_KERNEL_TABLE_DIVISIONS);

    // allocate and compute the kernel lookup table
    // (add a small safety buffer that will be filled with zeros)
    kernel[i] = new float[size + 4];

    // the tunable parameter, set to -1 to use default
    double a = (this->UseWindowParameter ? this->WindowParameter : -1.0);

    // constants for various windows
    static double hann[] = { 0.5, 0.5 };
    static double hamming[] = { 0.54, 0.46 };
    static double blackman[] = { 0.42, 0.50, 0.08 };
    // FJ Harris, "On the use of windows for harmonic analysis with
    // the discrete fourier transform," Proc. IEEE 66:51-83, 1978.
    static double harris3[] = { 0.42323, 0.49755, 0.07922 };
    static double harris4[] = { 0.35875, 0.48829, 0.14128, 0.01168 };
    // AH Nuttall, "Some windows with very good sidelobe behavior," IEEE
    // Transactions on Acoustics, Speech, and Signal Processing 29:84-91, 1981
    static double nuttall[] = { 0.355768, 0.487396, 0.144232, 0.012604 };
    static double nuttall3[] = { 0.4243801, 0.4973406, 0.0782793 };
    static double nuttall4[] = { 0.3635819, 0.4891775, 0.1365995, 0.0106411 };

    switch (this->WindowFunction)
    {
      case VTK_LANCZOS_WINDOW:
        vtkSincKernel::Lanczos(kernel[i], size, n, p);
        break;
      case VTK_KAISER_WINDOW:
        vtkSincKernel::Kaiser(kernel[i], size, n, p, a);
        break;
      case VTK_COSINE_WINDOW:
        vtkSincKernel::Cosine(kernel[i], size, n, p);
        break;
      case VTK_HANN_WINDOW:
        vtkSincKernel::Hamming<float, 2>(kernel[i], size, n, p, hann);
        break;
      case VTK_HAMMING_WINDOW:
        vtkSincKernel::Hamming<float, 2>(kernel[i], size, n, p, hamming);
        break;
      case VTK_BLACKMAN_WINDOW:
        vtkSincKernel::Hamming<float, 3>(kernel[i], size, n, p, blackman);
        break;
      case VTK_BLACKMAN_HARRIS3:
        vtkSincKernel::Hamming<float, 3>(kernel[i], size, n, p, harris3);
        break;
      case VTK_BLACKMAN_HARRIS4:
        vtkSincKernel::Hamming<float, 4>(kernel[i], size, n, p, harris4);
        break;
      case VTK_NUTTALL_WINDOW:
        vtkSincKernel::Hamming<float, 4>(kernel[i], size, n, p, nuttall);
        break;
      case VTK_BLACKMAN_NUTTALL3:
        vtkSincKernel::Hamming<float, 3>(kernel[i], size, n, p, nuttall3);
        break;
      case VTK_BLACKMAN_NUTTALL4:
        vtkSincKernel::Hamming<float, 4>(kernel[i], size, n, p, nuttall4);
        break;
    }

    // add a tail of zeros for when table is interpolated
    kernel[i][size] = 0;
    kernel[i][size+1] = 0;
    kernel[i][size+2] = 0;
    kernel[i][size+3] = 0;

    // renormalize the table if requested
    if (this->Renormalization)
    {
      vtkRenormalizeKernel(kernel[i], VTK_SINC_KERNEL_TABLE_DIVISIONS, m);
    }
    else if (b > 1.0)
    {
      // if kernel stretched to create blur, divide by stretch factor
      float *ktmp = kernel[i];
      float bf = 1.0/b;
      int j = size;
      do
      {
        *ktmp *= bf;
        ktmp++;
      }
      while (--j);
    }
  }

  this->KernelLookupTable[0] = kernel[0];
  this->KernelLookupTable[1] = kernel[1];
  this->KernelLookupTable[2] = kernel[2];

  this->LastBlurFactors[0] = this->BlurFactors[0];
  this->LastBlurFactors[1] = this->BlurFactors[1];
  this->LastBlurFactors[2] = this->BlurFactors[2];
}

//----------------------------------------------------------------------------
void vtkImageSincInterpolator::FreeKernelLookupTable()
{
  float *kernel = this->KernelLookupTable[0];
  if (kernel)
  {
    delete [] kernel;
    for (int i = 1; i < 3; i++)
    {
      if (this->KernelLookupTable[i] != kernel)
      {
        delete [] this->KernelLookupTable[i];
      }
    }
  }
}
