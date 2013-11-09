/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeCenters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEdgeCenters - generate points at center of edges
// .SECTION Description
// vtkEdgeCenters is a filter that takes as input any graph and
// generates on output points at the center of the cells in the dataset.
// These points can be used for placing glyphs (vtkGlyph3D) or labeling
// (vtkLabeledDataMapper). (The center is the parametric center of the
// cell, not necessarily the geometric or bounding box center.) The edge
// attributes will be associated with the points on output.
//
// .SECTION Caveats
// You can choose to generate just points or points and vertex cells.
// Vertex cells are drawn during rendering; points are not. Use the ivar
// VertexCells to generate cells.

// .SECTION See Also
// vtkGlyph3D vtkLabeledDataMapper

#ifndef __vtkEdgeCenters_h
#define __vtkEdgeCenters_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkEdgeCenters : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkEdgeCenters,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with vertex cell generation turned off.
  static vtkEdgeCenters *New();

  // Description:
  // Enable/disable the generation of vertex cells.
  vtkSetMacro(VertexCells,int);
  vtkGetMacro(VertexCells,int);
  vtkBooleanMacro(VertexCells,int);

protected:
  vtkEdgeCenters();
  ~vtkEdgeCenters() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int VertexCells;
private:
  vtkEdgeCenters(const vtkEdgeCenters&);  // Not implemented.
  void operator=(const vtkEdgeCenters&);  // Not implemented.
};

#endif
