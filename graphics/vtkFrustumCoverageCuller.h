
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumCoverageCuller.h
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
// .NAME vtkFrustumCoverageCuller - cull actors based on frustum coverage
// .SECTION Description
// vtkFrustumCoverageCuller will cull actors based on the coverage in
// the view frustum. The coverage is computed by enclosing the actor in
// a bounding sphere, projecting that to the viewing coordinate system, then
// taking a slice through the view frustum at the center of the sphere. This
// results in a circle on the plane slice through the view frustum. This
// circle is enclosed in a squared, and the fraction of the plane slice that
// this square covers is the coverage. This is a number between 0 and 1.
// If the number is less than the MinumumCoverage, the allocated render time
// for that actor is set to zero. If it is greater than the MaximumCoverage,
// the allocated render time is set to 1.0. In between, a linear ramp is used
// to convert coverage into allocated render time.


// .SECTION see also
// vtkCuller

#ifndef __vtkFrustumCoverageCuller_h
#define __vtkFrustumCoverageCuller_h

#include "vtkCuller.h"

class vtkActor;
class vtkRenderer;

class VTK_EXPORT vtkFrustumCoverageCuller : public vtkCuller
{
public:

// Description:
// Create a frustum coverage culler with default values
  vtkFrustumCoverageCuller();

  static vtkFrustumCoverageCuller *New() {return new vtkFrustumCoverageCuller;};
  const char *GetClassName() {return "vtkFrustumCoverageCuller";};
  void PrintSelf(ostream& os,vtkIndent indent);

  // Description:
  // This class implements an outer cull method
  float OuterCullMethod( vtkRenderer *ren, vtkActor **actorList,
			 int& listLength, int& initialized );

  // Description:
  // Every actor passes the inner cull method (the actor is not culled)
  int   InnerCullMethod( vtkRenderer *, vtkActor *) { return 1; };

  
  // Description:
  // Set/Get the minimum coverage - actors with less coverage than this
  // are given no time to render (they are culled)
  vtkSetMacro( MinimumCoverage, float );
  vtkGetMacro( MinimumCoverage, float );

  // Description:
  // Set/Get the maximum coverage - actors with more coverage than this are
  // given an allocated render time of 1.0 (the maximum)
  vtkSetMacro( MaximumCoverage, float );
  vtkGetMacro( MaximumCoverage, float );

protected:

  float        MinimumCoverage;
  float        MaximumCoverage;
};
                                         
#endif
