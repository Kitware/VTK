/*=========================================================================

  Program:   Visualization Library
  Module:    Follower.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlFollower - a subclass of actor that always faces the camera
// .SECTION Description
// vlFollowr is a subclass of vlActor that always follows its specified 
// camera.

#ifndef __vlFollower_hh
#define __vlFollower_hh

#include "Actor.hh"
class vlCamera;

class vlFollower : public vlActor
{
 public:
  vlFollower();
  ~vlFollower();
  char *GetClassName() {return "vlFollower";};
  void PrintSelf(ostream& os, vlIndent indent);

  virtual void GetMatrix(vlMatrix4x4& m);

  // Description:
  // Sets the Camera to follow
  vlSetObjectMacro(Camera,vlCamera);
  // Description:
  // Returns the Camera it is following
  vlGetObjectMacro(Camera,vlCamera);

protected:
  vlCamera *Camera; 
};

#endif

