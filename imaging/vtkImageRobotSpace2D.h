/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRobotSpace2D.h
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
// .NAME vtkImageRobotSpace2D - 2D JointedRobots with 3+ degrees of freedom.
// .SECTION Description
// vtkImageRobotSpace2D Uses a 2D image as a work space, and defines
// a 2D robot from edges.

#ifndef __vtkImageRobotSpace2D_h
#define __vtkImageRobotSpace2D_h

#include <stdio.h>
#include <stdlib.h>
#include "vtkStateSpace.h"
#include "vtkRobotTransform2D.h"
#include "vtkRobotJoint2D.h"
#include "vtkImagePaint.h"
#include "vtkClaw.h"


class vtkImageRobotSpace2D : public vtkStateSpace
{
public:
  vtkImageRobotSpace2D();
  ~vtkImageRobotSpace2D();
  char *GetClassName() {return "vtkImageRobotSpace2D";};

  // Description:
  // Set/Get the robot that will be moving aroung the work space.
  void SetRobot(vtkRobotTransform2D *robot);
  vtkGetObjectMacro(Robot, vtkRobotTransform2D);
  
  // Description:
  // The three dregrees of freedom are: x, y and rotation.
  int GetDegreesOfFreedom(){return 3 + this->NumberOfJoints;};

  // Description:
  // This states have three variables coresponding to the three degrees
  // of freedom.
  int GetStateDimensionality(){return this->GetDegreesOfFreedom();};

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
  vtkGetObjectMacro(Canvas,vtkImagePaint);
  
  // Description:
  // Set/Get the threhold which defines collision space.
  vtkSetMacro(Threshold,float);
  vtkGetMacro(Threshold,float);
  
  // Description:
  // Set/Get the number of segments in the robot.
  void SetNumberOfJoints(int);
  vtkGetMacro(NumberOfJoints,int);

  // Description:
  // For building a robot.
  void AddJoint(vtkRobotJoint2D *joint);

  void ClearCanvas();
  void DrawRobot(float *state);
  // Description:
  // The robot is drawn with this value.
  void SetDrawValue(float val){if(Canvas)Canvas->SetDrawColor(val);};

  void AnimatePath(vtkClaw *planner);
  
  
  // For testing the object from tcl
  void DrawRobot(float x, float y, float t2, float t3)
  {float s[4]; s[0]=x; s[1]=y; s[2]=t2; s[3]=t3; this->DrawRobot(s);};
  void DrawRobot(float x, float y, float t2, float t3, float t4)
  {float s[7]; s[0]=x; s[1]=y; s[2]=t2; s[3]=t3; s[4]=t4; this->DrawRobot(s);};
    void DrawRobot(float x, float y, float t2, float t3, float t4, float t5, float t6)
  {float s[7]; s[0]=x; s[1]=y; s[2]=t2; s[3]=t3; s[4]=t4; s[5]=t5; s[6]=t6;
  this->DrawRobot(s);};

  void DrawChild(float x, float y, float t2, float t3, int axis, float d)
  {float s[4]; s[0]=x; s[1]=y; s[2]=t2; s[3]=t3; 
  GetChildState(s, axis, d, s); this->DrawRobot(s);};
  
  void PrintCollision(float x, float y, float t2, float t3)
  {float s[4]; s[0]=x; s[1]=y; s[2]=t2; s[3]=t3;
  if (this->Collide(s)) printf("Collision\n"); else printf("Free\n");};
  
  void PrintCollision(float x, float y, float t2, float t3, float t4, float t5, float t6)
  {float s[7]; s[0]=x; s[1]=y; s[2]=t2; s[3]=t3; s[4]=t4; s[5]=t5; s[6]=t6;
  if (this->Collide(s)) printf("Collision\n"); else printf("Free\n");};
  
  vtkGetMacro(RotationFactor, float);
  
  
protected:
  vtkRobotTransform2D *Robot;
  vtkImageRegion *WorkSpace;
  vtkImageRegion *DistanceMap;
  vtkImagePaint *Canvas;
  float Threshold;
  float RotationFactor;

  int NumberOfJoints;
  int MaximumNumberOfJoints;
  vtkRobotJoint2D **Joints;
  
  // Description:
  // This function make sure the circular parameter is in range -rad -> rad.
  // Same point, smaller(smallest) absolute parameter values.
  void Wrap(float *state);

};

#endif















