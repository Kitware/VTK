/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DSMapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetMapper - map vtkDataSet and derived classes to graphics primitives
// .SECTION Description
// vtkDataSetMapper is a mapper to map data sets (i.e., vtkDataSet and 
// all derived classes) to graphics primitives. The mapping procedure
// is as follows: all 0-D, 1-D, and 2-D cells are converted into points,
// lines, and polygons/triangle strips and mapped. The 2-D faces of
// 3-D cells are mapped only if they are used by only one cell, i.e.,
// on the boundary of the data set.

#ifndef __vtkDataSetMapper_h
#define __vtkDataSetMapper_h

#include "GeomF.hh"
#include "PolyMap.hh"
#include "Renderer.hh"

class vtkDataSetMapper : public vtkMapper 
{
public:
  vtkDataSetMapper();
  ~vtkDataSetMapper();
  char *GetClassName() {return "vtkDataSetMapper";};
  void PrintSelf(ostream& os, vtkIndent indent);
  void Render(vtkRenderer *ren);
  float *GetBounds();

  // Description:
  // Specify the input data to map.
  void SetInput(vtkDataSet *in);
  void SetInput(vtkDataSet& in) {this->SetInput(&in);};

protected:
  vtkGeometryFilter *GeometryExtractor;
  vtkPolyMapper *PolyMapper;
};

#endif


