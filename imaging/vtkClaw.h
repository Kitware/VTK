/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClaw.h
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
// .NAME vtkClaw - Path finding algorithm.
// .SECTION Description
// vtkClaw maps freespace with "spheres"


#ifndef __vtkClaw_h
#define __vtkClaw_h

#include "vtkObject.h"
#include "vtkStateSpace.h"



#define VTK_CLAW_DIMENSIONS 3





// Definitions of the search strategies
#define VTK_CLAW_NEAREST_NETWORK      0
#define VTK_CLAW_NEAREST_MINIMUM      1
#define VTK_CLAW_NEAREST_GLOBAL       2
#define VTK_CLAW_PIONEER_LOCAL        3
#define VTK_CLAW_PIONEER_GLOBAL       4
#define VTK_CLAW_NEAREST_NET_PIONEER  5
#define VTK_CLAW_WELL_NOISE           6
#define VTK_CLAW_WELL_DIRECTED_NOISE  7
#define VTK_CLAW_MINIMUM_WELL         8
#define VTK_CLAW_INSERT               9
#define VTK_CLAW_NARROW_WEL           10


typedef struct SphereList;
typedef struct Sphere;


struct Sphere
  {
  float Center[VTK_CLAW_DIMENSIONS];
  float Radius;         /* radius of this sphere */
  SphereList *Neighbors; /* other spheres whose centers fall within this box */
  char  SortValid;
  float Sort;           /* the sphere with the largest sort value get choosen */
  char SurfaceAreaValid;
  float SurfaceArea;
  short Visited;        /* For searching graph */
  Sphere *Nearest;      /* nearest sphere of another network */
  float NearestVal;     /* distance of that sphere */
  };
 
 
struct SphereList
  {
  Sphere *Item;
  SphereList *Next;
  };



class vtkClaw : public vtkObject
{
public:
  vtkClaw();
  ~vtkClaw();
  char *GetClassName() {return "vtkClaw";};
  
  void SetStateSpace(vtkStateSpace *space);
  void SetStartState(float *state);
  void SetStartState(float x, float y, float z)
  {float s[3]; s[0] = x; s[1] = y; s[2] = z; this->SetStartState(s);};
  void SetGoalState(float *state);
  void SetGoalState(float x, float y, float z)
  {float s[3]; s[0] = x; s[1] = y; s[2] = z; this->SetGoalState(s);};
  void SetSearchStrategies (int num, int *strategies);
  void GeneratePath();
  void SmoothPath(int number);
  int SmoothPath();
  void PrintFreeSpheres();
  void SavePath(char *fileName);
  SphereList *GetPath();
  int GetPathLength();
  void GetPathState(int idx, float *state);
  
  
protected:
  // Defines which search strategies to use.
  int NumberOfSearchStrategies;
  int SearchStrategies[20];
  // A path must be found between these two points.
  float StartState[VTK_CLAW_DIMENSIONS];
  float GoalState[VTK_CLAW_DIMENSIONS];
  // The initial radius of the start and goal spheres.
  float InitialSphereRadius;
  // The final path has to be valid when sampled at this resolution.
  float VerifyStep;
  // Callback function is envoked, and search strategy is changed this often.
  int SamplePeriod;
  // The percentage of time the goal network is searched vs. the start net.
  float GoalPercentage;
  // The distance children are spawned from parent as fraction of radius.
  float ChildFraction;
  // Any returned path is put here.
  SphereList *Path;
  // The object which defines the state space we are searching.
  vtkStateSpace *StateSpace;

  int SmoothSphere(Sphere *s);
  void SphereLinkVerifiedRecord(Sphere *b0, Sphere *b1);
  int SphereLinkVerifiedAlready(Sphere *b0, Sphere *b1);
  void SphereVerifiedLinksClear();
  int SphereLinkStates(float *s1, float *s2, float distance);
  int SphereStartGoalInitialize(float *start_state, float *goal_state,
				float radius);
  Sphere *SphereNew(float *center, Sphere *parent);
  Sphere *SphereAdd(Sphere *b);
  int PathVerify(SphereList *path);
  int SphereLinkVerify(Sphere *b0, Sphere *b1);
  SphereList *PathGenerate(int additional_Spheres, 
			   int network, float child_fraction);
  SphereList *PathGetValid(Sphere *start_Sphere, Sphere *goal_Sphere);
  SphereList *PathSearch(Sphere *start, Sphere *end);
  SphereList *PathUnravel(Sphere *start);
};

#endif


