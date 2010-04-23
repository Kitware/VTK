/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPowerWeighting.h
  
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

#ifndef __vtkPowerWeighting_h
#define __vtkPowerWeighting_h

#include "vtkArrayDataAlgorithm.h"

// .NAME vtkPowerWeighting - Given an arbitrary-dimension array of doubles,
// replaces each value x with x^Power.

// .SECTION Thanks
// Developed by Jason Shepherd (jfsheph@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkPowerWeighting : public vtkArrayDataAlgorithm
{
public:
  static vtkPowerWeighting* New();
  vtkTypeMacro(vtkPowerWeighting, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the exponent to apply (default: 2.0)
  vtkSetMacro(Power, double);
  vtkGetMacro(Power, double);

//BTX
protected:
  vtkPowerWeighting();
  ~vtkPowerWeighting();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkPowerWeighting(const vtkPowerWeighting&); // Not implemented
  void operator=(const vtkPowerWeighting&);   // Not implemented

  double Power;
//ETX
};

#endif

