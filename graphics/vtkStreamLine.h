/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamLine.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStreamLine - generate streamline in arbitrary dataset
// .SECTION Description
// vtkStreamLine is a filter that generates a streamline for an arbitrary 
// dataset. A streamline is a line that is everywhere tangent to the vector
// field. Scalar values also are calculated along the streamline and can be 
// used to color the line. Streamlines are calculated by integrating from
// a starting point through the vector field. Integration can be performed
// forward in time (see where the line goes), backward in time (see where the
// line came from), or in both directions. It also is possible to compute
// vorticity along the streamline. Vorticity is the projection (i.e., dot
// product) of the flow rotation on the velocity vector, i.e., the rotation
// of flow around the streamline.
//
// vtkStreamLine defines the instance variable StepLength. This parameter 
// controls the length of the line segments used to define the streamline.
// The streamline(s) will consist of one (or more) polylines with line
// segment lengths of size StepLength. Smaller values reduce in more line 
// primitives but smoother streamlines. The StepLength instance variable is 
// defined in terms of time (i.e., the distance that the particle travels in
// the specified time period). Thus, the line segments will be smaller in areas
// of low velocity and larger in regions of high velocity. (NOTE: This is
// different than the IntegrationStepLength defined by the superclass
// vtkStreamer. IntegrationStepLength is used to control integration step 
// size and is expressed as a fraction of the cell length.) The StepLength
// instance variable is important because subclasses of vtkStreamLine (e.g.,
// vtkDashedStreamLine) depend on this value to build their representation.
// .SECTION See Also
// vtkStreamer vtkDashedStreamLine vtkStreamPoints

#ifndef __vtkStreamLine_h
#define __vtkStreamLine_h

#include "vtkStreamer.hh"

class vtkStreamLine : public vtkStreamer
{
public:
  vtkStreamLine();
  char *GetClassName() {return "vtkStreamLine";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the length of a line segment. The length is expressed in terms of
  // elapsed time. Smaller values result in smoother appearing streamlines, but
  // greater numbers of line primitives.
  vtkSetClampMacro(StepLength,float,0.000001,VTK_LARGE_FLOAT);
  vtkGetMacro(StepLength,float);

protected:
  // Convert streamer array into vtkPolyData
  void Execute();

  // the length of line primitives
  float StepLength;

};

#endif


