/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSphereSource.h
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
// .NAME vtkPSphereSource - sphere source that supports pieces

#ifndef __vtkPSphereSource_h
#define __vtkPSphereSource_h

#include "vtkSphereSource.h"

class VTK_PARALLEL_EXPORT vtkPSphereSource : public vtkSphereSource
{
public:
  vtkTypeRevisionMacro(vtkPSphereSource,vtkSphereSource);

  // Description:
  // Construct sphere with radius=0.5 and default resolution 8 in both Phi
  // and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
  static vtkPSphereSource *New();

  // Description:
  // Get the estimated memory size in 1024 bytes
  unsigned long GetEstimatedMemorySize();
  
protected:
  vtkPSphereSource() {};
  ~vtkPSphereSource() {};

  void Execute();
private:
  vtkPSphereSource(const vtkPSphereSource&);  // Not implemented.
  void operator=(const vtkPSphereSource&);  // Not implemented.
};

#endif
