/*=========================================================================

  Program:   Visualization Library
  Module:    Writer.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract class to write data to file(s)
//

#ifndef __vlWriter_hh
#define __vlWriter_hh

#include "Object.hh"

class vlWriter : virtual public vlObject 
{
public:
  char *GetClassName() {return "vlWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  virtual void Write() = 0;

  void SetStartWrite(void (*f)());
  void SetEndWrite(void (*f)());

protected:
  void (*StartWrite)();
  void (*EndWrite)();

};

#endif


