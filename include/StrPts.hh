/*=========================================================================

  Program:   Visualization Library
  Module:    StrPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPoints - topologically and geometrically regular array of data
// .SECTION Description
// vlStructuredPoints is a data object that is a concrete implementation of
// vlDataSet. vlStructuredPoints represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vlStructuredPoints_h
#define __vlStructuredPoints_h

#include "DataSet.hh"
#include "StrData.hh"

class vlStructuredPoints : public vlDataSet, public vlStructuredData 
{
public:
  vlStructuredPoints();
  vlStructuredPoints(const vlStructuredPoints& v);
  ~vlStructuredPoints();
  char *GetClassName() {return "vlStructuredPoints";};
  char *GetDataType() {return "vlStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  unsigned long GetMtime();

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredPoints(*this);};
  int GetNumberOfCells();
  int GetNumberOfPoints();
  float *GetPoint(int ptId);
  void GetPoint(int id, float x[3]);
  vlCell *GetCell(int cellId);
  int FindCell(float x[3], vlCell *cell, float tol2, int& subId, 
               float pcoords[3], float weights[MAX_CELL_SIZE]);
  int GetCellType(int cellId);
  void GetCellPoints(int cellId, vlIdList& ptIds);
  void GetPointCells(int ptId, vlIdList& cellIds);
  void ComputeBounds();

  void GetVoxelGradient(int i, int j, int k, vlScalars *s, vlFloatVectors& g);
  void GetPointGradient(int i, int j, int k, vlScalars *s, float g[3]);

  // Description:
  // Set the aspect ratio of the cubical cells that compose the structured
  // point set.
  vlSetVector3Macro(AspectRatio,float);
  vlGetVectorMacro(AspectRatio,float,3);

  // Description:
  // Set the origin of the data. The origin plus aspect ratio determine the
  // position in space of the structured points.
  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float,3);

protected:
  void Initialize();

  float Origin[3];
  float AspectRatio[3];
};

inline void vlStructuredPoints::GetPoint(int id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline int vlStructuredPoints::GetNumberOfCells() 
{
  return this->vlStructuredData::_GetNumberOfCells();
}

inline int vlStructuredPoints::GetNumberOfPoints()
{
  return this->vlStructuredData::_GetNumberOfPoints();
}

inline void vlStructuredPoints::GetCellPoints(int cellId, vlIdList& ptIds)
{
  this->vlStructuredData::_GetCellPoints(cellId, ptIds);
}

inline void vlStructuredPoints::GetPointCells(int ptId, vlIdList& cellIds)
{
  this->vlStructuredData::_GetPointCells(ptId, cellIds);
}

#endif
