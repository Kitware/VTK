/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSuperquadric
 * @brief   implicit function for a Superquadric
 *
 * vtkSuperquadric computes the implicit function and function gradient
 * for a superquadric. vtkSuperquadric is a concrete implementation of
 * vtkImplicitFunction.  The superquadric is centered at Center and axes
 * of rotation is along the y-axis. (Use the superclass'
 * vtkImplicitFunction transformation matrix if necessary to reposition.)
 * Roundness parameters (PhiRoundness and ThetaRoundness) control
 * the shape of the superquadric.  The Toroidal boolean controls whether
 * a toroidal superquadric is produced.  If so, the Thickness parameter
 * controls the thickness of the toroid:  0 is the thinnest allowable
 * toroid, and 1 has a minimum sized hole.  The Scale parameters allow
 * the superquadric to be scaled in x, y, and z (normal vectors are correctly
 * generated in any case).  The Size parameter controls size of the
 * superquadric.
 *
 * This code is based on "Rigid physically based superquadrics", A. H. Barr,
 * in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
 *
 * @warning
 * The Size and Thickness parameters control coefficients of superquadric
 * generation, and may do not exactly describe the size of the superquadric.
 *
*/

#ifndef vtkSuperquadric_h
#define vtkSuperquadric_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

#define VTK_MIN_SUPERQUADRIC_THICKNESS  1e-4

class VTKCOMMONDATAMODEL_EXPORT vtkSuperquadric : public vtkImplicitFunction
{
public:
  /**
   * Construct with superquadric radius of 0.5, toroidal off, center at 0.0,
   * scale (1,1,1), size 0.5, phi roundness 1.0, and theta roundness 0.0.
   */
  static vtkSuperquadric *New();

  vtkTypeMacro(vtkSuperquadric,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // ImplicitFunction interface
  double EvaluateFunction(double x[3]) VTK_OVERRIDE;
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;
  void EvaluateGradient(double x[3], double g[3]) VTK_OVERRIDE;

  //@{
  /**
   * Set the center of the superquadric. Default is 0,0,0.
   */
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
  //@}

  //@{
  /**
   * Set the scale factors of the superquadric. Default is 1,1,1.
   */
  vtkSetVector3Macro(Scale,double);
  vtkGetVectorMacro(Scale,double,3);
  //@}

  //@{
  /**
   * Set/Get Superquadric ring thickness (toroids only).
   * Changing thickness maintains the outside diameter of the toroid.
   */
  vtkGetMacro(Thickness,double);
  vtkSetClampMacro(Thickness,double,VTK_MIN_SUPERQUADRIC_THICKNESS,1.0);
  //@}

  //@{
  /**
   * Set/Get Superquadric north/south roundness.
   * Values range from 0 (rectangular) to 1 (circular) to higher orders.
   */
  vtkGetMacro(PhiRoundness,double);
  void SetPhiRoundness(double e);
  //@}

  //@{
  /**
   * Set/Get Superquadric east/west roundness.
   * Values range from 0 (rectangular) to 1 (circular) to higher orders.
   */
  vtkGetMacro(ThetaRoundness,double);
  void SetThetaRoundness(double e);
  //@}

  //@{
  /**
   * Set/Get Superquadric isotropic size.
   */
  vtkSetMacro(Size,double);
  vtkGetMacro(Size,double);
  //@}

  //@{
  /**
   * Set/Get whether or not the superquadric is toroidal (1) or ellipsoidal (0).
   */
  vtkBooleanMacro(Toroidal,int);
  vtkGetMacro(Toroidal,int);
  vtkSetMacro(Toroidal,int);
  //@}

protected:
  vtkSuperquadric();
  ~vtkSuperquadric() VTK_OVERRIDE {}

  int Toroidal;
  double Thickness;
  double Size;
  double PhiRoundness;
  double ThetaRoundness;
  double Center[3];
  double Scale[3];
private:
  vtkSuperquadric(const vtkSuperquadric&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSuperquadric&) VTK_DELETE_FUNCTION;
};

#endif


