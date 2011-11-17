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

#include "alglib/bdsvd.h"

bool bidiagonalsvddecompositioninternal(ap::real_1d_array& d,
     ap::real_1d_array e,
     int n,
     bool isupper,
     bool isfractionalaccuracyrequired,
     ap::real_2d_array& u,
     int ustart,
     int nru,
     ap::real_2d_array& c,
     int cstart,
     int ncc,
     ap::real_2d_array& vt,
     int vstart,
     int ncvt);
double extsignbdsqr(double a, double b);
void svd2x2(double f, double g, double h, double& ssmin, double& ssmax);
void svdv2x2(double f,
     double g,
     double h,
     double& ssmin,
     double& ssmax,
     double& snr,
     double& csr,
     double& snl,
     double& csl);

/*************************************************************************
Singular value decomposition of a bidiagonal matrix (extended algorithm)

The algorithm performs the singular value decomposition  of  a  bidiagonal
matrix B (upper or lower) representing it as B = Q*S*P^T, where Q and  P -
orthogonal matrices, S - diagonal matrix with non-negative elements on the
main diagonal, in descending order.

The  algorithm  finds  singular  values.  In  addition,  the algorithm can
calculate  matrices  Q  and P (more precisely, not the matrices, but their
product  with  given  matrices U and VT - U*Q and (P^T)*VT)).  Of  course,
matrices U and VT can be of any type, including identity. Furthermore, the
algorithm can calculate Q'*C (this product is calculated more  effectively
than U*Q,  because  this calculation operates with rows instead  of matrix
columns).

The feature of the algorithm is its ability to find  all  singular  values
including those which are arbitrarily close to 0  with  relative  accuracy
close to  machine precision. If the parameter IsFractionalAccuracyRequired
is set to True, all singular values will have high relative accuracy close
to machine precision. If the parameter is set to False, only  the  biggest
singular value will have relative accuracy  close  to  machine  precision.
The absolute error of other singular values is equal to the absolute error
of the biggest singular value.

Input parameters:
    D       -   main diagonal of matrix B.
                Array whose index ranges within [0..N-1].
    E       -   superdiagonal (or subdiagonal) of matrix B.
                Array whose index ranges within [0..N-2].
    N       -   size of matrix B.
    IsUpper -   True, if the matrix is upper bidiagonal.
    IsFractionalAccuracyRequired -
                accuracy to search singular values with.
    U       -   matrix to be multiplied by Q.
                Array whose indexes range within [0..NRU-1, 0..N-1].
                The matrix can be bigger, in that case only the  submatrix
                [0..NRU-1, 0..N-1] will be multiplied by Q.
    NRU     -   number of rows in matrix U.
    C       -   matrix to be multiplied by Q'.
                Array whose indexes range within [0..N-1, 0..NCC-1].
                The matrix can be bigger, in that case only the  submatrix
                [0..N-1, 0..NCC-1] will be multiplied by Q'.
    NCC     -   number of columns in matrix C.
    VT      -   matrix to be multiplied by P^T.
                Array whose indexes range within [0..N-1, 0..NCVT-1].
                The matrix can be bigger, in that case only the  submatrix
                [0..N-1, 0..NCVT-1] will be multiplied by P^T.
    NCVT    -   number of columns in matrix VT.

Output parameters:
    D       -   singular values of matrix B in descending order.
    U       -   if NRU>0, contains matrix U*Q.
    VT      -   if NCVT>0, contains matrix (P^T)*VT.
    C       -   if NCC>0, contains matrix Q'*C.

Result:
    True, if the algorithm has converged.
    False, if the algorithm hasn't converged (rare case).

Additional information:
    The type of convergence is controlled by the internal  parameter  TOL.
    If the parameter is greater than 0, the singular values will have
    relative accuracy TOL. If TOL<0, the singular values will have
    absolute accuracy ABS(TOL)*norm(B).
    By default, |TOL| falls within the range of 10*Epsilon and 100*Epsilon,
    where Epsilon is the machine precision. It is not  recommended  to  use
    TOL less than 10*Epsilon since this will  considerably  slow  down  the
    algorithm and may not lead to error decreasing.
History:
    * 31 March, 2007.
        changed MAXITR from 6 to 12.

  -- LAPACK routine (version 3.0) --
     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
     Courant Institute, Argonne National Lab, and Rice University
     October 31, 1999.
*************************************************************************/
bool rmatrixbdsvd(ap::real_1d_array& d,
     ap::real_1d_array e,
     int n,
     bool isupper,
     bool isfractionalaccuracyrequired,
     ap::real_2d_array& u,
     int nru,
     ap::real_2d_array& c,
     int ncc,
     ap::real_2d_array& vt,
     int ncvt)
{
    bool result;
    ap::real_1d_array d1;
    ap::real_1d_array e1;

    d1.setbounds(1, n);
    ap::vmove(&d1(1), &d(0), ap::vlen(1,n));
    if( n>1 )
    {
        e1.setbounds(1, n-1);
        ap::vmove(&e1(1), &e(0), ap::vlen(1,n-1));
    }
    result = bidiagonalsvddecompositioninternal(d1, e1, n, isupper, isfractionalaccuracyrequired, u, 0, nru, c, 0, ncc, vt, 0, ncvt);
    ap::vmove(&d(0), &d1(1), ap::vlen(0,n-1));
    return result;
}


/*************************************************************************
Obsolete 1-based subroutine. See RMatrixBDSVD for 0-based replacement.

History:
    * 31 March, 2007.
        changed MAXITR from 6 to 12.

  -- LAPACK routine (version 3.0) --
     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
     Courant Institute, Argonne National Lab, and Rice University
     October 31, 1999.
*************************************************************************/
bool bidiagonalsvddecomposition(ap::real_1d_array& d,
     ap::real_1d_array e,
     int n,
     bool isupper,
     bool isfractionalaccuracyrequired,
     ap::real_2d_array& u,
     int nru,
     ap::real_2d_array& c,
     int ncc,
     ap::real_2d_array& vt,
     int ncvt)
{
    bool result;

    result = bidiagonalsvddecompositioninternal(d, e, n, isupper, isfractionalaccuracyrequired, u, 1, nru, c, 1, ncc, vt, 1, ncvt);
    return result;
}


/*************************************************************************
Internal working subroutine for bidiagonal decomposition
*************************************************************************/
bool bidiagonalsvddecompositioninternal(ap::real_1d_array& d,
     ap::real_1d_array e,
     int n,
     bool isupper,
     bool isfractionalaccuracyrequired,
     ap::real_2d_array& u,
     int ustart,
     int nru,
     ap::real_2d_array& c,
     int cstart,
     int ncc,
     ap::real_2d_array& vt,
     int vstart,
     int ncvt)
{
    bool result;
    int i;
    int idir;
    int isub;
    int iter;
    int j;
    int ll = 0; // Eliminate compiler warning.
    int lll;
    int m;
    int maxit;
    int oldll;
    int oldm;
    double abse;
    double abss;
    double cosl;
    double cosr;
    double cs;
    double eps;
    double f;
    double g;
    double h;
    double mu;
    double oldcs;
    double oldsn = 0.; // Eliminate compiler warning.
    double r;
    double shift;
    double sigmn;
    double sigmx;
    double sinl;
    double sinr;
    double sll;
    double smax;
    double smin;
    double sminl;
    double sminoa;
    double sn;
    double thresh;
    double tol;
    double tolmul;
    double unfl;
    ap::real_1d_array work0;
    ap::real_1d_array work1;
    ap::real_1d_array work2;
    ap::real_1d_array work3;
    int maxitr;
    bool matrixsplitflag;
    bool iterflag;
    ap::real_1d_array utemp;
    ap::real_1d_array vttemp;
    ap::real_1d_array ctemp;
    ap::real_1d_array etemp;
    bool fwddir;
    double tmp;
    int mm1;
    int mm0;
    bool bchangedir;
    int uend;
    int cend;
    int vend;

    result = true;
    if( n==0 )
    {
        return result;
    }
    if( n==1 )
    {
        if( d(1)<0 )
        {
            d(1) = -d(1);
            if( ncvt>0 )
            {
                ap::vmul(&vt(vstart, vstart), ap::vlen(vstart,vstart+ncvt-1), -1);
            }
        }
        return result;
    }
    
    //
    // init
    //
    work0.setbounds(1, n-1);
    work1.setbounds(1, n-1);
    work2.setbounds(1, n-1);
    work3.setbounds(1, n-1);
    uend = ustart+ap::maxint(nru-1, 0);
    vend = vstart+ap::maxint(ncvt-1, 0);
    cend = cstart+ap::maxint(ncc-1, 0);
    utemp.setbounds(ustart, uend);
    vttemp.setbounds(vstart, vend);
    ctemp.setbounds(cstart, cend);
    maxitr = 12;
    fwddir = true;
    
    //
    // resize E from N-1 to N
    //
    etemp.setbounds(1, n);
    for(i = 1; i <= n-1; i++)
    {
        etemp(i) = e(i);
    }
    e.setbounds(1, n);
    for(i = 1; i <= n-1; i++)
    {
        e(i) = etemp(i);
    }
    e(n) = 0;
    idir = 0;
    
    //
    // Get machine constants
    //
    eps = ap::machineepsilon;
    unfl = ap::minrealnumber;
    
    //
    // If matrix lower bidiagonal, rotate to be upper bidiagonal
    // by applying Givens rotations on the left
    //
    if( !isupper )
    {
        for(i = 1; i <= n-1; i++)
        {
            generaterotation(d(i), e(i), cs, sn, r);
            d(i) = r;
            e(i) = sn*d(i+1);
            d(i+1) = cs*d(i+1);
            work0(i) = cs;
            work1(i) = sn;
        }
        
        //
        // Update singular vectors if desired
        //
        if( nru>0 )
        {
            applyrotationsfromtheright(fwddir, ustart, uend, 1+ustart-1, n+ustart-1, work0, work1, u, utemp);
        }
        if( ncc>0 )
        {
            applyrotationsfromtheleft(fwddir, 1+cstart-1, n+cstart-1, cstart, cend, work0, work1, c, ctemp);
        }
    }
    
    //
    // Compute singular values to relative accuracy TOL
    // (By setting TOL to be negative, algorithm will compute
    // singular values to absolute accuracy ABS(TOL)*norm(input matrix))
    //
    tolmul = ap::maxreal(double(10), ap::minreal(double(100), pow(eps, -0.125)));
    tol = tolmul*eps;
    if( !isfractionalaccuracyrequired )
    {
        tol = -tol;
    }
    
    //
    // Compute approximate maximum, minimum singular values
    //
    smax = 0;
    for(i = 1; i <= n; i++)
    {
        smax = ap::maxreal(smax, fabs(d(i)));
    }
    for(i = 1; i <= n-1; i++)
    {
        smax = ap::maxreal(smax, fabs(e(i)));
    }
    sminl = 0;
    if( tol>=0 )
    {
        
        //
        // Relative accuracy desired
        //
        sminoa = fabs(d(1));
        if( sminoa!=0 )
        {
            mu = sminoa;
            for(i = 2; i <= n; i++)
            {
                mu = fabs(d(i))*(mu/(mu+fabs(e(i-1))));
                sminoa = ap::minreal(sminoa, mu);
                if( sminoa==0 )
                {
                    break;
                }
            }
        }
        sminoa = sminoa/sqrt(double(n));
        thresh = ap::maxreal(tol*sminoa, maxitr*n*n*unfl);
    }
    else
    {
        
        //
        // Absolute accuracy desired
        //
        thresh = ap::maxreal(fabs(tol)*smax, maxitr*n*n*unfl);
    }
    
    //
    // Prepare for main iteration loop for the singular values
    // (MAXIT is the maximum number of passes through the inner
    // loop permitted before nonconvergence signalled.)
    //
    maxit = maxitr*n*n;
    iter = 0;
    oldll = -1;
    oldm = -1;
    
    //
    // M points to last element of unconverged part of matrix
    //
    m = n;
    
    //
    // Begin main iteration loop
    //
    while(true)
    {
        
        //
        // Check for convergence or exceeding iteration count
        //
        if( m<=1 )
        {
            break;
        }
        if( iter>maxit )
        {
            result = false;
            return result;
        }
        
        //
        // Find diagonal block of matrix to work on
        //
        if( tol<0&&fabs(d(m))<=thresh )
        {
            d(m) = 0;
        }
        smax = fabs(d(m));
        smin = smax;
        matrixsplitflag = false;
        for(lll = 1; lll <= m-1; lll++)
        {
            ll = m-lll;
            abss = fabs(d(ll));
            abse = fabs(e(ll));
            if( tol<0&&abss<=thresh )
            {
                d(ll) = 0;
            }
            if( abse<=thresh )
            {
                matrixsplitflag = true;
                break;
            }
            smin = ap::minreal(smin, abss);
            smax = ap::maxreal(smax, ap::maxreal(abss, abse));
        }
        if( !matrixsplitflag )
        {
            ll = 0;
        }
        else
        {
            
            //
            // Matrix splits since E(LL) = 0
            //
            e(ll) = 0;
            if( ll==m-1 )
            {
                
                //
                // Convergence of bottom singular value, return to top of loop
                //
                m = m-1;
                continue;
            }
        }
        ll = ll+1;
        
        //
        // E(LL) through E(M-1) are nonzero, E(LL-1) is zero
        //
        if( ll==m-1 )
        {
            
            //
            // 2 by 2 block, handle separately
            //
            svdv2x2(d(m-1), e(m-1), d(m), sigmn, sigmx, sinr, cosr, sinl, cosl);
            d(m-1) = sigmx;
            e(m-1) = 0;
            d(m) = sigmn;
            
            //
            // Compute singular vectors, if desired
            //
            if( ncvt>0 )
            {
                mm0 = m+(vstart-1);
                mm1 = m-1+(vstart-1);
                ap::vmove(&vttemp(vstart), &vt(mm1, vstart), ap::vlen(vstart,vend), cosr);
                ap::vadd(&vttemp(vstart), &vt(mm0, vstart), ap::vlen(vstart,vend), sinr);
                ap::vmul(&vt(mm0, vstart), ap::vlen(vstart,vend), cosr);
                ap::vsub(&vt(mm0, vstart), &vt(mm1, vstart), ap::vlen(vstart,vend), sinr);
                ap::vmove(&vt(mm1, vstart), &vttemp(vstart), ap::vlen(vstart,vend));
            }
            if( nru>0 )
            {
                mm0 = m+ustart-1;
                mm1 = m-1+ustart-1;
                ap::vmove(utemp.getvector(ustart, uend), u.getcolumn(mm1, ustart, uend), cosl);
                ap::vadd(utemp.getvector(ustart, uend), u.getcolumn(mm0, ustart, uend), sinl);
                ap::vmul(u.getcolumn(mm0, ustart, uend), cosl);
                ap::vsub(u.getcolumn(mm0, ustart, uend), u.getcolumn(mm1, ustart, uend), sinl);
                ap::vmove(u.getcolumn(mm1, ustart, uend), utemp.getvector(ustart, uend));
            }
            if( ncc>0 )
            {
                mm0 = m+cstart-1;
                mm1 = m-1+cstart-1;
                ap::vmove(&ctemp(cstart), &c(mm1, cstart), ap::vlen(cstart,cend), cosl);
                ap::vadd(&ctemp(cstart), &c(mm0, cstart), ap::vlen(cstart,cend), sinl);
                ap::vmul(&c(mm0, cstart), ap::vlen(cstart,cend), cosl);
                ap::vsub(&c(mm0, cstart), &c(mm1, cstart), ap::vlen(cstart,cend), sinl);
                ap::vmove(&c(mm1, cstart), &ctemp(cstart), ap::vlen(cstart,cend));
            }
            m = m-2;
            continue;
        }
        
        //
        // If working on new submatrix, choose shift direction
        // (from larger end diagonal element towards smaller)
        //
        // Previously was
        //     "if (LL>OLDM) or (M<OLDLL) then"
        // fixed thanks to Michael Rolle < m@rolle.name >
        // Very strange that LAPACK still contains it.
        //
        bchangedir = false;
        if( idir==1&&fabs(d(ll))<1.0E-3*fabs(d(m)) )
        {
            bchangedir = true;
        }
        if( idir==2&&fabs(d(m))<1.0E-3*fabs(d(ll)) )
        {
            bchangedir = true;
        }
        if( ll!=oldll||m!=oldm||bchangedir )
        {
            if( fabs(d(ll))>=fabs(d(m)) )
            {
                
                //
                // Chase bulge from top (big end) to bottom (small end)
                //
                idir = 1;
            }
            else
            {
                
                //
                // Chase bulge from bottom (big end) to top (small end)
                //
                idir = 2;
            }
        }
        
        //
        // Apply convergence tests
        //
        if( idir==1 )
        {
            
            //
            // Run convergence test in forward direction
            // First apply standard test to bottom of matrix
            //
            if( (fabs(e(m-1))<=fabs(tol)*fabs(d(m)))||(tol<0&&fabs(e(m-1))<=thresh) )
            {
                e(m-1) = 0;
                continue;
            }
            if( tol>=0 )
            {
                
                //
                // If relative accuracy desired,
                // apply convergence criterion forward
                //
                mu = fabs(d(ll));
                sminl = mu;
                iterflag = false;
                for(lll = ll; lll <= m-1; lll++)
                {
                    if( fabs(e(lll))<=tol*mu )
                    {
                        e(lll) = 0;
                        iterflag = true;
                        break;
                    }
                    mu = fabs(d(lll+1))*(mu/(mu+fabs(e(lll))));
                    sminl = ap::minreal(sminl, mu);
                }
                if( iterflag )
                {
                    continue;
                }
            }
        }
        else
        {
            
            //
            // Run convergence test in backward direction
            // First apply standard test to top of matrix
            //
            if( (fabs(e(ll))<=fabs(tol)*fabs(d(ll)))||(tol<0&&fabs(e(ll))<=thresh) )
            {
                e(ll) = 0;
                continue;
            }
            if( tol>=0 )
            {
                
                //
                // If relative accuracy desired,
                // apply convergence criterion backward
                //
                mu = fabs(d(m));
                sminl = mu;
                iterflag = false;
                for(lll = m-1; lll >= ll; lll--)
                {
                    if( fabs(e(lll))<=tol*mu )
                    {
                        e(lll) = 0;
                        iterflag = true;
                        break;
                    }
                    mu = fabs(d(lll))*(mu/(mu+fabs(e(lll))));
                    sminl = ap::minreal(sminl, mu);
                }
                if( iterflag )
                {
                    continue;
                }
            }
        }
        oldll = ll;
        oldm = m;
        
        //
        // Compute shift.  First, test if shifting would ruin relative
        // accuracy, and if so set the shift to zero.
        //
        if( tol>=0&&n*tol*(sminl/smax)<=ap::maxreal(eps, 0.01*tol) )
        {
            
            //
            // Use a zero shift to avoid loss of relative accuracy
            //
            shift = 0;
        }
        else
        {
            
            //
            // Compute the shift from 2-by-2 block at end of matrix
            //
            if( idir==1 )
            {
                sll = fabs(d(ll));
                svd2x2(d(m-1), e(m-1), d(m), shift, r);
            }
            else
            {
                sll = fabs(d(m));
                svd2x2(d(ll), e(ll), d(ll+1), shift, r);
            }
            
            //
            // Test if shift negligible, and if so set to zero
            //
            if( sll>0 )
            {
                if( ap::sqr(shift/sll)<eps )
                {
                    shift = 0;
                }
            }
        }
        
        //
        // Increment iteration count
        //
        iter = iter+m-ll;
        
        //
        // If SHIFT = 0, do simplified QR iteration
        //
        if( shift==0 )
        {
            if( idir==1 )
            {
                
                //
                // Chase bulge from top to bottom
                // Save cosines and sines for later singular vector updates
                //
                cs = 1;
                oldcs = 1;
                for(i = ll; i <= m-1; i++)
                {
                    generaterotation(d(i)*cs, e(i), cs, sn, r);
                    if( i>ll )
                    {
                        e(i-1) = oldsn*r;
                    }
                    generaterotation(oldcs*r, d(i+1)*sn, oldcs, oldsn, tmp);
                    d(i) = tmp;
                    work0(i-ll+1) = cs;
                    work1(i-ll+1) = sn;
                    work2(i-ll+1) = oldcs;
                    work3(i-ll+1) = oldsn;
                }
                h = d(m)*cs;
                d(m) = h*oldcs;
                e(m-1) = h*oldsn;
                
                //
                // Update singular vectors
                //
                if( ncvt>0 )
                {
                    applyrotationsfromtheleft(fwddir, ll+vstart-1, m+vstart-1, vstart, vend, work0, work1, vt, vttemp);
                }
                if( nru>0 )
                {
                    applyrotationsfromtheright(fwddir, ustart, uend, ll+ustart-1, m+ustart-1, work2, work3, u, utemp);
                }
                if( ncc>0 )
                {
                    applyrotationsfromtheleft(fwddir, ll+cstart-1, m+cstart-1, cstart, cend, work2, work3, c, ctemp);
                }
                
                //
                // Test convergence
                //
                if( fabs(e(m-1))<=thresh )
                {
                    e(m-1) = 0;
                }
            }
            else
            {
                
                //
                // Chase bulge from bottom to top
                // Save cosines and sines for later singular vector updates
                //
                cs = 1;
                oldcs = 1;
                for(i = m; i >= ll+1; i--)
                {
                    generaterotation(d(i)*cs, e(i-1), cs, sn, r);
                    if( i<m )
                    {
                        e(i) = oldsn*r;
                    }
                    generaterotation(oldcs*r, d(i-1)*sn, oldcs, oldsn, tmp);
                    d(i) = tmp;
                    work0(i-ll) = cs;
                    work1(i-ll) = -sn;
                    work2(i-ll) = oldcs;
                    work3(i-ll) = -oldsn;
                }
                h = d(ll)*cs;
                d(ll) = h*oldcs;
                e(ll) = h*oldsn;
                
                //
                // Update singular vectors
                //
                if( ncvt>0 )
                {
                    applyrotationsfromtheleft(!fwddir, ll+vstart-1, m+vstart-1, vstart, vend, work2, work3, vt, vttemp);
                }
                if( nru>0 )
                {
                    applyrotationsfromtheright(!fwddir, ustart, uend, ll+ustart-1, m+ustart-1, work0, work1, u, utemp);
                }
                if( ncc>0 )
                {
                    applyrotationsfromtheleft(!fwddir, ll+cstart-1, m+cstart-1, cstart, cend, work0, work1, c, ctemp);
                }
                
                //
                // Test convergence
                //
                if( fabs(e(ll))<=thresh )
                {
                    e(ll) = 0;
                }
            }
        }
        else
        {
            
            //
            // Use nonzero shift
            //
            if( idir==1 )
            {
                
                //
                // Chase bulge from top to bottom
                // Save cosines and sines for later singular vector updates
                //
                f = (fabs(d(ll))-shift)*(extsignbdsqr(double(1), d(ll))+shift/d(ll));
                g = e(ll);
                for(i = ll; i <= m-1; i++)
                {
                    generaterotation(f, g, cosr, sinr, r);
                    if( i>ll )
                    {
                        e(i-1) = r;
                    }
                    f = cosr*d(i)+sinr*e(i);
                    e(i) = cosr*e(i)-sinr*d(i);
                    g = sinr*d(i+1);
                    d(i+1) = cosr*d(i+1);
                    generaterotation(f, g, cosl, sinl, r);
                    d(i) = r;
                    f = cosl*e(i)+sinl*d(i+1);
                    d(i+1) = cosl*d(i+1)-sinl*e(i);
                    if( i<m-1 )
                    {
                        g = sinl*e(i+1);
                        e(i+1) = cosl*e(i+1);
                    }
                    work0(i-ll+1) = cosr;
                    work1(i-ll+1) = sinr;
                    work2(i-ll+1) = cosl;
                    work3(i-ll+1) = sinl;
                }
                e(m-1) = f;
                
                //
                // Update singular vectors
                //
                if( ncvt>0 )
                {
                    applyrotationsfromtheleft(fwddir, ll+vstart-1, m+vstart-1, vstart, vend, work0, work1, vt, vttemp);
                }
                if( nru>0 )
                {
                    applyrotationsfromtheright(fwddir, ustart, uend, ll+ustart-1, m+ustart-1, work2, work3, u, utemp);
                }
                if( ncc>0 )
                {
                    applyrotationsfromtheleft(fwddir, ll+cstart-1, m+cstart-1, cstart, cend, work2, work3, c, ctemp);
                }
                
                //
                // Test convergence
                //
                if( fabs(e(m-1))<=thresh )
                {
                    e(m-1) = 0;
                }
            }
            else
            {
                
                //
                // Chase bulge from bottom to top
                // Save cosines and sines for later singular vector updates
                //
                f = (fabs(d(m))-shift)*(extsignbdsqr(double(1), d(m))+shift/d(m));
                g = e(m-1);
                for(i = m; i >= ll+1; i--)
                {
                    generaterotation(f, g, cosr, sinr, r);
                    if( i<m )
                    {
                        e(i) = r;
                    }
                    f = cosr*d(i)+sinr*e(i-1);
                    e(i-1) = cosr*e(i-1)-sinr*d(i);
                    g = sinr*d(i-1);
                    d(i-1) = cosr*d(i-1);
                    generaterotation(f, g, cosl, sinl, r);
                    d(i) = r;
                    f = cosl*e(i-1)+sinl*d(i-1);
                    d(i-1) = cosl*d(i-1)-sinl*e(i-1);
                    if( i>ll+1 )
                    {
                        g = sinl*e(i-2);
                        e(i-2) = cosl*e(i-2);
                    }
                    work0(i-ll) = cosr;
                    work1(i-ll) = -sinr;
                    work2(i-ll) = cosl;
                    work3(i-ll) = -sinl;
                }
                e(ll) = f;
                
                //
                // Test convergence
                //
                if( fabs(e(ll))<=thresh )
                {
                    e(ll) = 0;
                }
                
                //
                // Update singular vectors if desired
                //
                if( ncvt>0 )
                {
                    applyrotationsfromtheleft(!fwddir, ll+vstart-1, m+vstart-1, vstart, vend, work2, work3, vt, vttemp);
                }
                if( nru>0 )
                {
                    applyrotationsfromtheright(!fwddir, ustart, uend, ll+ustart-1, m+ustart-1, work0, work1, u, utemp);
                }
                if( ncc>0 )
                {
                    applyrotationsfromtheleft(!fwddir, ll+cstart-1, m+cstart-1, cstart, cend, work0, work1, c, ctemp);
                }
            }
        }
        
        //
        // QR iteration finished, go back and check convergence
        //
        continue;
    }
    
    //
    // All singular values converged, so make them positive
    //
    for(i = 1; i <= n; i++)
    {
        if( d(i)<0 )
        {
            d(i) = -d(i);
            
            //
            // Change sign of singular vectors, if desired
            //
            if( ncvt>0 )
            {
                ap::vmul(&vt(i+vstart-1, vstart), ap::vlen(vstart,vend), -1);
            }
        }
    }
    
    //
    // Sort the singular values into decreasing order (insertion sort on
    // singular values, but only one transposition per singular vector)
    //
    for(i = 1; i <= n-1; i++)
    {
        
        //
        // Scan for smallest D(I)
        //
        isub = 1;
        smin = d(1);
        for(j = 2; j <= n+1-i; j++)
        {
            if( d(j)<=smin )
            {
                isub = j;
                smin = d(j);
            }
        }
        if( isub!=n+1-i )
        {
            
            //
            // Swap singular values and vectors
            //
            d(isub) = d(n+1-i);
            d(n+1-i) = smin;
            if( ncvt>0 )
            {
                j = n+1-i;
                ap::vmove(&vttemp(vstart), &vt(isub+vstart-1, vstart), ap::vlen(vstart,vend));
                ap::vmove(&vt(isub+vstart-1, vstart), &vt(j+vstart-1, vstart), ap::vlen(vstart,vend));
                ap::vmove(&vt(j+vstart-1, vstart), &vttemp(vstart), ap::vlen(vstart,vend));
            }
            if( nru>0 )
            {
                j = n+1-i;
                ap::vmove(utemp.getvector(ustart, uend), u.getcolumn(isub+ustart-1, ustart, uend));
                ap::vmove(u.getcolumn(isub+ustart-1, ustart, uend), u.getcolumn(j+ustart-1, ustart, uend));
                ap::vmove(u.getcolumn(j+ustart-1, ustart, uend), utemp.getvector(ustart, uend));
            }
            if( ncc>0 )
            {
                j = n+1-i;
                ap::vmove(&ctemp(cstart), &c(isub+cstart-1, cstart), ap::vlen(cstart,cend));
                ap::vmove(&c(isub+cstart-1, cstart), &c(j+cstart-1, cstart), ap::vlen(cstart,cend));
                ap::vmove(&c(j+cstart-1, cstart), &ctemp(cstart), ap::vlen(cstart,cend));
            }
        }
    }
    return result;
}


double extsignbdsqr(double a, double b)
{
    double result;

    if( b>=0 )
    {
        result = fabs(a);
    }
    else
    {
        result = -fabs(a);
    }
    return result;
}


void svd2x2(double f, double g, double h, double& ssmin, double& ssmax)
{
    double aas;
    double at;
    double au;
    double c;
    double fa;
    double fhmn;
    double fhmx;
    double ga;
    double ha;

    fa = fabs(f);
    ga = fabs(g);
    ha = fabs(h);
    fhmn = ap::minreal(fa, ha);
    fhmx = ap::maxreal(fa, ha);
    if( fhmn==0 )
    {
        ssmin = 0;
        if( fhmx==0 )
        {
            ssmax = ga;
        }
        else
        {
            ssmax = ap::maxreal(fhmx, ga)*sqrt(1+ap::sqr(ap::minreal(fhmx, ga)/ap::maxreal(fhmx, ga)));
        }
    }
    else
    {
        if( ga<fhmx )
        {
            aas = 1+fhmn/fhmx;
            at = (fhmx-fhmn)/fhmx;
            au = ap::sqr(ga/fhmx);
            c = 2/(sqrt(aas*aas+au)+sqrt(at*at+au));
            ssmin = fhmn*c;
            ssmax = fhmx/c;
        }
        else
        {
            au = fhmx/ga;
            if( au==0 )
            {
                
                //
                // Avoid possible harmful underflow if exponent range
                // asymmetric (true SSMIN may not underflow even if
                // AU underflows)
                //
                ssmin = fhmn*fhmx/ga;
                ssmax = ga;
            }
            else
            {
                aas = 1+fhmn/fhmx;
                at = (fhmx-fhmn)/fhmx;
                c = 1/(sqrt(1+ap::sqr(aas*au))+sqrt(1+ap::sqr(at*au)));
                ssmin = fhmn*c*au;
                ssmin = ssmin+ssmin;
                ssmax = ga/(c+c);
            }
        }
    }
}


void svdv2x2(double f,
     double g,
     double h,
     double& ssmin,
     double& ssmax,
     double& snr,
     double& csr,
     double& snl,
     double& csl)
{
    bool gasmal;
    bool swp;
    int pmax;
    double a;
    double clt = 0.; // Eliminate compiler warning.
    double crt = 0.; // Eliminate compiler warning.
    double d;
    double fa;
    double ft;
    double ga;
    double gt;
    double ha;
    double ht;
    double l;
    double m;
    double mm;
    double r;
    double s;
    double slt = 0.; // Eliminate compiler warning.
    double srt = 0.; // Eliminate compiler warning.
    double t;
    double temp;
    double tsign = 0.; // Eliminate compiler warning.
    double tt;
    double v;

    ft = f;
    fa = fabs(ft);
    ht = h;
    ha = fabs(h);
    
    //
    // PMAX points to the maximum absolute element of matrix
    //  PMAX = 1 if F largest in absolute values
    //  PMAX = 2 if G largest in absolute values
    //  PMAX = 3 if H largest in absolute values
    //
    pmax = 1;
    swp = ha>fa;
    if( swp )
    {
        
        //
        // Now FA .ge. HA
        //
        pmax = 3;
        temp = ft;
        ft = ht;
        ht = temp;
        temp = fa;
        fa = ha;
        ha = temp;
    }
    gt = g;
    ga = fabs(gt);
    if( ga==0 )
    {
        
        //
        // Diagonal matrix
        //
        ssmin = ha;
        ssmax = fa;
        clt = 1;
        crt = 1;
        slt = 0;
        srt = 0;
    }
    else
    {
        gasmal = true;
        if( ga>fa )
        {
            pmax = 2;
            if( fa/ga<ap::machineepsilon )
            {
                
                //
                // Case of very large GA
                //
                gasmal = false;
                ssmax = ga;
                if( ha>1 )
                {
                    v = ga/ha;
                    ssmin = fa/v;
                }
                else
                {
                    v = fa/ga;
                    ssmin = v*ha;
                }
                clt = 1;
                slt = ht/gt;
                srt = 1;
                crt = ft/gt;
            }
        }
        if( gasmal )
        {
            
            //
            // Normal case
            //
            d = fa-ha;
            if( d==fa )
            {
                l = 1;
            }
            else
            {
                l = d/fa;
            }
            m = gt/ft;
            t = 2-l;
            mm = m*m;
            tt = t*t;
            s = sqrt(tt+mm);
            if( l==0 )
            {
                r = fabs(m);
            }
            else
            {
                r = sqrt(l*l+mm);
            }
            a = 0.5*(s+r);
            ssmin = ha/a;
            ssmax = fa*a;
            if( mm==0 )
            {
                
                //
                // Note that M is very tiny
                //
                if( l==0 )
                {
                    t = extsignbdsqr(double(2), ft)*extsignbdsqr(double(1), gt);
                }
                else
                {
                    t = gt/extsignbdsqr(d, ft)+m/t;
                }
            }
            else
            {
                t = (m/(s+t)+m/(r+l))*(1+a);
            }
            l = sqrt(t*t+4);
            crt = 2/l;
            srt = t/l;
            clt = (crt+srt*m)/a;
            v = ht/ft;
            slt = v*srt/a;
        }
    }
    if( swp )
    {
        csl = srt;
        snl = crt;
        csr = slt;
        snr = clt;
    }
    else
    {
        csl = clt;
        snl = slt;
        csr = crt;
        snr = srt;
    }
    
    //
    // Correct signs of SSMAX and SSMIN
    //
    if( pmax==1 )
    {
        tsign = extsignbdsqr(double(1), csr)*extsignbdsqr(double(1), csl)*extsignbdsqr(double(1), f);
    }
    if( pmax==2 )
    {
        tsign = extsignbdsqr(double(1), snr)*extsignbdsqr(double(1), csl)*extsignbdsqr(double(1), g);
    }
    if( pmax==3 )
    {
        tsign = extsignbdsqr(double(1), snr)*extsignbdsqr(double(1), snl)*extsignbdsqr(double(1), h);
    }
    ssmax = extsignbdsqr(ssmax, tsign);
    ssmin = extsignbdsqr(ssmin, tsign*extsignbdsqr(double(1), f)*extsignbdsqr(double(1), h));
}



