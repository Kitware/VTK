
/************************************************************************
 *
 * File: Jacobi.cxx
 *
 * Created:       Mon Feb  1 17:10:45 1999 by Tony Chi-shao Pan
 * Last Modified: Wed Dec 22 14:15:49 1999 by Tony Chi-shao Pan
 *
 * < General description goes here > 
 *
 ************************************************************************/


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

void Test(ostream& strm) {
  // actual test
  int i, j;

  float *eigenvalue = new float[3];
  float **eigenvector = new float*[3];
  float **qmatrix = new float*[3];
  for (i = 0; i < 3; i++)
    {
    qmatrix[i] = new float[3];
    eigenvector[i] = new float[3];
    eigenvalue[i] = 0.0;
    for (j = 0; j < 3; j++)
      {
      eigenvector[i][j] = 0.0;
      }
    }      

  // initialize the qmatrix
  qmatrix[0][0] = 1.00;
  qmatrix[0][1] = 0.42;
  qmatrix[0][2] = 0.54;
  qmatrix[1][0] = 0.42;
  qmatrix[1][1] = 1.00;
  qmatrix[1][2] = 0.32;
  qmatrix[2][0] = 0.54;
  qmatrix[2][1] = 0.32;
  qmatrix[2][2] = 1.00;
  
  
  strm << "Jacobi - original matrix:" << endl;
  for (i = 0; i < 3; i++)
    {
    strm << "       ";
    for (j = 0; j< 3; j++)
      {
      strm << qmatrix[i][j] << " ";
      }
    strm << ";" << endl;
    }

  if (vtkMath::Jacobi(qmatrix, eigenvalue, eigenvector) == 0)
    {
    strm << "ERROR - Jacobi" << endl;
    }

  strm << "Jacobi - eigenvector matrix:" << endl;
  for (i = 0; i < 3; i++)
    {
    strm << "       ";
    for (j = 0; j< 3; j++)
      {
      strm << eigenvector[i][j] << " ";
      }
    strm << ";" << endl;
    }


  strm << "Jacobi - eigenvalue vector:" << endl;
  for (i = 0; i < 3; i++)
    {
    strm << eigenvalue[i] << " ";
    }
  strm << ";" << endl;

  
  // initialize the qmatrix
  qmatrix[0][0] = 1.00;
  qmatrix[0][1] = 0.42;
  qmatrix[0][2] = 0.54;
  qmatrix[1][0] = 0.42;
  qmatrix[1][1] = 1.00;
  qmatrix[1][2] = 0.32;
  qmatrix[2][0] = 0.54;
  qmatrix[2][1] = 0.32;
  qmatrix[2][2] = 1.00;
  
  
  strm << "JacobiN - original matrix:" << endl;
  for (i = 0; i < 3; i++)
    {
    strm << "       ";
    for (j = 0; j< 3; j++)
      {
      strm << qmatrix[i][j] << " ";
      }
    strm << ";" << endl;
    }

  if (vtkMath::JacobiN(qmatrix, 3, eigenvalue, eigenvector) == 0)
    {
    strm << "ERROR - JacobiN" << endl;
    }
  
  
  strm << "JacobiN - eigenvector matrix:" << endl;
  for (i = 0; i < 3; i++)
    {
    strm << "       ";
    for (j = 0; j< 3; j++)
      {
      strm << eigenvector[i][j] << " ";
      }
    strm << ";" << endl;
    }


  strm << "JacobiN - eigenvalue vector:" << endl;
  for (i = 0; i < 3; i++)
    {
    strm << eigenvalue[i] << " ";
    }
  strm << ";" << endl;
}

int main(int argc, char* argv[])
{
  rtOtherTestBase::RunTest(argc, argv, SelectorCommand, ComparatorCommand, Test);

  return 0;
} 
