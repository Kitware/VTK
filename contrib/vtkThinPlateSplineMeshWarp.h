/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineMeshWarp.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton (MINORI Project, Dental and Medical
             Informatics, Eastman Dental Institute, London, UK) who
             developed and contributed this class.

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
// .NAME vtkThinPlateSplineMeshWarp - warps polygonal meshes using landmarks
// .SECTION Description
// vtkThinPlateSplineMeshWarp warps a polygonal mesh into a different shape
// using two sets of landmarks (vtkPoints). Any point on the mesh close to a
// source landmark will be moved to a place close to the corresponding target
// landmark. The points in between are interpolated smoothly using
// Bookstein's Thin Plate Spline algorithm.
//
// The filter takes three inputs: the polygonal mesh to be warped (use
// SetInput), the source landmarks (SetSourceLandmarks) and the target
// Landmarks (SetTargetLandmarks).  There is one parameter (Sigma) that
// controls the 'stiffness' of the spline (default is 1.0).
//
// The topology of the mesh is not altered, only the geometry (the location
// of the points).

#ifndef __vtkThinPlateSplineMeshWarp_h
#define __vtkThinPlateSplineMeshWarp_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkThinPlateSplineMeshWarp : public vtkPolyDataToPolyDataFilter
{
public:
  const char *GetClassName() { return "vtkThinPlateSplineMeshWarp"; }
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with Sigma=1.0
  static vtkThinPlateSplineMeshWarp* New();

  // Description: 
  // Specify the 'stiffness' of the spline. The default of 1.0
  // should usually be fine.
  vtkGetMacro(Sigma,float);
  vtkSetMacro(Sigma,float);

  // Description:
  // Set the source landmarks for the warp.
  vtkSetObjectMacro(SourceLandmarks,vtkPoints);
  vtkGetObjectMacro(SourceLandmarks,vtkPoints);

  // Description:
  // Set the target landmarks for the warp
  vtkSetObjectMacro(TargetLandmarks,vtkPoints);
  vtkGetObjectMacro(TargetLandmarks,vtkPoints);

protected:
  vtkThinPlateSplineMeshWarp();
  ~vtkThinPlateSplineMeshWarp();

  void Execute();

  float Sigma;
  vtkPoints *SourceLandmarks;
  vtkPoints *TargetLandmarks;
};

#endif
