/*=========================================================================

  Program:   Visualization Library
  Module:    Source.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract class for specifying behavior of data sources
//
#ifndef __vlSource_h
#define __vlSource_h

#include "Object.hh"

class vlSource : virtual public vlObject 
{
public:
  vlSource() : StartMethod(0), EndMethod(0) {};
  ~vlSource() {};
  char *GetClassName() {return "vlSource";};
  void PrintSelf(ostream& os, vlIndent indent);
  virtual void Update();
  void SetStartMethod(void (*f)());
  void SetEndMethod(void (*f)());

protected:
  virtual void Execute();
  void (*StartMethod)();
  void (*EndMethod)();
  vlTimeStamp ExecuteTime;
};

#endif


