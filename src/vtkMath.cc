/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>
#include <stdlib.h>
#include <iostream.h>
#include "vtkMath.hh"

long vtkMath::Seed = 1177; // One authors home address

//
// some constants we need
//
#define K_A 16807
#define K_M 2147483647			/* Mersenne prime 2^31 -1 */
#define K_Q 127773			/* K_M div K_A */
#define K_R 2836			/* K_M mod K_A */

// Description:
// Generate random numbers between 0.0 and 1.0
// This is used to provide portability across different systems.
//
// Based on code in "Random Number Generators: Good Ones are Hard to Find",
// by Stephen K. Park and Keith W. Miller in Communications of the ACM,
// 31, 10 (Oct. 1988) pp. 1192-1201.
//
// Borrowed from: Fuat C. Baran, Columbia University, 1988

float vtkMath::Random()
{
  long hi, lo;
    
  hi = this->Seed / K_Q;
  lo = this->Seed % K_Q;
  if ((this->Seed = K_A * lo - K_R * hi) <= 0) Seed += K_M;
  return ((float) this->Seed / K_M);
}

// Description:
// Initialize seed value. NOTE: Random() has the bad property that 
// the first random number returned after RandomSeed() is called 
// is proportional to the seed value! To help solve this, I call 
// RandomSeed() a few times inside seed. This doesn't ruin the 
// repeatability of Random().
//
void vtkMath::RandomSeed(long s)
{
  this->Seed = s;

  vtkMath::Random();
  vtkMath::Random();
  vtkMath::Random();
}

// Description:
// Cross product of two 3-vectors. Result vector in z[3].
void vtkMath::Cross(float x[3], float y[3], float z[3])
{
  float Zx = x[1]*y[2] - x[2]*y[1]; 
  float Zy = x[2]*y[0] - x[0]*y[2];
  float Zz = x[0]*y[1] - x[1]*y[0];
  z[0] = Zx; z[1] = Zy; z[2] = Zz; 
}

#define FABS(x) fabs(x)
#define SIGN(a,b) ((b) >= 0 ? (a) : -(a))
#define PYTHAG(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
                     (ct=bt/at,at*sqrt(1+ct*ct)) : \
                     (bt!=0.0 ? (ct=at/bt,bt*sqrt(1+ct*ct)): 0))
#define MAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
                  (maxarg1) : (maxarg2))

// Description:
// Solve linear equations robustly using method of single value decomposition.
// See "Numerical Recipes" by Press et al. This method performs the 
// decomposition; use the method "SingularValueBackSubstitution" to actually
// solve Ax=B. This method creates the decomposition a = U*W*V. Note that the
// vector W are the eigenvalues; the columns of V are the eigenvectors.
void vtkMath::SingularValueDecomposition(double **a, int m, int n, 
                                        double *w, double **v)
{
  static double at, bt, ct;                       // double should be enough
  static double maxarg1, maxarg2;                // space for any Type?

  int flag, i, its, j, jj, k, l, nm;
  double c, f, h, s, x, y, z;
  double anorm=0, g=0, scale=0;
  double *rv1 = new double[n];

  // Householder reduction to bidiagonal form
  for (i=0; i<n; i++) 
    {
    l = i + 1;
    rv1[i] = scale*g;
    g = s = scale = 0.0;
    if (i < m) 
      {
      for (k=i; k<m; k++) scale += FABS(a[k][i]);
      if (scale != 0.0) 
        {
        for (k=i;k<m;k++) 
          {
          a[k][i] /= scale;
          s += a[k][i]*a[k][i];
          }
        f = a[i][i];
        g = -SIGN(sqrt(s),f);
        h = f*g - s;
        a[i][i] = f - g;
        if (i != n) 
          {
          for (j=l; j<n; j++) 
            {
            for (s=0,k=i; k<m; k++) s += a[k][i]*a[k][j];
            f = s/h;
            for (k=i; k<m; k++) a[k][j] += f*a[k][i];
            }
          }
        for (k=i; k<m; k++) a[k][i] = (a[k][i] * scale);
        }
      }

    w[i] = scale*g;
    g=s=scale=0;
    if (i < m && i != n) 
      {
      for (k=l; k<n; k++) scale += FABS(a[i][k]);
      if (scale != 0.0) 
        {
        for (k=l;k<n;k++) 
          {
          a[i][k] = a[i][k] / scale;
          s += a[i][k]*a[i][k];
          }
        f = a[i][l];
        g =  -SIGN(sqrt(s),f);
        h = f*g - s;
        a[i][l] = f-g;
        for (k=l; k<n; k++) rv1[k] = a[i][k]/h;
        if (i != m) 
          {
          for (j=l;j<m;j++) 
            {
            for (s=0,k=l; k<n; k++) s += a[j][k]*a[i][k];
            for (k=l; k<n; k++) a[j][k] += s*rv1[k];
            }
          }
        for (k=l; k<n; k++) a[i][k] = a[i][k] * scale;
        }
      }
    anorm = MAX(anorm,(FABS(w[i])+FABS(rv1[i])));
    }

  // Accumulation of right-hand transform V.
  for (i=n-1; i>=0; i--) 
    {
    if (i < n) 
      {
      if (g != 0.0) 
        {
        for (j=l;j<n;j++)
          v[j][i] = (a[i][j]/a[i][l])/g;  // double div to avoid -nderflow
        for (j=l; j<n; j++) 
          {
          for (s=0,k=l; k<n; k++) s += a[i][k]*v[k][j];
          for (k=l; k<n; k++) v[k][j] += s*v[k][i];
          }
        }
      for (j=l; j<n; j++) v[i][j] = v[j][i] = 0.0;
      }
    v[i][i] = 1.0;
    g = rv1[i];
    l = i;
    }

  // Accumulation of left-hand transform U.
  for (i=n-1;i>=0;i--) 
    {
    l = i+1;
    g = w[i];
    if (i < n) for (j=l; j<n; j++) a[i][j]=0;
    if ( g != 0.0) 
      {
      g = 1/g;
      if (i != n) 
        {
        for (j=l;j<n;j++) 
          {
          for (s=0,k=l; k<m; k++) s += a[k][i]*a[k][j];
          f = (s/a[i][i])*g;
          for (k=i; k<m; k++) a[k][j] += f*a[k][i];
          }
        }
      for (j=i; j<m; j++) a[j][i] *= g;
      } 
    else 
      {
      for (j=i; j<m; j++) a[j][i]=0;
      }
    ++a[i][i];
    }

  // Diagonalization of bidiagonal form
  for (k=n-1; k>=0; k--) //loop over singular values
    { 
    for (its=1; its<=25; its++) //loop over allowed iterations
      {
      flag = 1;
      for (l=k; l>=0; l--) //test for splitting
        {
        nm = l-1;
        if ((FABS(rv1[l])+anorm) == anorm) 
          {
          flag = 0;
          break;
          }
        if ((FABS(w[nm])+anorm) == anorm) break;
        }
      if (flag) 
        {
        c = 0;
        s = 1;
        for (i=l;i<=k;i++) 
          {
          f = s*rv1[i];
          rv1[i] = c*rv1[i];
          if ((FABS(f)+anorm) == anorm) break;
          g = w[i];
          h = PYTHAG(f,g);
          w[i] = h;
          h = 1/h;
          c = g*h;
          s = (-f*h);
          for (j=0;j<m;j++) 
            {
            y = a[j][nm];
            z = a[j][i];
            a[j][nm] = y*c + z*s;
            a[j][i] = z*c - y*s;
            }
          }
        }
      z = w[k];
      if (l == k) //convergence
        {
        if (z < 0) //singular val is made non neg-tive
          {
          w[k] = -z;
          for (j=0; j<n; j++) v[j][k] = (-v[j][k]);
          }
        break;
        }

      if (its == 25) cerr << "No convergence in singular value decomposition";

      x = w[l]; //shift from bottom 2x2 minor
      nm = k-1;
      y = w[nm];
      g = rv1[nm];
      h = rv1[k];
      f = ((y-z)*(y+z)+(g-h)*(g+h))/(2*h*y);
      g = PYTHAG(f,1);
      f = ((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
      c = s = 1;  //next QR transformation
      for (j = l; j<= nm; j++) 
        {
        i = j+1;
        g = rv1[i];
        y = w[i];
        h = s*g;
        g = c*g;
        z = PYTHAG(f,h);
        rv1[j] = z;
        c = f/z;
        s = h/z;
        f = x*c+g*s;
        g = g*c-x*s;
        h = y*s;
        y = y*c;
        for (jj = 0; jj<n; jj++) 
          {
          x = v[jj][j];
          z = v[jj][i];
          v[jj][j] = x*c + z*s;
          v[jj][i] = z*c - x*s;
          }
        z = PYTHAG(f,h);
        w[j] = z;
        if (z != 0.0) 
          {
          z = 1/z;
          c = f*z;
          s = h*z;
          }
        f = (c*g)+(s*y);
        x = (c*y)-(s*g);
        for (jj = 0; jj<m; jj++) 
          {
          y = a[jj][j];
          z = a[jj][i];
          a[jj][j] = (y*c+z*s);
          a[jj][i] = (z*c-y*s);
          }
        }
      rv1[l] = 0;
      rv1[k] = f;
      w[k] = x;
      }
    }
  delete [] rv1;
}
#undef FABS
#undef SIGN
#undef PYTHAG
#undef MAX

// Description:
// Solve matrix equation Ax = B for a vector x and load vector B. Note that
// matrix A must first be factored A = U*W*V using singular value 
// decomposition (method SingularValueDecomposition()).
void vtkMath::SingularValueBackSubstitution(double **u, double *w, double **v,
                                           int m, int n, double *b, double *x)
{
  int i, j;
  double s, *tmp;
  
  tmp = new double[n];
  for (j=0; j<n; j++)
    {
    s = 0.0;
    if ( w[j] != 0.0 )
      {
      for (i=0; i<m; i++) s += u[i][j] * b[i];
      s /= w[j];
      }
    tmp[j] = s;
    }

  for (j=0; j<n; j++)
    {
    s = 0.0;
    for (i=0; i<n; i++) s += v[j][i] * tmp[i];
    x[j] = s;
    }

  delete [] tmp;
}

// Description:
// Jacobi iteration for the solution of eigenvectors/eigenvalues of a real
// symmetric matrix. Square matrix a; system size n; output eigenvalues in d;
// eigenvectors in v; and number of jacobi rotations (output) nrot.
#define ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);\
        a[k][l]=h+s*(g-h*tau);

int vtkMath::Jacobi(float **a, int n, float *d, float **v, int *nrot)
{
  int j,iq,ip,i;
  float tresh,theta,tau,t,sm,s,h,g,c;
  static float *b = {NULL};
  static float *z = {NULL};
  static int nMax = {0};
//
//  Check on allocation of working vectors
//
  if ( n > nMax )
    {
    delete [] b; b = NULL;
    delete [] z; z = NULL;
    }
  if ( b == NULL ) 
    {
    b = new float[n];
    z = new float[n];
    nMax = n;
    } 

  for (ip=0;ip<n;ip++) 
    {
    for (iq=0;iq<n;iq++) v[ip][iq]=0.0;
    v[ip][ip]=1.0;
    }
  for (ip=0;ip<n;ip++) 
    {
    b[ip]=d[ip]=a[ip][ip];
    z[ip]=0.0;
    }
  *nrot=0;
  for (i=0;i<50;i++) 
    {
    sm=0.0;
    for (ip=0;ip<n-1;ip++) 
      {
      for (iq=ip+1;iq<n;iq++)
        sm += fabs(a[ip][iq]);
      }
    if (sm == 0.0) return 1;
    if (i < 4)
      tresh=0.2*sm/(n*n);
    else
      tresh=0.0;
    for (ip=0;ip<n-1;ip++) 
      {
      for (iq=ip+1;iq<n;iq++) 
        {
        g=100.0*fabs(a[ip][iq]);
        if (i > 4 && (float)(fabs(d[ip])+g) == (float)fabs(d[ip])
        && (float)(fabs(d[iq])+g) == (float)fabs(d[iq]))
          a[ip][iq]=0.0;
        else if (fabs(a[ip][iq]) > tresh) 
          {
          h=d[iq]-d[ip];
          if ((float)(fabs(h)+g) == (float)fabs(h))
                  t=(a[ip][iq])/h;
          else 
            {
            theta=0.5*h/(a[ip][iq]);
            t=1.0/(fabs(theta)+sqrt(1.0+theta*theta));
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
          for (j=0;j<ip-1;j++) 
            {
            ROTATE(a,j,ip,j,iq)
            }
          for (j=ip+1;j<iq-1;j++) 
            {
            ROTATE(a,ip,j,j,iq)
            }
          for (j=iq+1;j<n;j++) 
            {
            ROTATE(a,ip,j,iq,j)
            }
          for (j=0;j<n;j++) 
            {
            ROTATE(v,j,ip,j,iq)
            }
          ++(*nrot);
        }
      }
    }
    for (ip=0;ip<n;ip++) 
      {
      b[ip] += z[ip];
      d[ip]=b[ip];
      z[ip]=0.0;
      }
    }
//  vtkErrorMacro(<<"Error extracting eigenfunctions")'
  return 0;
}

#undef ROTATE

// Description:
// Sort eigenvalues/eigenvectors into descending order. Call after Jacobi.
// Vector d are the eigenvalues; matrix v are eigenvectors (in columns; n
// is the size of the matrix).
void vtkMath::Eigsrt(float *d, float **v, int n)
{
  int k,j,i;
  float p;

  for (i=0;i<n;i++) 
    {
    p=d[k=i];
    for (j=i; j<n; j++)
      if (d[j] >= p) p=d[k=j];
    if (k != i) 
      {
      d[k]=d[i];
      d[i]=p;
      for (j=0;j<n;j++) 
        {
        p=v[j][i];
        v[j][i]=v[j][k];
        v[j][k]=p;
        }
      }
    }
}

