/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkGridTransform - a nonlinear warp transformation
// .SECTION Description
// vtkGridTransform describes a nonlinear warp transformation as a set
// of displacement vectors sampled along a uniform 3D grid.
// .SECTION Caveats
// The inverse grid transform is calculated using an iterative method,
// and is several times more expensive than the forward transform.
// .SECTION see also
// vtkThinPlateSplineTransform vtkGeneralTransform vtkTransformToGrid


#ifndef __vtkGridTransform_h
#define __vtkGridTransform_h

#include "vtkWarpTransform.h"
#include "vtkImageData.h"

#define VTK_GRID_NEAREST 0
#define VTK_GRID_LINEAR 1
#define VTK_GRID_CUBIC 3

class VTK_HYBRID_EXPORT vtkGridTransform : public vtkWarpTransform
{
public:
  static vtkGridTransform *New();
  vtkTypeMacro(vtkGridTransform,vtkWarpTransform);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the grid transform (the grid transform must have three 
  // components for displacement in x, y, and z respectively).
  // The vtkGridTransform class will never modify the data.
  vtkSetObjectMacro(DisplacementGrid,vtkImageData);
  vtkGetObjectMacro(DisplacementGrid,vtkImageData);

  // Description:
  // Set scale factor to be applied to the displacements.
  // This is used primarily for grids which contain integer
  // data types.  Default: 1
  vtkSetMacro(DisplacementScale,float);
  vtkGetMacro(DisplacementScale,float);

  // Description:
  // Set a shift to be applied to the displacements.  The shift
  // is applied after the scale, i.e. x = scale*y + shift.
  // Default: 0
  vtkSetMacro(DisplacementShift,float);
  vtkGetMacro(DisplacementShift,float);

  // Description:
  // Set interpolation mode for sampling the grid.  Higher-order
  // interpolation allows you to use a sparser grid.
  // Default: Linear.
  void SetInterpolationMode(int mode);
  vtkGetMacro(InterpolationMode,int);
  void SetInterpolationModeToNearestNeighbor()
    { this->SetInterpolationMode(VTK_GRID_NEAREST); };
  void SetInterpolationModeToLinear()
    { this->SetInterpolationMode(VTK_GRID_LINEAR); };
  void SetInterpolationModeToCubic()
    { this->SetInterpolationMode(VTK_GRID_CUBIC); };
  const char *GetInterpolationModeAsString();

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

  // Description:
  // Get the MTime.
  unsigned long GetMTime();

protected:
  vtkGridTransform();
  ~vtkGridTransform();
  vtkGridTransform(const vtkGridTransform&);
  void operator=(const vtkGridTransform&);

  // Description:
  // Update the displacement grid.
  void InternalUpdate();

  // Description:
  // Copy this transform from another of the same type.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  // Description:
  // Internal functions for calculating the transformation.
  void ForwardTransformPoint(const float in[3], float out[3]);
  void ForwardTransformPoint(const double in[3], double out[3]);

  void ForwardTransformDerivative(const float in[3], float out[3],
				  float derivative[3][3]);
  void ForwardTransformDerivative(const double in[3], double out[3],
				  double derivative[3][3]);

  void InverseTransformPoint(const float in[3], float out[3]);
  void InverseTransformPoint(const double in[3], double out[3]);

  void InverseTransformDerivative(const float in[3], float out[3],
				  float derivative[3][3]);
  void InverseTransformDerivative(const double in[3], double out[3],
				  double derivative[3][3]);

//BTX
  void (*InterpolationFunction)(float point[3], float displacement[3],
				float derivatives[3][3],
				void *gridPtr, int gridType,
				int inExt[6], int inInc[3]);
//ETX
  int InterpolationMode;
  vtkImageData *DisplacementGrid;
  float DisplacementScale;
  float DisplacementShift;
};

//BTX

//----------------------------------------------------------------------------
inline const char *vtkGridTransform::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
    {
    case VTK_GRID_NEAREST:
      return "NearestNeighbor";
    case VTK_GRID_LINEAR:
      return "Linear";
    case VTK_GRID_CUBIC:
      return "Cubic";
    default:
      return "";
    }
}  
//ETX

#endif





