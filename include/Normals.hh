/*=========================================================================

  Program:   Visualization Library
  Module:    Normals.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlNormals - abstract interface to 3D normals
// .SECTION Description
// vlNormals provides an abstract interface to 3D normals. The data model
// for vlNormals is an array of nx-ny-nz triplets accessible by point id.
// (Each normal is assumed normalized |n| = 1). The subclasses of 
// vlNormals are concrete data types (float, int, etc.) that implement 
// the interface of vlNormals. 

#ifndef __vlNormals_h
#define __vlNormals_h

#include "RefCount.hh"

class vlIdList;
class vlFloatNormals;

class vlNormals : public vlRefCount 
{
public:
  vlNormals() {};
  virtual ~vlNormals() {};
  char *GetClassName() {return "vlNormals";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vlNormals *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of normals in array.
  virtual int GetNumberOfNormals() = 0;

  // Description:
  // Return a float normal n[3] for a particular point id.
  virtual float *GetNormal(int id) = 0;

  // Description:
  // Copy normal components into user provided array n[3] for specified
  // point id.
  virtual void GetNormal(int id, float n[3]);

  // Description:
  // Insert normal into object. No range checking performed (fast!).
  virtual void SetNormal(int id, float n[3]) = 0;

  // Description:
  // Insert normal into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertNormal(int id, float n[3]) = 0;

  // Description:
  // Insert normal into next available slot. Returns point id of slot.
  virtual int InsertNextNormal(float n[3]) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetNormals(vlIdList& ptId, vlFloatNormals& fp);
};

// These include files are placed here so that if Normals.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "FNormals.hh"

#endif
