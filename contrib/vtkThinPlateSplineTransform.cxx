/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class 
             based on code from vtkThinPlateSplineMeshWarp.cxx
	     written by Tim Hutton.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkThinPlateSplineTransform.h"
#include "vtkGeneralTransformInverse.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkThinPlateSplineTransform* vtkThinPlateSplineTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThinPlateSplineTransform");
  if(ret)
    {
    return (vtkThinPlateSplineTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThinPlateSplineTransform;
}

//------------------------------------------------------------------------
// some dull matrix things

static inline double** NewMatrix(int x, int y) 
{
  double** m = new double*[x];
  for(int i = 0; i < x; i++) 
    {
    m[i] = new double[y];
    }
  return m;
}

static inline void DeleteMatrix(double** m, int x, int vtkNotUsed(y)) 
{
  for(int i = 0; i < x; i++) 
    {
    delete [] m[i]; // OK, we don't actually need y
    }
  delete [] m;
}

static inline void FillMatrixWithZeros(double** m, int x, int y) 
{
  int i,j; 
  for(i = 0; i < x; i++) 
    {
    for(j = 0; j < y; j++) 
      {
      m[i][j] = 0.0;
      }
    }
}

static inline void MatrixMultiply(double** a, double** b, double** c,
				  int ar, int ac, int br, int bc) 
{
  if(ac != br) 
    {
    return;	// ac must equal br otherwise we can't proceed
    }
	
  // c must have size ar*bc (we assume this)
  const int cr = ar;
  const int cc = bc;
  int row,col,i;
  for(row = 0; row < cr; row++) 
    {
    for(col = 0; col < cc; col++) 
      {
      c[row][col] = 0.0;
      for(i = 0; i < ac; i++)
        {
        c[row][col] += a[row][i]*b[i][col];
        }
      }
    }
}

/* useful little debugging tool
static inline void PrintMatrix(double **m, int x, int y)
{
  int i,j; 
  for(i = 0; i < x; i++) 
    {
    for(j = 0; j < y; j++) 
      {
      cerr << m[i][j] << ((j != y-1) ? " " : "\n");
      }
    }
}
*/

//------------------------------------------------------------------------
vtkThinPlateSplineTransform::vtkThinPlateSplineTransform()
{
  this->TransformType = VTK_THINPLATESPLINE_TRANSFORM;

  this->SourceLandmarks=NULL;
  this->TargetLandmarks=NULL;
  this->Sigma = 1.0;

  // If the InverseFlag is set, then we use an iterative
  // method to invert the transformation.  
  // The InverseTolerance sets the precision to which we want to 
  // calculate the inverse.
  // The ApproximateInverse is a vtkThinPlateSlineTransform with
  // source & target landmarks swapped.  It provides the first
  // approximation in the iterative method.
  this->InverseFlag = 0;
  this->InverseTolerance = 0.001;
  this->ApproximateInverse = NULL;

  this->UpdateRequired = 0;
  this->NumberOfPoints = 0;
  this->MatrixW = NULL;
  this->UpdateMutex = vtkMutexLock::New();
}

//------------------------------------------------------------------------
vtkThinPlateSplineTransform::~vtkThinPlateSplineTransform()
{
  if (this->SourceLandmarks)
    {
    this->SourceLandmarks->Delete();
    }
  if (this->TargetLandmarks)
    {
    this->TargetLandmarks->Delete();
    }
  if (this->ApproximateInverse)
    {
    this->ApproximateInverse->Delete();
    }
  if (this->MatrixW)
    {
    DeleteMatrix(this->MatrixW,this->NumberOfPoints+3+1,3);
    this->MatrixW = NULL;
    }
  this->UpdateMutex->Delete();
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::SetSourceLandmarks(vtkPoints *source)
{
  if (this->SourceLandmarks == source)
    {
    return;
    }

  if (this->SourceLandmarks)
    {
    this->SourceLandmarks->Delete();
    }

  source->Register(this);
  this->SourceLandmarks = source;

  this->Modified();

  this->UpdateRequired = 1;
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::SetTargetLandmarks(vtkPoints *target)
{
  if (this->TargetLandmarks == target)
    {
    return;
    }

  if (this->TargetLandmarks)
    {
    this->TargetLandmarks->Delete();
    }

  target->Register(this);
  this->TargetLandmarks = target;
  this->Modified();

  this->UpdateRequired = 1;
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::TransformPoint(const float input[3],
						 float output[3])
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  if (this->NumberOfPoints != 0)
    {
    if (this->InverseFlag)
      {
      this->InverseTransformPoint(input,output);
      }
    else
      {
      this->ForwardTransformPoint(input,output);
      }
    }
  else
    {
    output[0] = input[0];
    output[0] = input[0];
    output[0] = input[0];
    }
}

//----------------------------------------------------------------------------
void vtkThinPlateSplineTransform::TransformPoints(vtkPoints *in, 
						  vtkPoints *out)
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  float outPoint[3];

  int i;
  int n = in->GetNumberOfPoints();

  if (this->NumberOfPoints == 0)
    {
    for (i = 0; i < n; i++)
      {
      out->InsertNextPoint(in->GetPoint(i));
      }
    return;
    }      

  if (this->InverseFlag)
    {
    for (i = 0; i < n; i++)
      {
      this->InverseTransformPoint(in->GetPoint(i),outPoint);
      out->InsertNextPoint(outPoint);
      }
    }
  else
    {
    for (i = 0; i < n; i++)
      {
      this->ForwardTransformPoint(in->GetPoint(i),outPoint);
      out->InsertNextPoint(outPoint);
      }
    }
}

//----------------------------------------------------------------------------
// transform the normals using the derivative of the transformation

static void LinearSolve3x3(float A[3][3], float x[3]);
static void Multiply3x3(float A[3][3], float x[3]);
static void Transpose3x3(float A[3][3]);

void vtkThinPlateSplineTransform::TransformNormals(vtkPoints *inPts,
					     vtkPoints *vtkNotUsed(outPts),
						   vtkNormals *inNms, 
						   vtkNormals *outNms)
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  float matrix[3][3];
  float point[3];
  float normal[3];

  int i;
  int n = inNms->GetNumberOfNormals();

  for (i = 0; i < n; i++)
    {
    inNms->GetNormal(i,normal);
    inPts->GetPoint(i,point);

    this->TransformDerivatives(point,point,matrix);
    Transpose3x3(matrix);

    if (this->InverseFlag)
      {
      Multiply3x3(matrix,normal);
      }
    else
      {
      LinearSolve3x3(matrix,normal);
      }

    vtkMath::Normalize(normal);
    outNms->InsertNextNormal(normal);
    }
}

//----------------------------------------------------------------------------
// transform vectors using the derivative of the transformation

void vtkThinPlateSplineTransform::TransformVectors(vtkPoints *inPts,
					     vtkPoints *vtkNotUsed(outPts),
						   vtkVectors *inVrs, 
						   vtkVectors *outVrs)
{
  if (this->AutoUpdate)
    {
    this->Update();
    }

  float matrix[3][3];
  float vect[3];
  float point[3];

  int i;
  int n = inVrs->GetNumberOfVectors();

  for (i = 0; i < n; i++)
    {
    inVrs->GetVector(i,vect);
    inPts->GetPoint(i,point);

    this->TransformDerivatives(point,point,matrix);

    if (this->InverseFlag)
      {
      LinearSolve3x3(matrix,vect);
      }
    else
      {
      Multiply3x3(matrix,vect);
      }

    outVrs->InsertNextVector(vect);
    }
}

//------------------------------------------------------------------------
unsigned long vtkThinPlateSplineTransform::GetMTime()
{
  unsigned long result = this->vtkGeneralTransform::GetMTime();
  unsigned long mtime;

  if (this->SourceLandmarks)
    {
    mtime = this->SourceLandmarks->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }
  if (this->TargetLandmarks)
    {
    mtime = this->TargetLandmarks->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }
  return result;
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::Update()
{
  // Lock so that threads don't collide.  The first thread will succeed
  // in the update, the other threads will wait until the lock is released
  // and then find that the update has already been done.
  this->UpdateMutex->Lock();

  if (this->SourceLandmarks == NULL || this->TargetLandmarks == NULL)
    {
    // gotta ensure thread safety with those instance variables...
    if (this->MatrixW)
      {
      DeleteMatrix(this->MatrixW,this->NumberOfPoints+3+1,3);
      }
    this->MatrixW = NULL;
    this->NumberOfPoints = 0;
    this->UpdateMutex->Unlock();
    return;
    }

  if (this->UpdateTime.GetMTime() > this->GetMTime() && 
      this->UpdateRequired == 0)
    {
    // already up-to-date!
    this->UpdateMutex->Unlock();
    return;
    }

  if (this->SourceLandmarks->GetNumberOfPoints() !=
      this->TargetLandmarks->GetNumberOfPoints())
    {
    vtkErrorMacro("Update: Source and Target Landmarks contain a different number of points");
    return;
    }

  // If we are calculating inverse transforms, then update 
  // ApproximateInverse
  if (this->InverseFlag)
    {
    if (this->ApproximateInverse == NULL)
      {
      this->ApproximateInverse = vtkThinPlateSplineTransform::New();
      }
    this->ApproximateInverse->SetSourceLandmarks(this->TargetLandmarks);
    this->ApproximateInverse->SetTargetLandmarks(this->SourceLandmarks);
    this->ApproximateInverse->SetSigma(this->Sigma);
    this->ApproximateInverse->Update();
    this->ApproximateInverse->AutoUpdateOff();
    }

  // Notation and inspiration from:
  // Fred L. Bookstein (1997) "Shape and the Information in Medical Images: 
  // A Decade of the Morphometric Synthesis" Computer Vision and Image 
  // Understanding 66(2):97-118
  // and online work published by Tim Cootes (http://www.wiau.man.ac.uk/~bim)
	
  const int N = this->SourceLandmarks->GetNumberOfPoints();
  const int D = 3; // dimensions

  // the input matrices
  double **L = NewMatrix(N+D+1,N+D+1);
  double **X = NewMatrix(N+D+1,D);
  // the output weights matrix
  double **W = NewMatrix(N+D+1,D); 

  // build L
  // will leave the bottom-right corner with zeros
  FillMatrixWithZeros(L,N+D+1,N+D+1);
  int r,c;
  float p[3],p2[3];
  float dx,dy,dz;
  for(r = 0; r < N; r++)
    {
    this->SourceLandmarks->GetPoint(r,p);
    // fill in the top-right and bottom-left corners of L (Q)
    L[N][r] = L[r][N] = 1.0;
    L[N+1][r] = L[r][N+1] = p[0];
    L[N+2][r] = L[r][N+2] = p[1];
    L[N+3][r] = L[r][N+3] = p[2];
    // fill in the top-left corner of L (K), using symmetry
    for(c = 0; c < r; c++)
      {
      this->SourceLandmarks->GetPoint(c,p2);
      dx = p[0]-p2[0]; dy = p[1]-p2[1]; dz = p[2]-p2[2];
      L[r][c] = L[c][r] = sqrt(dx*dx + dy*dy + dz*dz)/this->Sigma;
      }
    }

  // build X
  FillMatrixWithZeros(X,N+D+1,D);
  for(r = 0; r < N; r++)
    {
    this->TargetLandmarks->GetPoint(r,p);
    X[r][0] = p[0];
    X[r][1] = p[1];
    X[r][2] = p[2];
    }

  // solve for W, where W = Inverse(L)*X; 

  // use thread-safe version of InvertMatrix
  double **LI = NewMatrix(N+D+1,N+D+1);
  int *tmpInt = new int[N+D+1];
  double *tmpDbl = new double[N+D+1];
  vtkMath::InvertMatrix(L,LI,N+D+1,tmpInt,tmpDbl);
  delete [] tmpInt;
  delete [] tmpDbl;

  MatrixMultiply(LI,X,W,N+D+1,N+D+1,N+D+1,D);

  DeleteMatrix(LI,N+D+1,N+D+1);
  DeleteMatrix(L,N+D+1,N+D+1);
  DeleteMatrix(X,N+D+1,D);

  if (this->MatrixW)
    {
    DeleteMatrix(this->MatrixW,this->NumberOfPoints+D+1,D);
    }
  this->MatrixW = W;
  this->NumberOfPoints = N;

  this->UpdateTime.Modified();
  this->UpdateRequired = 0;

  this->UpdateMutex->Unlock();
}

//------------------------------------------------------------------------
// The matrix W was created by Update.  Not much has to be done to
// apply the transform:  do an affine transformation, then do
// perturbations based on the landmarks.
void vtkThinPlateSplineTransform::ForwardTransformPoint(const float point[3],
							float output[3])
{
  int N = this->NumberOfPoints;
  double **W = this->MatrixW;
  double *C = this->MatrixW[N]; 
  double **A = &this->MatrixW[N+1];

  float dx,dy,dz;
  float p[3];
  double U;

  double x = 0, y = 0, z = 0; 

  // do the nonlinear stuff
  for(int i = 0; i < N; i++)
    {
    this->SourceLandmarks->GetPoint(i,p);
    dx = point[0]-p[0]; dy = point[1]-p[1]; dz = point[2]-p[2];
    U = sqrt(dx*dx + dy*dy + dz*dz)/this->Sigma;
    x += U*W[i][0];
    y += U*W[i][1];
    z += U*W[i][2];
    }

  // finish off with the affine transformation
  x += C[0] + point[0]*A[0][0] + point[1]*A[1][0] + point[2]*A[2][0];
  y += C[1] + point[0]*A[0][1] + point[1]*A[1][1] + point[2]*A[2][1];
  z += C[2] + point[0]*A[0][2] + point[1]*A[1][2] + point[2]*A[2][2];

  output[0] = x;
  output[1] = y;
  output[2] = z;
}

//----------------------------------------------------------------------------
// calculate the thin plate spline as well as the derivatives
void vtkThinPlateSplineTransform::TransformDerivatives(const float point[3],
						       float output[3],
						       float derivatives[3][3])
{
  int N = this->NumberOfPoints;
  double **W = this->MatrixW;
  double *C = this->MatrixW[N]; 
  double **A = &this->MatrixW[N+1];

  float dx,dy,dz;
  float p[3];
  double r, f, U, Ux, Uy, Uz;
  double x = 0, y = 0, z = 0; 

  derivatives[0][0] = derivatives[0][1] = derivatives[0][2] = 0;
  derivatives[1][0] = derivatives[1][1] = derivatives[1][2] = 0;
  derivatives[2][0] = derivatives[2][1] = derivatives[2][2] = 0;

  // do the nonlinear stuff
  for(int i = 0; i < N; i++)
    {
    this->SourceLandmarks->GetPoint(i,p);
    dx = point[0]-p[0]; dy = point[1]-p[1]; dz = point[2]-p[2];
    r = sqrt(dx*dx + dy*dy + dz*dz);
    f = 0;
    if (r != 0)
      {
      f = 1.0/(r*this->Sigma);
      }

    U = r/this->Sigma;

    Ux = f*dx;
    Uy = f*dy;
    Uz = f*dz;

    x += U*W[i][0];
    y += U*W[i][1];
    z += U*W[i][2];

    derivatives[0][0] += Ux*W[i][0];
    derivatives[0][1] += Uy*W[i][0];
    derivatives[0][2] += Uz*W[i][0];
    derivatives[1][0] += Ux*W[i][1];
    derivatives[1][1] += Uy*W[i][1];
    derivatives[1][2] += Uz*W[i][1];
    derivatives[2][0] += Ux*W[i][2];
    derivatives[2][1] += Uy*W[i][2];
    derivatives[2][2] += Uz*W[i][2];
    }

  // finish with the affine transformation
  x += C[0] + point[0]*A[0][0] + point[1]*A[1][0] + point[2]*A[2][0];
  y += C[1] + point[0]*A[0][1] + point[1]*A[1][1] + point[2]*A[2][1];
  z += C[2] + point[0]*A[0][2] + point[1]*A[1][2] + point[2]*A[2][2];

  output[0] = x;
  output[1] = y;
  output[2] = z;

  derivatives[0][0] += A[0][0];
  derivatives[0][1] += A[1][0];
  derivatives[0][2] += A[2][0];
  derivatives[1][0] += A[0][1];
  derivatives[1][1] += A[1][1];
  derivatives[1][2] += A[2][1];
  derivatives[2][0] += A[0][2];
  derivatives[2][1] += A[1][2];
  derivatives[2][2] += A[2][2];
}  

//----------------------------------------------------------------------------
// helper function, swap two 3-vectors
template <class T>
static inline void SwapVectors(T v1[3], T v2[3])
{
  T tmpvec[3];
  memcpy(tmpvec,v1,3*sizeof(T));
  memcpy(v1,v2,3*sizeof(T));
  memcpy(v2,tmpvec,3*sizeof(T));
}

//----------------------------------------------------------------------------
// Unrolled LU factorization of a 3x3 matrix with pivoting.
// This decomposition is non-standard in that the diagonal
// elements are inverted, to convert a division to a multiplication
// in the backsubstitution.
static void LUFactor3x3(float A[3][3], int index[3])
{
  int i,maxI;
  float tmp,largest;
  float scale[3];

  // Loop over rows to get implicit scaling information

  for ( i = 0; i < 3; i++ ) 
    {
    largest =  fabs(A[i][0]);
    if ((tmp = fabs(A[i][1])) > largest)
      {
      largest = tmp;
      }
    if ((tmp = fabs(A[i][2])) > largest)
      {
      largest = tmp;
      }
    scale[i] = 1.0/largest;
    }
  
  // Loop over all columns using Crout's method

  // first column
  largest = scale[0]*fabs(A[0][0]);
  maxI = 0;
  if ((tmp = scale[1]*fabs(A[1][0])) >= largest) 
    {
    largest = tmp;
    maxI = 1;
    }
  if ((tmp = scale[2]*fabs(A[2][0])) >= largest) 
    {
    maxI = 2;
    }
  if (maxI != 0) 
    {
    SwapVectors(A[maxI],A[0]);
    scale[maxI] = scale[0];
    }
  index[0] = maxI;

  A[0][0] = 1.0/A[0][0];
  A[1][0] *= A[0][0];
  A[2][0] *= A[0][0];
    
  // second column
  A[1][1] -= A[1][0]*A[0][1];
  A[2][1] -= A[2][0]*A[0][1];
  largest = scale[1]*fabs(A[1][1]);
  maxI = 1;
  if ((tmp = scale[2]*fabs(A[2][1])) >= largest) 
    {
    maxI = 2;
    SwapVectors(A[2],A[1]);
    scale[2] = scale[1];
    }
  index[1] = maxI;
  A[1][1] = 1.0/A[1][1];
  A[2][1] *= A[1][1];

  // third column
  A[1][2] -= A[1][0]*A[0][2];
  A[2][2] -= A[2][0]*A[0][2] + A[2][1]*A[1][2];
  largest = scale[2]*fabs(A[2][2]);
  index[2] = 2;
  A[2][2] = 1.0/A[2][2];
}

//----------------------------------------------------------------------------
// Backsubsitution with an LU-decomposed matrix.  This is the standard
// LU decomposition, except that the diagonals elements have been inverted.
static void LUSolve3x3(float A[3][3], int index[3], float x[3])
{
  float sum;

  // forward substitution
  
  sum = x[index[0]];
  x[index[0]] = x[0];
  x[0] = sum;

  sum = x[index[1]];
  x[index[1]] = x[1];
  x[1] = sum - A[1][0]*x[0];

  sum = x[index[2]];
  x[index[2]] = x[2];
  x[2] = sum - A[2][0]*x[0] - A[2][1]*x[1];

  // back substitution
  
  x[2] = x[2]*A[2][2];
  x[1] = (x[1] - A[1][2]*x[2])*A[1][1];
  x[0] = (x[0] - A[0][1]*x[1] - A[0][2]*x[2])*A[0][0];
}  

//----------------------------------------------------------------------------
// helper function for newton's method: solves Ay = x for y, 
// the result is placed in x, the matrix A is destroyed
static void LinearSolve3x3(float A[3][3], float x[3])
{
  int index[3];

  LUFactor3x3(A,index);
  LUSolve3x3(A,index,x);
}

//----------------------------------------------------------------------------
static void Multiply3x3(float A[3][3], float v[3])
{
  float x,y,z;
  x = v[0]; y = v[1]; z = v[2];

  v[0] = A[0][0]*x + A[0][1]*y + A[0][2]*z;
  v[1] = A[1][0]*x + A[1][1]*y + A[1][2]*z;
  v[2] = A[2][0]*x + A[2][1]*y + A[2][2]*z;
}

//----------------------------------------------------------------------------
static void Transpose3x3(float A[3][3])
{
  float tmp;
  tmp = A[0][1];
  A[1][0] = A[0][1];
  A[0][1] = tmp;
  tmp = A[0][2];
  A[2][0] = A[0][2];
  A[0][2] = tmp;
  tmp = A[1][2];
  A[2][1] = A[1][2];
  A[1][2] = tmp;
}

//----------------------------------------------------------------------------
// Simply switching the input & output landmarks will not invert the 
// transform, so instead we use Newton's method to iteratively invert
// the transformation.
void vtkThinPlateSplineTransform::InverseTransformPoint(const float point[3], 
							float outPoint[3])
{
  float inverse[3];
  float delta[3];
  float derivatives[3][3];

  double errorSquared;
  double toleranceSquared = this->InverseTolerance*this->InverseTolerance;
 
  // first guess at inverse point
  this->ApproximateInverse->TransformPoint(point,inverse);

  int n = 10;
  int i;
  // do a maximum ten steps of iteration
  for (i = 0; i < n; i++)
    {    
    // put the inverse point back through the transform
    this->TransformDerivatives(inverse,delta,derivatives);

    // how far off are we?
    delta[0] -= point[0];
    delta[1] -= point[1];
    delta[2] -= point[2];

    // here is the critical step in Newton's method
    LinearSolve3x3(derivatives,delta);

    inverse[0] -= delta[0];
    inverse[1] -= delta[1];
    inverse[2] -= delta[2];

    errorSquared = delta[0]*delta[0] +
                   delta[1]*delta[1] +
                   delta[2]*delta[2];

    if (errorSquared < toleranceSquared) 
      { // hit tolerance: exit
      break;
      }
    }

  if (i >= n)
    {
    this->ApproximateInverse->TransformPoint(point,inverse);
    vtkWarningMacro(<< "InverseTransformPoint: no convergence");
    }    
  outPoint[0] = inverse[0];
  outPoint[1] = inverse[1];
  outPoint[2] = inverse[2];
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGeneralTransform::PrintSelf(os,indent);
  
  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
  os << indent << "InverseTolerance: " << this->InverseTolerance << "\n";
  os << indent << "Sigma: " << this->Sigma << "\n";
  os << indent << "Source Landmarks: " << this->SourceLandmarks << "\n";
  if (this->SourceLandmarks)
    {
    this->SourceLandmarks->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "Target Landmarks: " << this->TargetLandmarks << "\n";
  if (this->TargetLandmarks)
    {
    this->TargetLandmarks->PrintSelf(os,indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkThinPlateSplineTransform::Identity()
{
  this->SetSourceLandmarks(NULL);
  this->SetTargetLandmarks(NULL);
}

//----------------------------------------------------------------------------
void vtkThinPlateSplineTransform::Inverse()
{
  this->InverseFlag = !this->InverseFlag;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkThinPlateSplineTransform::MakeTransform()
{
  return vtkThinPlateSplineTransform::New(); 
}

//----------------------------------------------------------------------------
void vtkThinPlateSplineTransform::DeepCopy(vtkGeneralTransform *transform)
{
  if (transform->GetTransformType() & VTK_INVERSE_TRANSFORM)
    {
    transform = 
      ((vtkGeneralTransformInverse *)transform)->GetInverseTransform(); 
    }	
  if (this->TransformType != transform->GetTransformType())
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    }
  vtkThinPlateSplineTransform *t = (vtkThinPlateSplineTransform *)transform;

  if (t == this)
    {
    return;
    }

  if (this->InverseFlag != t->InverseFlag)
    {
    this->Inverse();
    }
  this->SetInverseTolerance(t->InverseTolerance);
  this->SetSigma(t->Sigma);
  this->SetSourceLandmarks(t->SourceLandmarks);
  this->SetTargetLandmarks(t->TargetLandmarks);
}


