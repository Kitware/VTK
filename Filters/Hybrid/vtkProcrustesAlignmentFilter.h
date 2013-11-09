/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcrustesAlignmentFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcrustesAlignmentFilter - aligns a set of pointsets together
// .SECTION Description
//
// vtkProcrustesAlignmentFilter is a filter that takes a set of pointsets
// (any object derived from vtkPointSet) and aligns them in a least-squares
// sense to their mutual mean. The algorithm is iterated until convergence,
// as the mean must be recomputed after each alignment.
//
// vtkProcrustesAlignmentFilter requires a vtkMultiBlock input consisting
// of vtkPointSets as first level children.
//
// The default (in vtkLandmarkTransform) is for a similarity alignment.
// For a rigid-body alignment (to build a 'size-and-shape' model) use:
//
//    GetLandmarkTransform()->SetModeToRigidBody().
//
// Affine alignments are not normally used but are left in for completeness:
//
//    GetLandmarkTransform()->SetModeToAffine().
//
// vtkProcrustesAlignmentFilter is an implementation of:
//
//    J.C. Gower (1975)
//    Generalized Procrustes Analysis. Psychometrika, 40:33-51.
//
// .SECTION Caveats
// All of the input pointsets must have the same number of points.
//
// .SECTION Thanks
// Tim Hutton and Rasmus Paulsen who developed and contributed this class
//
// .SECTION See Also
// vtkLandmarkTransform

#ifndef __vtkProcrustesAlignmentFilter_h
#define __vtkProcrustesAlignmentFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkLandmarkTransform;
class vtkPointSet;
class vtkPoints;

class VTKFILTERSHYBRID_EXPORT vtkProcrustesAlignmentFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkProcrustesAlignmentFilter,vtkMultiBlockDataSetAlgorithm);

  // Description:
  // Prints information about the state of the filter.
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates with similarity transform.
  static vtkProcrustesAlignmentFilter *New();

  // Description:
  // Get the internal landmark transform. Use it to constrain the number of
  // degrees of freedom of the alignment (i.e. rigid body, similarity, etc.).
  // The default is a similarity alignment.
  vtkGetObjectMacro(LandmarkTransform,vtkLandmarkTransform);

  // Description:
  // Get the estimated mean point cloud
  vtkGetObjectMacro(MeanPoints,vtkPoints);

  // Description:
  // When on, the initial alignment is to the centroid
  // of the cohort curves.  When off, the alignment is to the
  // centroid of the first input.  Default is off for
  // backward compatibility.
  vtkSetMacro(StartFromCentroid, bool);
  vtkGetMacro(StartFromCentroid, bool);
  vtkBooleanMacro(StartFromCentroid, bool);

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
  // the available precision settings. If the desired precision is
  // DEFAULT_PRECISION and any of the inputs are double precision, then the
  // mean points will be double precision. Otherwise, if the desired
  // precision is DEFAULT_PRECISION and all the inputs are single precision,
  // then the mean points will be single precision.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkProcrustesAlignmentFilter();
  ~vtkProcrustesAlignmentFilter();

  // Description:
  // Usual data generation method.
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkLandmarkTransform *LandmarkTransform;

  bool StartFromCentroid;

  vtkPoints *MeanPoints;
  int OutputPointsPrecision;

private:
  vtkProcrustesAlignmentFilter(const vtkProcrustesAlignmentFilter&);  // Not implemented.
  void operator=(const vtkProcrustesAlignmentFilter&);  // Not implemented.
};

#endif


