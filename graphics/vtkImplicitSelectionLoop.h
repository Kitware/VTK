/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSelectionLoop.h
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
// .NAME vtkImplicitSelectionLoop - implicit function for a selection loop
// .SECTION Description
// vtkImplicitSelectionLoop computes the implicit function value and
// function gradient for a irregular, cylinder-like object whose cross
// section is defined by a set of points forming a loop. The loop need
// not be convex nor its points coplanar. However, the loop must be
// non-self-intersecting when projected onto the plane defined by the
// accumulated cross product around the loop (i.e., the axis of the
// loop). (Alternatively, you can specify the normal to use.)
//
// The following procedure is used to compute the implicit function
// value for a point x. Each point of the loop is first projected onto
// the plane defined by the loop normal. This forms a polygon. Then,
// to evaluate the implicit function value, inside/outside tests are
// used to determine if x is inside the polygon, and the distance to
// the loop boundary is computed (negative values are inside the
// loop).
//
// One example application of this implicit function class is to draw a 
// loop on the surface of a mesh, and use the loop to clip or extract 
// cells from within the loop. Remember, the selection loop is "infinite" 
// in length, you can use a plane (in boolean combination) to cap the extent
// of the selection loop. Another trick is to use a connectivity filter to
// extract the closest region to a given point (i.e., one of the points used
// to define the selection loop).

// .SECTION See Also
// vtkImplicitFunction vtkImplicitBoolean vtkExtractGeometry vtkClipPolyData
// vtkConnectivityFilter vtkPolyDataConnectivityFilter

#ifndef __vtkImplicitSelectionLoop_h
#define __vtkImplicitSelectionLoop_h

#include "vtkImplicitFunction.h"
#include "vtkPolygon.h"

class VTK_EXPORT vtkImplicitSelectionLoop : public vtkImplicitFunction
{
public:
  // Description:
  // Instantiate object with no initial loop.
  vtkImplicitSelectionLoop();
  ~vtkImplicitSelectionLoop();
  static vtkImplicitSelectionLoop *New(){ return new vtkImplicitSelectionLoop;}
  const char *GetClassName() {return "vtkImplicitSelectionLoop";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Satisfy vtkImplicitFunction's API
  // Description:
  // Evaluate selection loop returning a signed distance.
  float EvaluateFunction(float x[3]);
  // Description:
  // Evaluate selection loop returning the gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Set/Get the array of point coordinates defining the loop. There must
  // be at least three points used to define a loop.
  vtkSetReferenceCountedObjectMacro(Loop,vtkPoints);
  vtkGetObjectMacro(Loop,vtkPoints);

  // Description:
  // Turn on/off automatic normal generation. By default, the normal is
  // computed from the accumulated cross product of the edges. You can also
  // specify the normal to use.
  vtkSetMacro(AutomaticNormalGeneration,int);
  vtkGetMacro(AutomaticNormalGeneration,int);
  vtkBooleanMacro(AutomaticNormalGeneration,int);

  // Description:
  // Normal used to determine what is inside and what is outside.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Overload GetMTime() because we depend on Loop
  unsigned long int GetMTime();

protected:
  vtkPoints *Loop;
  float Normal[3];
  int AutomaticNormalGeneration;

private:
  void Initialize();
  vtkPolygon *Polygon;

  float Origin[3];
  float Bounds[6];
  float DeltaX;
  float DeltaY;
  float DeltaZ;

  vtkTimeStamp InitializationTime;

};

#endif


