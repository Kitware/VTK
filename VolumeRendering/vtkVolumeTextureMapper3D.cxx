/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeTextureMapper3D.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRenderingFactory.h"


//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkVolumeTextureMapper3D);
//----------------------------------------------------------------------------

// This method moves the scalars from the input volume into volume1 (and
// possibly volume2) which are the 3D texture maps used for rendering.
//
// In the case where our volume is a power of two, the copy is done 
// directly. If we need to resample, then trilinear interpolation is used.
//
// A shift/scale is applied to the input scalar value to produce an 8 bit
// value for the texture volume.
//
// When the input data is one component, the scalar value is placed in the
// second component of the two component volume1. The first component is
// filled in later with the gradient magnitude.
// 
// When the input data is two component non-independent, the first component
// of the input data is placed in the first component of volume1, and the
// second component of the input data is placed in the third component of
// volume1. Volume1 has three components - the second is filled in later with
// the gradient magnitude.
//
// When the input data is four component non-independent, the first three
// components of the input data are placed in volume1 (which has three
// components), and the fourth component is placed in the second component
// of volume2. The first component of volume2 is later filled in with the
// gradient magnitude.

template <class T>
void vtkVolumeTextureMapper3DComputeScalars( T *dataPtr,
                                               vtkVolumeTextureMapper3D *me,
                                               float offset, float scale,
                                               unsigned char *volume1,
                                               unsigned char *volume2)
{
  T              *inPtr;
  unsigned char  *outPtr, *outPtr2;
  int             i, j, k;
  int             idx;

  int   inputDimensions[3];
  double inputSpacing[3];
  vtkImageData *input = me->GetInput();
  input->GetDimensions( inputDimensions );
  input->GetSpacing( inputSpacing );

  int   outputDimensions[3];
  float outputSpacing[3];
  me->GetVolumeDimensions( outputDimensions );
  me->GetVolumeSpacing( outputSpacing );

  int components = input->GetNumberOfScalarComponents();

  double wx, wy, wz;
  double fx, fy, fz;
  int x, y, z;

  double sampleRate[3];
  sampleRate[0] = outputSpacing[0] / static_cast<double>(inputSpacing[0]);
  sampleRate[1] = outputSpacing[1] / static_cast<double>(inputSpacing[1]);
  sampleRate[2] = outputSpacing[2] / static_cast<double>(inputSpacing[2]);

  // This is the case where no interpolation is needed
  if ( inputDimensions[0] == outputDimensions[0] &&
       inputDimensions[1] == outputDimensions[1] &&
       inputDimensions[2] == outputDimensions[2] )
    {
    int size = outputDimensions[0] * outputDimensions[1] * outputDimensions[2];

    inPtr = dataPtr;
    if ( components == 1 )
      {
      outPtr = volume1;
      if ( scale == 1.0 )
        {
        for ( i = 0; i < size; i++ )
          {
          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr++) = 0;
          *(outPtr++) = idx;
          }
        }
      else
        {
        for ( i = 0; i < size; i++ )
          {
          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr++) = 0;
          *(outPtr++) = idx;
          }
        }
      }
    else if ( components == 2 )
      {
      outPtr = volume1;
      if ( scale == 1.0 )
        {
        for ( i = 0; i < size; i++ )
          {
          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr++) = idx;

          *(outPtr++) = 0;

          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr++) = idx;
          }
        }
      else
        {
        for ( i = 0; i < size; i++ )
          {
          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr++) = idx;

          *(outPtr++) = 0;

          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr++) = idx;
          }
        }
      }
    else if ( components == 4 )
      {
      outPtr = volume1;
      outPtr2 = volume2;
      if ( scale == 1.0 )
        {
        for ( i = 0; i < size; i++ )
          {
          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr++) = idx;
          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr++) = idx;
          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr++) = idx;

          *(outPtr2++) = 0;
          idx = static_cast<int>(*(inPtr++) + offset);
          *(outPtr2++) = idx;
          }
        }
      else
        {
        for ( i = 0; i < size; i++ )
          {
          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr++) = idx;
          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr++) = idx;
          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr++) = idx;

          *(outPtr2++) = 0;
          idx = static_cast<int>((*(inPtr++) + offset) * scale);
          *(outPtr2++) = idx;
          }
        }
      }
    }
  // The sizes are different and interpolation is required
  else
    {
    outPtr  = volume1;
    outPtr2 = volume2;
 
    for ( k = 0; k < outputDimensions[2]; k++ )
      {
      fz = k * sampleRate[2];
      fz = (fz >= inputDimensions[2]-1)?(inputDimensions[2]-1.001):(fz);
      z  = vtkMath::Floor( fz );
      wz = fz - z;
      for ( j = 0; j < outputDimensions[1]; j++ )
        {
        fy = j * sampleRate[1];
        fy = (fy >= inputDimensions[1]-1)?(inputDimensions[1]-1.001):(fy);
        y  = vtkMath::Floor( fy );
        wy = fy - y;
        for ( i = 0; i < outputDimensions[0]; i++ )
          {
          fx = i * sampleRate[0];
          fx = (fx >= inputDimensions[0]-1)?(inputDimensions[0]-1.001):(fx);
          x  = vtkMath::Floor( fx );
          wx = fx - x;

          inPtr = 
            dataPtr + components * ( z*inputDimensions[0]*inputDimensions[1] +
                                     y*inputDimensions[0] +
                                     x );
          
          if ( components == 1 )
            {
            float A, B, C, D, E, F, G, H;
            A = static_cast<float>(*(inPtr));
            B = static_cast<float>(*(inPtr+1));
            C = static_cast<float>(*(inPtr+inputDimensions[0]));
            D = static_cast<float>(*(inPtr+inputDimensions[0]+1));
            E = static_cast<float>(*(inPtr+inputDimensions[0]*inputDimensions[1]));
            F = static_cast<float>(*(inPtr+inputDimensions[0]*inputDimensions[1]+1));
            G = static_cast<float>(*(inPtr+inputDimensions[0]*inputDimensions[1]+inputDimensions[0]));
            H = static_cast<float>(*(inPtr+inputDimensions[0]*inputDimensions[1]+inputDimensions[0]+1));

            float val = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*A +
              (    wx)*(1.0-wy)*(1.0-wz)*B +
              (1.0-wx)*(    wy)*(1.0-wz)*C +
              (    wx)*(    wy)*(1.0-wz)*D +
              (1.0-wx)*(1.0-wy)*(    wz)*E +
              (    wx)*(1.0-wy)*(    wz)*F +
              (1.0-wx)*(    wy)*(    wz)*G +
              (    wx)*(    wy)*(    wz)*H;

            idx = static_cast<int>((val + offset)*scale);
            *(outPtr++) = 0;
            *(outPtr++) = idx;
            }
          else if ( components == 2 )
            {
            float A1, B1, C1, D1, E1, F1, G1, H1;
            float A2, B2, C2, D2, E2, F2, G2, H2;
            A1 = static_cast<float>(*(inPtr));
            A2 = static_cast<float>(*(inPtr+1));
            B1 = static_cast<float>(*(inPtr+2));
            B2 = static_cast<float>(*(inPtr+3));
            C1 = static_cast<float>(*(inPtr+2*inputDimensions[0]));
            C2 = static_cast<float>(*(inPtr+2*inputDimensions[0]+1));
            D1 = static_cast<float>(*(inPtr+2*inputDimensions[0]+2));
            D2 = static_cast<float>(*(inPtr+2*inputDimensions[0]+3));
            E1 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]));
            E2 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+1));
            F1 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+2));
            F2 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+3));
            G1 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+2*inputDimensions[0]));
            G2 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+2*inputDimensions[0]+1));
            H1 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+2*inputDimensions[0]+2));
            H2 = static_cast<float>(*(inPtr+2*inputDimensions[0]*inputDimensions[1]+2*inputDimensions[0]+3));

            float val1 = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*A1 +
              (    wx)*(1.0-wy)*(1.0-wz)*B1 +
              (1.0-wx)*(    wy)*(1.0-wz)*C1 +
              (    wx)*(    wy)*(1.0-wz)*D1 +
              (1.0-wx)*(1.0-wy)*(    wz)*E1 +
              (    wx)*(1.0-wy)*(    wz)*F1 +
              (1.0-wx)*(    wy)*(    wz)*G1 +
              (    wx)*(    wy)*(    wz)*H1;


            float val2 = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*A2 +
              (    wx)*(1.0-wy)*(1.0-wz)*B2 +
              (1.0-wx)*(    wy)*(1.0-wz)*C2 +
              (    wx)*(    wy)*(1.0-wz)*D2 +
              (1.0-wx)*(1.0-wy)*(    wz)*E2 +
              (    wx)*(1.0-wy)*(    wz)*F2 +
              (1.0-wx)*(    wy)*(    wz)*G2 +
              (    wx)*(    wy)*(    wz)*H2;

            idx = static_cast<int>((val1 + offset) * scale);
            *(outPtr++) = idx;

            *(outPtr++) = 0;

            idx = static_cast<int>((val2 + offset) * scale);
            *(outPtr++) = idx;
            }
          else 
            {
            float Ar, Br, Cr, Dr, Er, Fr, Gr, Hr;
            float Ag, Bg, Cg, Dg, Eg, Fg, Gg, Hg;
            float Ab, Bb, Cb, Db, Eb, Fb, Gb, Hb;
            float Aa, Ba, Ca, Da, Ea, Fa, Ga, Ha;
            Ar = static_cast<float>(*(inPtr));
            Ag = static_cast<float>(*(inPtr+1));
            Ab = static_cast<float>(*(inPtr+2));
            Aa = static_cast<float>(*(inPtr+3));
            Br = static_cast<float>(*(inPtr+4));
            Bg = static_cast<float>(*(inPtr+5));
            Bb = static_cast<float>(*(inPtr+6));
            Ba = static_cast<float>(*(inPtr+7));
            Cr = static_cast<float>(*(inPtr+4*inputDimensions[0]));
            Cg = static_cast<float>(*(inPtr+4*inputDimensions[0]+1));
            Cb = static_cast<float>(*(inPtr+4*inputDimensions[0]+2));
            Ca = static_cast<float>(*(inPtr+4*inputDimensions[0]+3));
            Dr = static_cast<float>(*(inPtr+4*inputDimensions[0]+4));
            Dg = static_cast<float>(*(inPtr+4*inputDimensions[0]+5));
            Db = static_cast<float>(*(inPtr+4*inputDimensions[0]+6));
            Da = static_cast<float>(*(inPtr+4*inputDimensions[0]+7));
            Er = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]));
            Eg = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+1));
            Eb = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+2));
            Ea = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+3));
            Fr = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4));
            Fg = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+5));
            Fb = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+6));
            Fa = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+7));
            Gr = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]));
            Gg = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+1));
            Gb = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+2));
            Ga = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+3));
            Hr = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+4));
            Hg = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+5));
            Hb = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+6));
            Ha = static_cast<float>(*(inPtr+4*inputDimensions[0]*inputDimensions[1]+4*inputDimensions[0]+7));

            float valr = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*Ar +
              (    wx)*(1.0-wy)*(1.0-wz)*Br +
              (1.0-wx)*(    wy)*(1.0-wz)*Cr +
              (    wx)*(    wy)*(1.0-wz)*Dr +
              (1.0-wx)*(1.0-wy)*(    wz)*Er +
              (    wx)*(1.0-wy)*(    wz)*Fr +
              (1.0-wx)*(    wy)*(    wz)*Gr +
              (    wx)*(    wy)*(    wz)*Hr;

            float valg = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*Ag +
              (    wx)*(1.0-wy)*(1.0-wz)*Bg +
              (1.0-wx)*(    wy)*(1.0-wz)*Cg +
              (    wx)*(    wy)*(1.0-wz)*Dg +
              (1.0-wx)*(1.0-wy)*(    wz)*Eg +
              (    wx)*(1.0-wy)*(    wz)*Fg +
              (1.0-wx)*(    wy)*(    wz)*Gg +
              (    wx)*(    wy)*(    wz)*Hg;

            float valb = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*Ab +
              (    wx)*(1.0-wy)*(1.0-wz)*Bb +
              (1.0-wx)*(    wy)*(1.0-wz)*Cb +
              (    wx)*(    wy)*(1.0-wz)*Db +
              (1.0-wx)*(1.0-wy)*(    wz)*Eb +
              (    wx)*(1.0-wy)*(    wz)*Fb +
              (1.0-wx)*(    wy)*(    wz)*Gb +
              (    wx)*(    wy)*(    wz)*Hb;

            float vala = 
              (1.0-wx)*(1.0-wy)*(1.0-wz)*Aa +
              (    wx)*(1.0-wy)*(1.0-wz)*Ba +
              (1.0-wx)*(    wy)*(1.0-wz)*Ca +
              (    wx)*(    wy)*(1.0-wz)*Da +
              (1.0-wx)*(1.0-wy)*(    wz)*Ea +
              (    wx)*(1.0-wy)*(    wz)*Fa +
              (1.0-wx)*(    wy)*(    wz)*Ga +
              (    wx)*(    wy)*(    wz)*Ha;

            idx = static_cast<int>((valr + offset) * scale);
            *(outPtr++) = idx;
            idx = static_cast<int>((valg + offset) * scale);
            *(outPtr++) = idx;
            idx = static_cast<int>((valb + offset) * scale);
            *(outPtr++) = idx;

            *(outPtr2++) = 0;
            idx = static_cast<int>((vala + offset) * scale);
            *(outPtr2++) = idx;
            }
          }
        }
      }
    }
}


//-----------------------------------------------------------------------------
template <class T>
void vtkVolumeTextureMapper3DComputeGradients( T *dataPtr,
                                                 vtkVolumeTextureMapper3D *me,
                                                 double scalarRange[2],
                                                 unsigned char *volume1,
                                                 unsigned char *volume2,
                                                 unsigned char *volume3)
{
  int                 x, y, z;
  int                 offset, outputOffset;
  int                 x_start, x_limit;
  int                 y_start, y_limit;
  int                 z_start, z_limit;
  T                   *dptr;
  double               n[3], t;
  double               gvalue;
  double               zeroNormalThreshold;
  int                 xlow, xhigh;
  double              aspect[3];
  unsigned char       *outPtr1, *outPtr2;
  unsigned char       *normals, *gradmags;
  int                 gradmagIncrement;
  int                 gradmagOffset;
  double              floc[3];
  int                 loc[3];

//  me->InvokeEvent( vtkEvent::VolumeMapperComputeGradientsStartEvent, NULL );

  float outputSpacing[3];
  me->GetVolumeSpacing( outputSpacing );

  double spacing[3];
  vtkImageData *input = me->GetInput();
  input->GetSpacing( spacing );

  double sampleRate[3];
  sampleRate[0] = outputSpacing[0] / static_cast<double>(spacing[0]);
  sampleRate[1] = outputSpacing[1] / static_cast<double>(spacing[1]);
  sampleRate[2] = outputSpacing[2] / static_cast<double>(spacing[2]);
 
  int components = input->GetNumberOfScalarComponents();
 
  int dim[3];
  input->GetDimensions(dim);

  int outputDim[3];
  me->GetVolumeDimensions( outputDim );

  double avgSpacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;

  // adjust the aspect
  aspect[0] = spacing[0] * 2.0 / avgSpacing;
  aspect[1] = spacing[1] * 2.0 / avgSpacing;
  aspect[2] = spacing[2] * 2.0 / avgSpacing;
  
  double scale = 255.0 / (0.25*(scalarRange[1] - scalarRange[0]));

  // Get the length at or below which normals are considered to
  // be "zero"
  zeroNormalThreshold =.001 * (scalarRange[1] - scalarRange[0]);

  int thread_id = 0;
  int thread_count = 1;

  x_start = 0;
  x_limit = outputDim[0];
  y_start = 0;
  y_limit = outputDim[1];
  z_start = static_cast<int>(( thread_id / static_cast<float>(thread_count) ) *
                  outputDim[2] );
  z_limit = static_cast<int>(( (thread_id + 1) / static_cast<float>(thread_count) ) *
                  outputDim[2] );

  // Do final error checking on limits - make sure they are all within bounds
  // of the scalar input

  x_start = (x_start<0)?(0):(x_start);
  y_start = (y_start<0)?(0):(y_start);
  z_start = (z_start<0)?(0):(z_start);

  x_limit = (x_limit>dim[0])?(outputDim[0]):(x_limit);
  y_limit = (y_limit>dim[1])?(outputDim[1]):(y_limit);
  z_limit = (z_limit>dim[2])?(outputDim[2]):(z_limit);


  if ( components == 1 || components == 2 )
    {
    normals = volume2;
    gradmags = volume1;
    gradmagIncrement = components+1;
    gradmagOffset = components-1;
    }
  else 
    {
    normals = volume3;
    gradmags = volume2;
    gradmagIncrement = 2;
    gradmagOffset = 0;
    }

  double wx, wy, wz;

  // Loop through all the data and compute the encoded normal and
  // gradient magnitude for each scalar location
  for ( z = z_start; z < z_limit; z++ )
    {
    floc[2] = z*sampleRate[2];
    floc[2] = (floc[2]>=(dim[2]-1))?(dim[2]-1.001):(floc[2]);
    loc[2]  = vtkMath::Floor(floc[2]);
    wz = floc[2] - loc[2];

    for ( y = y_start; y < y_limit; y++ )
      {
      floc[1] = y*sampleRate[1];
      floc[1] = (floc[1]>=(dim[1]-1))?(dim[1]-1.001):(floc[1]);
      loc[1]  = vtkMath::Floor(floc[1]);
      wy = floc[1] - loc[1];

      xlow = x_start;
      xhigh = x_limit;
      outputOffset = z * outputDim[0] * outputDim[1] + y * outputDim[0] + xlow;

      // Set some pointers
      outPtr1 = gradmags + gradmagIncrement*outputOffset;
      outPtr2 = normals + 3*outputOffset;

      for ( x = xlow; x < xhigh; x++ )
        {
        floc[0] = x*sampleRate[0];
        floc[0] = (floc[0]>=(dim[0]-1))?(dim[0]-1.001):(floc[0]);
        loc[0]  = vtkMath::Floor(floc[0]);
        wx = floc[0] - loc[0];

        offset = loc[2] * dim[0] * dim[1] + loc[1] * dim[0] + loc[0];

        dptr = dataPtr + components*offset + components - 1;

        // Use a central difference method if possible,
        // otherwise use a forward or backward difference if
        // we are on the edge
        int sampleOffset[6];
        sampleOffset[0] = (loc[0]<1)        ?(0):(-components);
        sampleOffset[1] = (loc[0]>=dim[0]-2)?(0):( components);
        sampleOffset[2] = (loc[1]<1)        ?(0):(-components*dim[0]);
        sampleOffset[3] = (loc[1]>=dim[1]-2)?(0):( components*dim[0]);
        sampleOffset[4] = (loc[2]<1)        ?(0):(-components*dim[0]*dim[1]);
        sampleOffset[5] = (loc[2]>=dim[2]-2)?(0):( components*dim[0]*dim[1]);

        float sample[6];
        for ( int i = 0; i < 6; i++ )
          {
          float A, B, C, D, E, F, G, H;
          T *samplePtr = dptr + sampleOffset[i];

          A = static_cast<float>(*(samplePtr));
          B = static_cast<float>(*(samplePtr+components));
          C = static_cast<float>(*(samplePtr+components*dim[0]));
          D = static_cast<float>(*(samplePtr+components*dim[0]+components));
          E = static_cast<float>(*(samplePtr+components*dim[0]*dim[1]));
          F = static_cast<float>(*(samplePtr+components*dim[0]*dim[1]+components));
          G = static_cast<float>(*(samplePtr+components*dim[0]*dim[1]+components*dim[0]));
          H = static_cast<float>(*(samplePtr+components*dim[0]*dim[1]+components*dim[0]+components));

          sample[i] = 
            (1.0-wx)*(1.0-wy)*(1.0-wz)*A +
            (    wx)*(1.0-wy)*(1.0-wz)*B +
            (1.0-wx)*(    wy)*(1.0-wz)*C +
            (    wx)*(    wy)*(1.0-wz)*D +
            (1.0-wx)*(1.0-wy)*(    wz)*E +
            (    wx)*(1.0-wy)*(    wz)*F +
            (1.0-wx)*(    wy)*(    wz)*G +
            (    wx)*(    wy)*(    wz)*H;
          }

        n[0] = ((sampleOffset[0]==0 || sampleOffset[1]==0)?(2.0):(1.0))*(sample[0] -sample[1]);
        n[1] = ((sampleOffset[2]==0 || sampleOffset[3]==0)?(2.0):(1.0))*(sample[2] -sample[3]);
        n[2] = ((sampleOffset[4]==0 || sampleOffset[5]==0)?(2.0):(1.0))*(sample[4] -sample[5]);

        // Take care of the aspect ratio of the data
        // Scaling in the vtkVolume is isotropic, so this is the
        // only place we have to worry about non-isotropic scaling.
        n[0] /= aspect[0];
        n[1] /= aspect[1];
        n[2] /= aspect[2];

        // Compute the gradient magnitude
        t = sqrt( n[0]*n[0] + n[1]*n[1] + n[2]*n[2] );

        // Encode this into an 4 bit value 
        gvalue = t * scale; 

        gvalue = (gvalue<0.0)?(0.0):(gvalue);
        gvalue = (gvalue>255.0)?(255.0):(gvalue);


        *(outPtr1+gradmagOffset) = static_cast<unsigned char>(gvalue + 0.5);

        // Normalize the gradient direction
        if ( t > zeroNormalThreshold )
          {
          n[0] /= t;
          n[1] /= t;
          n[2] /= t;
          }
        else
          {
          n[0] = n[1] = n[2] = 0.0;
          }

        int nx = static_cast<int>((n[0] / 2.0 + 0.5)*255.0 + 0.5);
        int ny = static_cast<int>((n[1] / 2.0 + 0.5)*255.0 + 0.5);
        int nz = static_cast<int>((n[2] / 2.0 + 0.5)*255.0 + 0.5);

        nx = (nx<0)?(0):(nx);
        ny = (ny<0)?(0):(ny);
        nz = (nz<0)?(0):(nz);

        nx = (nx>255)?(255):(nx);
        ny = (ny>255)?(255):(ny);
        nz = (nz>255)?(255):(nz);

        *(outPtr2  ) = nx;
        *(outPtr2+1) = ny;
        *(outPtr2+2) = nz;

        outPtr1 += gradmagIncrement;
        outPtr2 += 3;
        }
      }
//    if ( z%8 == 7 )
//      {
//      float args[1];
//      args[0] = 
//        static_cast<float>(z - z_start) / 
//        static_cast<float>(z_limit - z_start - 1);
//      me->InvokeEvent( vtkEvent::VolumeMapperComputeGradientsProgressEvent, args );
//      }
    }
//  me->InvokeEvent( vtkEvent::VolumeMapperComputeGradientsEndEvent, NULL );
}


//-----------------------------------------------------------------------------
vtkVolumeTextureMapper3D::vtkVolumeTextureMapper3D()
{
  this->PolygonBuffer                 = NULL;
  this->IntersectionBuffer            = NULL;
  this->NumberOfPolygons              = 0;
  this->BufferSize                    = 0;

  // The input used when creating the textures
  this->SavedTextureInput             = NULL;
  
  // The input used when creating the color tables
  this->SavedParametersInput           = NULL;
  
  this->SavedRGBFunction              = NULL;
  this->SavedGrayFunction             = NULL;
  this->SavedScalarOpacityFunction    = NULL;
  this->SavedGradientOpacityFunction  = NULL;
  this->SavedColorChannels            = 0;
  this->SavedSampleDistance           = 0;
  this->SavedScalarOpacityDistance    = 0;

  this->Volume1                       = NULL;
  this->Volume2                       = NULL;
  this->Volume3                       = NULL;
  this->VolumeSize                    = 0;
  this->VolumeComponents              = 0;
  this->VolumeSpacing[0] = this->VolumeSpacing[1] = this->VolumeSpacing[2] = 0;
  this->VolumeDimensions[0]=0;
  this->VolumeDimensions[1]=0;
  this->VolumeDimensions[2]=0;
  
  this->SampleDistance                = 1.0;
  this->ActualSampleDistance          = 1.0;
  
  this->RenderMethod                  = vtkVolumeTextureMapper3D::NO_METHOD;
  this->PreferredRenderMethod         =
    vtkVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD;
  
  this->UseCompressedTexture          = false;
  this->SupportsNonPowerOfTwoTextures = false;
}

//-----------------------------------------------------------------------------
vtkVolumeTextureMapper3D::~vtkVolumeTextureMapper3D()
{
  delete [] this->PolygonBuffer;
  delete [] this->IntersectionBuffer;
  delete [] this->Volume1;
  delete [] this->Volume2;
  delete [] this->Volume3;
}


//-----------------------------------------------------------------------------
vtkVolumeTextureMapper3D *vtkVolumeTextureMapper3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkVolumeRenderingFactory::CreateInstance("vtkVolumeTextureMapper3D");
  return static_cast<vtkVolumeTextureMapper3D *>(ret);
}

//-----------------------------------------------------------------------------
void vtkVolumeTextureMapper3D::ComputePolygons( vtkRenderer *ren, 
                                                vtkVolume *vol,
                                                double inBounds[6] )
{
  // Get the camera position and focal point
  double focalPoint[4], position[4];
  double plane[4];
  vtkCamera *camera = ren->GetActiveCamera();

  camera->GetPosition( position );
  camera->GetFocalPoint( focalPoint );
 
  position[3]   = 1.0;
  focalPoint[3] = 1.0;
  
  // Pass the focal point and position through the inverse of the 
  // volume's matrix to map back into the data coordinates. We
  // are going to compute these polygons in the coordinate system
  // of the input data - this is easiest since this data must be 
  // axis aligned. Then we'll use OpenGL to transform these polygons
  // into the world coordinate system through the use of the
  // volume's matrix.
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  vol->GetMatrix( matrix );
  matrix->Invert();
  matrix->MultiplyPoint( position, position );
  matrix->MultiplyPoint( focalPoint, focalPoint );
  matrix->Delete();
  
  if ( position[3] )
    {
    position[0] /= position[3];
    position[1] /= position[3];
    position[2] /= position[3];
    }

  if ( focalPoint[3] )
    {
    focalPoint[0] /= focalPoint[3];
    focalPoint[1] /= focalPoint[3];
    focalPoint[2] /= focalPoint[3];
    }

  // Create a plane equation using the direction and position of the camera
  plane[0] = focalPoint[0] - position[0];
  plane[1] = focalPoint[1] - position[1];
  plane[2] = focalPoint[2] - position[2];
  
  vtkMath::Normalize( plane );
  
  plane[3] = -(plane[0] * position[0] + plane[1] * position[1] +
               plane[2] * position[2]);
 
  // Find the min and max distances of the boundary points of the volume
  double minDistance = VTK_DOUBLE_MAX;
  double maxDistance = VTK_DOUBLE_MIN;
 
  // The inBounds parameter is the bounds we are using for clipping the
  // texture planes against. First we need to clip these against the bounds
  // of the volume to make sure they don't exceed it.
  double volBounds[6];
  this->GetInput()->GetBounds( volBounds );

  double bounds[6];
  bounds[0] = (inBounds[0]>volBounds[0])?(inBounds[0]):(volBounds[0]);
  bounds[1] = (inBounds[1]<volBounds[1])?(inBounds[1]):(volBounds[1]);
  bounds[2] = (inBounds[2]>volBounds[2])?(inBounds[2]):(volBounds[2]);
  bounds[3] = (inBounds[3]<volBounds[3])?(inBounds[3]):(volBounds[3]);
  bounds[4] = (inBounds[4]>volBounds[4])?(inBounds[4]):(volBounds[4]);
  bounds[5] = (inBounds[5]<volBounds[5])?(inBounds[5]):(volBounds[5]);
 
  // Create 8 vertices for the bounding box we are rendering
  int i, j, k;
  double vertices[8][3];
 
  int idx = 0;
 
  for ( k = 0; k < 2; k++ )
    {
    for ( j = 0; j < 2; j++ )
      {
      for ( i = 0; i < 2; i++ )
        {
        vertices[idx][2] = bounds[4+k];
        vertices[idx][1] = bounds[2+j];
        vertices[idx][0] = bounds[i];

        double d = 
          plane[0] * vertices[idx][0] +
          plane[1] * vertices[idx][1] +
          plane[2] * vertices[idx][2] +
          plane[3];

        idx++;

        // Keep track of closest and farthest point
        minDistance = (d<minDistance)?(d):(minDistance);
        maxDistance = (d>maxDistance)?(d):(maxDistance);

        }
      }
    }

  int dim[6];
  this->GetVolumeDimensions(dim);
 
  float tCoordOffset[3], tCoordScale[3];

  tCoordOffset[0] = 0.5 / dim[0];
  tCoordOffset[1] = 0.5 / dim[1];
  tCoordOffset[2] = 0.5 / dim[2];
 
  tCoordScale[0] =  (dim[0]-1) / static_cast<float>(dim[0]);
  tCoordScale[1] =  (dim[1]-1) / static_cast<float>(dim[1]);
  tCoordScale[2] =  (dim[2]-1) / static_cast<float>(dim[2]);

  float spacing[3];
  this->GetVolumeSpacing( spacing );
 
  double offset = 
    0.333 * 0.5 * (spacing[0] + spacing[1] + spacing[2]);
 
  minDistance += 0.1*offset;
  maxDistance -= 0.1*offset;
 
  minDistance = (minDistance < offset)?(offset):(minDistance);
 
  double stepSize = this->ActualSampleDistance;
 
  // Determine the number of polygons
  int numPolys = static_cast<int>(
    (maxDistance - minDistance)/static_cast<double>(stepSize));
 
  // Check if we have space, free old space only if it is too small 
  if ( this->BufferSize < numPolys )
    {
    delete [] this->PolygonBuffer;
    delete [] this->IntersectionBuffer;

    this->BufferSize = numPolys;

    this->PolygonBuffer = new float [36*this->BufferSize];
    this->IntersectionBuffer = new float [12*this->BufferSize];
    }
 
  this->NumberOfPolygons = numPolys;
 
  // Compute the intersection points for each edge of the volume
  int lines[12][2] = { {0,1}, {1,3}, {2,3}, {0,2},
                       {4,5}, {5,7}, {6,7}, {4,6},
                       {0,4}, {1,5}, {3,7}, {2,6} };
 
  float *iptr, *pptr;
 
  for ( i = 0; i < 12; i++ )
    {
    double line[3];

    line[0] = vertices[lines[i][1]][0] - vertices[lines[i][0]][0];
    line[1] = vertices[lines[i][1]][1] - vertices[lines[i][0]][1];
    line[2] = vertices[lines[i][1]][2] - vertices[lines[i][0]][2];
 
    double d = maxDistance;
 
    iptr = this->IntersectionBuffer + i;

    double planeDotLineOrigin = vtkMath::Dot( plane, vertices[lines[i][0]] );
    double planeDotLine       = vtkMath::Dot( plane, line );
 
    double t, increment;

    if ( planeDotLine != 0.0 )
      {
      t = (d - planeDotLineOrigin - plane[3] ) / planeDotLine;
      increment = -stepSize / planeDotLine;
      }
    else
      {
      t         = -1.0;
      increment =  0.0;
      }

    for ( j = 0; j < numPolys; j++ )
      {
      *iptr = (t > 0.0 && t < 1.0)?(t):(-1.0);

      t += increment;
      iptr += 12;
      }
    }

  // Compute the polygons by determining which edges were intersected
  int neighborLines[12][6] = 
  { {  1,  2,  3,  4,  8,  9}, {  0,  2,  3,  5,  9, 10},
    {  0,  1,  3,  6, 10, 11}, {  0,  1,  2,  7,  8, 11},
    {  0,  5,  6,  7,  8,  9}, {  1,  4,  6,  7,  9, 10},
    {  2,  4,  5,  7, 10, 11}, {  3,  4,  5,  6,  8, 11},
    {  0,  3,  4,  7,  9, 11}, {  0,  1,  4,  5,  8, 10},
    {  1,  2,  5,  6,  9, 11}, {  2,  3,  6,  7,  8, 10} };

  float tCoord[12][4] =
  {{0,0,0,0}, {1,0,0,1}, {0,1,0,0}, {0,0,0,1},
   {0,0,1,0}, {1,0,1,1}, {0,1,1,0}, {0,0,1,1},
   {0,0,0,2}, {1,0,0,2}, {1,1,0,2}, {0,1,0,2}};
 
  double low[3];
  double high[3];

  low[0]  = (bounds[0] - volBounds[0]) / (volBounds[1] - volBounds[0]);
  high[0] = (bounds[1] - volBounds[0]) / (volBounds[1] - volBounds[0]);
  low[1]  = (bounds[2] - volBounds[2]) / (volBounds[3] - volBounds[2]);
  high[1] = (bounds[3] - volBounds[2]) / (volBounds[3] - volBounds[2]);
  low[2]  = (bounds[4] - volBounds[4]) / (volBounds[5] - volBounds[4]);
  high[2] = (bounds[5] - volBounds[4]) / (volBounds[5] - volBounds[4]);

  for ( i = 0; i < 12; i++ )
    {
    tCoord[i][0] = (tCoord[i][0])?(high[0]):(low[0]);
    tCoord[i][1] = (tCoord[i][1])?(high[1]):(low[1]);
    tCoord[i][2] = (tCoord[i][2])?(high[2]):(low[2]);
    }

  iptr = this->IntersectionBuffer;
  pptr = this->PolygonBuffer;
 
  for ( i = 0; i < numPolys; i++ )
    {
    // Look for a starting point
    int start = 0;

    while ( start < 12 && iptr[start] == -1.0 )
      {
      start++;
      }

    if ( start == 12 )
      {
      pptr[0] = -1.0;
      }
    else
      {
      int current = start;
      int previous = -1;
      int errFlag = 0;

      idx   = 0;

      while ( idx < 6 && !errFlag && ( idx == 0 || current != start) )
        {
        double t = iptr[current];

        *(pptr + idx*6)     = 
          tCoord[current][0] * tCoordScale[0] + tCoordOffset[0];
        *(pptr + idx*6 + 1) = 
          tCoord[current][1] * tCoordScale[1] + tCoordOffset[1];
        *(pptr + idx*6 + 2) = 
          tCoord[current][2] * tCoordScale[2] + tCoordOffset[2];
        
        int coord = static_cast<int>(tCoord[current][3]);
        *(pptr + idx*6 + coord) = 
          (low[coord] + t*(high[coord]-low[coord]))*tCoordScale[coord] + tCoordOffset[coord];

        *(pptr + idx*6 + 3) = static_cast<float>(
          vertices[lines[current][0]][0] + 
          t*(vertices[lines[current][1]][0] - vertices[lines[current][0]][0]));
        
        *(pptr + idx*6 + 4) = static_cast<float>(
          vertices[lines[current][0]][1] + 
          t*(vertices[lines[current][1]][1] - vertices[lines[current][0]][1]));
        
        *(pptr + idx*6 + 5) = static_cast<float>(
          vertices[lines[current][0]][2] + 
          t*(vertices[lines[current][1]][2] - vertices[lines[current][0]][2]));

        idx++;

        j = 0;

        while ( j < 6 &&
                (*(this->IntersectionBuffer + i*12 + 
                   neighborLines[current][j]) < 0 || 
                 neighborLines[current][j] == previous) ) 
          {
          j++;
          }

        if ( j >= 6 )
          {
          errFlag = 1;
          }
        else
          {
          previous = current;
          current = neighborLines[current][j];
          }
        }

      if ( idx < 6 )
        {
        *(pptr + idx*6) = -1;
        }
      }

    iptr += 12;
    pptr += 36;
    }
}

//-----------------------------------------------------------------------------
int vtkVolumeTextureMapper3D::UpdateVolumes(vtkVolume *vtkNotUsed(vol))
{
  int needToUpdate = 0;

  // Get the image data
  vtkImageData *input = this->GetInput();
  input->Update();
 
  // Has the volume changed in some way?
  if ( this->SavedTextureInput != input ||
       this->SavedTextureMTime.GetMTime() < input->GetMTime() )
    {
    needToUpdate = 1;
    }
 
  if ( !needToUpdate )
    {
    return 0;
    }
 
  this->SavedTextureInput = input;
  this->SavedTextureMTime.Modified();

  // How big does the Volume need to be?
  int dim[3];
  input->GetDimensions(dim);

  int components = input->GetNumberOfScalarComponents();
  
  int powerOfTwoDim[3];
  
  if(this->SupportsNonPowerOfTwoTextures)
    {
     for ( int i = 0; i < 3; i++ )
       {
       powerOfTwoDim[i]=dim[i];
       }
    }
  else
    {
    for ( int i = 0; i < 3; i++ )
      {
      powerOfTwoDim[i] = 32;
      while ( powerOfTwoDim[i] < dim[i] )
        {
        powerOfTwoDim[i] *= 2;
        }
      }
    }
 
  while ( ! this->IsTextureSizeSupported( powerOfTwoDim,components ) )
    {
    if ( powerOfTwoDim[0] >= powerOfTwoDim[1] &&
         powerOfTwoDim[0] >= powerOfTwoDim[2] )
      {
      powerOfTwoDim[0] /= 2;
      }
    else if ( powerOfTwoDim[1] >= powerOfTwoDim[0] &&
              powerOfTwoDim[1] >= powerOfTwoDim[2] )
      {
      powerOfTwoDim[1] /= 2;
      }
    else
      {
      powerOfTwoDim[2] /= 2;
      }
    }
 
  int neededSize = powerOfTwoDim[0] * powerOfTwoDim[1] * powerOfTwoDim[2];
 
  // What is the spacing?
  double spacing[3];
  input->GetSpacing(spacing);
 
  // Is it the right size? If not, allocate it.
  if ( this->VolumeSize != neededSize ||
       this->VolumeComponents != components )
    {
    delete [] this->Volume1;
    delete [] this->Volume2;
    delete [] this->Volume3;
    switch (components)
      {
      case 1:
        this->Volume1 = new unsigned char [2*neededSize];
        this->Volume2 = new unsigned char [3*neededSize];
        this->Volume3 = NULL;
        break;
      case 2:
        this->Volume1 = new unsigned char [3*neededSize];
        this->Volume2 = new unsigned char [3*neededSize];
        this->Volume3 = NULL;
        break;
      case 3:
      case 4:
        this->Volume1 = new unsigned char [3*neededSize];
        this->Volume2 = new unsigned char [2*neededSize];
        this->Volume3 = new unsigned char [3*neededSize];
        break;
      }

    this->VolumeSize       = neededSize;
    this->VolumeComponents = components;
    }
 
  // Find the scalar range
  double scalarRange[2];
  input->GetPointData()->GetScalars()->GetRange(scalarRange, components-1);
 
  // Is the difference between max and min less than 4096? If so, and if
  // the data is not of float or double type, use a simple offset mapping.
  // If the difference between max and min is 4096 or greater, or the data
  // is of type float or double, we must use an offset / scaling mapping.
  // In this case, the array size will be 4096 - we need to figure out the 
  // offset and scale factor.
  float offset;
  float scale;
 
  int arraySizeNeeded;
 
  int scalarType = input->GetScalarType();

  if ( scalarType == VTK_FLOAT ||
       scalarType == VTK_DOUBLE ||
       scalarRange[1] - scalarRange[0] > 255 )
    {
    arraySizeNeeded = 256;
    offset          = -scalarRange[0];
    scale           = 255.0 / (scalarRange[1] - scalarRange[0]);
    }
  else
    {
    arraySizeNeeded = static_cast<int>(scalarRange[1] - scalarRange[0] + 1);
    offset          = -scalarRange[0]; 
    scale           = 1.0;
   }
 
  this->ColorTableSize   = arraySizeNeeded;
  this->ColorTableOffset = offset;
  this->ColorTableScale  = scale;

  // Save the volume size
  this->VolumeDimensions[0] = powerOfTwoDim[0];
  this->VolumeDimensions[1] = powerOfTwoDim[1];
  this->VolumeDimensions[2] = powerOfTwoDim[2];
 
  // Compute the new spacing
  this->VolumeSpacing[0] = 
    (dim[0]-1.01)*spacing[0] / static_cast<double>(this->VolumeDimensions[0]-1);
  this->VolumeSpacing[1] = 
    (dim[1]-1.01)*spacing[1] / static_cast<double>(this->VolumeDimensions[1]-1);
  this->VolumeSpacing[2] = 
    ((dim[2])-1.01)*spacing[2] / static_cast<double>(this->VolumeDimensions[2]-1);


  // Transfer the input volume to the RGBA volume
  void *dataPtr = input->GetScalarPointer();


  switch ( scalarType )
    {
    vtkTemplateMacro(
      vtkVolumeTextureMapper3DComputeScalars(
        static_cast<VTK_TT *>(dataPtr), this,
        offset, scale,
        this->Volume1,
        this->Volume2));
    }

  switch ( scalarType )
    {
    vtkTemplateMacro( 
      vtkVolumeTextureMapper3DComputeGradients(
        static_cast<VTK_TT *>(dataPtr), this,
        scalarRange,
        this->Volume1,
        this->Volume2,
        this->Volume3));
    }

  return 1;
}


//-----------------------------------------------------------------------------
int vtkVolumeTextureMapper3D::UpdateColorLookup( vtkVolume *vol )
{
  int needToUpdate = 0;

  // Get the image data
  vtkImageData *input = this->GetInput();
  input->Update();

  // Has the volume changed in some way?
  if ( this->SavedParametersInput != input ||
       this->SavedParametersMTime.GetMTime() < input->GetMTime() )
    {
    needToUpdate = 1;
    }
 
  // What sample distance are we going to use for rendering? If we
  // have to render quickly according to our allocated render time,
  // don't necessary obey the sample distance requested by the user.
  // Instead set the sample distance to the average spacing.
  this->ActualSampleDistance = this->SampleDistance;
  if ( vol->GetAllocatedRenderTime() < 1.0 )
    {
    float spacing[3];
    this->GetVolumeSpacing(spacing);
    this->ActualSampleDistance = 
      0.333 * (static_cast<double>(spacing[0]) + static_cast<double>(spacing[1]) + static_cast<double>(spacing[2]));
    }

  // How many components?
  int components = input->GetNumberOfScalarComponents();

  // Has the sample distance changed?
  if ( this->SavedSampleDistance != this->ActualSampleDistance )
    {
    needToUpdate = 1;
    }

  vtkColorTransferFunction *rgbFunc  = NULL;
  vtkPiecewiseFunction     *grayFunc = NULL;
 
  // How many color channels for this component?
  int colorChannels = vol->GetProperty()->GetColorChannels(0);

  if ( components < 3 )
    {
    // Has the number of color channels changed?
    if ( this->SavedColorChannels != colorChannels )
      {
      needToUpdate = 1;
      }

    // Has the color transfer function changed in some way,
    // and we are using it?
    if ( colorChannels == 3 )
      {
      rgbFunc  = vol->GetProperty()->GetRGBTransferFunction(0);
      if ( this->SavedRGBFunction != rgbFunc ||
           this->SavedParametersMTime.GetMTime() < rgbFunc->GetMTime() )
        {
        needToUpdate = 1;
        }
      }

    // Has the gray transfer function changed in some way,
    // and we are using it?
    if ( colorChannels == 1 )
      {
      grayFunc = vol->GetProperty()->GetGrayTransferFunction(0);
      if ( this->SavedGrayFunction != grayFunc ||
           this->SavedParametersMTime.GetMTime() < grayFunc->GetMTime() )
        {
        needToUpdate = 1;
        }
      }
    }
 
  // Has the scalar opacity transfer function changed in some way?
  vtkPiecewiseFunction *scalarOpacityFunc = 
    vol->GetProperty()->GetScalarOpacity(0);
  if ( this->SavedScalarOpacityFunction != scalarOpacityFunc ||
       this->SavedParametersMTime.GetMTime() < 
       scalarOpacityFunc->GetMTime() )
    {
    needToUpdate = 1;
    }

  // Has the gradient opacity transfer function changed in some way?
  vtkPiecewiseFunction *gradientOpacityFunc = 
    vol->GetProperty()->GetGradientOpacity(0);
  if ( this->SavedGradientOpacityFunction != gradientOpacityFunc ||
       this->SavedParametersMTime.GetMTime() < 
       gradientOpacityFunc->GetMTime() )
    {
    needToUpdate = 1;
    }


  double scalarOpacityDistance = 
    vol->GetProperty()->GetScalarOpacityUnitDistance(0);
  if ( this->SavedScalarOpacityDistance != scalarOpacityDistance )
    {
    needToUpdate = 1;
    }
  
  // If we have not found any need to update, return now
  if ( !needToUpdate )
    {
    return 0;
    }

  this->SavedRGBFunction             = rgbFunc;
  this->SavedGrayFunction            = grayFunc;
  this->SavedScalarOpacityFunction   = scalarOpacityFunc;
  this->SavedGradientOpacityFunction = gradientOpacityFunc;
  this->SavedColorChannels           = colorChannels;
  this->SavedSampleDistance          = this->ActualSampleDistance;
  this->SavedScalarOpacityDistance   = scalarOpacityDistance;
  this->SavedParametersInput         = input;
  
  this->SavedParametersMTime.Modified();

  // Find the scalar range
  double scalarRange[2];
  input->GetPointData()->GetScalars()->GetRange(scalarRange, components-1);
  
  int arraySizeNeeded = this->ColorTableSize;

  if ( components < 3 )
    {
    // Sample the transfer functions between the min and max.
    if ( colorChannels == 1 )
      {
      grayFunc->GetTable( scalarRange[0], scalarRange[1], 
                          arraySizeNeeded, this->TempArray1 );
      }
    else
      {
      rgbFunc->GetTable( scalarRange[0], scalarRange[1], 
                         arraySizeNeeded, this->TempArray1 );
      }
    }
  
  scalarOpacityFunc->GetTable( scalarRange[0], scalarRange[1], 
                               arraySizeNeeded, this->TempArray2 );

  float goArray[256];
  gradientOpacityFunc->GetTable( 0, (scalarRange[1] - scalarRange[0])*0.25, 
                                 256, goArray );

  // Correct the opacity array for the spacing between the planes.
  int i;

  float *fptr2 = this->TempArray2;
  double factor = this->ActualSampleDistance / scalarOpacityDistance;
  for ( i = 0; i < arraySizeNeeded; i++ )
    {
    if ( *fptr2 > 0.0001 )
      {
      *fptr2 =  1.0-pow(static_cast<double>(1.0-(*fptr2)),factor);
      }
    fptr2++;
    }

  int goLoop;
  unsigned char *ptr, *rgbptr, *aptr;
  float *fptr1;
 
  switch (components)
    {
    case 1:
      // Move the two temp float arrays into one RGBA unsigned char array
      ptr = this->ColorLookup;
      for ( goLoop = 0; goLoop < 256; goLoop++ )
        {
        fptr1 = this->TempArray1;
        fptr2 = this->TempArray2;
        if ( colorChannels == 1 )
          {
          for ( i = 0; i < arraySizeNeeded; i++ )
            {
            *(ptr++) = static_cast<unsigned char>(*(fptr1)*255.0 + 0.5);
            *(ptr++) = static_cast<unsigned char>(*(fptr1)*255.0 + 0.5);
            *(ptr++) = static_cast<unsigned char>(*(fptr1++)*255.0 + 0.5);
            *(ptr++) = static_cast<unsigned char>(*(fptr2++)*goArray[goLoop]*255.0 + 0.5);
            }
          }
        else
          {
          for ( i = 0; i < arraySizeNeeded; i++ )
            {
            *(ptr++) = static_cast<unsigned char>(*(fptr1++)*255.0 + 0.5);
            *(ptr++) = static_cast<unsigned char>(*(fptr1++)*255.0 + 0.5);
            *(ptr++) = static_cast<unsigned char>(*(fptr1++)*255.0 + 0.5);
            *(ptr++) = static_cast<unsigned char>(*(fptr2++)*goArray[goLoop]*255.0 + 0.5);
            }
          }

        for ( ; i < 256; i++ )
          {
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          *(ptr++) = 0;
          }
        }
      break;

    case 2:
      // Move the two temp float arrays into one RGB unsigned char array and
      // one alpha array. 
      rgbptr = this->ColorLookup;
      aptr   = this->AlphaLookup;
      
      if ( colorChannels == 1 )
        {  
        for ( i = 0; i < arraySizeNeeded; i++ )
          {
          fptr1 = this->TempArray1;
          fptr2 = this->TempArray2;
          for ( goLoop = 0; goLoop < 256; goLoop++ )
            {
            *(rgbptr++) = static_cast<unsigned char>(*(fptr1)*255.0 + 0.5);
            *(rgbptr++) = static_cast<unsigned char>(*(fptr1)*255.0 + 0.5);
            *(rgbptr++) = static_cast<unsigned char>(*(fptr1++)*255.0 + 0.5);
            *(aptr++)   = static_cast<unsigned char>(*(fptr2++)*goArray[goLoop]*255.0 + 0.5);
            }
          }
        }
      else
        {
        fptr1 = this->TempArray1;
        fptr2 = this->TempArray2;
        for ( i = 0; i < arraySizeNeeded; i++ )
          {
          for ( goLoop = 0; goLoop < 256; goLoop++ )
            {
            *(rgbptr++) = static_cast<unsigned char>(*(fptr1)*255.0 + 0.5);
            *(rgbptr++) = static_cast<unsigned char>(*(fptr1+1)*255.0 + 0.5);
            *(rgbptr++) = static_cast<unsigned char>(*(fptr1+2)*255.0 + 0.5);
            *(aptr++)   = static_cast<unsigned char>(*(fptr2)*goArray[goLoop]*255.0 + 0.5);
            }
          fptr1+=3;
          fptr2++;
          }
        }

      for ( ; i < 256; i++ )
        {
        for ( goLoop = 0; goLoop < 256; goLoop++ )
          {
          *(rgbptr++) = 0;
          *(rgbptr++) = 0;
          *(rgbptr++) = 0;
          *(aptr++)   = 0;
          }
        }
      break;
      
    case 3:
    case 4:
      // Move the two temp float arrays into one alpha array
      aptr   = this->AlphaLookup;
      
      for ( goLoop = 0; goLoop < 256; goLoop++ )
        {
        fptr2 = this->TempArray2;
        for ( i = 0; i < arraySizeNeeded; i++ )
          {
          *(aptr++)   = static_cast<unsigned char>(*(fptr2++)*goArray[goLoop]*255.0 + 0.5);
          }
        for ( ; i < 256; i++ )
          {
          *(aptr++)   = 0;
          }
        }

      break;
    }
  return 1;
}


//-----------------------------------------------------------------------------
// Print the vtkVolumeTextureMapper3D
void vtkVolumeTextureMapper3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Distance: " << this->SampleDistance << endl;
  os << indent << "Render Method: " << this->RenderMethod << endl;
  os << indent << "Preferred Render Method: " << this->PreferredRenderMethod << endl;
  os << indent << "NumberOfPolygons: " << this->NumberOfPolygons << endl;
  os << indent << "ActualSampleDistance: " 
     << this->ActualSampleDistance << endl;
  os << indent << "VolumeDimensions: " << this->VolumeDimensions[0] << " "
     << this->VolumeDimensions[1] << " " << this->VolumeDimensions[2] << endl;
  os << indent << "VolumeSpacing: " << this->VolumeSpacing[0] << " "
     << this->VolumeSpacing[1] << " " << this->VolumeSpacing[2] << endl;
  
  os << indent << "UseCompressedTexture: " << this->UseCompressedTexture
     << endl;
}



