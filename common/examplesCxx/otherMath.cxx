#include "vtkMath.h"

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
  strm << " the number is 0.017453292519943295769237" << endl;
  strm << " the number is ";
  strm << vtkMath::DoubleDegreesToRadians() << endl;


  strm << "Cross test" << endl;
  float x[3], y[3], z[3];
  double dx[3], dy[3], dz[3];

  x[0] = 1.023;
  x[1] = 3.044;
  x[2] = 4.5;
  y[0] = 6.302;
  y[1] = 0.976;
  y[2] = 1.74;

  
  dx[0] = 1.023;
  dx[1] = 3.044;
  dx[2] = 4.5;
  dy[0] = 6.302;
  dy[1] = 0.976;
  dy[2] = 1.74;
  
  strm << " the first vector is: ";
  strm << x[0] << " " << x[1] << " " << x[2] << endl;
  strm << "                      " << dx[0] << " " << dx[1] << " " << dx[2] << endl;
  
  strm << " the second vector is: ";
  strm << y[0] << " " << y[1] << " " << y[2] << endl;
  strm << "                      " << dy[0] << " " << dy[1] << " " << dy[2] << endl;
  
  vtkMath::Cross(x, y, z);
  vtkMath::Cross(dx, dy, dz);

  strm << " the third vector is: ";
  strm << z[0] << " " << z[1] << " " << z[2] << endl;
  strm << "                      " << dz[0] << " " << dz[1] << " " << dz[2] << endl;
  
  


  int i;

  strm << "loop test: ";
  for (i = 0; i < 10; ++i)
    {
    strm << i << ", ";
    }
  strm << " fin" << endl;

  strm << "loop test 2: ";
  for (i = 0; i < 10; i++)
    {
    strm << i << ", ";
    }
  strm << " fin" << endl;
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
      cout << "cat";
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

  // double precision ?
  (*out).precision(27);
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
