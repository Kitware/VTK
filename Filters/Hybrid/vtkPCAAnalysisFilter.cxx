/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCAAnalysisFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPCAAnalysisFilter.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"

vtkStandardNewMacro(vtkPCAAnalysisFilter);

//------------------------------------------------------------------------
// Matrix ops. Some taken from vtkThinPlateSplineTransform.cxx
static inline double** NewMatrix(int rows, int cols)
{
  double *matrix = new double[rows*cols];
  double **m = new double *[rows];
  for(int i = 0; i < rows; i++) {
    m[i] = &matrix[i*cols];
  }
  return m;
}

//------------------------------------------------------------------------
static inline void DeleteMatrix(double **m)
{
  delete [] *m;
  delete [] m;
}

//------------------------------------------------------------------------
static inline void MatrixMultiply(double **a, double **b, double **c,
                                  int arows, int acols,
                                  int brows, int bcols)
{
  if(acols != brows)  {
    return; // acols must equal br otherwise we can't proceed
  }

  // c must have size arows*bcols (we assume this)

  for(int i = 0; i < arows; i++) {
    for(int j = 0; j < bcols; j++) {
      c[i][j] = 0.0;
      for(int k = 0; k < acols; k++) {
        c[i][j] += a[i][k]*b[k][j];
      }
    }
  }
}

//------------------------------------------------------------------------
// Subtracting the mean column from the observation matrix is equal
// to subtracting the mean shape from all shapes.
// The mean column is equal to the Procrustes mean (it is also returned)
static inline void SubtractMeanColumn(double **m, double *mean, int rows, int cols)
{
  int r,c;
  double csum;
  for (r = 0; r < rows; r++) {
    csum = 0.0F;
    for (c = 0; c < cols; c++) {
      csum += m[r][c];
    }
    // calculate average value of row
    csum /= cols;

    // Mean shape vector is updated
    mean[r] = csum;

    // average value is subtracted from all elements in the row
    for (c = 0; c < cols; c++) {
      m[r][c] -= csum;
    }
  }
}

//------------------------------------------------------------------------
// Normalise all columns to have length 1
// meaning that all eigenvectors are normalised
static inline void NormaliseColumns(double **m, int rows, int cols)
{
  for (int c = 0; c < cols; c++) {
    double cl = 0;
    for (int r = 0; r < rows; r++) {
      cl += m[r][c] * m[r][c];
    }
    cl = sqrt(cl);

    // If cl == 0 something is rotten, dont do anything now
    if (cl != 0) {
      for (int r = 0; r < rows; r++) {
        m[r][c] /= cl;
      }
    }
  }
}

//------------------------------------------------------------------------
// Here it is assumed that a rows >> a cols
// Output matrix is [a cols X a cols]
static inline void SmallCovarianceMatrix(double **a, double **c,
                                         int arows, int acols)
{
  const int s = acols;

  // c must have size acols*acols (we assume this)
  for(int i = 0; i < acols; i++) {
    for(int j = 0; j < acols; j++) {
      // Use symmetry
      if (i <= j) {
        c[i][j] = 0.0;
        for(int k = 0; k < arows; k++) {
          c[i][j] += a[k][i]*a[k][j];
        }
        c[i][j] /= (s-1);
        c[j][i] = c[i][j];
      }
    }
  }
}

//------------------------------------------------------------------------
static inline double* NewVector(int length)
{
  double *vec = new double[length];
  return vec;
}

//------------------------------------------------------------------------
static inline void DeleteVector(double *v)
{
  delete [] v;
}


//----------------------------------------------------------------------------
// protected
vtkPCAAnalysisFilter::vtkPCAAnalysisFilter()
{
  this->Evals = vtkFloatArray::New();
  this->evecMat2 = NULL;
  this->meanshape = NULL;
}

//----------------------------------------------------------------------------
// protected
vtkPCAAnalysisFilter::~vtkPCAAnalysisFilter()
{
  if (this->Evals) {
    this->Evals->Delete();
  }
  if (this->evecMat2) {
    DeleteMatrix(this->evecMat2);
    this->evecMat2 = NULL;
  }
  if (this->meanshape) {
    DeleteVector(this->meanshape);
    this->meanshape = NULL;
  }
}

//----------------------------------------------------------------------------
// protected
int vtkPCAAnalysisFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkMultiBlockDataSet *mbInput = vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Execute()");
  int i;

  // Clean up from previous computation
  if (this->evecMat2)
  {
    DeleteMatrix(this->evecMat2);
    this->evecMat2 = NULL;
  }
  if (this->meanshape)
  {
    DeleteVector(this->meanshape);
    this->meanshape = NULL;
  }

  const int N_SETS = mbInput->GetNumberOfBlocks();

  vtkPointSet* input = 0;
  for (i=0; i<N_SETS; i++)
  {
    input = vtkPointSet::SafeDownCast(mbInput->GetBlock(i));
    if (input)
    {
      break;
    }
  }

  if (!input)
  {
    return 1;
  }

  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointSet *tmpInput;
  // copy the inputs across
  for(i=0;i<N_SETS;i++)
  {
    tmpInput =
      vtkPointSet::SafeDownCast(mbInput->GetBlock(i));
    vtkPointSet* outputBlock = 0;
    if (tmpInput)
    {
      outputBlock = tmpInput->NewInstance();
      outputBlock->DeepCopy(tmpInput);
    }
    output->SetBlock(i, outputBlock);
    if (outputBlock)
    {
        outputBlock->Delete();
    }
  }

  // the number of points is determined by the first input (they must all be the same)
  const int N_POINTS = input->GetNumberOfPoints();

  vtkDebugMacro(<<"N_POINTS is " <<N_POINTS);

  if(N_POINTS == 0)
  {
    vtkErrorMacro(<<"No points!");
    return 1;
  }

  // all the inputs must have the same number of points to consider executing
  for(i=1;i<N_SETS;i++)
  {
    tmpInput =
      vtkPointSet::SafeDownCast(mbInput->GetBlock(i));
    if (!tmpInput)
    {
      continue;
    }
    if(tmpInput->GetNumberOfPoints() != N_POINTS)
    {
      vtkErrorMacro(<<"The inputs have different numbers of points!");
      return 1;
    }
  }

  // Number of shapes
  const int s = N_SETS;

  // Number of points in a shape
  const int n = N_POINTS;

  // Observation Matrix [number of points * 3 X number of shapes]
  double **D = NewMatrix(3*n, s);

  for (i = 0; i < n; i++)
  {
    for (int j = 0; j < s; j++)
    {
      tmpInput =
        vtkPointSet::SafeDownCast(mbInput->GetBlock(j));
      if (!tmpInput)
      {
        continue;
      }
      double p[3];
      tmpInput->GetPoint(i, p);
      D[i*3  ][j] = p[0];
      D[i*3+1][j] = p[1];
      D[i*3+2][j] = p[2];
    }
  }

  // The mean shape is also calculated
  meanshape = NewVector(3*n);

  SubtractMeanColumn(D, meanshape, 3*n, s);

  // Covariance matrix of dim [s x s]
  double **T = NewMatrix(s, s);
  SmallCovarianceMatrix(D, T, 3*n, s);

  double *ev       = NewVector(s);
  double **evecMat = NewMatrix(s, s);

  vtkMath::JacobiN(T, s, ev, evecMat);

  // Compute eigenvecs of DD' instead of T which is D'D
  // evecMat2 of dim [3*n x s]
  evecMat2 = NewMatrix(3*n, s);
  MatrixMultiply(D, evecMat, evecMat2, 3*n, s, s, s);

  // Normalise eigenvectors
  NormaliseColumns(evecMat2, 3*n, s);

  this->Evals->SetNumberOfValues(s);

  // Copy data to output structures
  for (int j = 0; j < s; j++)
  {
    this->Evals->SetValue(j, ev[j]);

    vtkPointSet* block = vtkPointSet::SafeDownCast(
      output->GetBlock(j));

    if (block)
    {
      for (i = 0; i < n; i++)
      {
        double x = evecMat2[i*3  ][j];
        double y = evecMat2[i*3+1][j];
        double z = evecMat2[i*3+2][j];

        block->GetPoints()->SetPoint(i, x, y, z);
      }
    }
  }

  DeleteMatrix(evecMat);
  DeleteVector(ev);
  DeleteMatrix(T);
  DeleteMatrix(D);

  return 1;
}


//----------------------------------------------------------------------------
// public
void vtkPCAAnalysisFilter::GetParameterisedShape(vtkFloatArray *b, vtkPointSet* shape)
{
  vtkPointSet* output = 0;

  vtkMultiBlockDataSet *mbOutput = this->GetOutput();

  int numBlocks = mbOutput->GetNumberOfBlocks();

  int i,j;

  for(i=0;i<numBlocks;i++)
  {
    output =
      vtkPointSet::SafeDownCast(mbOutput->GetBlock(i));
    if (output)
    {
      break;
    }
  }

  if (!output)
  {
    vtkErrorMacro(<<"No valid output block was found.");
    return;
  }

  const int bsize = b->GetNumberOfTuples();

  const int n = output->GetNumberOfPoints();

  if(shape->GetNumberOfPoints() != n)
  {
    vtkErrorMacro(<<"Input shape does not have the correct number of points");
    return;
  }

  double *shapevec = NewVector(n*3);

  // b is weighted by the eigenvals
  // make weigth vector for speed reasons
  double *w = NewVector(bsize);
  for (i = 0; i < bsize; i++) {
    w[i] =sqrt(this->Evals->GetValue(i)) * b->GetValue(i);
  }
  for (j = 0; j < n*3; j++) {
    shapevec[j] = meanshape[j];

    for (i = 0; i < bsize; i++) {
      shapevec[j] += w[i] * evecMat2[j][i];
    }
  }

  // Copy shape
  for (i = 0; i < n; i++) {
    shape->GetPoints()->SetPoint(i,shapevec[i*3  ], shapevec[i*3+1], shapevec[i*3+2]);
  }

  DeleteVector(shapevec);
  DeleteVector(w);
}

//----------------------------------------------------------------------------
// public
void vtkPCAAnalysisFilter::GetShapeParameters(vtkPointSet *shape, vtkFloatArray *b, int bsize)
{
  vtkPointSet* output = 0;

  vtkMultiBlockDataSet *mbOutput = this->GetOutput();

  int numBlocks = mbOutput->GetNumberOfBlocks();

  int i,j;

  for(i=0;i<numBlocks;i++)
  {
    output =
      vtkPointSet::SafeDownCast(mbOutput->GetBlock(i));
    if (output)
    {
      break;
    }
  }

  if (!output)
  {
    vtkErrorMacro(<<"No valid output block was found.");
    return;
  }

  // Local variant of b for fast access.
  double *bloc = NewVector(bsize);

  const int n = output->GetNumberOfPoints();

  if(shape->GetNumberOfPoints() != n)
  {
    vtkErrorMacro(<<"Input shape does not have the correct number of points");
    DeleteVector(bloc);
    return;
  }

  double *shapevec = NewVector(n*3);

  // Copy shape and subtract mean shape
  for (i = 0; i < n; i++)
  {
    double p[3];
    shape->GetPoint(i, p);
    shapevec[i*3  ] = p[0] - meanshape[i*3];
    shapevec[i*3+1] = p[1] - meanshape[i*3+1];
    shapevec[i*3+2] = p[2] - meanshape[i*3+2];
  }

  for (i = 0; i < bsize; i++)
  {
    bloc[i] = 0;

    // Project the shape onto eigenvector i
    for (j = 0; j < n*3; j++)
    {
      bloc[i] += shapevec[j] * evecMat2[j][i];
    }
  }

  // Return b in number of standard deviations
  b->SetNumberOfValues(bsize);
  for (i = 0; i < bsize; i++)
  {
    if (this->Evals->GetValue(i))
      b->SetValue(i, bloc[i]/sqrt(this->Evals->GetValue(i)));
    else
      b->SetValue(i, 0);
  }

  DeleteVector(shapevec);
  DeleteVector(bloc);
}


//----------------------------------------------------------------------------
// public
void vtkPCAAnalysisFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  this->Evals->PrintSelf(os,indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// public
int vtkPCAAnalysisFilter::GetModesRequiredFor(double proportion)
{
  int i;

  double eigen_total = 0.0F;
  for(i=0;i<this->Evals->GetNumberOfTuples();i++)
  {
    eigen_total += this->Evals->GetValue(i);
  }

  double running_total = 0.0F;
  for(i=0;i<this->Evals->GetNumberOfTuples();i++)
  {
    running_total += this->Evals->GetValue(i)/eigen_total;
    if(running_total>=proportion)
    {
      return i+1;
    }
  }

  return Evals->GetNumberOfTuples();
}
