/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthImageToPointCloud.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDepthImageToPointCloud
 * @brief   convert a depth image into a point cloud
 *
 *
 * vtkDepthImageToPointCloud is a filter that acquires its input
 * from a depth image and converts it to point cloud represented as a
 * vtkPolyData. This can then be used in a visualization pipeline.
 *
 * The filter takes two input images, one of which is optional. The first
 * image is a (required) depth image containing z-buffer values. The second
 * image is an (optional) scalar image. The information in the z-buffer
 * image, plus a specified camera, is used to generate x-y-z coordinates of
 * the output point cloud (i.e., the points in a vtkPolyData). The second
 * scalar image is (optionally) output as scalars to the output point
 * cloud. Note that the depth image must be a single component image, with
 * values ranging between the near and far clipping range [-1,1].
 *
 * Note that if only a single input is provided, then the input is
 * interpreted in one of two ways. First, if the "ZBuffer" point data is
 * provided, then the input image is assumed to be color scalars with the
 * depth data provided in the "ZBuffer" data array. (This is consistent with
 * the vtkRendererSource filter with DepthValues enabled.) Otherwise, the
 * input image is assumed to be a depth image.
 *
 * It is (optionally) possible to cull points located on the near and far
 * clipping planes. This may better simulate the generation of a scanned
 * object point cloud.
 *
 * @warning
 * For the camera to transform the image depths into a point cloud, this
 * filter makes assumptions about the origin of the depth image (and
 * associated color scalar image). This class performs point by point
 * transformation. The view matrix is used to transform each pixel. IMPORTANT
 * NOTE: The transformation occurs by normalizing the image pixels into the
 * (-1,1) view space (depth values are passed thru). The process follows the
 * vtkCoordinate class which is the standard for VTK rendering
 * transformations. Subtle differences in whether the lower left pixel origin
 * are at the center of the pixel versus the lower-left corner of the pixel
 * will make slight differences in how pixels are transformed. (Similarly for
 * the upper right pixel as well). This half pixel difference can cause
 * transformation issues. (The code is commented appropriately.)
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkRendererSource vtkWindowToImageFilter vtkCamera vtkPolyData
 * vtkCoordinate
*/

#ifndef vtkDepthImageToPointCloud_h
#define vtkDepthImageToPointCloud_h

#include "vtkRenderingImageModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"


class vtkCamera;

class VTKRENDERINGIMAGE_EXPORT vtkDepthImageToPointCloud : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard instantiation, type and print methods.
   */
  static vtkDepthImageToPointCloud *New();
  vtkTypeMacro(vtkDepthImageToPointCloud, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Return the MTime also considering the camera.
   */
  vtkMTimeType GetMTime();

  /**
   * Indicates what camera was used to generate the depth image. The camera
   * parameters define a transformation which is used to perform coordinate
   * conversion into the 3D x-y-z space of the point cloud.
   */
  void SetCamera(vtkCamera*);

  //@{
  /**
   * Returns the camera being used to generate the point cloud from the
   * depth image.
   */
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * Indicate whether to cull points that are located on the near clipping
   * plane. These typically are points that are part of the clipped foreground. By
   * default this is disabled.
   */
  vtkSetMacro(CullNearPoints,bool);
  vtkGetMacro(CullNearPoints,bool);
  vtkBooleanMacro(CullNearPoints,bool);
  //@}

  //@{
  /**
   * Indicate whether to cull points that are located on the far clipping
   * plane. These typically are points that are part of the background. By
   * default this is enabled.
   */
  vtkSetMacro(CullFarPoints,bool);
  vtkGetMacro(CullFarPoints,bool);
  vtkBooleanMacro(CullFarPoints,bool);
  //@}

  //@{
  /**
   * Indicate whether to output color scalar values along with the
   * point cloud (assuming that the scalar values are available on
   * input). By default this is enabled.
   */
  vtkSetMacro(ProduceColorScalars,bool);
  vtkGetMacro(ProduceColorScalars,bool);
  vtkBooleanMacro(ProduceColorScalars,bool);
  //@}

  //@{
  /**
   * Indicate whether to output a vertex cell array (i.e., Verts) in the
   * output point cloud. Some filters require this vertex cells to be
   * defined in order to execute properly. For example some mappers will
   * only render points if the vertex cells are defined.
   */
  vtkSetMacro(ProduceVertexCellArray,bool);
  vtkGetMacro(ProduceVertexCellArray,bool);
  vtkBooleanMacro(ProduceVertexCellArray,bool);
  //@}

  //@{
  /**
   * Set the desired precision for the output points.
   * See vtkAlgorithm::DesiredOutputPrecision for the available choices.
   * The default is double precision.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkDepthImageToPointCloud();
  ~vtkDepthImageToPointCloud();

  vtkCamera *Camera;
  bool CullNearPoints;
  bool CullFarPoints;
  bool ProduceColorScalars;
  bool ProduceVertexCellArray;
  int OutputPointsPrecision;

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);

  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **inInfo,
                                  vtkInformationVector *outInfo);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

private:
  vtkDepthImageToPointCloud(const vtkDepthImageToPointCloud&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDepthImageToPointCloud&) VTK_DELETE_FUNCTION;

};

#endif
