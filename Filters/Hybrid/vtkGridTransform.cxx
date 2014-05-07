/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGridTransform.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkImageInterpolatorInternals.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"

#include <math.h>

vtkStandardNewMacro(vtkGridTransform);

class vtkGridTransformConnectionHolder: public vtkAlgorithm
{
public:
  static vtkGridTransformConnectionHolder *New();
  vtkTypeMacro(vtkGridTransformConnectionHolder,vtkAlgorithm);

  vtkGridTransformConnectionHolder()
    {
      this->SetNumberOfInputPorts(1);
    }
};

vtkStandardNewMacro(vtkGridTransformConnectionHolder);

//----------------------------------------------------------------------------
// Nearest-neighbor interpolation of a displacement grid.
// The displacement as well as the derivatives are returned.
// There are two versions: one which computes the derivatives,
// and one which doesn't.

template <class T>
inline void vtkNearestHelper(double displacement[3], T *gridPtr, int increment)
{
  gridPtr += increment;
  displacement[0] = gridPtr[0];
  displacement[1] = gridPtr[1];
  displacement[2] = gridPtr[2];
}

inline void vtkNearestNeighborInterpolation(double point[3],
                                            double displacement[3],
                                            void *gridPtr, int gridType,
                                            int gridExt[6],
                                            vtkIdType gridInc[3])
{
  int gridId[3];
  gridId[0] = vtkInterpolationMath::Round(point[0])-gridExt[0];
  gridId[1] = vtkInterpolationMath::Round(point[1])-gridExt[2];
  gridId[2] = vtkInterpolationMath::Round(point[2])-gridExt[4];

  int ext[3];
  ext[0] = gridExt[1]-gridExt[0];
  ext[1] = gridExt[3]-gridExt[2];
  ext[2] = gridExt[5]-gridExt[4];

  // do bounds check, most points will be inside so optimize for that
  if ((gridId[0] | (ext[0] - gridId[0]) |
       gridId[1] | (ext[1] - gridId[1]) |
       gridId[2] | (ext[2] - gridId[2])) < 0)
    {
    for (int i = 0; i < 3; i++)
      {
      if (gridId[i] < 0)
        {
        gridId[i] = 0;
        }
      else if (gridId[i] > ext[i])
        {
        gridId[i] = ext[i];
        }
      }
    }

  // do nearest-neighbor interpolation
  vtkIdType increment = gridId[0]*gridInc[0] +
                        gridId[1]*gridInc[1] +
                        gridId[2]*gridInc[2];

  switch (gridType)
    {
    vtkTemplateMacro(
      vtkNearestHelper(displacement, static_cast<VTK_TT*>(gridPtr), increment));
    }
}

template <class T>
inline void vtkNearestHelper(double displacement[3], double derivatives[3][3],
                             T *gridPtr, int gridId[3], int gridId0[3],
                             int gridId1[3], vtkIdType gridInc[3])
{
  vtkIdType incX = gridId[0]*gridInc[0];
  vtkIdType incY = gridId[1]*gridInc[1];
  vtkIdType incZ = gridId[2]*gridInc[2];

  T *gridPtr0;
  T *gridPtr1 = gridPtr + incX + incY + incZ;

  displacement[0] = gridPtr1[0];
  displacement[1] = gridPtr1[1];
  displacement[2] = gridPtr1[2];

  vtkIdType incX0 = gridId0[0]*gridInc[0];
  vtkIdType incX1 = gridId1[0]*gridInc[0];
  vtkIdType incY0 = gridId0[1]*gridInc[1];

  vtkIdType incY1 = gridId1[1]*gridInc[1];
  vtkIdType incZ0 = gridId0[2]*gridInc[2];
  vtkIdType incZ1 = gridId1[2]*gridInc[2];

  gridPtr0 = gridPtr + incX0 + incY + incZ;
  gridPtr1 = gridPtr + incX1 + incY + incZ;

  derivatives[0][0] = gridPtr1[0] - gridPtr0[0];
  derivatives[1][0] = gridPtr1[1] - gridPtr0[1];
  derivatives[2][0] = gridPtr1[2] - gridPtr0[2];

  gridPtr0 = gridPtr + incX + incY0 + incZ;
  gridPtr1 = gridPtr + incX + incY1 + incZ;

  derivatives[0][1] = gridPtr1[0] - gridPtr0[0];
  derivatives[1][1] = gridPtr1[1] - gridPtr0[1];
  derivatives[2][1] = gridPtr1[2] - gridPtr0[2];

  gridPtr0 = gridPtr + incX + incY + incZ0;
  gridPtr1 = gridPtr + incX + incY + incZ1;

  derivatives[0][2] = gridPtr1[0] - gridPtr0[0];
  derivatives[1][2] = gridPtr1[1] - gridPtr0[1];
  derivatives[2][2] = gridPtr1[2] - gridPtr0[2];
}

static void vtkNearestNeighborInterpolation(double point[3], double displacement[3],
                                            double derivatives[3][3], void *gridPtr,
                                            int gridType, int gridExt[6],
                                            vtkIdType gridInc[3])
{
  if (derivatives == NULL)
    {
    vtkNearestNeighborInterpolation(point,displacement,gridPtr,gridType,
                                    gridExt,gridInc);
    return;
    }

  double f[3];
  int gridId0[3];
  gridId0[0] = vtkInterpolationMath::Floor(point[0],f[0])-gridExt[0];
  gridId0[1] = vtkInterpolationMath::Floor(point[1],f[1])-gridExt[2];
  gridId0[2] = vtkInterpolationMath::Floor(point[2],f[2])-gridExt[4];

  int gridId[3], gridId1[3];
  gridId[0] = gridId1[0] = gridId0[0] + 1;
  gridId[1] = gridId1[1] = gridId0[1] + 1;
  gridId[2] = gridId1[2] = gridId0[2] + 1;

  if (f[0] < 0.5)
    {
    gridId[0] = gridId0[0];
    }
  if (f[1] < 0.5)
    {
    gridId[1] = gridId0[1];
    }
  if (f[2] < 0.5)
    {
    gridId[2] = gridId0[2];
    }

  int ext[3];
  ext[0] = gridExt[1] - gridExt[0];
  ext[1] = gridExt[3] - gridExt[2];
  ext[2] = gridExt[5] - gridExt[4];

  // do bounds check, most points will be inside so optimize for that
  if ((gridId0[0] | (ext[0] - gridId1[0]) |
       gridId0[1] | (ext[1] - gridId1[1]) |
       gridId0[2] | (ext[2] - gridId1[2])) < 0)
    {
    for (int i = 0; i < 3; i++)
      {
      if (gridId0[i] < 0)
        {
        gridId[i] = 0;
        gridId0[i] = 0;
        gridId1[i] = 0;
        }
      else if (gridId1[i] > ext[i])
        {
        gridId[i] = ext[i];
        gridId0[i] = ext[i];
        gridId1[i] = ext[i];
        }
      }
    }

  // do nearest-neighbor interpolation
  switch (gridType)
    {
    vtkTemplateMacro(
      vtkNearestHelper(displacement, derivatives, static_cast<VTK_TT*>(gridPtr),
                       gridId, gridId0, gridId1, gridInc));
    }
}

//----------------------------------------------------------------------------
// Trilinear interpolation of a displacement grid.
// The displacement as well as the derivatives are returned.

template <class T>
inline void vtkLinearHelper(double displacement[3], double derivatives[3][3],
                            double fx, double fy, double fz, T *gridPtr,
                            int i000, int i001, int i010, int i011,
                            int i100, int i101, int i110, int i111)
{
  double rx = 1 - fx;
  double ry = 1 - fy;
  double rz = 1 - fz;

  double ryrz = ry*rz;
  double ryfz = ry*fz;
  double fyrz = fy*rz;
  double fyfz = fy*fz;

  double rxryrz = rx*ryrz;
  double rxryfz = rx*ryfz;
  double rxfyrz = rx*fyrz;
  double rxfyfz = rx*fyfz;
  double fxryrz = fx*ryrz;
  double fxryfz = fx*ryfz;
  double fxfyrz = fx*fyrz;
  double fxfyfz = fx*fyfz;

  if (!derivatives)
    {
    int i = 3;
    do
      {
      *displacement++ = (rxryrz*gridPtr[i000] + rxryfz*gridPtr[i001] +
                         rxfyrz*gridPtr[i010] + rxfyfz*gridPtr[i011] +
                         fxryrz*gridPtr[i100] + fxryfz*gridPtr[i101] +
                         fxfyrz*gridPtr[i110] + fxfyfz*gridPtr[i111]);
      gridPtr++;
      }
    while (--i);
    }
  else
    {
    double rxrz = rx*rz;
    double rxfz = rx*fz;
    double fxrz = fx*rz;
    double fxfz = fx*fz;

    double rxry = rx*ry;
    double rxfy = rx*fy;
    double fxry = fx*ry;
    double fxfy = fx*fy;

    double *derivative = *derivatives;

    int i = 3;
    do
      {
      *displacement++ = (rxryrz*gridPtr[i000] + rxryfz*gridPtr[i001] +
                         rxfyrz*gridPtr[i010] + rxfyfz*gridPtr[i011] +
                         fxryrz*gridPtr[i100] + fxryfz*gridPtr[i101] +
                         fxfyrz*gridPtr[i110] + fxfyfz*gridPtr[i111]);

      *derivative++ = (ryrz*(gridPtr[i100] - gridPtr[i000]) +
                       ryfz*(gridPtr[i101] - gridPtr[i001]) +
                       fyrz*(gridPtr[i110] - gridPtr[i010]) +
                       fyfz*(gridPtr[i111] - gridPtr[i011]));

      *derivative++ = (rxrz*(gridPtr[i010] - gridPtr[i000]) +
                       rxfz*(gridPtr[i011] - gridPtr[i001]) +
                       fxrz*(gridPtr[i110] - gridPtr[i100]) +
                       fxfz*(gridPtr[i111] - gridPtr[i101]));

      *derivative++ = (rxry*(gridPtr[i001] - gridPtr[i000]) +
                       rxfy*(gridPtr[i011] - gridPtr[i010]) +
                       fxry*(gridPtr[i101] - gridPtr[i100]) +
                       fxfy*(gridPtr[i111] - gridPtr[i110]));

      gridPtr++;
      }
    while (--i);
    }
}

static void vtkTrilinearInterpolation(double point[3], double displacement[3],
                                      double derivatives[3][3], void *gridPtr, int gridType,
                                      int gridExt[6], vtkIdType gridInc[3])
{
  // change point into integer plus fraction
  double f[3];
  int floorX = vtkInterpolationMath::Floor(point[0],f[0]);
  int floorY = vtkInterpolationMath::Floor(point[1],f[1]);
  int floorZ = vtkInterpolationMath::Floor(point[2],f[2]);

  int gridId0[3];
  gridId0[0] = floorX - gridExt[0];
  gridId0[1] = floorY - gridExt[2];
  gridId0[2] = floorZ - gridExt[4];

  int gridId1[3];
  gridId1[0] = gridId0[0] + 1;
  gridId1[1] = gridId0[1] + 1;
  gridId1[2] = gridId0[2] + 1;

  int ext[3];
  ext[0] = gridExt[1] - gridExt[0];
  ext[1] = gridExt[3] - gridExt[2];
  ext[2] = gridExt[5] - gridExt[4];

  // do bounds check, most points will be inside so optimize for that
  if ((gridId0[0] | (ext[0] - gridId1[0]) |
       gridId0[1] | (ext[1] - gridId1[1]) |
       gridId0[2] | (ext[2] - gridId1[2])) < 0)
    {
    for (int i = 0; i < 3; i++)
      {
      if (gridId0[i] < 0)
        {
        gridId0[i] = 0;
        gridId1[i] = 0;
        f[i] = 0;
        }
      else if (gridId1[i] > ext[i])
        {
        gridId0[i] = ext[i];
        gridId1[i] = ext[i];
        f[i] = 0;
        }
      }
    }

  // do trilinear interpolation
  vtkIdType factX0 = gridId0[0]*gridInc[0];
  vtkIdType factY0 = gridId0[1]*gridInc[1];
  vtkIdType factZ0 = gridId0[2]*gridInc[2];

  vtkIdType factX1 = gridId1[0]*gridInc[0];
  vtkIdType factY1 = gridId1[1]*gridInc[1];
  vtkIdType factZ1 = gridId1[2]*gridInc[2];

  vtkIdType i000 = factX0+factY0+factZ0;
  vtkIdType i001 = factX0+factY0+factZ1;
  vtkIdType i010 = factX0+factY1+factZ0;
  vtkIdType i011 = factX0+factY1+factZ1;
  vtkIdType i100 = factX1+factY0+factZ0;
  vtkIdType i101 = factX1+factY0+factZ1;
  vtkIdType i110 = factX1+factY1+factZ0;
  vtkIdType i111 = factX1+factY1+factZ1;

  switch (gridType)
    {
    vtkTemplateMacro(vtkLinearHelper(displacement, derivatives, f[0], f[1], f[2],
        static_cast<VTK_TT*>(gridPtr),
        i000, i001, i010, i011, i100, i101, i110, i111));
    }
}

//----------------------------------------------------------------------------
// Do tricubic interpolation of the input data 'gridPtr' of extent 'gridExt'
// at the 'point'.  The result is placed at 'outPtr'.
// The number of scalar components in the data is 'numscalars'

// The tricubic interpolation ensures that both the intensity and
// the first derivative of the intensity are smooth across the
// image.  The first derivative is estimated using a
// centered-difference calculation.


// helper function: set up the lookup indices and the interpolation
// coefficients

static void vtkSetTricubicInterpCoeffs(double F[4], int *l, int *m, double f,
                                       int interpMode)
{
  double fp1,fm1,fm2;

  switch (interpMode)
    {
    case 7:     // cubic interpolation
      *l = 0; *m = 4;
      fm1 = f-1;
      F[0] = -f*fm1*fm1/2;
      F[1] = ((3*f-2)*f-2)*fm1/2;
      F[2] = -((3*f-4)*f-1)*f/2;
      F[3] = f*f*fm1/2;
      break;
    case 0:     // no interpolation
    case 2:
    case 4:
    case 6:
      *l = 1; *m = 2;
      F[0] = 0;
      F[1] = 1;
      F[2] = 0;
      F[3] = 0;
      break;
    case 1:     // linear interpolation
      *l = 1; *m = 3;
      F[0] = 0;
      F[1] = 1-f;
      F[2] = f;
      F[3] = 0;
      break;
    case 3:     // quadratic interpolation
      *l = 1; *m = 4;
      fm1 = f-1; fm2 = fm1-1;
      F[0] = 0;
      F[1] = fm1*fm2/2;
      F[2] = -f*fm2;
      F[3] = f*fm1/2;
      break;
    case 5:     // quadratic interpolation
      *l = 0; *m = 3;
      fp1 = f+1; fm1 = f-1;
      F[0] = f*fm1/2;
      F[1] = -fp1*fm1;
      F[2] = fp1*f/2;
      F[3] = 0;
      break;
    }
}

// set coefficients to be used to find the derivative of the cubic
static void vtkSetTricubicDerivCoeffs(double F[4], double G[4], int *l, int *m,
                                      double f, int interpMode)
{
  double fp1,fm1,fm2;

  switch (interpMode)
    {
    case 7:     // cubic interpolation
      *l = 0; *m = 4;
      fm1 = f-1;
      F[0] = -f*fm1*fm1/2;
      F[1] = ((3*f-2)*f-2)*fm1/2;
      F[2] = -((3*f-4)*f-1)*f/2;
      F[3] = f*f*fm1/2;
      G[0] = -((3*f-4)*f+1)/2;
      G[1] =  (9*f-10)*f/2;
      G[2] = -((9*f-8)*f-1)/2;
      G[3] =  (3*f-2)*f/2;
      break;
    case 0:     // no interpolation
    case 2:
    case 4:
    case 6:
      *l = 1; *m = 2;
      F[0] = 0;
      F[1] = 1;
      F[2] = 0;
      F[3] = 0;
      G[0] = 0;
      G[1] = 0;
      G[2] = 0;
      G[3] = 0;
      break;
    case 1:     // linear interpolation
      *l = 1; *m = 3;
      F[0] = 0;
      F[1] = 1-f;
      F[2] = f;
      F[3] = 0;
      G[0] =  0;
      G[1] = -1;
      G[2] =  1;
      G[3] =  0;
      break;
    case 3:     // quadratic interpolation
      *l = 1; *m = 4;
      fm1 = f-1; fm2 = fm1-1;
      F[0] = 0;
      F[1] = fm1*fm2/2;
      F[2] = -f*fm2;
      F[3] = f*fm1/2;
      G[0] = 0;
      G[1] = f-1.5;
      G[2] = 2-2*f;
      G[3] = f-0.5;
      break;
    case 5:     // quadratic interpolation
      *l = 0; *m = 3;
      fp1 = f+1; fm1 = f-1;
      F[0] = f*fm1/2;
      F[1] = -fp1*fm1;
      F[2] = fp1*f/2;
      F[3] = 0;
      G[0] = f-0.5;
      G[1] = -2*f;
      G[2] = f+0.5;
      G[3] = 0;
      break;
    }
}

// tricubic interpolation of a warp grid with derivatives
// (set derivatives to NULL to avoid computing them).

template <class T>
inline void vtkCubicHelper(double displacement[3], double derivatives[3][3],
                           double fx, double fy, double fz, T *gridPtr,
                           int interpModeX, int interpModeY, int interpModeZ,
                           vtkIdType factX[4], vtkIdType factY[4],
                           vtkIdType factZ[4])
{
  double fX[4],fY[4],fZ[4];
  double gX[4],gY[4],gZ[4];
  int jl,jm,kl,km,ll,lm;

  if (derivatives)
    {
    for (int i = 0; i < 3; i++)
      {
      derivatives[i][0] = 0.0;
      derivatives[i][1] = 0.0;
      derivatives[i][2] = 0.0;
      }
    vtkSetTricubicDerivCoeffs(fX,gX,&ll,&lm,fx,interpModeX);
    vtkSetTricubicDerivCoeffs(fY,gY,&kl,&km,fy,interpModeY);
    vtkSetTricubicDerivCoeffs(fZ,gZ,&jl,&jm,fz,interpModeZ);
    }
  else
    {
    vtkSetTricubicInterpCoeffs(fX,&ll,&lm,fx,interpModeX);
    vtkSetTricubicInterpCoeffs(fY,&kl,&km,fy,interpModeY);
    vtkSetTricubicInterpCoeffs(fZ,&jl,&jm,fz,interpModeZ);
    }

  // Here is the tricubic interpolation
  // (or cubic-cubic-linear, or cubic-nearest-cubic, etc)
  double vY[3],vZ[3];
  displacement[0] = 0;
  displacement[1] = 0;
  displacement[2] = 0;
  for (int j = jl; j < jm; j++)
    {
    T *gridPtr1 = gridPtr + factZ[j];
    vZ[0] = 0;
    vZ[1] = 0;
    vZ[2] = 0;
    for (int k = kl; k < km; k++)
      {
      T *gridPtr2 = gridPtr1 + factY[k];
      vY[0] = 0;
      vY[1] = 0;
      vY[2] = 0;
      if (!derivatives)
        {
        for (int l = ll; l < lm; l++)
          {
          T *gridPtr3 = gridPtr2 + factX[l];
          double f = fX[l];
          vY[0] += gridPtr3[0] * f;
          vY[1] += gridPtr3[1] * f;
          vY[2] += gridPtr3[2] * f;
          }
        }
      else
        {
        for (int l = ll; l < lm; l++)
          {
          T *gridPtr3 = gridPtr2 + factX[l];
          double f = fX[l];
          double gff = gX[l]*fY[k]*fZ[j];
          double fgf = fX[l]*gY[k]*fZ[j];
          double ffg = fX[l]*fY[k]*gZ[j];
          double inVal = gridPtr3[0];
          vY[0] += inVal * f;
          derivatives[0][0] += inVal * gff;
          derivatives[0][1] += inVal * fgf;
          derivatives[0][2] += inVal * ffg;
          inVal = gridPtr3[1];
          vY[1] += inVal * f;
          derivatives[1][0] += inVal * gff;
          derivatives[1][1] += inVal * fgf;
          derivatives[1][2] += inVal * ffg;
          inVal = gridPtr3[2];
          vY[2] += inVal * f;
          derivatives[2][0] += inVal * gff;
          derivatives[2][1] += inVal * fgf;
          derivatives[2][2] += inVal * ffg;
          }
        }
        vZ[0] += vY[0]*fY[k];
        vZ[1] += vY[1]*fY[k];
        vZ[2] += vY[2]*fY[k];
      }
    displacement[0] += vZ[0]*fZ[j];
    displacement[1] += vZ[1]*fZ[j];
    displacement[2] += vZ[2]*fZ[j];
    }
}

static void vtkTricubicInterpolation(double point[3], double displacement[3],
                                     double derivatives[3][3], void *gridPtr,
                                     int gridType, int gridExt[6],
                                     vtkIdType gridInc[3])
{
  vtkIdType factX[4],factY[4],factZ[4];

  // change point into integer plus fraction
  double f[3];
  int floorX = vtkInterpolationMath::Floor(point[0],f[0]);
  int floorY = vtkInterpolationMath::Floor(point[1],f[1]);
  int floorZ = vtkInterpolationMath::Floor(point[2],f[2]);

  int gridId0[3];
  gridId0[0] = floorX - gridExt[0];
  gridId0[1] = floorY - gridExt[2];
  gridId0[2] = floorZ - gridExt[4];

  int gridId1[3];
  gridId1[0] = gridId0[0] + 1;
  gridId1[1] = gridId0[1] + 1;
  gridId1[2] = gridId0[2] + 1;

  int ext[3];
  ext[0] = gridExt[1] - gridExt[0];
  ext[1] = gridExt[3] - gridExt[2];
  ext[2] = gridExt[5] - gridExt[4];

  // the doInterpX,Y,Z variables are 0 if interpolation
  // does not have to be done in the specified direction.
  int doInterp[3];
  doInterp[0] = 1;
  doInterp[1] = 1;
  doInterp[2] = 1;

  // do bounds check, most points will be inside so optimize for that
  if ((gridId0[0] | (ext[0] - gridId1[0]) |
       gridId0[1] | (ext[1] - gridId1[1]) |
       gridId0[2] | (ext[2] - gridId1[2])) < 0)
    {
    for (int i = 0; i < 3; i++)
      {
      if (gridId0[i] < 0)
        {
        gridId0[i] = 0;
        gridId1[i] = 0;
        doInterp[i] = 0;
        f[i] = 0;
        }
      else if (gridId1[i] > ext[i])
        {
        gridId0[i] = ext[i];
        gridId1[i] = ext[i];
        doInterp[i] = 0;
        f[i] = 0;
        }
      }
    }

  // do tricubic interpolation

  for (int i = 0; i < 4; i++)
    {
    factX[i] = (gridId0[0]-1+i)*gridInc[0];
    factY[i] = (gridId0[1]-1+i)*gridInc[1];
    factZ[i] = (gridId0[2]-1+i)*gridInc[2];
    }

  // depending on whether we are at the edge of the
  // input extent, choose the appropriate interpolation
  // method to use

  int interpModeX = ((gridId0[0] > 0) << 2) +
                    ((gridId1[0] < ext[0]) << 1) +
                    doInterp[0];
  int interpModeY = ((gridId0[1] > 0) << 2) +
                    ((gridId1[1] < ext[1]) << 1) +
                    doInterp[1];
  int interpModeZ = ((gridId0[2] > 0) << 2) +
                    ((gridId1[2] < ext[2]) << 1) +
                    doInterp[2];

  switch (gridType)
    {
    vtkTemplateMacro(
      vtkCubicHelper(displacement, derivatives, f[0], f[1], f[2],
                     static_cast<VTK_TT*>(gridPtr),
                     interpModeX, interpModeY, interpModeZ,
                     factX, factY, factZ));
    }
}

//----------------------------------------------------------------------------
vtkGridTransform::vtkGridTransform()
{
  this->InterpolationMode = VTK_LINEAR_INTERPOLATION;
  this->InterpolationFunction = &vtkTrilinearInterpolation;
  this->DisplacementScale = 1.0;
  this->DisplacementShift = 0.0;
  this->GridPointer = 0;
  // the grid warp has a fairly large tolerance
  this->InverseTolerance = 0.01;

  this->ConnectionHolder = vtkGridTransformConnectionHolder::New();
}

//----------------------------------------------------------------------------
vtkGridTransform::~vtkGridTransform()
{
  this->ConnectionHolder->Delete();
  this->ConnectionHolder = 0;
}

//----------------------------------------------------------------------------
void vtkGridTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InterpolationMode: "
     << this->GetInterpolationModeAsString() << "\n";
  os << indent << "DisplacementScale: " << this->DisplacementScale << "\n";
  os << indent << "DisplacementShift: " << this->DisplacementShift << "\n";
}

//----------------------------------------------------------------------------
// need to check the input image data to determine MTime
unsigned long vtkGridTransform::GetMTime()
{
  unsigned long mtime,result;
  result = vtkWarpTransform::GetMTime();
  if (this->GetDisplacementGrid())
    {
    vtkAlgorithm* inputAlgorithm =
      this->ConnectionHolder->GetInputAlgorithm(0, 0);
    inputAlgorithm->UpdateInformation();

    vtkStreamingDemandDrivenPipeline* sddp =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(
        inputAlgorithm->GetExecutive());
    mtime = sddp->GetPipelineMTime();
    result = ( mtime > result ? mtime : result );
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkGridTransform::SetInterpolationMode(int mode)
{
  if (mode == this->InterpolationMode)
    {
    return;
    }
  this->InterpolationMode = mode;
  switch(mode)
    {
    case VTK_NEAREST_INTERPOLATION:
      this->InterpolationFunction = &vtkNearestNeighborInterpolation;
      break;
    case VTK_LINEAR_INTERPOLATION:
      this->InterpolationFunction = &vtkTrilinearInterpolation;
      break;
    case VTK_CUBIC_INTERPOLATION:
      this->InterpolationFunction = &vtkTricubicInterpolation;
      break;
    default:
      vtkErrorMacro( << "SetInterpolationMode: Illegal interpolation mode");
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGridTransform::ForwardTransformPoint(const double inPoint[3],
                                             double outPoint[3])
{
  if (!this->GridPointer)
    {
    outPoint[0] = inPoint[0];
    outPoint[1] = inPoint[1];
    outPoint[2] = inPoint[2];
    return;
    }

  void *gridPtr = this->GridPointer;
  int gridType = this->GridScalarType;

  double *spacing = this->GridSpacing;
  double *origin = this->GridOrigin;
  int *extent = this->GridExtent;
  vtkIdType *increments = this->GridIncrements;

  double scale = this->DisplacementScale;
  double shift = this->DisplacementShift;

  double point[3];
  double displacement[3];

  // Convert the inPoint to i,j,k indices into the deformation grid
  // plus fractions
  point[0] = (inPoint[0] - origin[0])/spacing[0];
  point[1] = (inPoint[1] - origin[1])/spacing[1];
  point[2] = (inPoint[2] - origin[2])/spacing[2];

  this->InterpolationFunction(point,displacement,NULL,
                              gridPtr,gridType,extent,increments);

  outPoint[0] = inPoint[0] + (displacement[0]*scale + shift);
  outPoint[1] = inPoint[1] + (displacement[1]*scale + shift);
  outPoint[2] = inPoint[2] + (displacement[2]*scale + shift);
}

//----------------------------------------------------------------------------
// convert double to double
void vtkGridTransform::ForwardTransformPoint(const float point[3],
                                             float output[3])
{
  double fpoint[3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->ForwardTransformPoint(fpoint,fpoint);

  output[0] = static_cast<float>(fpoint[0]);
  output[1] = static_cast<float>(fpoint[1]);
  output[2] = static_cast<float>(fpoint[2]);
}

//----------------------------------------------------------------------------
// calculate the derivative of the grid transform: only cubic interpolation
// provides well-behaved derivative so we always use that.
void vtkGridTransform::ForwardTransformDerivative(const double inPoint[3],
                                                  double outPoint[3],
                                                  double derivative[3][3])
{
  if (!this->GridPointer)
    {
    outPoint[0] = inPoint[0];
    outPoint[1] = inPoint[1];
    outPoint[2] = inPoint[2];
    vtkMath::Identity3x3(derivative);
    return;
    }

  void *gridPtr = this->GridPointer;
  int gridType = this->GridScalarType;

  double *spacing = this->GridSpacing;
  double *origin = this->GridOrigin;
  int *extent = this->GridExtent;
  vtkIdType *increments = this->GridIncrements;

  double scale = this->DisplacementScale;
  double shift = this->DisplacementShift;

  double point[3];
  double displacement[3];

  // convert the inPoint to i,j,k indices plus fractions
  point[0] = (inPoint[0] - origin[0])/spacing[0];
  point[1] = (inPoint[1] - origin[1])/spacing[1];
  point[2] = (inPoint[2] - origin[2])/spacing[2];

  this->InterpolationFunction(point,displacement,derivative,
                              gridPtr,gridType,extent,increments);

  for (int i = 0; i < 3; i++)
    {
    derivative[i][0] = derivative[i][0]*scale/spacing[0];
    derivative[i][1] = derivative[i][1]*scale/spacing[1];
    derivative[i][2] = derivative[i][2]*scale/spacing[2];
    derivative[i][i] += 1.0;
    }

  outPoint[0] = inPoint[0] + (displacement[0]*scale + shift);
  outPoint[1] = inPoint[1] + (displacement[1]*scale + shift);
  outPoint[2] = inPoint[2] + (displacement[2]*scale + shift);
}

//----------------------------------------------------------------------------
// convert double to double
void vtkGridTransform::ForwardTransformDerivative(const float point[3],
                                                  float output[3],
                                                  float derivative[3][3])
{
  double fpoint[3];
  double fderivative[3][3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->ForwardTransformDerivative(fpoint,fpoint,fderivative);

  for (int i = 0; i < 3; i++)
    {
    derivative[i][0] = static_cast<float>(fderivative[i][0]);
    derivative[i][1] = static_cast<float>(fderivative[i][1]);
    derivative[i][2] = static_cast<float>(fderivative[i][2]);
    output[i] = static_cast<float>(fpoint[i]);
    }
}

//----------------------------------------------------------------------------
// We use Newton's method to iteratively invert the transformation.
// This is actally quite robust as long as the Jacobian matrix is never
// singular.
// Note that this is similar to vtkWarpTransform::InverseTransformPoint()
// but has been optimized specifically for grid transforms.
void vtkGridTransform::InverseTransformDerivative(const double inPoint[3],
                                                  double outPoint[3],
                                                  double derivative[3][3])
{
  if (!this->GridPointer)
    {
    outPoint[0] = inPoint[0];
    outPoint[1] = inPoint[1];
    outPoint[2] = inPoint[2];
    return;
    }

  void *gridPtr = this->GridPointer;
  int gridType = this->GridScalarType;

  double *spacing = this->GridSpacing;
  double *origin = this->GridOrigin;
  int *extent = this->GridExtent;
  vtkIdType *increments = this->GridIncrements;

  double invSpacing[3];
  invSpacing[0] = 1.0/spacing[0];
  invSpacing[1] = 1.0/spacing[1];
  invSpacing[2] = 1.0/spacing[2];

  double shift = this->DisplacementShift;
  double scale = this->DisplacementScale;

  double point[3], inverse[3], lastInverse[3];
  double deltaP[3], deltaI[3];

  double functionValue = 0;
  double functionDerivative = 0;
  double lastFunctionValue = VTK_DOUBLE_MAX;

  double errorSquared = 0.0;
  double toleranceSquared = this->InverseTolerance;
  toleranceSquared *= toleranceSquared;

  double f = 1.0;
  double a;

  // convert the inPoint to i,j,k indices plus fractions
  point[0] = (inPoint[0] - origin[0])*invSpacing[0];
  point[1] = (inPoint[1] - origin[1])*invSpacing[1];
  point[2] = (inPoint[2] - origin[2])*invSpacing[2];

  // first guess at inverse point, just subtract displacement
  // (the inverse point is given in i,j,k indices plus fractions)
  this->InterpolationFunction(point, deltaP, NULL,
                              gridPtr, gridType, extent, increments);

  inverse[0] = point[0] - (deltaP[0]*scale + shift)*invSpacing[0];
  inverse[1] = point[1] - (deltaP[1]*scale + shift)*invSpacing[1];
  inverse[2] = point[2] - (deltaP[2]*scale + shift)*invSpacing[2];
  lastInverse[0] = inverse[0];
  lastInverse[1] = inverse[1];
  lastInverse[2] = inverse[2];

  // do a maximum 500 iterations, usually less than 10 are required
  int n = this->InverseIterations;
  int i, j;

  for (i = 0; i < n; i++)
    {
    this->InterpolationFunction(inverse, deltaP, derivative,
                                gridPtr, gridType, extent, increments);

    // convert displacement
    deltaP[0] = (inverse[0] - point[0])*spacing[0] + deltaP[0]*scale + shift;
    deltaP[1] = (inverse[1] - point[1])*spacing[1] + deltaP[1]*scale + shift;
    deltaP[2] = (inverse[2] - point[2])*spacing[2] + deltaP[2]*scale + shift;

    // convert derivative
    for (j = 0; j < 3; j++)
      {
      derivative[j][0] = derivative[j][0]*scale*invSpacing[0];
      derivative[j][1] = derivative[j][1]*scale*invSpacing[1];
      derivative[j][2] = derivative[j][2]*scale*invSpacing[2];
      derivative[j][j] += 1.0;
      }

    // get the current function value
    functionValue = (deltaP[0]*deltaP[0] +
                     deltaP[1]*deltaP[1] +
                     deltaP[2]*deltaP[2]);

    // if the function value is decreasing, do next Newton step
    // (the f < 1.0 is there because I found that convergence
    // is more stable if only a single reduction step is done)
    if (i == 0 || functionValue < lastFunctionValue || f < 1.0)
      {
      // here is the critical step in Newton's method
      vtkMath::LinearSolve3x3(derivative,deltaP,deltaI);

      // get the error value in the output coord space
      errorSquared = (deltaI[0]*deltaI[0] +
                      deltaI[1]*deltaI[1] +
                      deltaI[2]*deltaI[2]);

      // break if less than tolerance in both coordinate systems
      if (errorSquared < toleranceSquared &&
          functionValue < toleranceSquared)
        {
        break;
        }

      // save the last inverse point
      lastInverse[0] = inverse[0];
      lastInverse[1] = inverse[1];
      lastInverse[2] = inverse[2];

      // save error at last inverse point
      lastFunctionValue = functionValue;

      // derivative of functionValue at last inverse point
      functionDerivative = (deltaP[0]*derivative[0][0]*deltaI[0] +
                            deltaP[1]*derivative[1][1]*deltaI[1] +
                            deltaP[2]*derivative[2][2]*deltaI[2])*2;

      // calculate new inverse point
      inverse[0] -= deltaI[0]*invSpacing[0];
      inverse[1] -= deltaI[1]*invSpacing[1];
      inverse[2] -= deltaI[2]*invSpacing[2];

      // reset f to 1.0
      f = 1.0;

      continue;
      }

    // the error is increasing, so take a partial step
    // (see Numerical Recipes 9.7 for rationale, this code
    //  is a simplification of the algorithm provided there)

    // quadratic approximation to find best fractional distance
    a = -functionDerivative/(2*(functionValue -
                                lastFunctionValue -
                                functionDerivative));

    // clamp to range [0.1,0.5]
    f *= (a < 0.1 ? 0.1 : (a > 0.5 ? 0.5 : a));

    // re-calculate inverse using fractional distance
    inverse[0] = lastInverse[0] - f*deltaI[0]*invSpacing[0];
    inverse[1] = lastInverse[1] - f*deltaI[1]*invSpacing[1];
    inverse[2] = lastInverse[2] - f*deltaI[2]*invSpacing[2];
    }

  vtkDebugMacro("Inverse Iterations: " << (i+1));

  if (i >= n)
    {
    // didn't converge: back up to last good result
    inverse[0] = lastInverse[0];
    inverse[1] = lastInverse[1];
    inverse[2] = lastInverse[2];

    vtkWarningMacro("InverseTransformPoint: no convergence (" <<
                    inPoint[0] << ", " << inPoint[1] << ", " << inPoint[2] <<
                    ") error = " << sqrt(errorSquared) << " after " <<
                    i << " iterations.");
    }

  // convert point
  outPoint[0] = inverse[0]*spacing[0] + origin[0];
  outPoint[1] = inverse[1]*spacing[1] + origin[1];
  outPoint[2] = inverse[2]*spacing[2] + origin[2];
}

//----------------------------------------------------------------------------
// convert double to double and back again
void vtkGridTransform::InverseTransformDerivative(const float point[3],
                                                  float output[3],
                                                  float derivative[3][3])
{
  double fpoint[3];
  double fderivative[3][3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->InverseTransformDerivative(fpoint,fpoint,fderivative);

  for (int i = 0; i < 3; i++)
    {
    output[i] = static_cast<float>(fpoint[i]);
    derivative[i][0] = static_cast<float>(fderivative[i][0]);
    derivative[i][1] = static_cast<float>(fderivative[i][1]);
    derivative[i][2] = static_cast<float>(fderivative[i][2]);
    }
}

//----------------------------------------------------------------------------
void vtkGridTransform::InverseTransformPoint(const double point[3],
                                             double output[3])
{
  // the derivative won't be used, but it is required for Newton's method
  double derivative[3][3];
  this->InverseTransformDerivative(point,output,derivative);
}

//----------------------------------------------------------------------------
// convert double to double and back again
void vtkGridTransform::InverseTransformPoint(const float point[3],
                                             float output[3])
{
  double fpoint[3];
  double fderivative[3][3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->InverseTransformDerivative(fpoint,fpoint,fderivative);

  output[0] = static_cast<float>(fpoint[0]);
  output[1] = static_cast<float>(fpoint[1]);
  output[2] = static_cast<float>(fpoint[2]);
}

//----------------------------------------------------------------------------
void vtkGridTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkGridTransform *gridTransform = (vtkGridTransform *)transform;

  this->SetInverseTolerance(gridTransform->InverseTolerance);
  this->SetInverseIterations(gridTransform->InverseIterations);
  this->SetInterpolationMode(gridTransform->InterpolationMode);
  this->InterpolationFunction = gridTransform->InterpolationFunction;
  this->SetDisplacementScale(gridTransform->DisplacementScale);
  this->ConnectionHolder->SetInputConnection(
    0, gridTransform->ConnectionHolder->GetNumberOfInputConnections(0) ?
    gridTransform->ConnectionHolder->GetInputConnection(0, 0) : 0);
  this->SetDisplacementShift(gridTransform->DisplacementShift);
  this->SetDisplacementScale(gridTransform->DisplacementScale);

  if (this->InverseFlag != gridTransform->InverseFlag)
    {
    this->InverseFlag = gridTransform->InverseFlag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkGridTransform::InternalUpdate()
{
  vtkImageData *grid = this->GetDisplacementGrid();
  this->GridPointer = 0;

  if (grid == 0)
    {
    return;
    }

  vtkAlgorithm* inputAlgorithm =
    this->ConnectionHolder->GetInputAlgorithm(0, 0);
  inputAlgorithm->Update();

  // In case output changed.
  grid = this->GetDisplacementGrid();

  if (grid->GetNumberOfScalarComponents() != 3)
    {
    vtkErrorMacro(<< "TransformPoint: displacement grid must have 3 components");
    return;
    }
  if (grid->GetScalarType() != VTK_CHAR &&
      grid->GetScalarType() != VTK_UNSIGNED_CHAR &&
      grid->GetScalarType() != VTK_SHORT &&
      grid->GetScalarType() != VTK_UNSIGNED_SHORT &&
      grid->GetScalarType() != VTK_FLOAT &&
      grid->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "TransformPoint: displacement grid is of unsupported numerical type");
    return;
    }

  this->GridPointer = grid->GetScalarPointer();
  this->GridScalarType = grid->GetScalarType();

  grid->GetSpacing(this->GridSpacing);
  grid->GetOrigin(this->GridOrigin);
  grid->GetExtent(this->GridExtent);
  grid->GetIncrements(this->GridIncrements);
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkGridTransform::MakeTransform()
{
  return vtkGridTransform::New();
}

//----------------------------------------------------------------------------
void vtkGridTransform::SetDisplacementGridConnection(vtkAlgorithmOutput* output)
{
  this->ConnectionHolder->SetInputConnection(output);
}

//----------------------------------------------------------------------------
void vtkGridTransform::SetDisplacementGridData(vtkImageData* grid)
{
  vtkTrivialProducer* tp = vtkTrivialProducer::New();
  tp->SetOutput(grid);
  this->SetDisplacementGridConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
vtkImageData* vtkGridTransform::GetDisplacementGrid()
{
  return vtkImageData::SafeDownCast(
    this->ConnectionHolder->GetInputDataObject(0, 0));
}
