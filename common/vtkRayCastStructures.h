/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCastStructures.h
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

// .NAME vtkRayCastStructures - the structure definitions for ray casting

// .SECTION Description
// These are the structures required for ray casting.

// .SECTION See Also
// vtkRayCaster

#ifndef __vtkRayCastStructures_h
#define __vtkRayCastStructures_h

typedef struct 
{
  // These are the input values that define the ray. Depending on
  // whether we are casting a WorldRay or a ViewRay, these are in
  // world coordinates or view coordinates.
  float Origin[3];
  float Direction[3];

  // The pixel location for the ray that is being cast can be
  // important, for example if hardware ray bounding is being used
  // and the location in the depth buffer must be matched to this
  // ray.
  int   Pixel[2];

  // The world coordinate location of the camera is important for the
  // ray caster to be able to return a Z value for the intersection
  float CameraPosition[3];

  // This input value defines the size of the image
  int   ImageSize[2];

  // These are input values for clipping but may be changed
  // along the way
  float NearClip;
  float FarClip;

  // These are the return values - RGBA and Z
  float Color[4];
  float Depth;


  // Some additional space that may be useful for the
  // specific implementation of the ray caster. This structure
  // is a convenient place to put it, since there is one
  // per thread so that writing to these locations is safe

  // Ray information transformed into local coordinates
  float                        TransformedStart[4];
  float                        TransformedEnd[4];
  float                        TransformedDirection[4];
  float                        TransformedIncrement[3];
  
  // The number of steps we want to take if this is
  // a ray caster that takes steps
  int                          NumberOfStepsToTake;
  
  // The number of steps we actually take if this is
  // a ray caster that takes steps
  int                          NumberOfStepsTaken;

} VTKRayCastRayInfo;

#endif
