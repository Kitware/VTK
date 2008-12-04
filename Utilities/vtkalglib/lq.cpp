/*************************************************************************
Copyright (c) 2005-2007, Sergey Bochkanov (ALGLIB project).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer listed
  in this license in the documentation and/or other materials
  provided with the distribution.

- Neither the name of the copyright holders nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#include "alglib/lq.h"

/*************************************************************************
LQ decomposition of a rectangular matrix of size MxN

Input parameters:
    A   -   matrix A whose indexes range within [0..M-1, 0..N-1].
    M   -   number of rows in matrix A.
    N   -   number of columns in matrix A.

Output parameters:
    A   -   matrices L and Q in compact form (see below)
    Tau -   array of scalar factors which are used to form
            matrix Q. Array whose index ranges within [0..Min(M,N)-1].

Matrix A is represented as A = LQ, where Q is an orthogonal matrix of size
MxM, L - lower triangular (or lower trapezoid) matrix of size M x N.

The elements of matrix L are located on and below  the  main  diagonal  of
matrix A. The elements which are located in Tau array and above  the  main
diagonal of matrix A are used to form matrix Q as follows:

Matrix Q is represented as a product of elementary reflections

Q = H(k-1)*H(k-2)*...*H(1)*H(0),

where k = min(m,n), and each H(i) is of the form

H(i) = 1 - tau * v * (v^T)

where tau is a scalar stored in Tau[I]; v - real vector, so that v(0:i-1)=0,
v(i) = 1, v(i+1:n-1) stored in A(i,i+1:n-1).

  -- ALGLIB --
     Copyright 2005-2007 by Bochkanov Sergey
*************************************************************************/
void rmatrixlq(ap::real_2d_array& a, int m, int n, ap::real_1d_array& tau)
{
    ap::real_1d_array work;
    ap::real_1d_array t;
    int i;
    int k;
    int minmn;
    int maxmn;
    double tmp;

    minmn = ap::minint(m, n);
    maxmn = ap::maxint(m, n);
    work.setbounds(0, m);
    t.setbounds(0, n);
    tau.setbounds(0, minmn-1);
    k = ap::minint(m, n);
    for(i = 0; i <= k-1; i++)
    {
        
        //
        // Generate elementary reflector H(i) to annihilate A(i,i+1:n-1)
        //
        ap::vmove(&t(1), &a(i, i), ap::vlen(1,n-i));
        generatereflection(t, n-i, tmp);
        tau(i) = tmp;
        ap::vmove(&a(i, i), &t(1), ap::vlen(i,n-1));
        t(1) = 1;
        if( i<n )
        {
            
            //
            // Apply H(i) to A(i+1:m,i:n) from the right
            //
            applyreflectionfromtheright(a, tau(i), t, i+1, m-1, i, n-1, work);
        }
    }
}


/*************************************************************************
Partial unpacking of matrix Q from the LQ decomposition of a matrix A

Input parameters:
    A       -   matrices L and Q in compact form.
                Output of RMatrixLQ subroutine.
    M       -   number of rows in given matrix A. M>=0.
    N       -   number of columns in given matrix A. N>=0.
    Tau     -   scalar factors which are used to form Q.
                Output of the RMatrixLQ subroutine.
    QRows   -   required number of rows in matrix Q. N>=QRows>=0.

Output parameters:
    Q       -   first QRows rows of matrix Q. Array whose indexes range
                within [0..QRows-1, 0..N-1]. If QRows=0, the array remains
                unchanged.

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
void rmatrixlqunpackq(const ap::real_2d_array& a,
     int m,
     int n,
     const ap::real_1d_array& tau,
     int qrows,
     ap::real_2d_array& q)
{
    int i;
    int j;
    int k;
    int minmn;
    ap::real_1d_array v;
    ap::real_1d_array work;

    ap::ap_error::make_assertion(qrows<=n, "RMatrixLQUnpackQ: QRows>N!");
    if( m<=0||n<=0||qrows<=0 )
    {
        return;
    }
    
    //
    // init
    //
    minmn = ap::minint(m, n);
    k = ap::minint(minmn, qrows);
    q.setbounds(0, qrows-1, 0, n-1);
    v.setbounds(0, n);
    work.setbounds(0, qrows);
    for(i = 0; i <= qrows-1; i++)
    {
        for(j = 0; j <= n-1; j++)
        {
            if( i==j )
            {
                q(i,j) = 1;
            }
            else
            {
                q(i,j) = 0;
            }
        }
    }
    
    //
    // unpack Q
    //
    for(i = k-1; i >= 0; i--)
    {
        
        //
        // Apply H(i)
        //
        ap::vmove(&v(1), &a(i, i), ap::vlen(1,n-i));
        v(1) = 1;
        applyreflectionfromtheright(q, tau(i), v, 0, qrows-1, i, n-1, work);
    }
}


/*************************************************************************
Unpacking of matrix L from the LQ decomposition of a matrix A

Input parameters:
    A       -   matrices Q and L in compact form.
                Output of RMatrixLQ subroutine.
    M       -   number of rows in given matrix A. M>=0.
    N       -   number of columns in given matrix A. N>=0.

Output parameters:
    L       -   matrix L, array[0..M-1, 0..N-1].

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
void rmatrixlqunpackl(const ap::real_2d_array& a,
     int m,
     int n,
     ap::real_2d_array& l)
{
    int i;
    int k;

    if( m<=0||n<=0 )
    {
        return;
    }
    l.setbounds(0, m-1, 0, n-1);
    for(i = 0; i <= n-1; i++)
    {
        l(0,i) = 0;
    }
    for(i = 1; i <= m-1; i++)
    {
        ap::vmove(&l(i, 0), &l(0, 0), ap::vlen(0,n-1));
    }
    for(i = 0; i <= m-1; i++)
    {
        k = ap::minint(i, n-1);
        ap::vmove(&l(i, 0), &a(i, 0), ap::vlen(0,k));
    }
}


/*************************************************************************
Obsolete 1-based subroutine
See RMatrixLQ for 0-based replacement.
*************************************************************************/
void lqdecomposition(ap::real_2d_array& a,
     int m,
     int n,
     ap::real_1d_array& tau)
{
    ap::real_1d_array work;
    ap::real_1d_array t;
    int i;
    int k;
    int nmip1;
    int minmn;
    int maxmn;
    double tmp;

    minmn = ap::minint(m, n);
    maxmn = ap::maxint(m, n);
    work.setbounds(1, m);
    t.setbounds(1, n);
    tau.setbounds(1, minmn);
    
    //
    // Test the input arguments
    //
    k = ap::minint(m, n);
    for(i = 1; i <= k; i++)
    {
        
        //
        // Generate elementary reflector H(i) to annihilate A(i,i+1:n)
        //
        nmip1 = n-i+1;
        ap::vmove(&t(1), &a(i, i), ap::vlen(1,nmip1));
        generatereflection(t, nmip1, tmp);
        tau(i) = tmp;
        ap::vmove(&a(i, i), &t(1), ap::vlen(i,n));
        t(1) = 1;
        if( i<n )
        {
            
            //
            // Apply H(i) to A(i+1:m,i:n) from the right
            //
            applyreflectionfromtheright(a, tau(i), t, i+1, m, i, n, work);
        }
    }
}


/*************************************************************************
Obsolete 1-based subroutine
See RMatrixLQUnpackQ for 0-based replacement.
*************************************************************************/
void unpackqfromlq(const ap::real_2d_array& a,
     int m,
     int n,
     const ap::real_1d_array& tau,
     int qrows,
     ap::real_2d_array& q)
{
    int i;
    int j;
    int k;
    int minmn;
    ap::real_1d_array v;
    ap::real_1d_array work;
    int vm;

    ap::ap_error::make_assertion(qrows<=n, "UnpackQFromLQ: QRows>N!");
    if( m==0||n==0||qrows==0 )
    {
        return;
    }
    
    //
    // init
    //
    minmn = ap::minint(m, n);
    k = ap::minint(minmn, qrows);
    q.setbounds(1, qrows, 1, n);
    v.setbounds(1, n);
    work.setbounds(1, qrows);
    for(i = 1; i <= qrows; i++)
    {
        for(j = 1; j <= n; j++)
        {
            if( i==j )
            {
                q(i,j) = 1;
            }
            else
            {
                q(i,j) = 0;
            }
        }
    }
    
    //
    // unpack Q
    //
    for(i = k; i >= 1; i--)
    {
        
        //
        // Apply H(i)
        //
        vm = n-i+1;
        ap::vmove(&v(1), &a(i, i), ap::vlen(1,vm));
        v(1) = 1;
        applyreflectionfromtheright(q, tau(i), v, 1, qrows, i, n, work);
    }
}


/*************************************************************************
Obsolete 1-based subroutine
*************************************************************************/
void lqdecompositionunpacked(ap::real_2d_array a,
     int m,
     int n,
     ap::real_2d_array& l,
     ap::real_2d_array& q)
{
    int i;
    int j;
    ap::real_1d_array tau;

    if( n<=0 )
    {
        return;
    }
    q.setbounds(1, n, 1, n);
    l.setbounds(1, m, 1, n);
    
    //
    // LQDecomposition
    //
    lqdecomposition(a, m, n, tau);
    
    //
    // L
    //
    for(i = 1; i <= m; i++)
    {
        for(j = 1; j <= n; j++)
        {
            if( j>i )
            {
                l(i,j) = 0;
            }
            else
            {
                l(i,j) = a(i,j);
            }
        }
    }
    
    //
    // Q
    //
    unpackqfromlq(a, m, n, tau, n, q);
}



