/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class 
             based on code from vtkThinPlateSplineMeshWarp.cxx
	     written Tim Hutton.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

static inline double** NewMatrix(int x,int y) 
{
  double** m = new double*[x];
  for(int i=0;i<x;i++) 
    {
    m[i] = new double[y];
    }
  return m;
}
static inline void DeleteMatrix(double** m,int x,int vtkNotUsed(y)) 
{
  for(int i=0;i<x;i++) 
    {
    delete [] m[i]; // OK, we don't actually need y
    }
  delete [] m;
}
static inline void FillMatrixWithZeros(double** m,int x,int y) 
{
  int i,j; 
  for(i=0;i<x;i++) 
    {
    for(j=0;j<y;j++) 
      {
      m[i][j]=0.0;
      }
    }
}
static inline void MatrixMultiply(double** a,double** b,double** c,
				  int ar,int ac,int br,int bc) 
{
  if(ac!=br) 
    {
    return;	// ac must equal br otherwise we can't proceed
    }
	
  // c must have size ar*bc (we assume this)
  const int cr=ar,cc=bc;
  int row,col,i;
  for(row=0;row<cr;row++) 
    {
    for(col=0;col<cc;col++) 
      {
      c[row][col]=0.0;
      for(i=0;i<ac;i++)
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
  for(i=0;i<x;i++) 
    {
    for(j=0;j<y;j++) 
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
  this->MatrixWMutex = vtkMutexLock::New();
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
  if (this->MatrixW)
    {
    DeleteMatrix(this->MatrixW,this->NumberOfPoints+3+1,3);
    this->MatrixW = NULL;
    }
  this->MatrixWMutex->Delete();
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
  if (this->SourceLandmarks==NULL || this->TargetLandmarks==NULL)
    {
    // gotta ensure thread safety with those instance variables...
    this->MatrixWMutex->Lock();
    if (this->MatrixW)
      {
      DeleteMatrix(this->MatrixW,this->NumberOfPoints+3+1,3);
      }
    this->MatrixW = NULL;
    this->NumberOfPoints = 0;
    this->MatrixWMutex->Unlock();
    return;
    }

  if (this->UpdateTime.GetMTime() > this->GetMTime() && 
      this->UpdateRequired == 0)
    {
    // already up-to-date! 
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
  for(r=0;r<N;r++)
    {
    this->SourceLandmarks->GetPoint(r,p);
    // fill in the top-right corner of L (Q)
    L[r][N] = 1.0;
    L[r][N+1] = p[0];
    L[r][N+2] = p[1];
    L[r][N+3] = p[2];
    // fill in the bottom-left corner of L (Q transposed)
    L[N][r] = 1.0;
    L[N+1][r] = p[0];
    L[N+2][r] = p[1];
    L[N+3][r] = p[2];
    // fill in the top-left corner of L (K), using symmetry
    for(c=0;c<r;c++)
      {
      this->SourceLandmarks->GetPoint(c,p2);
      dx = p[0]-p2[0]; dy = p[1]-p2[1]; dz = p[2]-p2[2];
      L[r][c] = L[c][r] = sqrt(dx*dx + dy*dy + dz*dz)/this->Sigma;
      }
    }

  // build X
  FillMatrixWithZeros(X,N+D+1,D);
  for(r=0;r<N;r++)
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

  // ensure thread safety for instance variables
  this->MatrixWMutex->Lock();
  if (this->MatrixW)
    {
    DeleteMatrix(this->MatrixW,this->NumberOfPoints+D+1,D);
    }
  this->MatrixW = W;
  this->NumberOfPoints = N;
  this->MatrixWMutex->Unlock();

  this->UpdateTime.Modified();
  this->UpdateRequired = 0;
}

//------------------------------------------------------------------------
// The matrix W was created by Update.  Not much has to be done to
// apply the transform:  do an affine transformation, then do
// perturbations based on the landmarks.
void vtkThinPlateSplineTransform::ForwardTransformPoint(const float point[3],
							float output[3])
{
  // make sure another thread doesn't mess us up by Updating MatrixW
  // while we are using it to compute a transformation...
  this->MatrixWMutex->Lock();

  int N = this->NumberOfPoints;
  double **W = this->MatrixW;
  double *C = this->MatrixW[N]; 
  double **A = &this->MatrixW[N+1];

  float x,y,z;
  float dx,dy,dz;
  float p[3];
  double U;

  // start with the affine transformation
  x = C[0] + point[0]*A[0][0] + point[1]*A[1][0] + point[2]*A[2][0];
  y = C[1] + point[0]*A[0][1] + point[1]*A[1][1] + point[2]*A[2][1];
  z = C[2] + point[0]*A[0][2] + point[1]*A[1][2] + point[2]*A[2][2];

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

  output[0] = x;
  output[1] = y;
  output[2] = z;

  this->MatrixWMutex->Unlock();
}

//----------------------------------------------------------------------------
// calculate the thin plate spline as well as the derivatives
void vtkThinPlateSplineTransform::TransformDerivatives(const float point[3],
						       float output[3],
						       float derivatives[3][3])
{
  // make sure another thread doesn't mess us up by Updating MatrixW
  // while we are using it to compute a transformation...
  this->MatrixWMutex->Lock();

  int N = this->NumberOfPoints;
  double **W = this->MatrixW;
  double *C = this->MatrixW[N]; 
  double **A = &this->MatrixW[N+1];

  float dx,dy,dz;
  float p[3];
  double r, f, U, Ux, Uy, Uz;

  // start with the affine transformation
  output[0] = C[0] + point[0]*A[0][0] + point[1]*A[1][0] + point[2]*A[2][0];
  output[1] = C[1] + point[0]*A[0][1] + point[1]*A[1][1] + point[2]*A[2][1];
  output[2] = C[2] + point[0]*A[0][2] + point[1]*A[1][2] + point[2]*A[2][2];

  derivatives[0][0] = A[0][0];
  derivatives[0][1] = A[1][0];
  derivatives[0][2] = A[2][0];
  derivatives[1][0] = A[0][1];
  derivatives[1][1] = A[1][1];
  derivatives[1][2] = A[2][1];
  derivatives[2][0] = A[0][2];
  derivatives[2][1] = A[1][2];
  derivatives[2][2] = A[2][2];

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

    output[0] += U*W[i][0];
    output[1] += U*W[i][1];
    output[2] += U*W[i][2];

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

  this->MatrixWMutex->Unlock();
}  

//----------------------------------------------------------------------------
// helper function for newton's method: solves Ay = x for y
static void LinearSolve3x3(float A[3][3], float x[3], float y[3])
{
  int index[3];
  double xtmp[3];
  double AD[3][3];
  double *Atmp[3];

  AD[0][0] = A[0][0];
  AD[0][1] = A[0][1];
  AD[0][2] = A[0][2];
  AD[1][0] = A[1][0];
  AD[1][1] = A[1][1];
  AD[1][2] = A[1][2];
  AD[2][0] = A[2][0];
  AD[2][1] = A[2][1];
  AD[2][2] = A[2][2];

  Atmp[0] = AD[0];
  Atmp[1] = AD[1];
  Atmp[2] = AD[2];

  xtmp[0] = x[0];
  xtmp[1] = x[1];
  xtmp[2] = x[2];

  vtkMath::LUFactorLinearSystem(Atmp,index,3);
  vtkMath::LUSolveLinearSystem(Atmp,index,xtmp,3);

  y[0] = xtmp[0];
  y[1] = xtmp[1];
  y[2] = xtmp[2];
}

//----------------------------------------------------------------------------
// Simply switching the input & output landmarks will not invert the 
// transform, so instead we use Newton's method to iteratively invert
// the transformation.
void vtkThinPlateSplineTransform::InverseTransformPoint(const float point[3], 
							float outPoint[3])
{
  float inverse[3];
  float trypoint[3];
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
    this->TransformDerivatives(inverse,trypoint,derivatives);

    // how far off are we?
    trypoint[0] -= point[0];
    trypoint[1] -= point[1];
    trypoint[2] -= point[2];

    // here is the critical step in Newton's method
    LinearSolve3x3(derivatives,trypoint,delta);

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


