/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcrustesAlignmentFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tim Hutton who developed and contributed this class

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
// Retrieve the outputs using GetOutput(0) ... GetOutput(n-1).
//
// The default (in vtkLandmarkTransform) is for a similarity alignment.
// For a rigid-body alignment (to build a 'size-and-shape' model) use: 
// GetLandmarkTransform()->SetModeToRigidBody(). 
//
// Affine alignments are not normally used but are left in for completeness:
// GetLandmarkTransform()->SetModeToAffine(). 
//
// vtkProcrustesAlignmentFilter is an implementation of:
// J.C. Gower (1975) Generalized Procrustes Analysis. Psychometrika, 40:33-51.
//
// .SECTION Caveats
// All of the input pointsets must have the same number of points.
//
// .SECTION See Also
// vtkLandmarkTransform

#ifndef __vtkProcrustesAlignmentFilter_h
#define __vtkProcrustesAlignmentFilter_h

#include "vtkSource.h"

class vtkLandmarkTransform;
class vtkPointSet;

class VTK_HYBRID_EXPORT vtkProcrustesAlignmentFilter : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkProcrustesAlignmentFilter,vtkSource);

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
  // Specify how many pointsets are going to be given as input.
  void SetNumberOfInputs(int n);

  // Description:
  // Specify the input pointset with index idx.
  // Call SetNumberOfInputs before calling this function.
  void SetInput(int idx,vtkPointSet* p);

  // Description:
  // Retrieve the output point set with index idx.
  vtkPointSet* GetOutput(int idx);

protected:
  vtkProcrustesAlignmentFilter();
  ~vtkProcrustesAlignmentFilter();

  // Description:
  // Usual data generation method.
  void Execute();

  // Description:
  // A wrapper function for accessing the inputs within the filter and subclasses.
  vtkPointSet* GetInput(int idx);

  vtkLandmarkTransform *LandmarkTransform;

private:
  vtkProcrustesAlignmentFilter(const vtkProcrustesAlignmentFilter&);  // Not implemented.
  void operator=(const vtkProcrustesAlignmentFilter&);  // Not implemented.
};

#endif


