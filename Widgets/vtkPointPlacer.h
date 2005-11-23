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

  // Description:
  // Given a renderer and a display position in pixel coordinates,
  // compute the world position and orientation where this point
  // will be placed. This method is typically used by the
  // representation to place the point initially.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2], 
                                    double worldPos[3],
                                    double worldOrient[9] )=0;
  
  // Description:
  // Given a renderer, a display position, and a reference world
  // position, compute the new world position and orientation 
  // of this point. This method is typically used by the 
  // representation to move the point.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2], 
                                    double refWorldPos[3],
                                    double worldPos[3],
                                    double worldOrient[9] )=0;
  
  // Description:
  // Given a world position check the validity of this 
  // position according to the constraints of the placer
  virtual int ValidateWorldPosition( double worldPos[3] )=0;
  
  // Description:
  // Given a world position and a world orientation,
  // validate it according to the constraints of the placer
  virtual int ValidateWorldPosition( double worldPos[3],
                                     double worldOrient[9] )=0;

  // Description:
  // Given a current renderer, world position and orientation,
  // update them according to the constraints of the placer.
  // This method is typically used when UpdateContour is called
  // on the representation, which must be called after changes
  // are made to the constraints in the placer. A return 
  // value of 1 indicates that the point has been updated. A
  // return value of 0 indicates that the point could not
  // be updated and was left alone. By default this is a no-op - 
  // leaving the point as is.
  virtual int UpdateWorldPosition( vtkRenderer *ren,
                                   double worldPos[3],
                                   double worldOrient[9] );
  

  // Description:
  // Called by the representation to give the placer a chance
  // to update itself. 
  virtual int UpdateInternalState() {return 0;}
  
  // Description:
  // Set/get the tolerance used when performing computations
  // in display coordinates
  vtkSetClampMacro(PixelTolerance,int,1,100);
  vtkGetMacro(PixelTolerance,int);

  // Description:
  // Set/get the tolerance used when performing computations
  // in world coordinates
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





