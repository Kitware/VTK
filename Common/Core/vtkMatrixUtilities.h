/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathPrivate.hxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMathPrivate
 * @brief   Internal toolkit used in some vtkMath methods.
 *
 * vtkMatrixUtilities provides matrix indexing / wrapping tools. One can use
 * this utility to wrap a 1D array into a matrix shape, index it at compile
 * time.
 * @sa
 * vtkMath
 * vtkMathPrivate
 */

#ifndef vtkMatrixUtilities_hxx
#define vtkMatrixUtilities_hxx

#include <type_traits> // for type traits

namespace vtkMatrixUtilities
{
//=============================================================================
/**
 * This struct determines a prior transform to input matrices, chaging the
 * way they are indexed
 */
struct Layout
{
  /**
   * Input matrix is unchanged, i.e. sorted row-wise ordered
   */
  struct Identity;
  /*
   * Input matrix is transposed, i.e. sorted in column-wise ordered.
   */
  struct Transpose;

  /**
   * Input matrix is considered diagonal, and value at index idx points
   * to component of coordinates (idx, idx) in the diagonal matrix.
   */
  struct Diag;
};

namespace detail
{
// Extracting for STL-like containers
template <int ContainerTypeT, class ContainerT>
struct ScalarTypeExtractor
{
  typedef typename ContainerT::value_type value_type;
};

// Extracting for C++ arrays
template <class ContainerT>
struct ScalarTypeExtractor<1, ContainerT>
{
  typedef typename std::remove_all_extents<ContainerT>::type value_type;
};

// Extracting for C++ pointers
template <class ContainerT>
struct ScalarTypeExtractor<2, ContainerT>
{
  typedef typename std::remove_pointer<ContainerT>::type value_type;
};
} // namespace detail

//=============================================================================
/**
 * This class extract the underlying value type of containers. It works on
 * multi-dimensional C++ arrays as well as with STL container like that
 * have a value_type typedef.
 *
 * One can access the value type by fetching
 * ScalarTypeExtractor<ContainerT>::value_type.
 */
template <class ContainerT>
struct ScalarTypeExtractor
{
  typedef typename detail::ScalarTypeExtractor<
    // This parameter equals 0, 1 or 2
    std::is_array<ContainerT>::value + std::is_pointer<ContainerT>::value * 2,
    ContainerT>::value_type value_type;
};

namespace detail
{
// Class actually implementing matrix mapping.
template <int RowsT, int ColsT, class LayoutT>
struct Mapper;

// Specialization of the matrix mapper for when the layout is the identity
template <int RowsT, int ColsT>
struct Mapper<RowsT, ColsT, Layout::Identity>
{
  template <int RowT, int ColT>
  static constexpr int GetIndex()
  {
    static_assert(RowT >= 0 && RowT < RowsT, "RowT out of bounds");
    static_assert(ColT >= 0 && ColT < ColsT, "ColT out of bounds");
    return ColsT * RowT + ColT;
  }
};

template <int RowsT, int ColsT>
struct Mapper<RowsT, ColsT, Layout::Transpose>
{
  template <int RowT, int ColT>
  static constexpr int GetIndex()
  {
    static_assert(RowT >= 0 && RowT < RowsT, "RowT out of bounds");
    static_assert(ColT >= 0 && ColT < ColsT, "ColT out of bounds");
    return RowsT * ColT + RowT;
  }
};
} // namespace detail

//=============================================================================
/**
 * This class is a helper class to compute at compile time the index of a matrix
 * stored as a 1D array from its 2D coordinates. This class maps matrices of
 * dimension RowsT x ColsT. The LayoutT template parameter permits to switch to
 * the indexing of the transpose of the matrix. LayoutT can be set to
 * Layout::Identity for a row-wise ordering, or to Layout::Transpose for a
 * column-wise ordering
 *
 * @warning This mapper does not work with matrices stored as 2D arrays, or with
 * diagonal matrices.
 */
template <int RowsT, int ColsT, class LayoutT = Layout::Identity>
struct Mapper
{
  template <int RowT, int ColT>
  static constexpr int GetIndex()
  {
    return detail::Mapper<RowsT, ColsT, LayoutT>::template GetIndex<RowT, ColT>();
  }
};

namespace detail
{
// Class implementing matrix wrapping.
template <int RowsT, int ColsT, class MatrixT, class LayoutT, bool MatrixIs2DArrayT>
class Wrapper;

// Specializaion of matrix wrapping for matrices stored as 1D arrays
// in row-wise order
template <int RowsT, int ColsT, class MatrixT, class LayoutT>
class Wrapper<RowsT, ColsT, MatrixT, LayoutT, false>
{
private:
  using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type;

public:
  template <int RowT, int ColT>
  static const Scalar& Get(const MatrixT& M)
  {
    return M[Mapper<RowsT, ColsT, LayoutT>::template GetIndex<RowT, ColT>()];
  }

  template <int RowT, int ColT>
  static Scalar& Get(MatrixT& M)
  {
    return M[Mapper<RowsT, ColsT, LayoutT>::template GetIndex<RowT, ColT>()];
  }
};

// Specialization for matrices stored as 2D arrays with an unchanged layout
template <int RowsT, int ColsT, class MatrixT>
class Wrapper<RowsT, ColsT, MatrixT, Layout::Identity, true>
{
private:
  using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type;

public:
  template <int RowT, int ColT>
  static const Scalar& Get(const MatrixT& M)
  {
    return M[RowT][ColT];
  }

  template <int RowT, int ColT>
  static Scalar& Get(MatrixT& M)
  {
    return M[RowT][ColT];
  }
};

// Specialization for matrices stored as 2D arrays read as its transposed self.
template <int RowsT, int ColsT, class MatrixT>
class Wrapper<RowsT, ColsT, MatrixT, Layout::Transpose, true>
{
private:
  using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type;

public:
  template <int RowT, int ColT>
  static const Scalar& Get(const MatrixT& M)
  {
    return M[ColT][RowT];
  }

  template <int RowT, int ColT>
  static Scalar& Get(MatrixT& M)
  {
    return M[ColT][RowT];
  }
};

// Specialization for diagonal matrices.
// Note: a diagonal matrix has to be stored in a 1D array.
template <int RowsT, int ColsT, class MatrixT>
class Wrapper<RowsT, ColsT, MatrixT, Layout::Diag, false>
{
private:
  using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type;

  template <int RowT, int ColT>
  struct Helper
  {
    static constexpr Scalar ZERO = Scalar(0);

    static Scalar& Get(const MatrixT&) { return ZERO; }
  };

  template <int RowT>
  struct Helper<RowT, RowT>
  {
    static Scalar& Get(MatrixT& M) { return M[RowT]; }

    static const Scalar& Get(const MatrixT& M) { return M[RowT]; }
  };

public:
  template <int RowT, int ColT>
  const Scalar& Get(const MatrixT& M)
  {
    return Helper<RowT, ColT>::Get(M);
  }

  template <int RowT, int ColT>
  Scalar& Get(MatrixT& M)
  {
    return Helper<RowT, ColT>::Get(M);
  }
};
} // namespace detail

//=============================================================================
/**
 * Matrix wrapping class. This class implements a getter templated on the
 * coordinates of the wanted element. A matrix can be a 2D C++ array, a 1D C++
 * array row-wise ordered, or any STL-like container implementing operator[] and
 * having a value_type typedef.
 *
 * This class wraps a RowsT x ColsT matrix stored in the container MatrixT. The
 * LayoutT template parameter permits to reindex at compile-time the matrix. If
 * it is set to Layout::Identity, the matrix is assumed to be row-wised ordered.
 * If it is set to Layout::Transpose, the matrix is assumed to be column-wise ordered.
 * One can also convert a 1D input array into a diagonal matrix by setting
 * LayoutT to Layout::Diag. In ths particular case, method Get will return a
 * read-only zero on elements outside of the diagonal.
 */
template <int RowsT, int ColsT, class MatrixT, class LayoutT = Layout::Identity>
class Wrapper
{
private:
  using Scalar = typename ScalarTypeExtractor<MatrixT>::value_type;

  static constexpr bool MatrixIs2DArray =
    std::is_array<typename std::remove_extent<MatrixT>::type>::value;

  static_assert(!MatrixIs2DArray || !std::is_same<LayoutT, Layout::Diag>::value,
    "A diagonal  matrix cannot be a 2D array");

public:
  template <int RowT, int ColT>
  static const Scalar& Get(const MatrixT& M)
  {
    return detail::Wrapper<RowsT, ColsT, MatrixT, LayoutT, MatrixIs2DArray>::template Get<RowT,
      ColT>(M);
  }

  template <int RowT, int ColT>
  static Scalar& Get(MatrixT& M)
  {
    return detail::Wrapper<RowsT, ColsT, MatrixT, LayoutT, MatrixIs2DArray>::template Get<RowT,
      ColT>(M);
  }
};
} // namespace vtkMatrixUtilities
#endif
