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
// .NAME vlScalars - abstract interface to array of scalar data
// .SECTION Description
// vlScalars provides an abstract interface to an array of scalar data. 
// The data model for vlScalars is an array accessible by point id.
// The subclasses of vlScalars are concrete data types (float, int, etc.) 
// that implement the interface of vlScalars.
//    Scalars typically provide a single value per point. However, there are
// types of scalars that have multiple values per point (e.g., vlPixmap or
// vlAPixmap that provide three and four values per point, respectively).
// These are used when reading data in rgb and rgba form (e.g., images 
// and volumes).
//    Because of the close relationship between scalars and colors, scalars 
// also maintain in internal lookup table. If provided, this table is used 
// to map scalars into colors, rather than the lookup table that the vlMapper
// objects are associated with.

#ifndef __vlScalars_h
#define __vlScalars_h

#include "RefCount.hh"

class vlIdList;
class vlFloatScalars;
class vlLookupTable;

class vlScalars : public vlRefCount 
{
public:
  vlScalars();
  virtual ~vlScalars();
  char *GetClassName() {return "vlScalars";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vlScalars *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return number of points in array.
  virtual int GetNumberOfScalars() = 0;

  // Description:
  // Return the number of values per point. Should range between (1,4).
  // Used to distinguish between color scalars and single-valued ones.
  virtual int GetNumberOfValuesPerPoint() {return 1;};

  // Description:
  // Return a float scalar value for a particular point id.
  virtual float GetScalar(int id) = 0;

  // Description:
  // Insert scalar into array. No range checking performed (fast!).
  virtual void SetScalar(int id, float s) = 0;

  // Description:
  // Insert scalar into array. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertScalar(int id, float s) = 0;

  // Description:
  // Insert scalar into next available slot. Returns point id of slot.
  virtual int InsertNextScalar(float s) = 0;

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  // Description:
  // Get data as pointer to unsigned char. Used for high performance texture
  // and color manipulation. Not supported for all scalar types: check for
  // NULL return value. 
  virtual unsigned char *GetUCharPtr() {return NULL;};

  void GetScalars(vlIdList& ptId, vlFloatScalars& fs);
  virtual void ComputeRange();
  float *GetRange();
  void GetRange(float range[8]);

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  virtual void CreateDefaultLookupTable();

  void SetLookupTable(vlLookupTable *lut);
  vlGetObjectMacro(LookupTable,vlLookupTable);

protected:
  float Range[8];
  vlTimeStamp ComputeTime; // Time at which range computed
  vlLookupTable *LookupTable;
};

// These include files are placed here so that if Scalars.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "FScalars.hh"

#endif
