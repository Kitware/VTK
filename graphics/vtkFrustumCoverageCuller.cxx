/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumCoverageCuller.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkFrustumCoverageCuller.h"
#include "vtkActorCollection.h"
#include "vtkActor.h"
#include "vtkRenderer.h"

// Description:
// Create a frustum coverage culler with default values
vtkFrustumCoverageCuller::vtkFrustumCoverageCuller()
{
  this->MinimumCoverage = 0.0001;
  this->MaximumCoverage = 1.0;
}

// Description:
// The out cull method is where the culling occurs in this class.
// The coverage is computed for each actor, and a resulting allocated
// render time is computed. This is multiplied by the current allocated
// render time of the actor. After this, actors with no allocated time are
// removed from the list (and the list length is shortened) to make sure
// that they are not considered again by another culler or for rendering.
float vtkFrustumCoverageCuller::OuterCullMethod( vtkRenderer *ren, 
						 vtkActor **actorList,
						 int& listLength,
						 int& initialized )
{
  vtkActor            *actor;
  float               total_time;
  float               *bounds, center[3], radius;
  float               planes[24], d;
  float               coverage, screen_bounds[4];
  float               previous_time;
  int                 i, actor_loop;
  float               full_w, full_h, part_w, part_h;
  float               *allocatedTimeList;
  int                 last_non_zero;

  // We will return the total time of all actors. This is used for
  // normalization.
  total_time  = 0;

  // Get the view frustum planes from the active camera
  ren->GetActiveCamera()->GetFrustumPlanes( planes );

  // Keep a list of allocated times to help with sorting / removing
  // actors later
  allocatedTimeList = new float[listLength];

  // For each actor, compute coverage
  for ( actor_loop = 0; actor_loop < listLength; actor_loop++ )
    {
    // Get the actor out of the list
    actor = actorList[actor_loop];

    // Process only if the actor is visible
    if ( actor->GetVisibility() )
      {
      // If allocated render time has not been initialized yet (if this
      // is the first culler, it hasn't) then the previous time is set
      // to 0.0
      if ( !initialized )
	previous_time = 1.0;
      else
	previous_time = actor->GetAllocatedRenderTime();
      
      // Get the bounds of the actor and compute an enclosing sphere
      bounds = actor->GetBounds();
      center[0] = (bounds[0] + bounds[1]) / 2.0;
      center[1] = (bounds[2] + bounds[3]) / 2.0;
      center[2] = (bounds[4] + bounds[5]) / 2.0;
      radius = 0.5 * sqrt( (double) 
			   ( bounds[1] - bounds[0] ) *
			   ( bounds[1] - bounds[0] ) +
			   ( bounds[3] - bounds[2] ) *
			   ( bounds[3] - bounds[2] ) +
			   ( bounds[5] - bounds[4] ) *
			   ( bounds[5] - bounds[4] ) );
      
      // We start with a coverage of 1.0 and set it to zero if the actor
      // is culled during the plane tests
      coverage = 1.0;

      for ( i = 0; i < 6; i++ )
	{
	// Compute how far the center of the sphere is from this plane
	d = 
	  planes[i*4 + 0] * center[0] +
	  planes[i*4 + 1] * center[1] +
	  planes[i*4 + 2] * center[2] +
	  planes[i*4 + 3];

	// If d < -radius the actor is not within the view frustum
	if ( d < -radius )
	  {
	  coverage = 0.0;
	  i = 7;
	  }

	// The first four planes are the ones bounding the edges of the
	// view plane (the last two are the near and far planes) The
	// distance from the edge of the sphere to these planes is stored
	// to compute coverage.
	if ( i < 4 )
	  screen_bounds[i] = d - radius;
	}

      // If the actor wasn't culled during the plane tests...
      if ( coverage > 0.0 )
	{
	// Compute the width and height of this slice through the
	// view frustum that contains the center of the sphere
	full_w = screen_bounds[0] + screen_bounds[1] + 2.0 * radius;
	full_h = screen_bounds[2] + screen_bounds[3] + 2.0 * radius;
	
	// Subtract from the full width to get the width of the square
	// enclosing the circle slice from the sphere in the plane
	// through the center of the sphere. If the screen bounds for
	// the left and right planes (0,1) are greater than zero, then
	// the edge of the sphere was a positive distance away from the
	// plane, so there is a gap between the edge of the plane and
	// the edge of the box.
	part_w = full_w;
	if ( screen_bounds[0] > 0.0 ) part_w -= screen_bounds[0];
	if ( screen_bounds[1] > 0.0 ) part_w -= screen_bounds[1];

	// Do the same thing for the height with the top and bottom 
	// planes (2,3).
	part_h = full_h;
	if ( screen_bounds[2] > 0.0 ) part_h -= screen_bounds[2];
	if ( screen_bounds[3] > 0.0 ) part_h -= screen_bounds[3];
	
	// Compute the fraction of coverage
	coverage = (part_w * part_h) / (full_w * full_h);
	
	// Convert this to an allocated render time - coverage less than
	// the minumum result in 0.0 time, greater than the maximum result in
	// 1.0 time, and in between a linear ramp is used
	if ( coverage < this->MinimumCoverage ) 
	  coverage = 0;
	else if ( coverage > this->MaximumCoverage )
	  coverage = 1.0;
	else
	  coverage = (coverage-this->MinimumCoverage) / this->MaximumCoverage;
	}

      // Multiply the new allocated time by the previous allocated time
      coverage *= previous_time;
      actor->SetAllocatedRenderTime( coverage );

      // Save this in our array of allocated times which matches the
      // actor array
      allocatedTimeList[actor_loop] = coverage;

      // Add the time for this actor to the total time
      total_time += coverage;
      }
    else
      allocatedTimeList[actor_loop] = 0.0;
    }

  // Remove all actors that have no allocated render time or are not visible
  // from the list. First, find the last non-zero entry in the list
  last_non_zero = listLength - 1;
  while ( last_non_zero >= 0 && allocatedTimeList[last_non_zero] == 0.0 )
    last_non_zero--;

  // Now traverse the list from the beginning, swapping any zero entries with
  // the last non-zero entry and finding the new last non-zero
  for ( actor_loop = 0; actor_loop < last_non_zero; actor_loop++ )
    {
    if ( allocatedTimeList[actor_loop] == 0.0 )
      {
      allocatedTimeList[actor_loop] = allocatedTimeList[last_non_zero];
      actorList[actor_loop] = actorList[last_non_zero];

      actorList[last_non_zero] = NULL;
      allocatedTimeList[last_non_zero] = 0.0;

      while ( last_non_zero >= 0 && allocatedTimeList[last_non_zero] == 0.0 )
	last_non_zero--;
      }
    }
    
  // Compute the new list length
  listLength = last_non_zero + 1;

  // The allocated render times are now initialized
  initialized = 1;
  
  return total_time;
}

void vtkFrustumCoverageCuller::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkCuller::PrintSelf(os,indent);

}
