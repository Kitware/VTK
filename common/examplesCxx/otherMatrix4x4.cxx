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
  int i, j, k;
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
  
  k = 0;
  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++) 
      {
      mat1->SetElement(i, j, (double)(k++));
      }
    }
  strm << "mat1 " << endl;
  strm << *mat1;

  strm << "transposing mat1" << endl;
  mat1->Transpose();
  strm << "mat1 " << endl;
  strm << *mat1;
  
  strm << "mat2 = 5.6" << endl;
  *mat2 = 5.6;
  strm << "mat2 " << endl;
  strm << *mat2;
  
  strm << "zero mat2" << endl;
  mat2->Zero();
  strm << "mat2 " << endl;
  strm << *mat2;
  

  strm << "deep copy array into mat1" << endl;
  double array[16];
  array[0]  = .75;   array[1]  = -.4;  array[2]  = .5;  array[3] = 1;
  array[4]  = .65;   array[5]  = .625; array[6]  = -.4; array[7] = 2;
  array[8]  = -.125; array[9]  = .65;  array[10] = .75; array[11] = 3;
  array[12] = 0;     array[13] = 0;    array[14] = 0;   array[15] = 1;
  mat1->DeepCopy(array);
  strm << "mat1 " << endl;
  strm << *mat1;

  strm << "determinant of mat1" << endl;
  strm << "  " << mat1->Determinant(mat1) << endl;

  strm << "adjoint of mat1, put in mat2" << endl;
  mat1->Adjoint(mat1, mat2);
  strm << "mat1 " << endl;
  strm << *mat1;
  
  strm << "mat2 " << endl;
  strm << *mat2;
  
  strm << "inverse of mat1" << endl;
  mat1->Invert(mat1,mat2);
  strm << "mat2 " << endl;
  strm << *mat2;

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
  

  strm << "text test vtkMatrix4x4 completed" << endl;
  mat1->Delete();
  mat2->Delete();
}



int main(int argc, char* argv[])
{
  rtOtherTestBase::RunTest(argc, argv, SelectorCommand, ComparatorCommand, Test);
  return 0;  
}

