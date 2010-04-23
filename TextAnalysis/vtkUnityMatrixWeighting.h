/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnityMatrixWeighting.h
  
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

// .NAME vtkUnityMatrixWeighting - Term weight strategy where every term has weight 1.

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkUnityMatrixWeighting_h
#define __vtkUnityMatrixWeighting_h

#include <vtkArrayDataAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkUnityMatrixWeighting :
  public vtkArrayDataAlgorithm
{
public:
  static vtkUnityMatrixWeighting* New();
  vtkTypeMacro(vtkUnityMatrixWeighting, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the feature dimension.  Default: 0
  vtkGetMacro(FeatureDimension, int);
  vtkSetMacro(FeatureDimension, int);

//BTX
protected:
  vtkUnityMatrixWeighting();
  ~vtkUnityMatrixWeighting();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkUnityMatrixWeighting(const vtkUnityMatrixWeighting&); // Not implemented
  void operator=(const vtkUnityMatrixWeighting&);   // Not implemented

  int FeatureDimension;
//ETX
};

#endif

