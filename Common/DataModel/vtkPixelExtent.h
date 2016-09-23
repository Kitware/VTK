/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelExtenth.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPixelExtent
 *
 * Representation of a cartesian pixel plane and common operations
 * on it. The implementation is intended to be fast and light
 * so that it may be used in place of int[4] with little or no
 * performance penalty.
 *
 * NOTE in most cases operation on an empty object produces
 * incorrect results. If it an issue query Empty() first.
*/

#ifndef vtkPixelExtent_h
#define vtkPixelExtent_h

#include "vtkSystemIncludes.h" // for VTK's system header config
#include "vtkCommonDataModelModule.h" // for export

#include <deque> // for inline impl
#include <algorithm> // for inline impl
#include <iostream> // for inline impl
#include <climits> // for inline impl

class VTKCOMMONDATAMODEL_EXPORT vtkPixelExtent
{
public:
  vtkPixelExtent();

  template<typename T>
  vtkPixelExtent(const T *ext);

  template<typename T>
  vtkPixelExtent(T ilo, T ihi, T jlo, T jhi);

  template<typename T>
  vtkPixelExtent(T width, T height)
    { this->SetData(T(0), width-T(1), T(0), height-T(1)); }

  vtkPixelExtent(const vtkPixelExtent &other);

  vtkPixelExtent &operator=(const vtkPixelExtent &other);

  /**
   * Element access
   */
  int &operator[](int i){ return this->Data[i]; }
  const int &operator[](int i) const { return this->Data[i]; }

  /**
   * Set the extent.
   */
  void SetData(const vtkPixelExtent &ext);

  template<typename T>
  void SetData(const T *ext);

  template<typename T>
  void SetData(T ilo, T ihi, T jlo, T jhi);
  void Clear();

  /**
   * Direct access to internal data.
   */
  int *GetData(){ return this->Data; }
  const int *GetData() const { return this->Data; }

  template<typename T>
  void GetData(T data[4]) const;

  unsigned int *GetDataU()
    { return reinterpret_cast<unsigned int*>(this->Data); }

  const unsigned int *GetDataU() const
    { return reinterpret_cast<const unsigned int*>(this->Data); }

  //@{
  /**
   * Get the start/end index.
   */
  void GetStartIndex(int first[2]) const;
  void GetStartIndex(int first[2], const int origin[2]) const;
  void GetEndIndex(int last[2]) const;
  //@}

  /**
   * Return true if empty.
   */
  int Empty() const;

  /**
   * Test for equivalence.
   */
  bool operator==(const vtkPixelExtent &other) const;

  //@{
  /**
   * Return non-zero if this extent conatins the other.
   */
  int Contains(const vtkPixelExtent &other) const;
  int Contains(int i, int j) const;
  //@}

  /**
   * Return non-zero if the extent is disjoint from the other
   */
  int Disjoint(vtkPixelExtent other) const;

  /**
   * Get the number in each direction.
   */
  template<typename T>
  void Size(T nCells[2]) const;

  /**
   * Get the total number.
   */
  size_t Size() const;


  /**
   * In place intersection.
   */
  void operator&=(const vtkPixelExtent &other);

  /**
   * In place union
   */
  void operator|=(const vtkPixelExtent &other);



  //@{
  /**
   * Expand the extents by n.
   */
  void Grow(int n);
  void Grow(int q, int n);
  void GrowLow(int q, int n);
  void GrowHigh(int q, int n);
  //@}

  //@{
  /**
   * Shrink the extent by n.
   */
  void Shrink(int n);
  void Shrink(int q, int n);
  //@}

  /**
   * Shifts by low corner of this, moving to the origin.
   */
  void Shift();

  /**
   * Shift by low corner of the given extent.
   */
  void Shift(const vtkPixelExtent &ext);

  /**
   * Shift by the given amount.
   */
  void Shift(int *n);

  /**
   * Shift by the given amount in the given direction.
   */
  void Shift(int q, int n);

  /**
   * Divide the extent in half in the given direction. The
   * operation is done in-place the other half of the split
   * extent is returned. The retunr will be empty if the split
   * could not be made.
   */
  vtkPixelExtent Split(int dir);



  //@{
  /**
   * In-place conversion from cell based to node based extent, and vise-versa.
   */
  void CellToNode();
  void NodeToCell();
  //@}



  /**
   * Get the number in each direction.
   */
  template<typename T>
  static
  void Size(const vtkPixelExtent &ext, T nCells[2]);

  /**
   * Get the total number.
   */
  static
  size_t Size(const vtkPixelExtent &ext);

  /**
   * Add or remove ghost cells. If a problem domain is
   * provided then the result is clipled to be within the
   * problem domain.
   */
  static vtkPixelExtent Grow(const vtkPixelExtent &inputExt, int n);

  static vtkPixelExtent Grow(
      const vtkPixelExtent &inputExt,
      const vtkPixelExtent &problemDomain,
      int n);

  static vtkPixelExtent GrowLow(
      const vtkPixelExtent &ext,
      int q,
      int n);

  static vtkPixelExtent GrowHigh(
      const vtkPixelExtent &ext,
      int q,
      int n);

  /**
   * Remove ghost cells. If a problem domain is
   * provided the input is pinned at the domain.
   */
  static vtkPixelExtent Shrink(
      const vtkPixelExtent &inputExt,
      const vtkPixelExtent &problemDomain,
      int n);

  static vtkPixelExtent Shrink(
      const vtkPixelExtent &inputExt,
      int n);

  /**
   * Convert from point extent to cell extent
   * while respecting the dimensionality of the data.
   */
  static vtkPixelExtent NodeToCell(const vtkPixelExtent &inputExt);

  /**
   * Convert from cell extent to point extent
   * while respecting the dimensionality of the data.
   */
  static vtkPixelExtent CellToNode(const vtkPixelExtent &inputExt);

  //@{
  /**
   * Shift by the given amount while respecting mode.
   */
  static void Shift(int *ij, int n);
  static void Shift(int *ij, int *n);
  //@}


  /**
   * Split ext at i,j, resulting extents (up to 4) are appended
   * to newExts. If i,j is outside ext, ext is passed through
   * unmodified.
   */
  static void Split(
        int i,
        int j,
        const vtkPixelExtent &ext,
        std::deque<vtkPixelExtent> &newExts);

  /**
   * A - B = C
   * C is a set of disjoint extents such that the
   * intersection of B and C is empty and the intersection
   * of A and C is C.
   */
  static void Subtract(
        const vtkPixelExtent &A,
        vtkPixelExtent B,
        std::deque<vtkPixelExtent> &newExts);

  /**
   * Merge compatible extents in the list. Extents are compatible
   * if they are directly adjacent nad have the same extent along
   * the adjacent edge.
   */
  static void Merge(std::deque<vtkPixelExtent> &exts);

private:
  int Data[4];
};

/**
 * Stream insertion operator for formatted output of pixel extents.
 */
VTKCOMMONDATAMODEL_EXPORT
std::ostream &operator<<(std::ostream &os, const vtkPixelExtent &ext);

//-----------------------------------------------------------------------------
template<typename T>
void vtkPixelExtent::SetData(const T *ext)
{
  Data[0] = static_cast<int>(ext[0]);
  Data[1] = static_cast<int>(ext[1]);
  Data[2] = static_cast<int>(ext[2]);
  Data[3] = static_cast<int>(ext[3]);
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkPixelExtent::SetData(T ilo, T ihi, T jlo, T jhi)
{
  T ext[4] = {ilo, ihi, jlo, jhi};
  this->SetData(ext);
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::SetData(const vtkPixelExtent &other)
{
  this->SetData(other.GetData());
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkPixelExtent::GetData(T data[4]) const
{
  data[0] = static_cast<T>(this->Data[0]);
  data[1] = static_cast<T>(this->Data[1]);
  data[2] = static_cast<T>(this->Data[2]);
  data[3] = static_cast<T>(this->Data[3]);
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Clear()
{
  this->SetData<int>(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
}

//-----------------------------------------------------------------------------
inline
vtkPixelExtent::vtkPixelExtent()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
template<typename T>
vtkPixelExtent::vtkPixelExtent(const T *ext)
{
  this->SetData(ext);
}

//-----------------------------------------------------------------------------
template<typename T>
vtkPixelExtent::vtkPixelExtent(
      T ilo,
      T ihi,
      T jlo,
      T jhi)
{
  this->SetData(ilo, ihi, jlo, jhi);
}

//-----------------------------------------------------------------------------
inline
vtkPixelExtent &vtkPixelExtent::operator=(const vtkPixelExtent &other)
{
  if (&other == this)
  {
    return *this;
  }
  this->SetData(other);
  return *this;
}

//-----------------------------------------------------------------------------
inline
vtkPixelExtent::vtkPixelExtent(const vtkPixelExtent &other)
{
  *this = other;
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkPixelExtent::Size(const vtkPixelExtent &ext, T nCells[2])
{
  nCells[0] = ext[1] - ext[0] + 1;
  nCells[1] = ext[3] - ext[2] + 1;
}

//-----------------------------------------------------------------------------
inline
size_t vtkPixelExtent::Size(const vtkPixelExtent &ext)
{
  return (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
}

//-----------------------------------------------------------------------------
template<typename T>
void vtkPixelExtent::Size(T nCells[2]) const
{
  vtkPixelExtent::Size(*this, nCells);
}

//-----------------------------------------------------------------------------
inline
size_t vtkPixelExtent::Size() const
{
  return vtkPixelExtent::Size(*this);
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::GetStartIndex(int first[2]) const
{
  first[0] = this->Data[0];
  first[1] = this->Data[2];
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::GetStartIndex(int first[2], const int origin[2]) const
{
  first[0] = this->Data[0] - origin[0];
  first[1] = this->Data[2] - origin[1];
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::GetEndIndex(int last[2]) const
{
  last[0] = this->Data[1];
  last[1] = this->Data[3];
}

//-----------------------------------------------------------------------------
inline
int vtkPixelExtent::Empty() const
{
  if ( this->Data[0] > this->Data[1]
    || this->Data[2] > this->Data[3])
  {
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
inline
bool vtkPixelExtent::operator==(const vtkPixelExtent &other) const
{
  if ( (this->Data[0] == other.Data[0])
    && (this->Data[1] == other.Data[1])
    && (this->Data[2] == other.Data[2])
    && (this->Data[3] == other.Data[3]) )
  {
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int vtkPixelExtent::Contains(const vtkPixelExtent &other) const
{
  if ( (this->Data[0] <= other.Data[0])
    && (this->Data[1] >= other.Data[1])
    && (this->Data[2] <= other.Data[2])
    && (this->Data[3] >= other.Data[3]) )
  {
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int vtkPixelExtent::Contains(int i, int j) const
{
  if ( (this->Data[0] <= i)
    && (this->Data[1] >= i)
    && (this->Data[2] <= j)
    && (this->Data[3] >= j) )
  {
    return 1;
  }
  return 0;
}


//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::operator&=(const vtkPixelExtent &other)
{
  if (this->Empty())
  {
    return;
  }

  if (other.Empty())
  {
    this->Clear();
    return;
  }

  this->Data[0] = std::max(this->Data[0], other.Data[0]);
  this->Data[1] = std::min(this->Data[1], other.Data[1]);
  this->Data[2] = std::max(this->Data[2], other.Data[2]);
  this->Data[3] = std::min(this->Data[3], other.Data[3]);

  if (this->Empty())
  {
    this->Clear();
  }
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::operator|=(const vtkPixelExtent &other)
{
  if (other.Empty())
  {
    return;
  }

  if (this->Empty())
  {
    this->SetData(other.GetData());
    return;
  }

  this->Data[0] = std::min(this->Data[0], other.Data[0]);
  this->Data[1] = std::max(this->Data[1], other.Data[1]);
  this->Data[2] = std::min(this->Data[2], other.Data[2]);
  this->Data[3] = std::max(this->Data[3], other.Data[3]);
}

//-----------------------------------------------------------------------------
inline
int vtkPixelExtent::Disjoint(vtkPixelExtent other) const
{
  other &= *this;
  return other.Empty();
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Grow(int n)
{
  this->Data[0] -= n;
  this->Data[1] += n;
  this->Data[2] -= n;
  this->Data[3] += n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Grow(int q, int n)
{
  q *= 2;

  this->Data[q  ] -= n;
  this->Data[q+1] += n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::GrowLow(int q, int n)
{
  this->Data[2*q] -= n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::GrowHigh(int q, int n)
{
  this->Data[2*q+1] += n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Shrink(int n)
{
  this->Data[0] += n;
  this->Data[1] -= n;
  this->Data[2] += n;
  this->Data[3] -= n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Shrink(int q, int n)
{
  q *= 2;
  this->Data[q  ] += n;
  this->Data[q+1] -= n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Shift(int *n)
{
  this->Data[0] += n[0];
  this->Data[1] += n[0];
  this->Data[2] += n[1];
  this->Data[3] += n[1];
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Shift(int q, int n)
{
  q *= 2;
  this->Data[q  ] += n;
  this->Data[q+1] += n;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Shift(const vtkPixelExtent &other)
{
  for (int q=0; q<2; ++q)
  {
    int qq = q*2;
    int n = -other[qq];

    this->Data[qq  ] += n;
    this->Data[qq+1] += n;
  }
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::Shift()
{
  for (int q=0; q<2; ++q)
  {
    int qq = q*2;
    int n =- this->Data[qq];

    this->Data[qq  ] += n;
    this->Data[qq+1] += n;
  }
}

//-----------------------------------------------------------------------------
inline
vtkPixelExtent vtkPixelExtent::Split(int dir)
{
  vtkPixelExtent half;

  int q = 2 * dir;
  int l = this->Data[q+1] - this->Data[q] + 1;
  int s = l/2;

  if (s)
  {
    s += this->Data[q];
    half = *this;
    half.Data[q] = s;
    this->Data[q+1] = s - 1;
  }

  return half;
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::CellToNode()
{
  ++this->Data[1];
  ++this->Data[3];
}

//-----------------------------------------------------------------------------
inline
void vtkPixelExtent::NodeToCell()
{
  --this->Data[1];
  --this->Data[3];
}

//-----------------------------------------------------------------------------
inline
bool operator<(const vtkPixelExtent &l, const vtkPixelExtent &r)
{
  return l.Size() < r.Size();
}

#endif
// VTK-HeaderTest-Exclude: vtkPixelExtent.h
