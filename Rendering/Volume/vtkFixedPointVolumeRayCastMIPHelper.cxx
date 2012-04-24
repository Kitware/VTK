/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastMIPHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFixedPointVolumeRayCastMIPHelper.h"

#include "vtkImageData.h"
#include "vtkCommand.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkFixedPointRayCastImage.h"
#include "vtkDataArray.h"

#include <math.h>

vtkStandardNewMacro(vtkFixedPointVolumeRayCastMIPHelper);

// Construct a new vtkFixedPointVolumeRayCastMIPHelper with default values
vtkFixedPointVolumeRayCastMIPHelper::vtkFixedPointVolumeRayCastMIPHelper()
{
}

// Destruct a vtkFixedPointVolumeRayCastMIPHelper - clean up any memory used
vtkFixedPointVolumeRayCastMIPHelper::~vtkFixedPointVolumeRayCastMIPHelper()
{
}

// This method is called when the interpolation type is nearest neighbor and
// the data contains one component. In the inner loop we will compute the
// maximum value (in native type). After we have a maximum value for the ray
// we will convert it to unsigned short using the scale/shift, then use this
// index to lookup the final color/opacity.
template <class T>
void vtkFixedPointMIPHelperGenerateImageOneNN( T *data,
                                       int threadID,
                                       int threadCount,
                                       vtkFixedPointVolumeRayCastMapper *mapper,
                                       vtkVolume *vtkNotUsed(vol))
{
  VTKKWRCHelper_InitializationAndLoopStartNN();
  VTKKWRCHelper_InitializeMIPOneNN();
  VTKKWRCHelper_SpaceLeapSetup();

  if ( cropping )
    {
    int maxValueDefined = 0;
    unsigned short maxIdx = 0;

    for ( k = 0; k < numSteps; k++ )
      {
      if ( k )
        {
        mapper->FixedPointIncrement( pos, dir );
        }

      VTKKWRCHelper_MIPSpaceLeapCheck( maxIdx, maxValueDefined, mapper->GetFlipMIPComparison() );

      if ( !mapper->CheckIfCropped( pos ) )
        {
        mapper->ShiftVectorDown( pos, spos );
        dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];
        if ( !maxValueDefined ||
             ( (mapper->GetFlipMIPComparison() && *dptr < maxValue) ||
               (!mapper->GetFlipMIPComparison() && *dptr > maxValue) ) )
          {
          maxValue = *dptr;
          maxIdx = static_cast<unsigned short>((maxValue + shift[0])*scale[0]);
          maxValueDefined = 1;
          }
        }
      }

    if ( maxValueDefined )
      {
      VTKKWRCHelper_LookupColorMax( colorTable[0], scalarOpacityTable[0], maxIdx, imagePtr );
      }
    else
      {
      imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
      }
    }
  else
    {
    unsigned short maxIdx =
      static_cast<unsigned short>((maxValue + shift[0])*scale[0]);

    for ( k = 0; k < numSteps; k++ )
      {
      if ( k )
        {
        mapper->FixedPointIncrement( pos, dir );
        }

      VTKKWRCHelper_MIPSpaceLeapCheck( maxIdx, 1, mapper->GetFlipMIPComparison() );

      mapper->ShiftVectorDown( pos, spos );
      dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];
      if ( mapper->GetFlipMIPComparison() )
        {
        maxValue = ( *dptr < maxValue )?(*dptr):(maxValue);
        }
      else
        {
        maxValue = ( *dptr > maxValue )?(*dptr):(maxValue);
        }

      maxIdx = static_cast<unsigned short>((maxValue + shift[0])*scale[0]);
      }

    VTKKWRCHelper_LookupColorMax( colorTable[0], scalarOpacityTable[0], maxIdx, imagePtr );
    }

  VTKKWRCHelper_IncrementAndLoopEnd();

}

// This method is called when the interpolation type is nearest neighbor and
// the data has two or four dependent components. If it is four, they must be
// unsigned char components.  Compute max of last components in native type,
// then use first component to look up a color (2 component data) or first three
// as the color directly (four component data). Lookup alpha off the last component.
template <class T>
void vtkFixedPointMIPHelperGenerateImageDependentNN(
  T *data,
  int threadID,
  int threadCount,
  vtkFixedPointVolumeRayCastMapper *mapper,
  vtkVolume *vtkNotUsed(vol))
{
  VTKKWRCHelper_InitializationAndLoopStartNN();
  VTKKWRCHelper_InitializeMIPMultiNN();
  VTKKWRCHelper_SpaceLeapSetup();

  int maxValueDefined = 0;
  unsigned short maxIdxS = 0;

  for ( k = 0; k < numSteps; k++ )
    {
    if ( k )
      {
      mapper->FixedPointIncrement( pos, dir );
      }

    VTKKWRCHelper_MIPSpaceLeapCheck( maxIdxS, maxValueDefined,
                                     mapper->GetFlipMIPComparison() );
    VTKKWRCHelper_CroppingCheckNN( pos );

    mapper->ShiftVectorDown( pos, spos );
    dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];
    if ( !maxValueDefined ||
         ( ( mapper->GetFlipMIPComparison() && *(dptr + components - 1) < maxValue[components-1] ) ||
           ( !mapper->GetFlipMIPComparison() && *(dptr + components - 1) > maxValue[components-1] ) ) )
      {
      for ( c = 0; c < components; c++ )
        {
        maxValue[c] = *(dptr+c);
        }
      maxIdxS =
        static_cast<unsigned short>((maxValue[components-1] +
                                     shift[components-1])*scale[components-1]);
      maxValueDefined = 1;
      }
    }

  if ( maxValueDefined )
    {
    unsigned short maxIdx[4]={0,0,0,0};
    if ( components == 2 )
      {
      maxIdx[0] = static_cast<unsigned short>((maxValue[0] +
                                               shift[0])*scale[0]);
      maxIdx[1] = static_cast<unsigned short>((maxValue[1] +
                                               shift[1])*scale[1]);
      }
    else
      {
      maxIdx[0] = static_cast<unsigned short>(maxValue[0]);
      maxIdx[1] = static_cast<unsigned short>(maxValue[1]);
      maxIdx[2] = static_cast<unsigned short>(maxValue[2]);
      maxIdx[3] = static_cast<unsigned short>((maxValue[3] +
                                               shift[3])*scale[3]);
      }

    for ( c = 0; c < components; c++ )
      {
      }
    VTKKWRCHelper_LookupDependentColorUS( colorTable[0], scalarOpacityTable[0],
                                          maxIdx, components, imagePtr );
    }
  else
    {
    imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
    }

  VTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is called when the interpolation type is nearest neighbor and
// the data has more than one independent components. We compute the max of
// each component along the ray in native type, then use the scale/shift to
// convert this into an unsigned short index value. We use the index values
// to lookup the color/opacity per component, then use the component weights to
// blend these into one final color.
template <class T>
void vtkFixedPointMIPHelperGenerateImageIndependentNN(
  T *data,
  int threadID,
  int threadCount,
  vtkFixedPointVolumeRayCastMapper *mapper,
  vtkVolume *vol)
{
  VTKKWRCHelper_InitializeWeights();
  VTKKWRCHelper_InitializationAndLoopStartNN();
  VTKKWRCHelper_InitializeMIPMultiNN();
  VTKKWRCHelper_SpaceLeapSetupMulti();

  int maxValueDefined = 0;
  unsigned short maxIdx[4];

  for ( k = 0; k < numSteps; k++ )
    {
    if ( k )
      {
      mapper->FixedPointIncrement( pos, dir );
      }
    VTKKWRCHelper_CroppingCheckNN( pos );
    VTKKWRCHelper_MIPSpaceLeapPopulateMulti( maxIdx,
                                             mapper->GetFlipMIPComparison() )

    mapper->ShiftVectorDown( pos, spos );
    dptr = data +  spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];

    if ( !maxValueDefined )
      {
      for ( c = 0; c < components; c++ )
        {
        maxValue[c] = *(dptr+c);
        maxIdx[c] = static_cast<unsigned short>((maxValue[c] +
                                                 shift[c])*scale[c]);
        }
      maxValueDefined = 1;
      }
    else
      {
      for ( c = 0; c < components; c++ )
        {
        if ( VTKKWRCHelper_MIPSpaceLeapCheckMulti( c, mapper->GetFlipMIPComparison() ) &&
            ((mapper->GetFlipMIPComparison() &&  *(dptr + c) < maxValue[c] ) ||
             (!mapper->GetFlipMIPComparison() &&  *(dptr + c) > maxValue[c] )) )
          {
          maxValue[c] = *(dptr+c);
          maxIdx[c] = static_cast<unsigned short>((maxValue[c] +
                                                   shift[c])*scale[c]);
          }
        }
      }
    }

  imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
  if ( maxValueDefined )
    {
    VTKKWRCHelper_LookupAndCombineIndependentColorsMax(colorTable,
                                                       scalarOpacityTable,
                                                       maxIdx, weights,
                                                       components, imagePtr );
    }

  VTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is called when the interpolation type is linear, the
// data contains one component and scale = 1.0 and shift = 0.0. This is
// the simple case were we do not need to apply scale/shift in the
// inner loop. In the inner loop we compute the eight cell vertex values
// (if we have changed cells). We compute our weights within the cell
// according to our fractional position within the cell, and apply trilinear
// interpolation to compute the index. We find the maximum index along
// the ray, and then use this to look up a final color.
template <class T>
void vtkFixedPointMIPHelperGenerateImageOneSimpleTrilin(
  T *dataPtr,
  int threadID,
  int threadCount,
  vtkFixedPointVolumeRayCastMapper *mapper,
  vtkVolume *vtkNotUsed(vol))
{
  VTKKWRCHelper_InitializationAndLoopStartTrilin();
  VTKKWRCHelper_InitializeMIPOneTrilin();
  VTKKWRCHelper_SpaceLeapSetup();

  int maxValueDefined = 0;
  unsigned short maxIdx=0;
  unsigned int maxScalar = 0;

  for ( k = 0; k < numSteps; k++ )
    {
    if ( k )
      {
      mapper->FixedPointIncrement( pos, dir );
      }

    VTKKWRCHelper_MIPSpaceLeapCheck( maxIdx, maxValueDefined,
                                     mapper->GetFlipMIPComparison() );
    VTKKWRCHelper_CroppingCheckTrilin( pos );

    mapper->ShiftVectorDown( pos, spos );
    if ( spos[0] != oldSPos[0] ||
         spos[1] != oldSPos[1] ||
         spos[2] != oldSPos[2] )
      {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      dptr = dataPtr + spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];
      VTKKWRCHelper_GetCellScalarValuesSimple( dptr );
      if ( mapper->GetFlipMIPComparison() )
        {
        maxScalar = (A<B)?(A):(B);
        maxScalar = (C<maxScalar)?(C):(maxScalar);
        maxScalar = (D<maxScalar)?(D):(maxScalar);
        maxScalar = (E<maxScalar)?(E):(maxScalar);
        maxScalar = (F<maxScalar)?(F):(maxScalar);
        maxScalar = (G<maxScalar)?(G):(maxScalar);
        maxScalar = (H<maxScalar)?(H):(maxScalar);
        }
      else
        {
        maxScalar = (A>B)?(A):(B);
        maxScalar = (C>maxScalar)?(C):(maxScalar);
        maxScalar = (D>maxScalar)?(D):(maxScalar);
        maxScalar = (E>maxScalar)?(E):(maxScalar);
        maxScalar = (F>maxScalar)?(F):(maxScalar);
        maxScalar = (G>maxScalar)?(G):(maxScalar);
        maxScalar = (H>maxScalar)?(H):(maxScalar);
        }

      }

    if ( !maxValueDefined ||
         ((mapper->GetFlipMIPComparison() && maxScalar < static_cast<unsigned int>(maxValue) ) ||
          (!mapper->GetFlipMIPComparison() && maxScalar > static_cast<unsigned int>(maxValue) )) )
      {
      VTKKWRCHelper_ComputeWeights(pos);
      VTKKWRCHelper_InterpolateScalar(val);

      if ( !maxValueDefined ||
           ((mapper->GetFlipMIPComparison() && val < maxValue ) ||
            (!mapper->GetFlipMIPComparison() && val > maxValue )) )
        {
        maxValue = val;
        maxIdx = static_cast<unsigned short>(maxValue);
        maxValueDefined = 1;
        }
      }
    }

  if ( maxValueDefined )
    {
    VTKKWRCHelper_LookupColorMax( colorTable[0], scalarOpacityTable[0], maxIdx, imagePtr );
    }
  else
    {
    imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
    }

  VTKKWRCHelper_IncrementAndLoopEnd();
}


// This method is called when the interpolation type is linear, the
// data contains one component and scale != 1.0 or shift != 0.0. This
// means that we need to apply scale/shift in the inner loop to compute
// an unsigned short index value. In the inner loop we compute the eight cell
// vertex values (as unsigned short indices, if we have changed cells). We
// compute our weights within the cell according to our fractional position
// within the cell, and apply trilinear interpolation to compute the index.
// We find the maximum index along the ray, and then use this to look up a
// final color.
template <class T>
void vtkFixedPointMIPHelperGenerateImageOneTrilin(
  T *dataPtr,
  int threadID,
  int threadCount,
  vtkFixedPointVolumeRayCastMapper *mapper,
  vtkVolume *vtkNotUsed(vol))
{
  VTKKWRCHelper_InitializationAndLoopStartTrilin();
  VTKKWRCHelper_InitializeMIPOneTrilin();
  VTKKWRCHelper_SpaceLeapSetup();

  int maxValueDefined = 0;
  unsigned short maxIdx = 0;
  for ( k = 0; k < numSteps; k++ )
    {
    if ( k )
      {
      mapper->FixedPointIncrement( pos, dir );
      }

    VTKKWRCHelper_CroppingCheckTrilin( pos );
    VTKKWRCHelper_MIPSpaceLeapCheck( maxIdx, maxValueDefined,
                                     mapper->GetFlipMIPComparison() );

    mapper->ShiftVectorDown( pos, spos );
    if ( spos[0] != oldSPos[0] ||
         spos[1] != oldSPos[1] ||
         spos[2] != oldSPos[2] )
      {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];


      dptr = dataPtr + spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2];
      VTKKWRCHelper_GetCellScalarValues( dptr, scale[0], shift[0] );
      }

    VTKKWRCHelper_ComputeWeights(pos);
    VTKKWRCHelper_InterpolateScalar(val);

    if ( !maxValueDefined ||
         ((mapper->GetFlipMIPComparison() && val < maxValue ) ||
          (!mapper->GetFlipMIPComparison() && val > maxValue )) )
      {
      maxValue = val;
      maxIdx = static_cast<unsigned short>(maxValue);
      maxValueDefined = 1;
      }
    }

  if ( maxValueDefined )
    {
    VTKKWRCHelper_LookupColorMax( colorTable[0], scalarOpacityTable[0],
                                  maxIdx, imagePtr );
    }
  else
    {
    imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
    }

  VTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear, the data has
// two or four components and the components are not considered independent.
// For four component d>>(VTKKW_FP_SHIFT - 8));ata, the data must be unsigned char in type. In the
// inner loop we get the data value for the eight cell corners (if we have
// changed cells) for all components as unsigned shorts (we use the
// scale/shift to ensure the correct range). We compute our weights within
// the cell according to our fractional position within the cell, and apply
// trilinear interpolation to compute the index values. For two component data,
// We use the first index to lookup a color and the second to look up an opacity
// for this sample. For four component data we use the first three components
// directly as a color, then we look up the opacity using the fourth component.
// We then composite this into the color computed so far along the ray, and
// check if we can terminate at this point (if the accumulated opacity is
// higher than some threshold).
template <class T>
void vtkFixedPointMIPHelperGenerateImageDependentTrilin(
  T *dataPtr,
  int threadID,
  int threadCount,
  vtkFixedPointVolumeRayCastMapper *mapper,
  vtkVolume *vtkNotUsed(vol))
{
  VTKKWRCHelper_InitializationAndLoopStartTrilin();
  VTKKWRCHelper_InitializeMIPMultiTrilin();
  VTKKWRCHelper_SpaceLeapSetup();

  int maxValueDefined = 0;
  unsigned short maxIdx = 0;
  for ( k = 0; k < numSteps; k++ )
    {
    if ( k )
      {
      mapper->FixedPointIncrement( pos, dir );
      }

    VTKKWRCHelper_CroppingCheckTrilin( pos );
    VTKKWRCHelper_MIPSpaceLeapCheck( maxIdx, maxValueDefined,
                                     mapper->GetFlipMIPComparison() );

    mapper->ShiftVectorDown( pos, spos );
    if ( spos[0] != oldSPos[0] ||
         spos[1] != oldSPos[1] ||
         spos[2] != oldSPos[2] )
      {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      if ( components == 2 )
        {
        for ( c= 0; c < components; c++ )
          {
          dptr = dataPtr + spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2]+ c;
          VTKKWRCHelper_GetCellComponentScalarValues( dptr, c, scale[c],
                                                      shift[c] );
          }
        }
      else
        {
        for ( c= 0; c < 3; c++ )
          {
          dptr = dataPtr + spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2]+ c;
          VTKKWRCHelper_GetCellComponentRawScalarValues( dptr, c );
          }
        dptr = dataPtr + spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2] + c;
        VTKKWRCHelper_GetCellComponentScalarValues( dptr,3,scale[3],shift[3] );
        }

      }

    VTKKWRCHelper_ComputeWeights(pos);
    VTKKWRCHelper_InterpolateScalarComponent( val, c, components );

    if ( !maxValueDefined ||
         ((mapper->GetFlipMIPComparison() && (val[components-1] < maxValue[components-1]) ) ||
          (!mapper->GetFlipMIPComparison() && (val[components-1] > maxValue[components-1]) )) )
      {
      for ( c= 0; c < components; c++ )
        {
        maxValue[c] = val[c];
        }
      maxIdx = static_cast<unsigned short>((maxValue[components-1] +
                                            shift[components-1])*scale[components-1]);
      maxValueDefined = 1;
      }
    }

  if ( maxValueDefined )
    {
    VTKKWRCHelper_LookupDependentColorUS( colorTable[0],
                                          scalarOpacityTable[0],
                                          maxValue, components, imagePtr );
    }
  else
    {
    imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
    }

  VTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear, the data has
// more than one component and the components are considered independent. In
// the inner loop we get the data value for the eight cell corners (if we have
// changed cells) for all components as an unsigned shorts (we have to use the
// scale/shift to ensure that we obtained unsigned short indices) We compute
// our weights within the cell according to our fractional position within the
// cell, and apply trilinear interpolation to compute a value for each
// component. We do this for each sample along the ray to find a maximum value
// per component, then we look up a color/opacity for each component and blend
// them according to the component weights.
template <class T>
void vtkFixedPointMIPHelperGenerateImageIndependentTrilin(
  T *dataPtr,
  int threadID,
  int threadCount,
  vtkFixedPointVolumeRayCastMapper *mapper,
  vtkVolume *vol)
{
  VTKKWRCHelper_InitializeWeights();
  VTKKWRCHelper_InitializationAndLoopStartTrilin();
  VTKKWRCHelper_InitializeMIPMultiTrilin();

  int maxValueDefined = 0;
  for ( k = 0; k < numSteps; k++ )
    {
    if ( k )
      {
      mapper->FixedPointIncrement( pos, dir );
      }

    VTKKWRCHelper_CroppingCheckTrilin( pos );

    mapper->ShiftVectorDown( pos, spos );
    if ( spos[0] != oldSPos[0] ||
         spos[1] != oldSPos[1] ||
         spos[2] != oldSPos[2] )
      {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      for ( c= 0; c < components; c++ )
        {
        dptr = dataPtr + spos[0]*inc[0] + spos[1]*inc[1] + spos[2]*inc[2] + c;
        VTKKWRCHelper_GetCellComponentScalarValues( dptr, c, scale[c],
                                                    shift[c] );
        }
      }

    VTKKWRCHelper_ComputeWeights(pos);
    VTKKWRCHelper_InterpolateScalarComponent( val, c, components );

    if ( !maxValueDefined )
      {
      for ( c= 0; c < components; c++ )
        {
        maxValue[c] = val[c];
        }
      maxValueDefined = 1;
      }
    else
      {
      for ( c= 0; c < components; c++ )
        {
        if ( ( mapper->GetFlipMIPComparison() && val[c] < maxValue[c] ) ||
             ( !mapper->GetFlipMIPComparison() && val[c] > maxValue[c] ) )
          {
          maxValue[c] = val[c];
          }
        }
      }
    }

  imagePtr[0] = imagePtr[1] = imagePtr[2] = imagePtr[3] = 0;
  if ( maxValueDefined )
    {
    VTKKWRCHelper_LookupAndCombineIndependentColorsMax( colorTable,
                                                        scalarOpacityTable,
                                                        maxValue, weights,
                                                        components, imagePtr );
    }

  VTKKWRCHelper_IncrementAndLoopEnd();
}

void vtkFixedPointVolumeRayCastMIPHelper::GenerateImage(
  int threadID,
  int threadCount,
  vtkVolume *vol,
  vtkFixedPointVolumeRayCastMapper *mapper )
{
  void *dataPtr  = mapper->GetCurrentScalars()->GetVoidPointer(0);
  int scalarType = mapper->GetCurrentScalars()->GetDataType();

  // Nearest Neighbor interpolate
  if ( mapper->ShouldUseNearestNeighborInterpolation( vol ) )
    {
    // One component data
    if ( mapper->GetCurrentScalars()->GetNumberOfComponents() == 1 )
      {
      switch ( scalarType )
        {
        vtkTemplateMacro(
          vtkFixedPointMIPHelperGenerateImageOneNN(
            static_cast<VTK_TT *>(dataPtr),
            threadID, threadCount, mapper, vol) );
        }
      }
    // More that one independent components
    else if ( vol->GetProperty()->GetIndependentComponents() )
      {
      switch ( scalarType )
        {
        vtkTemplateMacro(
          vtkFixedPointMIPHelperGenerateImageIndependentNN(
            static_cast<VTK_TT *>(dataPtr),
            threadID, threadCount, mapper, vol) );
        }
      }
    // Dependent (color) components
    else
      {
      switch ( scalarType )
        {
        vtkTemplateMacro(
          vtkFixedPointMIPHelperGenerateImageDependentNN(
            static_cast<VTK_TT *>(dataPtr),
            threadID, threadCount, mapper, vol) );
        }
      }
    }
  // Trilinear Interpolation
  else
    {
    // One component
    if ( mapper->GetCurrentScalars()->GetNumberOfComponents() == 1 )
      {
      // Scale == 1.0 and shift == 0.0 - simple case (faster)
      if ( mapper->GetTableScale()[0] == 1.0 &&
           mapper->GetTableShift()[0] == 0.0 )
        {
        switch ( scalarType )
          {
          vtkTemplateMacro(
            vtkFixedPointMIPHelperGenerateImageOneSimpleTrilin(
              static_cast<VTK_TT *>(dataPtr),
              threadID, threadCount, mapper, vol) );
          }
        }
      // Scale != 1.0 or shift != 0.0 - must apply scale/shift in inner loop
      else
        {
        switch ( scalarType )
          {
          vtkTemplateMacro(
            vtkFixedPointMIPHelperGenerateImageOneTrilin(
              static_cast<VTK_TT *>(dataPtr),
              threadID, threadCount, mapper, vol) );
          }
        }
      }
    // Indepedent components (more than one)
    else if ( vol->GetProperty()->GetIndependentComponents() )
      {
      switch ( scalarType )
        {
        vtkTemplateMacro(
          vtkFixedPointMIPHelperGenerateImageIndependentTrilin(
            static_cast<VTK_TT *>(dataPtr),
            threadID, threadCount, mapper, vol) );
        }
      }
    // Dependent components
    else
      {
      switch ( scalarType )
        {
        vtkTemplateMacro(
          vtkFixedPointMIPHelperGenerateImageDependentTrilin(
            static_cast<VTK_TT *>(dataPtr),
            threadID, threadCount, mapper, vol) );
        }
      }
    }
}

// Print method for vtkFixedPointVolumeRayCastMIPHelper
void vtkFixedPointVolumeRayCastMIPHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


