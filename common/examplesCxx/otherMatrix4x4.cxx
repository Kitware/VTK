/*==========================================================================

  Program: 
  Module:    otherMatrix4x4.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1999 by General Electric Company,
                        Schenectady, NY 12301

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the matrix

#include "vtkMatrix4x4.h"

// all tests need: the following 3 includes, code to parse the args
// create the file stream, and clean up code at the end
#include <iostream.h>
#include <string.h>
#include <fstream.h>


void outputObj(vtkObject *obj, char *name, ostream& os) {
  os << name << ": " << endl;
  os << *obj;
}


void Test(ostream& strm)
{
  // actual test
  int i, j;
  strm << "text test matrix4x4 begin" << endl;

  strm << "initialize mat1" << endl;
  vtkMatrix4x4 *mat1 = vtkMatrix4x4::New();
  outputObj(mat1, "mat1", strm);

  strm << "initialize mat2" << endl;
  vtkMatrix4x4 *mat2 = vtkMatrix4x4::New();
  outputObj(mat2, "mat2", strm);
  
  strm << "setting mat1" << endl;
  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++) 
      {
      mat1->SetElement(i, j, (double)(i * j + 1));
      }
    }
  outputObj(mat1, "mat1", strm);

  strm << "transposing mat1" << endl;
  mat1->Transpose();
  outputObj(mat1, "mat1", strm);
  
  strm << "deep copy mat1 into mat2" << endl;
  mat2->DeepCopy(mat1);
  outputObj(mat2, "mat2", strm);
  
  strm << "zero mat2" << endl;
  mat2->Zero();
  outputObj(mat2, "mat2", strm);
  
  strm << "determinant of mat1" << endl;
  strm << "  " << mat1->Determinant(mat1);

  strm << "adjoint of mat1, put in mat2" << endl;
  mat1->Adjoint(mat1, mat2);
  outputObj(mat2, "mat2", strm);
  
  strm << "inverse of mat1" << endl;
  mat1->Invert();
  outputObj(mat1, "mat1", strm);


  double in[4];
  double out[4];
  for (i = 0; i < 3; i ++)
    {
    in[i] = i;
    }
  in[3] = 1;
  strm << "point = [" << in[0] << " " << in[1] << " " <<
    in[2] << " " << in[3] << "]" << endl;

  strm << "multiply point" << endl;
  mat1->MultiplyPoint(in, out);
  strm << "output point = [" << out[0] << " " << out[1] << " " <<
    out[2] << " " << out[3] << "]" << endl;
  

  strm << "point multiply" << endl;
  mat1->PointMultiply(in, out);
  strm << "output point = [" << out[0] << " " << out[1] << " " <<
    out[2] << " " << out[3] << "]" << endl;


  strm << "text test vtkMatrix4x4 completed" << endl;
  mat1->Delete();
  mat2->Delete();
}



int main(int argc, char* argv[])
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
      cout << "grep -v vtkMatrix4x4 | grep -v Modified";
      return 0;
      }
    else if (strcmp(argv[1], "-c") == 0)
      {
      cout << "diff";
      return 0;
      }
    else if (strcmp(argv[1], "-e") == 0)
      {
      cout << "rtr";
      return 0;
      }
    else
      {
      cout << "optional parameters are" << endl;
      cout << "       -S file    path and filename" << endl;
      cout << "       -f         print filter command string" << endl;
      cout << "       -c         print comparator command string" << endl;
      cout << "       -e         type and extension of result file" << endl;
      return 0;
      }
    }
  
  // double precision
  (*out).precision(40);
  Test(*out);

  // Clean up
  *out << flush;
  if (fileout == 1) 
    {
    ((ofstream *)out)->close();
    delete out;
    }
  
  return 0;  
}

