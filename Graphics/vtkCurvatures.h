/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCurvatures.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCurvatures - compute curvatures (Gauss and mean) of a Polydata object
// .SECTION Description
// vtkCurvatures takes a polydata input and computes the curvature of the
// mesh at each point. Two possible methods of computation are available :
//
// Gauss Curvature
// discrete Gauss curvature (K) computation,
// K(vertex v) = 2*PI-\sum_{facet neighbs f of v} (angle_f at v)
// The contribution of every facet is for the moment weighted by Area(facet)/3
//
// Mean Curvature
// H(vertex v) = average over edges neighbs e of H(e)
// H(edge e) = length(e)*dihedral_angle(e)
// NB: dihedral_angle is the ORIENTED angle between -PI and PI,
// this means that the surface is assumed to be orientable
// the computation creates the orientation
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
// fixes
//
// .SECTION See Also
//

#ifndef __vtkCurvatures_h
#define __vtkCurvatures_h

#include "vtkPolyDataToPolyDataFilter.h"

#define VTK_CURVATURE_GAUSS 0
#define VTK_CURVATURE_MEAN  1

class VTK_GRAPHICS_EXPORT vtkCurvatures : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkCurvatures,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with curvature type set to Gauss
  static vtkCurvatures *New();

  // Description:
  // Set/Get Curvature type
  // VTK_CURVATURE_GAUSS: Gauss curvature K, stored as DataArray "Gauss_Curvature"
  // VTK_CURVATURE_MEAN : Mean curvature  H, stored as DataArray "Mean_Curvature"
  vtkSetMacro(CurvatureType,int);
  vtkGetMacro(CurvatureType,int);
  void SetCurvatureTypeToGaussian()
  { this->SetCurvatureType(VTK_CURVATURE_GAUSS); }
  void SetCurvatureTypeToMean()
  { this->SetCurvatureType(VTK_CURVATURE_MEAN); }

  // Description:
  // Set/Get the flag which inverts the mean curvature calculation for
  // meshes with inward pointing normals (default false)
  vtkSetMacro(InvertMeanCurvature,int);
  vtkGetMacro(InvertMeanCurvature,int);
  vtkBooleanMacro(InvertMeanCurvature,int);
protected:
  vtkCurvatures();
  vtkCurvatures(const vtkCurvatures&) {};
  void operator=(const vtkCurvatures&) {};

  // Usual data generation method
  void Execute();

  // Description:
  // discrete Gauss curvature (K) computation,
  // cf http://www-ipg.umds.ac.uk/p.batchelor/curvatures/curvatures.html
  void GetGaussCurvature();

  // discrete Mean curvature (H) computation,
  // cf http://www-ipg.umds.ac.uk/p.batchelor/curvatures/curvatures.html
  void GetMeanCurvature();

  // Vars
  int CurvatureType;
  int InvertMeanCurvature;

private:

};

#endif


