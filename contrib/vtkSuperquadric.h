/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadric.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Halle, Brigham and Women's Hospital

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkSuperquadric - implicit function for a Superquadric
// .SECTION Description

// vtkSuperquadric computes the implicit function and function gradient
// for a superquadric. vtksuperquadric is a concrete implementation of
// vtkImplicitFunction.  The superquadric is centered at Center and axes
// of rotation is along the y-axis. (Use the superclass'
// vtkImplicitFunction transformation matrix if necessary to reposition.)
// Roundness parameters (PhiRoundness and ThetaRoundness) control
// the shape of the superquadric.  The Toroidal boolean controls whether
// a toroidal superquadric is produced.  If so, the Thickness parameter
// controls the thickness of the toroid:  0 is the thinnest allowable
// toroid, and 1 has a minimum sized hole.  The Scale parameters allow 
// the superquadric to be scaled in x, y, and z (normal vectors are correctly
// generated in any case).  The Size parameter controls size of the 
// superquadric.
//
// This code is based on "Rigid physically based superquadrics", A. H. Barr,
// in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
//
// .SECTION Caveats
// The Size and Thickness parameters control coefficients of superquadric
// generation, and may do not exactly describe the size of the superquadric.
//

#ifndef __vtkSuperquadric_h
#define __vtkSuperquadric_h

#include "vtkImplicitFunction.h"

#define VTK_MIN_SUPERQUADRIC_THICKNESS  1e-4

class VTK_EXPORT vtkSuperquadric : public vtkImplicitFunction
{
public:
  vtkSuperquadric();
  static vtkSuperquadric *New() {return new vtkSuperquadric;};
  const char *GetClassName() {return "vtkSuperquadric";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set the center of the superquadric. Default is 0,0,0.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the scale factors of the superquadric. Default is 1,1,1.
  vtkSetVector3Macro(Scale,float);
  vtkGetVectorMacro(Scale,float,3);

  // Description:
  // Set/Get Superquadric ring thickness (toriods only).
  // Changing thickness maintains the outside diameter of the toroid.
  vtkGetMacro(Thickness,float);
  vtkSetClampMacro(Thickness,float,VTK_MIN_SUPERQUADRIC_THICKNESS,1.0);

  // Description:
  // Set/Get Superquadric north/south roundness. 
  // Values range from 0 (rectanglar) to 1 (circular) to higher orders.
  vtkGetMacro(PhiRoundness,float);
  void SetPhiRoundness(float e); 

  // Description:
  // Set/Get Superquadric east/west roundness.
  // Values range from 0 (rectanglar) to 1 (circular) to higher orders.
  vtkGetMacro(ThetaRoundness,float);
  void SetThetaRoundness(float e);

  // Description:
  // Set/Get Superquadric isotropic size.
  vtkSetMacro(Size,float);
  vtkGetMacro(Size,float);

  // Description:
  // Set/Get whether or not the superquadric is toroidal (1) or episoidal (0).
  vtkBooleanMacro(Toroidal,int);
  vtkGetMacro(Toroidal,int);
  vtkSetMacro(Toroidal,int);

protected:
  int Toroidal;
  float Thickness;
  float Size;
  float PhiRoundness;
  float ThetaRoundness;
  float Center[3];
  float Scale[3];
};

#endif


