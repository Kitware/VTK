#include "vtkMath.h"

// all tests need: the following 3 includes, code to parse the args
// create the file stream, and clean up code at the end
#include <iostream.h>
#include "rtOtherTestBase.h"

void FilterCommand(ostream& strm) {
  strm << "cat";
}

void ComparatorCommand(ostream& strm) {
  strm << "diff";
}

void TypeCommand(ostream& strm) {
  strm << "rtr";
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
  rtOtherTestBase::RunTest(argc, argv, FilterCommand, ComparatorCommand,
                           TypeCommand, Test);
  
  return 0;  
}
