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
// .NAME vtkParametricFunctionSource - tessellate parametric functions
// .SECTION Description
// This class tessellates parametric functions. The user must specify how
// many points in the parametric coordinate directions are required (i.e.,
// the resolution), and the mode to use to generate scalars.
//
// .SECTION Thanks
// Andrew Maclean andrew.amaclean@gmail.com for creating and contributing
// the class.
//
// .SECTION See Also
// vtkParametricFunction
//
// Implementation of parametrics for 1D lines:
// vtkParametricSpline
//
// Subclasses of vtkParametricFunction implementing non-orentable surfaces:
// vtkParametricBoy vtkParametricCrossCap vtkParametricFigure8Klein
// vtkParametricKlein vtkParametricMobius vtkParametricRoman
//
// Subclasses of vtkParametricFunction implementing orientable surfaces:
// vtkParametricConicSpiral vtkParametricDini vtkParametricEllipsoid
// vtkParametricEnneper vtkParametricRandomHills vtkParametricSuperEllipsoid
// vtkParametricSuperToroid vtkParametricTorus
//
#ifndef vtkParametricFunctionSource_h
#define vtkParametricFunctionSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkParametricFunction;

class VTKFILTERSSOURCES_EXPORT vtkParametricFunctionSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkParametricFunctionSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a new instance with (50,50,50) points in the (u-v-w) directions.
  static vtkParametricFunctionSource *New();

  // Description:
  // Specify the parametric function to use to generate the tessellation.
  virtual void SetParametricFunction(vtkParametricFunction*);
  vtkGetObjectMacro(ParametricFunction,vtkParametricFunction);

  // Description:
  // Set/Get the number of subdivisions / tessellations in the u parametric
  // direction. Note that the number of tessellant points in the u
  // direction is the UResolution + 1.
  vtkSetClampMacro(UResolution,int,2,VTK_INT_MAX);
  vtkGetMacro(UResolution,int);

  // Description:
  // Set/Get the number of subdivisions / tessellations in the v parametric
  // direction. Note that the number of tessellant points in the v
  // direction is the VResolution + 1.
  vtkSetClampMacro(VResolution,int,2,VTK_INT_MAX);
  vtkGetMacro(VResolution,int);

  // Description:
  // Set/Get the number of subdivisions / tessellations in the w parametric
  // direction. Note that the number of tessellant points in the w
  // direction is the WResolution + 1.
  vtkSetClampMacro(WResolution,int,2,VTK_INT_MAX);
  vtkGetMacro(WResolution,int);

  // Description:
  // Set/Get the generation of texture coordinates. This is off by
  // default.
  // Note that this is only applicable to parametric surfaces
  // whose parametric dimension is 2.
  // Note that texturing may fail in some cases.
  vtkBooleanMacro(GenerateTextureCoordinates,int);
  vtkSetClampMacro(GenerateTextureCoordinates,int,0,1);
  vtkGetMacro(GenerateTextureCoordinates,int);

  // Description:
  // Set/Get the generation of normals. This is on by
  // default.
  // Note that this is only applicable to parametric surfaces
  // whose parametric dimension is 2.
  vtkBooleanMacro(GenerateNormals,int);
  vtkSetClampMacro(GenerateNormals,int,0,1);
  vtkGetMacro(GenerateNormals,int);

  // Description:
  // Enumerate the supported scalar generation modes.<br>
  // SCALAR_NONE - Scalars are not generated (default).<br>
  // SCALAR_U - The scalar is set to the u-value.<br>
  // SCALAR_V - The scalar is set to the v-value.<br>
  // SCALAR_U0 - The scalar is set to 1 if
  //  u = (u_max - u_min)/2 = u_avg, 0 otherwise.<br>
  // SCALAR_V0 - The scalar is set to 1 if
  //  v = (v_max - v_min)/2 = v_avg, 0 otherwise.<br>
  // SCALAR_U0V0 - The scalar is
  //  set to 1 if u == u_avg, 2 if v == v_avg,
  //  3 if u = u_avg && v = v_avg, 0 otherwise.<br>
  // SCALAR_MODULUS - The scalar is set to (sqrt(u*u+v*v)),
  //  this is measured relative to (u_avg,v_avg).<br>
  // SCALAR_PHASE - The scalar is set to (atan2(v,u))
  //  (in degrees, 0 to 360),
  //  this is measured relative to (u_avg,v_avg).<br>
  // SCALAR_QUADRANT - The scalar is set to 1, 2, 3 or 4.
  //  depending upon the quadrant of the point (u,v).<br>
  // SCALAR_X - The scalar is set to the x-value.<br>
  // SCALAR_Y - The scalar is set to the y-value.<br>
  // SCALAR_Z - The scalar is set to the z-value.<br>
  // SCALAR_DISTANCE - The scalar is set to (sqrt(x*x+y*y+z*z)).
  //  I.e. distance from the origin.<br>
  // SCALAR_USER_DEFINED - The scalar is set to the value
  //  returned from EvaluateScalar().<br>
  enum SCALAR_MODE { SCALAR_NONE = 0,
    SCALAR_U, SCALAR_V,
    SCALAR_U0, SCALAR_V0, SCALAR_U0V0,
    SCALAR_MODULUS, SCALAR_PHASE, SCALAR_QUADRANT,
    SCALAR_X, SCALAR_Y, SCALAR_Z, SCALAR_DISTANCE,
    SCALAR_FUNCTION_DEFINED };

  // Description:
  // Get/Set the mode used for the scalar data.
  // See SCALAR_MODE for a description of the types of scalars generated.
  vtkSetClampMacro(ScalarMode, int, SCALAR_NONE, SCALAR_FUNCTION_DEFINED);
  vtkGetMacro(ScalarMode, int);
  void SetScalarModeToNone( void ) {this->SetScalarMode(SCALAR_NONE);}
  void SetScalarModeToU( void ) {this->SetScalarMode(SCALAR_U);}
  void SetScalarModeToV( void ) {this->SetScalarMode(SCALAR_V);}
  void SetScalarModeToU0( void ) {this->SetScalarMode(SCALAR_U0);}
  void SetScalarModeToV0( void ) {this->SetScalarMode(SCALAR_V0);}
  void SetScalarModeToU0V0( void ) {this->SetScalarMode(SCALAR_U0V0);}
  void SetScalarModeToModulus( void ) {this->SetScalarMode(SCALAR_MODULUS);}
  void SetScalarModeToPhase( void ) {this->SetScalarMode(SCALAR_PHASE);}
  void SetScalarModeToQuadrant( void ) {this->SetScalarMode(SCALAR_QUADRANT);}
  void SetScalarModeToX( void ) {this->SetScalarMode(SCALAR_X);}
  void SetScalarModeToY( void ) {this->SetScalarMode(SCALAR_Y);}
  void SetScalarModeToZ( void ) {this->SetScalarMode(SCALAR_Z);}
  void SetScalarModeToDistance( void ) {this->SetScalarMode(SCALAR_DISTANCE);}
  void SetScalarModeToFunctionDefined( void )
    {this->SetScalarMode(SCALAR_FUNCTION_DEFINED);}

  // Description:
  // Return the MTime also considering the parametric function.
  unsigned long GetMTime();

  // Description:
  // Set/get the desired precision for the output points.
  // See the documentation for the vtkAlgorithm::Precision enum for an
  // explanation of the available precision settings.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkParametricFunctionSource();
  virtual ~vtkParametricFunctionSource();

  // Usual data generation method
  int RequestData(vtkInformation *info, vtkInformationVector **input,
                  vtkInformationVector *output);

  // Variables
  vtkParametricFunction *ParametricFunction;

  int UResolution;
  int VResolution;
  int WResolution;
  int GenerateTextureCoordinates;
  int GenerateNormals;
  int ScalarMode;
  int OutputPointsPrecision;

private:
  // Create output depending on function dimension
  void Produce1DOutput(vtkInformationVector *output);
  void Produce2DOutput(vtkInformationVector *output);

  // Description:
  // Generate triangles from an ordered set of points.
  //
  // Given a parametrization f(u,v)->(x,y,z), this function generates
  // a vtkCellAarray of point IDs over the range MinimumU <= u < MaximumU
  // and MinimumV <= v < MaximumV.
  //
  // Before using this function, ensure that: UResolution,
  // VResolution, MinimumU, MaximumU, MinimumV, MaximumV, JoinU, JoinV,
  // TwistU, TwistV, ordering are set appropriately for the parametric function.
  //
  void MakeTriangles ( vtkCellArray * strips, int PtsU, int PtsV );

  vtkParametricFunctionSource(const vtkParametricFunctionSource&);  // Not implemented.
  void operator=(const vtkParametricFunctionSource&);  // Not implemented.

};

#endif
