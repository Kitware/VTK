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
/**
 * @class   vtkCurvatures
 * @brief   compute curvatures (Gauss and mean) of a Polydata object
 *
 * vtkCurvatures takes a polydata input and computes the curvature of the
 * mesh at each point. Four possible methods of computation are available :
 *
 * Gauss Curvature
 * discrete Gauss curvature (K) computation,
 * \f$K(\text{vertex v}) = 2*\pi - \sum_{\text{facet neighbs f of v}} (\text{angle_f at v})\f$.
 * The contribution of every facet is for the moment weighted by \f$Area(facet)/3\f$.
 * The units of Gaussian Curvature are \f$[1/m^2]\f$.
 *
 * Mean Curvature
 * \f$H(vertex v) = \text{average over edges neighbs e of H(e)}\f$,
 * \f$H(edge e) = length(e) * dihedral\_angle(e)\f$.
 *
 * NB: dihedral_angle is the ORIENTED angle between -PI and PI,
 * this means that the surface is assumed to be orientable
 * the computation creates the orientation.
 * The units of Mean Curvature are [1/m].
 *
 * Maximum (\f$k_\max\f$) and Minimum (\f$k_\min\f$) Principal Curvatures
 * \f$k_\max = H + \sqrt{H^2 - K}\f$,
 * \f$k_\min = H - \sqrt{H^2 - K}\f$
 * Excepting spherical and planar surfaces which have equal principal
 * curvatures, the curvature at a point on a surface varies with the direction
 * one "sets off" from the point. For all directions, the curvature will pass
 * through two extrema: a minimum (\f$k_\min\f$) and a maximum (\f$k_\max\f$)
 * which occur at mutually orthogonal directions to each other.
 *
 * NB. The sign of the Gauss curvature is a geometric invariant, it should be
 * positive when the surface looks like a sphere, negative when it looks like a
 * saddle, however the sign of the Mean curvature is not, it depends on the
 * convention for normals. This code assumes that normals point outwards (i.e.
 * from the surface of a sphere outwards). If a given mesh produces curvatures
 * of opposite senses then the flag InvertMeanCurvature can be set and the
 * Curvature reported by the Mean calculation will be inverted.
 *
 * @par Thanks:
 * Philip Batchelor philipp.batchelor@kcl.ac.uk for creating and contributing
 * the class and Andrew Maclean a.maclean@acfr.usyd.edu.au for cleanups and
 * fixes. Thanks also to Goodwin Lawlor for contributing patch to calculate
 * principal curvatures
 *
 *
 *
*/

#ifndef vtkCurvatures_h
#define vtkCurvatures_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_CURVATURE_GAUSS 0
#define VTK_CURVATURE_MEAN  1
#define VTK_CURVATURE_MAXIMUM 2
#define VTK_CURVATURE_MINIMUM 3

class VTKFILTERSGENERAL_EXPORT vtkCurvatures : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCurvatures,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with curvature type set to Gauss
   */
  static vtkCurvatures *New();

  //@{
  /**
   * Set/Get Curvature type
   * VTK_CURVATURE_GAUSS: Gaussian curvature, stored as
   * DataArray "Gauss_Curvature"
   * VTK_CURVATURE_MEAN : Mean curvature, stored as
   * DataArray "Mean_Curvature"
   */
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
  //@}

  //@{
  /**
   * Set/Get the flag which inverts the mean curvature calculation for
   * meshes with inward pointing normals (default false)
   */
  vtkSetMacro(InvertMeanCurvature,vtkTypeBool);
  vtkGetMacro(InvertMeanCurvature,vtkTypeBool);
  vtkBooleanMacro(InvertMeanCurvature,vtkTypeBool);
  //@}

protected:
  vtkCurvatures();

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  /**
   * discrete Gauss curvature (K) computation,
   * cf http://www-ipg.umds.ac.uk/p.batchelor/curvatures/curvatures.html
   */
  void GetGaussCurvature(vtkPolyData *output);

  // discrete Mean curvature (H) computation,
  // cf http://www-ipg.umds.ac.uk/p.batchelor/curvatures/curvatures.html
  void GetMeanCurvature(vtkPolyData *output);

  /**
   * Maximum principal curvature \f$k_max = H + sqrt(H^2 -K)\f$
   */
  void GetMaximumCurvature(vtkPolyData *input, vtkPolyData *output);

  /**
   * Minimum principal curvature \f$k_min = H - sqrt(H^2 -K)\f$
   */
  void GetMinimumCurvature(vtkPolyData *input, vtkPolyData *output);


  // Vars
  int CurvatureType;
  vtkTypeBool InvertMeanCurvature;

private:
  vtkCurvatures(const vtkCurvatures&) = delete;
  void operator=(const vtkCurvatures&) = delete;

};

#endif


