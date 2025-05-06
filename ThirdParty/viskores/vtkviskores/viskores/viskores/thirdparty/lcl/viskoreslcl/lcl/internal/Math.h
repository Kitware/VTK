//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_internal_Math_h
#define lcl_internal_Math_h

#include <cmath>

#ifdef __CUDA_ARCH__
# define LCL_MATH_CALL(func, ...) func(__VA_ARGS__)
#else
# define LCL_MATH_CALL(func, ...) std::func(__VA_ARGS__)
#endif

namespace lcl
{

namespace internal
{
///=========================================================================
/// Basic vector class and functions for internal use

template <typename T, IdComponent Dim>
class Vector
{
public:
#if defined(_MSC_VER) && (_MSC_VER == 1900)
  // workaround vs2015 bug producing the following error:
  // error C2476: ‘constexpr’ constructor does not initialize all members
  LCL_EXEC Vector() noexcept {};
#else
  LCL_EXEC
  constexpr Vector() noexcept {};
#endif

  LCL_EXEC
  explicit Vector(const T& val) noexcept
  {
    for (auto& c : Data)
    {
      c = val;
    }
  }

  template <typename... Ts>
  LCL_EXEC
  constexpr explicit Vector(const T& c1, const Ts&... cs) noexcept
    : Data{c1, cs...}
  {
    static_assert(sizeof...(Ts) == (Dim - 1), "Invalid number of components passed");
  }

  static constexpr IdComponent getNumberOfComponents() noexcept
  {
    return Dim;
  }

  LCL_EXEC
  T& operator[](IdComponent c) noexcept
  {
    return this->Data[c];
  }

  LCL_EXEC
  constexpr const T& operator[](IdComponent c) const noexcept
  {
    return this->Data[c];
  }

  LCL_EXEC
  T* data() noexcept
  {
    return this->Data;
  }

  LCL_EXEC
  constexpr const T* data() const noexcept
  {
    return this->Data;
  }

private:
  T Data[Dim];
};

//---------------------------------------------------------------------------
template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim>& operator+=(Vector<T, Dim>& v1, const Vector<T, Dim>& v2) noexcept
{
  for (IdComponent i = 0; i < Dim; ++i)
  {
    v1[i] += v2[i];
  }
  return v1;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim>& operator-=(Vector<T, Dim>& v1, const Vector<T, Dim>& v2) noexcept
{
  for (IdComponent i = 0; i < Dim; ++i)
  {
    v1[i] -= v2[i];
  }
  return v1;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim>& operator*=(Vector<T, Dim>& v1, const Vector<T, Dim>& v2) noexcept
{
  for (IdComponent i = 0; i < Dim; ++i)
  {
    v1[i] *= v2[i];
  }
  return v1;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim>& operator*=(Vector<T, Dim>& v, const T& s) noexcept
{
  for (IdComponent i = 0; i < Dim; ++i)
  {
    v[i] *= s;
  }
  return v;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim>& operator/=(Vector<T, Dim>& v1, const Vector<T, Dim>& v2) noexcept
{
  for (IdComponent i = 0; i < Dim; ++i)
  {
    v1[i] /= v2[i];
  }
  return v1;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim>& operator/=(Vector<T, Dim>& v, const T& s) noexcept
{
  for (IdComponent i = 0; i < Dim; ++i)
  {
    v[i] /= s;
  }
  return v;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator+(Vector<T, Dim> v1, const Vector<T, Dim>& v2) noexcept
{
  return v1 += v2;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator-(Vector<T, Dim> v1, const Vector<T, Dim>& v2) noexcept
{
  return v1 -= v2;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator*(Vector<T, Dim> v1, const Vector<T, Dim>& v2) noexcept
{
  return v1 *= v2;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator*(Vector<T, Dim> v, const T& s) noexcept
{
  return v *= s;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator*(const T& s, Vector<T, Dim> v) noexcept
{
  return v *= s;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator/(Vector<T, Dim> v1, const Vector<T, Dim>& v2) noexcept
{
  return v1 /= v2;
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> operator/(Vector<T, Dim> v, const T& s) noexcept
{
  return v /= s;
}

//---------------------------------------------------------------------------
template <typename T, IdComponent Dim>
LCL_EXEC
T dot(const Vector<T, Dim>& v1, const Vector<T, Dim>& v2) noexcept
{
  T result{};
  for (IdComponent i = 0; i < Dim; ++i)
  {
    result += v1[i] * v2[i];
  }
  return result;
}

template <typename T>
LCL_EXEC
Vector<T, 3> cross(const Vector<T, 3>& v1, const Vector<T, 3>& v2) noexcept
{
  return Vector<T, 3>(v1[1] * v2[2] - v1[2] * v2[1],
                      v1[2] * v2[0] - v1[0] * v2[2],
                      v1[0] * v2[1] - v1[1] * v2[0]);
}

template <typename T, IdComponent Dim>
LCL_EXEC
T magnitude(const Vector<T, Dim>& v)
{
  return LCL_MATH_CALL(sqrt, (dot(v, v)));
}

template <typename T, IdComponent Dim>
LCL_EXEC
void normalize(Vector<T, Dim>& v)
{
  v /= magnitude(v);
}

template <typename T, IdComponent Dim>
LCL_EXEC
Vector<T, Dim> normal(Vector<T, Dim> v)
{
  normalize(v);
  return v;
}

///=========================================================================
/// Basic matrix class and functions for internal use

template <typename T, IdComponent NumberOfRows, IdComponent NumberOfColumns>
class Matrix
{
public:
#if defined(_MSC_VER) && (_MSC_VER == 1900)
  // workaround vs2015 bug producing the following error:
  // error C2476: ‘constexpr’ constructor does not initialize all members
  LCL_EXEC Matrix() noexcept {};
#else
  LCL_EXEC
  constexpr Matrix() noexcept {};
#endif

  LCL_EXEC
  T& operator()(IdComponent r, IdComponent c) noexcept
  {
    return this->Columns[c][r];
  }

  LCL_EXEC
  constexpr const T& operator()(IdComponent r, IdComponent c) const noexcept
  {
    return this->Columns[c][r];
  }

  LCL_EXEC
  static constexpr IdComponent getNumberOfRows() noexcept
  {
    return NumberOfRows;
  }

  LCL_EXEC
  static constexpr IdComponent getNumberOfColumns() noexcept
  {
    return NumberOfColumns;
  }

  LCL_EXEC
  constexpr const Vector<T, NumberOfRows>& getColumn(IdComponent c) const
  {
    return this->Columns[c];
  }

  LCL_EXEC
  Vector<T, NumberOfColumns> getRow(IdComponent r) const
  {
    Vector<T, NumberOfColumns> row;
    for (IdComponent i = 0; i < NumberOfColumns; ++i)
    {
      row[i] = this->Columns[i][r];
    }
    return row;
  }

  LCL_EXEC
  Matrix& operator+=(const Matrix& m) noexcept
  {
    for (IdComponent i = 0; i < NumberOfColumns; ++i)
    {
      this->Columns[i] += m.Columns[i];
    }
    return *this;
  }

  LCL_EXEC
  Matrix& operator-=(const Matrix& m) noexcept
  {
    for (IdComponent i = 0; i < NumberOfColumns; ++i)
    {
      this->Columns[i] -= m.Columns[i];
    }
    return *this;
  }

  LCL_EXEC
  Matrix& operator*=(const T& s) noexcept
  {
    for (auto& c : this->Columns)
    {
      c *= s;
    }
    return *this;
  }

  LCL_EXEC
  Matrix& operator/=(const T& s) noexcept
  {
    for (auto& c : this->Columns)
    {
      c /= s;
    }
    return *this;
  }

private:
  Vector<T, NumberOfRows> Columns[NumberOfColumns];
};

//---------------------------------------------------------------------------
template <typename T, IdComponent NRows, IdComponent NCols>
LCL_EXEC
Matrix<T, NRows, NCols> operator+(Matrix<T, NRows, NCols> m1,
                                  const Matrix<T, NRows, NCols>& m2) noexcept
{
  return m1 += m2;
}

template <typename T, IdComponent NRows, IdComponent NCols>
LCL_EXEC
Matrix<T, NRows, NCols> operator-(Matrix<T, NRows, NCols> m1,
                                  const Matrix<T, NRows, NCols>& m2) noexcept
{
  return m1 -= m2;
}

template <typename T, IdComponent NRows, IdComponent NCols>
LCL_EXEC
Matrix<T, NRows, NCols> operator*(Matrix<T, NRows, NCols> m, const T& s) noexcept
{
  return m *= s;
}

template <typename T, IdComponent NRows, IdComponent NCols>
LCL_EXEC
Matrix<T, NRows, NCols> operator/(Matrix<T, NRows, NCols> m, const T& s) noexcept
{
  return m /= s;
}

//---------------------------------------------------------------------------
template <typename T,
          IdComponent NumRow,
          IdComponent NumCol,
          IdComponent NumInternal>
LCL_EXEC
Matrix<T, NumRow, NumCol> matrixMultiply(
  const Matrix<T, NumRow, NumInternal>& leftFactor,
  const Matrix<T, NumInternal, NumCol>& rightFactor) noexcept
{
  Matrix<T, NumRow, NumCol> result;
  for (IdComponent rowIndex = 0; rowIndex < NumRow; ++rowIndex)
  {
    for (IdComponent colIndex = 0; colIndex < NumCol; ++colIndex)
    {
      T sum = T(leftFactor(rowIndex, 0) * rightFactor(0, colIndex));
      for (IdComponent internalIndex = 1; internalIndex < NumInternal; internalIndex++)
      {
        sum = T(sum + (leftFactor(rowIndex, internalIndex) * rightFactor(internalIndex, colIndex)));
      }
      result(rowIndex, colIndex) = sum;
    }
  }
  return result;
}

template <typename T, IdComponent NumRow, IdComponent NumCol>
LCL_EXEC
Vector<T, NumRow> matrixMultiply(
  const Matrix<T, NumRow, NumCol>& leftFactor,
  const Vector<T, NumCol>& rightFactor) noexcept
{
  Vector<T, NumRow> result;
  for (IdComponent rowIndex = 0; rowIndex < NumRow; ++rowIndex)
  {
    result[rowIndex] = dot(leftFactor.getRow(rowIndex), rightFactor);
  }
  return result;
}

template <typename T, IdComponent NumRow, IdComponent NumCol>
LCL_EXEC
Vector<T, NumCol> matrixMultiply(
  const Vector<T, NumRow>& leftFactor,
  const Matrix<T, NumRow, NumCol>& rightFactor) noexcept
{
  Vector<T, NumCol> result;
  for (IdComponent colIndex = 0; colIndex < NumCol; ++colIndex)
  {
    result[colIndex] = dot(leftFactor, rightFactor.getColumn(colIndex));
  }
  return result;
}

//---------------------------------------------------------------------------
// The following code is copied from Viskores

namespace detail
{

template <typename T, IdComponent Size>
LCL_EXEC
lcl::ErrorCode matrixLUPFactorFindPivot(Matrix<T, Size, Size>& m,
                                         Vector<IdComponent, Size>& permutation,
                                         IdComponent topCornerIndex,
                                         T& inversionParity) noexcept
{
  constexpr T epsilon = std::is_floating_point<T>::value ?
                        (std::is_same<T, float>::value ? T(1e-5f) : T(1e-9)) :
                        T{};

  IdComponent maxRowIndex = topCornerIndex;
  T maxValue = LCL_MATH_CALL(abs, (m(maxRowIndex, topCornerIndex)));
  for (IdComponent rowIndex = topCornerIndex + 1; rowIndex < Size; rowIndex++)
  {
    T compareValue = LCL_MATH_CALL(abs, (m(rowIndex, topCornerIndex)));
    if (maxValue < compareValue)
    {
      maxValue = compareValue;
      maxRowIndex = rowIndex;
    }
  }

  if (maxValue < epsilon)
  {
    return lcl::ErrorCode::MATRIX_LUP_FACTORIZATION_FAILED;
  }

  if (maxRowIndex != topCornerIndex)
  {
    // Swap rows in matrix.
    for (IdComponent i = 0; i < Size; ++i)
    {
      auto temp = m(topCornerIndex, i);
      m(topCornerIndex, i) = m(maxRowIndex, i);
      m(maxRowIndex, i) = temp;
    }

    // Record change in permutation matrix.
    IdComponent maxOriginalRowIndex = permutation[maxRowIndex];
    permutation[maxRowIndex] = permutation[topCornerIndex];
    permutation[topCornerIndex] = maxOriginalRowIndex;

    // Keep track of inversion parity.
    inversionParity = -inversionParity;
  }

  return lcl::ErrorCode::SUCCESS;
}

// Used with MatrixLUPFactor
template <typename T, IdComponent Size>
LCL_EXEC
void matrixLUPFactorFindUpperTriangleElements(
  Matrix<T, Size, Size>& m, IdComponent topCornerIndex) noexcept
{
  // Compute values for upper triangle on row topCornerIndex
  for (IdComponent colIndex = topCornerIndex + 1; colIndex < Size; colIndex++)
  {
    m(topCornerIndex, colIndex) /= m(topCornerIndex, topCornerIndex);
  }

  // Update the rest of the matrix for calculations on subsequent rows
  for (IdComponent rowIndex = topCornerIndex + 1; rowIndex < Size; rowIndex++)
  {
    for (IdComponent colIndex = topCornerIndex + 1; colIndex < Size; colIndex++)
    {
      m(rowIndex, colIndex) -= m(rowIndex, topCornerIndex) * m(topCornerIndex, colIndex);
    }
  }
}

/// Performs an LUP-factorization on the given matrix using Crout's method. The
/// LU-factorization takes a matrix A and decomposes it into a lower triangular
/// matrix L and upper triangular matrix U such that A = LU. The
/// LUP-factorization also allows permutation of A, which makes the
/// decomposition always possible so long as A is not singular. In addition to
/// matrices L and U, LUP also finds permutation matrix P containing all zeros
/// except one 1 per row and column such that PA = LU.
///
/// The result is done in place such that the lower triangular matrix, L, is
/// stored in the lower-left triangle of A including the diagonal. The upper
/// triangular matrix, U, is stored in the upper-right triangle of L not
/// including the diagonal. The diagonal of U in Crout's method is all 1's (and
/// therefore not explicitly stored).
///
/// The permutation matrix P is represented by the permutation vector. If
/// permutation[i] = j then row j in the original matrix A has been moved to
/// row i in the resulting matrices. The permutation matrix P can be
/// represented by a matrix with p_i,j = 1 if permutation[i] = j and 0
/// otherwise. If using LUP-factorization to compute a determinant, you also
/// need to know the parity (whether there is an odd or even amount) of
/// inversions. An inversion is an instance of a smaller number appearing after
/// a larger number in permutation. Although you can compute the inversion
/// parity after the fact, this function keeps track of it with much less
/// compute resources. The parameter inversionParity is set to 1.0 for even
/// parity and -1.0 for odd parity.
///
/// Not all matrices (specifically singular matrices) have an
/// LUP-factorization. If the LUP-factorization succeeds, valid is set to true.
/// Otherwise, valid is set to false and the result is indeterminant.
///
template <typename T, IdComponent Size>
LCL_EXEC
lcl::ErrorCode matrixLUPFactor(Matrix<T, Size, Size>& m,
                                Vector<IdComponent, Size>& permutation,
                                T& inversionParity) noexcept
{
  // Initialize permutation.
  for (IdComponent index = 0; index < Size; index++)
  {
    permutation[index] = index;
  }
  inversionParity = T(1);

  for (IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    LCL_RETURN_ON_ERROR(matrixLUPFactorFindPivot(m, permutation, rowIndex, inversionParity))
    matrixLUPFactorFindUpperTriangleElements(m, rowIndex);
  }

  return lcl::ErrorCode::SUCCESS;
}

/// Use a previous factorization done with MatrixLUPFactor to solve the
/// system Ax = b.  Instead of A, this method takes in the LU and P
/// matrices calculated by MatrixLUPFactor from A. The x matrix is returned.
///
template <typename T, IdComponent Size>
LCL_EXEC
Vector<T, Size> matrixLUPSolve(
  const Matrix<T, Size, Size>& LU,
  const Vector<IdComponent, Size>& permutation,
  const Vector<T, Size>& b) noexcept
{
  // The LUP-factorization gives us PA = LU or equivalently A = inv(P)LU.
  // Substituting into Ax = b gives us inv(P)LUx = b or LUx = Pb.
  // Now consider the intermediate vector y = Ux.
  // Substituting in the previous two equations yields Ly = Pb.
  // Solving Ly = Pb is easy because L is triangular and P is just a
  // permutation.
  Vector<T, Size> y;
  for (IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    y[rowIndex] = b[permutation[rowIndex]];
    // Recall that L is stored in the lower triangle of LU including diagonal.
    for (IdComponent colIndex = 0; colIndex < rowIndex; colIndex++)
    {
      y[rowIndex] -= LU(rowIndex, colIndex) * y[colIndex];
    }
    y[rowIndex] /= LU(rowIndex, rowIndex);
  }

  // Now that we have y, we can easily solve Ux = y for x.
  Vector<T, Size> x;
  for (IdComponent rowIndex = Size - 1; rowIndex >= 0; rowIndex--)
  {
    // Recall that U is stored in the upper triangle of LU with the diagonal
    // implicitly all 1's.
    x[rowIndex] = y[rowIndex];
    for (IdComponent colIndex = rowIndex + 1; colIndex < Size; colIndex++)
    {
      x[rowIndex] -= LU(rowIndex, colIndex) * x[colIndex];
    }
  }

  return x;
}

} // namespace detail

template <typename T, IdComponent Size>
LCL_EXEC
lcl::ErrorCode solveLinearSystem(const Matrix<T, Size, Size>& A,
                                  const Vector<T, Size>& b,
                                  Vector<T, Size>& x) noexcept
{
  // First, we will make an LUP-factorization to help us.
  Matrix<T, Size, Size> LU = A;
  Vector<IdComponent, Size> permutation;
  T inversionParity; // Unused.
  LCL_RETURN_ON_ERROR(detail::matrixLUPFactor(LU, permutation, inversionParity))

  // Next, use the decomposition to solve the system.
  x = detail::matrixLUPSolve(LU, permutation, b);
  return lcl::ErrorCode::SUCCESS;
}

/// Find and return the inverse of the given matrix. If the matrix is singular,
/// the inverse will not be correct and valid will be set to false.
///
template <typename T, IdComponent Size>
LCL_EXEC
lcl::ErrorCode matrixInverse(const Matrix<T, Size, Size>& A, Matrix<T, Size, Size>& invA) noexcept
{
  // First, we will make an LUP-factorization to help us.
  Matrix<T, Size, Size> LU = A;
  Vector<IdComponent, Size> permutation;
  T inversionParity; // Unused
  LCL_RETURN_ON_ERROR(detail::matrixLUPFactor(LU, permutation, inversionParity))

  // We will use the decomposition to solve AX = I for X where X is
  // clearly the inverse of A.  Our solve method only works for vectors,
  // so we solve for one column of invA at a time.
  Vector<T, Size> ICol(T(0));
  for (IdComponent colIndex = 0; colIndex < Size; colIndex++)
  {
    ICol[colIndex] = 1;
    Vector<T, Size> invACol = detail::matrixLUPSolve(LU, permutation, ICol);
    ICol[colIndex] = 0;
    for (IdComponent i = 0; i < Size; ++i)
    {
      invA(i, colIndex) = invACol[i];
    }
  }
  return lcl::ErrorCode::SUCCESS;
}

///=========================================================================
template <typename JacobianFunctor, typename FunctionFunctor, typename T, IdComponent Size>
LCL_EXEC
inline lcl::ErrorCode newtonsMethod(
  const JacobianFunctor& jacobianEvaluator,
  const FunctionFunctor& functionEvaluator,
  const Vector<T, Size>& rhs,
  Vector<T, Size>& result,
  T convergeDifference = 1e-3f,
  int maxIterations = 10) noexcept
{
  auto x = result;  // pass initial guess in result
  bool converged = false;

  for (int i = 0; !converged && i < maxIterations; ++i)
  {
    Matrix<T, Size, Size> jacobian;
    Vector<T, Size> fx(0);

    LCL_RETURN_ON_ERROR(jacobianEvaluator(x, jacobian))
    LCL_RETURN_ON_ERROR(functionEvaluator(x, fx))

    Vector<T, Size> deltax;
    LCL_RETURN_ON_ERROR(solveLinearSystem(jacobian, fx - rhs, deltax))
    x -= deltax;

    converged = true;
    for (int c = 0; c < Size; ++c)
    {
      converged &= (LCL_MATH_CALL(abs, (deltax[c])) < convergeDifference);
    }
  }

  result = x;
  return converged ? ErrorCode::SUCCESS : ErrorCode::SOLUTION_DID_NOT_CONVERGE;
}

///=========================================================================
template <typename T>
LCL_EXEC inline T lerp(T v0, T v1, T t)
{
  static_assert(std::is_floating_point<T>::value,
                "lerp requires floating point parameters");

  return LCL_MATH_CALL(fma, (t), (v1), (LCL_MATH_CALL(fma, (-t), (v0), (v0))));
}

}
} // lcl::internal

#endif // lcl_internal_Math_h
