/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageAnisotropicDiffusion3D - edge preserving smoothing.
// .SECTION Description
// vtkImageAnisotropicDiffusion3D  diffuses an volume iteratively.
// The neighborhood of the diffusion is determined by the instance
// flags. if "Faces" is on, the 6 voxels adjoined by faces are included
// in the neighborhood.  If "Edges" is on the 12 edge connected voxels
// are included, and if "Corners" is on, the 8 corner connected voxels
// are included.  "DiffusionFactor" determines how far a pixel value
// moves toward its neighbors, and is insensitive to the number of 
// neighbors choosen.  The diffusion is anisotropic because it only occurs
// when a gradient mesure is below "GradientThreshold".  Two gradient measures
// exist and are toggled by the "GradientMagnitudeThreshold" flag.
// When "GradientMagnitudeThreshold" is on, the magnitude of the gradient,
// computed by central differences, above "DiffusionThreshold"
// a voxel is not modified.  The alternative measure examines each
// neighbor independantly.  The gradient between the voxel and the neighbor
// must be below the "DiffusionThreshold" for diffusion to occur with
// THAT neighbor.
//  Input and output can be any indpendent data type.


#ifndef __vtkImageAnisotropicDiffusion3D_h
#define __vtkImageAnisotropicDiffusion3D_h


#include "vtkImageSpatialFilter.h"

class VTK_EXPORT vtkImageAnisotropicDiffusion3D : public vtkImageSpatialFilter
{
public:
  vtkImageAnisotropicDiffusion3D();
  static vtkImageAnisotropicDiffusion3D *New() {return new vtkImageAnisotropicDiffusion3D;};
  char *GetClassName() {return "vtkImageAnisotropicDiffusion3D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetNumberOfIterations(int num);
  // Description:
  // Get the number of iterations.
  vtkGetMacro(NumberOfIterations,int);

  // Description:
  // Set/Get the difference threshold that stops diffusion.
  vtkSetMacro(DiffusionThreshold,float);
  vtkGetMacro(DiffusionThreshold,float);
  
  // Description:
  // Set/Get the difference factor
  vtkSetMacro(DiffusionFactor,float);
  vtkGetMacro(DiffusionFactor,float);

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
  int NumberOfIterations;
  float DiffusionThreshold;
  float DiffusionFactor;  
  // to determine which neighbors to diffuse
  int Faces;
  int Edges;
  int Corners;
  // What threshold to use
  int GradientMagnitudeThreshold;
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void Iterate(vtkImageRegion *in, vtkImageRegion *out, 
	       float ar0, float ar1, float ar3, int *coreExtent, int count);
};

#endif



