/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPlacer.h

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



#ifndef __vtkPointPlacer_h
#define __vtkPointPlacer_h

#include "vtkObject.h"

class vtkRenderer;

class VTK_WIDGETS_EXPORT vtkPointPlacer : public vtkObject
{
public:
  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkPointPlacer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2], 
                                    double worldPos[3],
                                    double worldOrient[9] )=0;
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2], 
                                    double refWorldPos[3],
                                    double worldPos[3],
                                    double worldOrient[9] )=0;
  
  virtual int ValidateWorldPosition( double worldPos[3] )=0;
  virtual int ValidateWorldPosition( double worldPos[3],
                                     double worldOrient[9] )=0;
  
  vtkSetClampMacro(PixelTolerance,int,1,100);
  vtkGetMacro(PixelTolerance,int);

  vtkSetClampMacro(WorldTolerance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(WorldTolerance, double);

protected:
  vtkPointPlacer();
  ~vtkPointPlacer();

  int          PixelTolerance;
  double       WorldTolerance;
  
private:
  vtkPointPlacer(const vtkPointPlacer&);  //Not implemented
  void operator=(const vtkPointPlacer&);  //Not implemented
};

#endif
