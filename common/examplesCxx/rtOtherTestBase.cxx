/*==========================================================================

  Program: 
  Module:    rtOtherTestBase.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1999 by General Electric Company,
                        Schenectady, NY 12301

  ==========================================================================*/

// .NAME 
// .SECTION Description
// 

#include "rtOtherTestBase.h"
#include <string.h>
#include <fstream.h>
  
void rtOtherTestBase::OutputObj(vtkObject *obj, char *name, ostream& os) {
  os << name << ": " << endl;
  os << *obj;
}

void rtOtherTestBase::RunTest(int argc, char* argv[], void (*filter)(ostream&),
             void (*comparator)(ostream&), void (*type)(ostream&),
             void (*test)(ostream&)) 
{
  // first process the arguments.  this is where the test result path is 
  // specified to the test, and where the test type, selector and comparator
  // are specified to the testing script.  
  ostream *out = NULL;
  int fileout = 0;
  if (argc <= 1) 
    {
    cout << "outputting to stdout.  -h for options" << endl;
    out = &cout;
    fileout = 0;
    }
  else 
    {
    if (strcmp(argv[1], "-S") == 0)
      {
      if (argc >= 3)
        {
        out = new ofstream(argv[2]);
        fileout = 1;
        }
      else 
        {
        cout << "outputting to stdout.  -h for options" << endl;
        out = &cout;
        fileout = 0;
        }
      }
    else if (strcmp(argv[1], "-f") == 0)
      {
      (*filter)(cout);
      return;
      }
    else if (strcmp(argv[1], "-c") == 0)
      {
      (*comparator)(cout);
      return;
      }
    else if (strcmp(argv[1], "-e") == 0)
      {
      (*type)(cout);
      return;
      }
    else
      {
      cout << "optional parameters are" << endl;
      cout << "       -S file    path and filename" << endl;
      cout << "       -f         print filter command string" << endl;
      cout << "       -c         print comparator command string" << endl;
      cout << "       -e         type and extension of result file" << endl;
      return;
      }
    }

  // single precision
  (*out).precision(6);
  (*test)(*out);

  // Clean up
  *out << flush;
  if (fileout == 1) 
    {
    ((ofstream *)out)->close();
    delete out;
    }
}
