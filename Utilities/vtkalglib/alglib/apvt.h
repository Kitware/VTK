/********************************************************************
Templates for AP Library
Copyright (c) 2003-2008, Sergey Bochkanov (ALGLIB project).
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
#ifndef APVT_H
#define APVT_H

/********************************************************************
Template defining vector in memory. It is used by the basic 
subroutines of linear algebra.

Vector consists of Length elements of type T, starting from an element, 
which Data is pointed to. Interval between adjacent elements equals 
the value of Step.

The class provides an access for reading only.
********************************************************************/
template<class T>
class const_raw_vector
{
public:
    const_raw_vector(const T *Data, int Length, int Step):
        pData(const_cast<T*>(Data)),iLength(Length),iStep(Step){};

    const T* GetData() const
    { return pData; };

    int GetLength() const
    { return iLength; };

    int GetStep() const
    { return iStep; };
protected:
    T       *pData;
    int     iLength, iStep;
};


/********************************************************************
Template defining vector in memory, derived from const_raw_vector.
It is used by the basic subroutines of linear algebra.

Vector consists of Length elements of type T, starting from an element, 
which Data is pointed to. Interval between adjacent elements equals 
the value of Step.

The class provides an access both for reading and writing.
********************************************************************/
template<class T>
class raw_vector : public const_raw_vector<T>
{
public:
    raw_vector(T *Data, int Length, int Step):const_raw_vector<T>(Data, Length, Step){};

    T* GetData()
    { return const_raw_vector<T>::pData; };
};


/********************************************************************
Dot product
********************************************************************/
template<class T>
T vdotproduct(const_raw_vector<T> v1, const_raw_vector<T> v2)
{
    ap_error::make_assertion(v1.GetLength()==v2.GetLength());
    if( v1.GetStep()==1 && v2.GetStep()==1 )
    {
        //
        // fast
        //
        T r = 0;
        const T *p1 = v1.GetData();
        const T *p2 = v2.GetData();
        int imax = v1.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            r += (*p1)*(*p2) + p1[1]*p2[1] + p1[2]*p2[2] + p1[3]*p2[3];
            p1+=4;
            p2+=4;
        }
        for(i=0; i<v1.GetLength()%4; i++)
            r += (*(p1++))*(*(p2++));
        return r;
    }
    else
    {
        //
        // general
        //
        int offset11 = v1.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = v2.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T r = 0;
        const T *p1 = v1.GetData();
        const T *p2 = v2.GetData();
        int imax = v1.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            r += (*p1)*(*p2) + p1[offset11]*p2[offset21] + p1[offset12]*p2[offset22] + p1[offset13]*p2[offset23];
            p1+=offset14;
            p2+=offset24;
        }
        for(i=0; i<v1.GetLength()%4; i++)
        {
            r += (*p1)*(*p2);
            p1+=offset11;
            p2+=offset21;
        }
        return r;
    }
}


/********************************************************************
Dot product, another form
********************************************************************/
template<class T>
T _vdotproduct(const T *v1, const T *v2, int N)
{
    T r = 0;
    int imax = N/4;
    int i;
    for(i=imax; i!=0; i--)
    {
        r += (*v1)*(*v2) + v1[1]*v2[1] + v1[2]*v2[2] + v1[3]*v2[3];
        v1+=4;
        v2+=4;
    }
    for(i=0; i<N%4; i++)
        r+=(*(v1++))*(*(v2++));
    return r;
}


/********************************************************************
Copy one vector into another
********************************************************************/
template<class T>
void vmove(raw_vector<T> vdst, const_raw_vector<T> vsrc)
{
    ap_error::make_assertion(vdst.GetLength()==vsrc.GetLength());
    if( vdst.GetStep()==1 && vsrc.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/2;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 = *p2;
            p1[1] = p2[1];
            p1 += 2;
            p2 += 2;
        }
        if(vdst.GetLength()%2 != 0)
            *p1 = *p2;
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = vsrc.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 = *p2;
            p1[offset11] = p2[offset21];
            p1[offset12] = p2[offset22];
            p1[offset13] = p2[offset23];
            p1 += offset14;
            p2 += offset24;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 = *p2;
            p1 += vdst.GetStep();
            p2 += vsrc.GetStep();
        }
        return;
    }
}


/********************************************************************
vmove, abother form
********************************************************************/
template<class T>
void _vmove(T *vdst, const T* vsrc, int N)
{
    int imax = N/2;
    int i;
    for(i=imax; i!=0; i--)
    {
        *vdst = *vsrc;
        vdst[1] = vsrc[1];
        vdst += 2;
        vsrc += 2;
    }
    if(N%2!=0)
        *vdst = *vsrc;
}


/********************************************************************
Copy one vector multiplied by -1 into another.
********************************************************************/
template<class T>
void vmoveneg(raw_vector<T> vdst, const_raw_vector<T> vsrc)
{
    ap_error::make_assertion(vdst.GetLength()==vsrc.GetLength());
    if( vdst.GetStep()==1 && vsrc.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/2;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 = -*p2;
            p1[1] = -p2[1];
            p1 += 2;
            p2 += 2;
        }
        if(vdst.GetLength()%2 != 0)
            *p1 = -*p2;
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = vsrc.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 = -*p2;
            p1[offset11] = -p2[offset21];
            p1[offset12] = -p2[offset22];
            p1[offset13] = -p2[offset23];
            p1 += offset14;
            p2 += offset24;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 = -*p2;
            p1 += vdst.GetStep();
            p2 += vsrc.GetStep();
        }
        return;
    }
}


/********************************************************************
vmoveneg, another form
********************************************************************/
template<class T>
void _vmoveneg(T *vdst, const T *vsrc, int N)
{
    T *p1 = vdst;
    const T *p2 = vsrc;
    int imax = N/2;
    int i;
    for(i=0; i<imax; i++)
    {
        *p1 = -*p2;
        p1[1] = -p2[1];
        p1 += 2;
        p2 += 2;
    }
    if(N%2 != 0)
        *p1 = -*p2;
}


/********************************************************************
Copy one vector multiplied by a number into another vector.
********************************************************************/
template<class T, class T2>
void vmove(raw_vector<T> vdst, const_raw_vector<T> vsrc, T2 alpha)
{
    ap_error::make_assertion(vdst.GetLength()==vsrc.GetLength());
    if( vdst.GetStep()==1 && vsrc.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 = alpha*(*p2);
            p1[1] = alpha*p2[1];
            p1[2] = alpha*p2[2];
            p1[3] = alpha*p2[3];
            p1 += 4;
            p2 += 4;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
            *(p1++) = alpha*(*(p2++));
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = vsrc.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 = alpha*(*p2);
            p1[offset11] = alpha*p2[offset21];
            p1[offset12] = alpha*p2[offset22];
            p1[offset13] = alpha*p2[offset23];
            p1 += offset14;
            p2 += offset24;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 = alpha*(*p2);
            p1 += vdst.GetStep();
            p2 += vsrc.GetStep();
        }
        return;
    }
}


/********************************************************************
vmove, another form
********************************************************************/
template<class T, class T2>
void _vmove2(T *vdst, const T *vsrc, int N, T2 alpha)
{
    T *p1 = vdst;
    const T *p2 = vsrc;
    int imax = N/4;
    int i;
    for(i=imax; i!=0; i--)
    {
        *p1 = alpha*(*p2);
        p1[1] = alpha*p2[1];
        p1[2] = alpha*p2[2];
        p1[3] = alpha*p2[3];
        p1 += 4;
        p2 += 4;
    }
    for(i=0; i<N%4; i++)
        *(p1++) = alpha*(*(p2++));
}


/********************************************************************
Vector addition
********************************************************************/
template<class T>
void vadd(raw_vector<T> vdst, const_raw_vector<T> vsrc)
{
    ap_error::make_assertion(vdst.GetLength()==vsrc.GetLength());
    if( vdst.GetStep()==1 && vsrc.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 += *p2;
            p1[1] += p2[1];
            p1[2] += p2[2];
            p1[3] += p2[3];
            p1 += 4;
            p2 += 4;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
            *(p1++) += *(p2++);
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = vsrc.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 += *p2;
            p1[offset11] += p2[offset21];
            p1[offset12] += p2[offset22];
            p1[offset13] += p2[offset23];
            p1 += offset14;
            p2 += offset24;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 += *p2;
            p1 += vdst.GetStep();
            p2 += vsrc.GetStep();
        }
        return;
    }
}


/********************************************************************
vadd, another form
********************************************************************/
template<class T>
void _vadd(T *vdst, const T *vsrc, int N)
{
    T *p1 = vdst;
    const T *p2 = vsrc;
    int imax = N/4;
    int i;
    for(i=imax; i!=0; i--)
    {
        *p1 += *p2;
        p1[1] += p2[1];
        p1[2] += p2[2];
        p1[3] += p2[3];
        p1 += 4;
        p2 += 4;
    }
    for(i=0; i<N%4; i++)
        *(p1++) += *(p2++);
}


/********************************************************************
Add one vector multiplied by a number to another vector.
********************************************************************/
template<class T, class T2>
void vadd(raw_vector<T> vdst, const_raw_vector<T> vsrc, T2 alpha)
{
    ap_error::make_assertion(vdst.GetLength()==vsrc.GetLength());
    if( vdst.GetStep()==1 && vsrc.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 += alpha*(*p2);
            p1[1] += alpha*p2[1];
            p1[2] += alpha*p2[2];
            p1[3] += alpha*p2[3];
            p1 += 4;
            p2 += 4;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
            *(p1++) += alpha*(*(p2++));
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = vsrc.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 += alpha*(*p2);
            p1[offset11] += alpha*p2[offset21];
            p1[offset12] += alpha*p2[offset22];
            p1[offset13] += alpha*p2[offset23];
            p1 += offset14;
            p2 += offset24;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 += alpha*(*p2);
            p1 += vdst.GetStep();
            p2 += vsrc.GetStep();
        }
        return;
    }
}


/********************************************************************
vadd, another form
********************************************************************/
template<class T, class T2>
void _vadd2(T *vdst, const T *vsrc, int N, T2 alpha)
{
    T *p1 = vdst;
    const T *p2 = vsrc;
    int imax = N/4;
    int i;
    for(i=imax; i!=0; i--)
    {
        *p1 += alpha*(*p2);
        p1[1] += alpha*p2[1];
        p1[2] += alpha*p2[2];
        p1[3] += alpha*p2[3];
        p1 += 4;
        p2 += 4;
    }
    for(i=0; i<N%4; i++)
        *(p1++) += alpha*(*(p2++));
}


/********************************************************************
Vector subtraction
********************************************************************/
template<class T>
void vsub(raw_vector<T> vdst, const_raw_vector<T> vsrc)
{
    ap_error::make_assertion(vdst.GetLength()==vsrc.GetLength());
    if( vdst.GetStep()==1 && vsrc.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 -= *p2;
            p1[1] -= p2[1];
            p1[2] -= p2[2];
            p1[3] -= p2[3];
            p1 += 4;
            p2 += 4;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
            *(p1++) -= *(p2++);
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        int offset21 = vsrc.GetStep(), offset22 = 2*offset21, offset23 = 3*offset21, offset24 = 4*offset21;
        T *p1 = vdst.GetData();
        const T *p2 = vsrc.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 -= *p2;
            p1[offset11] -= p2[offset21];
            p1[offset12] -= p2[offset22];
            p1[offset13] -= p2[offset23];
            p1 += offset14;
            p2 += offset24;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 -= *p2;
            p1 += vdst.GetStep();
            p2 += vsrc.GetStep();
        }
        return;
    }
}


/********************************************************************
vsub, another form
********************************************************************/
template<class T>
void _vsub(T *vdst, const T *vsrc, int N)
{
    T *p1 = vdst;
    const T *p2 = vsrc;
    int imax = N/4;
    int i;
    for(i=imax; i!=0; i--)
    {
        *p1 -= *p2;
        p1[1] -= p2[1];
        p1[2] -= p2[2];
        p1[3] -= p2[3];
        p1 += 4;
        p2 += 4;
    }
    for(i=0; i<N%4; i++)
        *(p1++) -= *(p2++);
}


/********************************************************************
Subtract one vector multiplied by a number from another vector.
********************************************************************/
template<class T, class T2>
void vsub(raw_vector<T> vdst, const_raw_vector<T> vsrc, T2 alpha)
{
    vadd(vdst, vsrc, -alpha);
}


/********************************************************************
vsub, another form
********************************************************************/
template<class T, class T2>
void _vsub2(T *vdst, const T *vsrc, int N, T2 alpha)
{
    _vadd2(vdst, vsrc, N, -alpha);
}


/********************************************************************
In-place vector multiplication
********************************************************************/
template<class T, class T2>
void vmul(raw_vector<T> vdst, T2 alpha)
{
    if( vdst.GetStep()==1 )
    {
        //
        // fast
        //
        T *p1 = vdst.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=imax; i!=0; i--)
        {
            *p1 *= alpha;
            p1[1] *= alpha;
            p1[2] *= alpha;
            p1[3] *= alpha;
            p1 += 4;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
            *(p1++) *= alpha;
        return;
    }
    else
    {
        //
        // general
        //
        int offset11 = vdst.GetStep(), offset12 = 2*offset11, offset13 = 3*offset11, offset14 = 4*offset11;
        T *p1 = vdst.GetData();
        int imax = vdst.GetLength()/4;
        int i;
        for(i=0; i<imax; i++)
        {
            *p1 *= alpha;
            p1[offset11] *= alpha;
            p1[offset12] *= alpha;
            p1[offset13] *= alpha;
            p1 += offset14;
        }
        for(i=0; i<vdst.GetLength()%4; i++)
        {
            *p1 *= alpha;
            p1 += vdst.GetStep();
        }
        return;
    }
}


/********************************************************************
vmul, another form
********************************************************************/
template<class T, class T2>
void _vmul(T *vdst, int N, T2 alpha)
{
    T *p1 = vdst;
    int imax = N/4;
    int i;
    for(i=imax; i!=0; i--)
    {
        *p1 *= alpha;
        p1[1] *= alpha;
        p1[2] *= alpha;
        p1[3] *= alpha;
        p1 += 4;
    }
    for(i=0; i<N%4; i++)
        *(p1++) *= alpha;
}

#endif
