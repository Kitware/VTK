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
// Call SetNumberOfInputs(n) before calling SetInput(0) ... SetInput(n-1).
//
// Retrieve the outputs using GetOutput(0) ... GetOutput(n-1).
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

#include "vtkPointSetAlgorithm.h"

class vtkLandmarkTransform;
class vtkPointSet;
class vtkPoints;

class VTK_HYBRID_EXPORT vtkProcrustesAlignmentFilter : public vtkPointSetAlgorithm
{
public:
  vtkTypeMacro(vtkProcrustesAlignmentFilter,vtkPointSetAlgorithm);

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
  // Specify how many pointsets are going to be given as input.
  void SetNumberOfInputs(int n);

  // Description:
  // Specify the input pointset with index idx.
  // Call SetNumberOfInputs before calling this function.
  void SetInput(int idx, vtkPointSet* p);
  void SetInput(int idx, vtkDataObject* input);

  // Description:
  // When on, the initial alignment is to the centroid 
  // of the cohort curves.  When off, the alignment is to the 
  // centroid of the first input.  Default is off for
  // backward compatibility.
  vtkSetMacro(StartFromCentroid, bool);
  vtkGetMacro(StartFromCentroid, bool);
  vtkBooleanMacro(StartFromCentroid, bool);

  // Description:
  // Retrieve the input point set with index idx (usually only for pipeline
  // tracing).
  vtkPointSet* GetInput(int idx);

protected:
  vtkProcrustesAlignmentFilter();
  ~vtkProcrustesAlignmentFilter();

  // Description:
  // Usual data generation method.
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkLandmarkTransform *LandmarkTransform;

  bool StartFromCentroid;

  vtkPoints *MeanPoints;

private:
  vtkProcrustesAlignmentFilter(const vtkProcrustesAlignmentFilter&);  // Not implemented.
  void operator=(const vtkProcrustesAlignmentFilter&);  // Not implemented.
};

#endif


