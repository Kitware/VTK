/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineMeshWarp.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton (MINORI Project, Dental and Medical
             Informatics, Eastman Dental Institute, London, UK) who
             developed and contributed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkThinPlateSplineMeshWarp.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkThinPlateSplineMeshWarp* vtkThinPlateSplineMeshWarp::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkThinPlateSplineMeshWarp");
  if(ret)
    {
    return (vtkThinPlateSplineMeshWarp*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkThinPlateSplineMeshWarp;
}


vtkThinPlateSplineMeshWarp::vtkThinPlateSplineMeshWarp()
{
  this->SourceLandmarks=NULL;
  this->TargetLandmarks=NULL;
  this->Sigma=1.0;
}

vtkThinPlateSplineMeshWarp::~vtkThinPlateSplineMeshWarp()
{
  if (this->SourceLandmarks)
    {
    this->SourceLandmarks->Delete();
    }
  if (this->TargetLandmarks)
    {
    this->TargetLandmarks->Delete();
    }
}

//------------------------------------------------------------------------
// some dull matrix things

inline double** NewMatrix(int x,int y) 
{
  double** m = new double*[x];
  for(int i=0;i<x;i++) 
    {
    m[i] = new double[y];
    }
  return m;
}
inline void DeleteMatrix(double** m,int x,int y) 
{
  for(int i=0;i<x;i++) 
    {
    delete [] m[i]; // OK, we don't actually need y
    }
  delete [] m;
}
inline void FillMatrixWithZeros(double** m,int x,int y) 
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
inline void TransposeMatrix(double*** m,int x,int y) 
{
  double swap,*a,*b;
  int r,c;
  if(x==y)
    {
    // matrix is square, can just swap values over the diagonal (fast)
    for(c=1;c<x;c++)
      {
      for(r=0;r<c;r++) 
        {
        a = &((*m)[r][c]); // (what a mess)
        b = &((*m)[c][r]);
        swap=*a; 
        *a=*b; 
        *b=swap; 
        }
      }
    }
  else
    {
    // matrix is not square, must reallocate memory first
    double **result = NewMatrix(y,x);
    int r,c;
    for(r=0;r<y;r++)
      {
      for(c=0;c<x;c++)
        {
        result[r][c] = (*m)[c][r]; 
        }
      }
    // plug in the replacement matrix
    DeleteMatrix(*m,x,y);
    *m=result;
    }
}
inline void MatrixMultiply(double** a,double** b,double** c,int ar,int ac,
                           int br,int bc) 
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

//------------------------------------------------------------------------

// a teeny-tiny but rather crucial function
inline float U(float x,float sigma) {
  
  if(x==0.0F) 
    {
    return x; 
    }
  else 
    {
    return (x/sigma)*(x/sigma)*log(x/sigma); 
    }
}

//------------------------------------------------------------------------

void vtkThinPlateSplineMeshWarp::Execute()
{
  if(this->SourceLandmarks==NULL)
    {
    vtkWarningMacro(<<"No source landmarks - output will be empty");
    return;
    }
	
  if(this->TargetLandmarks==NULL)
    {
    vtkWarningMacro(<<"No target landmarks - output will be empty");
    return;
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
  double **W = NewMatrix(N+D+1,D); // will be transposed later, mind

  // build L
  FillMatrixWithZeros(L,N+D+1,N+D+1); // will leave the bottom-right corner with zeros
  int r,c;
  float p[3],p2[3];
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
    // fill in the top-left corner of L (K)
    for(c=0;c<N;c++)
      {
      this->SourceLandmarks->GetPoint(c,p2);
      L[r][c] = U(sqrt(vtkMath::Distance2BetweenPoints(p,p2)),this->Sigma);
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

  // solve for W
  double **LI = NewMatrix(N+D+1,N+D+1);
  vtkMath::InvertMatrix(L,LI,N+D+1);
  MatrixMultiply(LI,X,W,N+D+1,N+D+1,N+D+1,D);
  TransposeMatrix(&W,N+D+1,D);
  //W = Transpose(Inverse(L)*X); 
  // much easier if we have a decent matrix class, no?

  // resample input based on transform given by x' = Wu(x)
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  // copy the topology from the input mesh, as well as the point and cell
  // attributes
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  
  // create a new points structure for the output mesh
  vtkPoints *outputPoints = vtkPoints::New();
  output->SetPoints(outputPoints);
  outputPoints->SetNumberOfPoints(input->GetPoints()->GetNumberOfPoints());

  double **SOURCE = NewMatrix(D,1); // we know the target, we want the source
  double **UDX = NewMatrix(N+D+1,1);
  float *point;
  for(int j=0;j<input->GetPoints()->GetNumberOfPoints();j++)
    {
    point = input->GetPoints()->GetPoint(j);
    // build the N+D+1*1 vector UDX (Ud(x))
    for(int i=0;i<N;i++)
      {
      this->SourceLandmarks->GetPoint(i,p);
      UDX[i][0] = U(sqrt(vtkMath::Distance2BetweenPoints(point,p)),this->Sigma);
      }
    UDX[N][0] = 1.0;
    UDX[N+1][0] = point[0];
    UDX[N+2][0] = point[1];
    UDX[N+3][0] = point[2];
    // find the source point
    MatrixMultiply(W,UDX,SOURCE,D,N+D+1,N+D+1,1);
    //SOURCE = W*UDX; // would be so effortless...
    output->GetPoints()->SetPoint(j,SOURCE[0][0],SOURCE[1][0],SOURCE[2][0]);
    }

  DeleteMatrix(UDX,N+D+1,1);
  DeleteMatrix(SOURCE,D,1);
  DeleteMatrix(LI,N+D+1,N+D+1);
  DeleteMatrix(L,N+D+1,N+D+1);
  DeleteMatrix(X,N+D+1,D);
  DeleteMatrix(W,D,N+D+1); // W has been transposed since it was created
}

void vtkThinPlateSplineMeshWarp::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  
  os << indent << "Sigma:" << this->Sigma << "\n";
  os << indent << "Source Landmarks: " << this->SourceLandmarks << "\n";
  os << indent << "Target Landmarks: " << this->TargetLandmarks << "\n";
}

