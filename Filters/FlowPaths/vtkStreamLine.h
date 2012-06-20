/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
// controls the time increment used to generate individual points along
// the streamline(s). Smaller values result in more line
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

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkStreamer.h"

class VTKFILTERSFLOWPATHS_EXPORT vtkStreamLine : public vtkStreamer
{
public:
  vtkTypeMacro(vtkStreamLine,vtkStreamer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with step size set to 1.0.
  static vtkStreamLine *New();

  // Description:
  // Specify the length of a line segment. The length is expressed in terms of
  // elapsed time. Smaller values result in smoother appearing streamlines, but
  // greater numbers of line primitives.
  vtkSetClampMacro(StepLength,double,0.000001,VTK_DOUBLE_MAX);
  vtkGetMacro(StepLength,double);

protected:
  vtkStreamLine();
  ~vtkStreamLine() {};

  // Convert streamer array into vtkPolyData
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // the length of line primitives
  double StepLength;

private:
  vtkStreamLine(const vtkStreamLine&);  // Not implemented.
  void operator=(const vtkStreamLine&);  // Not implemented.
};

#endif
