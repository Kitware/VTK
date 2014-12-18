/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageAnisotropicDiffusion3D - edge preserving smoothing.
//
// .SECTION Description
// vtkImageAnisotropicDiffusion3D  diffuses an volume iteratively.
// The neighborhood of the diffusion is determined by the instance
// flags. if "Faces" is on, the 6 voxels adjoined by faces are included
// in the neighborhood.  If "Edges" is on the 12 edge connected voxels
// are included, and if "Corners" is on, the 8 corner connected voxels
// are included.  "DiffusionFactor" determines how far a pixel value
// moves toward its neighbors, and is insensitive to the number of
// neighbors chosen.  The diffusion is anisotropic because it only occurs
// when a gradient measure is below "GradientThreshold".  Two gradient measures
// exist and are toggled by the "GradientMagnitudeThreshold" flag.
// When "GradientMagnitudeThreshold" is on, the magnitude of the gradient,
// computed by central differences, above "DiffusionThreshold"
// a voxel is not modified.  The alternative measure examines each
// neighbor independently.  The gradient between the voxel and the neighbor
// must be below the "DiffusionThreshold" for diffusion to occur with
// THAT neighbor.

// .SECTION See Also
// vtkImageAnisotropicDiffusion2D

#ifndef vtkImageAnisotropicDiffusion3D_h
#define vtkImageAnisotropicDiffusion3D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageAnisotropicDiffusion3D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageAnisotropicDiffusion3D *New();
  vtkTypeMacro(vtkImageAnisotropicDiffusion3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // This method sets the number of interations which also affects the
  // input neighborhood needed to compute one output pixel.  Each iterations
  // requires an extra pixel layer on the neighborhood.  This is only relavent
  // when you are trying to stream or are requesting a sub extent of the "wholeExtent".
  void SetNumberOfIterations(int num);

  // Description:
  // Get the number of iterations.
  vtkGetMacro(NumberOfIterations,int);

  // Description:
  // Set/Get the difference threshold that stops diffusion.
  // when the difference between two pixel is greater than this threshold,
  // the pixels are not diffused.  This causes diffusion to avoid sharp edges.
  // If the GradientMagnitudeThreshold is set, then gradient magnitude is used
  // for comparison instead of pixel differences.
  vtkSetMacro(DiffusionThreshold,double);
  vtkGetMacro(DiffusionThreshold,double);

  // Description:
  // Set/Get the difference factor
  vtkSetMacro(DiffusionFactor,double);
  vtkGetMacro(DiffusionFactor,double);

  // Description:
  // Choose neighbors to diffuse (6 faces, 12 edges, 8 corners).
  vtkSetMacro(Faces,int);
  vtkGetMacro(Faces,int);
  vtkBooleanMacro(Faces,int);
  vtkSetMacro(Edges,int);
  vtkGetMacro(Edges,int);
  vtkBooleanMacro(Edges,int);
  vtkSetMacro(Corners,int);
  vtkGetMacro(Corners,int);
  vtkBooleanMacro(Corners,int);

  // Description:
  // Switch between gradient magnitude threshold and pixel gradient threshold.
  vtkSetMacro(GradientMagnitudeThreshold,int);
  vtkGetMacro(GradientMagnitudeThreshold,int);
  vtkBooleanMacro(GradientMagnitudeThreshold,int);

protected:
  vtkImageAnisotropicDiffusion3D();
  ~vtkImageAnisotropicDiffusion3D() {}

  int NumberOfIterations;
  double DiffusionThreshold;
  double DiffusionFactor;
  // to determine which neighbors to diffuse
  int Faces;
  int Edges;
  int Corners;
  // What threshold to use
  int GradientMagnitudeThreshold;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id);
  void Iterate(vtkImageData *in, vtkImageData *out,
               double ar0, double ar1, double ar3, int *coreExtent, int count);
private:
  vtkImageAnisotropicDiffusion3D(const vtkImageAnisotropicDiffusion3D&);  // Not implemented.
  void operator=(const vtkImageAnisotropicDiffusion3D&);  // Not implemented.
};

#endif



