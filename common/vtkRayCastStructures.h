/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCastStructures.h
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

// .NAME vtkRayCastStructures - the structure definitions for ray casting

// .SECTION Description
// These are the structures required for ray casting.

// .SECTION See Also
// vtkRayCaster

#ifndef __vtkRayCastStructures_h
#define __vtkRayCastStructures_h

typedef struct 
{
  // These are the input values that define the ray
  float Origin[3];
  float Direction[3];
  int   Pixel[2];

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
