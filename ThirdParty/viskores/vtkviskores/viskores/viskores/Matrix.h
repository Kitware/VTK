//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_Matrix_h
#define viskores_Matrix_h

#include <viskores/Assert.h>
#include <viskores/Math.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>

namespace viskores
{

/// @brief Basic Matrix type.
///
/// The Matrix class holds a small two dimensional array for simple linear
/// algebra and vector operations. Viskores provides several Matrix-based
/// operations to assist in visualization computations.
///
/// A Matrix is not intended to hold very large arrays. Rather, they are a
/// per-thread data structure to hold information like geometric transforms and
/// tensors.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
class Matrix
{
public:
  using ComponentType = T;
  static constexpr viskores::IdComponent NUM_ROWS = NumRow;
  static constexpr viskores::IdComponent NUM_COLUMNS = NumCol;

  /// Creates an uninitialized matrix. The values in the matrix are not determined.
  VISKORES_EXEC_CONT
  Matrix() {}

  /// Creates a matrix initialized with all values set to the provided `value`.
  VISKORES_EXEC_CONT
  explicit Matrix(const ComponentType& value)
    : Components(viskores::Vec<ComponentType, NUM_COLUMNS>(value))
  {
  }

  /// Brackets are used to reference a matrix like a 2D array (i.e.
  /// matrix[row][column]).
  ///
  VISKORES_EXEC_CONT
  const viskores::Vec<ComponentType, NUM_COLUMNS>& operator[](viskores::IdComponent rowIndex) const
  {
    VISKORES_ASSERT(rowIndex >= 0);
    VISKORES_ASSERT(rowIndex < NUM_ROWS);
    return this->Components[rowIndex];
  }

  /// Brackets are used to referens a matrix like a 2D array i.e.
  /// matrix[row][column].
  ///
  VISKORES_EXEC_CONT
  viskores::Vec<ComponentType, NUM_COLUMNS>& operator[](viskores::IdComponent rowIndex)
  {
    VISKORES_ASSERT(rowIndex >= 0);
    VISKORES_ASSERT(rowIndex < NUM_ROWS);
    return this->Components[rowIndex];
  }

  /// Parentheses are used to reference a matrix using mathematical tuple
  /// notation i.e. matrix(row,column).
  ///
  VISKORES_EXEC_CONT
  const ComponentType& operator()(viskores::IdComponent rowIndex,
                                  viskores::IdComponent colIndex) const
  {
    VISKORES_ASSERT(rowIndex >= 0);
    VISKORES_ASSERT(rowIndex < NUM_ROWS);
    VISKORES_ASSERT(colIndex >= 0);
    VISKORES_ASSERT(colIndex < NUM_COLUMNS);
    return (*this)[rowIndex][colIndex];
  }

  /// Parentheses are used to reference a matrix using mathematical tuple
  /// notation i.e. matrix(row,column).
  ///
  VISKORES_EXEC_CONT
  ComponentType& operator()(viskores::IdComponent rowIndex, viskores::IdComponent colIndex)
  {
    VISKORES_ASSERT(rowIndex >= 0);
    VISKORES_ASSERT(rowIndex < NUM_ROWS);
    VISKORES_ASSERT(colIndex >= 0);
    VISKORES_ASSERT(colIndex < NUM_COLUMNS);
    return (*this)[rowIndex][colIndex];
  }

private:
  viskores::Vec<viskores::Vec<ComponentType, NUM_COLUMNS>, NUM_ROWS> Components;
};

/// Returns a tuple containing the given row (indexed from 0) of the given
/// matrix.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT const viskores::Vec<T, NumCol>& MatrixGetRow(
  const viskores::Matrix<T, NumRow, NumCol>& matrix,
  viskores::IdComponent rowIndex)
{
  return matrix[rowIndex];
}

/// Returns a tuple containing the given column (indexed from 0) of the given
/// matrix.  Might not be as efficient as the `MatrixGetRow()` function.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT viskores::Vec<T, NumRow> MatrixGetColumn(
  const viskores::Matrix<T, NumRow, NumCol>& matrix,
  viskores::IdComponent columnIndex)
{
  viskores::Vec<T, NumRow> columnValues;
  for (viskores::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    columnValues[rowIndex] = matrix(rowIndex, columnIndex);
  }
  return columnValues;
}

/// Convenience function for setting a row of a matrix.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT void MatrixSetRow(viskores::Matrix<T, NumRow, NumCol>& matrix,
                                     viskores::IdComponent rowIndex,
                                     const viskores::Vec<T, NumCol>& rowValues)
{
  matrix[rowIndex] = rowValues;
}

/// Convenience function for setting a column of a matrix.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT void MatrixSetColumn(viskores::Matrix<T, NumRow, NumCol>& matrix,
                                        viskores::IdComponent columnIndex,
                                        const viskores::Vec<T, NumRow>& columnValues)
{
  for (viskores::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    matrix(rowIndex, columnIndex) = columnValues[rowIndex];
  }
}

/// Standard matrix multiplication.
///
template <typename T,
          viskores::IdComponent NumRow,
          viskores::IdComponent NumCol,
          viskores::IdComponent NumInternal>
VISKORES_EXEC_CONT viskores::Matrix<T, NumRow, NumCol> MatrixMultiply(
  const viskores::Matrix<T, NumRow, NumInternal>& leftFactor,
  const viskores::Matrix<T, NumInternal, NumCol>& rightFactor)
{
  viskores::Matrix<T, NumRow, NumCol> result;
  for (viskores::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    for (viskores::IdComponent colIndex = 0; colIndex < NumCol; colIndex++)
    {
      T sum = T(leftFactor(rowIndex, 0) * rightFactor(0, colIndex));
      for (viskores::IdComponent internalIndex = 1; internalIndex < NumInternal; internalIndex++)
      {
        sum = T(sum + (leftFactor(rowIndex, internalIndex) * rightFactor(internalIndex, colIndex)));
      }
      result(rowIndex, colIndex) = sum;
    }
  }
  return result;
}

/// Standard matrix-vector multiplication.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT viskores::Vec<T, NumRow> MatrixMultiply(
  const viskores::Matrix<T, NumRow, NumCol>& leftFactor,
  const viskores::Vec<T, NumCol>& rightFactor)
{
  viskores::Vec<T, NumRow> product;
  for (viskores::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    product[rowIndex] = viskores::Dot(viskores::MatrixGetRow(leftFactor, rowIndex), rightFactor);
  }
  return product;
}

/// Standard vector-matrix multiplication
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT viskores::Vec<T, NumCol> MatrixMultiply(
  const viskores::Vec<T, NumRow>& leftFactor,
  const viskores::Matrix<T, NumRow, NumCol>& rightFactor)
{
  viskores::Vec<T, NumCol> product;
  for (viskores::IdComponent colIndex = 0; colIndex < NumCol; colIndex++)
  {
    product[colIndex] = viskores::Dot(leftFactor, viskores::MatrixGetColumn(rightFactor, colIndex));
  }
  return product;
}

/// Returns the identity matrix.
///
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT viskores::Matrix<T, Size, Size> MatrixIdentity()
{
  viskores::Matrix<T, Size, Size> result(T(0));
  for (viskores::IdComponent index = 0; index < Size; index++)
  {
    result(index, index) = T(1.0);
  }
  return result;
}

/// Fills the given matrix with the identity matrix.
///
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT void MatrixIdentity(viskores::Matrix<T, Size, Size>& matrix)
{
  matrix = viskores::MatrixIdentity<T, Size>();
}

/// Returns the transpose of the given matrix.
///
template <typename T, viskores::IdComponent NumRows, viskores::IdComponent NumCols>
VISKORES_EXEC_CONT viskores::Matrix<T, NumCols, NumRows> MatrixTranspose(
  const viskores::Matrix<T, NumRows, NumCols>& matrix)
{
  viskores::Matrix<T, NumCols, NumRows> result;
  for (viskores::IdComponent index = 0; index < NumRows; index++)
  {
    viskores::MatrixSetColumn(result, index, viskores::MatrixGetRow(matrix, index));
#ifdef VISKORES_ICC
    // For reasons I do not really understand, the Intel compiler with with
    // optimization on is sometimes removing this for loop. It appears that the
    // optimizer sometimes does not recognize that the MatrixSetColumn function
    // has side effects. I cannot fathom any reason for this other than a bug in
    // the compiler, but unfortunately I do not know a reliable way to
    // demonstrate the problem.
    __asm__("");
#endif
  }
  return result;
}

namespace detail
{

// Used with MatrixLUPFactor.
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT void MatrixLUPFactorFindPivot(
  viskores::Matrix<T, Size, Size>& A,
  viskores::Vec<viskores::IdComponent, Size>& permutation,
  viskores::IdComponent topCornerIndex,
  T& inversionParity,
  bool& valid)
{
  viskores::IdComponent maxRowIndex = topCornerIndex;
  T maxValue = viskores::Abs(A(maxRowIndex, topCornerIndex));
  for (viskores::IdComponent rowIndex = topCornerIndex + 1; rowIndex < Size; rowIndex++)
  {
    T compareValue = viskores::Abs(A(rowIndex, topCornerIndex));
    if (maxValue < compareValue)
    {
      maxValue = compareValue;
      maxRowIndex = rowIndex;
    }
  }

  if (maxValue < viskores::Epsilon<T>())
  {
    valid = false;
    return;
  }

  if (maxRowIndex != topCornerIndex)
  {
    // Swap rows in matrix.
    viskores::Vec<T, Size> maxRow = viskores::MatrixGetRow(A, maxRowIndex);
    viskores::MatrixSetRow(A, maxRowIndex, viskores::MatrixGetRow(A, topCornerIndex));
    viskores::MatrixSetRow(A, topCornerIndex, maxRow);

    // Record change in permutation matrix.
    viskores::IdComponent maxOriginalRowIndex = permutation[maxRowIndex];
    permutation[maxRowIndex] = permutation[topCornerIndex];
    permutation[topCornerIndex] = maxOriginalRowIndex;

    // Keep track of inversion parity.
    inversionParity = -inversionParity;
  }
}

// Used with MatrixLUPFactor
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT void MatrixLUPFactorFindUpperTriangleElements(
  viskores::Matrix<T, Size, Size>& A,
  viskores::IdComponent topCornerIndex,
  bool& valid)
{
  // Compute values for upper triangle on row topCornerIndex
  if (A(topCornerIndex, topCornerIndex) == 0)
  {
    valid = false;
    return;
  }
  else
  {
    // Let's make the reciprocal approximation here.
    // Doesn't make things much fast for small 'Size',
    // but definitely improves performance as 'Size' gets large.
    T rAdiag = 1 / A(topCornerIndex, topCornerIndex);
    for (viskores::IdComponent colIndex = topCornerIndex + 1; colIndex < Size; colIndex++)
    {
      A(topCornerIndex, colIndex) *= rAdiag;
    }
  }

  // Update the rest of the matrix for calculations on subsequent rows
  for (viskores::IdComponent rowIndex = topCornerIndex + 1; rowIndex < Size; rowIndex++)
  {
    for (viskores::IdComponent colIndex = topCornerIndex + 1; colIndex < Size; colIndex++)
    {
      A(rowIndex, colIndex) -= A(rowIndex, topCornerIndex) * A(topCornerIndex, colIndex);
    }
  }
}

/// Performs an LUP-factorization on the given matrix using Crout's method. The
/// LU-factorization takes a matrix A  and decomposes it into a lower triangular
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
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT void MatrixLUPFactor(viskores::Matrix<T, Size, Size>& A,
                                        viskores::Vec<viskores::IdComponent, Size>& permutation,
                                        T& inversionParity,
                                        bool& valid)
{
  // Initialize permutation.
  for (viskores::IdComponent index = 0; index < Size; index++)
  {
    permutation[index] = index;
  }
  inversionParity = T(1);
  valid = true;

  for (viskores::IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    MatrixLUPFactorFindPivot(A, permutation, rowIndex, inversionParity, valid);
    if (!valid)
    {
      break;
    }
    MatrixLUPFactorFindUpperTriangleElements(A, rowIndex, valid);
    if (!valid)
    {
      break;
    }
  }
}

/// Use a previous factorization done with MatrixLUPFactor to solve the
/// system Ax = b.  Instead of A, this method takes in the LU and P
/// matrices calculated by MatrixLUPFactor from A. The x matrix is returned.
///
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT viskores::Vec<T, Size> MatrixLUPSolve(
  const viskores::Matrix<T, Size, Size>& LU,
  const viskores::Vec<viskores::IdComponent, Size>& permutation,
  const viskores::Vec<T, Size>& b)
{
  // The LUP-factorization gives us PA = LU or equivalently A = inv(P)LU.
  // Substituting into Ax = b gives us inv(P)LUx = b or LUx = Pb.
  // Now consider the intermediate vector y = Ux.
  // Substituting in the previous two equations yields Ly = Pb.
  // Solving Ly = Pb is easy because L is triangular and P is just a
  // permutation.
  viskores::Vec<T, Size> y;
  for (viskores::IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    y[rowIndex] = b[permutation[rowIndex]];
    // Recall that L is stored in the lower triangle of LU including diagonal.
    for (viskores::IdComponent colIndex = 0; colIndex < rowIndex; colIndex++)
    {
      y[rowIndex] -= LU(rowIndex, colIndex) * y[colIndex];
    }
    if (LU(rowIndex, rowIndex) == 0)
    {
      y[rowIndex] = std::numeric_limits<T>::quiet_NaN();
    }
    else
    {
      y[rowIndex] /= LU(rowIndex, rowIndex);
    }
  }

  // Now that we have y, we can easily solve Ux = y for x.
  viskores::Vec<T, Size> x;
  for (viskores::IdComponent rowIndex = Size - 1; rowIndex >= 0; rowIndex--)
  {
    // Recall that U is stored in the upper triangle of LU with the diagonal
    // implicitly all 1's.
    x[rowIndex] = y[rowIndex];
    for (viskores::IdComponent colIndex = rowIndex + 1; colIndex < Size; colIndex++)
    {
      x[rowIndex] -= LU(rowIndex, colIndex) * x[colIndex];
    }
  }

  return x;
}

} // namespace detail

/// Solve the linear system Ax = b for x. If a single solution is found, `valid`
/// is set to true, false otherwise.
///
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT viskores::Vec<T, Size> SolveLinearSystem(
  const viskores::Matrix<T, Size, Size>& A,
  const viskores::Vec<T, Size>& b,
  bool& valid)
{
  // First, we will make an LUP-factorization to help us.
  viskores::Matrix<T, Size, Size> LU = A;
  viskores::Vec<viskores::IdComponent, Size> permutation;
  T inversionParity; // Unused.
  detail::MatrixLUPFactor(LU, permutation, inversionParity, valid);

  // Next, use the decomposition to solve the system.
  return detail::MatrixLUPSolve(LU, permutation, b);
}

/// Find and return the inverse of the given matrix. If the matrix is singular,
/// the inverse will not be correct and valid will be set to false.
///
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT viskores::Matrix<T, Size, Size> MatrixInverse(
  const viskores::Matrix<T, Size, Size>& A,
  bool& valid)
{
  // First, we will make an LUP-factorization to help us.
  viskores::Matrix<T, Size, Size> LU = A;
  viskores::Vec<viskores::IdComponent, Size> permutation;
  T inversionParity; // Unused
  detail::MatrixLUPFactor(LU, permutation, inversionParity, valid);

  // We will use the decomposition to solve AX = I for X where X is
  // clearly the inverse of A.  Our solve method only works for vectors,
  // so we solve for one column of invA at a time.
  viskores::Matrix<T, Size, Size> invA;
  viskores::Vec<T, Size> ICol(T(0));
  for (viskores::IdComponent colIndex = 0; colIndex < Size; colIndex++)
  {
    ICol[colIndex] = 1;
    viskores::Vec<T, Size> invACol = detail::MatrixLUPSolve(LU, permutation, ICol);
    ICol[colIndex] = 0;
    viskores::MatrixSetColumn(invA, colIndex, invACol);
  }
  return invA;
}

/// Compute the determinant of a matrix.
///
template <typename T, viskores::IdComponent Size>
VISKORES_EXEC_CONT T MatrixDeterminant(const viskores::Matrix<T, Size, Size>& A)
{
  // First, we will make an LUP-factorization to help us.
  viskores::Matrix<T, Size, Size> LU = A;
  viskores::Vec<viskores::IdComponent, Size> permutation;
  T inversionParity;
  bool valid;
  detail::MatrixLUPFactor(LU, permutation, inversionParity, valid);

  // If the matrix is singular, the factorization is invalid, but in that
  // case we know that the determinant is 0.
  if (!valid)
  {
    return 0;
  }

  // The determinant is equal to the product of the diagonal of the L matrix,
  // possibly negated depending on the parity of the inversion. The
  // inversionParity variable is set to 1.0 and -1.0 for even and odd parity,
  // respectively. This sign determines whether the product should be negated.
  T product = inversionParity;
  for (viskores::IdComponent index = 0; index < Size; index++)
  {
    product *= LU(index, index);
  }
  return product;
}

// Specializations for common small determinants.

template <typename T>
VISKORES_EXEC_CONT T MatrixDeterminant(const viskores::Matrix<T, 1, 1>& A)
{
  return A(0, 0);
}

template <typename T>
VISKORES_EXEC_CONT T MatrixDeterminant(const viskores::Matrix<T, 2, 2>& A)
{
  return viskores::DifferenceOfProducts(A(0, 0), A(1, 1), A(1, 0), A(0, 1));
}

template <typename T>
VISKORES_EXEC_CONT T MatrixDeterminant(const viskores::Matrix<T, 3, 3>& A)
{
  return A(0, 0) * A(1, 1) * A(2, 2) + A(1, 0) * A(2, 1) * A(0, 2) + A(2, 0) * A(0, 1) * A(1, 2) -
    A(0, 0) * A(2, 1) * A(1, 2) - A(1, 0) * A(0, 1) * A(2, 2) - A(2, 0) * A(1, 1) * A(0, 2);
}

//---------------------------------------------------------------------------
// Implementations of traits for matrices.

/// Tag used to identify 2 dimensional types (matrices). A TypeTraits class
/// will typedef this class to DimensionalityTag.
///
struct TypeTraitsMatrixTag
{
};

template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
struct TypeTraits<viskores::Matrix<T, NumRow, NumCol>>
{
  using NumericTag = typename TypeTraits<T>::NumericTag;
  using DimensionalityTag = viskores::TypeTraitsMatrixTag;

  VISKORES_EXEC_CONT
  static viskores::Matrix<T, NumRow, NumCol> ZeroInitialization()
  {
    return viskores::Matrix<T, NumRow, NumCol>(viskores::TypeTraits<T>::ZeroInitialization());
  }
};

/// A matrix has vector traits to implement component-wise operations.
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
struct VecTraits<viskores::Matrix<T, NumRow, NumCol>>
{
private:
  using MatrixType = viskores::Matrix<T, NumRow, NumCol>;

public:
  using ComponentType = T;
  using BaseComponentType = typename viskores::VecTraits<T>::BaseComponentType;
  static constexpr viskores::IdComponent NUM_COMPONENTS = NumRow * NumCol;
  using HasMultipleComponents = viskores::VecTraitsTagMultipleComponents;
  using IsSizeStatic = viskores::VecTraitsTagSizeStatic;

  VISKORES_EXEC_CONT
  static viskores::IdComponent GetNumberOfComponents(const MatrixType&) { return NUM_COMPONENTS; }

  VISKORES_EXEC_CONT
  static const ComponentType& GetComponent(const MatrixType& matrix,
                                           viskores::IdComponent component)
  {
    viskores::IdComponent colIndex = component % NumCol;
    viskores::IdComponent rowIndex = component / NumCol;
    return matrix(rowIndex, colIndex);
  }
  VISKORES_EXEC_CONT
  static ComponentType& GetComponent(MatrixType& matrix, viskores::IdComponent component)
  {
    viskores::IdComponent colIndex = component % NumCol;
    viskores::IdComponent rowIndex = component / NumCol;
    return matrix(rowIndex, colIndex);
  }
  VISKORES_EXEC_CONT
  static void SetComponent(MatrixType& matrix, viskores::IdComponent component, T value)
  {
    GetComponent(matrix, component) = value;
  }

  template <typename NewComponentType>
  using ReplaceComponentType = viskores::Matrix<NewComponentType, NumRow, NumCol>;

  template <typename NewComponentType>
  using ReplaceBaseComponentType = viskores::Matrix<
    typename viskores::VecTraits<T>::template ReplaceBaseComponentType<NewComponentType>,
    NumRow,
    NumCol>;
};

//---------------------------------------------------------------------------
// Basic comparison operators.

template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT bool operator==(const viskores::Matrix<T, NumRow, NumCol>& a,
                                   const viskores::Matrix<T, NumRow, NumCol>& b)
{
  for (viskores::IdComponent colIndex = 0; colIndex < NumCol; colIndex++)
  {
    for (viskores::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
    {
      if (a(rowIndex, colIndex) != b(rowIndex, colIndex))
        return false;
    }
  }
  return true;
}
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_EXEC_CONT bool operator!=(const viskores::Matrix<T, NumRow, NumCol>& a,
                                   const viskores::Matrix<T, NumRow, NumCol>& b)
{
  return !(a == b);
}

/// Helper function for printing out matricies during testing
///
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
VISKORES_CONT std::ostream& operator<<(std::ostream& stream,
                                       const viskores::Matrix<T, NumRow, NumCol>& mat)
{
  stream << std::endl;
  for (viskores::IdComponent row = 0; row < NumRow; ++row)
  {
    stream << "| ";
    for (viskores::IdComponent col = 0; col < NumCol; ++col)
    {
      stream << mat(row, col) << "\t";
    }
    stream << "|" << std::endl;
  }
  return stream;
}
} // namespace viskores

#endif //viskores_Matrix_h
