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

#include <viskores/Matrix.h>

#include <viskores/VecTraits.h>

#include <viskores/testing/Testing.h>

// If more tests need a value for Matrix, we can move this to Testing.h
template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
viskores::Matrix<T, NumRow, NumCol> TestValue(viskores::Id index,
                                              const viskores::Matrix<T, NumRow, NumCol>&)
{
  viskores::Matrix<T, NumRow, NumCol> value;
  for (viskores::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    using RowType = viskores::Vec<T, NumCol>;
    RowType row = TestValue(index, RowType()) +
      RowType(static_cast<typename RowType::ComponentType>(10 * rowIndex));
    viskores::MatrixSetRow(value, rowIndex, row);
  }
  return value;
}

namespace
{

#define FOR_ROW_COL(matrix)                                           \
  for (viskores::IdComponent row = 0; row < (matrix).NUM_ROWS; row++) \
    for (viskores::IdComponent col = 0; col < (matrix).NUM_COLUMNS; col++)

template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
struct MatrixTest
{
  static constexpr viskores::IdComponent NUM_ROWS = NumRow;
  static constexpr viskores::IdComponent NUM_COLS = NumCol;
  using MatrixType = viskores::Matrix<T, NUM_ROWS, NUM_COLS>;
  using ComponentType = typename MatrixType::ComponentType;

  static void BasicCreation()
  {
    MatrixType matrix(5);
    FOR_ROW_COL(matrix)
    {
      VISKORES_TEST_ASSERT(test_equal(matrix(row, col), static_cast<T>(5)),
                           "Constant set incorrect.");
    }
  }

  static void BasicAccessors()
  {
    MatrixType matrix;
    MatrixType value = TestValue(0, MatrixType());
    FOR_ROW_COL(matrix)
    {
      matrix[row][col] = ComponentType(value(row, col) * 2);
    }
    FOR_ROW_COL(matrix)
    {
      VISKORES_TEST_ASSERT(test_equal(matrix(row, col), value(row, col) * 2),
                           "Bad set or retrieve.");
      const MatrixType const_matrix = matrix;
      VISKORES_TEST_ASSERT(test_equal(const_matrix(row, col), value(row, col) * 2),
                           "Bad set or retrieve.");
    }

    FOR_ROW_COL(matrix)
    {
      matrix(row, col) = value(row, col);
    }
    const MatrixType const_matrix = matrix;
    FOR_ROW_COL(matrix)
    {
      VISKORES_TEST_ASSERT(test_equal(matrix[row][col], value(row, col)), "Bad set or retrieve.");
      VISKORES_TEST_ASSERT(test_equal(const_matrix[row][col], value(row, col)),
                           "Bad set or retrieve.");
    }
    VISKORES_TEST_ASSERT(matrix == const_matrix, "Equal test operator not working.");
    VISKORES_TEST_ASSERT(!(matrix != const_matrix), "Not-Equal test operator not working.");
    VISKORES_TEST_ASSERT(test_equal(matrix, const_matrix), "Vector-based equal test not working.");
  }

  static void RowColAccessors()
  {
    using ColumnType = viskores::Vec<T, NUM_ROWS>;
    using RowType = viskores::Vec<T, NUM_COLS>;
    const MatrixType const_matrix = TestValue(0, MatrixType());
    MatrixType matrix;

    FOR_ROW_COL(matrix)
    {
      RowType rowvec = viskores::MatrixGetRow(const_matrix, row);
      VISKORES_TEST_ASSERT(test_equal(rowvec[col], const_matrix(row, col)), "Bad get row.");
      ColumnType columnvec = viskores::MatrixGetColumn(const_matrix, col);
      VISKORES_TEST_ASSERT(test_equal(columnvec[row], const_matrix(row, col)), "Bad get col.");
    }

    for (viskores::IdComponent row = 0; row < NUM_ROWS; row++)
    {
      RowType rowvec = viskores::MatrixGetRow(const_matrix, NUM_ROWS - row - 1);
      viskores::MatrixSetRow(matrix, row, rowvec);
    }
    FOR_ROW_COL(matrix)
    {
      VISKORES_TEST_ASSERT(test_equal(matrix(NUM_ROWS - row - 1, col), const_matrix(row, col)),
                           "Rows not set right.");
    }

    for (viskores::IdComponent col = 0; col < NUM_COLS; col++)
    {
      ColumnType colvec = viskores::MatrixGetColumn(const_matrix, NUM_COLS - col - 1);
      viskores::MatrixSetColumn(matrix, col, colvec);
    }
    FOR_ROW_COL(matrix)
    {
      VISKORES_TEST_ASSERT(test_equal(matrix(row, NUM_COLS - col - 1), const_matrix(row, col)),
                           "Columns not set right.");
    }
  }

  static void Multiply()
  {
    const MatrixType leftFactor = TestValue(0, MatrixType());
    viskores::Matrix<T, NUM_COLS, 4> rightFactor = TestValue(1, viskores::Matrix<T, NUM_COLS, 4>());

    viskores::Matrix<T, NUM_ROWS, 4> product = viskores::MatrixMultiply(leftFactor, rightFactor);

    FOR_ROW_COL(product)
    {
      viskores::Vec<T, NUM_COLS> leftVector = viskores::MatrixGetRow(leftFactor, row);
      viskores::Vec<T, NUM_COLS> rightVector = viskores::MatrixGetColumn(rightFactor, col);
      VISKORES_TEST_ASSERT(test_equal(product(row, col), viskores::Dot(leftVector, rightVector)),
                           "Matrix multiple wrong.");
    }

    MatrixType matrixFactor;
    viskores::Vec<T, NUM_ROWS> leftVector(2);
    viskores::Vec<T, NUM_COLS> rightVector;
    FOR_ROW_COL(matrixFactor)
    {
      matrixFactor(row, col) = T(row + 1);
      rightVector[col] = T(col + 1);
    }

    viskores::Vec<T, NUM_COLS> leftResult = viskores::MatrixMultiply(leftVector, matrixFactor);
    for (viskores::IdComponent index = 0; index < NUM_COLS; index++)
    {
      VISKORES_TEST_ASSERT(test_equal(leftResult[index], T(NUM_ROWS * (NUM_ROWS + 1))),
                           "Vector/matrix multiple wrong.");
    }

    viskores::Vec<T, NUM_ROWS> rightResult = viskores::MatrixMultiply(matrixFactor, rightVector);
    for (viskores::IdComponent index = 0; index < NUM_ROWS; index++)
    {
      VISKORES_TEST_ASSERT(
        test_equal(rightResult[index], T(((index + 1) * NUM_COLS * (NUM_COLS + 1)) / 2)),
        "Matrix/vector multiple wrong.");
    }
  }

  static void Identity()
  {
    MatrixType originalMatrix = TestValue(0, MatrixType());

    viskores::Matrix<T, NUM_COLS, NUM_COLS> identityMatrix;
    viskores::MatrixIdentity(identityMatrix);

    MatrixType multMatrix = viskores::MatrixMultiply(originalMatrix, identityMatrix);

    VISKORES_TEST_ASSERT(test_equal(originalMatrix, multMatrix),
                         "Identity is not really identity.");
  }

  static void Transpose()
  {
    MatrixType originalMatrix = TestValue(0, MatrixType());

    viskores::Matrix<T, NUM_COLS, NUM_ROWS> transMatrix = viskores::MatrixTranspose(originalMatrix);
    FOR_ROW_COL(originalMatrix)
    {
      VISKORES_TEST_ASSERT(test_equal(originalMatrix(row, col), transMatrix(col, row)),
                           "Transpose wrong.");
    }
  }

  static void Run()
  {
    BasicCreation();
    BasicAccessors();
    RowColAccessors();
    Multiply();
    Identity();
    Transpose();
  }

private:
  MatrixTest() = delete;
};

template <typename T, int NumRow>
void MatrixTest1()
{
  MatrixTest<T, NumRow, 1>::Run();
  MatrixTest<T, NumRow, 2>::Run();
  MatrixTest<T, NumRow, 3>::Run();
  MatrixTest<T, NumRow, 4>::Run();
  MatrixTest<T, NumRow, 5>::Run();
}

template <typename T>
void NonSingularMatrix(viskores::Matrix<T, 1, 1>& matrix)
{
  matrix(0, 0) = 1;
}

template <typename T>
void NonSingularMatrix(viskores::Matrix<T, 2, 2>& matrix)
{
  matrix(0, 0) = -5;
  matrix(0, 1) = 6;
  matrix(1, 0) = -7;
  matrix(1, 1) = -2;
}

template <typename T>
void NonSingularMatrix(viskores::Matrix<T, 3, 3>& matrix)
{
  matrix(0, 0) = 1;
  matrix(0, 1) = -2;
  matrix(0, 2) = 3;
  matrix(1, 0) = 6;
  matrix(1, 1) = 7;
  matrix(1, 2) = -1;
  matrix(2, 0) = -3;
  matrix(2, 1) = 1;
  matrix(2, 2) = 4;
}

template <typename T>
void NonSingularMatrix(viskores::Matrix<T, 4, 4>& matrix)
{
  matrix(0, 0) = 2;
  matrix(0, 1) = 1;
  matrix(0, 2) = 0;
  matrix(0, 3) = 3;
  matrix(1, 0) = -1;
  matrix(1, 1) = 0;
  matrix(1, 2) = 2;
  matrix(1, 3) = 4;
  matrix(2, 0) = 4;
  matrix(2, 1) = -2;
  matrix(2, 2) = 7;
  matrix(2, 3) = 0;
  matrix(3, 0) = -4;
  matrix(3, 1) = 3;
  matrix(3, 2) = 5;
  matrix(3, 3) = 1;
}

template <typename T>
void NonSingularMatrix(viskores::Matrix<T, 5, 5>& mat)
{
  mat(0, 0) = 2;
  mat(0, 1) = 1;
  mat(0, 2) = 3;
  mat(0, 3) = 7;
  mat(0, 4) = 5;
  mat(1, 0) = 3;
  mat(1, 1) = 8;
  mat(1, 2) = 7;
  mat(1, 3) = 9;
  mat(1, 4) = 8;
  mat(2, 0) = 3;
  mat(2, 1) = 4;
  mat(2, 2) = 1;
  mat(2, 3) = 6;
  mat(2, 4) = 2;
  mat(3, 0) = 4;
  mat(3, 1) = 0;
  mat(3, 2) = 2;
  mat(3, 3) = 2;
  mat(3, 4) = 3;
  mat(4, 0) = 7;
  mat(4, 1) = 9;
  mat(4, 2) = 1;
  mat(4, 3) = 5;
  mat(4, 4) = 4;
}

template <typename T, viskores::IdComponent S>
void PrintMatrix(const viskores::Matrix<T, S, S>& m)
{
  std::cout << "matrix\n";
  for (viskores::IdComponent i = 0; i < S; ++i)
  {
    std::cout << "\t" << m[i] << "\n";
  }
  std::cout << std::flush;
}

template <typename T, int Size>
void SingularMatrix(viskores::Matrix<T, Size, Size>& singularMatrix)
{
  FOR_ROW_COL(singularMatrix)
  {
    singularMatrix(row, col) = static_cast<T>(row + col);
  }
  constexpr bool larger_than_1 = Size > 1;
  if (larger_than_1)
  {
    viskores::MatrixSetRow(
      singularMatrix, 0, viskores::MatrixGetRow(singularMatrix, (Size + 1) / 2));
  }
}

// A simple but slow implementation of finding a determinant for comparison
// purposes.
template <typename T>
T RecursiveDeterminant(const viskores::Matrix<T, 1, 1>& A)
{
  return A(0, 0);
}

template <typename T, viskores::IdComponent Size>
T RecursiveDeterminant(const viskores::Matrix<T, Size, Size>& A)
{
  viskores::Matrix<T, Size - 1, Size - 1> cofactorMatrix;
  T sum = 0.0;
  T sign = 1.0;
  for (viskores::IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    // Create the cofactor matrix for entry A(rowIndex,0)
    for (viskores::IdComponent cofactorRowIndex = 0; cofactorRowIndex < rowIndex;
         cofactorRowIndex++)
    {
      for (viskores::IdComponent colIndex = 1; colIndex < Size; colIndex++)
      {
        cofactorMatrix(cofactorRowIndex, colIndex - 1) = A(cofactorRowIndex, colIndex);
      }
    }
    for (viskores::IdComponent cofactorRowIndex = rowIndex + 1; cofactorRowIndex < Size;
         cofactorRowIndex++)
    {
      for (viskores::IdComponent colIndex = 1; colIndex < Size; colIndex++)
      {
        cofactorMatrix(cofactorRowIndex - 1, colIndex - 1) = A(cofactorRowIndex, colIndex);
      }
    }
    sum += sign * A(rowIndex, 0) * RecursiveDeterminant(cofactorMatrix);
    sign = -sign;
  }
  return sum;
}

template <typename T, viskores::IdComponent Size>
struct SquareMatrixTest
{
  static constexpr viskores::IdComponent SIZE = Size;
  using MatrixType = viskores::Matrix<T, Size, Size>;

  static void CheckMatrixSize()
  {
    VISKORES_TEST_ASSERT(MatrixType::NUM_ROWS == SIZE, "Matrix has wrong size.");
    VISKORES_TEST_ASSERT(MatrixType::NUM_COLUMNS == SIZE, "Matrix has wrong size.");
  }

  static void LUPFactor()
  {
    MatrixType A;
    NonSingularMatrix(A);
    const MatrixType originalMatrix = A;
    viskores::Vec<viskores::IdComponent, SIZE> permutationVector;
    T inversionParity;
    bool valid;

    viskores::detail::MatrixLUPFactor(A, permutationVector, inversionParity, valid);
    VISKORES_TEST_ASSERT(valid, "Matrix declared singular?");

    // Reconstruct L and U matrices from A.
    MatrixType L(0);
    MatrixType U(0);
    FOR_ROW_COL(A)
    {
      if (row < col)
      {
        U(row, col) = A(row, col);
      }
      else //(row >= col)
      {
        L(row, col) = A(row, col);
        if (row == col)
        {
          U(row, col) = 1;
        }
      }
    }

    // Check parity of permutation.
    T computedParity = 1.0;
    for (int i = 0; i < SIZE; i++)
    {
      for (int j = i + 1; j < SIZE; j++)
      {
        if (permutationVector[i] > permutationVector[j])
        {
          computedParity = -computedParity;
        }
      }
    }
    VISKORES_TEST_ASSERT(test_equal(inversionParity, computedParity), "Got bad inversion parity.");

    // Reconstruct permutation matrix P.
    MatrixType P(0);
    for (viskores::IdComponent index = 0; index < Size; index++)
    {
      P(index, permutationVector[index]) = 1;
    }

    // Check that PA = LU is actually correct.
    MatrixType permutedMatrix = viskores::MatrixMultiply(P, originalMatrix);
    MatrixType productMatrix = viskores::MatrixMultiply(L, U);
    VISKORES_TEST_ASSERT(test_equal(permutedMatrix, productMatrix),
                         "LUP-factorization gave inconsistent answer.");

    // Check that a singular matrix is identified.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);
    viskores::detail::MatrixLUPFactor(singularMatrix, permutationVector, inversionParity, valid);
    VISKORES_TEST_ASSERT(!valid, "Expected matrix to be declared singular.");
  }

  static void SolveLinearSystem()
  {
    MatrixType A;
    viskores::Vec<T, SIZE> b;
    NonSingularMatrix(A);
    for (viskores::IdComponent index = 0; index < SIZE; index++)
    {
      b[index] = static_cast<T>(index + 1);
    }
    bool valid;

    viskores::Vec<T, SIZE> x = viskores::SolveLinearSystem(A, b, valid);
    VISKORES_TEST_ASSERT(valid, "Matrix declared singular?");

    // Check result.
    viskores::Vec<T, SIZE> check = viskores::MatrixMultiply(A, x);
    VISKORES_TEST_ASSERT(test_equal(b, check), "Linear solution does not solve equation.");

    // Check that a singular matrix is identified.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);

    x = viskores::SolveLinearSystem(singularMatrix, b, valid);
    for (viskores::IdComponent i = 0; i < SIZE; ++i)
    {
      VISKORES_TEST_ASSERT(viskores::IsNan(x[i]),
                           "Expected values of solution to singular matrix to be NaNs");
    }

    VISKORES_TEST_ASSERT(!valid, "Expected matrix to be declared singular.");
  }

  static void Invert()
  {
    MatrixType A;
    NonSingularMatrix(A);
    bool valid;

    viskores::Matrix<T, SIZE, SIZE> inverse = viskores::MatrixInverse(A, valid);
    VISKORES_TEST_ASSERT(valid, "Matrix declared singular?");

    // Check result.
    viskores::Matrix<T, SIZE, SIZE> product = viskores::MatrixMultiply(A, inverse);
    VISKORES_TEST_ASSERT(test_equal(product, viskores::MatrixIdentity<T, SIZE>()),
                         "Matrix inverse did not give identity.");

    // Check that a singular matrix is identified.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);
    viskores::MatrixInverse(singularMatrix, valid);
    VISKORES_TEST_ASSERT(!valid, "Expected matrix to be declared singular.");
  }

  static void Determinant()
  {
    MatrixType A;
    NonSingularMatrix(A);

    T determinant = viskores::MatrixDeterminant(A);

    // Check result.
    T determinantCheck = RecursiveDeterminant(A);
    VISKORES_TEST_ASSERT(test_equal(determinant, determinantCheck),
                         "Determinant computations do not agree.");

    // Check that a singular matrix has a zero determinant.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);
    determinant = viskores::MatrixDeterminant(singularMatrix);
    VISKORES_TEST_ASSERT(test_equal(determinant, T(0.0)),
                         "Non-zero determinant for singular matrix.");
  }

  static void Run()
  {
    CheckMatrixSize();
    LUPFactor();
    SolveLinearSystem();
    Invert();
    Determinant();
  }

private:
  SquareMatrixTest() = delete;
};

struct MatrixTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    MatrixTest1<T, 1>();
    MatrixTest1<T, 2>();
    MatrixTest1<T, 3>();
    MatrixTest1<T, 4>();
    MatrixTest1<T, 5>();
  }
};

struct SquareMatrixTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    SquareMatrixTest<T, 1>::Run();
    SquareMatrixTest<T, 2>::Run();
    SquareMatrixTest<T, 3>::Run();
    SquareMatrixTest<T, 4>::Run();
    SquareMatrixTest<T, 5>::Run();
  }
};

struct VectorMultFunctor
{
  template <class VectorType>
  void operator()(const VectorType&) const
  {
    // This is mostly to make sure the compile can convert from Tuples
    // to vectors.
    const int SIZE = viskores::VecTraits<VectorType>::NUM_COMPONENTS;
    using ComponentType = typename viskores::VecTraits<VectorType>::ComponentType;

    viskores::Matrix<ComponentType, SIZE, SIZE> matrix(0);
    VectorType inVec;
    VectorType outVec;
    for (viskores::IdComponent index = 0; index < SIZE; index++)
    {
      matrix(index, index) = 1;
      inVec[index] = ComponentType(index + 1);
    }

    outVec = viskores::MatrixMultiply(matrix, inVec);
    VISKORES_TEST_ASSERT(test_equal(inVec, outVec), "Bad identity multiply.");

    outVec = viskores::MatrixMultiply(inVec, matrix);
    VISKORES_TEST_ASSERT(test_equal(inVec, outVec), "Bad identity multiply.");
  }
};

void TestMatrices()
{
  //  std::cout << "****** Rectangle tests" << std::endl;
  //  viskores::testing::Testing::TryTypes(MatrixTestFunctor(),
  //                                   viskores::TypeListScalarAll());

  viskores::testing::Testing::TryTypes(SquareMatrixTestFunctor(), viskores::TypeListFieldScalar());

  //  std::cout << "***** Vector multiply tests" << std::endl;
  //  viskores::testing::Testing::TryTypes(VectorMultFunctor(),
  //                                   viskores::TypeListVecAll());
}

} // anonymous namespace

int UnitTestMatrix(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestMatrices, argc, argv);
}
