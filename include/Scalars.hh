/*=========================================================================

  Program:   Visualization Library
  Module:    Scalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlScalars - abstract interface to scalar array
// .SECTION Description
// vlScalars provides an abstract interface to an array of scalar data. 
// The data model for vlScalars is an array accessible by point id.
// The subclasses of vlScalars are concrete data types (float, int, etc.) 
// that implement the interface of vlScalars.

#ifndef __vlScalars_h
#define __vlScalars_h

#include "RefCount.hh"

class vlIdList;
class vlFloatScalars;

class vlScalars : public vlRefCount 
{
public:
  vlScalars();
  virtual ~vlScalars() {};
  char *GetClassName() {return "vlScalars";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vlScalars *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return number of points in array.
  virtual int GetNumberOfScalars() = 0;

  // Description:
  // Return a float scalar value for a particular point id.
  virtual float GetScalar(int i) = 0;

  // Description:
  // Insert scalar into array. No range checking performed (fast!).
  virtual void SetScalar(int i, float s) = 0;

  // Description:
  // Insert scalar into array. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertScalar(int i, float s) = 0;

  // Description:
  // Insert scalar into next available slot. Returns point id of slot.
  virtual int InsertNextScalar(float s) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetScalars(vlIdList& ptId, vlFloatScalars& fs);
  virtual void ComputeRange();
  float *GetRange();

protected:
  float Range[2];
  vlTimeStamp ComputeTime; // Time at which range computed
};

#endif
