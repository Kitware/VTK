/*=========================================================================

  Program:   Visualization Library
  Module:    PtData.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Class for manipulating data associated with points
//
#ifndef __vlPointData_h
#define __vlPointData_h

#include "Object.hh"
#include "Scalars.hh"
#include "Vectors.hh"
#include "Normals.hh"
#include "TCoords.hh"

class vlPointData : public vlObject 
{
public:
  vlPointData() : Scalars(0), Vectors(0), Normals(0), TCoords(0) {};
  void Initialize();
  ~vlPointData();
  char *GetClassName() {return "vlPointData";};
  void PrintSelf(ostream& os, vlIndent indent);
  vlPointData (const vlPointData& pd);
  vlPointData &operator=(vlPointData& pd);
  virtual void Update() {};

  // use to copy data
  void CopyInitialize(vlPointData* pd, int sze=0, int ext=1000, int sFlg=1, int vFlg=1, int nFlg=1, int tFlg=1);
  void CopyData(vlPointData *fromPd, int fromId, int toId);

  // use to interpolate data
  void InterpolateInitialize(vlPointData* pd, int sze=0, int ext=1000, int sFlg=1, int vFlg=1, int nFlg=1, int tFlg=1);
  void InterpolatePoint(vlPointData *fromPd, int toId, vlIdList *ptIds, float *weights);

  // Set point data to null values
  void NullPoint(int ptId);

  vlSetObjectMacro (Scalars, vlScalars);
  vlGetObjectMacro (Scalars, vlScalars);

  vlSetObjectMacro (Vectors, vlVectors);
  vlGetObjectMacro (Vectors, vlVectors);

  vlSetObjectMacro (Normals, vlNormals);
  vlGetObjectMacro (Normals, vlNormals);

  vlSetObjectMacro (TCoords, vlTCoords);
  vlGetObjectMacro (TCoords, vlTCoords);

protected:
  vlScalars *Scalars;
  vlVectors *Vectors;
  vlNormals *Normals;
  vlTCoords *TCoords;
  int CopyScalars;
  int CopyVectors;
  int CopyNormals;
  int CopyTCoords;
};

#endif


