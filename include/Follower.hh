/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Follower.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkFollower - a subclass of actor that always faces the camera
// .SECTION Description
// vtkFollowr is a subclass of vtkActor that always follows its specified 
// camera.

#ifndef __vtkFollower_hh
#define __vtkFollower_hh

#include "Actor.hh"
class vtkCamera;

class vtkFollower : public vtkActor
{
 public:
  vtkFollower();
  ~vtkFollower();
  char *GetClassName() {return "vtkFollower";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void GetMatrix(vtkMatrix4x4& m);

  // Description:
  // Sets the Camera to follow
  vtkSetObjectMacro(Camera,vtkCamera);
  // Description:
  // Returns the Camera it is following
  vtkGetObjectMacro(Camera,vtkCamera);

protected:
  vtkCamera *Camera; 
};

#endif

