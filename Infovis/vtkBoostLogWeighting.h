/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostLogWeighting.h
  
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

#ifndef __vtkBoostLogWeighting_h
#define __vtkBoostLogWeighting_h

#include "vtkArrayDataAlgorithm.h"

// .NAME vtkBoostLogWeighting - Given an arbitrary-dimension array of doubles,
// replaces each value with the base-e log of (value + 1)

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_INFOVIS_EXPORT vtkBoostLogWeighting : public vtkArrayDataAlgorithm
{
public:
  static vtkBoostLogWeighting* New();
  vtkTypeRevisionMacro(vtkBoostLogWeighting, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkBoostLogWeighting();
  ~vtkBoostLogWeighting();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkBoostLogWeighting(const vtkBoostLogWeighting&); // Not implemented
  void operator=(const vtkBoostLogWeighting&);   // Not implemented
};

#endif

