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
// of vlColorScalars may have additional methods to convert multi-dimensional
// color information into a single scalar value.
// .SECTION Caveats
// Derived classes of vlColorScalars treat colors differently. All derived 
// classes will return a rgba (red-green-blue-alpha transparency) array in
// response to "GetColor()" methods. However, when setting colors, the rgba
// data may be converted to internal form. For example, a vlGrayMap just
// takes the maximum component of rgb and uses that as its gray value.

#ifndef __vlColorScalars_h
#define __vlColorScalars_h

#include "Scalars.hh"

class vlAPixmap;

class vlColorScalars : public vlScalars 
{
public:
  vlColorScalars() {};
  ~vlColorScalars() {};
  char *GetClassName() {return "vlColorScalars";};

  // vlScalars interface
  char *GetDataType() {return "char";};
  float GetScalar(int i);
  void SetScalar(int i, float s);
  void InsertScalar(int i, float s);
  int InsertNextScalar(float s);

  // abstract interface for vlColorScalars
  // Description:
  // Return number of colors (same as number of scalars).
  int GetNumberOfColors() {return this->GetNumberOfScalars();};  

  // Description:
  // Return a unsigned char rgba for a particular point id.
  virtual unsigned char *GetColor(int id) = 0;

  // Description:
  // Copy color components into user provided array rgba[4] for specified
  // point id. The number of components returned is the number of values 
  // per point.
  virtual void GetColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into object. No range checking performed (fast!).
  virtual void SetColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into next available slot. Returns point id of slot.
  virtual int InsertNextColor(unsigned char rgba[4]) = 0;

  void GetColors(vlIdList& ptId, vlAPixmap& ap);

};

// These include files are placed here so that if CoScalar.hh is included 
// all other classes necessary for compilation are also included. 
#include "IdList.hh"
#include "APixmap.hh"

#endif
