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
#include "vtkGeneralTransformInverse.h"
#include "vtkThinPlateSplineTransform.h"
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

//------------------------------------------------------------------------
vtkThinPlateSplineTransform::vtkThinPlateSplineTransform()
{
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
  this->InverseTolerance = 0.001;
  this->ApproximateInverse = NULL;

  this->Basis = -1;
  this->SetBasisToR();

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

  // update ApproximateInverse
  if (this->ApproximateInverse == NULL)
    {
    this->ApproximateInverse = vtkThinPlateSplineTransform::New();
    }
  this->ApproximateInverse->SetSourceLandmarks(this->TargetLandmarks);
  this->ApproximateInverse->SetTargetLandmarks(this->SourceLandmarks);
  this->ApproximateInverse->SetSigma(this->Sigma);

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

  int q,c;
  float p[3],p2[3];
  float dx,dy,dz;
  double r;
  double (*phi)(double) = this->RadialBasisFunction;

  for(q = 0; q < N; q++)
    {
    this->SourceLandmarks->GetPoint(q,p);
    // fill in the top-right and bottom-left corners of L (Q)
    L[N][q] = L[q][N] = 1.0;
    L[N+1][q] = L[q][N+1] = p[0];
    L[N+2][q] = L[q][N+2] = p[1];
    L[N+3][q] = L[q][N+3] = p[2];
    // fill in the top-left corner of L (K), using symmetry
    for(c = 0; c < q; c++)
      {
      this->SourceLandmarks->GetPoint(c,p2);
      dx = p[0]-p2[0]; dy = p[1]-p2[1]; dz = p[2]-p2[2];
      r = sqrt(dx*dx + dy*dy + dz*dz);
      L[q][c] = L[c][q] = phi(r/this->Sigma);
      }
    }

  // build X
  FillMatrixWithZeros(X,N+D+1,D);
  for(q = 0; q < N; q++)
    {
    this->TargetLandmarks->GetPoint(q,p);
    X[q][0] = p[0];
    X[q][1] = p[1];
    X[q][2] = p[2];
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

  if (N == 0)
    {
    output[0] = point[0];
    output[1] = point[1];
    output[2] = point[2];
    return;
    }

  double **W = this->MatrixW;
  double *C = this->MatrixW[N]; 
  double **A = &this->MatrixW[N+1];

  float dx,dy,dz;
  float p[3];
  double U,r;
  double invSigma = 1.0/this->Sigma;

  double (*phi)(double) = this->RadialBasisFunction;

  double x = 0, y = 0, z = 0; 

  // do the nonlinear stuff
  for(int i = 0; i < N; i++)
    {
    this->SourceLandmarks->GetPoint(i,p);
    dx = point[0]-p[0]; dy = point[1]-p[1]; dz = point[2]-p[2];
    r = sqrt(dx*dx + dy*dy + dz*dz);
    U = phi(r*invSigma);
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
// calculate the thin plate spline as well as the derivative
void vtkThinPlateSplineTransform::ForwardTransformDerivative(
						       const float point[3],
						       float output[3],
						       float derivative[3][3])
{
  int N = this->NumberOfPoints;

  if (N == 0)
    {
    for (int i = 0; i < 3; i++)
      {
      output[i] = point[i];
      derivative[i][0] = derivative[i][1] = derivative[i][2] = 0.0;
      derivative[i][i] = 1.0;
      }
    return;
    }

  double **W = this->MatrixW;
  double *C = this->MatrixW[N]; 
  double **A = &this->MatrixW[N+1];

  float dx,dy,dz;
  float p[3];
  double r, U, f, Ux, Uy, Uz;
  double x = 0, y = 0, z = 0; 
  double invSigma = 1.0/this->Sigma;

  double (*phi)(double, double&) = this->RadialBasisDerivative;

  derivative[0][0] = derivative[0][1] = derivative[0][2] = 0;
  derivative[1][0] = derivative[1][1] = derivative[1][2] = 0;
  derivative[2][0] = derivative[2][1] = derivative[2][2] = 0;

  // do the nonlinear stuff
  for(int i = 0; i < N; i++)
    {
    this->SourceLandmarks->GetPoint(i,p);
    dx = point[0]-p[0]; dy = point[1]-p[1]; dz = point[2]-p[2];
    r = sqrt(dx*dx + dy*dy + dz*dz);

    // get both U and its derivative and do the sigma-mangling
    U = 0;
    f = 0;
    if (r != 0)
      {
      U = phi(r*invSigma,f);
      f *= invSigma/r;
      }

    Ux = f*dx;
    Uy = f*dy;
    Uz = f*dz;

    x += U*W[i][0];
    y += U*W[i][1];
    z += U*W[i][2];

    derivative[0][0] += Ux*W[i][0];
    derivative[0][1] += Uy*W[i][0];
    derivative[0][2] += Uz*W[i][0];
    derivative[1][0] += Ux*W[i][1];
    derivative[1][1] += Uy*W[i][1];
    derivative[1][2] += Uz*W[i][1];
    derivative[2][0] += Ux*W[i][2];
    derivative[2][1] += Uy*W[i][2];
    derivative[2][2] += Uz*W[i][2];
    }

  // finish with the affine transformation
  x += C[0] + point[0]*A[0][0] + point[1]*A[1][0] + point[2]*A[2][0];
  y += C[1] + point[0]*A[0][1] + point[1]*A[1][1] + point[2]*A[2][1];
  z += C[2] + point[0]*A[0][2] + point[1]*A[1][2] + point[2]*A[2][2];

  output[0] = x;
  output[1] = y;
  output[2] = z;

  derivative[0][0] += A[0][0];
  derivative[0][1] += A[1][0];
  derivative[0][2] += A[2][0];
  derivative[1][0] += A[0][1];
  derivative[1][1] += A[1][1];
  derivative[1][2] += A[2][1];
  derivative[2][0] += A[0][2];
  derivative[2][1] += A[1][2];
  derivative[2][2] += A[2][2];
}  

//----------------------------------------------------------------------------
// Simply switching the input & output landmarks will not invert the 
// transform, so instead we use Newton's method to iteratively invert
// the transformation.
void vtkThinPlateSplineTransform::InverseTransformPoint(const float point[3], 
							float output[3])
{
  if (this->NumberOfPoints == 0)
    {
    output[0] = point[0];
    output[1] = point[1];
    output[2] = point[2];
    return;
    }

  float inverse[3];
  float delta[3];
  float derivative[3][3];

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
    this->ForwardTransformDerivative(inverse,delta,derivative);

    // how far off are we?
    delta[0] -= point[0];
    delta[1] -= point[1];
    delta[2] -= point[2];

    // here is the critical step in Newton's method
    vtkGeneralTransform::LinearSolve3x3(derivative,delta,delta);

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

  output[0] = inverse[0];
  output[1] = inverse[1];
  output[2] = inverse[2];
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::PrintSelf(vtkOstream& os, vtkIndent indent)
{
  vtkWarpTransform::PrintSelf(os,indent);
  
  os << indent << "InverseTolerance: " << this->InverseTolerance << "\n";
  os << indent << "Sigma: " << this->Sigma << "\n";
  os << indent << "Basis: " << this->GetBasisAsString() << "\n";
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
vtkGeneralTransform *vtkThinPlateSplineTransform::MakeTransform()
{
  return vtkThinPlateSplineTransform::New(); 
}

//----------------------------------------------------------------------------
void vtkThinPlateSplineTransform::DeepCopy(vtkGeneralTransform *transform)
{
  if (strcmp("vtkGeneralTransformInverse",transform->GetClassName()) == 0)
    {
    transform = ((vtkGeneralTransformInverse *)transform)->GetTransform();
    }
  if (strcmp("vtkThinPlateSplineTransform",transform->GetClassName()) != 0)
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    return;
    }

  vtkThinPlateSplineTransform *t = (vtkThinPlateSplineTransform *)transform;

  if (t == this)
    {
    return;
    }

  this->SetInverseTolerance(t->InverseTolerance);
  this->SetSigma(t->Sigma);
  this->SetBasis(t->GetBasis());
  this->SetSourceLandmarks(t->SourceLandmarks);
  this->SetTargetLandmarks(t->TargetLandmarks);

  if (this->InverseFlag != t->InverseFlag)
    {
    this->InverseFlag = t->InverseFlag;
    this->Modified();
    }
}

//------------------------------------------------------------------------
// a very basic radial basis function
static double RBFr(double r)
{
  return r;
}

// calculate both phi(r) its derivative wrt r
static double RBFDRr(double r, double &dUdr)
{
  dUdr = 1;
  return r;
}

//------------------------------------------------------------------------
// the standard 2D thin plate spline basis function
static double RBFr2logr(double r)
{
  return r*r*log(r);
}

// calculate both phi(r) its derivative wrt r
static double RBFDRr2logr(double r, double &dUdr)
{
  double tmp = log(r);
  dUdr = r*(1+2*tmp);
  return r*r*tmp;
}

//------------------------------------------------------------------------
void vtkThinPlateSplineTransform::SetBasis(int basis)
{
  if (basis == this->Basis)
    {
    return;
    }

  switch (basis)
    {
    case VTK_RBF_R:
      this->RadialBasisFunction = &RBFr;
      this->RadialBasisDerivative = &RBFDRr;
      break;
    case VTK_RBF_R2LOGR:
      this->RadialBasisFunction = &RBFr2logr;
      this->RadialBasisDerivative = &RBFDRr2logr;
      break;
    default:
      vtkErrorMacro(<< "SetBasisFunction: Unrecognized basis function");
      break;
    }

  this->Basis = basis;
  this->Modified();
}  

//------------------------------------------------------------------------
const char *vtkThinPlateSplineTransform::GetBasisAsString()
{
  switch (this->Basis)
    {
    case VTK_RBF_R:
      return "R";
    case VTK_RBF_R2LOGR:
      return "R2LogR";
     }
  return "Unknown";
}








