/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricFunctionSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParametricFunctionSource - abstract interface for parametric functions
// .SECTION Description
// vtkParametricFunctionSource is an abstract interface that allows
// triangulations to be performed on surfaces defined by parametric mapping
// i.e. f(u,v)->(x,y,z) where u_min <= u < u_max, v_min <= v < v_max.  It
// generates a triangulated surface that can then be rendered.
//
// The interface contains a pure virtual function, called Evaluate(), that
// generates a point and the derivatives at that point which are then used to
// construct the surface.
//
// A second pure virtual function, called EvaluateScalar() can be used to 
// generate a scalar for the surface. This is only used if the user
// does not want to use one of the predefined scalar generation modes
// and needs to create a particular scalar for the surface.
//
// This class can only be used as an interface and as a base for 
// other classes. The derived class must implement the pure virtual
// functions Evaluate() and EvaluateScalar(). 
//
// The intent of this design is to allow the user to readily 
// implement new functions describing surfaces by just deriving 
// a class and, in most cases, implementing just the pure virtual 
// functions Evaluate() and EvaluateScalar().
//
// Derived classes implementing some orientable and non-orientable 
// surfaces are provided.
//
// .SECTION Caveats
// Care needs to be taken specifying the bounds correctly.
//
// .SECTION Thanks
// Andrew Maclean a.maclean@cas.edu.au for creating and contributing the
// class.
//
// .SECTION See Also
// Implementations of derived classes implementing non-orentable surfaces:
// vtkParametricBoy vtkParametricCrossCap vtkParametricFig8Klein
// vtkParametricKlein vtkParametricMobius vtkParametricRoman
//
// Implementations of derived classes implementing orientable surfaces:
// vtkParametricConicSpiral vtkParametricDini vtkParametricEnneper
// vtkParametricRandomHills vtkParametricSuperEllipsoid 
// vtkParametricSuperToroid vtkParametricTorus 
//
#ifndef __vtkParametricFunctionSource_h
#define __vtkParametricFunctionSource_h

#include "vtkPolyDataSource.h"

class vtkCellArray;

class VTK_GRAPHICS_EXPORT vtkParametricFunctionSource : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkParametricFunctionSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Enumerate the supported scalar generation modes.
  // <pre>
  // SCALAR_NONE, (default) scalars are not generated.
  // SCALAR_U, the scalar is set to the u-value. 
  // SCALAR_V, the scalar is set to the v-value.
  // SCALAR_U0, the scalar is set to 1 if u = (u_max - u_min)/2 = u_avg, 0 otherwise.
  // SCALAR_V0, the scalar is set to 1 if v = (v_max - v_min)/2 = v_avg, 0 otherwise.
  // SCALAR_U0V0, the scalar is 
  //   set to 1 if u == u_avg, 2 if v == v_avg, 3 if u = u_avg && v = v_avg, 0 otherwise.
  // SCALAR_MODULUS, the scalar is set to (sqrt(u*u+v*v)), this is measured relative to (u_avg,v_avg).
  // SCALAR_PHASE, the scalar is set to (atan2(v,u)) (in degrees, 0 to 360), this is measured relative to (u_avg,v_avg).
  // SCALAR_QUADRANT, the scalar is set to 1, 2, 3 or 4 
  //   depending upon the quadrant of the point (u,v).
  // SCALAR_X, the scalar is set to the x-value. 
  // SCALAR_Y, the scalar is set to the y-value. 
  // SCALAR_Z, the scalar is set to the z-value. 
  // SCALAR_DISTANCE, the scalar is set to (sqrt(x*x+y*y+z*z)). I.e. distance from the origin.
  // SCALAR_USER_DEFINED, the scalar is set to the value returned from EvaluateScalar().
  // </pre>
  enum SCALAR_MODE { SCALAR_NONE = 0, 
    SCALAR_U, SCALAR_V, 
    SCALAR_U0, SCALAR_V0, SCALAR_U0V0,
    SCALAR_MODULUS, SCALAR_PHASE, SCALAR_QUADRANT,
    SCALAR_X, SCALAR_Y, SCALAR_Z, SCALAR_DISTANCE,
    SCALAR_USER_DEFINED };
  //ETX

  // Description:
  // Set/Get the number of points in the u-direction.
  vtkSetMacro(NumberOfUPoints,int);
  vtkGetMacro(NumberOfUPoints,int);

  // Description:
  // Set/Get the number of points in the v-direction.
  vtkSetMacro(NumberOfVPoints,int);
  vtkGetMacro(NumberOfVPoints,int);

  // Description:
  // Set/Get the minimum u-value.
  vtkSetMacro(MinimumU,double);
  vtkGetMacro(MinimumU,double);

  // Description:
  // Set/Get the maximum u-value.
  vtkSetMacro(MaximumU,double);
  vtkGetMacro(MaximumU,double);

  // Description:
  // Set/Get the minimum v-value.
  vtkSetMacro(MinimumV,double);
  vtkGetMacro(MinimumV,double);

  // Description:
  // Set/Get the maximum v-value.
  vtkSetMacro(MaximumV,double);
  vtkGetMacro(MaximumV,double);

  // Description:
  // Set/Get the flag which joins the first triangle strip to the last one.
  vtkSetMacro(JoinUTessellation,int);
  vtkGetMacro(JoinUTessellation,int);
  vtkBooleanMacro(JoinUTessellation,int);

  // Description:
  // Set/Get the flag which joins the the ends of the triangle strips.
  vtkSetMacro(JoinVTessellation,int);
  vtkGetMacro(JoinVTessellation,int);
  vtkBooleanMacro(JoinVTessellation,int);

  // Description:
  // Set/Get the flag which joins the first triangle strip to 
  // the last one with a twist.
  // JoinUTessellation must also be set if this is set. Used when building
  // some non-orientable surfaces.
  vtkSetMacro(TwistUTessellation,int);
  vtkGetMacro(TwistUTessellation,int);
  vtkBooleanMacro(TwistUTessellation,int);

  // Description:
  // Set/Get the flag which joins the ends of the 
  // triangle strips with a twist.
  // JoinVTessellation must also be set if this is set. Used when building
  // some non-orientable surfaces.
  vtkSetMacro(TwistVTessellation,int);
  vtkGetMacro(TwistVTessellation,int);
  vtkBooleanMacro(TwistVTessellation,int);

  // Description:
  // Set/Get the flag which determines the ordering of the the 
  // vertices forming the triangle strips. The ordering of the 
  // points being inserted into the triangle strip is important 
  // because it determines the direction of the normals for the 
  // lighting. If set, the ordering is clockwise, otherwise the
  // ordering is anti-clockwise. Default is true (i.e. clockwise
  // ordering).
  vtkSetMacro(ClockwiseOrdering,int);
  vtkGetMacro(ClockwiseOrdering,int);
  vtkBooleanMacro(ClockwiseOrdering,int);

  // Description:
  // Set/Get the flag which determines whether derivatives are 
  // supplied by the user or whether to let VTK calculate them.
  // If set then the user must supply the derivatives in order 
  // for the normals to be calculated. Letting VTK calculate the
  // normals is OK for orientable surfaces, but for non-orientable 
  // surfaces, you should always supply the derivatives because 
  // VTK has no way to determine normals on a non-orientable surface.
  // Default is true.
  vtkSetMacro(DerivativesSupplied,int);
  vtkGetMacro(DerivativesSupplied,int);
  vtkBooleanMacro(DerivativesSupplied,int);

  // Description:
  // Get/Set the mode used for the scalar data.  The options are:
  // SCALAR_NONE, (default) scalars are not generated.
  // SCALAR_U, the scalar is set to the u-value. 
  // SCALAR_V, the scalar is set to the v-value.
  // SCALAR_U0, the scalar is set to 1 if u = (u_max - u_min)/2 = u_avg, 0 otherwise.
  // SCALAR_V0, the scalar is set to 1 if v = (v_max - v_min)/2 = v_avg, 0 otherwise.
  // SCALAR_U0V0, the scalar is 
  //   set to 1 if u == u_avg, 2 if v == v_avg, 3 if u = u_avg && v = v_avg, 0 otherwise.
  // SCALAR_MODULUS, the scalar is set to (sqrt(u*u+v*v)), this is measured relative to (u_avg,v_avg).
  // SCALAR_PHASE, the scalar is set to (atan2(v,u)) (in degrees, 0 to 360), this is measured relative to (u_avg,v_avg).
  // SCALAR_QUADRANT, the scalar is set to 1, 2, 3 or 4 
  //   depending upon the quadrant of the point (u,v).
  // SCALAR_X, the scalar is set to the x-value. 
  // SCALAR_Y, the scalar is set to the y-value. 
  // SCALAR_Z, the scalar is set to the z-value. 
  // SCALAR_DISTANCE, the scalar is set to (sqrt(x*x+y*y+z*z)). I.e. distance from the origin.
  // SCALAR_USER_DEFINED, the scalar is set to the value returned from EvaluateScalar().
  vtkSetClampMacro(ScalarMode, int, SCALAR_NONE, SCALAR_USER_DEFINED);
  vtkGetMacro(ScalarMode, int);
  void SetScalarModeToNone( void );
  void SetScalarModeToU( void );
  void SetScalarModeToV( void );
  void SetScalarModeToU0( void );
  void SetScalarModeToV0( void );
  void SetScalarModeToU0V0( void );
  void SetScalarModeToModulus( void );
  void SetScalarModeToPhase( void );
  void SetScalarModeToQuadrant( void );
  void SetScalarModeToX( void );
  void SetScalarModeToY( void );
  void SetScalarModeToZ( void );
  void SetScalarModeToDistance( void );
  void SetScalarModeToUserDefined( void );

  // Description:
  // Get the values of all the parameters used in the triangulator.
  // This excludes the derivatives supplied indicator.
  void GetAllParametricTriangulatorParameters (
    int & numberOfUPoints,
    int & numberOfVPoints,
    double & minimumU,
    double & maximumU,
    double & minimumV,
    double & maximumV,
    int & joinUTessellation,
    int & joinVTessellation,
    int & twistUTessellation,
    int & twistVTessellation,
    int & clockwiseOrdering,
    int & scalarMode
  );

  // Description:
  // Set the values of all the parameters used in the triangulator.
  // This excludes the derivatives supplied indicator.
  void SetAllParametricTriangulatorParameters (
    int numberOfUPoints,
    int numberOfVPoints,
    double minimumU,
    double maximumU,
    double minimumV,
    double maximumV,
    int joinUTessellation,
    int joinVTessellation,
    int twistUTessellation,
    int twistVTessellation,
    int clockwiseOrdering,
    int scalarMode
  );

  // Description:
  // Calculate Evaluate(u,v)->(Pt,Du,Dv).
  // This is a pure virtual function that must be instantiated in 
  // a derived class. 
  //
  // u,v are the parameters with Pt the returned cartesian point, 
  // Du, Dv are the derivatives of this point with respect to u and v.
  //
  // By setting DerivativesSupplied to false, Du and Dv are ignored 
  // and the normals are calculated using vtkPolyDataNormals.
  // Do not do this if the surface is non-orintable - if you do it, 
  // the normals will be incorrect.
  virtual void Evaluate(double u, double v, double Pt[3], 
                        double Du[3], double Dv[3]) = 0;

  // Description:
  // Calculate a user defined scalar using none, one or all of u,v,Pt,Du,Dv.
  // This is a pure virtual function that must be instantiated in 
  // a derived class. 
  //
  // u,v are the parameters with Pt being the the cartesian point, 
  // Du, Dv are the derivatives of this point with respect to u and v.
  // Pt, Du, Dv are obtained from Evaluate().
  //
  // This function is only called if the ScalarMode has the value
  // vtkParametricFunctionSource::userDefined
  //
  // If the user does not need to calculate a scalar, then the 
  // instantiated function should return zero.
  virtual double EvaluateScalar(double u, double v, double Pt[3], 
                                double Du[3], double Dv[3]) = 0;

protected:
  vtkParametricFunctionSource();
  virtual ~vtkParametricFunctionSource();

  // Usual data generation method
  void Execute();

  // Variables
  int NumberOfUPoints;
  int NumberOfVPoints;
  double MinimumU;
  double MaximumU;
  double MinimumV;
  double MaximumV;
  int JoinUTessellation;
  int JoinVTessellation;
  int TwistUTessellation;
  int TwistVTessellation;
  int ClockwiseOrdering;
  int DerivativesSupplied;
  int ScalarMode;

private:
  // Description:
  // Generate triangle strips from an ordered set of points.
  //
  // Given a parametrization f(u,v)->(x,y,z), this function generates 
  // a vtkCellAarray of point IDs over the range MinimumU <= u < MaximumU 
  // and MinimumV <= v < MaximumV.
  //
  // Before using this function, ensure that: NumberOfUPoints,
  // NumberOfVPoints, MinimumU, MaximumU, MinimumV, MaximumV,
  // JoinUTessellation, JoinVTessellation, TwistUTessellation,
  // TwistVTessellation, ordering are set appropriately for the surface.
  //
  void MakeTriangleStrips ( vtkCellArray * strips, int PtsU, int PtsV );
  
  vtkParametricFunctionSource(const vtkParametricFunctionSource&);  // Not implemented.
  void operator=(const vtkParametricFunctionSource&);  // Not implemented.

};

#endif
