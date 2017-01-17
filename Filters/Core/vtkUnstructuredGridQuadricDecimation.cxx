/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridQuadricDecimation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  Copyright 2007, 2008 by University of Utah.

=========================================================================*/
#include "vtkUnstructuredGridQuadricDecimation.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPoints.h"
#include "vtkDataSetAttributes.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include <map>

class vtkUnstructuredGridQuadricDecimationEdge;
class vtkUnstructuredGridQuadricDecimationFace;
class vtkUnstructuredGridQuadricDecimationFaceHash;
class vtkUnstructuredGridQuadricDecimationFaceMap;
class vtkUnstructuredGridQuadricDecimationVec4;
class vtkUnstructuredGridQuadricDecimationSymMat4;
class vtkUnstructuredGridQuadricDecimationQEF;
class vtkUnstructuredGridQuadricDecimationTetra;
class vtkUnstructuredGridQuadricDecimationVertex;
class vtkUnstructuredGridQuadricDecimationTetMesh;


// floating point epsilons
#define VTK_FEPS 1e-6
#define VTK_TEPS 1e-6
#define VTK_SWAP(a, b, type)                    \
  { type t = a; a = b; b = t; }
#define VTK_PRECHECK(pointer)                   \
  if (pointer)                                  \
    delete [] (pointer);                        \
  (pointer)

// =============================================================================
// Vector 4 class
class vtkUnstructuredGridQuadricDecimationVec4
{
public:
  vtkUnstructuredGridQuadricDecimationVec4()
  {
    memset(values, 0, sizeof(values));
  }

  vtkUnstructuredGridQuadricDecimationVec4(float v1, float v2,
                                           float v3, float v4)
  {
    values[0] = v1;
    values[1] = v2;
    values[2] = v3;
    values[3] = v4;
  }

  float & operator[](const int &index)
  {
    return values[index];
  }
  float operator[](const int &index) const
  {
    return values[index];
  }

  vtkUnstructuredGridQuadricDecimationVec4 operator+(
    const vtkUnstructuredGridQuadricDecimationVec4 &v) const
  {
    return vtkUnstructuredGridQuadricDecimationVec4(values[0] + v.values[0],
                                                    values[1] + v.values[1],
                                                    values[2] + v.values[2],
                                                    values[3] + v.values[3]);
  }

  vtkUnstructuredGridQuadricDecimationVec4 operator-(
    const vtkUnstructuredGridQuadricDecimationVec4 &v) const
  {
    return vtkUnstructuredGridQuadricDecimationVec4(values[0] - v.values[0],
                                                    values[1] - v.values[1],
                                                    values[2] - v.values[2],
                                                    values[3] - v.values[3]);
  }

  vtkUnstructuredGridQuadricDecimationVec4 operator*(const float &f) const
  {
    return vtkUnstructuredGridQuadricDecimationVec4(values[0] * f,
                                                    values[1] * f,
                                                    values[2] * f,
                                                    values[3] * f);
  }
  float operator*(const vtkUnstructuredGridQuadricDecimationVec4 &v) const
  {
    return
      values[0] * v.values[0] +
      values[1] * v.values[1] +
      values[2] * v.values[2] +
      values[3] * v.values[3];
  }

  vtkUnstructuredGridQuadricDecimationVec4 operator/(const float &f) const
  {
    return vtkUnstructuredGridQuadricDecimationVec4(values[0] / f,
                                                    values[1] / f,
                                                    values[2] / f,
                                                    values[3] / f);
  }

  const vtkUnstructuredGridQuadricDecimationVec4& operator*=(const float &f)
  {
    values[0] *= f;
    values[1] *= f;
    values[2] *= f;
    values[3] *= f;
    return *this;
  }

  const vtkUnstructuredGridQuadricDecimationVec4& operator/=(const float &f)
  {
    values[0] /= f;
    values[1] /= f;
    values[2] /= f;
    values[3] /= f;
    return *this;
  }

  const vtkUnstructuredGridQuadricDecimationVec4& operator+=(
    const vtkUnstructuredGridQuadricDecimationVec4 &v)
  {
    values[0] += v.values[0];
    values[1] += v.values[1];
    values[2] += v.values[2];
    values[3] += v.values[3];
    return *this;
  }

  const vtkUnstructuredGridQuadricDecimationVec4& operator-=(
    const vtkUnstructuredGridQuadricDecimationVec4 &v)
  {
    values[0] -= v.values[0];
    values[1] -= v.values[1];
    values[2] -= v.values[2];
    values[3] -= v.values[3];
    return *this;
  }

  // dot product
  float Dot(const vtkUnstructuredGridQuadricDecimationVec4 &v) const
  {
    return
      values[0] * v.values[0] +
      values[1] * v.values[1] +
      values[2] * v.values[2] +
      values[3] * v.values[3];
  }

  // A = e * eT
  vtkUnstructuredGridQuadricDecimationSymMat4 MultTransposeSym() const;

  float Length() const
  {
    return sqrt(values[0]*values[0] + values[1]*values[1] +
                values[2]*values[2] + values[3]*values[3]);
  }

  void Normalize()
  {
    float len = Length();
    if (len!=0)
    {
      values[0] /= len;
      values[1] /= len;
      values[2] /= len;
      values[3] /= len;
    }
  }

  float values[4];
};

// =============================================================================
// Symmetric 4x4 Matrix class
// Storing lower half
#define SM4op(i, OP)                            \
  result.values[i] = values[i] OP m.values[i];

class vtkUnstructuredGridQuadricDecimationSymMat4
{
public:
  vtkUnstructuredGridQuadricDecimationSymMat4()
  {
    memset(values, 0, sizeof(values));
  }
  void Identity()
  {
    memset(values, 0, sizeof(values));
    values[0] = values[2] = values[5] = values[9] = 1.;
  }

  vtkUnstructuredGridQuadricDecimationSymMat4 operator+(
    const vtkUnstructuredGridQuadricDecimationSymMat4 &m) const
  {
    static vtkUnstructuredGridQuadricDecimationSymMat4 result;
    SM4op(0,+);
    SM4op(1,+);
    SM4op(2,+);
    SM4op(3,+);
    SM4op(4,+);
    SM4op(5,+);
    SM4op(6,+);
    SM4op(7,+);
    SM4op(8,+);
    SM4op(9,+);
    return result;
  }

  vtkUnstructuredGridQuadricDecimationSymMat4 operator-(
    const vtkUnstructuredGridQuadricDecimationSymMat4 &m) const
  {
    static vtkUnstructuredGridQuadricDecimationSymMat4 result;
    SM4op(0,-);
    SM4op(1,-);
    SM4op(2,-);
    SM4op(3,-);
    SM4op(4,-);
    SM4op(5,-);
    SM4op(6,-);
    SM4op(7,-);
    SM4op(8,-);
    SM4op(9,-);
    return result;
  }

  vtkUnstructuredGridQuadricDecimationVec4 operator*(
    const vtkUnstructuredGridQuadricDecimationVec4 &v) const
  {
    return vtkUnstructuredGridQuadricDecimationVec4(
      values[0]*v.values[0] + values[1]*v.values[1] +
      values[3]*v.values[2] + values[6]*v.values[3],
      values[1]*v.values[0] + values[2]*v.values[1] +
      values[4]*v.values[2] + values[7]*v.values[3],
      values[3]*v.values[0] + values[4]*v.values[1] +
      values[5]*v.values[2] + values[8]*v.values[3],
      values[6]*v.values[0] + values[7]*v.values[1] +
      values[8]*v.values[2] + values[9]*v.values[3]);
  }

  vtkUnstructuredGridQuadricDecimationSymMat4 operator*(const float &f) const
  {
    vtkUnstructuredGridQuadricDecimationSymMat4 result;
    for (int i=0; i<10; i++)
    {
      result.values[i] = values[i] * f;
    }
    return result;
  }

  vtkUnstructuredGridQuadricDecimationSymMat4 operator/(const float &f) const
  {
    vtkUnstructuredGridQuadricDecimationSymMat4 result;
    for (int i=0; i<10; i++)
    {
      result.values[i] = values[i] / f;
    }
    return result;
  }

  const vtkUnstructuredGridQuadricDecimationSymMat4& operator*=(const float &f)
  {
    for (int i=0; i<10; i++)
    {
      values[i] *= f;
    }
    return *this;
  }

  const vtkUnstructuredGridQuadricDecimationSymMat4& operator/=(const float &f)
  {
    for (int i=0; i<10; i++)
    {
      values[i] /= f;
    }
    return *this;
  }

  const vtkUnstructuredGridQuadricDecimationSymMat4& operator+=(
    const vtkUnstructuredGridQuadricDecimationSymMat4 &m)
  {
    for (int i=0; i<10; i++)
    {
      values[i] += m.values[i];
    }
    return *this;
  }

  const vtkUnstructuredGridQuadricDecimationSymMat4& operator-=(
    const vtkUnstructuredGridQuadricDecimationSymMat4 &m)
  {
    for (int i=0; i<10; i++)
    {
      values[i] -= m.values[i];
    }
    return *this;
  }


  float square(const vtkUnstructuredGridQuadricDecimationVec4 &v) const
  {
    return
      v.values[0] * (values[0]*v.values[0] + values[1]*v.values[1] +
                     values[3]*v.values[2] + values[6]*v.values[3]) +
      v.values[1] * ( values[1]*v.values[0] + values[2]*v.values[1] +
                      values[4]*v.values[2] + values[7]*v.values[3]) +
      v.values[2] * (values[3]*v.values[0] + values[4]*v.values[1] +
                     values[5]*v.values[2] + values[8]*v.values[3]) +
      v.values[3] * (values[6]*v.values[0] + values[7]*v.values[1] +
                     values[8]*v.values[2] + values[9]*v.values[3]);

  }

  bool ConjugateR(const vtkUnstructuredGridQuadricDecimationSymMat4 &A1,
                  const vtkUnstructuredGridQuadricDecimationSymMat4 &A2,
                  const vtkUnstructuredGridQuadricDecimationVec4 &p1,
                  vtkUnstructuredGridQuadricDecimationVec4 &x) const;
  float values[10];
};
#undef SM4op

vtkUnstructuredGridQuadricDecimationSymMat4
vtkUnstructuredGridQuadricDecimationVec4::MultTransposeSym() const
{
  static vtkUnstructuredGridQuadricDecimationSymMat4 result;
  result.values[0] = values[0] * values[0];
  result.values[1] = values[0] * values[1];
  result.values[2] = values[1] * values[1];
  result.values[3] = values[0] * values[2];
  result.values[4] = values[1] * values[2];
  result.values[5] = values[2] * values[2];
  result.values[6] = values[0] * values[3];
  result.values[7] = values[1] * values[3];
  result.values[8] = values[2] * values[3];
  result.values[9] = values[3] * values[3];
  return result;
}

bool vtkUnstructuredGridQuadricDecimationSymMat4::ConjugateR(
  const vtkUnstructuredGridQuadricDecimationSymMat4 &A1,
  const vtkUnstructuredGridQuadricDecimationSymMat4 &A2,
  const vtkUnstructuredGridQuadricDecimationVec4 &p1,
  vtkUnstructuredGridQuadricDecimationVec4 &x) const
{
  float e(1e-3 / 4. * (values[0] + values[2] + values[5] + values[9]));
  vtkUnstructuredGridQuadricDecimationVec4 r((A1-A2)*(p1-x));
  vtkUnstructuredGridQuadricDecimationVec4 p;
  for (int k=0; k<4; k++)
  {
    float s(r.Dot(r));
    if (s<=0)
    {
      break;
    }
    p += (r/s);
    vtkUnstructuredGridQuadricDecimationVec4 q((*this)*p);
    float t(p.Dot(q));
    if (s*t<=e)
    {
      break;
    }
    r -= (q/t);
    x += (p/t);
  }
  return true;
}

// =============================================================================
// QEF (QEF Error Function) Representation
class vtkUnstructuredGridQuadricDecimationQEF
{
public:
  vtkUnstructuredGridQuadricDecimationQEF(): p(0, 0, 0, 0), e(0.0)
  {}
  vtkUnstructuredGridQuadricDecimationQEF(
    const vtkUnstructuredGridQuadricDecimationSymMat4 &AA,
    const vtkUnstructuredGridQuadricDecimationVec4 &pp, const double &ee):
    A(AA), p(pp), e(ee) {}
  vtkUnstructuredGridQuadricDecimationQEF(
    const vtkUnstructuredGridQuadricDecimationQEF &Q1,
    const vtkUnstructuredGridQuadricDecimationQEF &Q2,
    const vtkUnstructuredGridQuadricDecimationVec4 &x):
    A(Q1.A+Q2.A), p(x)
  {
    A.ConjugateR(Q1.A, Q2.A, Q1.p, p);
    e = Q1.e + Q2.e + Q1.A.square(p-Q1.p) + Q2.A.square(p-Q2.p);
  }

  void Zeros()
  {
    memset(A.values, 0, sizeof(A.values));
    memset(p.values, 0, sizeof(p.values));
    e = 0.0;
  }

  void Sum(const vtkUnstructuredGridQuadricDecimationQEF &Q1,
           const vtkUnstructuredGridQuadricDecimationQEF &Q2,
           const vtkUnstructuredGridQuadricDecimationVec4 &x)
  {
    A = Q1.A + Q2.A;
    p = x;
    A.ConjugateR(Q1.A, Q2.A, Q1.p, p);
    e = Q1.e + Q2.e + Q1.A.square(p-Q1.p)+Q2.A.square(p-Q2.p);
  }

  void Sum(const vtkUnstructuredGridQuadricDecimationQEF &Q1,
           const vtkUnstructuredGridQuadricDecimationQEF &Q2)
  {
    A = Q1.A + Q2.A;
    p = (Q1.p + Q2.p) * 0.5;
    A.ConjugateR(Q1.A, Q2.A, Q1.p, p);
    e = Q1.e + Q2.e + Q1.A.square(p-Q1.p) + Q2.A.square(p-Q2.p);
  }


  void Scale(const double &f)
  {
    A *= f;
    p *= f;
    e *= f;
  }

  vtkUnstructuredGridQuadricDecimationSymMat4 A;
  vtkUnstructuredGridQuadricDecimationVec4 p;
  float e;
};

class vtkUnstructuredGridQuadricDecimationVertex
{
public:
  vtkUnstructuredGridQuadricDecimationVertex(): Corner(-1)
  {}
  vtkUnstructuredGridQuadricDecimationVertex(float ix, float iy,
                                             float iz, float is = 0.0):
    Corner(-1)
  {
    Q.p = vtkUnstructuredGridQuadricDecimationVec4(ix, iy, iz, is);
  }

  vtkUnstructuredGridQuadricDecimationQEF Q;

  int Corner;
};

class vtkUnstructuredGridQuadricDecimationEdge
{
public:
  // NOTE: vertex list are always sorted
  vtkUnstructuredGridQuadricDecimationEdge()
  {
    Verts[0] = Verts[1] = NULL;
  }
  vtkUnstructuredGridQuadricDecimationEdge(
    vtkUnstructuredGridQuadricDecimationVertex *va,
    vtkUnstructuredGridQuadricDecimationVertex *vb)
  {
    Verts[0] = va;
    Verts[1] = vb;
    SortVerts();
  }

  bool operator==(const vtkUnstructuredGridQuadricDecimationEdge &e) const
  {
    return (Verts[0]==e.Verts[0] && Verts[1]==e.Verts[1]);
  }

  // '<' operand is useful with hash_map
  bool operator<(const vtkUnstructuredGridQuadricDecimationEdge &e) const
  {
    if (Verts[0]<e.Verts[0])
    {
      return true;
    }
    else
    {
      if (Verts[0]==e.Verts[0] && Verts[1]<e.Verts[1])
      {
        return true;
      }
    }
    return false;
  }

  // sort the vertices to be increasing
  void SortVerts()
  {
    if (Verts[0]>Verts[1])
    {
      vtkUnstructuredGridQuadricDecimationVertex *v = Verts[1];
      Verts[1] = Verts[0];
      Verts[0] = v;
    }
  }

  void ChangeVerts(vtkUnstructuredGridQuadricDecimationVertex *v1,
                   vtkUnstructuredGridQuadricDecimationVertex *v2)
  {
    Verts[0] = v1;
    Verts[1] = v2;
    SortVerts();
  }

  // 2 ends of the edge
  vtkUnstructuredGridQuadricDecimationVertex *Verts[2];
};

class vtkUnstructuredGridQuadricDecimationFace
{
public:
  vtkUnstructuredGridQuadricDecimationFace(
    vtkUnstructuredGridQuadricDecimationVertex *va,
    vtkUnstructuredGridQuadricDecimationVertex *vb,
    vtkUnstructuredGridQuadricDecimationVertex *vc)
  {
    Verts[0] = va;
    Verts[1] = vb;
    Verts[2] = vc;
    SortVerts();
  }

  bool operator==(const vtkUnstructuredGridQuadricDecimationFace &f) const
  {
    return (Verts[0]==f.Verts[0] && Verts[1]==f.Verts[1]
            && Verts[2]==f.Verts[2]);
  }

  // compare by sorting the verts and one-one comparison
  bool operator<(const vtkUnstructuredGridQuadricDecimationFace &f) const
  {
    for (int i = 0; i<3; i++)
    {
      if (Verts[i]<f.Verts[i])
      {
        return true;
      }
      else
      {
        if (Verts[i]>f.Verts[i])
        {
          return false;
        }
      }
    }
    return false;
  }

  // has the magnitude of 2 * area of this face
  // we don't care about if it pos or neg now
  double Orientation() const;

  // compute the normal of the face
  vtkUnstructuredGridQuadricDecimationVec4 Normal() const;

  // check to see if a vertex belongs to this face
  bool ContainVertex(vtkUnstructuredGridQuadricDecimationVertex *v) const;

  // change vertex v1 on the list to vertex v2 (for edge collapsing)
  void ChangeVertex(vtkUnstructuredGridQuadricDecimationVertex *fromV,
                    vtkUnstructuredGridQuadricDecimationVertex *toV);

  // sort the vertices
  void SortVerts()
  {
    if (Verts[1]<Verts[0] && Verts[1]<Verts[2])
    {
      VTK_SWAP(Verts[0], Verts[1], vtkUnstructuredGridQuadricDecimationVertex*);
    }

    if (Verts[2]<Verts[0] && Verts[2]<Verts[1])
    {
      VTK_SWAP(Verts[0], Verts[2], vtkUnstructuredGridQuadricDecimationVertex*);
    }

    if (Verts[2]<Verts[1])
    {
      VTK_SWAP(Verts[1], Verts[2], vtkUnstructuredGridQuadricDecimationVertex*);
    }
  }

  vtkUnstructuredGridQuadricDecimationVertex *Verts[3];

  // find the orthonormal e1, e2, the tangent plane
  void FindOrthonormal(vtkUnstructuredGridQuadricDecimationVec4 &e1,
                       vtkUnstructuredGridQuadricDecimationVec4 &e2) const;

  // Compute the Quadric Error for this face
  void UpdateQuadric(float boundaryWeight = 1.);
};

//=============================================================
// Face_hash class declaration
// bundle of support functions for Face hash_map
class vtkUnstructuredGridQuadricDecimationFaceHash
{
public:
  // This is the average load that the hash table tries to maintain.
  static const size_t bucket_size = 4;

  // This is the minimum number of buckets in the hash table.  It must be
  // a positive power of two.
  static const size_t min_buckets = 8;

  // This method must define an ordering on two faces.  It is used to compare
  // faces that has to the same bucket.
  bool operator() (const vtkUnstructuredGridQuadricDecimationFace &f1,
                   const vtkUnstructuredGridQuadricDecimationFace &f2) const
  {
    return f1 < f2;
  }

  // This produces the actual hash code for the Face
  size_t operator() (const vtkUnstructuredGridQuadricDecimationFace &f) const
  {
    return (size_t)f.Verts[0] * (size_t)f.Verts[1] * (size_t)f.Verts[2];
  }
};

//=============================================================
// FaceMap declaration
// try to make a hash_map from Face -> Face *
typedef std::map<vtkUnstructuredGridQuadricDecimationFace,
        vtkUnstructuredGridQuadricDecimationFace *,
        vtkUnstructuredGridQuadricDecimationFaceHash>
        vtkUnstructuredGridQuadricDecimationFaceHashMap;

class vtkUnstructuredGridQuadricDecimationFaceMap
{
public:
  vtkUnstructuredGridQuadricDecimationFaceMap()
  {}
  ~vtkUnstructuredGridQuadricDecimationFaceMap()
  {
    clear();
  }

  // for iteration
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator begin()
  {
    return faces.begin();
  }
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator end()
  {
    return faces.end();
  }

  // clear
  void clear();
  size_t size() const
  {
    return faces.size();
  }

  // insert a new face
  vtkUnstructuredGridQuadricDecimationFace * AddFace(
    const vtkUnstructuredGridQuadricDecimationFace &f);

  // return the face that is the same as f
  // if there's no such face, then return NULL
  vtkUnstructuredGridQuadricDecimationFace *GetFace(
    const vtkUnstructuredGridQuadricDecimationFace &f);

  // remove the face has the content f
  void RemoveFace(const vtkUnstructuredGridQuadricDecimationFace &f);

  // add a face, and check if it can't be a border face
  // then kill it. Return NULL -> failed adding
  vtkUnstructuredGridQuadricDecimationFace * AddFaceBorder(
    const vtkUnstructuredGridQuadricDecimationFace &f);

  vtkUnstructuredGridQuadricDecimationFaceHashMap faces;
private:
  // add face without checking existence
  vtkUnstructuredGridQuadricDecimationFace * DirectAddFace(
    const vtkUnstructuredGridQuadricDecimationFace &f);

  // remove face without checking existence
  void DirectRemoveFace(vtkUnstructuredGridQuadricDecimationFace *f);
  void DirectRemoveFace(
    vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i);
};

//=============================================================
// Face class implementation
// NOTE: the vertices of the Face are always sorted!
// construct the face, sort the vertice as well
// calculate the orientation, or 2*area of the face
double vtkUnstructuredGridQuadricDecimationFace::Orientation() const
{
  vtkUnstructuredGridQuadricDecimationVec4 v;
  // this is cross product of v1-v0 and v2-v0
  v[0] = (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]) *
         (Verts[2]->Q.p[2] - Verts[0]->Q.p[2])
         - (Verts[2]->Q.p[1] - Verts[0]->Q.p[1]) *
         (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[1] = - (Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[2]->Q.p[2] - Verts[0]->Q.p[2])
         + (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[2] = (Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[2]->Q.p[1] - Verts[0]->Q.p[1])
         - (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]);

  v[3] = 0;

  return v.Length();
}

vtkUnstructuredGridQuadricDecimationVec4
vtkUnstructuredGridQuadricDecimationFace::Normal() const
{
  vtkUnstructuredGridQuadricDecimationVec4 v;
  // this is cross product of v1-v0 and v2-v0
  v[0] = (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]) *
         (Verts[2]->Q.p[2] - Verts[0]->Q.p[2])
         - (Verts[2]->Q.p[1] - Verts[0]->Q.p[1]) *
         (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[1] = - (Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[2]->Q.p[2] - Verts[0]->Q.p[2])
         + (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[1]->Q.p[2] - Verts[0]->Q.p[2]);

  v[2] = (Verts[1]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[2]->Q.p[1] - Verts[0]->Q.p[1])
         - (Verts[2]->Q.p[0] - Verts[0]->Q.p[0]) *
         (Verts[1]->Q.p[1] - Verts[0]->Q.p[1]);

  v[3] = 0;
  return v/v.Length();
}

// Compute the Quadric Error for this face
void vtkUnstructuredGridQuadricDecimationFace::UpdateQuadric(
  float boundaryWeight)
{
  vtkUnstructuredGridQuadricDecimationVec4 e1, e2;

  e1 = Verts[1]->Q.p - Verts[0]->Q.p;
  e2 = Verts[2]->Q.p - Verts[0]->Q.p;

  e1.Normalize();

  e2 = e2 - e1*e2.Dot(e1);
  e2.Normalize();

  // A = I - e1.e1T - e2.e2T
  static vtkUnstructuredGridQuadricDecimationSymMat4 A;
  A.Identity();
  A -= (e1.MultTransposeSym() + e2.MultTransposeSym());
  A *= fabs(Orientation())/6.0 * boundaryWeight;
  //  A *= boundaryWeight;
  for (int i=0; i<3; i++)
  {
    Verts[i]->Q.A += A;
  }
}

bool vtkUnstructuredGridQuadricDecimationFace::ContainVertex(
  vtkUnstructuredGridQuadricDecimationVertex *v) const
{
  for (int i=0; i<3; i++)
  {
    if (Verts[i]==v)
    {
      return true;
    }
  }
  return false;
}

// change vertex v1 on the list to vertex v2 (for edge collapsing)
void vtkUnstructuredGridQuadricDecimationFace::ChangeVertex(
  vtkUnstructuredGridQuadricDecimationVertex *v1,
  vtkUnstructuredGridQuadricDecimationVertex *v2)
{
  for (int i=0; i<3; i++)
  {
    if (Verts[i]==v1)
    {
      Verts[i] = v2;
    }
  }
  SortVerts();
}

//=============================================================
// FaceMap class implementation
void vtkUnstructuredGridQuadricDecimationFaceMap::clear()
{
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.begin();
  // free all the memory
  while (i!=faces.end())
  {
    delete (*i).second;
    ++i;
  }
  // clear the hash table
  faces.clear();
}

// insert in new faces
vtkUnstructuredGridQuadricDecimationFace *
vtkUnstructuredGridQuadricDecimationFaceMap::AddFace(
  const vtkUnstructuredGridQuadricDecimationFace &f)
{
  if (GetFace(f)==NULL)
  {
    return DirectAddFace(f);
  }
  else
  {
    return NULL;
  }
}

// return the face that is the same as f
// if there's no such face, then return NULL
vtkUnstructuredGridQuadricDecimationFace *
vtkUnstructuredGridQuadricDecimationFaceMap::GetFace(
  const vtkUnstructuredGridQuadricDecimationFace &f)
{
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.find(f);
  if (i!=faces.end())
  {
    return (*i).second;
  }
  else
  {
    return NULL;
  }
}

// remove the face has the content f
void vtkUnstructuredGridQuadricDecimationFaceMap::RemoveFace(
  const vtkUnstructuredGridQuadricDecimationFace &f)
{
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.find(f);
  if (i!=faces.end())
  {
    DirectRemoveFace(i);
  }
}

// add a face, and check if it can't be a border face
// then kill it. Return NULL -> failed adding
vtkUnstructuredGridQuadricDecimationFace *
vtkUnstructuredGridQuadricDecimationFaceMap::AddFaceBorder(
  const vtkUnstructuredGridQuadricDecimationFace &f)
{
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i = faces.find(f);
  if (i!=faces.end())
  {
    // exist -> has 2 tets -> not a border -> kill it
    DirectRemoveFace(i);
    return NULL;
  }
  else
  {
    // not exist -> add it in
    return DirectAddFace(f);
  }
}

// add face without checking existence
vtkUnstructuredGridQuadricDecimationFace *
vtkUnstructuredGridQuadricDecimationFaceMap::DirectAddFace(
  const vtkUnstructuredGridQuadricDecimationFace &f)
{
  vtkUnstructuredGridQuadricDecimationFace *newF =
    new vtkUnstructuredGridQuadricDecimationFace(f);
  faces[f] = newF;
  return newF;
}

// remove face without checking existence
void vtkUnstructuredGridQuadricDecimationFaceMap::DirectRemoveFace(
  vtkUnstructuredGridQuadricDecimationFace *f)
{
  faces.erase(*f);
  delete f;
}

void vtkUnstructuredGridQuadricDecimationFaceMap::DirectRemoveFace(
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator i)
{
  vtkUnstructuredGridQuadricDecimationFace *f = (*i).second;
  faces.erase(i);
  delete f;
}

class vtkUnstructuredGridQuadricDecimationTetra
{
public:
  vtkUnstructuredGridQuadricDecimationTetra(): index(-1) {}
  vtkUnstructuredGridQuadricDecimationTetra(
    vtkUnstructuredGridQuadricDecimationVertex *va,
    vtkUnstructuredGridQuadricDecimationVertex *vb,
    vtkUnstructuredGridQuadricDecimationVertex *vc,
    vtkUnstructuredGridQuadricDecimationVertex *vd)
  {
    Verts[0] = va;
    Verts[1] = vb;
    Verts[2] = vc;
    Verts[3] = vd;
  }

  // pointers to 4 vertices
  vtkUnstructuredGridQuadricDecimationVertex *Verts[4];

  //
  // the orientation of this order of vertices
  // positive - good orientation
  // zero - all in one plane
  // negative - bad orientation
  //
  // it is also 6 times the volume of this tetrahedra
  float Orientation() const;
  float Orientation(const vtkUnstructuredGridQuadricDecimationVec4 &v0,
                    const vtkUnstructuredGridQuadricDecimationVec4 &v1,
                    const vtkUnstructuredGridQuadricDecimationVec4 &v2,
                    const vtkUnstructuredGridQuadricDecimationVec4 &v3) const;

  // swap vertices so that the orientation is positive
  void FixOrientation();

  // check to see if a vertex belongs to this tetrahedron
  bool ContainVertex(vtkUnstructuredGridQuadricDecimationVertex *v) const
  {
    if (Verts[0]==v || Verts[1]==v || Verts[2]==v || Verts[3]==v)
    {
      return true;
    }
    return false;
  }

  // check to see if we can change fromV to toV without changing the orietation
  bool Changeable(vtkUnstructuredGridQuadricDecimationVertex *fromV,
                  const vtkUnstructuredGridQuadricDecimationVec4 &v4)
  {
    if (fromV==Verts[0])
    {
      return
        Orientation(v4, Verts[1]->Q.p, Verts[2]->Q.p, Verts[3]->Q.p)>VTK_TEPS;
    }
    if (fromV==Verts[1])
    {
      return
        Orientation(Verts[0]->Q.p, v4, Verts[2]->Q.p, Verts[3]->Q.p)>VTK_TEPS;
    }
    if (fromV==Verts[2])
    {
      return
        Orientation(Verts[0]->Q.p, Verts[1]->Q.p, v4, Verts[3]->Q.p)>VTK_TEPS;
    }
    if (fromV==Verts[3])
    {
      return
        Orientation(Verts[0]->Q.p, Verts[1]->Q.p, Verts[2]->Q.p, v4)>VTK_TEPS;
    }
    return true;
  }

  // change vertex v1 on the list to vertex v2 (for edge collapsing)
  void ChangeVertex(vtkUnstructuredGridQuadricDecimationVertex *fromV,
                    vtkUnstructuredGridQuadricDecimationVertex *toV);

  // find the orthonormal e1, e2, e3, the tangent space
  void FindOrthonormal(vtkUnstructuredGridQuadricDecimationVec4 &e1,
                       vtkUnstructuredGridQuadricDecimationVec4 &e2,
                       vtkUnstructuredGridQuadricDecimationVec4 &e3) const;

  // Compute the Quadric Error for this tetrahedron
  void UpdateQuadric();

  int index;
};

#define U(c) (Verts[1]->Q.p[c] - Verts[0]->Q.p[c])
#define V(c) (Verts[2]->Q.p[c] - Verts[0]->Q.p[c])
#define W(c) (Verts[3]->Q.p[c] - Verts[0]->Q.p[c])
float vtkUnstructuredGridQuadricDecimationTetra::Orientation() const
{
  return
    U(0) * (V(1)*W(2) - V(2)*W(1)) -
    V(0) * (U(1)*W(2) - U(2)*W(1)) +
    W(0) * (U(1)*V(2) - U(2)*V(1));
}
#undef U
#undef V
#undef W

#define U(c) (v1[c] - v0[c])
#define V(c) (v2[c] - v0[c])
#define W(c) (v3[c] - v0[c])
float vtkUnstructuredGridQuadricDecimationTetra::Orientation(
  const vtkUnstructuredGridQuadricDecimationVec4 &v0,
  const vtkUnstructuredGridQuadricDecimationVec4 &v1,
  const vtkUnstructuredGridQuadricDecimationVec4 &v2,
  const vtkUnstructuredGridQuadricDecimationVec4 &v3) const
{
  return
    U(0) * (V(1)*W(2) - V(2)*W(1)) -
    V(0) * (U(1)*W(2) - U(2)*W(1)) +
    W(0) * (U(1)*V(2) - U(2)*V(1));
}
#undef U
#undef V
#undef W

void vtkUnstructuredGridQuadricDecimationTetra::FixOrientation()
{
  if (Orientation()<0)
  {
    VTK_SWAP(Verts[2], Verts[3], vtkUnstructuredGridQuadricDecimationVertex *);
  }
  if (Orientation()<0)
  {
    VTK_SWAP(Verts[1], Verts[2], vtkUnstructuredGridQuadricDecimationVertex *);
  }
}

// change vertex v1 on the list to vertex v2 (for edge collapsing)
void vtkUnstructuredGridQuadricDecimationTetra::ChangeVertex(
  vtkUnstructuredGridQuadricDecimationVertex *v1,
  vtkUnstructuredGridQuadricDecimationVertex *v2)
{
  for (int i=0; i<4; i++)
  {
    if (Verts[i]==v1)
    {
      Verts[i] = v2;
    }
  }
}

// find the orthonormal tangent space e1, e2, e3
void vtkUnstructuredGridQuadricDecimationTetra::FindOrthonormal(
  vtkUnstructuredGridQuadricDecimationVec4 &e1,
  vtkUnstructuredGridQuadricDecimationVec4 &e2,
  vtkUnstructuredGridQuadricDecimationVec4 &e3) const
{
  vtkUnstructuredGridQuadricDecimationVec4 e0(Verts[0]->Q.p);

  // Ei = Ui - U0
  e1 = Verts[1]->Q.p - e0;
  e2 = Verts[2]->Q.p - e0;
  e3 = Verts[3]->Q.p - e0;

  e1.Normalize();

  e2 = e2 - e1*e2.Dot(e1);
  e2.Normalize();

  e3 = e3 - e1*e3.Dot(e1) - e2*e3.Dot(e2);
  e3.Normalize();
}

// compute the quadric error based on this tet
#define ax a.values[0]
#define ay a.values[1]
#define az a.values[2]
#define af a.values[3]
#define bx b.values[0]
#define by b.values[1]
#define bz b.values[2]
#define bf b.values[3]
#define cx c.values[0]
#define cy c.values[1]
#define cz c.values[2]
#define cf c.values[3]
void vtkUnstructuredGridQuadricDecimationTetra::UpdateQuadric()
{
  vtkUnstructuredGridQuadricDecimationVec4 a(Verts[1]->Q.p - Verts[0]->Q.p);
  vtkUnstructuredGridQuadricDecimationVec4 b(Verts[2]->Q.p - Verts[0]->Q.p);
  vtkUnstructuredGridQuadricDecimationVec4 c(Verts[3]->Q.p - Verts[0]->Q.p);
  vtkUnstructuredGridQuadricDecimationVec4
    n(ay*(bz*cf-bf*cz) + az*(bf*cy-by*cf) + af*(by*cz-bz*cy),
      az*(bx*cf-bf*cx) + af*(bz*cx-bx*cz) + ax*(bf*cz-bz*cf),
      af*(bx*cy-by*cx) + ax*(by*cf-bf*cy) + ay*(bf*cx-bx*cf),
      ax*(bz*cy-by*cz) + ay*(bx*cz-bz*cx) + az*(by*cx-bx*cy));
  vtkUnstructuredGridQuadricDecimationSymMat4 A(n.MultTransposeSym());
  // weight by the volume of the tet
  //  Q.Scale(Orientation()/6.0);
  // we want to divide by 4 also, for each vertex
  A *= 1.5 / fabs(Orientation());

  for (int i=0; i<4; i++)
  {
    Verts[i]->Q.A += A;
  }
}
#undef ax
#undef ay
#undef az
#undef af
#undef bx
#undef by
#undef bz
#undef bf
#undef cx
#undef cy
#undef cz
#undef cf

class vtkUnstructuredGridQuadricDecimationTetMesh
{
public:
  vtkUnstructuredGridQuadricDecimationTetMesh():
    setSize(8), doublingRatio(0.4), noDoubling(false),
    boundaryWeight(100.0),
    Verts(NULL), tets(NULL), PT(NULL),
    unusedTets(0), unusedVerts(0), L(NULL) {}

  ~vtkUnstructuredGridQuadricDecimationTetMesh()
  {
    clear();
  }

  void AddTet(vtkUnstructuredGridQuadricDecimationTetra *t);

  void clear();   // clear the mesh -> empty

  int LoadUnstructuredGrid(vtkUnstructuredGrid *vgrid,
                           const char *scalarsName);
  int SaveUnstructuredGrid(vtkUnstructuredGrid *vgrid);

  // Simplification
  int setSize;
  float doublingRatio;
  bool noDoubling;
  float boundaryWeight;
  void BuildFullMesh();
  int Simplify(int n, int desiredTets);

  int vCount;
  int tCount;
  vtkUnstructuredGridQuadricDecimationVertex *Verts;
  vtkUnstructuredGridQuadricDecimationTetra *tets;
  vtkUnstructuredGridQuadricDecimationTetra **PT;
  vtkUnstructuredGridQuadricDecimationFaceMap faces;

  // number of tets deleted but not free
  int unusedTets;
  int unusedVerts;
  int maxTet;

  int *L;

private:
  void AddCorner(vtkUnstructuredGridQuadricDecimationVertex *v, int corner);

  // check if this edge can be collapsed (i.e. without violating boundary,
  // vol...)
  bool Contractable(vtkUnstructuredGridQuadricDecimationEdge &e,
                    const vtkUnstructuredGridQuadricDecimationVec4 &target);

  // Simplification
  void MergeTets(vtkUnstructuredGridQuadricDecimationVertex *dst,
                 vtkUnstructuredGridQuadricDecimationVertex *src);
  void DeleteMin(vtkUnstructuredGridQuadricDecimationEdge &e,
                 vtkUnstructuredGridQuadricDecimationQEF &Q);
};

#define VTK_ADDFACEBORDER(i,j,k)                                        \
  faces.AddFaceBorder(vtkUnstructuredGridQuadricDecimationFace(         \
                        t->Verts[i], t->Verts[j], t->Verts[k]))
void vtkUnstructuredGridQuadricDecimationTetMesh::AddTet(
  vtkUnstructuredGridQuadricDecimationTetra *t)
{
  if (t->Orientation()<-VTK_FEPS)
  {
    t->FixOrientation();
  }

  // add all of its faces to the FaceMap => 4 faces
  // NOTE: adding faces to vertices' list will be done
  // after we have all the faces (because some faces
  // might be deleted if it is not on the surface!!!)
  VTK_ADDFACEBORDER(0,1,2);
  VTK_ADDFACEBORDER(0,1,3);
  VTK_ADDFACEBORDER(0,2,3);
  VTK_ADDFACEBORDER(1,2,3);
}
#undef VTK_ADDFACEBORDER

void vtkUnstructuredGridQuadricDecimationTetMesh::AddCorner(
  vtkUnstructuredGridQuadricDecimationVertex *v, int corner)
{
  if (v->Corner<0)
  {
    v->Corner = corner;
    L[corner] = corner;
  }
  else
  {
    L[corner] = L[v->Corner];
    L[v->Corner] = corner;
  }
}

// Clean the mesh
void vtkUnstructuredGridQuadricDecimationTetMesh::clear()
{
  VTK_PRECHECK(Verts) = NULL;
  VTK_PRECHECK(tets) = NULL;
  VTK_PRECHECK(PT) = NULL;
  VTK_PRECHECK(L) = NULL;
  faces.clear();
  unusedTets = 0;
  unusedVerts = 0;
}

// SIMPLIFICATION IMPLEMENTATION

// BuildFullMesh
//  - Adding faces to vertices list and initialize their quadrics
//  - Compute quadric error at each vertex or remove unused vertices
void vtkUnstructuredGridQuadricDecimationTetMesh::BuildFullMesh()
{
  vtkUnstructuredGridQuadricDecimationFaceHashMap::iterator fi =
    faces.begin();
  while (fi!=faces.end())
  {
    vtkUnstructuredGridQuadricDecimationFace *f = (*fi).second;
    f->UpdateQuadric(boundaryWeight);
    ++fi;
  }
}

void vtkUnstructuredGridQuadricDecimationTetMesh::DeleteMin(
  vtkUnstructuredGridQuadricDecimationEdge &finalE,
  vtkUnstructuredGridQuadricDecimationQEF &minQ)
{
  // Multiple Choice Randomize set
  static float lasterror = 0;
  bool stored(false);
  vtkUnstructuredGridQuadricDecimationQEF Q;
  vtkUnstructuredGridQuadricDecimationEdge e(NULL, NULL);
  for (int j=0; j<2; j++)
  {
    for (int i=0; i<setSize; i++)
    {
      int k = rand() % maxTet;
      if (tets[k].index<0)
      {
        do
        {
          maxTet--;
        }
        while (maxTet>0 && tets[maxTet].index<0);
        if (k<maxTet)
        {
          int idx = tets[k].index;
          tets[k] = tets[maxTet];
          tets[maxTet].index = idx;
          PT[tets[k].index] = &tets[k];
        }
        else
        {
          k = maxTet++;
        }
      }

      e.Verts[0] = tets[k].Verts[rand() % 4];
      do
      {
        e.Verts[1] = tets[k].Verts[rand() % 4];
      }
      while (e.Verts[1]==e.Verts[0]);

      if (!stored)
      {
        finalE = e;
        minQ.Sum(e.Verts[0]->Q, e.Verts[1]->Q);
        stored = true;
      }
      else
      {
        if (e.Verts[0]->Q.e + e.Verts[1]->Q.e < minQ.e)
        {
          Q.Sum(e.Verts[0]->Q, e.Verts[1]->Q);
          if (Q.e<minQ.e)
          {
            finalE = e;
            minQ = Q;
          }
        }
      }
    }
    if (noDoubling || (minQ.e-lasterror)/lasterror<=doublingRatio)
    {
      break;
    }
  }
  lasterror = minQ.e;
}

// Simplify the mesh by a series of N edge contractions
// or to the number of desiredTets
// it returns the actual number of edge contractions
int vtkUnstructuredGridQuadricDecimationTetMesh::Simplify(int n,
                                                          int desiredTets)
{
  int count = 0;
  int uncontractable = 0;
  int run = 0;
  while ((count<n || desiredTets<(tCount-unusedTets)) && (run<1000))
  {
    // as long as we want to collapse
    vtkUnstructuredGridQuadricDecimationQEF Q;
    vtkUnstructuredGridQuadricDecimationEdge e;

    DeleteMin(e, Q);

    if (Contractable(e, Q.p))
    {
      run = 0;
      // begin to collapse the edge Va + Vb -> Va = e.target
      vtkUnstructuredGridQuadricDecimationVertex *va = e.Verts[0];
      vtkUnstructuredGridQuadricDecimationVertex *vb = e.Verts[1];

      // Constructing new vertex
      va->Q = Q;

      // Merge all faces and tets of Va and Vb, remove the degenerated ones
      MergeTets(va, vb);
      vb->Corner = -1;
      unusedVerts++;

      // Complete the edge contraction
      count++;
    }
    else
    {
      uncontractable++;
      run++;
    }
  }
  return count;
}

// Merge all tets of Vb to Va by changing Vb to Va
// and add all tets of Vb to Va's Tet List
// Also, it will remove all tets contain both Vb and Va
// In fact, this is merging corners
void vtkUnstructuredGridQuadricDecimationTetMesh::MergeTets(
  vtkUnstructuredGridQuadricDecimationVertex *dst,
  vtkUnstructuredGridQuadricDecimationVertex *src)
{
  int next = src->Corner;
  vtkUnstructuredGridQuadricDecimationTetra *t = NULL;
  do
  {
    t = PT[next/4];
    if (t)
    {
      if (t->ContainVertex(dst))
      {
        t->index = -t->index-1;
        unusedTets++;
        PT[next/4] = NULL;
      }
      else
      {
        t->ChangeVertex(src, dst);
      }
    }
    next = L[next];
  }
  while (next!=src->Corner);

  // Then we merge them all together
  next = L[dst->Corner];
  L[dst->Corner] = L[src->Corner];
  L[src->Corner] = next;
  bool notstop = true;
  int prev = dst->Corner;
  next = L[prev];
  do
  {
    notstop = next!=dst->Corner;
    t = PT[next/4];
    if (!t)
    {
      next = L[next];
      L[prev] = next;
    }
    else
    {
      prev = next;
      next = L[next];
    }
  }
  while (notstop);
  dst->Corner = prev;
}

// Check if an edge can be contracted
bool vtkUnstructuredGridQuadricDecimationTetMesh::Contractable(
  vtkUnstructuredGridQuadricDecimationEdge &e,
  const vtkUnstructuredGridQuadricDecimationVec4 &target)
{
  // need to check all the tets around both vertices to see if they can
  // adapt the new target vertex or not
  vtkUnstructuredGridQuadricDecimationTetra *t = NULL;
  for (int i=0; i<2; i++)
  {
    int c = e.Verts[i]->Corner;
    do
    {
      t = PT[c/4];
      if (t)
      {
        if (!(t->ContainVertex(e.Verts[0]) &&
              t->ContainVertex(e.Verts[1])) &&
            !(t->Changeable(e.Verts[i], target)))
        {
          return false;
        }
      }
      c = L[c];
    }
    while (c!=e.Verts[i]->Corner);
  }
  return true;
}

int vtkUnstructuredGridQuadricDecimationTetMesh::LoadUnstructuredGrid(
  vtkUnstructuredGrid *vgrid, const char *scalarsName)
{
  clear();
  // Read all the vertices first
  vCount = vgrid->GetNumberOfPoints();
  VTK_PRECHECK(Verts) = new vtkUnstructuredGridQuadricDecimationVertex[vCount];
  vtkPoints *vp = vgrid->GetPoints();
  vtkDataArray *vs = NULL;
  if (scalarsName)
  {
    vs = vgrid->GetPointData()->GetArray(scalarsName);
  }
  else
  {
    vs = vgrid->GetPointData()->GetScalars();
    if (!vs)
    {
      vs = vgrid->GetPointData()->GetArray("scalars");
    }
  }
  if (!vs)
  {
    return vtkUnstructuredGridQuadricDecimation::NO_SCALARS;
  }
  for (int i=0; i<vCount; i++)
  {
    double *pos = vp->GetPoint(i);
    double *scalar = vs->GetTuple(i);
    Verts[i].Q.p[0] = pos[0];
    Verts[i].Q.p[1] = pos[1];
    Verts[i].Q.p[2] = pos[2];
    Verts[i].Q.p[3] = scalar[0];
  }

  // Read all the tets
  tCount = vgrid->GetNumberOfCells();
  if (!tCount)
  {
    return vtkUnstructuredGridQuadricDecimation::NO_CELLS;
  }
  maxTet = tCount;
  VTK_PRECHECK(tets) = new vtkUnstructuredGridQuadricDecimationTetra[tCount];
  VTK_PRECHECK(PT) = new vtkUnstructuredGridQuadricDecimationTetra*[tCount];
  VTK_PRECHECK(L) = new int[4 * tCount];
  vtkCellArray *vt = vgrid->GetCells();
  vtkIdType npts;
  vtkIdType *idx;
  vtkIdType curIdx = 0;
  for (int i=0; i<tCount; i++)
  {
    vt->GetCell(curIdx, npts, idx);
    curIdx += (npts+1);
    if (npts==4)
    {
      for (int k=0; k<4; k++)
      {
        tets[i].Verts[k] = &Verts[idx[k]];
      }
      AddTet(&tets[i]);
      AddCorner(tets[i].Verts[0], i*4 + 0);
      AddCorner(tets[i].Verts[1], i*4 + 1);
      AddCorner(tets[i].Verts[2], i*4 + 2);
      AddCorner(tets[i].Verts[3], i*4 + 3);
      tets[i].UpdateQuadric();
      PT[i] = &tets[i];
      tets[i].index = i;
    }
    else
    {
      return vtkUnstructuredGridQuadricDecimation::NON_TETRAHEDRA;
    }
  }

  return vtkUnstructuredGridQuadricDecimation::NO_ERROR;
}

int vtkUnstructuredGridQuadricDecimationTetMesh::SaveUnstructuredGrid(
  vtkUnstructuredGrid *vgrid)
{
  vtkIdType growSize = (tCount-unusedTets)*4;
  vgrid->Allocate(growSize, growSize);
  vtkPoints *vp = vtkPoints::New();
  vtkDoubleArray *vs = vtkDoubleArray::New();

  // Output vertices
  // We need a map for indexing
  std::map<vtkUnstructuredGridQuadricDecimationVertex *, int> indexes;

  int nPoints = 0;
  for (int i=0; i<vCount; i++)
  {
    if (Verts[i].Corner>=0)
    {
      ++nPoints;
    }
  }

  vp->SetNumberOfPoints(nPoints);
  vs->SetNumberOfValues(nPoints);
  int vIdx = 0;
  for (int i=0; i<vCount; i++)
  {
    if (Verts[i].Corner>=0)
    {
      vp->SetPoint(vIdx, Verts[i].Q.p[0], Verts[i].Q.p[1], Verts[i].Q.p[2]);
      vs->SetValue(vIdx, Verts[i].Q.p[3]);
      indexes[&Verts[i]] = vIdx++;
    }
  }
  vgrid->SetPoints(vp);
  vp->Delete();
  vs->SetName("scalars");
  vgrid->GetPointData()->AddArray(vs);
  vgrid->GetPointData()->SetScalars(vs);
  vs->Delete();

  vtkIdType idx[4];
  for (int i=0; i<maxTet; i++)
  {
    if (tets[i].index>=0)
    {
      for (int j=0; j<4; j++)
      {
        idx[j] = indexes[tets[i].Verts[j]];
      }
      vgrid->InsertNextCell(VTK_TETRA, 4, idx);
    }
  }
  return vtkUnstructuredGridQuadricDecimation::NO_ERROR;
}

#undef VTK_PRECHECK
#undef VTK_FEPS
#undef VTK_TEPS
#undef VTK_SWAP


////////////////////////////////////////////////////////////////////////////////
/* ========================================================================== */
////////////////////////////////////////////////////////////////////////////////
/* === vtkUnstructuredGridQuadricDecimation                               === */
////////////////////////////////////////////////////////////////////////////////
/* ========================================================================== */
////////////////////////////////////////////////////////////////////////////////

vtkStandardNewMacro(vtkUnstructuredGridQuadricDecimation);

void vtkUnstructuredGridQuadricDecimation::PrintSelf(ostream& os,
                                                     vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Number of Tets to Output: " << this->NumberOfTetsOutput
     << "\n";
  os << indent << "Number of Edges to Decimate: "
     << this->NumberOfEdgesToDecimate << "\n";
  os << indent << "Number of Candidates Per Set: "
     << this->NumberOfCandidates << "\n";
  os << indent << "AutoAddCandidates: " << this->AutoAddCandidates << "\n";
  os << indent << "AutoAddCandidatesThreshold: "
     << this->AutoAddCandidatesThreshold << "\n";
  os << indent << "Boundary Weight: " << this->BoundaryWeight << "\n";
}

vtkUnstructuredGridQuadricDecimation::vtkUnstructuredGridQuadricDecimation()
{
  this->TargetReduction = 1.0;
  this->NumberOfTetsOutput = 0;
  this->NumberOfEdgesToDecimate = 0;
  this->NumberOfCandidates = 8;
  this->AutoAddCandidates = 1;
  this->AutoAddCandidatesThreshold = 0.4;
  this->BoundaryWeight = 100.0;
  this->ScalarsName = NULL;
}

vtkUnstructuredGridQuadricDecimation::~vtkUnstructuredGridQuadricDecimation()
{
  if (this->ScalarsName)
  {
    delete [] this->ScalarsName;
  }
}

void vtkUnstructuredGridQuadricDecimation::ReportError(int err)
{
  switch (err)
  {
    case vtkUnstructuredGridQuadricDecimation::NON_TETRAHEDRA:
      vtkErrorMacro(<< "Non-tetrahedral cells not supported!");
      break;
    case vtkUnstructuredGridQuadricDecimation::NO_SCALARS:
      vtkErrorMacro(<< "Can't simplify without scalars!");
      break;
    case vtkUnstructuredGridQuadricDecimation::NO_CELLS:
      vtkErrorMacro(<< "No Cells!");
      break;
    default:
      break;
  }
}

int vtkUnstructuredGridQuadricDecimation::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast(
                                 inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
                                  outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGridQuadricDecimationTetMesh myMesh;
  myMesh.doublingRatio = this->AutoAddCandidatesThreshold;
  myMesh.noDoubling = !this->AutoAddCandidates;
  myMesh.boundaryWeight = this->BoundaryWeight;
  int err = myMesh.LoadUnstructuredGrid((vtkUnstructuredGrid*)(input),
                                        this->ScalarsName);
  if (err!=vtkUnstructuredGridQuadricDecimation::NO_ERROR)
  {
    this->ReportError(err);
    return 0;
  }

  myMesh.BuildFullMesh();

  int desiredTets = this->NumberOfTetsOutput;
  if (desiredTets==0)
  {
    desiredTets = (int)((1 - this->TargetReduction) * myMesh.tCount);
  }
  myMesh.Simplify(this->NumberOfEdgesToDecimate, desiredTets);
  myMesh.SaveUnstructuredGrid(output);
  return 1;
}
