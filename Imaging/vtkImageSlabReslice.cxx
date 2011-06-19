/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSlabReslice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSlabReslice.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageResliceDetail.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkImageSlabReslice);

//----------------------------------------------------------------------------
vtkImageSlabReslice::vtkImageSlabReslice()
{
  // Input is 3D, ouptut is a 2D projection within the slab.
  this->OutputDimensionality = 2;

  // Number of sample points along the blendDirection to the resliced
  // direction that will be "slabbed"
  this->NumBlendSamplePoints = 1;

  // Default blend mode is maximum intensity projection through the data.
  this->BlendMode = VTK_IMAGESLAB_BLEND_MAX;

  this->SlabThickness = 10; // mm or world coords
  this->SlabResolution = 1; // mm or world coords
}

//----------------------------------------------------------------------------
vtkImageSlabReslice::~vtkImageSlabReslice()
{
}

//----------------------------------------------------------------------------
int vtkImageSlabReslice::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  this->NumBlendSamplePoints = 2*(static_cast<
      int >(this->SlabThickness/(2.0 * this->SlabResolution))) + 1;

  return 1;
}

//----------------------------------------------------------------------------
// Blending functions and Interpolation functions

template<class F, class T>
struct vtkImageSlabResliceInterpolate
{
  static int NearestNeighbor(
    F *&outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int interpMode, int blendMode, const void *background);

  static int Trilinear(
    F *&outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int interpMode, int blendMode, const void *background);

  static int Tricubic(
    F *&outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int interpMode, int blendMode, const void *background);
};


#define VTK_RESLICE_BACKGROUND 0   // use background if out-of-bounds
#define VTK_RESLICE_BORDER     3   // use a half-voxel border

#define max(a,b) \
  ((a) > (b) ? (a) : (b))

#define min(a,b) \
  ((a) < (b) ? (a) : (b))


// This macro is executed to exercise conditional blends. The mean simply
// sums the data. It is expected to be averaged outside the loop.
#define vtkImageSlabResliceBlend( res, in, blendMode ) \
  if (blendMode == VTK_IMAGESLAB_BLEND_MAX) \
    { \
    res = max(res, in); \
    } \
  else if (blendMode == VTK_IMAGESLAB_BLEND_MIN) \
    { \
    res = min(res, in); \
    } \
  else if (blendMode == VTK_IMAGESLAB_BLEND_MEAN) \
    { \
    res = ((res) + (in)); \
    }

//----------------------------------------------------------------------------
void vtkImageSlabResliceGetBorderModes(
    vtkImageSlabReslice *self,
    int &interpolationBorderMode, int &wrap )
{
  // FIXME: Only BORDER has been tested so far. Wrap and Mirror also have
  // never been tested.

  wrap = (self->GetWrap() || self->GetMirror());

  if (self->GetMirror())
    {
    interpolationBorderMode = VTK_RESLICE_MIRROR;
    }
  else if (self->GetWrap())
    {
    interpolationBorderMode = VTK_RESLICE_REPEAT;
    }
  else if (self->GetBorder())
    {
    interpolationBorderMode = VTK_RESLICE_BORDER;
    }
  else
    {
    interpolationBorderMode = VTK_RESLICE_BACKGROUND;
    }
}

//----------------------------------------------------------------------------
// Nearest Neighbor interpolation and blending
// See vtkImageReslice for details. This is nearly the same as that method,
// except that its enhanced to blend the data as well.
//
template <class F, class T>
int vtkImageSlabResliceInterpolate<F, T>::NearestNeighbor(
  F *&outVoidPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int interpmode, int blendMode, const void * /*voidBackground */ )
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  F *outPtr = outVoidPtr;

  int inIdX0 = vtkResliceRound(point[0]) - inExt[0];
  int inIdY0 = vtkResliceRound(point[1]) - inExt[2];
  int inIdZ0 = vtkResliceRound(point[2]) - inExt[4];

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  if (inIdX0 < 0 || inIdX0 >= inExtX ||
      inIdY0 < 0 || inIdY0 >= inExtY ||
      inIdZ0 < 0 || inIdZ0 >= inExtZ)
    {
    if (interpmode == VTK_RESLICE_REPEAT)
      {
      vtkInterpolateWrap3( inIdX0, inIdY0, inIdZ0, inExtX, inExtY, inExtZ );
      }
    else if (interpmode == VTK_RESLICE_MIRROR)
      {
      vtkInterpolateMirror3( inIdX0, inIdY0, inIdZ0, inExtX, inExtY, inExtZ );
      }
    else
      {
      return 0;
      }
    }

  inPtr += inIdX0*inInc[0]+inIdY0*inInc[1]+inIdZ0*inInc[2];


  do
    {
    vtkImageSlabResliceBlend( *outPtr, *inPtr, blendMode );
    ++outPtr;
    ++inPtr;
    }
  while (--numscalars);

  return 1;
}

//----------------------------------------------------------------------------
// Trilinear interpolation and blending
// See vtkImageReslice for details. This is nearly the same as that method,
// except that its enhanced to blend the data as well.
//
template <class F, class T>
int vtkImageSlabResliceInterpolate<F, T>::Trilinear(
  F *&outVoidPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int interpmode, int blendMode, const void * /* voidBackground */ )
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  F *outPtr = outVoidPtr;

  F fx, fy, fz;
  int floorX = vtkResliceFloor(point[0], fx);
  int floorY = vtkResliceFloor(point[1], fy);
  int floorZ = vtkResliceFloor(point[2], fz);

  int inIdX0 = floorX - inExt[0];
  int inIdY0 = floorY - inExt[2];
  int inIdZ0 = floorZ - inExt[4];

  int inIdX1 = inIdX0 + (fx != 0);
  int inIdY1 = inIdY0 + (fy != 0);
  int inIdZ1 = inIdZ0 + (fz != 0);

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  if (inIdX0 < 0 || inIdX1 >= inExtX ||
      inIdY0 < 0 || inIdY1 >= inExtY ||
      inIdZ0 < 0 || inIdZ1 >= inExtZ)
    {
    if (interpmode == VTK_RESLICE_BORDER)
      {
      if (vtkInterpolateBorder(inIdX0, inIdX1, inExtX, fx) ||
          vtkInterpolateBorder(inIdY0, inIdY1, inExtY, fy) ||
          vtkInterpolateBorder(inIdZ0, inIdZ1, inExtZ, fz))
        {
        return 0;
        }
      }
    else if (interpmode == VTK_RESLICE_REPEAT)
      {
      vtkInterpolateWrap3( inIdX0, inIdY0, inIdZ0, inExtX, inExtY, inExtZ );
      vtkInterpolateWrap3( inIdX1, inIdY1, inIdZ1, inExtX, inExtY, inExtZ );
      }
    else if (interpmode == VTK_RESLICE_MIRROR)
      {
      vtkInterpolateMirror3( inIdX0, inIdY0, inIdZ0, inExtX, inExtY, inExtZ );
      vtkInterpolateMirror3( inIdX1, inIdY1, inIdZ1, inExtX, inExtY, inExtZ );
      }
    else
      {
      return 0;
      }
    }

  const vtkIdType factX0 = inIdX0*inInc[0];
  const vtkIdType factX1 = inIdX1*inInc[0];
  const vtkIdType factY0 = inIdY0*inInc[1];
  const vtkIdType factY1 = inIdY1*inInc[1];
  const vtkIdType factZ0 = inIdZ0*inInc[2];
  const vtkIdType factZ1 = inIdZ1*inInc[2];

  const vtkIdType i00 = factY0 + factZ0;
  const vtkIdType i01 = factY0 + factZ1;
  const vtkIdType i10 = factY1 + factZ0;
  const vtkIdType i11 = factY1 + factZ1;

  const F rx = 1 - fx;
  const F ry = 1 - fy;
  const F rz = 1 - fz;

  const F ryrz = ry*rz;
  const F fyrz = fy*rz;
  const F ryfz = ry*fz;
  const F fyfz = fy*fz;

  const T *inPtr0 = inPtr + factX0;
  const T *inPtr1 = inPtr + factX1;

  do
    {
    F result = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                    fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                    fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));
    vtkImageSlabResliceBlend( *outPtr, result, blendMode );
    ++outPtr;
    ++inPtr0;
    ++inPtr1;
    }
  while (--numscalars);

  return 1;
}


//----------------------------------------------------------------------------
// Tri-cubic interpolation and blending
// See vtkImageReslice for details. This is nearly the same as that method,
// except that its enhanced to blend the data as well.
//
template <class F, class T>
int vtkImageSlabResliceInterpolate<F, T>::Tricubic(
  F *&outVoidPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int interpmode, int blendMode, const void * /* voidBackground */ )
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  F *outPtr = outVoidPtr;

  F fx, fy, fz;
  int floorX = vtkResliceFloor(point[0], fx);
  int floorY = vtkResliceFloor(point[1], fy);
  int floorZ = vtkResliceFloor(point[2], fz);

  int fxIsNotZero = (fx != 0);
  int fyIsNotZero = (fy != 0);
  int fzIsNotZero = (fz != 0);

  int inIdX0 = floorX - inExt[0];
  int inIdY0 = floorY - inExt[2];
  int inIdZ0 = floorZ - inExt[4];

  int inIdX1 = inIdX0 + fxIsNotZero;
  int inIdY1 = inIdY0 + fyIsNotZero;
  int inIdZ1 = inIdZ0 + fzIsNotZero;

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  vtkIdType inIncX = inInc[0];
  vtkIdType inIncY = inInc[1];
  vtkIdType inIncZ = inInc[2];

  vtkIdType factX[4], factY[4], factZ[4];

  if (inIdX0 < 0 || inIdX1 >= inExtX ||
      inIdY0 < 0 || inIdY1 >= inExtY ||
      inIdZ0 < 0 || inIdZ1 >= inExtZ)
    {
    if (interpmode == VTK_RESLICE_BORDER)
      {
      if (vtkInterpolateBorderCheck(inIdX0, inIdX1, inExtX, fx) ||
          vtkInterpolateBorderCheck(inIdY0, inIdY1, inExtY, fy) ||
          vtkInterpolateBorderCheck(inIdZ0, inIdZ1, inExtZ, fz))
        {
        return 0;
        }
      }
    else if (interpmode != VTK_RESLICE_REPEAT && interpmode != VTK_RESLICE_MIRROR)
      {
      return 0;
      }
    }

  F fX[4], fY[4], fZ[4];
  int k1, k2, j1, j2, i1, i2;

  if (interpmode == VTK_RESLICE_REPEAT || interpmode == VTK_RESLICE_MIRROR)
    {
    i1 = 0;
    i2 = 3;
    vtkTricubicInterpWeights(fX, i1, i2, fx);

    j1 = 1 - fyIsNotZero;
    j2 = 1 + (fyIsNotZero<<1);
    vtkTricubicInterpWeights(fY, j1, j2, fy);

    k1 = 1 - fzIsNotZero;
    k2 = 1 + (fzIsNotZero<<1);
    vtkTricubicInterpWeights(fZ, k1, k2, fz);

    if (interpmode == VTK_RESLICE_REPEAT)
      {
      for (int i = 0; i < 4; i++)
        {
        factX[i] = vtkInterpolateWrap(inIdX0 + i - 1, inExtX)*inIncX;
        factY[i] = vtkInterpolateWrap(inIdY0 + i - 1, inExtY)*inIncY;
        factZ[i] = vtkInterpolateWrap(inIdZ0 + i - 1, inExtZ)*inIncZ;
        }
      }
    else
      {
      for (int i = 0; i < 4; i++)
        {
        factX[i] = vtkInterpolateMirror(inIdX0 + i - 1, inExtX)*inIncX;
        factY[i] = vtkInterpolateMirror(inIdY0 + i - 1, inExtY)*inIncY;
        factZ[i] = vtkInterpolateMirror(inIdZ0 + i - 1, inExtZ)*inIncZ;
        }
      }
    }
  else if (interpmode == VTK_RESLICE_BORDER)
    {
    // clamp to the border of the input extent

    i1 = 1 - fxIsNotZero;
    j1 = 1 - fyIsNotZero;
    k1 = 1 - fzIsNotZero;

    i2 = 1 + 2*fxIsNotZero;
    j2 = 1 + 2*fyIsNotZero;
    k2 = 1 + 2*fzIsNotZero;

    vtkTricubicInterpWeights(fX, i1, i2, fx);
    vtkTricubicInterpWeights(fY, j1, j2, fy);
    vtkTricubicInterpWeights(fZ, k1, k2, fz);

    int tmpExt = inExtX - 1;
    int tmpId = tmpExt - inIdX0 - 1;
    factX[0] = (inIdX0 - 1)*(inIdX0 - 1 >= 0)*inIncX;
    factX[1] = (inIdX0)*(inIdX0 >= 0)*inIncX;
    factX[2] = (tmpExt - (tmpId)*(tmpId >= 0))*inIncX;
    factX[3] = (tmpExt - (tmpId - 1)*(tmpId - 1 >= 0))*inIncX;

    tmpExt = inExtY - 1;
    tmpId = tmpExt - inIdY0 - 1;
    factY[0] = (inIdY0 - 1)*(inIdY0 - 1 >= 0)*inIncY;
    factY[1] = (inIdY0)*(inIdY0 >= 0)*inIncY;
    factY[2] = (tmpExt - (tmpId)*(tmpId >= 0))*inIncY;
    factY[3] = (tmpExt - (tmpId - 1)*(tmpId - 1 >= 0))*inIncY;

    tmpExt = inExtZ - 1;
    tmpId = tmpExt - inIdZ0 - 1;
    factZ[0] = (inIdZ0 - 1)*(inIdZ0 - 1 >= 0)*inIncZ;
    factZ[1] = (inIdZ0)*(inIdZ0 >= 0)*inIncZ;
    factZ[2] = (tmpExt - (tmpId)*(tmpId >= 0))*inIncZ;
    factZ[3] = (tmpExt - (tmpId - 1)*(tmpId - 1 >= 0))*inIncZ;
    }
  else
    {
    // depending on whether we are at the edge of the
    // input extent, choose the appropriate interpolation
    // method to use

    i1 = 1 - (inIdX0 > 0)*fxIsNotZero;
    j1 = 1 - (inIdY0 > 0)*fyIsNotZero;
    k1 = 1 - (inIdZ0 > 0)*fzIsNotZero;

    i2 = 1 + (1 + (inIdX0 + 2 < inExtX))*fxIsNotZero;
    j2 = 1 + (1 + (inIdY0 + 2 < inExtY))*fyIsNotZero;
    k2 = 1 + (1 + (inIdZ0 + 2 < inExtZ))*fzIsNotZero;

    vtkTricubicInterpWeights(fX, i1, i2, fx);
    vtkTricubicInterpWeights(fY, j1, j2, fy);
    vtkTricubicInterpWeights(fZ, k1, k2, fz);

    factX[1] = inIdX0*inIncX;
    factX[0] = factX[1] - inIncX;
    factX[2] = factX[1] + inIncX;
    factX[3] = factX[2] + inIncX;

    factY[1] = inIdY0*inIncY;
    factY[0] = factY[1] - inIncY;
    factY[2] = factY[1] + inIncY;
    factY[3] = factY[2] + inIncY;

    factZ[1] = inIdZ0*inIncZ;
    factZ[0] = factZ[1] - inIncZ;
    factZ[2] = factZ[1] + inIncZ;
    factZ[3] = factZ[2] + inIncZ;

    // this little bit of wierdness allows us to unroll the x loop
    if (i1 > 0)
      {
      factX[0] = factX[1];
      }
    if (i2 < 3)
      {
      factX[3] = factX[1];
      if (i2 < 2)
        {
        factX[2] = factX[1];
        }
      }
    }

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
        const T *tmpPtr = inPtr + factzy;

        vtkImageSlabResliceBlend( val,
                    fzy*(fX[0]*tmpPtr[factX[0]] +
                    fX[1]*tmpPtr[factX[1]] +
                    fX[2]*tmpPtr[factX[2]] +
                    fX[3]*tmpPtr[factX[3]]),
                    blendMode );
        }
      while (++j <= j2);
      }
    while (++k <= k2);

    vtkResliceClamp(val, *outPtr++);
    inPtr++;
    }
  while (--numscalars);

  return 1;
}

//--------------------------------------------------------------------------
// Get the interpolation function to be applied to the data.
template <class F>
void vtkGetImageSlabResliceInterpFunc(vtkImageSlabReslice *self,
                             int (**interpolate)(F *&outPtr,
                                                 const void *inPtr,
                                                 const int inExt[6],
                                                 const vtkIdType inInc[3],
                                                 int numscalars,
                                                 const F point[3],
                                                 int interpMode,
                                                 int blendmode,
                                                 const void *background))
{
  int dataType = self->GetOutput()->GetScalarType();

  int interpolationMode = self->GetInterpolationMode();

  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageSlabResliceInterpolate<F, VTK_TT>::NearestNeighbor)
          );
        default:
          *interpolate = 0;
        }
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageSlabResliceInterpolate<F, VTK_TT>::Trilinear)
          );
        default:
          *interpolate = 0;
        }
      break;
    case VTK_RESLICE_CUBIC:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageSlabResliceInterpolate<F, VTK_TT>::Tricubic)
          );
        default:
          *interpolate = 0;
        }
      break;
    default:
      *interpolate = 0;
    }
}

//----------------------------------------------------------------------------
void vtkImageSlabResliceOptimizedExecute(vtkImageSlabReslice *self,
                         vtkImageData *inData, void *in,
                         vtkImageData *outData, void *out,
                         int outExt[6], int /*id*/, const double newmat[4][4],
                         vtkAbstractTransform *newtrans)
{

  double xAxis[4], yAxis[4], zAxis[4], f, spDen[3], incA[3], incB[3];
  double inPoint[4], inPoint0[4], inPoint1[4];
  double spacing[3], org[3], sp[3], p[3], origin[4];
  int idX, sliceIdx, numEvaluations, nScalars, inExt[6];
  vtkIdType outIncX, outIncY, outIncZ, inInc[3];
  void *background;

  // Interpolation function to be applied to the data.
  int (*interpolate)(double *&out, const void *in,
                     const int inExt[6], const vtkIdType inInc[3],
                     int numscalars, const double point[3],
                     int interp_mode, int blend_mode, const void *background);

  // Function that sets n output pixels (each with numscalars scalar
  // components)
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // Function that sets one pixel with numscalars components. This function
  // does a rounded-casting to the output scalar type if necessary. The reason
  // is that blending is done with double precision. The result is typically
  // casted down to whatever the scalar type of the image is. Rounding is
  // employed to reduce in-accuracy.
  void (*roundcast)(void *&out, const double *in, int numscalars);

  // Get the blend mode.
  const int blendMode = self->GetBlendMode();

  // Get the interpolation mode and wrap flags/
  int interpolationBorderMode, wrap;
  vtkImageSlabResliceGetBorderModes( self, interpolationBorderMode, wrap );

  const bool perspective = (newmat[3][0] != 0 || newmat[3][1] != 0 ||
                            newmat[3][2] != 0 || newmat[3][3] != 1);

  inData->GetExtent(inExt);
  const int numscalars = inData->GetNumberOfScalarComponents();
  const int scalarSize = outData->GetScalarSize();

  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Get function pointers to various functions, depending on user modes.
  vtkGetImageSlabResliceInterpFunc(self, &interpolate);
  vtkGetSetPixelsFunc(self, &setpixels);
  vtkGetCastPixelsFunc(self, &roundcast);

  // default value assigned to voxels that are mapped outside the buffer
  vtkAllocBackgroundPixel(self, &background, numscalars);


  // Get reslice axes. Compute the direction along which blending is done
  // from the direction consines. This is simply the third row.
  vtkMatrix4x4 *resliceAxes = self->GetResliceAxes();
  const double blendDirection[3] =
      { resliceAxes->GetElement(2,0),
        resliceAxes->GetElement(2,1),
        resliceAxes->GetElement(2,2) };

  inData->GetOrigin(org);
  inData->GetSpacing(sp);
  outData->GetSpacing(spacing);

  // Number of slices blended on either side of the central slice.
  const int slices = (int)(self->GetNumBlendSamplePoints()/2);

  // Do some precomputations here for use later.
  for (int i = 0; i < 3; i++)
    {
    spDen[i] = 1.0/sp[i];
    incA[i] = blendDirection[i] * self->GetSlabResolution() * spDen[i];
    incB[i] = incA[i] * (double)slices;
    }


  for (int i = 0; i < 4; ++i)
    {
    xAxis[i]  = newmat[i][0];
    yAxis[i]  = newmat[i][1];
    zAxis[i]  = newmat[i][2];
    origin[i] = newmat[i][3];
    inPoint0[i] = origin[i] + outExt[4]*zAxis[i];
    }

  // Placeholder for the interpolated values. These are blended using
  // across the slab for each voxel.
  double *pixel = new double[numscalars];

  // Increment of a full scan line along Y.
  const vtkIdType outYScanLineInc = outIncY * scalarSize;

  // Number of blend points.
  const int numEvaluationsMax = 2 * slices + 1;

  for ( int idY = outExt[2]; idY <= outExt[3]; ++idY )
    {

    for (int j = 0; j < 4; ++j)
      {
      inPoint1[j] = inPoint0[j] + idY*yAxis[j];
      }

    for (idX = outExt[0]; idX <= outExt[1]; idX++)
      {

      inPoint[0] = inPoint1[0] + idX*xAxis[0] - incB[0];
      inPoint[1] = inPoint1[1] + idX*xAxis[1] - incB[1];
      inPoint[2] = inPoint1[2] + idX*xAxis[2] - incB[2];

      if (perspective)
        {
        f = 1.0 / (inPoint1[3] + idX*xAxis[3]);
        }

      // Number of summations along the blendDirection. This can be any number from 0
      // to self->NumBlendSamplePoints, depending on whether all samples along the
      // blendDirection lie within the volume or not (accounting for whether the wrap
      // interpolationBorderMode etc is set). The averaging is done based on 
      // the number of summations along the ray. If numEvaluations is 0, ie
      // all points along the ray lie outside, then the value is set to the
      // background pixel.
      numEvaluations = 0;

      // Reset the sum to 0 for all components before blending
      for (nScalars = 0; nScalars < numscalars; nScalars++)
        {
        pixel[nScalars] = 0;
        }

      // Blend across the slab

      for (sliceIdx = 0; sliceIdx < numEvaluationsMax; ++sliceIdx)
        {

        p[0] = inPoint[0];
        p[1] = inPoint[1];
        p[2] = inPoint[2];

        if (perspective)
          {
          vtkMath::MultiplyScalar(p,f);
          p[3] = inPoint1[3];
          }

        // Optionally apply transform
        vtkResliceApplyTransform(newtrans, p, org, spDen);


        // interplate and if the pixel lies within, the numEvaluations is
        // incremented by 1.

        numEvaluations += interpolate(pixel, in, inExt,
                      inInc, numscalars, p,
                      interpolationBorderMode, blendMode, background);


        // Increment the point along the blendDirection.
        vtkImageResliceIncrement( inPoint, incA );
        }


      if (numEvaluations != 0)
        {

        // If the blend mode is mean, we need to take the mean of the computed
        // samples and divide by the number of samples that lie within the slab
        // bounding box. The numEvaluations is generally the same as the
        // number of slices, except in cases where a portion of it lies outside
        // the volume. For the other blending functions (MIN, MAX),
        // pixel should already contain the final values.

        if (blendMode == VTK_IMAGESLAB_BLEND_MEAN)
          {
          for (nScalars = 0; nScalars < numscalars; ++nScalars)
            {
            pixel[nScalars] /= ((double)(numEvaluations));
            }
          }

        // Round the double value to the output and increment it along X.
        roundcast( out, pixel, numscalars );
        }
      else
        {
        // Fill with default values
        setpixels( out, background, numscalars, 1 );
        }
      }

    // increment
    out = (void*)(((char *)(out)) + outYScanLineInc);
    }

  // free
  vtkFreeBackgroundPixel(self, &background);
  delete pixel;
}

//----------------------------------------------------------------------------
void vtkImageSlabReslice::InternalThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  if (inData[0][0]->GetScalarType() == outData[0]->GetScalarType())
    {

    // Get the output pointer
    void *inPtr = inData[0][0]->GetScalarPointerForExtent(inData[0][0]->GetExtent());
    void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

    vtkImageSlabResliceOptimizedExecute(
        this, inData[0][0], inPtr, outData[0], outPtr,
        outExt, id, this->IndexMatrix->Element, this->OptimizedTransform);

    }
  else
    {
    vtkErrorMacro(<< "Scalar types do not match" );
    }
}

//----------------------------------------------------------------------------
void vtkImageSlabReslice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Blend mode: " <<  this->BlendMode << endl;
  os << indent << "SlabResolution (world units): " << this->SlabResolution << endl;
  os << indent << "SlabThickness (world units): " << this->SlabThickness << endl;
  os << indent << "Max Number of slices blended: "
     << this->NumBlendSamplePoints << endl;
}
