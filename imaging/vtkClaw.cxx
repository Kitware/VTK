/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClaw.cxx
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


// Startint to convert the path planner to VTK (again)
// Minimal changes to get working.
// 10/31/96 -- happy Holloween



// States are just vectors.
// User supplies a Space object which can compute:
// The distance between states, a guide tube function and

// Outstanding issue:  How do we return a path (what format).


#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include "vtkClaw.h"



/* The current start and goal Sphere of the planner */
static Sphere *START_SPHERE = NULL;
static Sphere *GOAL_SPHERE = NULL;






//----------------------------------------------------------------------------
vtkClaw::vtkClaw()
{
  this->StartState = this->GoalState = NULL;
  this->InitialSphereRadius = 2.0;
  this->VerifyStep = 1.0;
  this->SamplePeriod = 200;
  this->GoalPercentage = 20.0;
  this->ChildFraction = 0.75;
  this->NeighborFraction = 0.9;
  this->Path = NULL;
  this->StateSpace = NULL;
  this->Candidates = NULL;

  this->Viewer = NULL;
  
  this->FreeSpheres = NULL;
  this->Collisions = NULL;
  
  this->SearchStrategies[0] = VTK_CLAW_NEAREST_NETWORK;
  //this->SearchStrategies[1] = VTK_CLAW_NEAREST_GLOBAL; // ...
  this->SearchStrategies[2] = VTK_CLAW_PIONEER_LOCAL;
  this->SearchStrategies[3] = VTK_CLAW_WELL_NOISE;
  this->NumberOfSearchStrategies = 3;
  
}

//----------------------------------------------------------------------------
vtkClaw::~vtkClaw()
{
}
  
  
//----------------------------------------------------------------------------
// Description:
// Set the state space to search.  This should be done first.
void vtkClaw::SetStateSpace(vtkStateSpace *space)
{
  int idx;

  this->StateSpace = space;
  this->Modified();
  this->StateDimensionality = space->GetStateDimensionality();
  this->DegreesOfFreedom = space->GetDegreesOfFreedom();
  
  // Free previos allocated memory
  if (this->Candidates)
    {
    delete [] this->Candidates;
    }
  if (this->StartState)
    {
    delete [] this->StartState;
    }
  if (this->GoalState)
    {
    delete [] this->GoalState;
    }
  
  // Allocate static temporary array used to compute uncovered candidates.
  this->Candidates = (int *)malloc(sizeof(int) * this->DegreesOfFreedom * 2);
  // Allocate start and goal states.
  this->StartState = space->NewState();
  this->GoalState = space->NewState();
  
  // Initialize to 0.
  for (idx = 0; idx < this->StateDimensionality; ++idx)
    {
    this->StartState[idx] = this->GoalState[idx] = 0.0;
    }
  for (idx = 0; idx < this->DegreesOfFreedom * 2; ++idx)
    {
    this->Candidates[idx] = 0;
    }
}



//----------------------------------------------------------------------------
// This method sets the starting position (transform)
void vtkClaw::SetStartState(float *state)
{
  int idx;
  
  if ( ! this->StateSpace)
    {
    vtkErrorMacro(<< "SetStartSpace: Set StateSpace first!");
    return;
    }

  this->Modified();
  for (idx = 0; idx < this->StateDimensionality; ++idx)
    {
    this->StartState[idx] = state[idx];
    }
}


//----------------------------------------------------------------------------
// This method sets the goal position (transform)
void vtkClaw::SetGoalState(float *state)
{
  int idx;
  
  if ( ! this->StateSpace)
    {
    vtkErrorMacro(<< "SetGoalSpace: Set StateSpace first!");
    return;
    }

  this->Modified();
  for (idx = 0; idx < this->StateDimensionality; ++idx)
    {
    this->GoalState[idx] = state[idx];
    }
}



//----------------------------------------------------------------------------
// This method sets the search strategys
void vtkClaw::SetSearchStrategies (int num, int *strategies)
{
  int idx;

  /* make sure at least one strategy is set */
  if (num <= 0 || num >= 20)
    {
    vtkErrorMacro(<< "SetSearchStrategies: Bad number");
    return;
    }
    
  // Set the strategies
  for (idx = 0; idx < num; ++idx)
    {
    this->SearchStrategies[idx] = strategies[idx];
    }
  this->NumberOfSearchStrategies = num;

  // do not modify because this does not invalidate the path.
  // this->Modified();
}

  





// Global that gives search strategies a scale.
static float ROBOT_RADIUS;


//#include "vtkImageXViewer.h"
#include "vtkImageRobotSpace2D.h"
//----------------------------------------------------------------------------
void vtkClaw::GeneratePath()
{
  SphereList *path = NULL;
  int goalIterations, startIterations;
  int strategyIdx;
  //vtkImageRobotSpace2D *rSpace; // ...
  
  if (this->Viewer)
    {
    this->Viewer->Delete();
    }
  //this->Viewer = new vtkImageXViewer; // ...
  
  //rSpace = (vtkImageRobotSpace2D *)(this->StateSpace); // ...
  //this->Viewer->SetInput(rSpace->GetCanvas()->GetOutput()); // ...
  //this->Viewer->Render(); // ...

  // Give the space some scale.
  ROBOT_RADIUS = this->InitialSphereRadius * 2;
  
  /* get rid of all the Spheres from previous runs */
  // SphereAllFree();
  // this->SphereVerifiedLinksClear();

  if ( ! START_SPHERE)
    {
    this->SphereStartGoalInitialize(this->StartState, this->GoalState, 
				    this->InitialSphereRadius);
    }
  
  /* call the path generation routines */
  strategyIdx = -1;
  while( ! path)
    {
    /* determine how much time to spend searching goal */
    goalIterations = (int)(this->SamplePeriod*this->GoalPercentage * 0.01);
    startIterations = this->SamplePeriod - goalIterations;
    
    /* Change search strategy */
    ++strategyIdx;
    if (strategyIdx >= this->NumberOfSearchStrategies)
      {
      strategyIdx = 0;
      }
    this->SphereSearchStrategySet(this->SearchStrategies[strategyIdx]);
    
    /* search the start network */
    if (! path && startIterations)
      {
      vtkDebugMacro(<< "Searching start");
      path = this->PathGenerate(startIterations, 0, this->ChildFraction);
      vtkDebugMacro(<< "num free = " << SpheresFreeCount()
                    << ", num collisions = " << SpheresCollisionCount());
      }

    /* search the goal network */
    if (! path && goalIterations)
      {
      vtkDebugMacro(<< "Searching goal");
      path = PathGenerate(goalIterations, 1, this->ChildFraction);
      vtkDebugMacro(<< "num free = " << SpheresFreeCount()
                    << ", num collisions = " << SpheresCollisionCount());
      }

    SphereCollisionsPrune();
    
    //this->Viewer->Render(); // ...
    //rSpace->ClearCanvas(); // ...
    
    fflush(stdout);
    }

  // how should we return the path
  // should we delete any previous path????????????????????????????????????
  this->Path = path;
}




//----------------------------------------------------------------------------
// Description:
// This method calls the SmoothPath method a number of times.
void vtkClaw::SmoothPath(int number)
{
  int idx;
  
  for (idx = 0; idx < number; ++idx)
    {
    if ( ! this->SmoothPath())
      {
      printf("No more smoothing needed !!!!!!!!!!!!!!!!!!!!!!!!\n");
      return;
      }
    }
}




//----------------------------------------------------------------------------
// Description:
// This method smooths an existing path (found by GeneratePath) by making
// the map around the path more acurate.  Since the path naturally stays
// clear of collision space, the path will be smoother.  If the path breaks
// CLAW is invoked to connect the gaps.
// It returns 1 if the path was changed, 0 otherwise.
int vtkClaw::SmoothPath()
{
  SphereList *path;
  int flag = 0;
  
  // Create all the children of nodes in the path.
  path = this->Path;
  // loop through states in a path
  while (path)
    {
    if (this->SmoothSphere(path->Item))
      {
      flag = 1;
      }
    path = path->Next;
    }
  
  // Assuming that claw is initialized  properly (see GeneratePath)
  // Only use nearest network strategy (assume gaps are small)
  this->SphereSearchStrategySet(VTK_CLAW_NEAREST_NETWORK);
  path = this->PathGetValid(START_SPHERE, GOAL_SPHERE);
  while( ! path)
    {
    // Only search goal (because oll other networks are considered start)
    vtkDebugMacro(<< "Searching goal");
    path = PathGenerate(200, 1, this->ChildFraction);
    vtkDebugMacro(<< "num free = " << SpheresFreeCount()
                  << ", num collisions = " << SpheresCollisionCount());
    }

  // should we delete any previous path?????????????????????????????????
  this->Path = path;
  
  return flag;
}

//----------------------------------------------------------------------------
// Description:
// A helper method for SmoothPath.  This method makes sure a sphere 
// has no free neighbors.  It returns 1 if the sphere was modified.
int vtkClaw::SmoothSphere(Sphere *s)
{
  int axis;
  float *child = this->StateSpace->NewState();
  float temp;
  int flag = 0;
  
  if (this->Debug)
    {
    printf("Smoothing sphere:\n    ");
    SpherePrint(s);
    }

  temp = s->Radius * this->ChildFraction;
  // loop through axes (to find children)
  for (axis = 0; axis < this->DegreesOfFreedom; ++axis)
    {
    this->StateSpace->GetChildState(s->Center, axis, temp, child);
    if (this->StateSpace->Collide(child))
      {
      // Collision
      this->CollisionAdd(child, s);
      vtkDebugMacro(<< "Collision");
      // Call this function recursively (sphere now has smaller radius)
      this->SmoothSphere(s);
      delete [] child;
      return 1;
      }
    else if (this->SphereCandidateValid(s, child))
      {
      // Make a neighbor
      flag = 1;
      this->SphereNew(child, s);
      }

    this->StateSpace->GetChildState(s->Center, axis, -temp, child);
    if (this->StateSpace->Collide(child))
      {
      // Collision
      this->CollisionAdd(child, s);
      vtkDebugMacro(<< "Collision");
      // Call this function recursively (sphere now has smaller radius)
      this->SmoothSphere(s);
      delete [] child;
      return 1;
      }
    else if (this->SphereCandidateValid(s, child))
      {
      // Make a neighbor
      flag = 1;
      this->SphereNew(child, s);
      }
    }
  
  delete [] child;
  return flag;
}




//----------------------------------------------------------------------------
// for debugging
void vtkClaw::PrintFreeSpheres ()
{
  this->FreeSpheresPrint();
}


//----------------------------------------------------------------------------
// Description:
// Get a list spheres which are collisions.
// DO NOT DELETE THE LIST RETURNED.
SphereList *vtkClaw::GetCollisions()
{
  return this->Collisions;
}


//----------------------------------------------------------------------------
// Description:
// Get a list of all spheres.
// DO NOT DELETE THE LIST RETURNED.
SphereList *vtkClaw::GetSpheres()
{
  return this->FreeSpheres;
}



//----------------------------------------------------------------------------
// Description:
// Get a list spheres that defines the path.
// DO NOT DELETE THE LIST RETURNED.
SphereList *vtkClaw::GetPath()
{
  return this->Path;
}



//----------------------------------------------------------------------------
void vtkClaw::SavePath(char *fileName)
{
  SphereList *path = this->Path;
  Sphere *b;
  int idx;
  ofstream *file;
  
  file = new ofstream(fileName, ios::out);
  if (! file)
    {
    vtkDebugMacro(<< "SavePath: Could not open file " << fileName);
    return;
    }  
  
  // Print the path to the file
  while (path)
    {
    b = path->Item;
    path = path->Next;
    (*file) << b->Center[0];
    for (idx = 1; idx < this->StateDimensionality; ++idx)
      {
      (*file) << " ";
      (*file) << b->Center[idx];
      }
    (*file) << "\n";
    }
  file->close();
}


//----------------------------------------------------------------------------
void vtkClaw::LoadPath(char *fileName)
{
  SphereList *path = NULL;
  Sphere *b;
  int idx;
  FILE *file;
  float *state, temp;
  
  
  file = fopen(fileName, "r");
  if (! file)
    {
    vtkDebugMacro(<< "LoadPath: Could not open file " << fileName);
    return;
    }  
  
  // keep loading states.
  state = this->StateSpace->NewState();
  while (1)
    {
    for (idx = 0; idx < this->StateDimensionality; ++idx)
      {
      if ( ! fscanf(file, "%f", &temp))
	{
	// We should delete the old path ..
	this->Path = path;
	delete [] state;
	fclose(file);
	return;
	}
      state[idx] = temp;
      }
    // Make a sphere and add it to the path.
    b = this->SphereNew(state, NULL);
    path = this->SphereListAdd(path, b);
    }
}



//----------------------------------------------------------------------------
// Description: 
// gets the number of states in a path.
int vtkClaw::GetPathLength()
{
  SphereList *path = this->Path;
  int count = 0;
  
  while (path)
    {
    ++count;
    path = path->Next;
    }

  return count;
}


//----------------------------------------------------------------------------
// Description: 
// This method returns a path state from its index.
void vtkClaw::GetPathState(int idx, float *state)
{
  SphereList *path = this->Path;
  Sphere *b;
  
  if (path == NULL)
    {
    vtkErrorMacro(<< "GetPathSate: Path has no states");
    return;
    }
  
  // Find the correct Sphere
  while (path && idx > 0)
    {
    --idx;
    path = path->Next;
    }
  b = path->Item;

  // Copy the state
  for (idx = 0; idx < this->StateDimensionality; ++idx)
    {
    state[idx] = b->Center[idx];
    }
}











//############################################################################
// Private C functions that have not been converted to methods yet.
//############################################################################



/* determines where child should be created in sphere */
static float SPHERE_CHILD_FRACTION = 0.65;

/* For returning bestSphere from last network searched */
static int LAST_NETWORK_SEARCHED = 0;

/* tells which search strategy to use */
static int SEARCH_STRATEGY = 0;

/* The radius of the largest unknown Sphere */
static float SPHERE_MAX_RESOLUTION;

/* This flag is set when the goal network of spheres merges with another net */
/* It tells the main routine to look for a path, and recompute goal net */
static int GOAL_MERGED = 0;

static float SPHERE_MAX_RADIUS = 10000.0;








/*============================================================================
  Linked list for storing spheres.
============================================================================*/

static SphereList *SPHERELIST_HEAP = NULL;

//----------------------------------------------------------------------------
// This function frees one cell (element) of a SphereList.
void vtkClaw::SphereListElementFree(SphereList *l)
{
  l->Next = SPHERELIST_HEAP;
  SPHERELIST_HEAP = l;
}

//----------------------------------------------------------------------------
// This function frees all the elements in a list.
void vtkClaw::SphereListAllFree(SphereList *l)
{
  SphereList *temp;

  while(l){
    temp = l;
    l = l->Next;
    this->SphereListElementFree(temp);
  }
}

//----------------------------------------------------------------------------
// This function removes the first cell containing "b", from the SphereList "l".
void vtkClaw::SphereListItemRemove(SphereList **pl, void *ptr)
{
  SphereList *temp;

  while (*pl != NULL){
    if((*pl)->Item == ptr){
      temp = *pl;
      *pl = (*pl)->Next;
      this->SphereListElementFree(temp);
      return;
    } else {
      pl = &((*pl)->Next);
    }
  }
}



//----------------------------------------------------------------------------
// Returns a new list with "item" appended to the begining of "l".
SphereList *vtkClaw::SphereListAdd(SphereList *l, Sphere *item)
{
  SphereList *element;

  if(SPHERELIST_HEAP){
    element = SPHERELIST_HEAP;
    SPHERELIST_HEAP = element->Next;
  } else {
    element = (SphereList *)malloc(sizeof(struct SphereList));
  }
  if( !element){
    printf("malloc failed. (SphereListAdd)\n");
    return l;
  }

  element->Next = l;
  element->Item = item;

  return element;
}


//----------------------------------------------------------------------------
// Finds the best Sphere of a network.
Sphere *vtkClaw::SphereListNetworkBest(SphereList *list, int network)
{
  float biggest;
  Sphere *bestSphere = NULL;
  Sphere *b;
  SphereList *l;

  biggest = 0.0;
  SPHERE_MAX_RESOLUTION = 0.0;

  l = list;

  while (l){
    b = l->Item;
    l = l->Next;
    /* make sure we have an element from the right network */
    if ((network == 0 && b->Visited == 0) ||
	(network == 1 && b->Visited > 0)){
      
      /* keep track of the largest Sphere radius (for feedback) */
      if (b->Radius > SPHERE_MAX_RESOLUTION)
	SPHERE_MAX_RESOLUTION = b->Radius;

      if (!bestSphere || this->SphereSort(b) > biggest){
	bestSphere = b;
	biggest = this->SphereSort(b);
      }
    }
  }
  
  return bestSphere;
}

/*============================================================================
  End of list functions.
============================================================================*/






//----------------------------------------------------------------------------
// External.
// This function sets the starting point and goal of the planner.
int vtkClaw::SphereStartGoalInitialize(float *startState, float *goalState,
				       float radius)
{
  /* Create the first Sphere and last Sphere*/
  START_SPHERE = this->SphereNew(startState, NULL);
  if(!START_SPHERE){
    fprintf(stderr, "start position not in free space. (startGoalInit)\n");
    exit(0);
  }
  this->SphereRadiusReduce(START_SPHERE, radius);

  GOAL_SPHERE = this->SphereNew(goalState, NULL);
  if(!GOAL_SPHERE){
    fprintf(stderr, "goal position not in free space. (startGoalInit)\n");
    exit(0);
  }
  this->SphereRadiusReduce(GOAL_SPHERE, radius);
  GOAL_MERGED = 1;

  return 1;
}





//----------------------------------------------------------------------------
// External
// For debugging
void vtkClaw::FreeSpheresPrint()
{
  this->SpheresPrint(this->FreeSpheres);
}


//----------------------------------------------------------------------------
// External
// Returns the number of spheres in free space.
int vtkClaw::SpheresFreeCount()
{
  return this->SpheresCount(this->FreeSpheres);
}

//----------------------------------------------------------------------------
// External
// Returns the number of spheres in collision space.
int vtkClaw::SpheresCollisionCount()
{
  return this->SpheresCount(this->Collisions);
}






/*----------------------------------------------------------------------------
  Build and manipulate spheres
----------------------------------------------------------------------------*/


//----------------------------------------------------------------------------
// This function reduces the radius of a Sphere, and updates neighbors.
// It does not resort the this->FreeSpheres list.
void vtkClaw::SphereRadiusReduce(Sphere *b, float radius)
{
  /* Set the new radius */
  b->Radius = radius;

  this->SphereNeighborsPrune(b);
}

//----------------------------------------------------------------------------
// This function removes all neighbors that should not be in list.
// I also makes sure the Sphere is not in neighbor neighbors list.
void vtkClaw::SphereNeighborsPrune(Sphere *b)
{
  SphereList *list, *prunedList, *temp;
  Sphere *neighbor;

  prunedList = NULL;
  list = b->Neighbors;
  while (list){
    neighbor = list->Item;
    /* if this box no longer overlaps neighbor .. (avoid round off) */
    if (this->StateSpace->Distance(neighbor->Center, b->Center) >= 
	(neighbor->Radius + b->Radius) * this->NeighborFraction) {
      /* remove this neighbor from list */
      temp = list;
      list = list->Next;
      this->SphereListElementFree(temp);
      /* remove this Sphere from neighbor's neighbor list also */
      this->SphereListItemRemove(&(neighbor->Neighbors), b);
    } else {
      /* Place this neighbor on in list (Keep the neighbor) */
      temp = list;
      list = list->Next;
      temp->Next = prunedList;
      prunedList = temp;
    }
    /* Update neighbors free surface area */
    neighbor->SortValid = (char)(0);
    neighbor->SurfaceAreaValid = (char)(0);
  }
  /* replace neighbor_list with pruned list */
  b->Neighbors = prunedList;
  /* Update this spheres free surface area */
  b->SortValid = (char)(0);
  b->SurfaceAreaValid = (char)(0);
}



//----------------------------------------------------------------------------
// This function makes a Sphere structure.
Sphere *vtkClaw::SphereMake(float *center, float radius, int visited)
{
  Sphere *b;
  int idx;

  b = (Sphere *)malloc(sizeof(struct Sphere));
  if (!b)
    {
    printf("malloc failed. (SphereMake)\n");
    return NULL;
  }

  // Allocate and set the center state.
  b->Center = this->StateSpace->NewState();
  for(idx = 0; idx < this->StateDimensionality; ++idx)
    {
    b->Center[idx] = center[idx];
    }

  b->Radius = radius;
  b->Neighbors = NULL;
  b->Sort = 0.0;
  b->Visited = visited;
  b->Nearest = NULL;
  b->SortValid = (char)(0);
  b->SurfaceAreaValid = (char)(0);
  b->SurfaceArea = 0.0;

  return b;
}




//----------------------------------------------------------------------------
// This function takes a position (not in free space), and shrinks any
// spheres if necessary. (only neighbors of parent need to be checked).
void vtkClaw::CollisionAdd(float *state, Sphere *parent)
{
  Sphere *b;
  
  b = this->SphereMake(state, 0.0, -1);
  this->SphereCollisionAdd(b, parent);
}


//----------------------------------------------------------------------------
// This function takes a position (not in free space), and shrinks any
// spheres if necessary. (only neighbors of parent need to be checked).
void vtkClaw::SphereCollisionAdd(Sphere *b, Sphere *parent)
{
  SphereList *l;
  float temp;

  this->Collisions = this->SphereListAdd(this->Collisions, b);
  b->Visited = -1;

  if ( parent){
    /* We are garenteed parent is the only sphere which contains collision */
    temp = this->StateSpace->Distance(parent->Center, b->Center);
    /* Reduce the spheres radius */
    this->SphereRadiusReduce(parent, temp);
    /* Add collision neighbor */
    b->Neighbors = this->SphereListAdd(b->Neighbors, parent);
  } else {
    /* This collision must be from verifiing a path. Check all spheres */
    l = this->FreeSpheres;
    while (l){
    temp = this->StateSpace->Distance(l->Item->Center, b->Center);
    if (temp < l->Item->Radius){
	/* Reduce the spheres radius */
	this->SphereRadiusReduce(l->Item, temp);
	/* Add collision neighbor */
	b->Neighbors = this->SphereListAdd(b->Neighbors, l->Item);
      }
      l = l->Next;
    }
  }
}


//----------------------------------------------------------------------------
// This function frees a Sphere
// It does not bother removing itself as a neighbor of other spheres.
void vtkClaw::SphereFree(Sphere *b)
{
  this->SphereListAllFree(b->Neighbors);
  free(b);
}


//----------------------------------------------------------------------------
// EXTERNAL
// This function removes all spheres (to start the algorithm over)
void vtkClaw::SphereAllFree()
{
  SphereList *l1;

  /* free the collision spheres */  
  l1 = this->Collisions;
  while (l1){
    this->SphereFree(l1->Item);
    l1 = l1->Next;
  }
  this->SphereListAllFree(this->Collisions);
  this->Collisions = NULL;


  /* free the free spheres */  
  l1 = this->FreeSpheres;
  while (l1){
    this->SphereFree(l1->Item);
    l1 = l1->Next;
  }
  this->SphereListAllFree(this->FreeSpheres);
  this->FreeSpheres = NULL;
}
	
	


//----------------------------------------------------------------------------
// This function removes collisions that do not have neighbors.
void vtkClaw::SphereCollisionsPrune()
{
  SphereList *newCollisions = NULL;
  SphereList *newNeighbors;
  SphereList *l1, *l2, *temp;
  
  l1 = this->Collisions;
  while (l1){
    newNeighbors = NULL;
    l2 = l1->Item->Neighbors;
    while (l2){
      if (this->StateSpace->Distance(l1->Item->Center, l2->Item->Center) <=
	  l2->Item->Radius + 0.001){
	/* keep: move to new neighbors list */
	temp = l2->Next;
	l2->Next = newNeighbors;
	newNeighbors = l2;
	l2 = temp;
      } else {
	/* dispose of this neighbor */
	temp = l2->Next;
	this->SphereListElementFree(l2);
	l2 = temp;
      }
    }
    l1->Item->Neighbors = newNeighbors;

    if (newNeighbors){
      /* keep: move to new collisions list */
      temp = l1->Next;
      l1->Next = newCollisions;
      newCollisions = l1;
      l1 = temp;
    } else {
      /* dispose of this Sphere */
      temp = l1->Next;
      free(l1->Item);
      this->SphereListElementFree(l1);
      l1 = temp;
    }
  }

  this->Collisions = newCollisions;
}
	
	


//----------------------------------------------------------------------------
// This function recomputes a spheres nearest if necessary.
// It returns the nearest_val.
float vtkClaw::SphereNearestVal(Sphere *b)
{
  /* ignore collision nodes */
  if (b->Visited < 0)
    return 10000;

  /* make sure nearest is up to date */
  if ( ! this->SphereNearest(b))
    return 10000;  /* could not find nearest */

  // return the distance between sphere surfaces
  // How much closer before they are neighbors.
  return (b->NearestVal - 
	  (b->Radius + b->Nearest->Radius) * this->NeighborFraction);
}


//----------------------------------------------------------------------------
// This function recomputes a spheres nearest if necessary.
// It returns the nearest.
Sphere *vtkClaw::SphereNearest(Sphere *b)
{
  /* ignore collision nodes */
  if (b->Visited < 0)
    return NULL;

  if (b->Nearest)
    if ((((b->Visited > 0) && (b->Nearest->Visited == 0)) || 
	 ((b->Visited == 0) && (b->Nearest->Visited > 0))))
      return b->Nearest;

  b->Nearest = NULL;
  b->NearestVal = 1;
  {
    SphereList *l;
    Sphere *other;
    float temp;

    /* compute nearest */
    l = this->FreeSpheres;
    while (l){
      other = l->Item;
      l = l->Next;

      if (((b->Visited > 0) && (other->Visited == 0)) || 
	  ((b->Visited == 0) && (other->Visited > 0))){
	temp = this->StateSpace->Distance(b->Center, other->Center);
	if (!(b->Nearest) || temp < b->NearestVal){
	  b->Nearest = other;
	  b->NearestVal = temp;
	  b->SortValid = (char)(0);
	}
	if (other->Nearest && temp < other->NearestVal){
	  other->Nearest = b;
	  other->NearestVal = temp;
	  other->SortValid = (char)(0);
	  other->SortValid = (char)(0);
	}
      }
    }
  }

  return b->Nearest;
}



//----------------------------------------------------------------------------
// This function tries to makes a new Sphere from a center and radius.
// If the center is in free space the Sphere is created.  Either way,
// the entire network is updated with the new information.
// The function returns the Sphere if one was created, NULL otherwise.
Sphere *vtkClaw::SphereNew(float *center, Sphere *parent)
{
  Sphere *b;
  
  if (this->StateSpace->Collide(center)){
    this->CollisionAdd(center, parent);
    return NULL;
  }

  if (parent)
    b = this->SphereMake(center, SPHERE_MAX_RADIUS, parent->Visited);
  else
    b = this->SphereMake(center, SPHERE_MAX_RADIUS, 0);

  
  //if (this->Debug)
  //  {
  //  printf("New Sphere:\n   ");
  //  this->SpherePrint(b);
  //  }
  
  return this->SphereAdd(b);
}


//----------------------------------------------------------------------------
// This function makes a sphere from a free state, but defers adding the
// sphere until later.  It is needed because new spheres need to be added
// in the process of adding another sphere.
void vtkClaw::NewDeferredSphere(float *center)
{
  Sphere *b;
  
  b = this->SphereMake(center, SPHERE_MAX_RADIUS, 0);
  this->DeferredSpheres = this->SphereListAdd(this->DeferredSpheres, b);
}


//----------------------------------------------------------------------------
// This method adds spheres from the deferred list.
// Note: New spheres may be added to the deferred list in the middle
// of this method.
void vtkClaw::AddDeferredSpheres()
{
  SphereList *temp;
  
  while (this->DeferredSpheres)
    {
    temp = this->DeferredSpheres;
    this->DeferredSpheres = temp->Next;
    this->SphereAdd(temp->Item);
    this->SphereListElementFree(temp);
    }
}




//----------------------------------------------------------------------------
// This function adds another Sphere to the space.
// It updates all links. (should in shink the new Sphere if necessary?)
// It calculates the "Unknown" surface area of the box, and adds
// it the the list "SPHERES".
Sphere *vtkClaw::SphereAdd(Sphere *b)
{
  SphereList *l;
  Sphere *other;
  float temp;
  //vtkImageRobotSpace2D *rSpace; // ...

  //rSpace = (vtkImageRobotSpace2D *)(this->StateSpace); // ...
  //if (b->Visited)
  //  {
  //  rSpace->GetCanvas()->SetDrawValue(0);
  //  }
  //else
  //  {
  //  rSpace->GetCanvas()->SetDrawValue(50);
  //  }
  
    
  //((vtkImageRobotSpace2D *)(this->StateSpace))->DrawRobot(b->Center); // ...

  /* Find the closest collision to determine radius */
  other = NULL;
  l = this->Collisions;
  while(l){
    temp = this->StateSpace->Distance(b->Center, l->Item->Center);
    if (temp < b->Radius){
      b->Radius = temp;
      other = l->Item;
    }
    l = l->Next;
  }

  /* the closest collision */
  if(other){
    other->Neighbors = this->SphereListAdd(other->Neighbors, b);
  }

  // Determine which spheres are neighbors.
  l = this->FreeSpheres;
  while (l){
    other = l->Item;
    l = l->Next;
    temp = this->StateSpace->Distance(b->Center, other->Center);

    /* compute nearest as a side action */
    if (((b->Visited && !(other->Visited)) || 
	 (!(b->Visited) && other->Visited))){
      if (!(b->Nearest) || temp < b->NearestVal){
	b->Nearest = other;
	b->NearestVal = temp;
	b->SortValid = (char)(0);
      }
      if (other->Nearest && temp < other->NearestVal){
	other->Nearest = b;
	other->NearestVal = temp;
	other->SortValid = (char)(0);
      }
    }

    /* if these two spheres are touching  make them neighbors */
    if (temp <= ((b->Radius + other->Radius) * this->NeighborFraction)) {
      /* update the neighbor lists */
      b->Neighbors = this->SphereListAdd(b->Neighbors, other);
      other->Neighbors = this->SphereListAdd(other->Neighbors, b);
      /* Update neighbors free surface area */
      other->SortValid = (char)(0);
      other->SurfaceAreaValid = (char)(0);
    }
  }

  // Add the new sphere to the list
  this->FreeSpheres = this->SphereListAdd(this->FreeSpheres, b);

  // Update this spheres free surface area
  b->SortValid = (char)(0);
  b->SurfaceAreaValid = (char)(0);

  // We may have found a path if the networks are merged.
  this->CheckForMergedNetworks(b);

  return b;
}

//----------------------------------------------------------------------------
void vtkClaw::CheckForMergedNetworks(Sphere *s)
{
  SphereList *l;
  Sphere *other;
  
  
  if (GOAL_MERGED)
    {
    return;
    }
  
  l = s->Neighbors;
  while (l)
    {
    other = l->Item;
    l = l->Next;
    
    /* Check for merged goal net */
    if ((s->Visited && !other->Visited) || (!s->Visited && other->Visited))
      {  // Start and goal spheres are touching. Verify link.
      if (this->SphereLinkVerify(s, other))
	{
	GOAL_MERGED = 1;
	return;
	}
      else
	{ // A collision must have been added (neighbors may have changed)
	this->CheckForMergedNetworks(s);
	return;
	}
      }
    }
}

    
    
//----------------------------------------------------------------------------
void vtkClaw::SpherePrint(Sphere *b)
{
  int idx;

  if(!b){
    printf("NULL\n");
    return;
  }

  printf("space DrawRobot %.4f", b->Center[0]);
  for (idx = 1; idx < this->StateDimensionality; ++idx)
    printf(" %.4f", b->Center[idx]);
  printf("; # r: %.3f, sort: %.6f, net %d, num %.1f,%d, dist %.1f, near %.3f,%.3f",
	 b->Radius, this->SphereSort(b), b->Visited, 
	 this->SphereSurfaceArea(b), this->SphereNumNeighbors(b),
	 this->StateSpace->BoundsTest(b->Center), this->SphereNearestVal(b), 
	 b->NearestVal);

  b = this->SphereNearest(b);
  if (b)
    printf(", %.3f\n", this->SphereNearestVal(b));
  else
    printf("\n");

  fflush(stdout);
}

      


//----------------------------------------------------------------------------
int vtkClaw::SphereNumNeighbors(Sphere *b)
{
  SphereList *l;
  int count = 0;

  l = b->Neighbors;
  while (l){
    l = l->Next;
    ++count;
  }

  return count;
}



      
      
      
//----------------------------------------------------------------------------
// This function returns 1 if a candidate child for a sphere is not
// Already mapped. It returns 0 otherwise.
int vtkClaw::SphereCandidateValid(Sphere *b, float *proposed)
{
  float temp = b->Radius * SPHERE_CHILD_FRACTION;
  SphereList *l;
  Sphere *other;

  /* handle if the sphere touches itself */
  if (this->StateSpace->Distance(b->Center, proposed) < (temp * 0.9))
    {
    return 0;
    }

  /* Remove if a neighbor contains this point */
  l = b->Neighbors;
  while (l)
    {
    other = l->Item;
    l = l->Next;
    if (this->StateSpace->Distance(other->Center, proposed) < other->Radius)
      {
      return 0;
      }
    }
  
  return 1;
}
  

//----------------------------------------------------------------------------
// This function returns (pointer argument) an array which tells which 
// search directions of a Sphere are already filled (and which are free).
// The integer return value is the number of candidates.
int vtkClaw::SphereCandidatesGet(Sphere *b)
{
  float *proposed = this->StateSpace->NewState();
  int numCandidates, valid;
  int direction, axis;
  float temp;

  numCandidates = 0;
  
  /* Find the number of unoccupied inbounds candidates */
  temp = b->Radius * SPHERE_CHILD_FRACTION;
  for (axis = 0; axis < this->DegreesOfFreedom; ++axis){
    for (direction = 0; direction < 2; ++direction){
      /* Set up child state */
      if (direction)
	{
	this->StateSpace->GetChildState(b->Center, axis, temp, proposed);
	}
      else
	{
	this->StateSpace->GetChildState(b->Center, axis, -temp, proposed);
	}

      valid = this->SphereCandidateValid(b, proposed);

      numCandidates += valid;
      this->Candidates[axis * 2 + direction] = valid;
    }
  }
  
  delete [] proposed;
  return numCandidates;
}



//----------------------------------------------------------------------------
// This function returns (pointer argument) an array which tells which
// search direstions of a Sphere are already filled (and which are free). 
// The integer return value is the number of candidates.
float vtkClaw::SphereSurfaceArea(Sphere *b)
{
  if ( ! b->SurfaceAreaValid){
    b->SurfaceArea = (float)(SphereCandidatesGet(b));
    /* consider the guide tube at this point */
    b->SurfaceArea *= this->StateSpace->BoundsTest(b->Center);
    b->SurfaceAreaValid = (char)(1);
  }

  return b->SurfaceArea;
}


/*----------------------------------------------------------------------------
  End of Sphere stuff
----------------------------------------------------------------------------*/














/*============================================================================
  Actually create the spheres to fill the free space.
============================================================================*/






//----------------------------------------------------------------------------
void vtkClaw::SpheresPrint(SphereList *spheres)
{
  SphereList *list;

  printf("Sphere list:\n");
  list = spheres;
  while (list){
    this->SpherePrint(list->Item);
    list = list->Next;
  }
  printf("\n");
}



//----------------------------------------------------------------------------
// Returns the length of the list.
int vtkClaw::SpheresCount(SphereList *spheres)
{
  SphereList *list;
  int count = 0;

  list = spheres;
  while (list){
    ++count;
    list = list->Next;
  }
  return count;
}


/*----------------------------------------------------------------------------
                 Search strategy stuff.
----------------------------------------------------------------------------*/



//----------------------------------------------------------------------------
// External
// Changes the search strategy.
void vtkClaw::SphereSearchStrategySet(int strategy)
{
  SphereList *l;
  Sphere *b;

  SEARCH_STRATEGY = strategy;
  printf("\nchanging search strategy to: %d\n", strategy);

  /* recompute all the sort_values */
  l = this->FreeSpheres;
  while (l){
    b = l->Item;
    l = l->Next;
    b->SortValid = (char)(0);
  }
}
    


//----------------------------------------------------------------------------
// This function chooses the next position to spawn a child.
// It uses the search strategy to select a huristic.
// Returns the chosen position in proposed.
void vtkClaw::SphereCandidateChoose(Sphere *b, float *proposed)
{
  int direction, axis;
  float best, temp, first = 1;
  int bestDirection, bestAxis;

  best = 0.0;
  bestDirection = 0;
  bestAxis = 0;
  // determined which candidates are covered.
  this->SphereCandidatesGet(b);

  // Loop over all the candidates.
  for (axis = 0; axis < this->DegreesOfFreedom; ++axis)
    {
    for (direction = 0; direction < 2; ++direction)
      {
      // set up the candidate state
      temp = b->Radius * SPHERE_CHILD_FRACTION;
      if (direction)
	{
	this->StateSpace->GetChildState(b->Center, axis, temp, proposed);
	}
      else
	{
	this->StateSpace->GetChildState(b->Center, axis, -temp, proposed);
	}
      /* get the rating of this position from the searchStrategy */
      if (SEARCH_STRATEGY == 0)
	temp = this->SphereNearestNetworkMoveEvaluate(b, axis, direction);
      if (SEARCH_STRATEGY == 1)
	temp = this->SphereNearestNetworkMoveEvaluate(b, axis, direction);
      if (SEARCH_STRATEGY == 2)
	temp = this->SphereNearestGlobalMoveEvaluate(b, proposed);
      if (SEARCH_STRATEGY == 3)
	temp = this->SpherePioneerLocalMoveEvaluate(b, proposed);
      if (SEARCH_STRATEGY == 4)
	temp = this->SpherePioneerGlobalMoveEvaluate(b, proposed);
      if (SEARCH_STRATEGY == 5)
	temp = this->SphereNearestNetworkMoveEvaluate(b, axis, direction);
      if (SEARCH_STRATEGY == 6)
	temp = this->SphereNoiseMoveEvaluate();
      if (SEARCH_STRATEGY == 7)
	temp = this->SphereNearestNoiseMoveEvaluate(b, proposed);
      if (SEARCH_STRATEGY == 8)
	temp = this->SphereNearestNetworkMoveEvaluate(b, axis, direction);
      if (SEARCH_STRATEGY == 9)
	temp = this->SphereNoiseMoveEvaluate();
      if (SEARCH_STRATEGY == 10)
	temp = this->SphereNoiseMoveEvaluate();

      // We want uncovered first, but if all are covered, rank the covered.
      if (this->Candidates[axis * 2 + direction] == 0)
	{
	temp *= 0.0001;
	}
      
      // Should we consider State Bounds?
      // Are they taken care of in "MoveEvaluate" or
      // only in selection of the parent sphere?

      // if this beats the best so far, (or is the first) save it
      if (first || temp > best)
	{
	first = 0;
	best = temp;
	bestAxis = axis;
	bestDirection = direction;
	}
      
      }
    }
  
  // set up the best proposed position
  temp = b->Radius * SPHERE_CHILD_FRACTION;
  if (bestDirection)
    {
    this->StateSpace->GetChildState(b->Center, bestAxis, temp, proposed);
    }
  else
    {
    this->StateSpace->GetChildState(b->Center, bestAxis, -temp, proposed);
    }
}


//----------------------------------------------------------------------------
// Get as close to the other network as possible (the nearest)
// larger return value is better.
float vtkClaw::SphereNearestNetworkMoveEvaluate(Sphere *b, int axis, 
						int direction)
{
  Sphere *nearest;
  
  nearest = this->SphereNearest(b);

  if ( ! nearest)
    return 0.0;

  // Note: This should be in the state space.
  // the vector representation does not necessarily preseve distance.
  if (direction)
    return nearest->Center[axis] - b->Center[axis];
  else
    return b->Center[axis] - nearest->Center[axis];
}
  

//----------------------------------------------------------------------------
// Move toward the ultimate goal, the other networks root.
// larger return value is better.
float vtkClaw::SphereNearestGlobalMoveEvaluate(Sphere *b, float *proposed)
{
  float temp;

  if (b->Visited){
    /* this Sphere *is in the goal network */
    if (! START_SPHERE)
      return 0.0;
    temp = this->StateSpace->Distance(proposed, START_SPHERE->Center);
  } else {
    /* this Sphere is in the start network */
    if (! GOAL_SPHERE)
      return 0.0;
    temp = this->StateSpace->Distance(proposed, GOAL_SPHERE->Center);
  }

  return 1 / temp;
}


//----------------------------------------------------------------------------
// Move away from neighbors.
// larger return value is better.
float vtkClaw::SpherePioneerLocalMoveEvaluate(Sphere *b, float *proposed)
{
  SphereList *l;
  float temp = 0.0;

  l = b->Neighbors;
  while (l){
    temp += this->StateSpace->Distance(proposed, l->Item->Center);
    l = l->Next;
  }

  return temp;
}


//----------------------------------------------------------------------------
// Move away from this spheres root.
// larger return value is better.
float vtkClaw::SpherePioneerGlobalMoveEvaluate(Sphere *b, float *proposed)
{
  float temp;

  if (b->Visited){
    /* this Sphere is in the goal network */
    if (! GOAL_SPHERE)
      return 0.0;
    temp = this->StateSpace->Distance(proposed, GOAL_SPHERE->Center);
  } else {
    /* this Sphere *is in the start network */
    if (! START_SPHERE)
      return 0.0;
    temp = this->StateSpace->Distance(proposed, START_SPHERE->Center);
  }

  return temp;
}
  

//----------------------------------------------------------------------------
// Adds noise to the search. Just returns a randorm number.
float vtkClaw::SphereNoiseMoveEvaluate()
{
  return  (float)(rand()) / 2147483648.0;
}
  



//----------------------------------------------------------------------------
// Selects randomly among directions which move the Sphere closer
// to the nearset network
float vtkClaw::SphereNearestNoiseMoveEvaluate(Sphere *b, float *proposed)
{
  Sphere *nearest;

  nearest = this->SphereNearest(b);
  if (this->StateSpace->Distance(proposed, nearest->Center) < b->NearestVal)
    return  (float)(rand()) / 2147483648.0;
  else
    return 0.0;
}
  





//----------------------------------------------------------------------------
// A function to compute the free surface area of a box.
// A variation of the above. (neighbor must include proposed point).
float vtkClaw::SphereSort(Sphere *b)
{
  /* It is possible that this value could change with out setting valid to 0 */
  /* (for nearestMinimum)  (if nearest changes its nearest_val) */
  /* this small inconsistancey should not make much difference */
  if (b->SortValid)
    return b->Sort;

    
  if (SEARCH_STRATEGY == 0)
    b->Sort = this->SphereNearestNetworkSortCompute(b);
  if (SEARCH_STRATEGY == 1)
    b->Sort = this->SphereNearestMinimumSortCompute(b);
  if (SEARCH_STRATEGY == 2)
    b->Sort = this->SphereNearestGlobalSortCompute(b);
  if (SEARCH_STRATEGY == 3)
    b->Sort = this->SpherePioneerLocalSortCompute(b);
  if (SEARCH_STRATEGY == 4)
    b->Sort = this->SpherePioneerGlobalSortCompute(b);
  if (SEARCH_STRATEGY == 6)
    b->Sort = this->SphereMinimumWellSortCompute(b);
  if (SEARCH_STRATEGY == 7)
    b->Sort = this->SphereMinimumWellSortCompute(b);
  if (SEARCH_STRATEGY == 8)
    b->Sort = this->SphereMinimumWellSortCompute(b);
  if (SEARCH_STRATEGY == 9)
    b->Sort = this->SphereCloseToleranceSortCompute(b);
  if (SEARCH_STRATEGY == 10)
    b->Sort = this->SphereNarrowWellSortCompute(b);
  b->SortValid = (char)(1);
  return b->Sort;
}
  



//----------------------------------------------------------------------------
// 1/StateSpace->Distance(nearestNeighbor);
// larger is better.
float vtkClaw::SphereNearestNetworkSortCompute(Sphere *b)
{
  float temp = this->SphereNearestVal(b);

  /* hack to cover up intermitent bug */
  if (temp <= 0.00001)
    temp = 0.00001;

  return b->Radius * (this->SphereSurfaceArea(b) + 0.0001) / temp;
}


//----------------------------------------------------------------------------
// Search LOCAL minimum.
// larger is better.
float vtkClaw::SphereNearestMinimumSortCompute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = this->SphereNearest(b);
  if ( ! nearest)
    return 0.00001;

  temp = this->SphereNearestVal(b) - this->SphereNearestVal(nearest);

  if (temp <= 0.00001)
    temp = 0.00001;

  /* what value should I use (0.5, 1.0, some measure of STD) */
  return b->Radius * (this->SphereSurfaceArea(b) + 0.0001) 
    / (0.5 + temp);
}


//----------------------------------------------------------------------------
// 1 / (StateSpace->Distance(GOAL))
// larger is better.
float vtkClaw::SphereNearestGlobalSortCompute(Sphere *b)
{
  if (b->Visited > 0){
    /* this Sphere is part of the goal netework */
    if ( ! START_SPHERE)
      return 0.00001;
    return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) 
      / this->StateSpace->Distance(b->Center, START_SPHERE->Center);
  } else {
    /* this Sphere *is part of the start netework */
    if ( ! GOAL_SPHERE)
      return 0.00001;
    return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) 
      / this->StateSpace->Distance(b->Center, GOAL_SPHERE->Center);
  }
}


//----------------------------------------------------------------------------
// 1 / (length(neighbors));
// larger is better.
float vtkClaw::SpherePioneerLocalSortCompute(Sphere *b)
{
  return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) 
    / (0.01 + (float)(this->SphereNumNeighbors(b)));
}


//----------------------------------------------------------------------------
// StateSpace->Distance(START).
// larger is better.
float vtkClaw::SpherePioneerGlobalSortCompute(Sphere *b)
{
  if (b->Visited > 0){
    /* this Sphere is part of the goal netework */
    if ( ! GOAL_SPHERE)
      return 0.00001;
    return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) 
      * this->StateSpace->Distance(b->Center, GOAL_SPHERE->Center);
  } else {
    /* this Sphere is part of the start netework */
    if ( ! START_SPHERE)
      return 0.00001;
    return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) 
      * this->StateSpace->Distance(b->Center, START_SPHERE->Center);
  }
}



//----------------------------------------------------------------------------
// focus on the local minimum
// larger is better.
// problem with this: shhould never return 0 (sometimes all are 0)
float vtkClaw::SphereMinimumWellSortCompute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = this->SphereNearest(b);
  if ( ! nearest)
    return 0.00001;

  temp = (ROBOT_RADIUS * 0.02)
    - (this->SphereNearestVal(b) - this->SphereNearestVal(nearest));

  if (temp <= 0.00001)
    temp = 0.00001; /* hack */

  return b->Radius * (this->SphereSurfaceArea(b) + 0.0000001) * temp;
}

//----------------------------------------------------------------------------
// same as nearest minimum but much more focused search of minimum.
// larger is better.
float vtkClaw::SphereCloseToleranceSortCompute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = this->SphereNearest(b);
  if ( ! nearest)
    return 0.0;

  temp = (ROBOT_RADIUS * 0.01)
    - (this->SphereNearestVal(b) - this->SphereNearestVal(nearest));

  if (temp <= 0.00001)
    temp = 0.00001; /* hack */

  return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) * temp;
}


//----------------------------------------------------------------------------
// same as close tolerence, except narrow minimumare prefered to wide.
// (ignore radius => narrow has lots with small radius and is prefered)
float vtkClaw::SphereNarrowWellSortCompute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = this->SphereNearest(b);
  if ( ! nearest)
    return 0.0;
  this->SphereNearest(nearest);

  temp = (ROBOT_RADIUS * 0.02) - (b->NearestVal - nearest->NearestVal);

  if (temp <= 0.00001)
    temp = 0.00001; /* hack */

  return b->Radius * (this->SphereSurfaceArea(b) + 0.00001) * temp;
}





/*----------------------------------------------------------------------------
               End of search strategy stuff.
----------------------------------------------------------------------------*/














/*----------------------------------------------------------------------------
  Routines for searching the Sphere space for a path, and verifiing the path.
----------------------------------------------------------------------------*/





//----------------------------------------------------------------------------
// External
// This function fills the free space with spheres until a path is found,
// or until "additionalSpheres" additional spheres are created.
// It returns the verified path or NULL.
SphereList *vtkClaw::PathGenerate(int additionalSpheres, 
				  int network, float childFraction)
{
  Sphere *b;
  float *center = this->StateSpace->NewState();
  SphereList *path = NULL;

  
  SPHERE_CHILD_FRACTION = childFraction;
  LAST_NETWORK_SEARCHED = network;

  if(!START_SPHERE || !GOAL_SPHERE)
    {
    vtkErrorMacro(<< "start and goal position must be initialized "
        << "(PathGenerate)");
    delete [] center;
    return NULL;
    }
  
  while (!path && additionalSpheres > 0)
    { // if the newest Sphere contains goal, find path and try to verify 
    if (GOAL_MERGED)
      {
      path = this->PathGetValid(START_SPHERE, GOAL_SPHERE);
      GOAL_MERGED = 0;
      } 
    else 
      {
      --additionalSpheres;
      // Pick the biggest Sphere in unknown space 
      b = SphereListNetworkBest(this->FreeSpheres, network);
      // Pick a direction to extend the space 
      SphereCandidateChoose(b, center);
      // Create a new Sphere if new position is valid 
      b = this->SphereNew(center, b);
      // Flush the sphere buffer (waiting line).
      this->AddDeferredSpheres();
      }
    }
  
  delete [] center;
  return path;
}



//----------------------------------------------------------------------------
// This function is called when the goal and the start spaces merge.  
// The space is searched until a valid path is found, 
// or until the space is split into two sections again.
SphereList *vtkClaw::PathGetValid(Sphere *startSphere, Sphere *goalSphere)
{
  SphereList *path;

  vtkDebugMacro(<< "-----pathGet_valid");
  while ( (path = this->PathSearch(startSphere, goalSphere)) ){
    if (this->PathVerify(path)){
      printf("-----path validated\n");
      return path;
    }

    /* Free the current path */
    SphereListAllFree(path);
    printf("     This path no good. Oh well, try again\n");
  }
  
  /* No more paths found */
  printf("-----No more paths to try.\n");

  return NULL;
}


//----------------------------------------------------------------------------
// This function verifies a path by verifying each link.
int vtkClaw::PathVerify(SphereList *path)
{
  SphereList *ll;

  ll = path;
  while (ll && ll->Next){
    /* if this link is not valid, then return */
    if (! this->SphereLinkVerify(ll->Item, ll->Next->Item))
      return 0;

    ll = ll->Next;
  }

  /* all links are OK */
  return 1;
}

/*----------------------------------------------------------------------------
  Verifying links with memory. (does not verify a link more than once)
----------------------------------------------------------------------------*/

/* Global variable which is only used in this section */
static SphereList *VERIFIED_LINKS = NULL;
//----------------------------------------------------------------------------
// This function checks to see if a link has already been verified.
int vtkClaw::SphereLinkVerifiedAlready(Sphere *b0, Sphere *b1)
{
  SphereList *l;
  
  l = VERIFIED_LINKS;
  while (l){
    if (l->Item == b0 && l->Next->Item == b1)
      return 1;
    if (l->Item == b1 && l->Next->Item == b0)
      return 1;
    l = l->Next->Next;
  }

  return 0;
}

//----------------------------------------------------------------------------
// This function records that a link has been verified.
void vtkClaw::SphereLinkVerifiedRecord(Sphere *b0, Sphere *b1)
{
  VERIFIED_LINKS = SphereListAdd(VERIFIED_LINKS, b0);
  VERIFIED_LINKS = SphereListAdd(VERIFIED_LINKS, b1);
}

//----------------------------------------------------------------------------
// EXTERNAL
// Starts from scratch. All previous verifications are cleared.
void vtkClaw::SphereVerifiedLinksClear()
{
  SphereListAllFree(VERIFIED_LINKS);
  VERIFIED_LINKS = NULL;
}


//----------------------------------------------------------------------------
// This recursive function checks to se if two states are linked.
int vtkClaw::SphereLinkStates(float *s1, float *s2, float distance)
{
  // If distance between two states is below specified resolution,
  // confirm and return.
  if (distance <= this->VerifyStep)
    {
    distance = this->StateSpace->Distance(s1, s2);
    if (distance > this->VerifyStep)
      {
      return this->SphereLinkStates(s1, s2, distance);
      }
    else
      {
      return 1;
      }
    }
  else
    {
    float *middle = this->StateSpace->NewState();
    // Split the link in two by adding mid point state.
    this->StateSpace->GetMiddleState(s1, s2, middle);
    if (this->StateSpace->Collide(middle))
      {
      this->CollisionAdd(middle, NULL);
      delete [] middle;
      return 0;
      }
    else
      {
      // Recursion: Call this function on the two links
      if (this->SphereLinkStates(s1, middle, distance/2.0) &&
	  this->SphereLinkStates(middle, s2, distance/2.0))
	{ // link is OK
	delete [] middle;
	return 1;
	}
      else
	{ // Link is No good, but save middle as free sphere
	this->NewDeferredSphere(middle);
	delete [] middle;
	return 0;
	}
      }
    }
}

    
	 
      
	
  
  



//----------------------------------------------------------------------------
// This function verifies that two spheres are actually connected by taking
// small steps from one two the other. (does not recheck the actual nodes)
int vtkClaw::SphereLinkVerify(Sphere *b0, Sphere *b1)
{
  float distance;
  static Sphere *s0 = NULL;
  static Sphere *s1 = NULL;
  
  // first see if we have already verified this link
  if (this->SphereLinkVerifiedAlready(b0, b1))
    {
    return 1;
    }

  
  // I am have trouble with invalid links being test over and over again.
  if (s0 == b0 && s1 == b1)
    {
    vtkErrorMacro(<< "SphereLinkVerify: We already tried this link!");
    }
  s0 = b0;
  s1 = b1;
  
  distance = this->StateSpace->Distance(b0->Center, b1->Center);
  if ( ! this->SphereLinkStates(b0->Center, b1->Center, distance))
    {
    vtkDebugMacro(<< "SphereLinkVerify: Not Verified " << b0 << ", " << b1);
    return 0;
    }
  
  vtkDebugMacro(<< "SphereLinkVerify: Verified " << b0 << ", " << b1);

  // save the fact that this link is valid 
  this->SphereLinkVerifiedRecord(b0, b1);

  return 1;
}


/*----------------------------------------------------------------------------
  End Verifying links.
----------------------------------------------------------------------------*/



//----------------------------------------------------------------------------
// This function performs a breadth first search of the Sphere network
// to find the shortest path between two spheres.  
// Returns the path on success, NULL otherwise.
// The visited value of the spheres ends up being the distance+1 to goal.
SphereList *vtkClaw::PathSearch(Sphere *start, Sphere *end)
{
  SphereList *leaves, *lastLeaf, *ll;
  Sphere *b;

  /* Set all the spheres to not visited */
  ll = this->FreeSpheres;
  while(ll){
    ll->Item->Visited = 0;
    ll = ll->Next;
  }

  /* Depth first search from goal */
  leaves = SphereListAdd(NULL, end);
  end->Visited = 1;
  lastLeaf = leaves;
  while (leaves){
    /* first Sphere on the list */
    b = leaves->Item;

    /* Add neighbors to end of leaf list */
    ll = b->Neighbors;
    while (ll){
      if ( ! ll->Item->Visited){
	ll->Item->Visited = b->Visited + 1;
	/* Add new Sphere to end of leaves list */
	lastLeaf->Next = SphereListAdd(NULL, ll->Item);
	lastLeaf = lastLeaf->Next;
	/* If we have reached the start Sphere, the a path has been found */
	if(ll->Item == start){
	  SphereListAllFree(leaves);
	  return this->PathUnravel(start);
	}
      }
      ll = ll->Next;
    }
    /* remove the first list element */
    ll = leaves;
    leaves = leaves->Next;
    SphereListElementFree(ll);
  }

  /* No paths exist */
  return NULL;
}
  


//----------------------------------------------------------------------------
// This function generates a path from the start Sphere, and distance to goal
// infomation stored in the visited slots.
SphereList *vtkClaw::PathUnravel(Sphere *start)
{
  SphereList *path, *last, *ll;
  Sphere *b;

  path = last = SphereListAdd(NULL, start);
  b = start;
  while(b->Visited > 1){
    /* find the neighbor closest to goal */
    ll = b->Neighbors;
    while(ll){
      /* Avoid collision spheres (visited = -1) */
      if(ll->Item->Visited > 0 && ll->Item->Visited < b->Visited)
	b = ll->Item;
      ll = ll->Next;
    }
    /* add this neighbor to the end of the path */
    last->Next = SphereListAdd(NULL, b);
    last = last->Next;
  }

  return path;
}


/*----------------------------------------------------------------------------
  End of path routines 
----------------------------------------------------------------------------*/


























