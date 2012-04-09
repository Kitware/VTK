/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCurvatures.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCurvatures - compute curvatures (Gauss and mean) of a Polydata object
// .SECTION Description
// vtkCurvatures takes a polydata input and computes the curvature of the
// mesh at each point. Four possible methods of computation are available :
//
// Gauss Curvature
// discrete Gauss curvature (K) computation,
// \f$K(vertex v) = 2*PI-\sum_{facet neighbs f of v} (angle_f at v)\f$
// The contribution of every facet is for the moment weighted by \f$Area(facet)/3\f$
// The units of Gaussian Curvature are \f$[1/m^2]\f$
//
// Mean Curvature
// \f$H(vertex v) = average over edges neighbs e of H(e)\f$
// \f$H(edge e) = length(e)*dihedral_angle(e)\f$
// NB: dihedral_angle is the ORIENTED angle between -PI and PI,
// this means that the surface is assumed to be orientable
// the computation creates the orientation
// The units of Mean Curvature are [1/m]
//
// Maximum (\f$k_max\f$) and Minimum (\f$k_min\f$) Principal Curvatures
// \f$k_max = H + sqrt(H^2 - K)\f$
// \f$k_min = H - sqrt(H^2 - K)\f$
// Excepting spherical and planar surfaces which have equal principal curvatures,
// the curvature at a point on a surface varies with the direction one "sets off"
// from the point. For all directions, the curvature will pass through two extrema:
// a minimum (\f$k_min\f$) and a maximum (\f$k_max\f$) which occur at mutually orthogonal
// directions to each other.
//
// NB. The sign of the Gauss curvature is a geometric ivariant, it should be +ve
// when the surface looks like a sphere, -ve when it looks like a saddle,
// however, the sign of the Mean curvature is not, it depends on the
// convention for normals - This code assumes that normals point outwards (ie
// from the surface of a sphere outwards). If a given mesh produces curvatures
// of opposite senses then the flag InvertMeanCurvature can be set and the
// Curvature reported by the Mean calculation will be inverted.
//
// .SECTION Thanks
// Philip Batchelor philipp.batchelor@kcl.ac.uk for creating and contributing
// the class and Andrew Maclean a.maclean@acfr.usyd.edu.au for cleanups and 
// fixes. Thanks also to Goodwin Lawlor for contributing patch to calculate
// principal curvatures

//
// .SECTION See Also
//

#ifndef __vtkCurvatures_h
#define __vtkCurvatures_h

#include "vtkPolyDataAlgorithm.h"

#define VTK_CURVATURE_GAUSS 0
#define VTK_CURVATURE_MEAN  1
#define VTK_CURVATURE_MAXIMUM 2
#define VTK_CURVATURE_MINIMUM 3

class VTK_GRAPHICS_EXPORT vtkCurvatures : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCurvatures,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with curvature type set to Gauss
  static vtkCurvatures *New();

  // Description:
  // Set/Get Curvature type
  // VTK_CURVATURE_GAUSS: Gaussian curvature, stored as
  // DataArray "Gauss_Curvature"
  // VTK_CURVATURE_MEAN : Mean curvature, stored as
  // DataArray "Mean_Curvature"
  vtkSetMacro(CurvatureType,int);
  vtkGetMacro(CurvatureType,int);
  void SetCurvatureTypeToGaussian()
  { this->SetCurvatureType(VTK_CURVATURE_GAUSS); }
  void SetCurvatureTypeToMean()
  { this->SetCurvatureType(VTK_CURVATURE_MEAN); }
  void SetCurvatureTypeToMaximum()
  { this->SetCurvatureType(VTK_CURVATURE_MAXIMUM); }
  void SetCurvatureTypeToMinimum()
  { this->SetCurvatureType(VTK_CURVATURE_MINIMUM); }

  // Description:
  // Set/Get the flag which inverts the mean curvature calculation for
  // meshes with inward pointing normals (default false)
  vtkSetMacro(InvertMeanCurvature,int);
  vtkGetMacro(InvertMeanCurvature,int);
  vtkBooleanMacro(InvertMeanCurvature,int);
protected:
  vtkCurvatures();

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // discrete Gauss curvature (K) computation,
  // cf http://www-ipg.umds.ac.uk/p.batchelor/curvatures/curvatures.html
  void GetGaussCurvature(vtkPolyData *output);

  // discrete Mean curvature (H) computation,
  // cf http://www-ipg.umds.ac.uk/p.batchelor/curvatures/curvatures.html
  void GetMeanCurvature(vtkPolyData *output);
  
  //Description:
  // Maximum principal curvature \f$k_max = H + sqrt(H^2 -K)\f$
  void GetMaximumCurvature(vtkPolyData *input, vtkPolyData *output);
  
  //Description:
  // Minimum principal curvature \f$k_min = H - sqrt(H^2 -K)\f$
  void GetMinimumCurvature(vtkPolyData *input, vtkPolyData *output);
  

  // Vars
  int CurvatureType;
  int InvertMeanCurvature;

private:
  vtkCurvatures(const vtkCurvatures&);  // Not implemented.
  void operator=(const vtkCurvatures&);  // Not implemented.

};

#endif


