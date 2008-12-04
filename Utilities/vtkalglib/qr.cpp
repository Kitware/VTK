/*************************************************************************
Copyright (c) 1992-2007 The University of Tennessee.  All rights reserved.

Contributors:
    * Sergey Bochkanov (ALGLIB project). Translation from FORTRAN to
      pseudocode.

See subroutines comments for additional copyrights.

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

#include "alglib/qr.h"

/*************************************************************************
QR decomposition of a rectangular matrix of size MxN

Input parameters:
    A   -   matrix A whose indexes range within [0..M-1, 0..N-1].
    M   -   number of rows in matrix A.
    N   -   number of columns in matrix A.

Output parameters:
    A   -   matrices Q and R in compact form (see below).
    Tau -   array of scalar factors which are used to form
            matrix Q. Array whose index ranges within [0.. Min(M-1,N-1)].

Matrix A is represented as A = QR, where Q is an orthogonal matrix of size
MxM, R - upper triangular (or upper trapezoid) matrix of size M x N.

The elements of matrix R are located on and above the main diagonal of
matrix A. The elements which are located in Tau array and below the main
diagonal of matrix A are used to form matrix Q as follows:

Matrix Q is represented as a product of elementary reflections

Q = H(0)*H(2)*...*H(k-1),

where k = min(m,n), and each H(i) is in the form

H(i) = 1 - tau * v * (v^T)

where tau is a scalar stored in Tau[I]; v - real vector,
so that v(0:i-1) = 0, v(i) = 1, v(i+1:m-1) stored in A(i+1:m-1,i).

  -- LAPACK routine (version 3.0) --
     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
     Courant Institute, Argonne National Lab, and Rice University
     February 29, 1992.
     Translation from FORTRAN to pseudocode (AlgoPascal)
     by Sergey Bochkanov, ALGLIB project, 2005-2007.
*************************************************************************/
void rmatrixqr(ap::real_2d_array& a, int m, int n, ap::real_1d_array& tau)
{
    ap::real_1d_array work;
    ap::real_1d_array t;
    int i;
    int k;
    int minmn;
    double tmp;

    if( m<=0||n<=0 )
    {
        return;
    }
    minmn = ap::minint(m, n);
    work.setbounds(0, n-1);
    t.setbounds(1, m);
    tau.setbounds(0, minmn-1);
    
    //
    // Test the input arguments
    //
    k = minmn;
    for(i = 0; i <= k-1; i++)
    {
        
        //
        // Generate elementary reflector H(i) to annihilate A(i+1:m,i)
        //
        ap::vmove(t.getvector(1, m-i), a.getcolumn(i, i, m-1));
        generatereflection(t, m-i, tmp);
        tau(i) = tmp;
        ap::vmove(a.getcolumn(i, i, m-1), t.getvector(1, m-i));
        t(1) = 1;
        if( i<n )
        {
            
            //
            // Apply H(i) to A(i:m-1,i+1:n-1) from the left
            //
            applyreflectionfromtheleft(a, tau(i), t, i, m-1, i+1, n-1, work);
        }
    }
}


/*************************************************************************
Partial unpacking of matrix Q from the QR decomposition of a matrix A

Input parameters:
    A       -   matrices Q and R in compact form.
                Output of RMatrixQR subroutine.
    M       -   number of rows in given matrix A. M>=0.
    N       -   number of columns in given matrix A. N>=0.
    Tau     -   scalar factors which are used to form Q.
                Output of the RMatrixQR subroutine.
    QColumns -  required number of columns of matrix Q. M>=QColumns>=0.

Output parameters:
    Q       -   first QColumns columns of matrix Q.
                Array whose indexes range within [0..M-1, 0..QColumns-1].
                If QColumns=0, the array remains unchanged.

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
void rmatrixqrunpackq(const ap::real_2d_array& a,
     int m,
     int n,
     const ap::real_1d_array& tau,
     int qcolumns,
     ap::real_2d_array& q)
{
    int i;
    int j;
    int k;
    int minmn;
    ap::real_1d_array v;
    ap::real_1d_array work;

    ap::ap_error::make_assertion(qcolumns<=m, "UnpackQFromQR: QColumns>M!");
    if( m<=0||n<=0||qcolumns<=0 )
    {
        return;
    }
    
    //
    // init
    //
    minmn = ap::minint(m, n);
    k = ap::minint(minmn, qcolumns);
    q.setbounds(0, m-1, 0, qcolumns-1);
    v.setbounds(1, m);
    work.setbounds(0, qcolumns-1);
    for(i = 0; i <= m-1; i++)
    {
        for(j = 0; j <= qcolumns-1; j++)
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
        ap::vmove(v.getvector(1, m-i), a.getcolumn(i, i, m-1));
        v(1) = 1;
        applyreflectionfromtheleft(q, tau(i), v, i, m-1, 0, qcolumns-1, work);
    }
}


/*************************************************************************
Unpacking of matrix R from the QR decomposition of a matrix A

Input parameters:
    A       -   matrices Q and R in compact form.
                Output of RMatrixQR subroutine.
    M       -   number of rows in given matrix A. M>=0.
    N       -   number of columns in given matrix A. N>=0.

Output parameters:
    R       -   matrix R, array[0..M-1, 0..N-1].

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
void rmatrixqrunpackr(const ap::real_2d_array& a,
     int m,
     int n,
     ap::real_2d_array& r)
{
    int i;
    int k;

    if( m<=0||n<=0 )
    {
        return;
    }
    k = ap::minint(m, n);
    r.setbounds(0, m-1, 0, n-1);
    for(i = 0; i <= n-1; i++)
    {
        r(0,i) = 0;
    }
    for(i = 1; i <= m-1; i++)
    {
        ap::vmove(&r(i, 0), &r(0, 0), ap::vlen(0,n-1));
    }
    for(i = 0; i <= k-1; i++)
    {
        ap::vmove(&r(i, i), &a(i, i), ap::vlen(i,n-1));
    }
}


/*************************************************************************
Obsolete 1-based subroutine. See RMatrixQR for 0-based replacement.
*************************************************************************/
void qrdecomposition(ap::real_2d_array& a,
     int m,
     int n,
     ap::real_1d_array& tau)
{
    ap::real_1d_array work;
    ap::real_1d_array t;
    int i;
    int k;
    int mmip1;
    int minmn;
    double tmp;

    minmn = ap::minint(m, n);
    work.setbounds(1, n);
    t.setbounds(1, m);
    tau.setbounds(1, minmn);
    
    //
    // Test the input arguments
    //
    k = ap::minint(m, n);
    for(i = 1; i <= k; i++)
    {
        
        //
        // Generate elementary reflector H(i) to annihilate A(i+1:m,i)
        //
        mmip1 = m-i+1;
        ap::vmove(t.getvector(1, mmip1), a.getcolumn(i, i, m));
        generatereflection(t, mmip1, tmp);
        tau(i) = tmp;
        ap::vmove(a.getcolumn(i, i, m), t.getvector(1, mmip1));
        t(1) = 1;
        if( i<n )
        {
            
            //
            // Apply H(i) to A(i:m,i+1:n) from the left
            //
            applyreflectionfromtheleft(a, tau(i), t, i, m, i+1, n, work);
        }
    }
}


/*************************************************************************
Obsolete 1-based subroutine. See RMatrixQRUnpackQ for 0-based replacement.
*************************************************************************/
void unpackqfromqr(const ap::real_2d_array& a,
     int m,
     int n,
     const ap::real_1d_array& tau,
     int qcolumns,
     ap::real_2d_array& q)
{
    int i;
    int j;
    int k;
    int minmn;
    ap::real_1d_array v;
    ap::real_1d_array work;
    int vm;

    ap::ap_error::make_assertion(qcolumns<=m, "UnpackQFromQR: QColumns>M!");
    if( m==0||n==0||qcolumns==0 )
    {
        return;
    }
    
    //
    // init
    //
    minmn = ap::minint(m, n);
    k = ap::minint(minmn, qcolumns);
    q.setbounds(1, m, 1, qcolumns);
    v.setbounds(1, m);
    work.setbounds(1, qcolumns);
    for(i = 1; i <= m; i++)
    {
        for(j = 1; j <= qcolumns; j++)
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
        vm = m-i+1;
        ap::vmove(v.getvector(1, vm), a.getcolumn(i, i, m));
        v(1) = 1;
        applyreflectionfromtheleft(q, tau(i), v, i, m, 1, qcolumns, work);
    }
}


/*************************************************************************
Obsolete 1-based subroutine. See RMatrixQR for 0-based replacement.
*************************************************************************/
void qrdecompositionunpacked(ap::real_2d_array a,
     int m,
     int n,
     ap::real_2d_array& q,
     ap::real_2d_array& r)
{
    int i;
    int k;
    ap::real_1d_array tau;
    ap::real_1d_array work;
    ap::real_1d_array v;

    k = ap::minint(m, n);
    if( n<=0 )
    {
        return;
    }
    work.setbounds(1, m);
    v.setbounds(1, m);
    q.setbounds(1, m, 1, m);
    r.setbounds(1, m, 1, n);
    
    //
    // QRDecomposition
    //
    qrdecomposition(a, m, n, tau);
    
    //
    // R
    //
    for(i = 1; i <= n; i++)
    {
        r(1,i) = 0;
    }
    for(i = 2; i <= m; i++)
    {
        ap::vmove(&r(i, 1), &r(1, 1), ap::vlen(1,n));
    }
    for(i = 1; i <= k; i++)
    {
        ap::vmove(&r(i, i), &a(i, i), ap::vlen(i,n));
    }
    
    //
    // Q
    //
    unpackqfromqr(a, m, n, tau, m, q);
}



