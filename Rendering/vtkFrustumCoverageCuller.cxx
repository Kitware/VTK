/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumCoverageCuller.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkFrustumCoverageCuller.h"
#include "vtkProp.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkFrustumCoverageCuller* vtkFrustumCoverageCuller::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFrustumCoverageCuller");
  if(ret)
    {
    return (vtkFrustumCoverageCuller*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFrustumCoverageCuller;
}




// Create a frustum coverage culler with default values
vtkFrustumCoverageCuller::vtkFrustumCoverageCuller()
{
  this->MinimumCoverage = 0.0001;
  this->MaximumCoverage = 1.0;
  this->SortingStyle    = VTK_CULLER_SORT_NONE;
}

// The coverage is computed for each prop, and a resulting allocated
// render time is computed. This is multiplied by the current allocated
// render time of the prop. After this, props with no allocated time are
// removed from the list (and the list length is shortened) to make sure
// that they are not considered again by another culler or for rendering.
float vtkFrustumCoverageCuller::Cull( vtkRenderer *ren, 
				      vtkProp **propList,
				      int& listLength,
				      int& initialized )
{
  vtkProp            *prop;
  float               total_time;
  float               *bounds, center[3];
  float               radius = 0.0;
  float               planes[24], d;
  float               coverage, screen_bounds[4];
  float               previous_time;
  int                 i, propLoop;
  float               full_w, full_h, part_w, part_h;
  float               *allocatedTimeList;
  float               *distanceList;
  int                 index1, index2;
  float               tmp;
  float               aspect[2];

  // We will create a center distance entry for each prop in the list
  // If SortingStyle is set to BackToFront or FrontToBack we will then
  // sort the props that have a non-zero AllocatedRenderTime by their
  // center distance
  distanceList = new float[listLength];

  // We will return the total time of all props. This is used for
  // normalization.
  total_time  = 0;

  ren->GetAspect( aspect );
  // Get the view frustum planes from the active camera
  ren->GetActiveCamera()->GetFrustumPlanes( (aspect[0] / aspect[1]), planes );

  // Keep a list of allocated times to help with sorting / removing
  // props later
  allocatedTimeList = new float[listLength];

  // For each prop, compute coverage
  for ( propLoop = 0; propLoop < listLength; propLoop++ )
    {
    // Get the prop out of the list
    prop = propList[propLoop];

    // If allocated render time has not been initialized yet (if this
    // is the first culler, it hasn't) then the previous time is set
    // to 0.0
    if ( !initialized )
      {
      previous_time = 1.0;
      }
    else
      {
      previous_time = prop->GetRenderTimeMultiplier();
      }

    // Get the bounds of the prop and compute an enclosing sphere
    bounds = prop->GetBounds();

    // We start with a coverage of 1.0 and set it to zero if the prop
    // is culled during the plane tests
    coverage = 1.0;
    // make sure the bounds are defined - they won't be for a 2D prop which
    // means that they will never be culled. Maybe this should be changed in
    // the future?
    if (bounds)
      {
      // a duff dataset like a polydata with no cells will have bad bounds
      if ((bounds[0] == -VTK_LARGE_FLOAT) || (bounds[0] == VTK_LARGE_FLOAT))
        {
	      coverage = 0.0;
	      i = 7;
        }
      else
        {
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
        for ( i = 0; i < 6; i++ )
          {
          // Compute how far the center of the sphere is from this plane
          d =
          planes[i*4 + 0] * center[0] +
          planes[i*4 + 1] * center[1] +
          planes[i*4 + 2] * center[2] +
          planes[i*4 + 3];
          // If d < -radius the prop is not within the view frustum
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
          {
            screen_bounds[i] = d - radius;
          }
	      // The fifth plane is the near plane - use the distance to
        // the center (d) as the value to sort by
	      if ( i == 4 )
	        {
	        distanceList[propLoop] = d;
	        }
        }
      }
      // If the prop wasn't culled during the plane tests...
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
        if ( screen_bounds[0] > 0.0 )
          {
          part_w -= screen_bounds[0];
          }
        if ( screen_bounds[1] > 0.0 )
          {
          part_w -= screen_bounds[1];
          }
        // Do the same thing for the height with the top and bottom
        // planes (2,3).
        part_h = full_h;
        if ( screen_bounds[2] > 0.0 )
          {
          part_h -= screen_bounds[2];
          }
        if ( screen_bounds[3] > 0.0 )
          {
          part_h -= screen_bounds[3];
          }

        // Compute the fraction of coverage
        coverage = (part_w * part_h) / (full_w * full_h);
        // Convert this to an allocated render time - coverage less than
        // the minumum result in 0.0 time, greater than the maximum result in
        // 1.0 time, and in between a linear ramp is used
        if ( coverage < this->MinimumCoverage )
          {
          coverage = 0;
          }
        else if ( coverage > this->MaximumCoverage )
          {
          coverage = 1.0;
          }
        else
          {
          coverage = (coverage-this->MinimumCoverage) /
          this->MaximumCoverage;
          }
        }
      }
    // This is a 2D prop - keep them at the beginning of the list in the same
    // order they came in (by giving them all the same distance) and set
    // the coverage to something small so that they won't get much
    // allocated render time (because they aren't LOD it doesn't matter,
    // and they generally do draw fast so you don't want to take too much
    // time away from the 3D prop because you added a title to your
    // window for example) They are put at the beginning of the list so
    // that when sorted back to front they will be rendered last.
    else
      {
      distanceList[propLoop] = -VTK_LARGE_FLOAT;
      coverage = 0.001;
      }
    // Multiply the new allocated time by the previous allocated time
    coverage *= previous_time;
    prop->SetRenderTimeMultiplier( coverage );

    // Save this in our array of allocated times which matches the
    // prop array. Also save the center distance
    allocatedTimeList[propLoop] = coverage;

    // Add the time for this prop to the total time
    total_time += coverage;
    }

  // Now traverse the list from the beginning, swapping any zero entries back
  // in the list, while preserving the order of the non-zero entries. This
  // requires two indices for the two items we are comparing at any step.
  // The second index always moves back by one, but the first index moves back
  // by one only when it is pointing to something that has a non-zero value.
  index1 = 0;
  for ( index2 = 1; index2 < listLength; index2++ )
    {
    if ( allocatedTimeList[index1] == 0.0 )
      {
      if ( allocatedTimeList[index2] != 0.0 )
        {
        allocatedTimeList[index1] = allocatedTimeList[index2];
        distanceList[index1]      = distanceList[index2];
        propList[index1]          = propList[index2];
        propList[index2]          = NULL;
        allocatedTimeList[index2] = 0.0;
        distanceList[index2]      = 0.0;
        }
      else
        {
        propList[index1]          = propList[index2]           = NULL;
        allocatedTimeList[index1] = allocatedTimeList[index2]  = 0.0;
        distanceList[index1]      = distanceList[index2]       = 0.0;
        }
      }
    if ( allocatedTimeList[index1] != 0.0 )
      {
      index1++;
      }
    }

  // Compute the new list length - index1 is always pointing to the
  // first 0.0 entry or the last entry if none were zero (in which case
  // we won't change the list length)
  listLength = (allocatedTimeList[index1] == 0.0)?(index1):listLength;

  // Now reorder the list if sorting is on
  // Do it by a simple bubble sort - there probably aren't that
  // many props....

  if ( this->SortingStyle == VTK_CULLER_SORT_FRONT_TO_BACK )
    {
    for ( propLoop = 1; propLoop < listLength; propLoop++ )
      {
      index1 = propLoop;
      while ( (index1 - 1) >= 0 &&
        distanceList[index1] < distanceList[index1-1] )
        {
        tmp = distanceList[index1-1];
        distanceList[index1-1] = distanceList[index1];
        distanceList[index1] = tmp;
        prop = propList[index1-1];
        propList[index1-1] = propList[index1];
        propList[index1] = prop;
        index1--;
        }
      }
    }
  if ( this->SortingStyle == VTK_CULLER_SORT_BACK_TO_FRONT )
    {
    for ( propLoop = 1; propLoop < listLength; propLoop++ )
      {
      index1 = propLoop;
      while ( (index1 - 1) >= 0 &&
        distanceList[index1] > distanceList[index1-1] )
        {
        tmp = distanceList[index1-1];
        distanceList[index1-1] = distanceList[index1];
        distanceList[index1] = tmp;
        prop = propList[index1-1];
        propList[index1-1] = propList[index1];
        propList[index1] = prop;
        index1--;
        }
      }
    }
  // The allocated render times are now initialized
  initialized = 1;
  delete [] allocatedTimeList;
  delete [] distanceList;
  return total_time;
}

// Description:
// Return the sorting style as a descriptive character string.
const char *vtkFrustumCoverageCuller::GetSortingStyleAsString(void)
{
  if( this->SortingStyle == VTK_CULLER_SORT_NONE )
    {
    return "None";
    }
  if( this->SortingStyle == VTK_CULLER_SORT_FRONT_TO_BACK )
    {
    return "Front To Back";
    }
  if( this->SortingStyle == VTK_CULLER_SORT_BACK_TO_FRONT )
    {
    return "Back To Front";
    }
  else
    {
    return "Unknown";
    }
}

void vtkFrustumCoverageCuller::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkCuller::PrintSelf(os,indent);

  os << indent << "Minimum Coverage: " 
     << this->MinimumCoverage << endl;

  os << indent << "Maximum Coverage: " 
     << this->MaximumCoverage << endl;

  os << indent << "Sorting Style: "
     << this->GetSortingStyleAsString() << endl;

}
