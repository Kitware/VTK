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
class vtkStateSpace;

//#include "vtkImageXViewer.h"


#define VTK_CLAW_MAX_NUMBER_OF_SEARCH_STRATEGIES 20

// Definitions of the search strategies
#define VTK_CLAW_NEAREST_NETWORK      0
#define VTK_CLAW_NEAREST_MINIMUM      1
#define VTK_CLAW_NEAREST_GLOBAL       2
#define VTK_CLAW_PIONEER_LOCAL        3
#define VTK_CLAW_PIONEER_GLOBAL       4
#define VTK_CLAW_WELL_NOISE           6
#define VTK_CLAW_WELL_DIRECTED_NOISE  7
#define VTK_CLAW_MINIMUM_WELL         8
#define VTK_CLAW_INSERT               9
#define VTK_CLAW_NARROW_WEL           10


typedef struct SphereList;
typedef struct Sphere;


struct Sphere
  {
  float *Center;
  float Radius;         // radius of this sphere 
  SphereList *Neighbors; // other spheres whose centers fall within this box 
  char  SortValid;
  float Sort;           // the sphere with the largest sort value get choosen
  char SurfaceAreaValid;
  float SurfaceArea;
  short Visited;        // For searching graph
  Sphere *Nearest;      // nearest sphere of another network
  float NearestVal;     // distance of that sphere
  };
 
 
struct SphereList
  {
  Sphere *Item;
  SphereList *Next;
  };



class VTK_EXPORT vtkClaw : public vtkObject
{
public:
  vtkClaw();
  ~vtkClaw();
  char *GetClassName() {return "vtkClaw";};

  void CollisionsPrune();

  // Description:
  // When PruneCollisions is on, Collisions that are not limiting
  // any spheres are removed (every sample).
  vtkSetMacro(PruneCollisions, int);
  vtkGetMacro(PruneCollisions, int);
  vtkBooleanMacro(PruneCollisions, int);
  
  // Description:
  // Sample period id the number of spheres that are added before the
  // strategy changes.  Also, a call back method is called at the end
  // of each period.
  vtkSetMacro(SamplePeriod, int);
  vtkGetMacro(SamplePeriod, int);

  // Description:
  // Turing CallBackOn makes the planner report to the states space
  // When a collision is added, sphere is added, or a sample period is
  // finished.  The state space can display this information however it
  // wants.
  vtkSetMacro(CallBacks, int);
  vtkGetMacro(CallBacks, int);
  vtkBooleanMacro(CallBacks, int);
  
  // To customize the seach strategies.
  void ClearSearchStrategies();
  void AddSearchStrategy(int strategy);

  // Description:
  // Set/Get the ChildFraction.  New spheres are created from old spheres.
  // Child fraction is the fraction of the distance from the center to the
  // Parents surface.
  vtkSetMacro(ChildFraction, float);
  vtkGetMacro(ChildFraction, float);
  
  // Description:
  // Set/Get the NeighborFraction.  
  // Neighbor fraction determines how much spheres must overlap before they
  // are considered neighbors.  1=> spheres must touch, 0=>spheres can
  // never be neighbors.
  void SetNeighborFraction(float fraction);
  vtkGetMacro(NeighborFraction, float);
  
  void SetStateSpace(vtkStateSpace *space);
  void SetStartState(float *state);
  void SetStartState(float x, float y)
  {float s[2]; s[0] = x; s[1] = y; this->SetStartState(s);};
  void SetStartState(float x, float y, float z)
  {float s[3]; s[0] = x; s[1] = y; s[2] = z; this->SetStartState(s);};
  void SetStartState(float x, float y, float z, float t)
  {float s[4]; s[0]=x; s[1]=y; s[2]=z; s[3]=t; this->SetStartState(s);};
  void SetStartState(float x, float y, float z, float t3, float t4)
  {float s[7]; s[0]=x; s[1]=y; s[2]=z; s[3]=t3; s[4]=t4; this->SetStartState(s);};
  void SetStartState(float x, float y, float z, float t3, float t4, float t5, float t6)
  {float s[7]; s[0]=x; s[1]=y; s[2]=z; s[3]=t3; s[4]=t4; s[5]=t5; s[6]=t6;
  this->SetStartState(s);};

  void SetGoalState(float *state);
  void SetGoalState(float x, float y)
  {float s[2]; s[0] = x; s[1] = y; this->SetGoalState(s);};
  void SetGoalState(float x, float y, float z)
  {float s[3]; s[0] = x; s[1] = y; s[2] = z; this->SetGoalState(s);};
  void SetGoalState(float x, float y, float z, float t)
  {float s[4]; s[0]=x; s[1]=y; s[2]=z; s[3]=t; this->SetGoalState(s);};
  void SetGoalState(float x, float y, float z, float t3, float t4)
  {float s[7]; s[0]=x; s[1]=y; s[2]=z; s[3]=t3; s[4]=t4; this->SetGoalState(s);};
  void SetGoalState(float x, float y, float z, float t3, float t4, float t5, float t6)
  {float s[7]; s[0]=x; s[1]=y; s[2]=z; s[3]=t3; s[4]=t4; s[5]=t5; s[6]=t6;
  this->SetGoalState(s);};

  void SetSearchStrategies (int num, int *strategies);
  void GeneratePath();

  void ExplorePath(int number);
  int ExplorePath();

  void SmoothPath(int number);
  int SmoothPath();

  void PrintFreeSpheres();
  void SavePath(char *fileName);
  void LoadPath(char *fileName);
  int GetPathLength();
  void GetPathState(int idx, float *state);
  
  // Set/Get the percentage of time this goal network is searched.
  vtkSetMacro(GoalPercentage, float);
  vtkGetMacro(GoalPercentage, float);
  
  // Get the list of collisions
  SphereList *GetCollisions();
  // Get the list of spheres
  SphereList *GetSpheres();
  // Get A list of spheres that defines the path.
  SphereList *GetPath();
  // Get A list of all spheres
  SphereList *GetFreeSpheres(){return this->FreeSpheres;};
  // Get A list of all collision spheres
  SphereList *GetCollisionSpheres(){return this->Collisions;};
  
  
protected:
  // When this flag is set, collisions are pruned every sample.
  int PruneCollisions;
  
  // Flag to call state space to display collisions, spheres ...
  int CallBacks;
  
  // How many directions can we move (gotten from StateSpace.
  int DegreesOfFreedom;
  // The length of State vectors.
  int StateDimensionality;
  
  // Defines which search strategies to use.
  int NumberOfSearchStrategies;
  int SearchStrategies[VTK_CLAW_MAX_NUMBER_OF_SEARCH_STRATEGIES];
  // A path must be found between these two points.
  float *StartState;
  float *GoalState;
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
  // To avoid numerical errors and add some "tolerance"
  // neighbors have to be a little closer that r1+r2.
  // Distance between neighbors has to be less than frac*(r1+r2).
  float NeighborFraction;
  
  // Any returned path is put here.
  SphereList *Path;
  // The object which defines the state space we are searching.
  vtkStateSpace *StateSpace;
  // An array to compute which children are covered. ([2*DegreesOfFreedom]).
  float *Candidates;

  // Space approximation manger.
  // A list of points (stored as Sphere centers) which are not in free space
  SphereList *Collisions;
  SphereList *FreeSpheres;
  // Create spheres while testing links (but add them later.
  SphereList *DeferredSpheres;

  // Functions to handle Spheres and locking ...
  Sphere *SphereNew(float *center, Sphere *parent);
  void SphereAdd(Sphere *b);
  void NewDeferredSphere(float *center);
  void AddDeferredSpheres();
  void CheckForMergedNetworks(Sphere *s);
  void SphereCollisionAdd(Sphere *b, Sphere *parent);
  void CollisionAdd(float *state, Sphere *parent);
  
  Sphere *SphereMake(float *center, float radius, int visited);
  void SphereListElementFree(SphereList *l);
  void SphereListAllFree(SphereList *l);
  void SphereListItemRemove(SphereList **pl, void *ptr);
  SphereList *SphereListAdd(SphereList *l, Sphere *item);
  Sphere *SphereListNetworkBest(SphereList *list, int network);
  void FreeSpheresPrint();
  int SpheresFreeCount();
  int SpheresCollisionCount();
  void SphereRadiusReduce(Sphere *b, float radius);
  void SphereNeighborsPrune(Sphere *b);
  void SphereFree(Sphere *b);
  void SphereAllFree();
  float SphereNearestVal(Sphere *b);
  Sphere *SphereNearest(Sphere *b);
  void SpherePrint(Sphere *b);
  int SphereNumNeighbors(Sphere *b);
  float SphereCandidateValid(Sphere *b, float *proposed);
  float SphereCandidatesGet(Sphere *b);
  float SphereSurfaceArea(Sphere *b);
  void SpheresPrint(SphereList *spheres);
  int SpheresCount(SphereList *spheres);
  void SphereSearchStrategySet(int strategy);
  void SphereCandidateChoose(Sphere *b, float *proposed);
  float SphereNearestNetworkMoveEvaluate(Sphere *b, float *proposed);
  float SphereNearestGlobalMoveEvaluate(Sphere *b, float *proposed);
  float SpherePioneerLocalMoveEvaluate(Sphere *b, float *proposed);
  float SpherePioneerGlobalMoveEvaluate(Sphere *b, float *proposed);
  float SphereNoiseMoveEvaluate();
  float SphereNearestNoiseMoveEvaluate(Sphere *b, float *proposed);
  float SphereSort(Sphere *b);
  float SphereNearestNetworkSortCompute(Sphere *b);
  float SphereNearestMinimumSortCompute(Sphere *b);
  float SphereNearestGlobalSortCompute(Sphere *b);
  float SpherePioneerLocalSortCompute(Sphere *b);
  float SpherePioneerGlobalSortCompute(Sphere *b);
  float SphereMinimumWellSortCompute(Sphere *b);
  float SphereCloseToleranceSortCompute(Sphere *b);
  float SphereNarrowWellSortCompute(Sphere *b);
  int ExploreSphere(Sphere *s);
  int SmoothBend(Sphere *s1, Sphere *s2, Sphere *s3);
  void SphereLinkVerifiedRecord(Sphere *b0, Sphere *b1);
  int SphereLinkVerifiedAlready(Sphere *b0, Sphere *b1);
  void SphereVerifiedLinksClear();
  int SphereLinkStates(float *s1, float *s2, float distance);
  int SphereStartGoalInitialize(float *start_state, float *goal_state,
				float radius);
  int PathVerify(SphereList *path);
  int SphereLinkVerify(Sphere *b0, Sphere *b1);
  SphereList *PathGenerate(int additional_Spheres, int network);
  SphereList *PathGetValid(Sphere *start_Sphere, Sphere *goal_Sphere);
  SphereList *PathSearch(Sphere *start, Sphere *end);
  SphereList *PathUnravel(Sphere *start);
};

#endif


