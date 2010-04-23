/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEntropyMatrixWeighting.h
  
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

// .NAME vtkEntropyMatrixWeighting - Term weight strategy where every term has weight related to its entropy
//
// .SECTION Description
//
// This filter assigns to all terms weight between 0 and 1 where 1
// indicates maximum information content and 0 indicates a term that
// may safely be used to line the birdcage.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkEntropyMatrixWeighting_h
#define __vtkEntropyMatrixWeighting_h

#include <vtkArrayDataAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkEntropyMatrixWeighting :
  public vtkArrayDataAlgorithm
{
public:
  static vtkEntropyMatrixWeighting* New();
  vtkTypeMacro(vtkEntropyMatrixWeighting, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the feature dimension.  Default: 0
  vtkGetMacro(FeatureDimension, int);
  vtkSetMacro(FeatureDimension, int);

//BTX
protected:
  vtkEntropyMatrixWeighting();
  ~vtkEntropyMatrixWeighting();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkEntropyMatrixWeighting(const vtkEntropyMatrixWeighting&); // Not implemented
  void operator=(const vtkEntropyMatrixWeighting&);   // Not implemented

  int FeatureDimension;
//ETX
};

#endif

