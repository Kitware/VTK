/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeRayCastSpaceLeapingImageFilter
 * @brief   Builds the space leaping data structure.
 *
 * This is an optimized multi-threaded imaging filter that builds the space
 * leaping datastructure, used by vtkFixedPointVolumeRayCastMapper. Empty
 * space leaping is used to skip large empty regions in the scalar
 * opacity and/or the gradient opacity transfer functions. Depending on
 * the various options set by vtkFixedPointVolumeRayCastMapper, the class
 * will internally invoke one of the many optmized routines to compute the
 * min/max/gradient-max values within a fixed block size, trying to
 * compute everything in a single multi-threaded pass through the data
 *
 * The block size may be changed at compile time. Its ifdef'ed to 4 in the CXX
 * file.
*/

#ifndef vtkVolumeRayCastSpaceLeapingImageFilter_h
#define vtkVolumeRayCastSpaceLeapingImageFilter_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class vtkDataArray;

class VTKRENDERINGVOLUME_EXPORT vtkVolumeRayCastSpaceLeapingImageFilter : public vtkThreadedImageAlgorithm
{
public:
  vtkTypeMacro(vtkVolumeRayCastSpaceLeapingImageFilter,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkVolumeRayCastSpaceLeapingImageFilter *New();

  //@{
  /**
   * Set the scalars.
   */
  virtual void SetCurrentScalars( vtkDataArray * );
  vtkGetObjectMacro( CurrentScalars, vtkDataArray );
  //@}

  //@{
  /**
   * Do we use independent components, or dependent components ?
   */
  vtkSetMacro( IndependentComponents, int );
  vtkGetMacro( IndependentComponents, int );
  //@}

  //@{
  /**
   * Compute gradient opacity ?
   */
  vtkSetMacro( ComputeGradientOpacity, int );
  vtkGetMacro( ComputeGradientOpacity, int );
  vtkBooleanMacro( ComputeGradientOpacity, int );
  //@}

  //@{
  /**
   * Compute the min max structure ?.
   */
  vtkSetMacro( ComputeMinMax, int );
  vtkGetMacro( ComputeMinMax, int );
  vtkBooleanMacro( ComputeMinMax, int );
  //@}

  //@{
  /**
   * Update the gradient opacity flags. (The scalar opacity flags are always
   * updated upon execution of this filter.)
   */
  vtkSetMacro( UpdateGradientOpacityFlags, int );
  vtkGetMacro( UpdateGradientOpacityFlags, int );
  vtkBooleanMacro( UpdateGradientOpacityFlags, int );
  //@}

  /**
   * Get the last execution time. This is updated every
   * time the scalars or the gradient opacity values are computed
   */
  vtkMTimeType GetLastMinMaxBuildTime()
    { return LastMinMaxBuildTime.GetMTime(); }

  /**
   * Get the last execution time. This is updated every time the flags bits
   * are re-computed.
   */
  vtkMTimeType GetLastMinMaxFlagTime()
    { return LastMinMaxFlagTime.GetMTime(); }

  //@{
  /**
   * Is the difference between max and min of the data less than 32768? If so,
   * and if the data is not of float/double type, use a simple offset mapping.
   * If the difference between max and min is 32768 or greater, or the data
   * is of type float or double, we must use an offset / scaling mapping.
   * In this case, the array size will be 32768 - we need to figure out the
   * offset and scale factor.
   */
  vtkSetVector4Macro( TableShift, float );
  vtkGetVector4Macro( TableShift, float );
  vtkSetVector4Macro( TableScale, float );
  vtkGetVector4Macro( TableScale, float );
  vtkSetVector4Macro( TableSize,  int   );
  vtkGetVector4Macro( TableSize,  int   );
  //@}

  /**
   * Get the number of independent components for which we need to keep track
   * of min/max
   */
  int GetNumberOfIndependentComponents();

  /**
   * Get the raw pointer to the final computed space leaping datastructure.
   * The result is only valid after Update() has been called on the filter.
   * Note that this filter holds onto its memory. The dimensions of the min-
   * max volume are in dims. The 4th value in the array indicates the number
   * of independent components, (also queried via
   * GetNumberOfIndependentComponents())
   */
  unsigned short * GetMinMaxVolume( int dims[4] );

  /**
   * INTERNAL - Do not use
   * Set the last cached min-max volume, as used by
   * vtkFixedPointVolumeRayCastMapper.
   */
  virtual void SetCache(vtkImageData * imageCache);

  /**
   * Compute the extents and dimensions of the input that's required to
   * generate an output min-max structure given by outExt.
   * INTERNAL - Do not use
   */
  static void ComputeInputExtentsForOutput( int inExt[6],
      int inDim[3], int outExt[6], vtkImageData *inData );

  //@{
  /**
   * Get the first non-zero scalar opacity and gradient opacity indices for
   * each independent copmonent
   * INTERNAL - Do not use.
   */
  unsigned short * GetMinNonZeroScalarIndex();
  unsigned char  * GetMinNonZeroGradientMagnitudeIndex();
  //@}

  //@{
  /**
   * Pointer to the pre-computed gradient magnitude structure. This is pre-
   * computed by the vtkFixedPointVolumeRayCastMapper class. This should be
   * set if one has the ComputeGradientOpacity flag enabled.
   */
  void SetGradientMagnitude( unsigned char ** gradientMagnitude );
  unsigned char **GetGradientMagnitude();
  //@}

  //@{
  /**
   * Set the scalar opacity and gradient opacity tables computed for each
   * component by the vtkFixedPointVolumeRayCastMapper
   */
  void SetScalarOpacityTable( int c, unsigned short * t);
  void SetGradientOpacityTable( int c, unsigned short * t );
  //@}

  /**
   * INTERNAL - Do not use
   * Compute the offset within an image of whole extents wholeExt, to access
   * the data starting at extents ext.
   */
  vtkIdType ComputeOffset(const int ext[6], const int wholeExt[6],
      int nComponents);

  // This method helps debug. It writes out a specific component of the
  // computed min-max-volume structure
  //static void WriteMinMaxVolume( int component, unsigned short *minMaxVolume,
  //                           int minMaxVolumeSize[4], const char *filename );

protected:
  vtkVolumeRayCastSpaceLeapingImageFilter();
  ~vtkVolumeRayCastSpaceLeapingImageFilter() VTK_OVERRIDE;

  int               IndependentComponents;
  vtkTimeStamp      LastMinMaxBuildTime;
  vtkTimeStamp      LastMinMaxFlagTime;
  vtkDataArray   *  CurrentScalars;
  float             TableShift[4];
  float             TableScale[4];
  int               TableSize[4];
  int               ComputeGradientOpacity;
  int               ComputeMinMax;
  int               UpdateGradientOpacityFlags;
  unsigned short *  MinNonZeroScalarIndex;
  unsigned char  *  MinNonZeroGradientMagnitudeIndex;
  unsigned char  ** GradientMagnitude;
  unsigned short  * ScalarOpacityTable[4];
  unsigned short  * GradientOpacityTable[4];
  vtkImageData    * Cache;


  void InternalRequestUpdateExtent(int *, int*);

  //@{
  /**
   * See superclass for details
   */
  int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;
  void ThreadedRequestData(       vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector,
                                  vtkImageData ***inData,
                                  vtkImageData **outData,
                                  int outExt[6], int id) VTK_OVERRIDE;
  int RequestData(        vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector) VTK_OVERRIDE;
  int RequestInformation( vtkInformation *,
                                  vtkInformationVector**,
                                  vtkInformationVector *) VTK_OVERRIDE;
  //@}

  /**
   * Compute the first non-zero scalar opacity and gradient opacity values
   * that are encountered when marching from the beginning of the transfer
   * function tables.
   */
  void ComputeFirstNonZeroOpacityIndices();

  /**
   * Fill the flags after processing the min/max/gradient structure. This
   * optimized version is invoked when only scalar opacity table is needed.
   */
  void FillScalarOpacityFlags(
      vtkImageData *minMaxVolume, int outExt[6] );

  /**
   * Fill the flags after processing the min/max/gradient structure. This
   * optimized version is invoked when both scalar and gradient opacity
   * tables need to be visited.
   */
  void FillScalarAndGradientOpacityFlags(
      vtkImageData *minMaxVolume, int outExt[6] );

  //@{
  /**
   * Allocate the output data. If we have a cache with the same metadata as
   * the output we are going to generate, re-use the cache as we may not be
   * updating all data in the min-max structure.
   */
  void AllocateOutputData(vtkImageData *out,
                                  vtkInformation* outInfo,
                                  int *uExtent) VTK_OVERRIDE;
  vtkImageData *AllocateOutputData(vtkDataObject *out,
                                           vtkInformation *outInfo) VTK_OVERRIDE;
  //@}

private:
  vtkVolumeRayCastSpaceLeapingImageFilter(const vtkVolumeRayCastSpaceLeapingImageFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeRayCastSpaceLeapingImageFilter&) VTK_DELETE_FUNCTION;
};

#endif
