/*=========================================================================

  Program:   Visualization Library
  Module:    Vectors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPoints - abstract interface to 3D vectors
// .SECTION Description
// vlVectors provides an abstract interface to 3D vectors. The data model
// for vlVectors is an array of vx-vy-vz triplets accessible by point id.
// The subclasses of vlVectors are concrete data types (float, int, etc.)
// that implement the interface of vlVectors.

#ifndef __vlVectors_h
#define __vlVectors_h

#include "RefCount.hh"

class vlIdList;
class vlFloatVectors;

class vlVectors : public vlRefCount 
{
public:
  vlVectors();
  virtual ~vlVectors() {};
  char *GetClassName() {return "vlVectors";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vlVectors *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return number of vectors in array.
  virtual int GetNumberOfVectors() = 0;

  // Description:
  // Return a pointer to a float vector v[3] for a specific point id.
  virtual float *GetVector(int id) = 0;

  // Description:
  // Copy vector componenets into user provided array v[3] for specified
  // point id.
  virtual void GetVector(int id, float v[3]);

  // Description:
  // Insert vector into object. No range checking performed (fast!).
  virtual void SetVector(int id, float v[3]) = 0;

  // Description:
  // Insert vector into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertVector(int id, float v[3]) = 0;

  // Description:
  // Insert vector into next available slot. Returns point id of slot.
  virtual int InsertNextVector(float v[3]) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetVectors(vlIdList& ptId, vlFloatVectors& fp);
  virtual void ComputeMaxNorm();
  float GetMaxNorm();

protected:
  float MaxNorm;
  vlTimeStamp ComputeTime; // Time at which MaxNorm computed
};

#endif
