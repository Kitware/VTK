/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrequencyMatrixWeighting.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkFrequencyMatrixWeighting - computes a weighting vector for an input matrix.

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkFrequencyMatrixWeighting_h
#define __vtkFrequencyMatrixWeighting_h

#include <vtkArrayDataAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkFrequencyMatrixWeighting :
  public vtkArrayDataAlgorithm
{
public:
  static vtkFrequencyMatrixWeighting* New();
  vtkTypeRevisionMacro(vtkFrequencyMatrixWeighting, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the feature dimension.  Default: 0
  vtkGetMacro(FeatureDimension, int);
  vtkSetMacro(FeatureDimension, int);

//BTX
  enum
  {
    ENTROPY = 0
  };
//ETX

  // Description:
  // Set the type of weighting vector to compute.  Default: ENTROPY
  vtkGetMacro(WeightType, int);
  vtkSetMacro(WeightType, int);

//BTX
protected:
  vtkFrequencyMatrixWeighting();
  ~vtkFrequencyMatrixWeighting();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkFrequencyMatrixWeighting(const vtkFrequencyMatrixWeighting&); // Not implemented
  void operator=(const vtkFrequencyMatrixWeighting&);   // Not implemented

  int FeatureDimension;
  int WeightType;
//ETX
};

#endif

