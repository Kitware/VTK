/*=========================================================================

  Program:   Visualization Library
  Module:    AGraymap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlAGraymap - scalar data in intensity + alpha (grayscale  + texture) form
// .SECTION Description
// vlAGraymap is a concrete implementation of vlScalars. vlAGraymap 
// represents scalars using using one value for intensity (grayscale) and
// one value for alpha (transparency). The intensity and alpha values range 
// between (0,255) (i.e., an unsigned char value).

#ifndef __vlAGraymap_h
#define __vlAGraymap_h

#include "CoScalar.hh"
#include "CArray.hh"

class vlAGraymap : public vlColorScalars 
{
public:
  vlAGraymap() {};
  ~vlAGraymap() {};
  vlAGraymap(const vlAGraymap& fs) {this->S = fs.S;};
  vlAGraymap(const int sz, const int ext=1000):S(2*sz,2*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(2*sz,2*ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlAGraymap";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1)/2;};
  void Squeeze() {this->S.Squeeze();};
  int GetNumberOfValuesPerScalar() {return 2;};

  // miscellaneous
  vlAGraymap &operator=(const vlAGraymap& fs);
  void operator+=(const vlAGraymap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  void WrotePtr();


  // vlColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

protected:
  vlCharArray S;
};

// Description:
// Set a rgba color value at a particular array location. Does not do 
// range checking.
inline void vlAGraymap::SetColor(int i, unsigned char rgba[4]) 
{
  i *= 2; 
  this->S[i] = (rgba[0] > rgba[1] ? (rgba[0] > rgba[2] ? rgba[0] : rgba[2]) :
                                    (rgba[1] > rgba[2] ? rgba[1] : rgba[2]));
  this->S[i+1] = rgba[3]; 
}

// Description:
// Insert a rgba color value at a particular array location. Does range 
// checking and will allocate additional memory if necessary.
inline void vlAGraymap::InsertColor(int i, unsigned char rgba[4]) 
{
  this->S.InsertValue(2*i+1, rgba[3]);
  this->S[2*i] = (rgba[0] > rgba[1] ? (rgba[0] > rgba[2] ? rgba[0] : rgba[2]) :
                                      (rgba[1] > rgba[2] ? rgba[1] : rgba[2]));
}

// Description:
// Insert a rgba color value at the next available slot in the array. Will
// allocate memory if necessary.
inline int vlAGraymap::InsertNextColor(unsigned char rgba[4]) 
{
  int id = this->S.GetMaxId() + 1;
  this->S.InsertValue(id,rgba[3]);
  this->S[id-1] = (rgba[0] > rgba[1] ? (rgba[0] > rgba[2] ? rgba[0] : rgba[2]) :
                                       (rgba[1] > rgba[2] ? rgba[1] : rgba[2]));

  return id/2;
}

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vlAGraymap::GetPtr(const int id)
{
  return this->S.GetPtr(2*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline unsigned char *vlAGraymap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(2*id,2*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vlAGraymap::WrotePtr() {}


#endif
