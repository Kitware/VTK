/*==========================================================================

  Program: 
  Module:    rtOtherTestBase.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1999 by General Electric Company,
                        Schenectady, NY 12301

  ==========================================================================*/

// .NAME 
// .SECTION Description
//   

#ifndef __rtOtherTestBase_h
#define __rtOtherTestBase_h

#include <iostream.h>
#include "vtkObject.h"

class rtOtherTestBase {
 public:
  static void RunTest(int argc, char* argv[], void (*filter)(ostream&),
                      void (*comparator)(ostream&), void (*type)(ostream&),
                      void (*test)(ostream&));
  static void OutputObj(vtkObject *obj, char *name, ostream& os);
};

#endif 
