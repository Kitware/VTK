/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2DRobotSpace.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImage2DRobotSpace - 2D Robots with 3 degrees of freedom.
// .SECTION Description
// vtkImage2DRobotSpace Uses a 2D image as a work space, and defines
// a 2D robot from edges.

#ifndef __vtkImage2DRobotSpace_h
#define __vtkImage2DRobotSpace_h

#include <stdio.h>
#include "vtkStateSpace.h"
#include "vtkImageRegion.h"
#include "vtkImageDraw.h"
#include "vtkClaw.h"


class vtkImage2DRobotSpace : public vtkStateSpace
{
public:
  vtkImage2DRobotSpace();
  ~vtkImage2DRobotSpace();
  char *GetClassName() {return "vtkImage2DRobotSpace";};

  // Description:
  // The three dregrees of freedom are: x, y and rotation.
  int GetDegreesOfFreedom(){return 3;};

  // Description:
  // This states have three variables coresponding to the three degrees
  // of freedom.
  int GetStateDimensionality(){return 3;};

  // Description:
  // Allocates a new state.
  float *NewState() {return new float[this->GetStateDimensionality()];};
  
  // Description:
  // Returns  0.0 if state is out of bounds
  float BoundsTest(float *state);

  // Description:
  // This function computes max distance between two points.
  // Manhatten distance. 
  float Distance(float *s0, float *s1);

  // Description:
  // This function determines collision space from free space
  int Collide(float *state);

  void GetMiddleState(float *s0, float *s1, float *middle);
  
  void GetChildState(float *state, int axis, float distance, float *child);
  
  // Description:
  // Set/Get the image which defines the space.
  void SetWorkSpace(vtkImageRegion *workSpace);
  vtkGetObjectMacro(WorkSpace,vtkImageRegion);
  // Description:
  // Get the distance map image used for collision detection.
  vtkGetObjectMacro(DistanceMap,vtkImageRegion);
  // Description:
  // Get the canvas image for displaying the robot state.
  vtkGetObjectMacro(Canvas,vtkImageDraw);
  
  // Description:
  // Set/Get the threhold which defines collision space.
  vtkSetMacro(Threshold,float);
  vtkGetMacro(Threshold,float);
  
  // Description:
  // Set/Get the number of segments in the robot.
  void SetNumberOfSegments(int);
  vtkGetMacro(NumberOfSegments,int);

  // Description:
  // Get the Rotation factor (small for large robots).
  vtkGetMacro(RotationFactor,float);

  // Description:
  // For building a robot.
  void AddSegment(float x0, float y0, float x1, float y1);

  void ClearCanvas();
  void DrawRobot(float *state);
  // Description:
  // The robot is drawn with this value.
  void SetDrawValue(float val){if(Canvas)Canvas->SetDrawValue(val);};

  // For testing the object from tcl
  void DrawRobot(float x, float y, float theta)
  {float state[3]; state[0] = x; state[1] = y; state[2] = theta;
  this->DrawRobot(state);};

  void DrawChild(float x, float y, float theta, int axis, float d)
  {float state[3]; state[0] = x; state[1] = y; state[2] = theta;
  GetChildState(state, axis, d, state); this->DrawRobot(state);};
  
  void PrintCollision(float x, float y, float theta)
  {float state[3]; state[0] = x; state[1] = y; state[2] = theta;
  if (this->Collide(state)) printf("Collision\n"); else printf("Free\n");};
  
  void AnimatePath(vtkClaw *planner);
  
  
protected:
  vtkImageRegion *WorkSpace;
  vtkImageRegion *DistanceMap;
  vtkImageDraw *Canvas;
  float Threshold;

  int NumberOfSegments;
  int MaximumNumberOfSegments;
  float *Segments;
  float RobotBounds[4];
  float RotationFactor;
  
  int CollideSegment(float x0, float y0, short d0,
		     float x1, float y1, short d1,
		     float length, short *map, 
		     int xInc, int yInc);
  // Description:
  // This function make sure the circular parameter is in range -rad -> rad.
  // Same point, smaller(smallest) absolute parameter values.
  void Wrap(float *state);

};

#endif















