/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFocalPlanePointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME 
// .SECTION Description
// 
//
// .SECTION See Also

#ifndef __vtkFocalPlanePointPlacer_h
#define __vtkFocalPlanePointPlacer_h

#include "vtkPointPlacer.h"

class vtkRenderer;

class VTK_WIDGETS_EXPORT vtkFocalPlanePointPlacer : public vtkPointPlacer
{
public:
  // Description:
  // Instantiate this class.
  static vtkFocalPlanePointPlacer *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkFocalPlanePointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);

  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2], 
                            double refWorldPos[3],
                            double worldPos[3],
                            double worldOrient[9] );
  int ComputeWorldPosition( vtkRenderer *ren,
                            double displayPos[2], 
                            double worldPos[3],
                            double worldOrient[9] );
  int ValidateWorldPosition( double worldPos[3] );
  int ValidateWorldPosition( double worldPos[3],
                             double worldOrient[9]);
  
protected:
  vtkFocalPlanePointPlacer();
  ~vtkFocalPlanePointPlacer();

  void GetCurrentOrientation( double worldOrient[9] );
  
private:
  vtkFocalPlanePointPlacer(const vtkFocalPlanePointPlacer&);  //Not implemented
  void operator=(const vtkFocalPlanePointPlacer&);  //Not implemented
};

#endif
