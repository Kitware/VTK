/*=========================================================================

  Program:   Visualization Library
  Module:    Mapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract class to map pipeline data to graphics primitives
//
#ifndef __vlMapper_hh
#define __vlMapper_hh

#include "Object.hh"
#include "GeomPrim.hh"
#include "Lut.hh"

class vlRenderer;

class vlMapper : public vlObject 
{
public:
  vlMapper();
  ~vlMapper();
  char *GetClassName() {return "vlMapper";};
  void PrintSelf(ostream& os, vlIndent indent);
  void operator=(const vlMapper& m);
  virtual void Render(vlRenderer *) = 0;
  virtual float *GetBounds() = 0;
  void SetStartRender(void (*f)());
  void SetEndRender(void (*f)());

  vlSetObjectMacro(LookupTable,vlLookupTable);
  vlGetObjectMacro(LookupTable,vlLookupTable);

  vlSetMacro(ScalarsVisible,int);
  vlGetMacro(ScalarsVisible,int);
  vlBooleanMacro(ScalarsVisible,int);

  vlSetVector2Macro(ScalarRange,float)
  vlGetVectorMacro(ScalarRange,float)

protected:
  void (*StartRender)();
  void (*EndRender)();
  vlLookupTable *LookupTable;
  int ScalarsVisible;
  vlTimeStamp BuildTime;
  float ScalarRange[2];

};

#endif


