/********************************************************************
AP Library version 1.2
Copyright (c) 2003-2007, Sergey Bochkanov (ALGLIB project).
See www.alglib.net or alglib.sources.ru for details.

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
********************************************************************/

#include "alglib/ap.h"


/********************************************************************
Optimized ABLAS interface
********************************************************************/
#ifdef AP_WIN32
#include <windows.h>
extern "C"
{
typedef double  (*_ddot1)(const double*, const double*, long);
typedef void    (*_dmove1)(const double*, const double*, long);
typedef void    (*_dmoves1)(const double*, const double*, long, double);
typedef void    (*_dmoveneg1)(const double*, const double*, long);
typedef void    (*_dadd1)(const double*, const double*, long);
typedef void    (*_dadds1)(const double*, const double*, long, double);
typedef void    (*_dsub1)(const double*, const double*, long);
typedef void    (*_dmuls1)(const double*, long, double);
}
HINSTANCE ABLAS = LoadLibrary("ablas.dll");

static _ddot1     ddot1     = ABLAS==NULL ? NULL :     (_ddot1)  GetProcAddress(ABLAS, "ASMDotProduct1");
static _dmove1    dmove1    = ABLAS==NULL ? NULL :    (_dmove1)  GetProcAddress(ABLAS, "ASMMove1");
static _dmoves1   dmoves1   = ABLAS==NULL ? NULL :   (_dmoves1)  GetProcAddress(ABLAS, "ASMMoveS1");
static _dmoveneg1 dmoveneg1 = ABLAS==NULL ? NULL : (_dmoveneg1)  GetProcAddress(ABLAS, "ASMMoveNeg1");
static _dadd1     dadd1     = ABLAS==NULL ? NULL :     (_dadd1)  GetProcAddress(ABLAS, "ASMAdd1");
static _dadds1    dadds1    = ABLAS==NULL ? NULL :    (_dadds1)  GetProcAddress(ABLAS, "ASMAddS1");
static _dsub1     dsub1     = ABLAS==NULL ? NULL :     (_dsub1)  GetProcAddress(ABLAS, "ASMSub1");
static _dmuls1    dmuls1    = ABLAS==NULL ? NULL :     (_dmuls1) GetProcAddress(ABLAS, "ASMMulS1");
#endif

const double ap::machineepsilon = 5E-16;
const double ap::maxrealnumber  = 1E300;
const double ap::minrealnumber  = 1E-300;

/********************************************************************
ap::complex operations
********************************************************************/
bool ap::operator==(const ap::complex& lhs, const ap::complex& rhs)
{ return lhs.x==rhs.x && lhs.y==rhs.y; }

bool ap::operator!=(const ap::complex& lhs, const ap::complex& rhs)
{ return lhs.x!=rhs.x || lhs.y!=rhs.y; }

const ap::complex ap::operator+(const ap::complex& lhs)
{ return lhs; }

const ap::complex ap::operator-(const ap::complex& lhs)
{ return ap::complex(-lhs.x, -lhs.y); }

const ap::complex ap::operator+(const ap::complex& lhs, const ap::complex& rhs)
{ ap::complex r = lhs; r += rhs; return r; }

const ap::complex ap::operator+(const ap::complex& lhs, const double& rhs)
{ ap::complex r = lhs; r += rhs; return r; }

const ap::complex ap::operator+(const double& lhs, const ap::complex& rhs)
{ ap::complex r = rhs; r += lhs; return r; }

const ap::complex ap::operator-(const ap::complex& lhs, const ap::complex& rhs)
{ ap::complex r = lhs; r -= rhs; return r; }

const ap::complex ap::operator-(const ap::complex& lhs, const double& rhs)
{ ap::complex r = lhs; r -= rhs; return r; }

const ap::complex ap::operator-(const double& lhs, const ap::complex& rhs)
{ ap::complex r = lhs; r -= rhs; return r; }

const ap::complex ap::operator*(const ap::complex& lhs, const ap::complex& rhs)
{ return ap::complex(lhs.x*rhs.x - lhs.y*rhs.y,  lhs.x*rhs.y + lhs.y*rhs.x); }

const ap::complex ap::operator*(const ap::complex& lhs, const double& rhs)
{ return ap::complex(lhs.x*rhs,  lhs.y*rhs); }

const ap::complex ap::operator*(const double& lhs, const ap::complex& rhs)
{ return ap::complex(lhs*rhs.x,  lhs*rhs.y); }

const ap::complex ap::operator/(const ap::complex& lhs, const ap::complex& rhs)
{
    ap::complex result;
    double e;
    double f;
    if( fabs(rhs.y)<fabs(rhs.x) )
    {
        e = rhs.y/rhs.x;
        f = rhs.x+rhs.y*e;
        result.x = (lhs.x+lhs.y*e)/f;
        result.y = (lhs.y-lhs.x*e)/f;
    }
    else
    {
        e = rhs.x/rhs.y;
        f = rhs.y+rhs.x*e;
        result.x = (lhs.y+lhs.x*e)/f;
        result.y = (-lhs.x+lhs.y*e)/f;
    }
    return result;
}

const ap::complex ap::operator/(const double& lhs, const ap::complex& rhs)
{
    ap::complex result;
    double e;
    double f;
    if( fabs(rhs.y)<fabs(rhs.x) )
    {
        e = rhs.y/rhs.x;
        f = rhs.x+rhs.y*e;
        result.x = lhs/f;
        result.y = -lhs*e/f;
    }
    else
    {
        e = rhs.x/rhs.y;
        f = rhs.y+rhs.x*e;
        result.x = lhs*e/f;
        result.y = -lhs/f;
    }
    return result;
}

const ap::complex ap::operator/(const ap::complex& lhs, const double& rhs)
{ return ap::complex(lhs.x/rhs, lhs.y/rhs); }

double ap::abscomplex(const ap::complex &z)
{
    double w;
    double xabs;
    double yabs;
    double v;

    xabs = fabs(z.x);
    yabs = fabs(z.y);
    w = xabs>yabs ? xabs : yabs;
    v = xabs<yabs ? xabs : yabs; 
    if( v==0 )
        return w;
    else
    {
        double t = v/w;
        return w*sqrt(1+t*t);
    }
}

const ap::complex ap::conj(const ap::complex &z)
{ return ap::complex(z.x, -z.y); }

const ap::complex ap::csqr(const ap::complex &z)
{ return ap::complex(z.x*z.x-z.y*z.y, 2*z.x*z.y); }

/********************************************************************
BLAS functions
********************************************************************/
double ap::vdotproduct(const double *v1, const double *v2, int N)
{
#ifdef AP_WIN32
    if( ddot1!=NULL )
        return ddot1(v1, v2, N);
#endif
    return ap::_vdotproduct<double>(v1, v2, N);
}

ap::complex ap::vdotproduct(const ap::complex *v1, const ap::complex *v2, int N)
{
    return ap::_vdotproduct<ap::complex>(v1, v2, N);
}

void ap::vmove(double *vdst, const double* vsrc, int N)
{
#ifdef AP_WIN32
    if( dmove1!=NULL )
    {
        dmove1(vdst, vsrc, N);
        return;
    }
#endif
    ap::_vmove<double>(vdst, vsrc, N);
}

void ap::vmove(ap::complex *vdst, const ap::complex* vsrc, int N)
{
    ap::_vmove<ap::complex>(vdst, vsrc, N);
}

void ap::vmoveneg(double *vdst, const double *vsrc, int N)
{
#ifdef AP_WIN32
    if( dmoveneg1!=NULL )
    {
        dmoveneg1(vdst, vsrc, N);
        return;
    }
#endif
    ap::_vmoveneg<double>(vdst, vsrc, N);
}

void ap::vmoveneg(ap::complex *vdst, const ap::complex *vsrc, int N)
{
    ap::_vmoveneg<ap::complex>(vdst, vsrc, N);
}

void ap::vmove(double *vdst, const double *vsrc, int N, double alpha)
{
#ifdef AP_WIN32
    if( dmoves1!=NULL )
    {
        dmoves1(vdst, vsrc, N, alpha);
        return;
    }
#endif
    ap::_vmove2<double,double>(vdst, vsrc, N, alpha);
}

void ap::vmove(ap::complex *vdst, const ap::complex *vsrc, int N, double alpha)
{
    ap::_vmove2<ap::complex,double>(vdst, vsrc, N, alpha);
}

void ap::vmove(ap::complex *vdst, const ap::complex *vsrc, int N, ap::complex alpha)
{
    ap::_vmove2<ap::complex,ap::complex>(vdst, vsrc, N, alpha);
}

void ap::vadd(double *vdst, const double *vsrc, int N)
{
#ifdef AP_WIN32
    if( dadd1!=NULL )
    {
        dadd1(vdst, vsrc, N);
        return;
    }
#endif
    ap::_vadd<double>(vdst, vsrc, N);
}

void ap::vadd(ap::complex *vdst, const ap::complex *vsrc, int N)
{
    ap::_vadd<ap::complex>(vdst, vsrc, N);
}

void ap::vadd(double *vdst, const double *vsrc, int N, double alpha)
{
#ifdef AP_WIN32
    if( dadds1!=NULL )
    {
        dadds1(vdst, vsrc, N, alpha);
        return;
    }
#endif
    ap::_vadd2<double,double>(vdst, vsrc, N, alpha);
}

void ap::vadd(ap::complex *vdst, const ap::complex *vsrc, int N, double alpha)
{
    ap::_vadd2<ap::complex,double>(vdst, vsrc, N, alpha);
}

void ap::vadd(ap::complex *vdst, const ap::complex *vsrc, int N, ap::complex alpha)
{
    ap::_vadd2<ap::complex,ap::complex>(vdst, vsrc, N, alpha);
}

void ap::vsub(double *vdst, const double *vsrc, int N)
{
#ifdef AP_WIN32
    if( dsub1!=NULL )
    {
        dsub1(vdst, vsrc, N);
        return;
    }
#endif
    ap::_vsub<double>(vdst, vsrc, N);
}

void ap::vsub(ap::complex *vdst, const ap::complex *vsrc, int N)
{
    ap::_vsub<ap::complex>(vdst, vsrc, N);
}

void ap::vsub(double *vdst, const double *vsrc, int N, double alpha)
{
#ifdef AP_WIN32
    if( dadds1!=NULL )
    {
        dadds1(vdst, vsrc, N, -alpha);
        return;
    }
#endif
    ap::_vsub2<double,double>(vdst, vsrc, N, alpha);
}

void ap::vsub(ap::complex *vdst, const ap::complex *vsrc, int N, double alpha)
{
    ap::_vsub2<ap::complex,double>(vdst, vsrc, N, alpha);
}

void ap::vsub(ap::complex *vdst, const ap::complex *vsrc, int N, ap::complex alpha)
{
    ap::_vsub2<ap::complex,ap::complex>(vdst, vsrc, N, alpha);
}

void ap::vmul(double *vdst, int N, double alpha)
{
#ifdef AP_WIN32
    if( dmuls1!=NULL )
    {
        dmuls1(vdst, N, alpha);
        return;
    }
#endif
    ap::_vmul<double,double>(vdst, N, alpha);
}

void ap::vmul(ap::complex *vdst, int N, double alpha)
{
    ap::_vmul<ap::complex,double>(vdst, N, alpha);
}

void ap::vmul(ap::complex *vdst, int N, ap::complex alpha)
{
    ap::_vmul<ap::complex,ap::complex>(vdst, N, alpha);
}

/********************************************************************
standard functions
********************************************************************/
int ap::sign(double x)
{
    if( x>0 ) return  1;
    if( x<0 ) return -1;
    return 0;
}

double ap::randomreal()
{
    int i = rand();
    while(i==RAND_MAX)
        i =rand();
    return double(i)/double(RAND_MAX);
}

int ap::randominteger(int maxv)
{  return rand()%maxv; }

int ap::round(double x)
{ return int(floor(x+0.5)); }

int ap::trunc(double x)
{ return int(x>0 ? floor(x) : ceil(x)); }

int ap::ifloor(double x)
{ return int(floor(x)); }

int ap::iceil(double x)
{ return int(ceil(x)); }

double ap::pi()
{ return 3.14159265358979323846; }

double ap::sqr(double x)
{ return x*x; }

int ap::maxint(int m1, int m2)
{
    return m1>m2 ? m1 : m2;
}

int ap::minint(int m1, int m2)
{
    return m1>m2 ? m2 : m1;
}

double ap::maxreal(double m1, double m2)
{
    return m1>m2 ? m1 : m2;
}

double ap::minreal(double m1, double m2)
{
    return m1>m2 ? m2 : m1;
}

/********************************************************************
Service routines:
********************************************************************/
void* ap::amalloc(size_t size, size_t alignment)
{
    if( alignment<=1 )
    {
        //
        // no alignment, just call malloc
        //
        void *block = malloc(sizeof(void*)+size);
        void **p = (void**)block;
        *p = block;
        return (void*)((char*)block+sizeof(void*));
    }
    else
    {
        //
        // align.
        //
        void *block = malloc(alignment-1+sizeof(void*)+size);
        char *result = (char*)block+sizeof(void*);
        //if( ((unsigned int)(result))%alignment!=0 )
        //    result += alignment - ((unsigned int)(result))%alignment;
        if( (result-(char*)0)%alignment!=0 )
            result += alignment - (result-(char*)0)%alignment;
        *((void**)(result-sizeof(void*))) = block;
        return result;
    }
}

void ap::afree(void *block)
{
    void *p = *((void**)((char*)block-sizeof(void*)));
    free(p);
}

int ap::vlen(int n1, int n2)
{
    return n2-n1+1;
}

