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
// a function to wrap a state to remove any duplicate states.


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
  int idx;
  
  for (idx = 0; idx < VTK_CLAW_DIMENSIONS; ++idx)
    {
    this->StartState[idx] = this->GoalState[idx] = 0.0;
    }
  this->InitialSphereRadius = 2.0;
  this->VerifyStep = 0.1;
  this->SamplePeriod = 200;
  this->GoalPercentage = 20.0;
  this->ChildFraction = 0.75;
  this->Path = NULL;
  this->StateSpace = NULL;

  this->SearchStrategies[0] = VTK_CLAW_NEAREST_NETWORK;
  this->SearchStrategies[1] = VTK_CLAW_PIONEER_LOCAL;
  this->SearchStrategies[2] = VTK_CLAW_WELL_NOISE;
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
  this->StateSpace = space;
  this->Modified();
}


//----------------------------------------------------------------------------
// This method sets the starting position (transform)
void vtkClaw::SetStartState(float *state)
{
  int idx;
  
  for (idx = 0; idx < VTK_CLAW_DIMENSIONS; ++idx)
    {
    this->StartState[idx] = state[idx];
    }
  this->Modified();
}


//----------------------------------------------------------------------------
// This method sets the goal position (transform)
void vtkClaw::SetGoalState(float *state)
{
  int idx;
  
  for (idx = 0; idx < VTK_CLAW_DIMENSIONS; ++idx)
    {
    this->GoalState[idx] = state[idx];
    }
  this->Modified();
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
  this->Modified();
}

  





// Global untill we convert the rest of the functions to methods.
vtkStateSpace *STATE_SPACE;
// Global that gives search strategies a scale.
static float ROBOT_RADIUS;




static void path_verify_step_set(float step_size);
static void Sphere_search_strategy_set(int strategy);
static int Spheres_free_count();
static int Spheres_collision_count();
static void Sphere_collisions_prune();
static void Sphere_all_free();
//----------------------------------------------------------------------------
void vtkClaw::GeneratePath()
{
  SphereList *path = NULL;
  int goal_iterations, start_iterations;
  int strategyIdx;


  // Save the Space as a global
  STATE_SPACE = this->StateSpace;
  // Give the space some scale.
  ROBOT_RADIUS = this->InitialSphereRadius * 2;
  
  /* get rid of all the Spheres from previous runs */
  Sphere_all_free();
  this->SphereVerifiedLinksClear();

  this->SphereStartGoalInitialize(this->StartState, this->GoalState, 
				  this->InitialSphereRadius);

  
  /* set the step resolution for verification */
  path_verify_step_set(this->VerifyStep);
  
  /* call the path generation routines */
  strategyIdx = -1;
  while( ! path)
    {
    /* determine how much time to spend searching goal */
    goal_iterations = (int)(this->SamplePeriod*this->GoalPercentage * 0.01);
    start_iterations = this->SamplePeriod - goal_iterations;
    
    /* Change search strategy */
    ++strategyIdx;
    if (strategyIdx >= this->NumberOfSearchStrategies)
      {
      strategyIdx = 0;
      }
    Sphere_search_strategy_set(this->SearchStrategies[strategyIdx]);
    
    /* search the start network */
    if (! path && start_iterations)
      {
      vtkDebugMacro(<< "Searching start");
      path = this->PathGenerate(start_iterations, 0, this->ChildFraction);
      vtkDebugMacro(<< "num free = " << Spheres_free_count()
                    << ", num collisions = " << Spheres_collision_count());
      }

    /* search the goal network */
    if (! path && goal_iterations)
      {
      vtkDebugMacro(<< "Searching goal");
      path = PathGenerate(goal_iterations, 1, this->ChildFraction);
      vtkDebugMacro(<< "num free = " << Spheres_free_count()
                    << ", num collisions = " << Spheres_collision_count());
      }

    Sphere_collisions_prune();
    
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




static void SpherePrint(Sphere *b);
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
  Sphere_search_strategy_set(VTK_CLAW_NEAREST_NETWORK);
  path = this->PathGetValid(START_SPHERE, GOAL_SPHERE);
  while( ! path)
    {
    // Only search goal (because oll other networks are considered start)
    vtkDebugMacro(<< "Searching goal");
    path = PathGenerate(200, 1, this->ChildFraction);
    vtkDebugMacro(<< "num free = " << Spheres_free_count()
                  << ", num collisions = " << Spheres_collision_count());
    }

  // should we delete any previous path?????????????????????????????????
  this->Path = path;
  
  return flag;
}

static int SphereCandidateValid(Sphere *b, float *proposed);
static Sphere *Sphere_make(float *center, float radius, int visited);
static void Sphere_collision_add(Sphere *b, Sphere *parent);
//----------------------------------------------------------------------------
// Description:
// A helper method for SmoothPath.  This method makes sure a sphere 
// has no free neighbors.  It returns 1 if the sphere was modified.
int vtkClaw::SmoothSphere(Sphere *s)
{
  int axis;
  float child[VTK_CLAW_DIMENSIONS];
  float temp;
  int flag = 0;
  
  if (this->Debug)
    {
    printf("Smoothing sphere:\n    ");
    SpherePrint(s);
    }

  temp = s->Radius * this->ChildFraction;
  // loop through axes (to find children)
  for (axis = 0; axis < VTK_CLAW_DIMENSIONS; ++axis)
    {
    STATE_SPACE->GetChildState(s->Center, axis, temp, child);
    if (STATE_SPACE->Collide(child))
      {
      // Collision
      Sphere_collision_add(Sphere_make(child, 0.0, -1), s);
      vtkDebugMacro(<< "Collision");
      // Call this function recursively (sphere now has smaller radius)
      this->SmoothSphere(s);
      return 1;
      }
    else if (SphereCandidateValid(s, child))
      {
      // Make a neighbor
      flag = 1;
      this->SphereNew(child, s);
      }

    STATE_SPACE->GetChildState(s->Center, axis, -temp, child);
    if (STATE_SPACE->Collide(child))
      {
      // Collision
      Sphere_collision_add(Sphere_make(child, 0.0, -1), s);
      vtkDebugMacro(<< "Collision");
      // Call this function recursively (sphere now has smaller radius)
      this->SmoothSphere(s);
      return 1;
      }
    else if (SphereCandidateValid(s, child))
      {
      // Make a neighbor
      flag = 1;
      this->SphereNew(child, s);
      }
    }
  
  return flag;
}




static void free_Spheres_print();
//----------------------------------------------------------------------------
// for debugging
void vtkClaw::PrintFreeSpheres ()
{
  free_Spheres_print();
}


//----------------------------------------------------------------------------
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
    for (idx = 1; idx < VTK_CLAW_DIMENSIONS; ++idx)
      {
      (*file) << " ";
      (*file) << b->Center[idx];
      }
    (*file) << "\n";
    }
  file->close();
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
  for (idx = 0; idx < VTK_CLAW_DIMENSIONS; ++idx)
    {
    state[idx] = b->Center[idx];
    }
}











//############################################################################
// Private C functions that have not been converted to methods yet.
//############################################################################



/* determines where child should be created in sphere */
static float SPHERE_CHILD_FRACTION = 0.65;

/* For returning best_Sphere from last network searched */
static int LAST_NETWORK_SEARCHED = 0;

/* tells which search strategy to use */
static int SEARCH_STRATEGY = 0;

/* A list of all FREE_SPHERES */
static SphereList *FREE_SPHERES = NULL;
/* The radius of the largest unknown Sphere */
static float SPHERE_MAX_RESOLUTION;

/* A list of points (stored as Sphere centers) which are not in free space */
static SphereList *COLLISIONS = NULL;

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
static void SphereList_element_free(SphereList *l)
{
  l->Next = SPHERELIST_HEAP;
  SPHERELIST_HEAP = l;
}

//----------------------------------------------------------------------------
// This function frees all the elements in a list.
static void SphereList_all_free(SphereList *l)
{
  SphereList *temp;

  while(l){
    temp = l;
    l = l->Next;
    SphereList_element_free(temp);
  }
}

//----------------------------------------------------------------------------
// This function removes the first cell containing "b", from the SphereList "l".
static void SphereList_item_remove(SphereList **pl, void *ptr)
{
  SphereList *temp;

  while (*pl != NULL){
    if((*pl)->Item == ptr){
      temp = *pl;
      *pl = (*pl)->Next;
      SphereList_element_free(temp);
      return;
    } else {
      pl = &((*pl)->Next);
    }
  }
}



//----------------------------------------------------------------------------
// Returns a new list with "item" appended to the begining of "l".
static SphereList *SphereListAdd(SphereList *l, Sphere *item)
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


static float SphereSort(Sphere *b);
//----------------------------------------------------------------------------
// Finds the best Sphere of a network.
static Sphere *SphereListNetworkBest(SphereList *list, int network)
{
  float biggest;
  Sphere *best_Sphere = NULL;
  Sphere *b;
  SphereList *l;


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

      if (!best_Sphere || SphereSort(b) > biggest){
	best_Sphere = b;
	biggest = SphereSort(b);
      }
    }
  }
  
  return best_Sphere;
}

/*============================================================================
  End of list functions.
============================================================================*/






static void SphereRadiusReduce(Sphere *b, float radius);
//----------------------------------------------------------------------------
// External.
// This function sets the starting point and goal of the planner.
int vtkClaw::SphereStartGoalInitialize(float *start_state, float *goal_state,
				       float radius)
{
  /* Create the first Sphere and last Sphere*/
  START_SPHERE = this->SphereNew(start_state, NULL);
  if(!START_SPHERE){
    fprintf(stderr, "start position not in free space. (start_goal_init)\n");
    exit(0);
  }
  SphereRadiusReduce(START_SPHERE, radius);

  GOAL_SPHERE = this->SphereNew(goal_state, NULL);
  if(!GOAL_SPHERE){
    fprintf(stderr, "goal position not in free space. (start_goal_init)\n");
    exit(0);
  }
  SphereRadiusReduce(GOAL_SPHERE, radius);
  GOAL_MERGED = 1;

  return 1;
}





static Sphere *Sphere_nearest(Sphere *b);
static void Spheres_print(SphereList *spheres);
//----------------------------------------------------------------------------
// External
// For debugging
static void free_Spheres_print()
{
  Spheres_print(FREE_SPHERES);
}


static int Spheres_count(SphereList *spheres);
//----------------------------------------------------------------------------
// External
// Returns the number of spheres in free space.
static int Spheres_free_count()
{
  return Spheres_count(FREE_SPHERES);
}

//----------------------------------------------------------------------------
// External
// Returns the number of spheres in collision space.
static int Spheres_collision_count()
{
  return Spheres_count(COLLISIONS);
}






/*----------------------------------------------------------------------------
  Build and manipulate spheres
----------------------------------------------------------------------------*/

//----------------------------------------------------------------------------
// This function tries to makes a new Sphere from a center and radius.
// If the center is in free space the Sphere is created.  Either way,
// the entire network is updated with the new information.
// The function returns the Sphere if one was created, NULL otherwise.
Sphere *vtkClaw::SphereNew(float *center, Sphere *parent)
{
  Sphere *b;

  
  if (STATE_SPACE->Collide(center)){
    b = Sphere_make(center, 0.0, -1);
    Sphere_collision_add(b, parent);
    return NULL;
  }

  if (parent)
    b = Sphere_make(center, SPHERE_MAX_RADIUS, parent->Visited);
  else
    b = Sphere_make(center, SPHERE_MAX_RADIUS, 0);

  if (this->Debug)
    {
    printf("New Sphere:\n   ");
    SpherePrint(b);
    }
  
  return this->SphereAdd(b);
}


static void Sphere_neighbors_prune(Sphere *b);
//----------------------------------------------------------------------------
// This function reduces the radius of a Sphere, and updates neighbors.
// It does not resort the FREE_SPHERES list.
static void SphereRadiusReduce(Sphere *b, float radius)
{
  /* Set the new radius */
  b->Radius = radius;

  Sphere_neighbors_prune(b);
}

//----------------------------------------------------------------------------
// This function removes all neighbors that should not be in list.
// I also makes sure the Sphere is not in neighbor neighbors list.
static void Sphere_neighbors_prune(Sphere *b)
{
  SphereList *list, *pruned_list, *temp;
  Sphere *neighbor;

  pruned_list = NULL;
  list = b->Neighbors;
  while (list){
    neighbor = list->Item;
    /* if this box no longer overlaps neighbor ... (avoid round off) */
    if (STATE_SPACE->Distance(neighbor->Center, b->Center) + 0.00001 >= 
	(neighbor->Radius + b->Radius)){
      /* remove this neighbor from list */
      temp = list;
      list = list->Next;
      SphereList_element_free(temp);
      /* remove this Sphere from neighbor's neighbor list also */
      SphereList_item_remove(&(neighbor->Neighbors), b);
    } else {
      /* Place this neighbor on in list (Keep the neighbor) */
      temp = list;
      list = list->Next;
      temp->Next = pruned_list;
      pruned_list = temp;
    }
    /* Update neighbors free surface area */
    neighbor->SortValid = (char)(0);
    neighbor->SurfaceAreaValid = (char)(0);
  }
  /* replace neighbor_list with pruned list */
  b->Neighbors = pruned_list;
  /* Update this spheres free surface area */
  b->SortValid = (char)(0);
  b->SurfaceAreaValid = (char)(0);
}



//----------------------------------------------------------------------------
// This function makes a Sphere structure.
static Sphere *Sphere_make(float *center, float radius, int visited)
{
  Sphere *b;
  int idx;

  b = (Sphere *)malloc(sizeof(struct Sphere));
  if (!b){
    printf("malloc failed. (Sphere_make)\n");
    return NULL;
  }

  for(idx = 0; idx < VTK_CLAW_DIMENSIONS; ++idx)
    b->Center[idx] = center[idx];

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
static void Sphere_collision_add(Sphere *b, Sphere *parent)
{
  SphereList *l;
  float temp;

  COLLISIONS = SphereListAdd(COLLISIONS, b);
  b->Visited = -1;

  if ( parent){
    /* We are garenteed parent is the only sphere which contains collision */
    temp = STATE_SPACE->Distance(parent->Center, b->Center) - b->Radius;
    /* Reduce the spheres radius */
    SphereRadiusReduce(parent, temp);
    /* Add collision neighbor */
    b->Neighbors = SphereListAdd(b->Neighbors, parent);
  } else {
    /* This collision must be fron verifiing a path. Check all spheres */
    l = FREE_SPHERES;
    while (l){
      temp = STATE_SPACE->Distance(l->Item->Center, b->Center) - b->Radius;
      if (temp <= l->Item->Radius){
	/* Reduce the spheres radius */
	SphereRadiusReduce(l->Item, temp);
	/* Add collision neighbor */
	b->Neighbors = SphereListAdd(b->Neighbors, l->Item);
      }
      l = l->Next;
    }
  }
}


//----------------------------------------------------------------------------
// This function frees a Sphere
// It does not bother removing itself as a neighbor of other spheres.
static void Sphere_free(Sphere *b)
{
  SphereList_all_free(b->Neighbors);
  free(b);
}


//----------------------------------------------------------------------------
// EXTERNAL
// This function removes all spheres (to start the algorithm over)
static void Sphere_all_free()
{
  SphereList *l1;

  /* free the collision spheres */  
  l1 = COLLISIONS;
  while (l1){
    Sphere_free(l1->Item);
    l1 = l1->Next;
  }
  SphereList_all_free(COLLISIONS);
  COLLISIONS = NULL;


  /* free the free spheres */  
  l1 = FREE_SPHERES;
  while (l1){
    Sphere_free(l1->Item);
    l1 = l1->Next;
  }
  SphereList_all_free(FREE_SPHERES);
  FREE_SPHERES = NULL;
}
	
	


//----------------------------------------------------------------------------
// EXTERNAL
// This function removes collisions that do not have neighbors.
static void Sphere_collisions_prune()
{
  SphereList *new_collisions = NULL;
  SphereList *new_neighbors;
  SphereList *l1, *l2, *temp;
  
  l1 = COLLISIONS;
  while (l1){
    new_neighbors = NULL;
    l2 = l1->Item->Neighbors;
    while (l2){
      if (STATE_SPACE->Distance(l1->Item->Center, l2->Item->Center) <=
	  l2->Item->Radius + 0.001){
	/* keep: move to new neighbors list */
	temp = l2->Next;
	l2->Next = new_neighbors;
	new_neighbors = l2;
	l2 = temp;
      } else {
	/* dispose of this neighbor */
	temp = l2->Next;
	SphereList_element_free(l2);
	l2 = temp;
      }
    }
    l1->Item->Neighbors = new_neighbors;

    if (new_neighbors){
      /* keep: move to new collisions list */
      temp = l1->Next;
      l1->Next = new_collisions;
      new_collisions = l1;
      l1 = temp;
    } else {
      /* dispose of this Sphere */
      temp = l1->Next;
      free(l1->Item);
      SphereList_element_free(l1);
      l1 = temp;
    }
  }

  COLLISIONS = new_collisions;
}
	
	


//----------------------------------------------------------------------------
// This function recomputes a spheres nearest if necessary.
// It returns the nearest_val.
static float Sphere_nearest_val(Sphere *b)
{
  /* ignore collision nodes */
  if (b->Visited < 0)
    return 10000;

  /* make sure nearest is up to date */
  if ( ! Sphere_nearest(b))
    return 10000;  /* could not find nearest */

  /* return the distance between sphere surfaces */
  return (b->NearestVal - 
	  (b->Radius + b->Nearest->Radius));
}


//----------------------------------------------------------------------------
// This function recomputes a spheres nearest if necessary.
// It returns the nearest.
static Sphere *Sphere_nearest(Sphere *b)
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
    l = FREE_SPHERES;
    while (l){
      other = l->Item;
      l = l->Next;

      if (((b->Visited > 0) && (other->Visited == 0)) || 
	  ((b->Visited == 0) && (other->Visited > 0))){
	temp = STATE_SPACE->Distance(b->Center, other->Center);
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
// This function adds another Sphere to the space.
// It updates all links. (should in shink the new Sphere if necessary?)
// It calculates the "Unknown" surface area of the box, and adds
// it the the list "SPHERES".
Sphere *vtkClaw::SphereAdd(Sphere *b)
{
  SphereList *l;
  Sphere *other;
  float temp;

  /* Find the closest collision to determine radius */
  other = NULL;
  l = COLLISIONS;
  while(l){
    temp = STATE_SPACE->Distance(b->Center, l->Item->Center);
    if (temp < b->Radius){
      b->Radius = temp;
      other = l->Item;
    }
    l = l->Next;
  }

  /* the closest collision */
  if(other){
    other->Neighbors = SphereListAdd(other->Neighbors, b);
  }

  /* Add neighbors from valid FREE_SPHERES */
  l = FREE_SPHERES;
  while (l){
    other = l->Item;
    l = l->Next;
    temp = STATE_SPACE->Distance(b->Center, other->Center);

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

    /* if these two spheres are touching */
    if (temp <= ((b->Radius + other->Radius))){
      /* Check for merged goal net */
      if ((b->Visited && !other->Visited) || (!b->Visited && other->Visited)){
	/* Start and goal spheres are touching. Verify link. */
	if ( !GOAL_MERGED && this->SphereLinkVerify(b, other))
	  GOAL_MERGED = 1;
      }
    }

    /* if these two spheres are touching */
    if (temp <= ((b->Radius + other->Radius))){
      /* update the neighbor lists */
      b->Neighbors = SphereListAdd(b->Neighbors, other);
      other->Neighbors = SphereListAdd(other->Neighbors, b);
      /* Update neighbors free surface area */
      other->SortValid = (char)(0);
      other->SurfaceAreaValid = (char)(0);
    }
  }

  /* Add to the list */
  FREE_SPHERES = SphereListAdd(FREE_SPHERES, b);

  /* Update this spheres free surface area */
  b->SortValid = (char)(0);
  b->SurfaceAreaValid = (char)(0);

/*
  printf("new valid Sphere:\n");
  SpherePrint(b);
*/

  return b;
}


static float Sphere_nearest_val(Sphere *b);
static int Sphere_num_neighbors(Sphere *b);
static float Sphere_surface_area(Sphere *b);
//----------------------------------------------------------------------------
static void SpherePrint(Sphere *b)
{
  int idx;

  if(!b){
    printf("NULL\n");
    return;
  }

  printf("space DrawRobot %.4f", b->Center[0]);
  for (idx = 1; idx < VTK_CLAW_DIMENSIONS; ++idx)
    printf(" %.4f", b->Center[idx]);
  printf("; # r: %.3f, sort: %.6f, net %d, num %.1f,%d, dist %.1f, near %.3f,%.3f",
	 b->Radius, SphereSort(b), b->Visited, 
	 Sphere_surface_area(b), Sphere_num_neighbors(b),
	 STATE_SPACE->BoundsTest(b->Center), Sphere_nearest_val(b), 
	 b->NearestVal);

  b = Sphere_nearest(b);
  if (b)
    printf(", %.3f\n", Sphere_nearest_val(b));
  else
    printf("\n");

  fflush(stdout);
}

      


//----------------------------------------------------------------------------
static int Sphere_num_neighbors(Sphere *b)
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
static int 
SphereCandidateValid(Sphere *b, float *proposed)
{
  float temp = b->Radius * SPHERE_CHILD_FRACTION;
  SphereList *l;
  Sphere *other;

  /* handle if the sphere touches itself */
  if (STATE_SPACE->Distance(b->Center, proposed) < (temp * 0.9))
    {
    return 0;
    }

  /* Remove if a neighbor contains this point */
  l = b->Neighbors;
  while (l)
    {
    other = l->Item;
    l = l->Next;
    if (STATE_SPACE->Distance(other->Center, proposed) < other->Radius)
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
static int 
Sphere_candidates_get(Sphere *b, int candidates[VTK_CLAW_DIMENSIONS][2])
{
  float proposed[VTK_CLAW_DIMENSIONS];
  int num_candidates, valid;
  int direction, axis;
  float temp;

  num_candidates = 0;
  
  /* Find the number of unoccupied inbounds candidates */
  temp = b->Radius * SPHERE_CHILD_FRACTION;
  for (axis = 0; axis < VTK_CLAW_DIMENSIONS; ++axis){
    for (direction = 0; direction < 2; ++direction){
      /* Set up child state */
      if (direction)
	{
	STATE_SPACE->GetChildState(b->Center, axis, temp, proposed);
	}
      else
	{
	STATE_SPACE->GetChildState(b->Center, axis, -temp, proposed);
	}
      /* Wrap the new position if necessary */
      STATE_SPACE->Wrap(proposed);

      valid = SphereCandidateValid(b, proposed);

      num_candidates += valid;
      candidates[axis][direction] = valid;
    }
  }
  
  return num_candidates;
}



//----------------------------------------------------------------------------
// This function returns (pointer argument) an array which tells which
// search direstions of a Sphere are already filled (and which are free). 
// The integer return value is the number of candidates.
static float Sphere_surface_area(Sphere *b)
{
  if ( ! b->SurfaceAreaValid){
    int candidates[VTK_CLAW_DIMENSIONS][2];

    b->SurfaceArea = (float)(Sphere_candidates_get(b, candidates));
    /* consider the guide tube at this point */
    b->SurfaceArea *= STATE_SPACE->BoundsTest(b->Center);
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
static void Spheres_print(SphereList *spheres)
{
  SphereList *list;

  printf("Sphere list:\n");
  list = spheres;
  while (list){
    SpherePrint(list->Item);
    list = list->Next;
  }
  printf("\n");
}



//----------------------------------------------------------------------------
// Returns the length of the list.
static int Spheres_count(SphereList *spheres)
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
static void Sphere_search_strategy_set(int strategy)
{
  SphereList *l;
  Sphere *b;

  SEARCH_STRATEGY = strategy;
  printf("\nchanging search strategy to: %d\n", strategy);

  /* recompute all the sort_values */
  l = FREE_SPHERES;
  while (l){
    b = l->Item;
    l = l->Next;
    b->SortValid = (char)(0);
  }
}
    


static float 
Sphere_nearest_network_move_evaluate(Sphere *b, int axis, int direction);
static float Sphere_nearest_global_move_evaluate(Sphere *b, float *proposed);
static float Sphere_pioneer_local_move_evaluate(Sphere *b, float *proposed);
static float Sphere_pioneer_global_move_evaluate(Sphere *b, float *proposed);
static float Sphere_noise_move_evaluate();
static float Sphere_nearest_noise_move_evaluate(Sphere *b, float *proposed);
//----------------------------------------------------------------------------
// This function chooses the next position to spawn a child.
// It uses the search strategy to select a huristic.
// Returns the chosen position in proposed.
static int Sphere_candidate_choose(Sphere *b, float *proposed)
{
  int candidates[VTK_CLAW_DIMENSIONS][2];
  int direction, axis;
  float best, temp, first = 1;
  int best_direction, best_axis;
  float sortSave;

  sortSave = SphereSort(b);
  /* find the free directions */
  Sphere_candidates_get(b, candidates);

  /* Choose the best free direction. */
  for (axis = 0; axis < VTK_CLAW_DIMENSIONS; ++axis)
    {
    for (direction = 0; direction < 2; ++direction)
      {
      if (candidates[axis][direction])
	{
	/* set up the proposed position */
	temp = b->Radius * SPHERE_CHILD_FRACTION;
	if (direction)
	  {
	  STATE_SPACE->GetChildState(b->Center, axis, temp, proposed);
	  }
	else
	  {
	  STATE_SPACE->GetChildState(b->Center, axis, -temp, proposed);
	  }
	STATE_SPACE->Wrap(proposed);
	/* get the rating of this position from the search_strategy */
	if (SEARCH_STRATEGY == 0)
	  temp = Sphere_nearest_network_move_evaluate(b, axis, direction);
	if (SEARCH_STRATEGY == 1)
	  temp = Sphere_nearest_network_move_evaluate(b, axis, direction);
	if (SEARCH_STRATEGY == 2)
	  temp = Sphere_nearest_global_move_evaluate(b, proposed);
	if (SEARCH_STRATEGY == 3)
	  temp = Sphere_pioneer_local_move_evaluate(b, proposed);
	if (SEARCH_STRATEGY == 4)
	  temp = Sphere_pioneer_global_move_evaluate(b, proposed);
	if (SEARCH_STRATEGY == 5)
	  temp = Sphere_nearest_network_move_evaluate(b, axis, direction);
	if (SEARCH_STRATEGY == 6)
	  temp = Sphere_noise_move_evaluate();
	if (SEARCH_STRATEGY == 7)
	  temp = Sphere_nearest_noise_move_evaluate(b, proposed);
	if (SEARCH_STRATEGY == 8)
	  temp = Sphere_nearest_network_move_evaluate(b, axis, direction);
	if (SEARCH_STRATEGY == 9)
	  temp = Sphere_noise_move_evaluate();
	if (SEARCH_STRATEGY == 10)
	  temp = Sphere_noise_move_evaluate();
	/* if this beats the best so far, (or is the first) save it */
	if (first || temp > best)
	  {
	  first = 0;
	  best = temp;
	  best_axis = axis;
	  best_direction = direction;
	  }
	}
      }
    }
  
  
  if (first)
    {
    printf("problem choosing best direction (no free directions) %f\n",
	   sortSave);
    // Just incase something was not upto date
    SpherePrint(b);
    b->SortValid = 0;
    b->SurfaceAreaValid = 0;
    SpherePrint(b);
    
    return 0;
    }
  

  /* set up the best proposed position */
  temp = b->Radius * SPHERE_CHILD_FRACTION;
  if (best_direction)
    {
    STATE_SPACE->GetChildState(b->Center, best_axis, temp, proposed);
    }
  else
    {
    STATE_SPACE->GetChildState(b->Center, best_axis, -temp, proposed);
    }
  STATE_SPACE->Wrap(proposed);
  
  return 1;
}


//----------------------------------------------------------------------------
// Get as close to the other network as possible (the nearest)
// larger return value is better.
static float 
Sphere_nearest_network_move_evaluate(Sphere *b, int axis, int direction)
{
  Sphere *nearest;
  
  nearest = Sphere_nearest(b);

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
static float Sphere_nearest_global_move_evaluate(Sphere *b, float *proposed)
{
  float temp;

  if (b->Visited){
    /* this Sphere *is in the goal network */
    if (! START_SPHERE)
      return 0.0;
    temp = STATE_SPACE->Distance(proposed, START_SPHERE->Center);
  } else {
    /* this Sphere is in the start network */
    if (! GOAL_SPHERE)
      return 0.0;
    temp = STATE_SPACE->Distance(proposed, GOAL_SPHERE->Center);
  }

  return 1 / temp;
}



//----------------------------------------------------------------------------
// Move away from neighbors.
// larger return value is better.
static float Sphere_pioneer_local_move_evaluate(Sphere *b, float *proposed)
{
  SphereList *l;
  float temp = 0.0;

  l = b->Neighbors;
  while (l){
    temp += STATE_SPACE->Distance(proposed, l->Item->Center);
    l = l->Next;
  }

  return temp;
}


//----------------------------------------------------------------------------
// Move away from this spheres root.
// larger return value is better.
static float Sphere_pioneer_global_move_evaluate(Sphere *b, float *proposed)
{
  float temp;

  if (b->Visited){
    /* this Sphere is in the goal network */
    if (! GOAL_SPHERE)
      return 0.0;
    temp = STATE_SPACE->Distance(proposed, GOAL_SPHERE->Center);
  } else {
    /* this Sphere *is in the start network */
    if (! START_SPHERE)
      return 0.0;
    temp = STATE_SPACE->Distance(proposed, START_SPHERE->Center);
  }

  return temp;
}
  

//----------------------------------------------------------------------------
// Adds noise to the search. Just returns a randorm number.
static float Sphere_noise_move_evaluate()
{
  return  (float)(rand()) / 2147483648.0;
}
  



//----------------------------------------------------------------------------
// Selects randomly among directions which move the Sphere closer
// to the nearset network
static float Sphere_nearest_noise_move_evaluate(Sphere *b, float *proposed)
{
  Sphere *nearest;

  nearest = Sphere_nearest(b);
  if (STATE_SPACE->Distance(proposed, nearest->Center) < b->NearestVal)
    return  (float)(rand()) / 2147483648.0;
  else
    return 0.0;
}
  





static float Sphere_nearest_network_sort_compute(Sphere *b);
static float Sphere_nearest_minimum_sort_compute(Sphere *b);
static float Sphere_nearest_global_sort_compute(Sphere *b);
static float Sphere_pioneer_local_sort_compute(Sphere *b);
static float Sphere_pioneer_global_sort_compute(Sphere *b);
static float Sphere_close_tolerance_sort_compute(Sphere *b);
static float Sphere_nearest_net_pioneer_sort_compute(Sphere *b);
static float Sphere_minimum_well_sort_compute(Sphere *b);
static float Sphere_narrow_well_sort_compute(Sphere *b);
//----------------------------------------------------------------------------
// A function to compute the free surface area of a box.
// A variation of the above. (neighbor must include proposed point).
static float SphereSort(Sphere *b)
{
  /* It is possible that this value could change with out setting valid to 0 */
  /* (for nearest_minimum)  (if nearest changes its nearest_val) */
  /* this small inconsistancey should not make much difference */
  if (b->SortValid)
    return b->Sort;

    
  if (SEARCH_STRATEGY == 0)
    b->Sort = Sphere_nearest_network_sort_compute(b);
  if (SEARCH_STRATEGY == 1)
    b->Sort = Sphere_nearest_minimum_sort_compute(b);
  if (SEARCH_STRATEGY == 2)
    b->Sort = Sphere_nearest_global_sort_compute(b);
  if (SEARCH_STRATEGY == 3)
    b->Sort = Sphere_pioneer_local_sort_compute(b);
  if (SEARCH_STRATEGY == 4)
    b->Sort = Sphere_pioneer_global_sort_compute(b);
  if (SEARCH_STRATEGY == 5)
    b->Sort = Sphere_nearest_net_pioneer_sort_compute(b);
  if (SEARCH_STRATEGY == 6)
    b->Sort = Sphere_minimum_well_sort_compute(b);
  if (SEARCH_STRATEGY == 7)
    b->Sort = Sphere_minimum_well_sort_compute(b);
  if (SEARCH_STRATEGY == 8)
    b->Sort = Sphere_minimum_well_sort_compute(b);
  if (SEARCH_STRATEGY == 9)
    b->Sort = Sphere_close_tolerance_sort_compute(b);
  if (SEARCH_STRATEGY == 10)
    b->Sort = Sphere_narrow_well_sort_compute(b);
  b->SortValid = (char)(1);
  return b->Sort;
}
  



//----------------------------------------------------------------------------
// 1/StateSpace->Distance(nearest_neighbor);
// larger is better.
static float Sphere_nearest_network_sort_compute(Sphere *b)
{
  float temp = Sphere_nearest_val(b);

  /* hack to cover up intermitent bug */
  if (temp <= 0.00001)
    temp = 0.00001;

  return b->Radius * (Sphere_surface_area(b) + 0.0001) / temp;
}


//----------------------------------------------------------------------------
// Search LOCAL minimum.
// larger is better.
static float Sphere_nearest_minimum_sort_compute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = Sphere_nearest(b);
  if ( ! nearest)
    return 0.00001;

  temp = Sphere_nearest_val(b) - Sphere_nearest_val(nearest);

  if (temp <= 0.00001)
    temp = 0.00001;

  /* what value should I use (0.5, 1.0, some measure of STD) */
  return b->Radius * (Sphere_surface_area(b) + 0.0001) 
    / (0.5 + temp);
}


//----------------------------------------------------------------------------
// 1 / (StateSpace->Distance(GOAL))
// larger is better.
static float Sphere_nearest_global_sort_compute(Sphere *b)
{
  if (b->Visited > 0){
    /* this Sphere is part of the goal netework */
    if ( ! START_SPHERE)
      return 0.00001;
    return b->Radius * (Sphere_surface_area(b) + 0.00001) 
      / STATE_SPACE->Distance(b->Center, START_SPHERE->Center);
  } else {
    /* this Sphere *is part of the start netework */
    if ( ! GOAL_SPHERE)
      return 0.00001;
    return b->Radius * (Sphere_surface_area(b) + 0.00001) 
      / STATE_SPACE->Distance(b->Center, GOAL_SPHERE->Center);
  }
}


//----------------------------------------------------------------------------
// 1 / (length(neighbors));
// larger is better.
static float Sphere_pioneer_local_sort_compute(Sphere *b)
{
  return b->Radius * (Sphere_surface_area(b) + 0.00001) 
    / (0.01 + (float)(Sphere_num_neighbors(b)));
}




//----------------------------------------------------------------------------
// StateSpace->Distance(START).
// larger is better.
static float Sphere_pioneer_global_sort_compute(Sphere *b)
{
  if (b->Visited > 0){
    /* this Sphere is part of the goal netework */
    if ( ! GOAL_SPHERE)
      return 0.00001;
    return b->Radius * (Sphere_surface_area(b) + 0.00001) 
      * STATE_SPACE->Distance(b->Center, GOAL_SPHERE->Center);
  } else {
    /* this Sphere is part of the start netework */
    if ( ! START_SPHERE)
      return 0.00001;
    return b->Radius * (Sphere_surface_area(b) + 0.00001) 
      * STATE_SPACE->Distance(b->Center, START_SPHERE->Center);
  }
}



//----------------------------------------------------------------------------
// 1/(StateSpace->Distance(nearest_neighbor) * num_neighbors);
// larger is better.
static float Sphere_nearest_net_pioneer_sort_compute(Sphere *b)
{
  b = b;
  printf("nearest_net_pioneer is no longer available (spheres.cls)\n");
  return 1.0;
}


//----------------------------------------------------------------------------
// focus on the local minimum
// larger is better.
// problem with this: shhould never return 0 (sometimes all are 0)
static float Sphere_minimum_well_sort_compute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = Sphere_nearest(b);
  if ( ! nearest)
    return 0.00001;

  temp = (ROBOT_RADIUS * 0.02)
    - (Sphere_nearest_val(b) - Sphere_nearest_val(nearest));

  if (temp <= 0.00001)
    temp = 0.00001; /* hack */

  return b->Radius * (Sphere_surface_area(b) + 0.0000001) * temp;
}

//----------------------------------------------------------------------------
// same as nearest minimum but much more focused search of minimum.
// larger is better.
static float Sphere_close_tolerance_sort_compute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = Sphere_nearest(b);
  if ( ! nearest)
    return 0.0;

  temp = (ROBOT_RADIUS * 0.01)
    - (Sphere_nearest_val(b) - Sphere_nearest_val(nearest));

  if (temp <= 0.00001)
    temp = 0.00001; /* hack */

  return b->Radius * (Sphere_surface_area(b) + 0.00001) * temp;
}


//----------------------------------------------------------------------------
// same as close tolerence, except narrow minimumare prefered to wide.
// (ignore radius => narrow has lots with small radius and is prefered)
static float Sphere_narrow_well_sort_compute(Sphere *b)
{
  float temp;
  Sphere *nearest;

  nearest = Sphere_nearest(b);
  if ( ! nearest)
    return 0.0;
  Sphere_nearest(nearest);

  temp = (ROBOT_RADIUS * 0.02) - (b->NearestVal - nearest->NearestVal);

  if (temp <= 0.00001)
    temp = 0.00001; /* hack */

  return b->Radius * (Sphere_surface_area(b) + 0.00001) * temp;
}





/*----------------------------------------------------------------------------
               End of search strategy stuff.
----------------------------------------------------------------------------*/














/*----------------------------------------------------------------------------
  Routines for searching the Sphere space for a path, and verifiing the path.
----------------------------------------------------------------------------*/


float VERIFY_STEP = 1.0;
//----------------------------------------------------------------------------
// EXTERNAL
static void path_verify_step_set(float step_size)
{
  VERIFY_STEP = step_size;
}



static int Sphere_candidate_choose(Sphere *b, float *center);
static void SpherePrint(Sphere *b);
static Sphere *Sphere_nearest(Sphere *b);
//----------------------------------------------------------------------------
// External
// This function fills the free space with spheres until a path is found,
// or until "additional_spheres" additional spheres are created.
// It returns the verified path or NULL.
SphereList *vtkClaw::PathGenerate(int additional_Spheres, 
				  int network, float child_fraction)
{
  Sphere *b;
  float center[VTK_CLAW_DIMENSIONS];
  SphereList *path = NULL;
  int temp;

  SPHERE_CHILD_FRACTION = child_fraction;
  LAST_NETWORK_SEARCHED = network;

  if(!START_SPHERE || !GOAL_SPHERE){
    vtkErrorMacro(<< "start and goal position must be initialized (PathGenerate)");
    return NULL;
  }
  
  while (!path && additional_Spheres > 0){
    /* if the newest Sphere contains goal, find path and try to verify */
    if (GOAL_MERGED){
      path = this->PathGetValid(START_SPHERE, GOAL_SPHERE);
      GOAL_MERGED = 0;
    } else {
      --additional_Spheres;
      /* Pick the biggest Sphere in unknown space */
      b = SphereListNetworkBest(FREE_SPHERES, network);
      /* Pick a direction to extend the space */
      temp = Sphere_candidate_choose(b, center);

      /* Create a new Sphere if new position is valid */
      if (temp){
	b = this->SphereNew(center, b);
      }
    }
  }

  printf("Sphere resolution = %f\n", SPHERE_MAX_RESOLUTION);

  return path;
}



//----------------------------------------------------------------------------
// This function is called when the goal and the start spaces merge.  
// The space is searched until a valid path is found, 
// or until the space is split into two sections again.
SphereList *vtkClaw::PathGetValid(Sphere *start_Sphere, Sphere *goal_Sphere)
{
  SphereList *path;

  vtkDebugMacro(<< "-----path_get_valid");
  while ( (path = this->PathSearch(start_Sphere, goal_Sphere)) ){
    if (this->PathVerify(path)){
      printf("-----path validated\n");
      return path;
    }

    /* Free the current path */
    SphereList_all_free(path);
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
  SphereList_all_free(VERIFIED_LINKS);
  VERIFIED_LINKS = NULL;
}


//----------------------------------------------------------------------------
// This recursive function checks to se if two states are linked.
int vtkClaw::SphereLinkStates(float *s1, float *s2, float distance)
{
  // If distance between two states is below specified resolution,
  // confirm and return.
  if (distance <= VERIFY_STEP)
    {
    distance = STATE_SPACE->Distance(s1, s2);
    if (distance > VERIFY_STEP)
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
    float middle[VTK_CLAW_DIMENSIONS];
    // Split the link in two by adding mid point state.
    STATE_SPACE->GetMiddleState(s1, s2, middle);
    if (STATE_SPACE->Collide(middle))
      {
      Sphere_collision_add(Sphere_make(middle, 0.0, -1), NULL);
      printf(" link no good\n");
      fflush(stdout);
      return 0;
      }
    else
      {
      // Recursion: Call this function on the two links
      return (this->SphereLinkStates(s1, middle, distance/2.0) &&
	      this->SphereLinkStates(middle, s2, distance/2.0));
      }
    }
}

    
	 
      
	
  
  



//----------------------------------------------------------------------------
// This function verifies that two spheres are actually connected by taking
// small steps from one two the other. (does not recheck the actual nodes)
int vtkClaw::SphereLinkVerify(Sphere *b0, Sphere *b1)
{
  float distance;

  /* first see if we have already verified this link */
  if (this->SphereLinkVerifiedAlready(b0, b1))
    {
    return 1;
    }

  distance = STATE_SPACE->Distance(b0->Center, b1->Center);
  if ( ! this->SphereLinkStates(b0->Center, b1->Center, distance))
    {
    vtkDebugMacro(<< "SphereLinkVerify: Not Verified " << b0 << ", " << b1);
    return 0;
    }
  
  vtkDebugMacro(<< "SphereLinkVerify: Verified " << b0 << ", " << b1);

  /* save the fact that this link is valid */
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
  SphereList *leaves, *last_leaf, *ll;
  Sphere *b;

  /* Set all the spheres to not visited */
  ll = FREE_SPHERES;
  while(ll){
    ll->Item->Visited = 0;
    ll = ll->Next;
  }

  /* Depth first search from goal */
  leaves = SphereListAdd(NULL, end);
  end->Visited = 1;
  last_leaf = leaves;
  while (leaves){
    /* first Sphere on the list */
    b = leaves->Item;

    /* Add neighbors to end of leaf list */
    ll = b->Neighbors;
    while (ll){
      if ( ! ll->Item->Visited){
	ll->Item->Visited = b->Visited + 1;
	/* Add new Sphere to end of leaves list */
	last_leaf->Next = SphereListAdd(NULL, ll->Item);
	last_leaf = last_leaf->Next;
	/* If we have reached the start Sphere, the a path has been found */
	if(ll->Item == start){
	  SphereList_all_free(leaves);
	  return this->PathUnravel(start);
	}
      }
      ll = ll->Next;
    }
    /* remove the first list element */
    ll = leaves;
    leaves = leaves->Next;
    SphereList_element_free(ll);
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


























