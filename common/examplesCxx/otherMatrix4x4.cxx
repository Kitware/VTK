/*==========================================================================

  Program: 
  Module:    otherMatrix4x4.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the matrix

#include "vtkMatrix4x4.h"

// All tests need:
//   the following include
//   a Selector proc
//   a Comparator proc
//   a Test proc
//   and a main
#include "rtOtherTestBase.h"

void SelectorCommand(ostream& strm) {
  strm << "grep -v vtkMatrix4x4 | grep -v Modified";
}

void ComparatorCommand(ostream& strm) {
  strm << "diff";
}

void Test(ostream& strm)
{
  // actual test
  int i, j;
  strm << "text test matrix4x4 begin" << endl;

  strm << "initialize mat1" << endl;
  vtkMatrix4x4 *mat1 = vtkMatrix4x4::New();
  strm << "mat1 " << endl;
  strm << *mat1;
  
  strm << "initialize mat2" << endl;
  vtkMatrix4x4 *mat2 = vtkMatrix4x4::New();
  strm << "mat2 " << endl;
  strm << *mat2;
  
  strm << "setting mat1" << endl;
  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++) 
      {
      mat1->SetElement(i, j, (double)(i * j + 1));
      }
    }
  strm << "mat1 " << endl;
  strm << *mat1;

  strm << "transposing mat1" << endl;
  mat1->Transpose();
  strm << "mat1 " << endl;
  strm << *mat1;
  
  strm << "deep copy mat1 into mat2" << endl;
  mat2->DeepCopy(mat1);
  strm << "mat2 " << endl;
  strm << *mat2;
  
  strm << "zero mat2" << endl;
  mat2->Zero();
  strm << "mat2 " << endl;
  strm << *mat2;
  
  strm << "determinant of mat1" << endl;
  strm << "  " << mat1->Determinant(mat1);

  strm << "adjoint of mat1, put in mat2" << endl;
  mat1->Adjoint(mat1, mat2);
  strm << "mat1 " << endl;
  strm << *mat1;
  
  strm << "mat2 " << endl;
  strm << *mat2;
  
  strm << "inverse of mat1" << endl;
  mat1->Invert();
  strm << "mat1 " << endl;
  strm << *mat1;

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
  rtOtherTestBase::RunTest(argc, argv, SelectorCommand, ComparatorCommand, Test);
  return 0;  
}

