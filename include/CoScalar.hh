/*=========================================================================

  Program:   Visualization Library
  Module:    CoScalar.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlColorScalars - abstract class represents scalar data in color specification
// .SECTION Description
// vlColorScalars is an abstract class whose subclasses represent scalar
// data using a color specification such as rgb, grayscale, rgba, hsv, etc. 
//    In order to be a vlScalar subclass, vlColorScalars must be able to 
// return a single value given a point id. By default, this operation is 
// performed by computing intensity as the single value. Concrete subclasses 
// of vlColorScalars may have additional  methods to convert multi-dimensional
// color information into a single scalar value.
// .SECTION Caveats
// Derived classes of vlColorScalars loosely interpret the meaning of "rgb".
// For example, when inserting a grayscale value, the "r" value is interpreted
// as the gray value. However, when getting the color, each of "r", "g", and 
// "b" is set to the gray value. Thus when getting color, assume that the rgb
// value is correct.
//    The derived class vlAPixmap must be used with caution. It operates on 
// rgba[4] data. If you might be using this class, make sure that the array
// rgb is dimensioned to [4]  to avoid memory corruption.

#ifndef __vlColorScalars_h
#define __vlColorScalars_h

#include "Scalars.hh"

class vlPixmap;

class vlColorScalars : public vlScalars 
{
public:
  vlColorScalars() {};
  ~vlColorScalars() {};
  char *GetClassName() {return "vlColorScalars";};

  // vlScalars interface
  float GetScalar(int i);
  void SetScalar(int i, float s);
  void InsertScalar(int i, float s);
  int InsertNextScalar(float s);

  // abstract interface for vlColorScalars
  // Description:
  // Return number of colors (same as number of scalars).
  int GetNumberOfColors() {return this->GetNumberOfScalars();};  

  // Description:
  // Return a unsigned char rgb for a particular point id.
  virtual unsigned char *GetColor(int id) = 0;

  // Description:
  // Copy rgb components into user provided array rgb[3] for specified
  // point id.
  virtual void GetColor(int id, unsigned char rgb[3]) = 0;

  // Description:
  // Insert color into object. No range checking performed (fast!).
  virtual void SetColor(int id, unsigned char rgb[3]) = 0;

  // Description:
  // Insert color into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertColor(int id, unsigned char rgb[3]) = 0;

  // Description:
  // Insert color into next available slot. Returns point id of slot.
  virtual int InsertNextColor(unsigned char rgb[3]) = 0;

  void GetColors(vlIdList& ptId, vlPixmap& p);

};

// These include files are placed here so that if CoScalar.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "Pixmap.hh"

#endif
