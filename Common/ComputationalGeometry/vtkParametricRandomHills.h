/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricRandomHills.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricRandomHills - Generate a surface covered with randomly placed hills.
// .SECTION Description
// vtkParametricRandomHills generates a surface covered with randomly placed
// hills. Hills will vary in shape and height since the presence
// of nearby hills will contribute to the shape and height of a given hill.
// An option is provided for placing hills on a regular grid on the surface.
// In this case the hills will all have the same shape and height.
//
// For further information about this surface, please consult the
// technical description "Parametric surfaces" in http://www.vtk.org/documents.php
// in the "VTK Technical Documents" section in the VTk.org web pages.
//
// .SECTION Thanks
// Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
// class.
//
#ifndef vtkParametricRandomHills_h
#define vtkParametricRandomHills_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkParametricFunction.h"

class vtkDoubleArray;
class vtkMinimalStandardRandomSequence;

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkParametricRandomHills : public vtkParametricFunction
{

public:
  vtkTypeMacro(vtkParametricRandomHills,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Return the parametric dimension of the class.
  virtual int GetDimension() {return 2;}

  // Description:
  // Construct a surface of random hills with the following parameters:
  // MinimumU = -10, MaximumU = 10,
  // MinimumV = -10, MaximumV = 10,
  // JoinU = 0, JoinV = 0,
  // TwistU = 0, TwistV = 0;
  // ClockwiseOrdering = 1,
  // DerivativesAvailable = 0,
  // Number of hills = 30,
  // Variance of the hills 2.5 in both x- and y- directions,
  // Scaling factor for the variances 1/3 in both x- and y- directions,
  // Amplitude of each hill = 2,
  // Scaling factor for the amplitude = 1/3,
  // RandomSeed = 1,
  // AllowRandomGeneration = 1.
  static vtkParametricRandomHills *New();

  // Description:
  // Set/Get the number of hills.
  // Default is 30.
  vtkSetMacro(NumberOfHills,int);
  vtkGetMacro(NumberOfHills,int);

  // Description:
  // Set/Get the hill variance in the x-direction.
  // Default is 2.5.
  vtkSetMacro(HillXVariance,double);
  vtkGetMacro(HillXVariance,double);

  // Description:
  // Set/Get the hill variance in the y-direction.
  // Default is 2.5.
  vtkSetMacro(HillYVariance,double);
  vtkGetMacro(HillYVariance,double);

  // Description:
  // Set/Get the hill amplitude (height).
  // Default is 2.
  vtkSetMacro(HillAmplitude,double);
  vtkGetMacro(HillAmplitude,double);

  // Description:
  // Set/Get the Seed for the random number generator,
  // a value of 1 will initialize the random number generator,
  // a negative value will initialize it with the system time.
  // Default is 1.
  vtkSetMacro(RandomSeed,int);
  vtkGetMacro(RandomSeed,int);

  // Description:
  // Set/Get the random generation flag.
  // A value of 0 will disable the generation of random hills on the surface
  // allowing a reproducible number of identically shaped hills to be
  // generated. If zero, then the number of hills used will be the nearest
  // perfect square less than or equal to the number of hills.
  // For example, selecting 30 hills will result in a 5 X 5 array of
  // hills being generated. Thus a square array of hills will be generated.
  //
  // Any other value means that the hills will be placed randomly on the
  // surface.
  // Default is 1.
  vtkSetClampMacro(AllowRandomGeneration,int,0,1);
  vtkGetMacro(AllowRandomGeneration,int);
  vtkBooleanMacro(AllowRandomGeneration,int);

  // Description:
  // Set/Get the scaling factor for the variance in the x-direction.
  // Default is 1/3.
  vtkSetMacro(XVarianceScaleFactor,double);
  vtkGetMacro(XVarianceScaleFactor,double);

  // Description:
  // Set/Get the scaling factor for the variance in the y-direction.
  // Default is 1/3.
  vtkSetMacro(YVarianceScaleFactor,double);
  vtkGetMacro(YVarianceScaleFactor,double);

  // Description:
  // Set/Get the scaling factor for the amplitude.
  // Default is 1/3.
  vtkSetMacro(AmplitudeScaleFactor,double);
  vtkGetMacro(AmplitudeScaleFactor,double);

  // Description:
  // Generate the centers of the hills, their standard deviations and
  // their amplitudes. This function creates a series of vectors representing
  // the u, v coordinates of each hill, its variance in the u, v directions and
  // the amplitude.
  // \deprecated 6.2 This function is no longer needed.
  VTK_LEGACY(void GenerateTheHills(void));

  // Description:
  // Construct a terrain consisting of hills on a surface.
  //
  // This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
  // as Pt. It also returns the partial derivatives Du and Dv.
  // \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
  // Then the normal is \f$N = Du X Dv\f$ .
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]);

  // Description:
  // Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
  //
  // uvw are the parameters with Pt being the the Cartesian point,
  // Duvw are the derivatives of this point with respect to u, v and w.
  // Pt, Duvw are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED
  //
  // If the user does not need to calculate a scalar, then the
  // instantiated function should return zero.
  //
  virtual double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]);

protected:
  vtkParametricRandomHills();
  ~vtkParametricRandomHills();

  // Variables
  int NumberOfHills;
  double HillXVariance;
  double HillYVariance;
  double HillAmplitude;
  int RandomSeed;
  double XVarianceScaleFactor;
  double YVarianceScaleFactor;
  double AmplitudeScaleFactor;
  int AllowRandomGeneration;

  // These variables store the previous values of the above ones.
  int previousNumberOfHills;
  double previousHillXVariance;
  double previousHillYVariance;
  double previousHillAmplitude;
  int previousRandomSeed;
  double previousXVarianceScaleFactor;
  double previousYVarianceScaleFactor;
  double previousAmplitudeScaleFactor;
  int previousAllowRandomGeneration;

private:
  vtkParametricRandomHills(const vtkParametricRandomHills&);  // Not implemented.
  void operator=(const vtkParametricRandomHills&);  // Not implemented.

  // Description:
  // Initialise the random number generator.
  void InitRNG ( int RandomSeed );

  // Description:
  // Return a random number between 0 and 1.
  double Rand ( void );

  // Description:
  // A random sequence generator.
  vtkMinimalStandardRandomSequence *randomSequenceGenerator;

  // Description:
  // Generate the centers of the hills, their standard deviations and
  // their amplitudes. This function creates a series of vectors representing
  // the u, v coordinates of each hill, their variances in the u, v directions
  // and their amplitudes.
  void MakeTheHillData( void );

  // Description:
  // True if any parameters have changed.
  bool ParametersChanged();

  // Description:
  // Set the previous values of the parameters with the current values.
  void CopyParameters();

  // Description:
  // Centers (x,y), variances (x,y) and amplitudes of the hills.
  vtkDoubleArray *hillData;
};

#endif
