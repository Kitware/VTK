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
// .NAME vlPointData - represent and manipulate point attribute data
// .SECTION Description
// vlPointData is a class that is used to represent and manipulate
// point attribute data (e.g., scalars, vectors, normals, texture 
// coordinates, etc.) Special methods are provided to work with filter
// objects such as passing data through filter, copying data from one 
// point to another, and interpolating data given shape functions.

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
  vlPointData();
  void Initialize();
  ~vlPointData();
  char *GetClassName() {return "vlPointData";};
  void PrintSelf(ostream& os, vlIndent indent);
  vlPointData (const vlPointData& pd);
  vlPointData &operator=(vlPointData& pd);
  virtual void Update() {};

  // pass thru all input data to output
  void PassData(vlPointData* pd);

  // use to copy data on a point by point basis
  void CopyAllocate(vlPointData* pd, int sze=0, int ext=1000);
  void CopyData(vlPointData *fromPd, int fromId, int toId);

  // use to interpolate data
  void InterpolateAllocate(vlPointData* pd, int sze=0, int ext=1000);
  void InterpolatePoint(vlPointData *fromPd, int toId, vlIdList *ptIds, float *weights);

  // Set point data to null values
  void NullPoint(int ptId);

  // Reclaim memory
  void Squeeze();

  // Description:
  // Set the scalar data.
  vlSetRefCountedObjectMacro (Scalars, vlScalars);
  vlGetObjectMacro (Scalars, vlScalars);

  // Description:
  // Set the vector data.
  vlSetRefCountedObjectMacro (Vectors, vlVectors);
  vlGetObjectMacro (Vectors, vlVectors);

  // Description:
  // Set the normal data.
  vlSetRefCountedObjectMacro (Normals, vlNormals);
  vlGetObjectMacro (Normals, vlNormals);

  // Description:
  // Set the texture coordinate data.
  vlSetRefCountedObjectMacro (TCoords, vlTCoords);
  vlGetObjectMacro (TCoords, vlTCoords);

  // Description:
  // Turn on/off the copying of scalar data.
  vlSetMacro(CopyScalars,int);
  vlGetMacro(CopyScalars,int);
  vlBooleanMacro(CopyScalars,int);

  // Description:
  // Turn on/off the copying of vector data.
  vlSetMacro(CopyVectors,int);
  vlGetMacro(CopyVectors,int);
  vlBooleanMacro(CopyVectors,int);

  // Description:
  // Turn on/off the copying of normals data.
  vlSetMacro(CopyNormals,int);
  vlGetMacro(CopyNormals,int);
  vlBooleanMacro(CopyNormals,int);

  // Description:
  // Turn on/off the copying of texture coordinates data.
  vlSetMacro(CopyTCoords,int);
  vlGetMacro(CopyTCoords,int);
  vlBooleanMacro(CopyTCoords,int);

  void CopyAllOn();
  void CopyAllOff();

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


