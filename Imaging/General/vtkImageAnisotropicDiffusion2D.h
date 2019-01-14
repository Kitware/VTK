/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageAnisotropicDiffusion2D
 * @brief   edge preserving smoothing.
 *
 *
 * vtkImageAnisotropicDiffusion2D diffuses a 2d image iteratively.
 * The neighborhood of the diffusion is determined by the instance
 * flags. If "Edges" is on the 4 edge connected voxels
 * are included, and if "Corners" is on, the 4 corner connected voxels
 * are included.  "DiffusionFactor" determines how far a pixel value
 * moves toward its neighbors, and is insensitive to the number of
 * neighbors chosen.  The diffusion is anisotropic because it only occurs
 * when a gradient measure is below "GradientThreshold".  Two gradient measures
 * exist and are toggled by the "GradientMagnitudeThreshold" flag.
 * When "GradientMagnitudeThreshold" is on, the magnitude of the gradient,
 * computed by central differences, above "DiffusionThreshold"
 * a voxel is not modified.  The alternative measure examines each
 * neighbor independently.  The gradient between the voxel and the neighbor
 * must be below the "DiffusionThreshold" for diffusion to occur with
 * THAT neighbor.
 *
 * @sa
 * vtkImageAnisotropicDiffusion3D
*/

#ifndef vtkImageAnisotropicDiffusion2D_h
#define vtkImageAnisotropicDiffusion2D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"
class VTKIMAGINGGENERAL_EXPORT vtkImageAnisotropicDiffusion2D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageAnisotropicDiffusion2D *New();
  vtkTypeMacro(vtkImageAnisotropicDiffusion2D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method sets the number of iterations which also affects the
   * input neighborhood needed to compute one output pixel.  Each iteration
   * requires an extra pixel layer on the neighborhood.  This is only relevant
   * when you are trying to stream or are requesting a sub extent of the "wholeExtent".
   */
  void SetNumberOfIterations(int num);

  //@{
  /**
   * Get the number of iterations.
   */
  vtkGetMacro(NumberOfIterations,int);
  //@}

  //@{
  /**
   * Set/Get the difference threshold that stops diffusion.
   * when the difference between two pixel is greater than this threshold,
   * the pixels are not diffused.  This causes diffusion to avoid sharp edges.
   * If the GradientMagnitudeThreshold is set, then gradient magnitude is used
   * for comparison instead of pixel differences.
   */
  vtkSetMacro(DiffusionThreshold,double);
  vtkGetMacro(DiffusionThreshold,double);
  //@}

  //@{
  /**
   * The diffusion factor specifies how much neighboring pixels effect each other.
   * No diffusion occurs with a factor of 0, and a diffusion factor of 1 causes
   * the pixel to become the average of all its neighbors.
   */
  vtkSetMacro(DiffusionFactor,double);
  vtkGetMacro(DiffusionFactor,double);
  //@}

  //@{
  /**
   * Choose neighbors to diffuse (6 faces, 12 edges, 8 corners).
   */
  vtkSetMacro(Faces,vtkTypeBool);
  vtkGetMacro(Faces,vtkTypeBool);
  vtkBooleanMacro(Faces,vtkTypeBool);
  vtkSetMacro(Edges,vtkTypeBool);
  vtkGetMacro(Edges,vtkTypeBool);
  vtkBooleanMacro(Edges,vtkTypeBool);
  vtkSetMacro(Corners,vtkTypeBool);
  vtkGetMacro(Corners,vtkTypeBool);
  vtkBooleanMacro(Corners,vtkTypeBool);
  //@}

  //@{
  /**
   * Switch between gradient magnitude threshold and pixel gradient threshold.
   */
  vtkSetMacro(GradientMagnitudeThreshold,vtkTypeBool);
  vtkGetMacro(GradientMagnitudeThreshold,vtkTypeBool);
  vtkBooleanMacro(GradientMagnitudeThreshold,vtkTypeBool);
  //@}

protected:
  vtkImageAnisotropicDiffusion2D();
  ~vtkImageAnisotropicDiffusion2D() override {}

  int NumberOfIterations;
  double DiffusionThreshold;
  double DiffusionFactor;
  // to determine which neighbors to diffuse
  vtkTypeBool Faces;
  vtkTypeBool Edges;
  vtkTypeBool Corners;
  // What threshold to use
  vtkTypeBool GradientMagnitudeThreshold;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) override;
  void Iterate(vtkImageData *in, vtkImageData *out,
               double ar0, double ar1, int *coreExtent, int count);
private:
  vtkImageAnisotropicDiffusion2D(const vtkImageAnisotropicDiffusion2D&) = delete;
  void operator=(const vtkImageAnisotropicDiffusion2D&) = delete;
};

#endif



