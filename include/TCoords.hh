/*=========================================================================

  Program:   Visualization Library
  Module:    TCoords.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract interface to texture coordinates
//
#ifndef __vlTCoords_h
#define __vlTCoords_h

#include "Object.hh"

class vlIdList;
class vlFloatTCoords;

class vlTCoords : public vlObject 
{
public:
  vlTCoords(int dim=2) {this->Dimension=dim;};
  virtual ~vlTCoords() {};
  virtual vlTCoords *MakeObject(int sze, int d=2, int ext=1000) = 0;
  virtual int GetNumberOfTCoords() = 0;
  virtual float *GetTCoord(int i) = 0;
  virtual void SetTCoord(int i,float *x) = 0;          // fast insert
  virtual void InsertTCoord(int i, float *x) = 0;      // allocates memory as necessary
  virtual void Squeeze() = 0;

  void GetTCoords(vlIdList& ptId, vlFloatTCoords& fp);
  char *GetClassName() {return "vlTCoords";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(Dimension,int,1,3);
  vlGetMacro(Dimension,int);

protected:
  int Dimension;

};

#endif
