/*==========================================================================

  Program: 
  Module:    otherLotkin.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the vtkMath::InvertMatrix method

#include "vtkMath.h"

// All tests need:
//   the following include
//   a Selector proc
//   a Comparator proc
//   a Test proc
//   and a main
#include "rtOtherTestBase.h"

void SelectorCommand(ostream& strm) {
  strm << "cat";
}

void ComparatorCommand(ostream& strm) {
  strm << "diff";
}

void Test(ostream& strm)
{
  // actual test
  int i, j;
  strm << "Test vtkMath::InvertMatrix Start" << endl;

  double matrix[5][5], inverse[5][5];
  double *lotkin[5], *Ilotkin[5];

  matrix[0][0] = 1.00000;
  matrix[0][1] = 1.00000;
  matrix[0][2] = 1.00000;
  matrix[0][3] = 1.00000;
  matrix[0][4] = 1.00000;
  matrix[1][0] = 0.500000;
  matrix[1][1] = 1.0 / 3.0;
  matrix[1][2] = 0.250000;
  matrix[1][3] = 0.200000;
  matrix[1][4] = 1.0 / 6.0;
  matrix[2][0] = 1.0 / 3.0;
  matrix[2][1] = 0.250000;
  matrix[2][2] = 0.200000;
  matrix[2][3] = 1.0 / 6.0;
  matrix[2][4] = 1.0 / 7.0;
  matrix[3][0] = 0.250000;
  matrix[3][1] = 0.200000;
  matrix[3][2] = 1.0 / 6.0;
  matrix[3][3] = 1.0 / 7.0;
  matrix[3][4] = 0.125000;
  matrix[4][0] = 0.200000;
  matrix[4][1] = 1.0 / 6.0;
  matrix[4][2] = 1.0 / 7.0;
  matrix[4][3] = 0.125000;
  matrix[4][4] = 1.0 / 9.0;

  lotkin[0] = &matrix[0][0];
  lotkin[1] = &matrix[1][0];
  lotkin[2] = &matrix[2][0];
  lotkin[3] = &matrix[3][0];
  lotkin[4] = &matrix[4][0];
  
  Ilotkin[0] = &inverse[0][0];
  Ilotkin[1] = &inverse[1][0];
  Ilotkin[2] = &inverse[2][0];
  Ilotkin[3] = &inverse[3][0];
  Ilotkin[4] = &inverse[4][0];
  

  strm << "Lotkin Matrix" << endl;
  for (j = 0; j < 5; j++)
    {
    strm << "\t";
    for (i = 0; i < 5; i++)
      {
      strm << matrix[j][i] << ", ";
      }
    strm << endl;
    }

  strm << "vtkMath::InvertMatrix(double **, double **, int)" << endl;
  vtkMath::InvertMatrix (lotkin, Ilotkin, 5);

  for (j = 0; j < 5; j++)
    {
    strm << "\t";
    for (i = 0; i < 5; i++)
      {
      strm << inverse[j][i] << ", ";
      }
    strm << endl;
    }

  matrix[0][0] = 1.00000;
  matrix[0][1] = 1.00000;
  matrix[0][2] = 1.00000;
  matrix[0][3] = 1.00000;
  matrix[0][4] = 1.00000;
  matrix[1][0] = 0.500000;
  matrix[1][1] = 1.0 / 3.0;
  matrix[1][2] = 0.250000;
  matrix[1][3] = 0.200000;
  matrix[1][4] = 1.0 / 6.0;
  matrix[2][0] = 1.0 / 3.0;
  matrix[2][1] = 0.250000;
  matrix[2][2] = 0.200000;
  matrix[2][3] = 1.0 / 6.0;
  matrix[2][4] = 1.0 / 7.0;
  matrix[3][0] = 0.250000;
  matrix[3][1] = 0.200000;
  matrix[3][2] = 1.0 / 6.0;
  matrix[3][3] = 1.0 / 7.0;
  matrix[3][4] = 0.125000;
  matrix[4][0] = 0.200000;
  matrix[4][1] = 1.0 / 6.0;
  matrix[4][2] = 1.0 / 7.0;
  matrix[4][3] = 0.125000;
  matrix[4][4] = 1.0 / 9.0;

  strm << "vtkMath:InvertMatrix(double **, double **, int, int *, double *)" << endl;
  int tmpIntSpace[5];
  double tmpDoubleSpace[5];
  vtkMath::InvertMatrix (lotkin, Ilotkin, 5, tmpIntSpace, tmpDoubleSpace);

  for (j = 0; j < 5; j++)
    {
    strm << "\t";
    for (i = 0; i < 5; i++)
      {
      strm << inverse[j][i] << ", ";
      }
    strm << endl;
    }
  strm << "Test vtkMath::InvertMatrix Complete" << endl;

  matrix[0][0] = 1.00000;
  matrix[0][1] = 1.00000;
  matrix[0][2] = 1.00000;
  matrix[0][3] = 1.00000;
  matrix[0][4] = 1.00000;
  matrix[1][0] = 0.500000;
  matrix[1][1] = 1.0 / 3.0;
  matrix[1][2] = 0.250000;
  matrix[1][3] = 0.200000;
  matrix[1][4] = 1.0 / 6.0;
  matrix[2][0] = 1.0 / 3.0;
  matrix[2][1] = 0.250000;
  matrix[2][2] = 0.200000;
  matrix[2][3] = 1.0 / 6.0;
  matrix[2][4] = 1.0 / 7.0;
  matrix[3][0] = 0.250000;
  matrix[3][1] = 0.200000;
  matrix[3][2] = 1.0 / 6.0;
  matrix[3][3] = 1.0 / 7.0;
  matrix[3][4] = 0.125000;
  matrix[4][0] = 0.200000;
  matrix[4][1] = 1.0 / 6.0;
  matrix[4][2] = 1.0 / 7.0;
  matrix[4][3] = 0.125000;
  matrix[4][4] = 1.0 / 9.0;

  strm << "vtkMath:LUFactorLinearSystem(double **, int *, int, double *)" << endl;
  vtkMath::LUFactorLinearSystem (lotkin, tmpIntSpace, 5, tmpDoubleSpace);

  for (j = 0; j < 5; j++)
    {
    strm << "\t";
    for (i = 0; i < 5; i++)
      {
      strm << matrix[j][i] << ", ";
      }
    strm << endl;
    }
  
  strm << "vtkMath:EstimateMatrixCondition(double **, int)" << endl;
  strm << "Condition is: " << vtkMath::EstimateMatrixCondition (lotkin, 5) << endl;
}



int main(int argc, char* argv[])
{
  rtOtherTestBase::RunTest(argc, argv, SelectorCommand, ComparatorCommand, Test);
  return 0;  
}

