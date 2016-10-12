/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientEstimator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkEncodedGradientEstimator
 * @brief   Superclass for gradient estimation
 *
 * vtkEncodedGradientEstimator is an abstract superclass for gradient
 * estimation. It takes a scalar input of vtkImageData, computes
 * a gradient value for every point, and encodes this value into a
 * three byte value (2 for direction, 1 for magnitude) using the
 * vtkDirectionEncoder. The direction encoder is defaulted to a
 * vtkRecursiveSphereDirectionEncoder, but can be overridden with the
 * SetDirectionEncoder method. The scale and the bias values for the gradient
 * magnitude are used to convert it into a one byte value according to
 * v = m*scale + bias where m is the magnitude and v is the resulting
 * one byte value.
 * @sa
 * vtkFiniteDifferenceGradientEstimator vtkDirectionEncoder
*/

#ifndef vtkEncodedGradientEstimator_h
#define vtkEncodedGradientEstimator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class vtkImageData;
class vtkDirectionEncoder;
class vtkMultiThreader;

class VTKRENDERINGVOLUME_EXPORT vtkEncodedGradientEstimator : public vtkObject
{
public:
  vtkTypeMacro(vtkEncodedGradientEstimator,vtkObject);
  void PrintSelf( ostream& os, vtkIndent indent );

  //@{
  /**
   * Set/Get the scalar input for which the normals will be
   * calculated. Note that this call does not setup a pipeline
   * connection. vtkEncodedGradientEstimator is not an algorithm
   * and does not update its input. If you are directly using this
   * class, you may need to manually update the algorithm that produces
   * this data object.
   */
  virtual void SetInputData(vtkImageData*);
  vtkGetObjectMacro( InputData, vtkImageData );
  //@}

  //@{
  /**
   * Set/Get the scale and bias for the gradient magnitude
   */
  vtkSetMacro( GradientMagnitudeScale, float );
  vtkGetMacro( GradientMagnitudeScale, float );
  vtkSetMacro( GradientMagnitudeBias, float );
  vtkGetMacro( GradientMagnitudeBias, float );
  //@}

  //@{
  /**
   * Turn on / off the bounding of the normal computation by
   * the this->Bounds bounding box
   */
  vtkSetClampMacro( BoundsClip, int, 0, 1 );
  vtkGetMacro( BoundsClip, int );
  vtkBooleanMacro( BoundsClip, int );
  //@}

  //@{
  /**
   * Set / Get the bounds of the computation (used if
   * this->ComputationBounds is 1.) The bounds are specified
   * xmin, xmax, ymin, ymax, zmin, zmax.
   */
  vtkSetVector6Macro( Bounds, int );
  vtkGetVectorMacro(  Bounds, int, 6 );
  //@}

  /**
   * Recompute the encoded normals and gradient magnitudes.
   */
  void  Update( void );

  /**
   * Get the encoded normals.
   */
  unsigned short  *GetEncodedNormals( void );

  //@{
  /**
   * Get the encoded normal at an x,y,z location in the volume
   */
  int   GetEncodedNormalIndex( vtkIdType xyz_index );
  int   GetEncodedNormalIndex( int x_index, int y_index, int z_index );
  //@}

  /**
   * Get the gradient magnitudes
   */
  unsigned char *GetGradientMagnitudes(void);

  //@{
  /**
   * Get/Set the number of threads to create when encoding normals
   * This defaults to the number of available processors on the machine
   */
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );
  //@}

  //@{
  /**
   * Set / Get the direction encoder used to encode normal directions
   * to fit within two bytes
   */
  void SetDirectionEncoder( vtkDirectionEncoder *direnc );
  vtkGetObjectMacro( DirectionEncoder, vtkDirectionEncoder );
  //@}

  //@{
  /**
   * If you don't want to compute gradient magnitudes (but you
   * do want normals for shading) this can be used. Be careful - if
   * if you a non-constant gradient magnitude transfer function and
   * you turn this on, it may crash
   */
  vtkSetMacro( ComputeGradientMagnitudes, int );
  vtkGetMacro( ComputeGradientMagnitudes, int );
  vtkBooleanMacro( ComputeGradientMagnitudes, int );
  //@}

  //@{
  /**
   * If the data in each slice is only contained within a circle circumscribed
   * within the slice, and the slice is square, then don't compute anything
   * outside the circle. This circle through the slices forms a cylinder.
   */
  vtkSetMacro( CylinderClip, int );
  vtkGetMacro( CylinderClip, int );
  vtkBooleanMacro( CylinderClip, int );
  //@}

  //@{
  /**
   * Get the time required for the last update in seconds or cpu seconds
   */
  vtkGetMacro( LastUpdateTimeInSeconds, float );
  vtkGetMacro( LastUpdateTimeInCPUSeconds, float );
  //@}

  vtkGetMacro( UseCylinderClip, int );
  int *GetCircleLimits() { return this->CircleLimits; };

  //@{
  /**
   * Set / Get the ZeroNormalThreshold - this defines the minimum magnitude
   * of a gradient that is considered sufficient to define a
   * direction. Gradients with magnitudes at or less than this value are given
   * a "zero normal" index. These are handled specially in the shader,
   * and you can set the intensity of light for these zero normals in
   * the gradient shader.
   */
  void SetZeroNormalThreshold( float v );
  vtkGetMacro( ZeroNormalThreshold, float );
  //@}

  //@{
  /**
   * Assume that the data value outside the volume is zero when
   * computing normals.
   */
  vtkSetClampMacro( ZeroPad, int, 0, 1 );
  vtkGetMacro( ZeroPad, int );
  vtkBooleanMacro( ZeroPad, int );
  //@}


  // These variables should be protected but are being
  // made public to be accessible to the templated function.
  // We used to have the templated function as a friend, but
  // this does not work with all compilers

  // The input scalar data on which the normals are computed
  vtkImageData         *InputData;

  // The encoded normals (2 bytes) and the size of the encoded normals
  unsigned short        *EncodedNormals;
  int                   EncodedNormalsSize[3];

  // The magnitude of the gradient array and the size of this array
  unsigned char         *GradientMagnitudes;

  // The time at which the normals were last built
  vtkTimeStamp          BuildTime;

  vtkGetVectorMacro( InputSize, int, 3 );
  vtkGetVectorMacro( InputAspect, float, 3 );

protected:
  vtkEncodedGradientEstimator();
  ~vtkEncodedGradientEstimator();

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  // The number of threads to use when encoding normals
  int                        NumberOfThreads;

  vtkMultiThreader           *Threader;

  vtkDirectionEncoder        *DirectionEncoder;

  virtual void               UpdateNormals( void ) = 0;

  float                      GradientMagnitudeScale;
  float                      GradientMagnitudeBias;

  float                      LastUpdateTimeInSeconds;
  float                      LastUpdateTimeInCPUSeconds;

  float                      ZeroNormalThreshold;

  int                        CylinderClip;
  int                        *CircleLimits;
  int                        CircleLimitsSize;
  int                        UseCylinderClip;
  void                       ComputeCircleLimits( int size );

  int                        BoundsClip;
  int                        Bounds[6];

  int                        InputSize[3];
  float                      InputAspect[3];

  int                        ComputeGradientMagnitudes;

  int                        ZeroPad;

private:
  vtkEncodedGradientEstimator(const vtkEncodedGradientEstimator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEncodedGradientEstimator&) VTK_DELETE_FUNCTION;
};


#endif

