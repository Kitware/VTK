/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAMRVolumeMapper
 * @brief   AMR class for a volume mapper
 *
 *
 * vtkAMRVolumeMapper is the definition of a volume mapper.
 * for AMR Structured Data
 *
 *
*/

#ifndef vtkAMRVolumeMapper_h
#define vtkAMRVolumeMapper_h

#include "vtkRenderingVolumeAMRModule.h" // For export macro
#include "vtkVolumeMapper.h"
#include "vtkImageReslice.h" // for VTK_RESLICE_NEAREST, VTK_RESLICE_CUBIC

class vtkAMRResampleFilter;
class vtkCamera;
class vtkImageData;
class vtkOverlappingAMR;
class vtkSmartVolumeMapper;
class vtkUniformGrid;

class VTKRENDERINGVOLUMEAMR_EXPORT vtkAMRVolumeMapper : public vtkVolumeMapper
{
public:
  static vtkAMRVolumeMapper *New();
  vtkTypeMacro(vtkAMRVolumeMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent ) override;

  //@{
  /**
   * Set the input data
   */
  void SetInputData( vtkImageData* ) override;
  void SetInputData( vtkDataSet* ) override;
  virtual void SetInputData( vtkOverlappingAMR* );
  void SetInputConnection (int port, vtkAlgorithmOutput *input) override;
  void SetInputConnection (vtkAlgorithmOutput *input) override
  {this->SetInputConnection(0, input);}
  //@}

  //@{
  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double *GetBounds() override;
  void GetBounds(double bounds[6]) override
    {this->vtkVolumeMapper::GetBounds(bounds); };
  //@}

  /**
   * Control how the mapper works with scalar point data and cell attribute
   * data.  By default (ScalarModeToDefault), the mapper will use point data,
   * and if no point data is available, then cell data is used. Alternatively
   * you can explicitly set the mapper to use point data
   * (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
   * You can also choose to get the scalars from an array in point field
   * data (ScalarModeToUsePointFieldData) or cell field data
   * (ScalarModeToUseCellFieldData).  If scalars are coming from a field
   * data array, you must call SelectScalarArray.
   */
  void SetScalarMode(int mode) override;

  //@{
  /**
   * Set/Get the blend mode.
   * Additive blend mode adds scalars along the ray and multiply them by
   * their opacity mapping value.
   */
  void SetBlendMode(int mode) override;
  int GetBlendMode() override;
  //@}

  //@{
  /**
   * When ScalarMode is set to UsePointFieldData or UseCellFieldData,
   * you can specify which scalar array to use during rendering.
   * The transfer function in the vtkVolumeProperty (attached to the calling
   * vtkVolume) will decide how to convert vectors to colors.
   */
  void SelectScalarArray(int arrayNum) override;
  void SelectScalarArray(const char* arrayName) override;
  //@}

  //@{
  /**
   * Get the array name or number and component to use for rendering.
   */
  char* GetArrayName() override;
  int GetArrayId() override;
  int GetArrayAccessMode() override;
  //@}

  /**
   * Return the method for obtaining scalar data.
   */
  const char *GetScalarModeAsString();
  //@{
  /**
   * Turn On/Off orthogonal cropping. (Clipping planes are
   * perpendicular to the coordinate axes.)
   */
  void SetCropping(vtkTypeBool) override;
  vtkTypeBool GetCropping() override;
  //@}

  //@{
  /**
   * Set/Get the Cropping Region Planes ( xmin, xmax, ymin, ymax, zmin, zmax )
   * These planes are defined in volume coordinates - spacing and origin are
   * considered.
   */
  void SetCroppingRegionPlanes(double arg1, double arg2, double arg3,
                               double arg4, double arg5, double arg6) override;
  void SetCroppingRegionPlanes(double *planes) override
    {this->SetCroppingRegionPlanes(
        planes[0],planes[1],planes[2],
        planes[3],planes[4],planes[5]);}
  void GetCroppingRegionPlanes(double *planes) override;
  double *GetCroppingRegionPlanes() VTK_SIZEHINT(6) override;
  //@}
  //@{
  /**
   * Set the flags for the cropping regions. The clipping planes divide the
   * volume into 27 regions - there is one bit for each region. The regions
   * start from the one containing voxel (0,0,0), moving along the x axis
   * fastest, the y axis next, and the z axis slowest. These are represented
   * from the lowest bit to bit number 27 in the integer containing the
   * flags. There are several convenience functions to set some common
   * configurations - subvolume (the default), fence (between any of the
   * clip plane pairs), inverted fence, cross (between any two of the
   * clip plane pairs) and inverted cross.
   */
  void SetCroppingRegionFlags(int mode) override;
  int GetCroppingRegionFlags() override;
  //@}

// The possible values for the default and current render mode ivars
  enum
  {
    DefaultRenderMode=0,
    RayCastAndTextureRenderMode,
    RayCastRenderMode,
    TextureRenderMode,
    GPURenderMode,
    UndefinedRenderMode,
    InvalidRenderMode
  };

  //@{
  /**
   * Set the requested render mode. The default is
   * vtkSmartVolumeMapper::DefaultRenderMode.
   */
  void SetRequestedRenderMode(int mode);
  int GetRequestedRenderMode();
  //@}

  /**
   * Set the requested render mode to vtkAMRVolumeMapper::DefaultRenderMode.
   * This is the best option for an application that must adapt to different
   * data types, hardware, and rendering parameters.
   */
  void SetRequestedRenderModeToDefault()
  {this->SetRequestedRenderMode(vtkAMRVolumeMapper::DefaultRenderMode);}

  /**
   * Set the requested render mode to
   * vtkAMRVolumeMapper::RayCastAndTextureRenderMode.
   * This is a good option if you want to avoid using advanced OpenGL
   * functionality, but would still like to used 3D texture mapping, if
   * available, for interactive rendering.
   */
  void SetRequestedRenderModeToRayCastAndTexture()
  {this->SetRequestedRenderMode(vtkAMRVolumeMapper::RayCastAndTextureRenderMode);}

  /**
   * Set the requested render mode to vtkAMRVolumeMapper::RayCastRenderMode.
   * This option will use software rendering exclusively. This is a good option
   * if you know there is no hardware acceleration.
   */
  void SetRequestedRenderModeToRayCast()
  {this->SetRequestedRenderMode(vtkAMRVolumeMapper::RayCastRenderMode);}

  /**
   * Set the requested render mode to
   * vtkAMRVolumeMapper::TextureRenderMode.
   * This is a good option if you want to use 3D texture mapping, if
   * available, for interactive rendering.
   */
  void SetRequestedRenderModeToTexture()
  {this->SetRequestedRenderMode(vtkAMRVolumeMapper::TextureRenderMode);}

  /**
   * Set the requested render mode to
   * vtkAMRVolumeMapper::GPURenderMode.
   * This will do the volume rendering on the GPU
   */
  void SetRequestedRenderModeToGPU()
  {this->SetRequestedRenderMode(vtkAMRVolumeMapper::GPURenderMode);}

  //@{
  /**
   * Set interpolation mode for downsampling (lowres GPU)
   * (initial value: cubic).
   */
  void SetInterpolationMode(int mode);
  int GetInterpolationMode();
  //@}

  void SetInterpolationModeToNearestNeighbor()
  {this->SetInterpolationMode(VTK_RESLICE_NEAREST);}

  void SetInterpolationModeToLinear()
  {this->SetInterpolationMode(VTK_RESLICE_LINEAR);}

  void SetInterpolationModeToCubic()
  {this->SetInterpolationMode(VTK_RESLICE_CUBIC);}

  //@{
  /**
   * Set/Get the number of samples/cells along the i/j/k directions.
   * The default is 128x128x128
   */
  vtkSetVector3Macro(NumberOfSamples,int);
  vtkGetVector3Macro(NumberOfSamples,int);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(vtkRenderer *ren, vtkVolume *vol) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

  void ProcessUpdateExtentRequest(vtkRenderer *renderer, vtkInformation*info,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);
  void ProcessInformationRequest(vtkRenderer *renderer, vtkInformation*info,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);
  void UpdateResampler(vtkRenderer *ren, vtkOverlappingAMR *amr);
  void UpdateResamplerFrustrumMethod(vtkRenderer *ren, vtkOverlappingAMR *amr);

  //@{
  /**
   * Select the type of resampling techinque approach to use.
   */
  vtkSetMacro(RequestedResamplingMode, int);
  vtkGetMacro(RequestedResamplingMode, int);
  vtkSetMacro(FreezeFocalPoint, bool);
  vtkGetMacro(FreezeFocalPoint, bool);
  //@}

  //@{
  /**
   * Sets/Gets the tolerance used to determine if the resampler needs
   * to be updated. Default is 10e-8
   */
  vtkSetMacro(ResamplerUpdateTolerance, double);
  vtkGetMacro(ResamplerUpdateTolerance, double);
  //@}

  //@{
  /**
   * Sets/Gets a flag that indicates the internal volume mapper
   * should use the  default number of threads.  This is useful in applications
   * such as ParaView that will turn off multiple threads by default. Default is false
   */
  vtkSetMacro(UseDefaultThreading, bool);
  vtkGetMacro(UseDefaultThreading, bool);
  //@}

  /**
   * Utility method used by UpdateResamplerFrustrumMethod() to compute the
   * bounds.
   */
  static bool ComputeResamplerBoundsFrustumMethod(
    vtkCamera* camera, vtkRenderer* renderer,
    const double data_bounds[6], double out_bounds[6]);

protected:
  vtkAMRVolumeMapper();
  ~vtkAMRVolumeMapper() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;
  void UpdateGrid();

  vtkSmartVolumeMapper *InternalMapper;
  vtkAMRResampleFilter *Resampler;
  vtkUniformGrid *Grid;
  int NumberOfSamples[3];
  double Bounds[6];
  // This indicates that the input has meta data for
  // doing demand driven operations.
  bool HasMetaData;
  int RequestedResamplingMode;
  bool FreezeFocalPoint;
  // Cached values for camera focal point and
  // the distance between the camera position and
  // focal point
  double LastFocalPointPosition[3];
  double LastPostionFPDistance;
  // This is used when determining if
  // either the camera or focal point has
  // move enough to cause the resampler to update
  double ResamplerUpdateTolerance;
  bool GridNeedsToBeUpdated;
  bool UseDefaultThreading;

private:
  vtkAMRVolumeMapper(const vtkAMRVolumeMapper&) = delete;
  void operator=(const vtkAMRVolumeMapper&) = delete;
};


#endif
