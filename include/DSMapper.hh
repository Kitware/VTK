/*=========================================================================

  Program:   Visualization Library
  Module:    DSMapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetMapper - map vlDataSet and derived classes to graphics primitives
// .SECTION Description
// vlDataSetMapper is a mapper to map data sets (i.e., vlDataSet and 
// all derived classes) to graphics primitives. The mapping procedure
// is as follows: all 0-D, 1-D, and 2-D cells are converted into points,
// lines, and polygons/triangle strips and mapped. The 2-D faces of
// 3-D cells are mapped only if they are used by only one cell, i.e.,
// on the boundary of the data set.

#ifndef __vlDataSetMapper_h
#define __vlDataSetMapper_h

#include "GeomF.hh"
#include "PolyMap.hh"
#include "DataSet.hh"
#include "Renderer.hh"

class vlDataSetMapper : public vlMapper 
{
public:
  vlDataSetMapper();
  ~vlDataSetMapper();
  char *GetClassName() {return "vlDataSetMapper";};
  void PrintSelf(ostream& os, vlIndent indent);
  void Render(vlRenderer *ren);
  float *GetBounds();

  // Description:
  // Specify the input data to map.
  virtual void SetInput(vlDataSet *in);
  void SetInput(vlDataSet& in) {this->SetInput(&in);};
  virtual vlDataSet* GetInput();

protected:
  vlDataSet *Input;
  vlGeometryFilter *GeometryExtractor;
  vlPolyMapper *PolyMapper;
};

#endif


