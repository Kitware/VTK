/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLine.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPolyLine.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"


//------------------------------------------------------------------------------
vtkPolyLine* vtkPolyLine::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPolyLine");
  if(ret)
    {
    return (vtkPolyLine*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPolyLine;
}




vtkPolyLine::vtkPolyLine()
{
  this->Line = vtkLine::New();
}

vtkPolyLine::~vtkPolyLine()
{
  this->Line->Delete();
}

vtkCell *vtkPolyLine::MakeObject()
{
  vtkCell *cell = vtkPolyLine::New();
  cell->DeepCopy(this);
  return cell;
}

// Given points and lines, compute normals to lines. These are not true 
// normals, they are "orientation" normals used by classes like vtkTubeFilter
// that control the rotation around the line. The normals try to stay pointing
// in the same direction as much as possible (i.e., minimal rotation).
int vtkPolyLine::GenerateSlidingNormals(vtkPoints *pts, vtkCellArray *lines,
					vtkDataArray *normals)
{
  vtkIdType npts;
  vtkIdType *linePts;
  float sPrev[3], sNext[3], q[3], w[3], normal[3], theta;
  float p[3], pNext[3];
  float c[3], f1, f2;
  int i, j, largeRotation;

  //  Loop over all lines
  // 
  for (lines->InitTraversal(); lines->GetNextCell(npts,linePts); )
    {
    //  Determine initial starting normal
    // 
    if ( npts <= 0 )
      {
      continue;
      }

    else if ( npts == 1 ) //return arbitrary
      {
      normal[0] = normal[1] = 0.0;
      normal[2] = 1.0;
      normals->InsertTuple(linePts[0],normal);
      }

    else //more than one point
      {
      //  Compute first normal. All "new" normals try to point in the same 
      //  direction.
      //
      for (j=0; j<npts; j++) 
        {

        if ( j == 0 ) //first point
          {
          pts->GetPoint(linePts[0],p);
          pts->GetPoint(linePts[1],pNext);

          for (i=0; i<3; i++) 
            {
            sPrev[i] = pNext[i] - p[i];
            sNext[i] = sPrev[i];
            }

          if ( vtkMath::Normalize(sNext) == 0.0 )
            {
            vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

	  // the following logic will produce a normal orthogonal
	  // to the first line segment. If we have three points
	  // we use special logic to select a normal orthogonal
	  // too the first two line segments
	  if (npts > 2)
	    {
	    float ftmp[3];
	    
	    pts->GetPoint(linePts[2],ftmp);
            for (i=0; i<3; i++) 
              {
              ftmp[i] = ftmp[i] - pNext[i];
              }
            if ( vtkMath::Normalize(ftmp) == 0.0 )
              {
              vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
              return 0;
              }
	    // now the starting normal should simply be the cross product
	    // in the following if statement we check for the case where
	    /// the first three points are colinear 
            vtkMath::Cross(sNext,ftmp,normal);
	    }
          if ((npts <= 2)|| (vtkMath::Normalize(normal) == 0.0)) 
	    {
	    for (i=0; i<3; i++) 
	      {
	      // a little trick to find othogonal normal
	      if ( sNext[i] != 0.0 ) 
		{
		normal[(i+2)%3] = 0.0;
		normal[(i+1)%3] = 1.0;
		normal[i] = -sNext[(i+1)%3]/sNext[i];
		break;
		}
	      }
	    }
          vtkMath::Normalize(normal);
          normals->InsertTuple(linePts[0],normal);
          }

        else if ( j == (npts-1) ) //last point; just insert previous
          {
          normals->InsertTuple(linePts[j],normal);
          }

        else //inbetween points
          {
          //  Generate normals for new point by projecting previous normal
          for (i=0; i<3; i++)
            {
            p[i] = pNext[i];
            }
          pts->GetPoint(linePts[j+1],pNext);

          for (i=0; i<3; i++) 
            {
            sPrev[i] = sNext[i];
            sNext[i] = pNext[i] - p[i];
            }

          if ( vtkMath::Normalize(sNext) == 0.0 )
            {
            vtkErrorMacro(<<"Coincident points in polyline...can't compute normals");
            return 0;
            }

          //compute rotation vector
          vtkMath::Cross(sPrev,normal,w);
          if ( vtkMath::Normalize(w) == 0.0 ) 
            {
            vtkErrorMacro(<<"normal and sPrev coincident");
            return 0;
            }

          //see whether we rotate greater than 90 degrees.
          if ( vtkMath::Dot(sPrev,sNext) < 0.0 )
            {
            largeRotation = 1;
            }
          else
            {
            largeRotation = 0;
            }

          //compute rotation of line segment
          vtkMath::Cross (sNext, sPrev, q);
          if ( (theta=asin((double)vtkMath::Normalize(q))) == 0.0 ) 
            { //no rotation, use previous normal
            normals->InsertTuple(linePts[j],normal);
            continue;
            }
          if ( largeRotation )
            {
            if ( theta > 0.0 )
              {
              theta = vtkMath::Pi() - theta;
              }
            else
              {
              theta = -vtkMath::Pi() - theta;
              }
            }

          // new method
          for (i=0; i<3; i++)
            {
            c[i] = sNext[i] + sPrev[i];
            }
          vtkMath::Normalize(c);
          f1 = vtkMath::Dot(q,normal);
          f2 = 1.0 - f1*f1;
          if (f2 > 0.0)
            {
            f2 = sqrt(1.0 - f1*f1);
            }
          else
            {
            f2 = 0.0;
            }
          vtkMath::Cross(c,q,w);
          vtkMath::Cross(sPrev,q,c);
          if (vtkMath::Dot(normal,c)*vtkMath::Dot(w,c) < 0)
            {
            f2 = -1.0*f2;
            }
          for (i=0; i<3; i++)
            {
            normal[i] = f1*q[i] + f2*w[i];
            }
          
          normals->InsertTuple(linePts[j],normal);
          }//for this point
        }//else
      }//else if
    }//for this line
  return 1;
}

int vtkPolyLine::EvaluatePosition(float x[3], float* closestPoint,
                                 int& subId, float pcoords[3], 
                                 float& minDist2, float *weights)
{
  float closest[3];
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float lineWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  return_status = 0;
  weights[0] = 0.0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<this->Points->GetNumberOfPoints()-1; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));
    status = this->Line->EvaluatePosition(x,closest,ignoreId,pc,
                                          dist2,lineWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      return_status = status;
      if (closestPoint)
	{
	closestPoint[0] = closest[0]; 
	closestPoint[1] = closest[1]; 
	closestPoint[2] = closest[2]; 
	}
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      weights[i] = lineWeights[0];
      weights[i+1] = lineWeights[1];
      }
    else
      {
      weights[i+1] = 0.0;
      }
    }

  return return_status;
}

void vtkPolyLine::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                   float *weights)
{
  int i;
  float *a1 = this->Points->GetPoint(subId);
  float *a2 = this->Points->GetPoint(subId+1);

  for (i=0; i<3; i++) 
    {
    x[i] = a1[i] + pcoords[0]*(a2[i] - a1[i]);
    }
  
  weights[0] = 1.0 - pcoords[0];
  weights[1] = pcoords[0];
}

int vtkPolyLine::CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
{
  pts->SetNumberOfIds(1);

  if ( pcoords[0] >= 0.5 )
    {
    pts->SetId(0,this->PointIds->GetId(subId+1));
    if ( pcoords[0] > 1.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else
    {
    pts->SetId(0,this->PointIds->GetId(subId));
    if ( pcoords[0] < 0.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
}

void vtkPolyLine::Contour(float value, vtkDataArray *cellScalars,
                          vtkPointLocator *locator, vtkCellArray *verts, 
                          vtkCellArray *lines, vtkCellArray *polys, 
                          vtkPointData *inPd, vtkPointData *outPd,
                          vtkCellData *inCd, vtkIdType cellId,
                          vtkCellData *outCd)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkDataArray *lineScalars=cellScalars->MakeObject();
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    if ( outPd )
      {
      this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
      this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));
      }

    lineScalars->SetTuple(0,cellScalars->GetTuple(i));
    lineScalars->SetTuple(1,cellScalars->GetTuple(i+1));

    this->Line->Contour(value, lineScalars, locator, verts,
                       lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
  lineScalars->Delete();
}

// Intersect with sub-lines
//
int vtkPolyLine::IntersectWithLine(float p1[3], float p2[3],float tol,float& t,
                                  float x[3], float pcoords[3], int& subId)
{
  int subTest, numLines=this->Points->GetNumberOfPoints() - 1;

  for (subId=0; subId < numLines; subId++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

    if ( this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}

int vtkPolyLine::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                             vtkPoints *pts)
{
  int numLines=this->Points->GetNumberOfPoints() - 1;
  pts->Reset();
  ptIds->Reset();

  for (int subId=0; subId < numLines; subId++)
    {
    pts->InsertNextPoint(this->Points->GetPoint(subId));
    ptIds->InsertNextId(this->PointIds->GetId(subId));

    pts->InsertNextPoint(this->Points->GetPoint(subId+1));
    ptIds->InsertNextId(this->PointIds->GetId(subId+1));
    }

  return 1;
}

void vtkPolyLine::Derivatives(int subId, float pcoords[3], float *values, 
                              int dim, float *derivs)
{
  this->Line->PointIds->SetNumberOfIds(2);

  this->Line->Points->SetPoint(0,this->Points->GetPoint(subId));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(subId+1));

  this->Line->Derivatives(0, pcoords, values+dim*subId, dim, derivs);
}

void vtkPolyLine::Clip(float value, vtkDataArray *cellScalars, 
                       vtkPointLocator *locator, vtkCellArray *lines,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                       int insideOut)
{
  int i, numLines=this->Points->GetNumberOfPoints() - 1;
  vtkFloatArray *lineScalars=vtkFloatArray::New();
  lineScalars->SetNumberOfTuples(2);

  for ( i=0; i < numLines; i++)
    {
    this->Line->Points->SetPoint(0,this->Points->GetPoint(i));
    this->Line->Points->SetPoint(1,this->Points->GetPoint(i+1));

    this->Line->PointIds->SetId(0,this->PointIds->GetId(i));
    this->Line->PointIds->SetId(1,this->PointIds->GetId(i+1));

    lineScalars->SetComponent(0,0,cellScalars->GetComponent(i,0));
    lineScalars->SetComponent(1,0,cellScalars->GetComponent(i+1,0));

    this->Line->Clip(value, lineScalars, locator, lines, inPd, outPd, 
                    inCd, cellId, outCd, insideOut);
    }
  
  lineScalars->Delete();
}

// Return the center of the point cloud in parametric coordinates.
int vtkPolyLine::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = 0.5; pcoords[1] = pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints() - 1) / 2);
}




// Ellipse fitting code


static void ROTATE(double **a, int i, int j, int k, int l, double tau, double s);
static void jacobi(double **a, int n, double *d, double **v, int nrot);
static void choldc(double **a, int n, double **l);
static int inverse(double **TB, double **InvB, int N);
static void AperB(double **_A, double **_B, double **_res, int _righA, int _colA, int _righB, int _colB);
static void A_TperB(double **_A, double **_B, double **_res, int _righA, int _colA, int _righB, int _colB);
static void AperB_T(double **_A, double **_B, double **_res, int _righA, int _colA, int _righB, int _colB);
static double **AllocateMatrix ( int rows, int columns );
static void DeallocateMatrix ( double** m, int rows );


// Fit an ellipse, setting the proper values in the parameters array.
float* vtkPolyLine::FitEllipseStatic ( vtkPoints* points, int xindex, int yindex )
{
  static float Parameters[6];
  
  // Create the best fit ellipse for this point data
  int i;
  int NumberOfPoints = points->GetNumberOfPoints();
  double point[3];

  int np = points->GetNumberOfPoints();           // number of points

  for ( i = 0; i < 6; i++ )
    {
    Parameters[i] = 0.0;
    }
  if ( np < 6 )
    {
    vtkGenericWarningMacro ( << "GetEllipseParameters requires 6 or more points" );
    return Parameters;
    }
  if ( xindex < 0 || xindex > 2 || yindex < 0 || yindex > 2 || xindex == yindex )
    {
    vtkGenericWarningMacro ( << "GetEllipseParameters: xindex and yindex are out of range, or the same" );
    return Parameters;
    }
  
  double **D = AllocateMatrix ( np+1, 7 );
  double **S = AllocateMatrix ( 7,7 );
  double **Const  = AllocateMatrix ( 7,7 );
  double **temp = AllocateMatrix ( 7,7 );
  double **L = AllocateMatrix ( 7, 7 );
  double **C = AllocateMatrix ( 7, 7 );
  double **invL = AllocateMatrix ( 7, 7 );
  double **V = AllocateMatrix ( 7, 7 );
  double **sol = AllocateMatrix ( 7, 7 );
  double *d = new double[7];
  double *pvec = new double[7];
  int nrot=0;

  // Constraints
  Const[1][3]=-2;
  Const[2][2]=1;
  Const[3][1]=-2;	

  // Fill in the Design matrix
  for ( i = 0; i < NumberOfPoints; i++ )
    {
    points->GetPoint ( i, point );
    D[i+1][1] = point[xindex] * point[xindex];
    D[i+1][2] = point[xindex] * point[yindex];
    D[i+1][3] = point[yindex] * point[yindex];
    D[i+1][4] = point[xindex];
    D[i+1][5] = point[yindex];
    D[i+1][6] = 1;
    }
  
  //pm(Const,"Constraint");
  // Now compute scatter matrix  S
  A_TperB(D,D,S,np,6,np,6);
  //pm(S,"Scatter");
  
  choldc(S,6,L);    
  //pm(L,"Cholesky");
  
  inverse(L,invL,6);
  //pm(invL,"inverse");
  
  AperB_T(Const,invL,temp,6,6,6,6);
  AperB(invL,temp,C,6,6,6,6);
  //pm(C,"The C matrix");
	
  jacobi(C,6,d,V,nrot);
  //pm(V,"The Eigenvectors");  /* OK */
  //pv(d,"The eigevalues");
	
  A_TperB(invL,V,sol,6,6,6,6);
  //pm(sol,"The GEV solution unnormalized");  /* SOl */

  // Now normalize them 
  for (int j=1;j<=6;j++)  /* Scan columns */
    {
    double mod = 0.0;
    for (int i=1;i<=6;i++)
      mod += sol[i][j]*sol[i][j];
    for (int i=1;i<=6;i++)
      sol[i][j] /=  sqrt(mod); 
    }

  //pm(sol,"The GEV solution");  /* SOl */
	
  double zero=10e-20;
  int  solind=0;
  for (i=1; i<=6; i++)
    {
    if (d[i]<0 && fabs(d[i])>zero)
      {
      solind = i;
      }
    }
  // Now fetch the right solution
  for (int j=1;j<=6;j++)
    {
    Parameters[j - 1] = sol[j][solind];
    }
  //pv(pvec,"the solution");

  DeallocateMatrix ( D, np+1 );
  DeallocateMatrix ( S, 7 );
  DeallocateMatrix ( Const, 7 );
  DeallocateMatrix ( temp, 7 );
  DeallocateMatrix ( L, 7 );
  DeallocateMatrix ( C, 7 );
  DeallocateMatrix ( invL, 7 );
  DeallocateMatrix ( V, 7 );
  DeallocateMatrix ( sol, 7 );
  delete[] d;
  delete[] pvec;
  return Parameters;
}


// Supporting code
inline double tjcbrt(double a) { return((a<0)?-exp(log(-a)/3):exp(log(a)/3)); }
static double **AllocateMatrix ( int rows, int columns )
{
  int i, j;
  double **m;
  m = new double*[rows];
  for ( i = 0; i < rows; i++ )
    {
    m[i] = new double[columns];
    for ( j = 0; j < columns; j++ )
      {
      m[i][j] = 0.0;
      }
    }
  
  return m;
}

static void DeallocateMatrix ( double** m, int rows )
{
  int i;
  for ( i = 0; i < rows; i++ )
    {
    delete[] m[i];
    }
  delete[] m;
}



static void ROTATE(double **a, int i, int j, int k, int l, double tau, double s) 
{
  double g,h;
  g=a[i][j];
  h=a[k][l];
  a[i][j]=g-s*(h+g*tau);
  a[k][l]=h+s*(g-h*tau);
}
    
static void jacobi(double **a, int n, double *d , double **v, int nrot)      
{
  int j,iq,ip,i;
  double tresh,theta,tau,t,sm,s,h,g,c;
  
  double *b = new double[n+1];
  double *z = new double[n+1];
  
  for (ip=1;ip<=n;ip++)
    {
    for (iq=1;iq<=n;iq++) v[ip][iq]=0.0;
    v[ip][ip]=1.0;
    }
  for (ip=1;ip<=n;ip++)
    {
    b[ip]=d[ip]=a[ip][ip];
    z[ip]=0.0;
    }
  nrot=0;
  for (i=1;i<=50;i++)
    {
    sm=0.0;
    for (ip=1;ip<=n-1;ip++)
      {
      for (iq=ip+1;iq<=n;iq++)
	{
	sm += fabs(a[ip][iq]);
	}
      }
    if (sm == 0.0)
      {
      delete b;
      delete z;
      /*    free_vector(z,1,n);
	    free_vector(b,1,n);  */
      return;
      }
    if (i < 4)
      {
      tresh=0.2*sm/(n*n);
      }
    else
      {
      tresh=0.0;
      }
    for (ip=1;ip<=n-1;ip++)
      {
      for (iq=ip+1;iq<=n;iq++)
	{
	g=100.0* fabs(a[ip][iq]);
	if (i > 4 && fabs(d[ip])+g == fabs(d[ip])
	    && fabs(d[iq])+g == fabs(d[iq]))
	  {
	  a[ip][iq]=0.0;
	  }
	else if (fabs(a[ip][iq]) > tresh)
	  {
	  h=d[iq]-d[ip];
	  if (fabs(h)+g == fabs(h))
	    {
	    t=(a[ip][iq])/h;
	    }
	  else
	    {
	    theta=0.5*h/(a[ip][iq]);
	    t=1.0/(fabs(theta)+ sqrt(1.0+theta*theta));
	    if (theta < 0.0) t = -t;
	    }
	  c=1.0/sqrt(1+t*t);
	  s=t*c;
	  tau=s/(1.0+c);
	  h=t*a[ip][iq];
	  z[ip] -= h;
	  z[iq] += h;
	  d[ip] -= h;
	  d[iq] += h;
	  a[ip][iq]=0.0;
	  for (j=1;j<=ip-1;j++)
	    {
	    ROTATE(a,j,ip,j,iq,tau,s);
	    }
	  for (j=ip+1;j<=iq-1;j++)
	    {
	    ROTATE(a,ip,j,j,iq,tau,s);
	    }
	  for (j=iq+1;j<=n;j++)
	    {
	    ROTATE(a,ip,j,iq,j,tau,s);
	    }
	  for (j=1;j<=n;j++)
	    {
	    ROTATE(v,j,ip,j,iq,tau,s);
	    }
	  ++nrot;
	  }
	}
      }
    for (ip=1;ip<=n;ip++)
      {
      b[ip] += z[ip];
      d[ip]=b[ip];
      z[ip]=0.0;
      }
    }
  delete d;
  delete z;
  //printf("Too many iterations in routine JACOBI");
}
    

//  Perform the Cholesky decomposition    
// Return the lower triangular L  such that L*L'=A  
static void choldc(double **a, int n, double **l)
{
  int i,j,k;
  double sum;
  double *p = new double[n+1];
  
  for (i=1; i<=n; i++)
    {
    for (j=i; j<=n; j++)
      {
      for (sum=a[i][j],k=i-1;k>=1;k--) sum -= a[i][k]*a[j][k];
      if (i == j)
	{
	if (sum<=0.0)  
		// printf("\nA is not poitive definite!");
	  {
	  }
	else
	  {
	  p[i]=sqrt(sum);
	  }
	}
      else 
	{
	a[j][i]=sum/p[i];
	}
      }
    }       
  for (i=1; i<=n; i++)
    {
    for (j=i; j<=n; j++)
      {
      if (i==j)
	{
	l[i][i] = p[i];
	}
      else
	{
	l[j][i]=a[j][i];  
	l[i][j]=0.0;
	}
      }
    }
  delete p;
}


/********************************************************************/
/**    Calcola la inversa della matrice  B mettendo il risultato   **/
/**    in InvB . Il metodo usato per l'inversione e' quello di     **/
/**    Gauss-Jordan.   N e' l'ordine della matrice .               **/
/**    ritorna 0 se l'inversione  corretta altrimenti ritorna     **/
/**    SINGULAR .                                                  **/
/********************************************************************/
static int inverse(double **TB, double **InvB, int N)
{  
  int k,i,j,p,q;
  double mult;
  double D,temp;
  double maxpivot;
  int npivot;
  double **B = AllocateMatrix ( N+1,N+2 );
  double **A = AllocateMatrix ( N+1,2*N+2 );
  double **C = AllocateMatrix ( N+1,N+1 );
  double eps = 10e-20;
  
      
  for(k=1;k<=N;k++)
    {
    for(j=1;j<=N;j++)
      {
      B[k][j]=TB[k][j];
      }
    }
      
  for (k=1;k<=N;k++)
    {
    for (j=1;j<=N+1;j++)
      A[k][j]=B[k][j];
    for (j=N+2;j<=2*N+1;j++)
      A[k][j]=(float)0;
    A[k][k-1+N+2]=(float)1;
    }
  for (k=1;k<=N;k++)
    {
    maxpivot=fabs((double)A[k][k]);
    npivot=k;
    for (i=k;i<=N;i++)
      if (maxpivot<fabs((double)A[i][k]))
	{
	maxpivot=fabs((double)A[i][k]);
	npivot=i;
	}
    if (maxpivot>=eps)
      {
      if (npivot!=k)
	for (j=k;j<=2*N+1;j++)
	  {
	  temp=A[npivot][j];
	  A[npivot][j]=A[k][j];
	  A[k][j]=temp;
	  } ;
      D=A[k][k];
      for (j=2*N+1;j>=k;j--)
	A[k][j]=A[k][j]/D;
      for (i=1;i<=N;i++)
	{
	if (i!=k)
	  {
	  mult=A[i][k];
	  for (j=2*N+1;j>=k;j--)
	    A[i][j]=A[i][j]-mult*A[k][j] ;
	  }
	}
      }
    else
      {
      // printf("\n The matrix may be singular !!") ;
      DeallocateMatrix ( B, N+1 );
      DeallocateMatrix ( A, N+1 );
      DeallocateMatrix ( C, N+1 );
      return(-1);
      }
    }
  /**   Copia il risultato nella matrice InvB  ***/
  for (k=1,p=1;k<=N;k++,p++)
    {
    for (j=N+2,q=1;j<=2*N+1;j++,q++)
      {
      InvB[p][q]=A[k][j];
      }
    }
  DeallocateMatrix ( B, N+1 );
  DeallocateMatrix ( A, N+1 );
  DeallocateMatrix ( C, N+1 );
  return(0);
}            /*  End of INVERSE   */
    

    
static void AperB(double **_A, double **_B, double **_res, 
	   int _righA, int _colA, int _righB, int _colB)
{
  int p,q,l;                                      
  for (p=1;p<=_righA;p++)                        
    for (q=1;q<=_colB;q++)                        
      {
      _res[p][q]=0.0;                            
      for (l=1;l<=_colA;l++)                     
	_res[p][q]=_res[p][q]+_A[p][l]*_B[l][q];  
      }                                            
}                                                 

static void A_TperB(double **_A, double  **_B, double **_res,
	     int _righA, int _colA, int _righB, int _colB)
{
  int p,q,l;                                      
  for (p=1;p<=_colA;p++)                        
    for (q=1;q<=_colB;q++)                        
      { _res[p][q]=0.0;                            
      for (l=1;l<=_righA;l++)                    
	_res[p][q]=_res[p][q]+_A[l][p]*_B[l][q];  
      }                                            
}

static void AperB_T(double **_A, double **_B, double **_res,
	     int _righA, int _colA, int _righB, int _colB)
{
  int p,q,l;                                      
  for (p=1;p<=_colA;p++)                         
    for (q=1;q<=_colB;q++)                        
      { _res[p][q]=0.0;                            
      for (l=1;l<=_righA;l++)                    
	_res[p][q]=_res[p][q]+_A[p][l]*_B[q][l];  
      }                                            
}


float* vtkPolyLine::ConvertEllipseToImplicitStatic ( float Parameters[6] )
{
  static float * Implicit = NULL;

  if ( Implicit == NULL )
    {
    Implicit = new float[6];
    }
  float solution[6];
  float CenterX = 0.0, CenterY = 0.0, MajorAxis = 0.0, MinorAxis = 0.0, Orientation = 0.0;
  float norm = 0.0;
  float div = 0.0;
  float tmp = 0.0;
  float trace = 0.0, det = 0.0, disc = 0.0, a2 = 0.0, b2 = 0.0;
  int i;
  Implicit[0] = Implicit[1] = Implicit[2] = Implicit[3] = Implicit[4] = 0.0;
  for ( i = 0; i < 6; i++ )
    {
    solution[i] = Parameters[i];
    }

  norm = solution[0]*solution[2]*solution[5] + (solution[1]*solution[3]*solution[4] - solution[0]*solution[4]*solution[4] - solution[2]*solution[3]*solution[3] - solution[5]*solution[1]*solution[1])/4.0;

  if( norm >= 0.0 )
    {
    norm = tjcbrt(norm);
    }
  else
    {
    norm = -fabs( tjcbrt(norm) );
    }
  if ( norm == 0.0 )
    {
    return Implicit;
    }
  
  for(i=0;i<6;i++)
    {
    solution[i] /= norm;
    }

  // The centre of the ellipse is defined as (CenterX,CenterY)

  div = 4.0*solution[0]*solution[2] - solution[1]*solution[1];
  CenterX = (solution[4]*solution[1] - 2.0*solution[2]*solution[3])/div;
  CenterY = (solution[3]*solution[1] - 2.0*solution[0]*solution[4])/div;

  // The semi-major and semi-minor axes are defined as (MajorAxis,MinorAxis)
  trace = solution[0] + solution[2];
  det = solution[0]*solution[2] - solution[1]*solution[1]/4.0;
  disc = sqrt(trace*trace - 4.0*det);
  a2 = (-trace+disc)/(2.0*det*det);
  b2 = (-trace-disc)/(2.0*det*det);
  MajorAxis = sqrt(a2);  MinorAxis = sqrt(b2);
  
  // The orientation of the major axes with respect to the
  // x axis, theta is given as follows (0<=theta<180):
  float alpha,beta,cos_theta,theta1,theta2;

  
  alpha = - tjcbrt(b2/(a2*a2));
  beta = - tjcbrt(a2/(b2*b2));
  tmp = (alpha*solution[0] - beta*solution[2])/(alpha*alpha-beta*beta);
  if ( tmp < 0.0 )
    {
    tmp = -tmp;
    }
  cos_theta = sqrt( tmp );
  theta1 = acos(cos_theta);
  theta2 = acos(-cos_theta);

  // Make sure that theta1 is less than theta2. In fact, theta1
  // must be in the range 0->90 and theta2 90->180. We choose
  // the correct solution depending upon the sign of solution[1].
  // *** NOTE THE SIGN CONVENTION FOR THE IMPLICITELLIPSE ***
  Orientation = 0.0;
  if( theta1 > theta2 )
    {
    tmp = theta2;
    theta2 = theta1;
    theta1 = tmp;
    }
  if( solution[1] > 0.0 )
    {
    Orientation = - theta1;
    }
  else if( solution[1] < 0.0 )
    {
    Orientation = - theta2;
    }
  else
    {
    Orientation = 0.0;
    }
  
  Implicit[0] = CenterX;
  Implicit[1] = CenterY;
  Implicit[2] = MajorAxis;
  Implicit[3] = MinorAxis;
  Implicit[4] = Orientation;
  // cout << "a2: " << a2 << " b2 " << b2 << " cos_theta: " << cos_theta << " theta1: " << theta1 << " theta2: " << theta2 << endl;
  // cout << "alpha: " << alpha << " beta: " << beta << endl;
  // cout << "Orientation: " << Orientation << " Implicit[4]: " << Implicit[4] << endl;
  return Implicit;
  
}
