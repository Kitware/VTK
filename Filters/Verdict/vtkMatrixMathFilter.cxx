/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMatrixMathFilter.h"
#include "vtk_verdict.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkMatrixMathFilter);

vtkMatrixMathFilter::~vtkMatrixMathFilter ()
{
}

vtkMatrixMathFilter::vtkMatrixMathFilter ()
{
  this->Operation = NONE;
  this->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::TENSORS);
}

void vtkMatrixMathFilter::PrintSelf (ostream& os, vtkIndent indent)
{
  static const char* OperationNames [] =
  {
    "None",
    "Determinant",
    "Eigenvalue"
    "Eigenvector",
    "Inverse",
  };

  this->Superclass::PrintSelf(os, indent);
  os << indent << "Operation : "
     << OperationNames[this->Operation] << endl;
}

int vtkMatrixMathFilter::RequestData
(vtkInformation* vtkNotUsed(request),
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* in = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* out = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy input to get a start point
  out->CopyStructure(in);

  int association = vtkDataObject::FIELD_ASSOCIATION_NONE;
  vtkDataArray* inTensors =
    this->GetInputArrayToProcess(0, inputVector, association);

  bool const pointQuality =
    vtkDataObject::FIELD_ASSOCIATION_POINTS == association;
  bool const cellQuality  =
    vtkDataObject::FIELD_ASSOCIATION_CELLS  == association;
  if (!pointQuality && !cellQuality)
  {
    vtkWarningMacro("Unknown association " << association);
    return 1;
  }

  vtkIdType const nCells  = in->GetNumberOfCells ();
  vtkIdType const nPoints = in->GetNumberOfPoints();
  if ((pointQuality && 0 == nPoints) || (cellQuality && 0 == nCells))
  {
    vtkWarningMacro("No data to work.");
    return 1;
  }

  // Allocate storage for the computation
  vtkSmartPointer<vtkDoubleArray> quality =
    vtkSmartPointer<vtkDoubleArray>::New();
  // Set different number of component and name depending on the quality
  switch (this->GetOperation())
  {
    case DETERMINANT :
      quality->SetName("Determinant");
      quality->SetNumberOfComponents(1);
      break;
    case EIGENVALUE :
      quality->SetName("Eigenvalue");
      quality->SetNumberOfComponents(3);
      break;
    case EIGENVECTOR :
      quality->SetName("Eigenvector");
      quality->SetNumberOfComponents(9);
      break;
    case INVERSE :
      quality->SetName("Inverse");
      quality->SetNumberOfComponents(9);
      break;
    default:
      vtkWarningMacro("Bad Operation (" << this->GetOperation() << ")");
      return 1;
  }
  quality->SetNumberOfTuples(pointQuality ? nPoints : nCells);

  // Support progress and abort.
  vtkIdType const tenth = (nCells >= 10 ? nCells/10 : 1);
  double const nCellInv = 1./nCells;

  // Actual computation of the selected quality
  for (vtkIdType i = 0, n = (pointQuality ? nPoints : nCells); i < n; ++i)
  {
    // Periodically update progress and check for an abort request.
    if (0 == i % tenth)
    {
      this->UpdateProgress((i+1)*nCellInv);
      if (this->GetAbortExecute()) { break; }
    }

    // Interpret the associated data as a 3 by 3 matrix and evaluate it for the
    // requested quality measure.
    switch (this->GetOperation())
    {
      case DETERMINANT :
      {
        double tensor[9];
        inTensors->GetTuple(i, tensor);
        if (inTensors->GetNumberOfComponents() == 6)
        {
          vtkMath::TensorFromSymmetricTensor(tensor);
        }
        double const q = vtkMath::Determinant3x3(
          reinterpret_cast<double(*)[3]>(tensor));
        quality->SetTuple(i, &q);
        break;
      }
      case EIGENVALUE :
      case EIGENVECTOR :
      {
        double d[9];
        inTensors->GetTuple(i, d);
        if (inTensors->GetNumberOfComponents() == 6)
        {
          vtkMath::TensorFromSymmetricTensor(d);
        }

        double  w[3]={0}, v[9]={0}, t[]={d[1]-d[3], d[2]-d[6], d[5]-d[7]};

        // Use Jacobi iterative method only if the matrix is real symmetric.
        // Return singular values (all zeros) all other cases.
        if (-1e-5 <= t[0] && t[0] <= 1e-5 &&
            -1e-5 <= t[1] && t[1] <= 1e-5 &&
            -1e-5 <= t[2] && t[2] <= 1e-5)
        {
          // I have to do this conversion due to the Jacobi implementation.
          double* dd [] = {d, d+3, d+6};
          double* vv [] = {v, v+3, v+6};
          vtkMath::Jacobi(dd, w, vv);
        }

        if (EIGENVALUE == this->GetOperation())
        {
          quality->SetTuple(i, w);
        }
        else
        {
          quality->SetTuple(i, v);
        }
        break;
      }
      case INVERSE :
      {
        double tensor[9];
        inTensors->GetTuple(i, tensor);
        if (inTensors->GetNumberOfComponents() == 6)
        {
          vtkMath::TensorFromSymmetricTensor(tensor);
        }

        double AI [3][3] = {{0}}, (*A) [3]
          = reinterpret_cast<double(*)[3]>(tensor);

        // vtkMath::Invert3x3 should quite fit here, unfortunately, it does not
        // check for matrix singularity which in the worest case leads to divide
        // by zero.
        // Below is a copy of the code with the necessary check.

        double a1 = A[0][0]; double b1 = A[0][1]; double c1 = A[0][2];
        double a2 = A[1][0]; double b2 = A[1][1]; double c2 = A[1][2];
        double a3 = A[2][0]; double b3 = A[2][1]; double c3 = A[2][2];

        // Compute the adjoint
        double d1 = vtkMath::Determinant2x2(b2, b3, c2, c3);
        double d2 =-vtkMath::Determinant2x2(a2, a3, c2, c3);
        double d3 = vtkMath::Determinant2x2(a2, a3, b2, b3);

        double e1 =-vtkMath::Determinant2x2(b1, b3, c1, c3);
        double e2 = vtkMath::Determinant2x2(a1, a3, c1, c3);
        double e3 =-vtkMath::Determinant2x2(a1, a3, b1, b3);

        double f1 = vtkMath::Determinant2x2(b1, b2, c1, c2);
        double f2 =-vtkMath::Determinant2x2(a1, a2, c1, c2);
        double f3 = vtkMath::Determinant2x2(a1, a2, b1, b2);

        // Divide by the determinant
        double det = a1*d1 + b1*d2 + c1*d3;

        // Compute inverse only if the matrix is non-singular
        if (det < -VTK_DBL_EPSILON || det > VTK_DBL_EPSILON)
        {
          AI[0][0] = d1/det;
          AI[1][0] = d2/det;
          AI[2][0] = d3/det;

          AI[0][1] = e1/det;
          AI[1][1] = e2/det;
          AI[2][1] = e3/det;

          AI[0][2] = f1/det;
          AI[1][2] = f2/det;
          AI[2][2] = f3/det;
        }

        quality->SetTuple(i, AI[0]);
        break;
      }
    }
  }

  if (pointQuality)
  {
    out->GetPointData()->AddArray(quality);
  }
  else
  {
    out->GetCellData()->AddArray(quality);
  }

  return 1;
}
