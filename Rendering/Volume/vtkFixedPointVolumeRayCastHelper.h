/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFixedPointVolumeRayCastHelper
 * @brief   An abstract helper that generates images for the volume ray cast mapper
 *
 * This is the abstract superclass of all helper classes for the
 * vtkFixedPointVolumeRayCastMapper. This class should not be used directly.
 *
 * @sa
 * vtkFixedPointVolumeRayCastMapper
*/

#ifndef vtkFixedPointVolumeRayCastHelper_h
#define vtkFixedPointVolumeRayCastHelper_h

#define VTKKWRCHelper_GetCellScalarValues( DATA, SCALE, SHIFT ) \
  A = static_cast<unsigned int >(SCALE*(*(DATA     ) + SHIFT)); \
  B = static_cast<unsigned int >(SCALE*(*(DATA+Binc) + SHIFT)); \
  C = static_cast<unsigned int >(SCALE*(*(DATA+Cinc) + SHIFT)); \
  D = static_cast<unsigned int >(SCALE*(*(DATA+Dinc) + SHIFT)); \
  E = static_cast<unsigned int >(SCALE*(*(DATA+Einc) + SHIFT)); \
  F = static_cast<unsigned int >(SCALE*(*(DATA+Finc) + SHIFT)); \
  G = static_cast<unsigned int >(SCALE*(*(DATA+Ginc) + SHIFT)); \
  H = static_cast<unsigned int >(SCALE*(*(DATA+Hinc) + SHIFT))

#define VTKKWRCHelper_GetCellScalarValuesSimple( DATA )  \
  A = static_cast<unsigned int >(*(DATA     ));          \
  B = static_cast<unsigned int >(*(DATA+Binc));          \
  C = static_cast<unsigned int >(*(DATA+Cinc));          \
  D = static_cast<unsigned int >(*(DATA+Dinc));          \
  E = static_cast<unsigned int >(*(DATA+Einc));          \
  F = static_cast<unsigned int >(*(DATA+Finc));          \
  G = static_cast<unsigned int >(*(DATA+Ginc));          \
  H = static_cast<unsigned int >(*(DATA+Hinc))

#define VTKKWRCHelper_GetCellMagnitudeValues( ABCD, EFGH )       \
  mA = static_cast<unsigned int >(*(ABCD      ));                \
  mB = static_cast<unsigned int >(*(ABCD+mBFinc));               \
  mC = static_cast<unsigned int >(*(ABCD+mCGinc));               \
  mD = static_cast<unsigned int >(*(ABCD+mDHinc));               \
  mE = static_cast<unsigned int >(*(EFGH       ));               \
  mF = static_cast<unsigned int >(*(EFGH+mBFinc));               \
  mG = static_cast<unsigned int >(*(EFGH+mCGinc));               \
  mH = static_cast<unsigned int >(*(EFGH+mDHinc))

#define VTKKWRCHelper_GetCellDirectionValues( ABCD, EFGH )      \
  normalA   = static_cast<unsigned int >(*(ABCD       ));       \
  normalB   = static_cast<unsigned int >(*(ABCD+dBFinc));       \
  normalC   = static_cast<unsigned int >(*(ABCD+dCGinc));       \
  normalD   = static_cast<unsigned int >(*(ABCD+dDHinc));       \
  normalE   = static_cast<unsigned int >(*(EFGH       ));       \
  normalF   = static_cast<unsigned int >(*(EFGH+dBFinc));       \
  normalG   = static_cast<unsigned int >(*(EFGH+dCGinc));       \
  normalH   = static_cast<unsigned int >(*(EFGH+dDHinc));

#define VTKKWRCHelper_GetCellComponentScalarValues( DATA, CIDX, SCALE, SHIFT )    \
  A[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA     ) + SHIFT));             \
  B[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Binc) + SHIFT));             \
  C[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Cinc) + SHIFT));             \
  D[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Dinc) + SHIFT));             \
  E[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Einc) + SHIFT));             \
  F[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Finc) + SHIFT));             \
  G[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Ginc) + SHIFT));             \
  H[CIDX] = static_cast<unsigned int >(SCALE*(*(DATA+Hinc) + SHIFT))

#define VTKKWRCHelper_GetCellComponentRawScalarValues( DATA, CIDX ) \
  A[CIDX] = static_cast<unsigned int >((*(DATA     )));             \
  B[CIDX] = static_cast<unsigned int >((*(DATA+Binc)));             \
  C[CIDX] = static_cast<unsigned int >((*(DATA+Cinc)));             \
  D[CIDX] = static_cast<unsigned int >((*(DATA+Dinc)));             \
  E[CIDX] = static_cast<unsigned int >((*(DATA+Einc)));             \
  F[CIDX] = static_cast<unsigned int >((*(DATA+Finc)));             \
  G[CIDX] = static_cast<unsigned int >((*(DATA+Ginc)));             \
  H[CIDX] = static_cast<unsigned int >((*(DATA+Hinc)))

#define VTKKWRCHelper_GetCellComponentMagnitudeValues( ABCD, EFGH, CIDX )      \
  mA[CIDX] = static_cast<unsigned int >(*(ABCD       ));                       \
  mB[CIDX] = static_cast<unsigned int >(*(ABCD+mBFinc));                       \
  mC[CIDX] = static_cast<unsigned int >(*(ABCD+mCGinc));                       \
  mD[CIDX] = static_cast<unsigned int >(*(ABCD+mDHinc));                       \
  mE[CIDX] = static_cast<unsigned int >(*(EFGH       ));                       \
  mF[CIDX] = static_cast<unsigned int >(*(EFGH+mBFinc));                       \
  mG[CIDX] = static_cast<unsigned int >(*(EFGH+mCGinc));                       \
  mH[CIDX] = static_cast<unsigned int >(*(EFGH+mDHinc))

#define VTKKWRCHelper_GetCellComponentDirectionValues( ABCD, EFGH, CIDX )       \
  normalA[CIDX]   = static_cast<unsigned int >(*(ABCD       ));                 \
  normalB[CIDX]   = static_cast<unsigned int >(*(ABCD+dBFinc));                 \
  normalC[CIDX]   = static_cast<unsigned int >(*(ABCD+dCGinc));                 \
  normalD[CIDX]   = static_cast<unsigned int >(*(ABCD+dDHinc));                 \
  normalE[CIDX]   = static_cast<unsigned int >(*(EFGH       ));                 \
  normalF[CIDX]   = static_cast<unsigned int >(*(EFGH+dBFinc));                 \
  normalG[CIDX]   = static_cast<unsigned int >(*(EFGH+dCGinc));                 \
  normalH[CIDX]   = static_cast<unsigned int >(*(EFGH+dDHinc));

#define VTKKWRCHelper_ComputeWeights( POS )                                             \
  w2X = (POS[0]&VTKKW_FP_MASK);                                                         \
  w2Y = (POS[1]&VTKKW_FP_MASK);                                                         \
  w2Z = (POS[2]&VTKKW_FP_MASK);                                                         \
                                                                                        \
  w1X = ((~w2X)&VTKKW_FP_MASK);                                                         \
  w1Y = ((~w2Y)&VTKKW_FP_MASK);                                                         \
  w1Z = ((~w2Z)&VTKKW_FP_MASK);                                                         \
                                                                                        \
  w1Xw1Y = (0x4000+(w1X*w1Y))>>VTKKW_FP_SHIFT;                                          \
  w2Xw1Y = (0x4000+(w2X*w1Y))>>VTKKW_FP_SHIFT;                                          \
  w1Xw2Y = (0x4000+(w1X*w2Y))>>VTKKW_FP_SHIFT;                                          \
  w2Xw2Y = (0x4000+(w2X*w2Y))>>VTKKW_FP_SHIFT;                                          \

#define VTKKWRCHelper_InterpolateScalar( VAL )                                  \
  VAL =                                                                         \
    (0x7fff + ((A*((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (B*((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (C*((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (D*((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (E*((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (F*((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (G*((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (H*((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;

#define VTKKWRCHelper_InterpolateMagnitude( VAL )                                       \
  VAL =                                                                                 \
    (0x7fff + ((mA*((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                           \
               (mB*((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                           \
               (mC*((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                           \
               (mD*((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                           \
               (mE*((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                           \
               (mF*((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                           \
               (mG*((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                           \
               (mH*((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;

#define VTKKWRCHelper_InterpolateScalarComponent( VAL, CIDX, COMPONENTS )               \
  for ( CIDX = 0; CIDX < COMPONENTS; CIDX++ )                                           \
  {                                                                                   \
    VAL[CIDX] =                                                                         \
    (0x7fff + ((A[CIDX]*((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (B[CIDX]*((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (C[CIDX]*((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (D[CIDX]*((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (E[CIDX]*((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (F[CIDX]*((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (G[CIDX]*((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (H[CIDX]*((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;   \
  }                                                                                   \

#define VTKKWRCHelper_InterpolateMagnitudeComponent( VAL, CIDX, COMPONENTS )            \
  for ( CIDX = 0; CIDX < COMPONENTS; CIDX++ )                                           \
  {                                                                                   \
    VAL[CIDX] =                                                                         \
    (0x7fff + ((mA[CIDX]*((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                     \
               (mB[CIDX]*((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                     \
               (mC[CIDX]*((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                     \
               (mD[CIDX]*((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                     \
               (mE[CIDX]*((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                     \
               (mF[CIDX]*((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                     \
               (mG[CIDX]*((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                     \
               (mH[CIDX]*((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;  \
  }

#define VTKKWRCHelper_InterpolateShading( DTABLE, STABLE, COLOR )                                       \
  unsigned int _tmpDColor[3];                                                                           \
  unsigned int _tmpSColor[3];                                                                           \
                                                                                                        \
  _tmpDColor[0] =                                                                                       \
    (0x7fff + ((DTABLE[3*normalA] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalB] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalC] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalD] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalE] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalF] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalG] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                          \
               (DTABLE[3*normalH] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;       \
                                                                                                        \
  _tmpDColor[1] =                                                                                       \
    (0x7fff + ((DTABLE[3*normalA+1] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalB+1] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalC+1] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalD+1] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalE+1] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalF+1] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalG+1] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalH+1] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;     \
                                                                                                        \
  _tmpDColor[2] =                                                                                       \
    (0x7fff + ((DTABLE[3*normalA+2] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalB+2] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalC+2] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalD+2] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalE+2] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalF+2] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalG+2] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (DTABLE[3*normalH+2] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;     \
                                                                                                        \
  _tmpSColor[0] =                                                                                       \
    (0x7fff + ((STABLE[3*normalA] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalB] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalC] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalD] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalE] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalF] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalG] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                          \
               (STABLE[3*normalH] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;       \
                                                                                                        \
  _tmpSColor[1] =                                                                                       \
    (0x7fff + ((STABLE[3*normalA+1] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalB+1] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalC+1] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalD+1] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalE+1] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalF+1] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalG+1] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalH+1] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;     \
                                                                                                        \
  _tmpSColor[2] =                                                                                       \
    (0x7fff + ((STABLE[3*normalA+2] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalB+2] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalC+2] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalD+2] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalE+2] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalF+2] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalG+2] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                        \
               (STABLE[3*normalH+2] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;     \
                                                                                                        \
                                                                                                        \
  COLOR[0] = static_cast<unsigned short>((_tmpDColor[0]*COLOR[0]+0x7fff)>>VTKKW_FP_SHIFT);              \
  COLOR[1] = static_cast<unsigned short>((_tmpDColor[1]*COLOR[1]+0x7fff)>>VTKKW_FP_SHIFT);              \
  COLOR[2] = static_cast<unsigned short>((_tmpDColor[2]*COLOR[2]+0x7fff)>>VTKKW_FP_SHIFT);              \
  COLOR[0] += (_tmpSColor[0]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;                                        \
  COLOR[1] += (_tmpSColor[1]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;                                        \
  COLOR[2] += (_tmpSColor[2]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;

#define VTKKWRCHelper_InterpolateShadingComponent( DTABLE, STABLE, COLOR, CIDX )                                \
  unsigned int _tmpDColor[3];                                                                                   \
  unsigned int _tmpSColor[3];                                                                                   \
                                                                                                                \
  _tmpDColor[0] =                                                                                               \
    (0x7fff + ((DTABLE[CIDX][3*normalA[CIDX]] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalB[CIDX]] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalC[CIDX]] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalD[CIDX]] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalE[CIDX]] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalF[CIDX]] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalG[CIDX]] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (DTABLE[CIDX][3*normalH[CIDX]] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;   \
                                                                                                                \
  _tmpDColor[1] =                                                                                               \
    (0x7fff + ((DTABLE[CIDX][3*normalA[CIDX]+1] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalB[CIDX]+1] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalC[CIDX]+1] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalD[CIDX]+1] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalE[CIDX]+1] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalF[CIDX]+1] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalG[CIDX]+1] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalH[CIDX]+1] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT; \
                                                                                                                \
  _tmpDColor[2] =                                                                                               \
    (0x7fff + ((DTABLE[CIDX][3*normalA[CIDX]+2] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalB[CIDX]+2] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalC[CIDX]+2] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalD[CIDX]+2] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalE[CIDX]+2] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalF[CIDX]+2] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalG[CIDX]+2] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (DTABLE[CIDX][3*normalH[CIDX]+2] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT; \
                                                                                                                \
  _tmpSColor[0] =                                                                                               \
    (0x7fff + ((STABLE[CIDX][3*normalA[CIDX]] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalB[CIDX]] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalC[CIDX]] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalD[CIDX]] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalE[CIDX]] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalF[CIDX]] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalG[CIDX]] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                      \
               (STABLE[CIDX][3*normalH[CIDX]] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT;   \
                                                                                                                \
  _tmpSColor[1] =                                                                                               \
    (0x7fff + ((STABLE[CIDX][3*normalA[CIDX]+1] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalB[CIDX]+1] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalC[CIDX]+1] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalD[CIDX]+1] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalE[CIDX]+1] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalF[CIDX]+1] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalG[CIDX]+1] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalH[CIDX]+1] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT; \
                                                                                                                \
  _tmpSColor[2] =                                                                                               \
    (0x7fff + ((STABLE[CIDX][3*normalA[CIDX]+2] * ((0x4000 + w1Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalB[CIDX]+2] * ((0x4000 + w2Xw1Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalC[CIDX]+2] * ((0x4000 + w1Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalD[CIDX]+2] * ((0x4000 + w2Xw2Y*w1Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalE[CIDX]+2] * ((0x4000 + w1Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalF[CIDX]+2] * ((0x4000 + w2Xw1Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalG[CIDX]+2] * ((0x4000 + w1Xw2Y*w2Z)>>VTKKW_FP_SHIFT)) +                    \
               (STABLE[CIDX][3*normalH[CIDX]+2] * ((0x4000 + w2Xw2Y*w2Z)>>VTKKW_FP_SHIFT)))) >> VTKKW_FP_SHIFT; \
                                                                                                                \
                                                                                                                \
  COLOR[0] = static_cast<unsigned short>((_tmpDColor[0]*COLOR[0]+0x7fff)>>VTKKW_FP_SHIFT);                      \
  COLOR[1] = static_cast<unsigned short>((_tmpDColor[1]*COLOR[1]+0x7fff)>>VTKKW_FP_SHIFT);                      \
  COLOR[2] = static_cast<unsigned short>((_tmpDColor[2]*COLOR[2]+0x7fff)>>VTKKW_FP_SHIFT);                      \
  COLOR[0] += (_tmpSColor[0]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;                                                \
  COLOR[1] += (_tmpSColor[1]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;                                                \
  COLOR[2] += (_tmpSColor[2]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;

#define VTKKWRCHelper_LookupColorUS( COLORTABLE, SCALAROPACITYTABLE, IDX, COLOR )       \
  COLOR[3] = SCALAROPACITYTABLE[IDX];                                                   \
  if ( !COLOR[3] ) {continue;}                                                          \
  COLOR[0] = static_cast<unsigned short>                                                \
    ((COLORTABLE[3*IDX  ]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));                        \
  COLOR[1] = static_cast<unsigned short>                                                \
    ((COLORTABLE[3*IDX+1]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));                        \
  COLOR[2] = static_cast<unsigned short>                                                \
    ((COLORTABLE[3*IDX+2]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));

#define VTKKWRCHelper_LookupColorMax( COLORTABLE, SCALAROPACITYTABLE, IDX, COLOR )    \
  COLOR[3] = SCALAROPACITYTABLE[IDX];                                                 \
  COLOR[0] = static_cast<unsigned short>                                              \
    ((COLORTABLE[3*IDX  ]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));                      \
  COLOR[1] = static_cast<unsigned short>                                              \
    ((COLORTABLE[3*IDX+1]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));                      \
  COLOR[2] = static_cast<unsigned short>                                              \
    ((COLORTABLE[3*IDX+2]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));

#define VTKKWRCHelper_LookupDependentColorUS( COLORTABLE, SCALAROPACITYTABLE, IDX, CMPS, COLOR )        \
  {                                                                                                     \
  unsigned short _alpha;                                                                                \
  switch ( CMPS )                                                                                       \
  {                                                                                                   \
    case 2:                                                                                             \
      _alpha = SCALAROPACITYTABLE[IDX[1]];                                                              \
      COLOR[0] = static_cast<unsigned short>                                                            \
        ((COLORTABLE[3*IDX[0]  ]*_alpha + 0x7fff)>>(VTKKW_FP_SHIFT));                                   \
      COLOR[1] = static_cast<unsigned short>                                                            \
        ((COLORTABLE[3*IDX[0]+1]*_alpha + 0x7fff)>>(VTKKW_FP_SHIFT));                                   \
      COLOR[2] = static_cast<unsigned short>                                                            \
        ((COLORTABLE[3*IDX[0]+2]*_alpha + 0x7fff)>>(VTKKW_FP_SHIFT));                                   \
      COLOR[3] = _alpha;                                                                                \
      break;                                                                                            \
    case 4:                                                                                             \
      _alpha = SCALAROPACITYTABLE[IDX[3]];                                                              \
      COLOR[0] = static_cast<unsigned short>((IDX[0]*_alpha + 0x7f)>>8 );                               \
      COLOR[1] = static_cast<unsigned short>((IDX[1]*_alpha + 0x7f)>>8 );                               \
      COLOR[2] = static_cast<unsigned short>((IDX[2]*_alpha + 0x7f)>>8 );                               \
      COLOR[3] = _alpha;                                                                                \
      break;                                                                                            \
  }                                                                                                   \
  }

#define VTKKWRCHelper_LookupColorGOUS( CTABLE, SOTABLE, GOTABLE, IDX, IDX2, COLOR )     \
  COLOR[3] = (SOTABLE[IDX] * GOTABLE[IDX2] + 0x7fff)>>VTKKW_FP_SHIFT;                   \
  if ( !COLOR[3] ) {continue;}                                                          \
  COLOR[0] = static_cast<unsigned short>                                                \
    ((CTABLE[3*IDX  ]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));                            \
  COLOR[1] = static_cast<unsigned short>                                                \
    ((CTABLE[3*IDX+1]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));                            \
  COLOR[2] = static_cast<unsigned short>                                                \
    ((CTABLE[3*IDX+2]*COLOR[3] + 0x7fff)>>(VTKKW_FP_SHIFT));

#define VTKKWRCHelper_LookupShading( DTABLE, STABLE, NORMAL, COLOR )                            \
  COLOR[0] = static_cast<unsigned short>((DTABLE[3*NORMAL  ]*COLOR[0]+0x7fff)>>VTKKW_FP_SHIFT); \
  COLOR[1] = static_cast<unsigned short>((DTABLE[3*NORMAL+1]*COLOR[1]+0x7fff)>>VTKKW_FP_SHIFT); \
  COLOR[2] = static_cast<unsigned short>((DTABLE[3*NORMAL+2]*COLOR[2]+0x7fff)>>VTKKW_FP_SHIFT); \
  COLOR[0] += (STABLE[3*NORMAL  ]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;                           \
  COLOR[1] += (STABLE[3*NORMAL+1]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;                           \
  COLOR[2] += (STABLE[3*NORMAL+2]*COLOR[3] + 0x7fff)>>VTKKW_FP_SHIFT;

#define VTKKWRCHelper_LookupAndCombineIndependentColorsUS( COLORTABLE, SOTABLE,                                                 \
                                                           SCALAR, WEIGHTS,                                                     \
                                                           COMPONENTS, COLOR )                                                  \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                             \
  unsigned short _alpha[4] = {0,0,0,0};                                                                                         \
  unsigned int _totalAlpha = 0;                                                                                                 \
                                                                                                                                \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    _alpha[_idx] = static_cast<unsigned short>(SOTABLE[_idx][SCALAR[_idx]]*WEIGHTS[_idx]);                                      \
    _totalAlpha += _alpha[_idx];                                                                                                \
    }}                                                                                                                          \
                                                                                                                                \
  if ( !_totalAlpha ) {continue;}                                                                                               \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _tmp[0] += static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]  ])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmp[1] += static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+1])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmp[2] += static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+2])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmp[3] += ((_alpha[_idx]*_alpha[_idx])/_totalAlpha);                                                                     \
    }                                                                                                                         \
    }}                                                                                                                          \
  if ( !_tmp[3] ) {continue;}                                                                                                   \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                                 \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                                 \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                                 \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);

#define VTKKWRCHelper_LookupAndCombineIndependentColorsMax( COLORTABLE, SCALAROPACITYTABLE,                             \
                                                            IDX, WEIGHTS, CMPS, COLOR )                                 \
{                                                                                                                     \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                     \
  for ( int _idx = 0; _idx < CMPS; _idx++ )                                                                             \
  {                                                                                                                   \
    unsigned short _alpha = static_cast<unsigned short>(SCALAROPACITYTABLE[_idx][IDX[_idx]]*WEIGHTS[_idx]);             \
    _tmp[0] += static_cast<unsigned short>(((COLORTABLE[_idx][3*IDX[_idx]  ])*_alpha + 0x7fff)>>(VTKKW_FP_SHIFT));      \
    _tmp[1] += static_cast<unsigned short>(((COLORTABLE[_idx][3*IDX[_idx]+1])*_alpha + 0x7fff)>>(VTKKW_FP_SHIFT));      \
    _tmp[2] += static_cast<unsigned short>(((COLORTABLE[_idx][3*IDX[_idx]+2])*_alpha + 0x7fff)>>(VTKKW_FP_SHIFT));      \
    _tmp[3] += _alpha;                                                                                                  \
  }                                                                                                                   \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                         \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                         \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                         \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);                                                                         \
}

#define VTKKWRCHelper_LookupAndCombineIndependentColorsGOUS( COLORTABLE, SOTABLE,                                               \
                                                           GOTABLE,                                                             \
                                                           SCALAR, MAG, WEIGHTS,                                                \
                                                           COMPONENTS, COLOR )                                                  \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                             \
  unsigned short _alpha[4] = {0,0,0,0};                                                                                         \
  unsigned int _totalAlpha = 0;                                                                                                 \
  COMPONENTS = (COMPONENTS < 4) ? COMPONENTS : 4;                                                                               \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    _alpha[_idx] =  static_cast<unsigned short>(SOTABLE[_idx][SCALAR[_idx]]*WEIGHTS[_idx]);                                     \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _alpha[_idx] = static_cast<unsigned short>((_alpha[_idx]*GOTABLE[_idx][MAG[_idx]] + 0x7fff)>>(VTKKW_FP_SHIFT));           \
      _totalAlpha += _alpha[_idx];                                                                                              \
    }                                                                                                                         \
    }}                                                                                                                          \
                                                                                                                                \
  if ( !_totalAlpha ) {continue;}                                                                                               \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _tmp[0] += static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]  ])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmp[1] += static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+1])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmp[2] += static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+2])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmp[3] += ((_alpha[_idx]*_alpha[_idx])/_totalAlpha);                                                                     \
    }                                                                                                                         \
    }}                                                                                                                          \
  if ( !_tmp[3] ) {continue;};                                                                                                  \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                                 \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                                 \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                                 \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);

#define VTKKWRCHelper_LookupAndCombineIndependentColorsShadeUS( COLORTABLE, SOTABLE,                                            \
                                                                DTABLE, STABLE,                                                 \
                                                                SCALAR, NORMAL, WEIGHTS,                                        \
                                                                COMPONENTS, COLOR )                                             \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                             \
  unsigned int _tmpC[3];                                                                                                        \
  unsigned short _alpha[4] = {0,0,0,0};                                                                                         \
  unsigned int _totalAlpha = 0;                                                                                                 \
                                                                                                                                \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    _alpha[_idx] = static_cast<unsigned short>(SOTABLE[_idx][SCALAR[_idx]]*WEIGHTS[_idx]);                                      \
    _totalAlpha += _alpha[_idx];                                                                                                \
    }}                                                                                                                          \
                                                                                                                                \
  if ( !_totalAlpha ) {continue;}                                                                                               \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _tmpC[0] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]  ])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[1] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+1])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[2] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+2])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[0] = static_cast<unsigned short>((DTABLE[_idx][3*NORMAL[_idx]  ]*_tmpC[0]+0x7fff)>>VTKKW_FP_SHIFT);                 \
      _tmpC[1] = static_cast<unsigned short>((DTABLE[_idx][3*NORMAL[_idx]+1]*_tmpC[1]+0x7fff)>>VTKKW_FP_SHIFT);                 \
      _tmpC[2] = static_cast<unsigned short>((DTABLE[_idx][3*NORMAL[_idx]+2]*_tmpC[2]+0x7fff)>>VTKKW_FP_SHIFT);                 \
      _tmpC[0] += (STABLE[_idx][3*NORMAL[_idx]  ]*_alpha[_idx] + 0x7fff)>>VTKKW_FP_SHIFT;                                       \
      _tmpC[1] += (STABLE[_idx][3*NORMAL[_idx]+1]*_alpha[_idx] + 0x7fff)>>VTKKW_FP_SHIFT;                                       \
      _tmpC[2] += (STABLE[_idx][3*NORMAL[_idx]+2]*_alpha[_idx] + 0x7fff)>>VTKKW_FP_SHIFT;                                       \
      _tmp[0] += _tmpC[0];                                                                                                      \
      _tmp[1] += _tmpC[1];                                                                                                      \
      _tmp[2] += _tmpC[2];                                                                                                      \
      _tmp[3] += ((_alpha[_idx]*_alpha[_idx])/_totalAlpha);                                                                     \
    }                                                                                                                         \
    }}                                                                                                                           \
  if ( !_tmp[3] ) {continue;}                                                                                                   \
                                                                                                                                \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                                 \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                                 \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                                 \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);

#define VTKKWRCHelper_LookupAndCombineIndependentColorsInterpolateShadeUS( COLORTABLE, SOTABLE,                                 \
                                                                           DTABLE, STABLE,                                      \
                                                                           SCALAR, WEIGHTS,                                     \
                                                                           COMPONENTS, COLOR )                                  \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                             \
  unsigned int _tmpC[4];                                                                                                        \
  unsigned short _alpha[4] = {0,0,0,0};                                                                                         \
  unsigned int _totalAlpha = 0;                                                                                                 \
                                                                                                                                \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    _alpha[_idx] = static_cast<unsigned short>(SOTABLE[_idx][SCALAR[_idx]]*WEIGHTS[_idx]);                                      \
    _totalAlpha += _alpha[_idx];                                                                                                \
    }}                                                                                                                          \
                                                                                                                                \
  if ( !_totalAlpha ) {continue;}                                                                                               \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _tmpC[0] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]  ])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[1] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+1])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[2] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+2])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[3] = _alpha[_idx];                                                                                                  \
      VTKKWRCHelper_InterpolateShadingComponent( DTABLE, STABLE, _tmpC, _idx );                                                 \
      _tmp[0] += _tmpC[0];                                                                                                      \
      _tmp[1] += _tmpC[1];                                                                                                      \
      _tmp[2] += _tmpC[2];                                                                                                      \
      _tmp[3] += ((_alpha[_idx]*_alpha[_idx])/_totalAlpha);                                                                     \
    }                                                                                                                         \
    }}                                                                                                                          \
  if (!_tmp[3]) {continue;}                                                                                                     \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                                 \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                                 \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                                 \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);

#define VTKKWRCHelper_CompositeColorAndCheckEarlyTermination( COLOR, TMP, REMAININGOPACITY )    \
  COLOR[0] += (TMP[0]*REMAININGOPACITY+0x7fff)>>VTKKW_FP_SHIFT;                                 \
  COLOR[1] += (TMP[1]*REMAININGOPACITY+0x7fff)>>VTKKW_FP_SHIFT;                                 \
  COLOR[2] += (TMP[2]*REMAININGOPACITY+0x7fff)>>VTKKW_FP_SHIFT;                                 \
  REMAININGOPACITY = (REMAININGOPACITY*((~(TMP[3])&VTKKW_FP_MASK))+0x7fff)>>VTKKW_FP_SHIFT;     \
  if ( REMAININGOPACITY < 0xff )                                                                \
  {                                                                                           \
    break;                                                                                      \
  }

#define VTKKWRCHelper_LookupAndCombineIndependentColorsGOShadeUS( COLORTABLE, SOTABLE, GOTABLE,                                 \
                                                                DTABLE, STABLE,                                                 \
                                                                SCALAR, MAG, NORMAL, WEIGHTS,                                   \
                                                                COMPONENTS, COLOR )                                             \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                             \
  unsigned int _tmpC[3];                                                                                                        \
  unsigned short _alpha[4] = {0,0,0,0};                                                                                         \
  unsigned int _totalAlpha = 0;                                                                                                 \
                                                                                                                                \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    _alpha[_idx] =  static_cast<unsigned short>(SOTABLE[_idx][SCALAR[_idx]]*WEIGHTS[_idx]);                                     \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _alpha[_idx] = static_cast<unsigned short>((_alpha[_idx]*GOTABLE[_idx][MAG[_idx]] + 0x7fff)>>(VTKKW_FP_SHIFT));           \
      _totalAlpha += _alpha[_idx];                                                                                              \
    }                                                                                                                         \
    }}                                                                                                                          \
                                                                                                                                \
  if ( !_totalAlpha ) {continue;}                                                                                               \
                                                                                                                                \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _tmpC[0] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]  ])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[1] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+1])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[2] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+2])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[0] = static_cast<unsigned short>((DTABLE[_idx][3*NORMAL[_idx]  ]*_tmpC[0]+0x7fff)>>VTKKW_FP_SHIFT);                 \
      _tmpC[1] = static_cast<unsigned short>((DTABLE[_idx][3*NORMAL[_idx]+1]*_tmpC[1]+0x7fff)>>VTKKW_FP_SHIFT);                 \
      _tmpC[2] = static_cast<unsigned short>((DTABLE[_idx][3*NORMAL[_idx]+2]*_tmpC[2]+0x7fff)>>VTKKW_FP_SHIFT);                 \
      _tmpC[0] += (STABLE[_idx][3*NORMAL[_idx]  ]*_alpha[_idx] + 0x7fff)>>VTKKW_FP_SHIFT;                                       \
      _tmpC[1] += (STABLE[_idx][3*NORMAL[_idx]+1]*_alpha[_idx] + 0x7fff)>>VTKKW_FP_SHIFT;                                       \
      _tmpC[2] += (STABLE[_idx][3*NORMAL[_idx]+2]*_alpha[_idx] + 0x7fff)>>VTKKW_FP_SHIFT;                                       \
      _tmp[0] += _tmpC[0];                                                                                                      \
      _tmp[1] += _tmpC[1];                                                                                                      \
      _tmp[2] += _tmpC[2];                                                                                                      \
      _tmp[3] += ((_alpha[_idx]*_alpha[_idx])/_totalAlpha);                                                                     \
    }                                                                                                                         \
    }}                                                                                                                          \
  if ( !_tmp[3] ) {continue;}                                                                                                   \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                                 \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                                 \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                                 \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);

#define VTKKWRCHelper_LookupAndCombineIndependentColorsGOInterpolateShadeUS( COLORTABLE, SOTABLE, GOTABLE,                      \
                                                                DTABLE, STABLE,                                                 \
                                                                SCALAR, MAG, WEIGHTS,                                           \
                                                                COMPONENTS, COLOR )                                             \
  unsigned int _tmp[4] = {0,0,0,0};                                                                                             \
  unsigned int _tmpC[4];                                                                                                        \
  unsigned short _alpha[4] = {0,0,0,0};                                                                                         \
  unsigned int _totalAlpha = 0;                                                                                                 \
                                                                                                                                \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    _alpha[_idx] =  static_cast<unsigned short>(SOTABLE[_idx][SCALAR[_idx]]*WEIGHTS[_idx]);                                     \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _alpha[_idx] = static_cast<unsigned short>((_alpha[_idx]*GOTABLE[_idx][MAG[_idx]] + 0x7fff)>>(VTKKW_FP_SHIFT));           \
      _totalAlpha += _alpha[_idx];                                                                                              \
    }                                                                                                                         \
    }}                                                                                                                          \
                                                                                                                                \
  if ( !_totalAlpha ) {continue;}                                                                                               \
  {for ( int _idx = 0; _idx < COMPONENTS; _idx++ )                                                                              \
    {                                                                                                                           \
    if ( _alpha[_idx] )                                                                                                         \
    {                                                                                                                         \
      _tmpC[0] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]  ])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[1] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+1])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[2] = static_cast<unsigned short>(((COLORTABLE[_idx][3*SCALAR[_idx]+2])*_alpha[_idx] + 0x7fff)>>(VTKKW_FP_SHIFT));   \
      _tmpC[3] = _alpha[_idx];                                                                                                  \
      VTKKWRCHelper_InterpolateShadingComponent( DTABLE, STABLE, _tmpC, _idx );                                                 \
      _tmp[0] += _tmpC[0];                                                                                                      \
      _tmp[1] += _tmpC[1];                                                                                                      \
      _tmp[2] += _tmpC[2];                                                                                                      \
      _tmp[3] += ((_alpha[_idx]*_alpha[_idx])/_totalAlpha);                                                                     \
    }                                                                                                                         \
    }}                                                                                                                           \
  if ( !_tmp[3] ) {continue;}                                                                                                   \
  COLOR[0] = (_tmp[0]>32767)?(32767):(_tmp[0]);                                                                                 \
  COLOR[1] = (_tmp[1]>32767)?(32767):(_tmp[1]);                                                                                 \
  COLOR[2] = (_tmp[2]>32767)?(32767):(_tmp[2]);                                                                                 \
  COLOR[3] = (_tmp[3]>32767)?(32767):(_tmp[3]);

#define VTKKWRCHelper_SetPixelColor( IMAGEPTR, COLOR, REMAININGOPACITY )        \
  IMAGEPTR[0] = (COLOR[0]>32767)?(32767):(COLOR[0]);                            \
  IMAGEPTR[1] = (COLOR[1]>32767)?(32767):(COLOR[1]);                            \
  IMAGEPTR[2] = (COLOR[2]>32767)?(32767):(COLOR[2]);                           \
  unsigned int tmpAlpha = (~REMAININGOPACITY)&VTKKW_FP_MASK;                    \
  IMAGEPTR[3] = (tmpAlpha>32767)?(32767):(tmpAlpha);

#define VTKKWRCHelper_MoveToNextSampleNN()                                      \
  if ( k < numSteps-1 )                                                         \
  {                                                                           \
    mapper->FixedPointIncrement( pos, dir );                                    \
    mapper->ShiftVectorDown( pos, spos );                                       \
    dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];            \
  }

#define VTKKWRCHelper_MoveToNextSampleGONN()                            \
  if ( k < numSteps-1 )                                                 \
  {                                                                   \
    mapper->FixedPointIncrement( pos, dir );                            \
    mapper->ShiftVectorDown( pos, spos );                               \
    dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];    \
    magPtr = gradientMag[spos[2]] + spos[0]*mInc[0] + spos[1]*mInc[1];  \
  }

#define VTKKWRCHelper_MoveToNextSampleShadeNN()                         \
  if ( k < numSteps-1 )                                                 \
  {                                                                   \
    mapper->FixedPointIncrement( pos, dir );                            \
    mapper->ShiftVectorDown( pos, spos );                               \
    dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];    \
    dirPtr = gradientDir[spos[2]] + spos[0]*dInc[0] + spos[1]*dInc[1];  \
  }

#define VTKKWRCHelper_MoveToNextSampleGOShadeNN()                       \
  if ( k < numSteps-1 )                                                 \
  {                                                                   \
    mapper->FixedPointIncrement( pos, dir );                            \
    mapper->ShiftVectorDown( pos, spos );                               \
    dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];    \
    magPtr = gradientMag[spos[2]] + spos[0]*mInc[0] + spos[1]*mInc[1];  \
    dirPtr = gradientDir[spos[2]] + spos[0]*dInc[0] + spos[1]*dInc[1];  \
  }

#define VTKKWRCHelper_InitializeVariables()                                                     \
  int i, j;                                                                                     \
  unsigned short *imagePtr;                                                                     \
                                                                                                \
  int imageInUseSize[2];                                                                        \
  int imageMemorySize[2];                                                                       \
  int imageViewportSize[2];                                                                     \
  int imageOrigin[2];                                                                           \
  int dim[3];                                                                                   \
  float shift[4];                                                                               \
  float scale[4];                                                                               \
                                                                                                \
  mapper->GetRayCastImage()->GetImageInUseSize(imageInUseSize);                                 \
  mapper->GetRayCastImage()->GetImageMemorySize(imageMemorySize);                               \
  mapper->GetRayCastImage()->GetImageViewportSize(imageViewportSize);                           \
  mapper->GetRayCastImage()->GetImageOrigin(imageOrigin);                                       \
  mapper->GetInput()->GetDimensions(dim);                                                       \
  mapper->GetTableShift( shift );                                                               \
  mapper->GetTableScale( scale );                                                               \
                                                                                                \
  int *rowBounds                     = mapper->GetRowBounds();                                  \
  unsigned short *image              = mapper->GetRayCastImage()->GetImage();                   \
  vtkRenderWindow *renWin            = mapper->GetRenderWindow();                               \
  int components                     = mapper->GetInput()->GetNumberOfScalarComponents();       \
  int cropping                       = (mapper->GetCropping() &&                                \
                                        mapper->GetCroppingRegionFlags() != 0x2000 );           \
                                                                                                \
  components = (components < 4) ? components : 4;                                               \
  unsigned short *colorTable[4];                                                                \
  unsigned short *scalarOpacityTable[4];                                                        \
                                                                                                \
  int c;                                                                                        \
  for ( c = 0; c < 4; c++ )                                                                     \
  {                                                                                           \
    colorTable[c]         = mapper->GetColorTable(c);                                           \
    (void)(colorTable[c]);                                                                      \
    scalarOpacityTable[c] = mapper->GetScalarOpacityTable(c);                                   \
  }                                                                                           \
                                                                                                \
  vtkIdType inc[3];                                                                             \
  inc[0] = components;                                                                          \
  inc[1] = inc[0]*dim[0];                                                                       \
  inc[2] = inc[1]*dim[1];

#define VTKKWRCHelper_InitializeWeights()                       \
  float weights[4] = {};                                        \
  weights[0] = vol->GetProperty()->GetComponentWeight(0);       \
  weights[1] = vol->GetProperty()->GetComponentWeight(1);       \
  weights[2] = vol->GetProperty()->GetComponentWeight(2);       \
  weights[3] = vol->GetProperty()->GetComponentWeight(3);

#define VTKKWRCHelper_InitializeVariablesGO()                           \
  unsigned short *gradientOpacityTable[4];                              \
  for ( c = 0; c < 4; c++ )                                             \
  {                                                                   \
    gradientOpacityTable[c] = mapper->GetGradientOpacityTable(c);       \
  }                                                                   \
  unsigned char **gradientMag = mapper->GetGradientMagnitude();         \
                                                                        \
  vtkIdType mInc[3];                                                    \
  if ( vol->GetProperty()->GetIndependentComponents() )                 \
  {                                                                   \
    mInc[0] = inc[0];                                                   \
    mInc[1] = inc[1];                                                   \
    mInc[2] = inc[2];                                                   \
  }                                                                   \
  else                                                                  \
  {                                                                   \
    mInc[0] = 1;                                                        \
    mInc[1] = mInc[0]*dim[0];                                           \
    mInc[2] = mInc[1]*dim[1];                                           \
  }

#define VTKKWRCHelper_InitializeVariablesShade()                        \
  unsigned short *diffuseShadingTable[4];                               \
  unsigned short *specularShadingTable[4];                              \
  for ( c = 0; c < 4; c++ )                                             \
  {                                                                   \
    diffuseShadingTable[c] = mapper->GetDiffuseShadingTable(c);         \
    specularShadingTable[c] = mapper->GetSpecularShadingTable(c);       \
  }                                                                   \
  unsigned short **gradientDir = mapper->GetGradientNormal();           \
  vtkIdType dInc[3];                                                    \
  if ( vol->GetProperty()->GetIndependentComponents() )                 \
  {                                                                   \
    dInc[0] = inc[0];                                                   \
    dInc[1] = inc[1];                                                   \
    dInc[2] = inc[2];                                                   \
  }                                                                   \
  else                                                                  \
  {                                                                   \
    dInc[0] = 1;                                                        \
    dInc[1] = dInc[0]*dim[0];                                           \
    dInc[2] = dInc[1]*dim[1];                                           \
  }

#define VTKKWRCHelper_InitializeTrilinVariables() \
  vtkIdType Binc = components;                    \
  vtkIdType Cinc = Binc*dim[0];                   \
  vtkIdType Dinc = Cinc + Binc;                   \
  vtkIdType Einc = Cinc*dim[1];                   \
  vtkIdType Finc = Einc + Binc;                   \
  vtkIdType Ginc = Einc + Cinc;                   \
  vtkIdType Hinc = Ginc + Binc;

#define VTKKWRCHelper_InitializeTrilinVariablesGO()             \
  vtkIdType magOffset;                                          \
  if (  vol->GetProperty()->GetIndependentComponents() )        \
  {                                                           \
    magOffset = components;                                     \
  }                                                           \
  else                                                          \
  {                                                           \
    magOffset = 1;                                              \
  }                                                           \
                                                                \
  vtkIdType mBFinc =                    magOffset;              \
  vtkIdType mCGinc = dim[0]*magOffset;                          \
  vtkIdType mDHinc = dim[0]*magOffset + magOffset;

#define VTKKWRCHelper_InitializeTrilinVariablesShade()          \
  vtkIdType dirOffset;                                          \
  if (  vol->GetProperty()->GetIndependentComponents() )        \
  {                                                           \
    dirOffset = components;                                     \
  }                                                           \
  else                                                          \
  {                                                           \
    dirOffset = 1;                                              \
  }                                                           \
                                                                \
  vtkIdType dBFinc =                    dirOffset;              \
  vtkIdType dCGinc = dim[0]*dirOffset;                          \
  vtkIdType dDHinc = dim[0]*dirOffset + dirOffset;

#define VTKKWRCHelper_OuterInitialization()                             \
    if ( j%threadCount != threadID )                                    \
    {                                                                 \
      continue;                                                         \
    }                                                                 \
     if ( !threadID )                                                   \
     {                                                                 \
      if ( renWin->CheckAbortStatus() )                                 \
      {                                                               \
        break;                                                          \
      }                                                               \
     }                                                                 \
    else if ( renWin->GetAbortRender() )                                \
    {                                                                 \
      break;                                                            \
    }                                                                 \
    imagePtr = image + 4*(j*imageMemorySize[0] + rowBounds[j*2]);

#define VTKKWRCHelper_InnerInitialization()             \
  unsigned int   numSteps;                              \
  unsigned int   pos[3];                                \
  unsigned int   dir[3];                                \
  mapper->ComputeRayInfo( i, j, pos, dir, &numSteps );  \
  if ( numSteps == 0 )                                  \
  {                                                   \
    *(imagePtr  ) = 0;                                  \
    *(imagePtr+1) = 0;                                  \
    *(imagePtr+2) = 0;                                  \
    *(imagePtr+3) = 0;                                  \
    imagePtr += 4;                                      \
    continue;                                           \
  }                                                   \
  unsigned int   spos[3];                               \
  unsigned int   k;

#define VTKKWRCHelper_InitializeMIPOneNN()                              \
  mapper->ShiftVectorDown( pos, spos );                                 \
  T *dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];   \
  T maxValue = *(dptr);

#define VTKKWRCHelper_InitializeMIPMultiNN()                            \
  mapper->ShiftVectorDown( pos, spos );                                 \
  T *dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];   \
  T maxValue[4] = {};                                                   \
  for ( c = 0; c < components; c++ )                                    \
  {                                                                   \
    maxValue[c] = *(dptr+c);                                            \
  }

#define VTKKWRCHelper_InitializeMIPOneTrilin()          \
  T *dptr;                                              \
  unsigned int oldSPos[3];                              \
                                                        \
  oldSPos[0] = (pos[0] >> VTKKW_FP_SHIFT) + 1;          \
  oldSPos[1] = 0;                                       \
  oldSPos[2] = 0;                                       \
                                                        \
  unsigned int w1X, w1Y, w1Z;                           \
  unsigned int w2X, w2Y, w2Z;                           \
  unsigned int w1Xw1Y, w2Xw1Y, w1Xw2Y, w2Xw2Y;          \
                                                        \
  unsigned short  maxValue=0;                           \
  unsigned short  val;                                  \
  unsigned int A=0,B=0,C=0,D=0,E=0,F=0,G=0,H=0;

#define VTKKWRCHelper_InitializeMIPMultiTrilin()                \
  T *dptr;                                                      \
  unsigned int oldSPos[3];                                      \
                                                                \
  oldSPos[0] = (pos[0] >> VTKKW_FP_SHIFT) + 1;                  \
  oldSPos[1] = 0;                                               \
  oldSPos[2] = 0;                                               \
                                                                \
  unsigned int w1X, w1Y, w1Z;                                   \
  unsigned int w2X, w2Y, w2Z;                                   \
  unsigned int w1Xw1Y, w2Xw1Y, w1Xw2Y, w2Xw2Y;                  \
                                                                \
  unsigned short  maxValue[4] = {};                             \
  unsigned short  val[4] = {};                                  \
  unsigned int    A[4] = {}, B[4] = {}, C[4] = {}, D[4] = {},   \
                  E[4] = {}, F[4] = {}, G[4] = {}, H[4] = {};

#define VTKKWRCHelper_InitializeCompositeGONN()                                      \
  unsigned char *magPtr = gradientMag[spos[2]] + spos[0]*mInc[0] + spos[1]*mInc[1];

#define VTKKWRCHelper_InitializeCompositeShadeNN()                                      \
  unsigned short *dirPtr = gradientDir[spos[2]] + spos[0]*dInc[0] + spos[1]*dInc[1];

#define VTKKWRCHelper_InitializeCompositeOneNN()                        \
  mapper->ShiftVectorDown( pos, spos );                                 \
  T *dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];   \
  unsigned int color[3] = {0,0,0};                                      \
  unsigned short remainingOpacity = 0x7fff;                             \
  unsigned short tmp[4];

#define VTKKWRCHelper_InitializeCompositeMultiNN()                      \
  mapper->ShiftVectorDown( pos, spos );                                 \
  T *dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];   \
  unsigned int color[3] = {0,0,0};                                      \
  unsigned int remainingOpacity = 0x7fff;                               \
  unsigned short tmp[4] = {};                                           \
  unsigned short val[4] = {};

#define VTKKWRCHelper_InitializeCompositeOneTrilin()    \
  T *dptr;                                              \
  unsigned int oldSPos[3];                              \
                                                        \
  oldSPos[0] = (pos[0] >> VTKKW_FP_SHIFT) + 1;          \
  oldSPos[1] = 0;                                       \
  oldSPos[2] = 0;                                       \
                                                        \
  unsigned int w1X, w1Y, w1Z;                           \
  unsigned int w2X, w2Y, w2Z;                           \
  unsigned int w1Xw1Y, w2Xw1Y, w1Xw2Y, w2Xw2Y;          \
                                                        \
  unsigned short  val;                                  \
  unsigned int    A=0,B=0,C=0,D=0,E=0,F=0,G=0,H=0;      \
                                                        \
  unsigned int color[3] = {0,0,0};                      \
  unsigned short remainingOpacity = 0x7fff;             \
  unsigned short tmp[4];

#define VTKKWRCHelper_InitializeCompositeOneGOTrilin()          \
  unsigned char  *magPtrABCD = 0, *magPtrEFGH = 0;              \
  unsigned short  mag;                                          \
  unsigned int    mA=0,mB=0,mC=0,mD=0,mE=0,mF=0,mG=0,mH=0;

#define VTKKWRCHelper_InitializeCompositeOneShadeTrilin()       \
  unsigned short *dirPtrABCD = 0, *dirPtrEFGH = 0;              \
  unsigned int    normalA=0,normalB=0,normalC=0,normalD=0;      \
  unsigned int    normalE=0,normalF=0,normalG=0,normalH=0;

#define VTKKWRCHelper_InitializeCompositeMultiTrilin()          \
  T *dptr;                                                      \
  unsigned int oldSPos[3];                                      \
                                                                \
  oldSPos[0] = (pos[0] >> VTKKW_FP_SHIFT) + 1;                  \
  oldSPos[1] = 0;                                               \
  oldSPos[2] = 0;                                               \
                                                                \
  unsigned int w1X, w1Y, w1Z;                                   \
  unsigned int w2X, w2Y, w2Z;                                   \
  unsigned int w1Xw1Y, w2Xw1Y, w1Xw2Y, w2Xw2Y;                  \
                                                                \
  unsigned short  val[4] = {0, 0, 0, 0};                        \
  unsigned int    A[4]   = {0, 0, 0, 0};                        \
  unsigned int    B[4]   = {0, 0, 0, 0};                        \
  unsigned int    C[4]   = {0, 0, 0, 0};                        \
  unsigned int    D[4]   = {0, 0, 0, 0};                        \
  unsigned int    E[4]   = {0, 0, 0, 0};                        \
  unsigned int    F[4]   = {0, 0, 0, 0};                        \
  unsigned int    G[4]   = {0, 0, 0, 0};                        \
  unsigned int    H[4]   = {0, 0, 0, 0};                        \
                                                                \
  unsigned int color[3] = {0,0,0};                              \
  unsigned short remainingOpacity = 0x7fff;                     \
  unsigned short tmp[4];

#define VTKKWRCHelper_InitializeCompositeMultiGOTrilin()                \
  unsigned char  *magPtrABCD = 0, *magPtrEFGH = 0;                      \
  unsigned short  mag[4] = {};                                          \
  unsigned int    mA[4] = {}, mB[4] = {}, mC[4] = {},                   \
                  mD[4] = {}, mE[4] = {}, mF[4] = {},                   \
                  mG[4] = {}, mH[4] = {};

#define VTKKWRCHelper_InitializeCompositeMultiShadeTrilin()     \
  unsigned short *dirPtrABCD = 0, *dirPtrEFGH = 0;              \
  unsigned int    normalA[4],normalB[4],normalC[4],normalD[4];  \
  unsigned int    normalE[4],normalF[4],normalG[4],normalH[4];

#define VTKKWRCHelper_InitializationAndLoopStartNN()            \
  VTKKWRCHelper_InitializeVariables();                          \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartGONN()          \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeVariablesGO();                        \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartShadeNN()       \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeVariablesShade();                     \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartGOShadeNN()     \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeVariablesGO();                        \
  VTKKWRCHelper_InitializeVariablesShade();                     \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartTrilin()        \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeTrilinVariables();                    \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartGOTrilin()      \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeVariablesGO();                        \
  VTKKWRCHelper_InitializeTrilinVariables();                    \
  VTKKWRCHelper_InitializeTrilinVariablesGO();                  \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartShadeTrilin()   \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeVariablesShade();                     \
  VTKKWRCHelper_InitializeTrilinVariables();                    \
  VTKKWRCHelper_InitializeTrilinVariablesShade();               \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_InitializationAndLoopStartGOShadeTrilin() \
  VTKKWRCHelper_InitializeVariables();                          \
  VTKKWRCHelper_InitializeVariablesShade();                     \
  VTKKWRCHelper_InitializeVariablesGO();                        \
  VTKKWRCHelper_InitializeTrilinVariables();                    \
  VTKKWRCHelper_InitializeTrilinVariablesShade();               \
  VTKKWRCHelper_InitializeTrilinVariablesGO();                  \
  for ( j = 0; j < imageInUseSize[1]; j++ )                     \
  {                                                           \
    VTKKWRCHelper_OuterInitialization();                        \
    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )      \
    {                                                         \
      VTKKWRCHelper_InnerInitialization();

#define VTKKWRCHelper_IncrementAndLoopEnd()                                             \
      imagePtr+=4;                                                                      \
      }                                                                                 \
    if ( (j/threadCount)%8 == 7 && threadID == 0)                                       \
    {                                                                                 \
      double fargs[1];                                                                  \
      fargs[0] = static_cast<double>(j)/static_cast<float>(imageInUseSize[1]-1);        \
      mapper->InvokeEvent( vtkCommand::VolumeMapperRenderProgressEvent, fargs );        \
    }                                                                                 \
    }

#define VTKKWRCHelper_CroppingCheckTrilin( POS )        \
  if ( cropping )                                       \
  {                                                   \
    if ( mapper->CheckIfCropped( POS ) )                \
    {                                                 \
      continue;                                         \
    }                                                 \
  }

#define VTKKWRCHelper_CroppingCheckNN( POS )            \
  if ( cropping )                                       \
  {                                                   \
    if ( mapper->CheckIfCropped( POS ) )                \
    {                                                 \
      continue;                                         \
    }                                                 \
  }

#define VTKKWRCHelper_SpaceLeapSetup()          \
  unsigned int mmpos[3];                        \
  mmpos[0] = (pos[0] >> VTKKW_FPMM_SHIFT) + 1;  \
  mmpos[1] = 0;                                 \
  mmpos[2] = 0;                                 \
  int mmvalid = 0;

#define VTKKWRCHelper_SpaceLeapSetupMulti()     \
  unsigned int mmpos[3];                        \
  mmpos[0] = (pos[0] >> VTKKW_FPMM_SHIFT) + 1;  \
  mmpos[1] = 0;                                 \
  mmpos[2] = 0;                                 \
  int mmvalid[4] = {0,0,0,0};

#define VTKKWRCHelper_SpaceLeapCheck()                          \
  if ( pos[0] >> VTKKW_FPMM_SHIFT != mmpos[0] ||                \
       pos[1] >> VTKKW_FPMM_SHIFT != mmpos[1] ||                \
       pos[2] >> VTKKW_FPMM_SHIFT != mmpos[2] )                 \
  {                                                           \
    mmpos[0] = pos[0] >> VTKKW_FPMM_SHIFT;                      \
    mmpos[1] = pos[1] >> VTKKW_FPMM_SHIFT;                      \
    mmpos[2] = pos[2] >> VTKKW_FPMM_SHIFT;                      \
    mmvalid = mapper->CheckMinMaxVolumeFlag( mmpos, 0 );        \
  }                                                           \
                                                                \
  if ( !mmvalid )                                               \
  {                                                           \
    continue;                                                   \
  }

#define VTKKWRCHelper_MIPSpaceLeapCheck( MAXIDX, MAXIDXDEF, FLIP )      \
  if ( pos[0] >> VTKKW_FPMM_SHIFT != mmpos[0] ||                        \
       pos[1] >> VTKKW_FPMM_SHIFT != mmpos[1] ||                        \
       pos[2] >> VTKKW_FPMM_SHIFT != mmpos[2] )                         \
  {                                                                   \
    mmpos[0] = pos[0] >> VTKKW_FPMM_SHIFT;                              \
    mmpos[1] = pos[1] >> VTKKW_FPMM_SHIFT;                              \
    mmpos[2] = pos[2] >> VTKKW_FPMM_SHIFT;                              \
    mmvalid = (MAXIDXDEF)?                                              \
     (mapper->CheckMIPMinMaxVolumeFlag( mmpos, 0, MAXIDX, FLIP )):(1);  \
  }                                                                   \
                                                                        \
  if ( !mmvalid )                                                       \
  {                                                                   \
    continue;                                                           \
  }

#define VTKKWRCHelper_MIPSpaceLeapPopulateMulti( MAXIDX, FLIP )                   \
  if ( pos[0] >> VTKKW_FPMM_SHIFT != mmpos[0] ||                                  \
       pos[1] >> VTKKW_FPMM_SHIFT != mmpos[1] ||                                  \
       pos[2] >> VTKKW_FPMM_SHIFT != mmpos[2] )                                   \
  {                                                                             \
    mmpos[0] = pos[0] >> VTKKW_FPMM_SHIFT;                                        \
    mmpos[1] = pos[1] >> VTKKW_FPMM_SHIFT;                                        \
    mmpos[2] = pos[2] >> VTKKW_FPMM_SHIFT;                                        \
    for ( c = 0; c < components; c++ )                                            \
    {                                                                           \
      mmvalid[c] = mapper->CheckMIPMinMaxVolumeFlag( mmpos, c, MAXIDX[c], FLIP ); \
    }                                                                           \
  }

#define VTKKWRCHelper_MIPSpaceLeapCheckMulti( COMP, FLIP )  mmvalid[COMP]

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class vtkFixedPointVolumeRayCastMapper;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkFixedPointVolumeRayCastHelper : public vtkObject
{
public:
  static vtkFixedPointVolumeRayCastHelper *New();
  vtkTypeMacro(vtkFixedPointVolumeRayCastHelper,vtkObject);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  virtual void   GenerateImage( int,
                                int,
                                vtkVolume *,
                                vtkFixedPointVolumeRayCastMapper *) {}

protected:
  vtkFixedPointVolumeRayCastHelper();
  ~vtkFixedPointVolumeRayCastHelper() VTK_OVERRIDE;


private:
  vtkFixedPointVolumeRayCastHelper(const vtkFixedPointVolumeRayCastHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFixedPointVolumeRayCastHelper&) VTK_DELETE_FUNCTION;
};

#endif



