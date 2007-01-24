/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraphToPolyData - convert a vtkGraph to vtkPolyData
//
// .SECTION Description
// Converts a vtkGraph to a vtkPolyData.  This assumes that the points
// of the graph have already been filled (perhaps by vtkGraphLayout),
// and coverts all the arc of the graph into lines in the polydata.
// The node data is passed along to the point data, and the arc data
// is passed along to the cell data.
//
// Only the owned graph arcs (i.e. arcs with ghost level 0) are copied
// into the vtkPolyData.
//
// The algorithm may also produce arrows ont he arcs to show direction
// by setting DrawArrows to true.  The cells representing the arrows
// are placed in a separate output polydata (output port 1).

#ifndef __vtkGraphToPolyData_h
#define __vtkGraphToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkGraphToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkGraphToPolyData *New();
  vtkTypeRevisionMacro(vtkGraphToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Whether to draw arrows on the arcs.  Default is off.
  // The cells representing the arrows are placed in a vtkPolyData in output port 1.
  vtkGetMacro(DrawArrows, bool);
  vtkSetMacro(DrawArrows, bool);
  vtkBooleanMacro(DrawArrows, bool);

  // Description:
  // The position of the arrow along the arc.  0.0 is at the source, 1.0 is at the target.
  // Default is 0.6.
  vtkGetMacro(ArrowPosition, double);
  vtkSetClampMacro(ArrowPosition, double, 0.0, 1.0);

  // Description:
  // The size of the arrows.  Default is 0.1;
  vtkGetMacro(ArrowSize, double);
  vtkSetClampMacro(ArrowSize, double, 0.0, VTK_DOUBLE_MAX);

  // Description:
  // The arrow angle in degrees, from 0 to 180.  Default is 45.
  vtkGetMacro(ArrowAngle, double);
  vtkSetClampMacro(ArrowAngle, double, 0.0, 180.0);

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkGraphToPolyData();
  ~vtkGraphToPolyData() {}

  bool DrawArrows;
  double ArrowPosition;
  double ArrowSize;
  double ArrowAngle;

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkGraphToPolyData(const vtkGraphToPolyData&);  // Not implemented.
  void operator=(const vtkGraphToPolyData&);  // Not implemented.
};

#endif
