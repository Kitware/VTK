// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMathPrivate
 * @brief   Internal toolkit used in some vtkMath methods.
 *
 * vtkMathPrivate provides meta-classes helpers that are used in some vtkMath
 * methods.
 * @sa
 * vtkMath
 * vtkMatrixUtilities
 */

#ifndef vtkMathPrivate_hxx
#define vtkMathPrivate_hxx

#include "vtkCommonCoreModule.h" //required for correct implementation
#include "vtkMatrixUtilities.h"

#include <type_traits>
#include <utility>

namespace vtkMathPrivate
{
VTK_ABI_NAMESPACE_BEGIN
static constexpr int VTK_MATH_PRIVATE_PACK_SIZE = 4;

//=============================================================================
// This class computes the dot product between row RowT of matrix M1
// and column ColT of matrix M2.
// The template parameter IdxT is on index ahead from computation.
// Template parameters LayoutT1 and LayoutT2 respectively reindex
// input matrices M1 and M2 according to MatrixLayout enumeration
// M1 (or M1^T if LayoutT1 == vtkMatrixUtilities::Layout::Transpose) is a matrix of RowsT   x
// MidDimT M2 (or M2^T if LayoutT2 == vtkMatrixUtilities::Layout::Transpose) is a matrix of MidDimT
// x ColsDimT
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT,
  class LayoutT1 = vtkMatrixUtilities::Layout::Identity,
  class LayoutT2 = vtkMatrixUtilities::Layout::Identity, int IdxT = 0, int PackSizeT = MidDimT>
class ContractRowWithCol
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    return ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT,
             Shift>::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2)) +
      ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
        IdxT + Shift, PackSizeT - Shift>::Compute(std::forward<MatrixT1>(M1),
        std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr int Shift =
    (MidDimT - IdxT) / VTK_MATH_PRIVATE_PACK_SIZE ? VTK_MATH_PRIVATE_PACK_SIZE : MidDimT - IdxT - 1;
};

//=============================================================================
// Specialization for when there are 4 components left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 4>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    using Wrap1 = vtkMatrixUtilities::Wrapper<RowsT, MidDimT, MatrixT1, LayoutT1>;
    using Wrap2 = vtkMatrixUtilities::Wrapper<MidDimT, ColsT, MatrixT2, LayoutT2>;

    return Wrap1::template Get<RowT, IdxT>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT, ColT>(std::forward<MatrixT2>(M2)) +
      Wrap1::template Get<RowT, IdxT + 1>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT + 1, ColT>(std::forward<MatrixT2>(M2)) +
      Wrap1::template Get<RowT, IdxT + 2>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT + 2, ColT>(std::forward<MatrixT2>(M2)) +
      Wrap1::template Get<RowT, IdxT + 3>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT + 3, ColT>(std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// Specialization for when there are 3 components left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 3>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    using Wrap1 = vtkMatrixUtilities::Wrapper<RowsT, MidDimT, MatrixT1, LayoutT1>;
    using Wrap2 = vtkMatrixUtilities::Wrapper<MidDimT, ColsT, MatrixT2, LayoutT2>;

    return Wrap1::template Get<RowT, IdxT>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT, ColT>(std::forward<MatrixT2>(M2)) +
      Wrap1::template Get<RowT, IdxT + 1>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT + 1, ColT>(std::forward<MatrixT2>(M2)) +
      Wrap1::template Get<RowT, IdxT + 2>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT + 2, ColT>(std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// Specialization for when there are 2 components left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 2>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    using Wrap1 = vtkMatrixUtilities::Wrapper<RowsT, MidDimT, MatrixT1, LayoutT1>;
    using Wrap2 = vtkMatrixUtilities::Wrapper<MidDimT, ColsT, MatrixT2, LayoutT2>;

    return Wrap1::template Get<RowT, IdxT>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT, ColT>(std::forward<MatrixT2>(M2)) +
      Wrap1::template Get<RowT, IdxT + 1>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT + 1, ColT>(std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// Specialization for when there is 1 component left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 1>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    using Wrap1 = vtkMatrixUtilities::Wrapper<RowsT, MidDimT, MatrixT1, LayoutT1>;
    using Wrap2 = vtkMatrixUtilities::Wrapper<MidDimT, ColsT, MatrixT2, LayoutT2>;

    return Wrap1::template Get<RowT, IdxT>(std::forward<MatrixT1>(M1)) *
      Wrap2::template Get<IdxT, ColT>(std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// This class handles row and col contraction when at least one of the 2 input
// matrices are diagonal
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2>
class DiagContractRowWithCol;

//=============================================================================
// Specialization for when M1 is diagonal
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT2>
class DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT,
  vtkMatrixUtilities::Layout::Diag, LayoutT2>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    return M1[RowT] * M2[Mapper2::template GetIndex<RowT, ColT>()];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  using Mapper2 = vtkMatrixUtilities::Mapper<MidDimT, ColsT, LayoutT2>;
};

//=============================================================================
// Specialization for when M2 is diagonal
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1>
class DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1,
  vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    return M1[Mapper1::template GetIndex<RowT, ColT>()] * M2[ColT];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  using Mapper1 = vtkMatrixUtilities::Mapper<RowsT, MidDimT, LayoutT1>;
};

//=============================================================================
// This class returns zero. It is used for rectangular diagonal matrices, when
// lines / rows are filled with zeros.
template <class ScalarT>
class NullContractRowWithCol
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(MatrixT1&&, MatrixT2&&)
  {
    return ScalarT(0);
  }
};

//=============================================================================
// Helper class to choose between regular contraction class vs diagonal
// handling contraction class.
// By default, Type is an instance of ContractRowWithCol
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, bool HasOneDiagonalMatrixT = false>
struct ContractRowWithColSwitch
{
  typedef ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2> Type;

  static constexpr bool NO_INPUT_MATRIX_CAN_BE_DIAGONAL =
    !std::is_same<LayoutT1, vtkMatrixUtilities::Layout::Diag>::value &&
    !std::is_same<LayoutT2, vtkMatrixUtilities::Layout::Diag>::value;
  static_assert(NO_INPUT_MATRIX_CAN_BE_DIAGONAL,
    "LayoutT1 and LayoutT2 cannot equal vtkMatrixUtilities::Layout::Diag in this setup");
};

//=============================================================================
// Specialization for diagonal matrices.
// Rectangular diagonal matrices are handled using NullContractRowWithCol.
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2>
struct ContractRowWithColSwitch<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
  true>
{
  static constexpr bool IsDiagonal1 =
    std::is_same<LayoutT1, vtkMatrixUtilities::Layout::Diag>::value;
  static constexpr bool IsDiagonal2 =
    std::is_same<LayoutT2, vtkMatrixUtilities::Layout::Diag>::value;

  // If on of the diagonal matrix is rectangular and we are out of the diagonal
  // length, all remaining values are null.
  static constexpr bool UseNullContractRowWithCol =
    (IsDiagonal1 && RowT >= MidDimT) || (IsDiagonal2 && ColT >= MidDimT);

  typedef typename std::conditional<UseNullContractRowWithCol, NullContractRowWithCol<ScalarT>,
    DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2>>::type
    Type;

  static constexpr bool NEEDS_AT_LEAST_ONE_DIAGONAL_INPUT_MATRIX = IsDiagonal1 || IsDiagonal2;
  static_assert(NEEDS_AT_LEAST_ONE_DIAGONAL_INPUT_MATRIX,
    "LayoutT1 or LayoutT2 must equal vtkMatrixUtilities::Layout::Diag in this setup");
};

namespace detail
{
//=============================================================================
// Class in charge for actually multiplying 2 matrices.
// This method is called by MultiplyMatrix::Compute for chunks of size
// VTK_MATH_PRIVATE_PACK_SIZE at most, specified in ColPackSizeT.
// This class mostly consists on explicitly onfold computation for those chunks.
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT,
  int ColPackSizeT>
class MultiplyMatrix;

//=============================================================================
// Specialization for a chunk of size 4
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 4>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT3>::value_type;
    using Wrap3 = vtkMatrixUtilities::Wrapper<RowsT, ColsT, MatrixT3>;

    Wrap3::template Get<RowT, ColT>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
    Wrap3::template Get<RowT, ColT + 1>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT + 1, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
    Wrap3::template Get<RowT, ColT + 2>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT + 2, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
    Wrap3::template Get<RowT, ColT + 3>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT + 3, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT + 3 < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool OneMatrixIsDiagonal =
    std::is_same<LayoutT1, vtkMatrixUtilities::Layout::Diag>::value ||
    std::is_same<LayoutT2, vtkMatrixUtilities::Layout::Diag>::value;
};

//=============================================================================
// Specialization for a chunk of size 3
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 3>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT3>::value_type;
    using Wrap3 = vtkMatrixUtilities::Wrapper<RowsT, ColsT, MatrixT3>;

    Wrap3::template Get<RowT, ColT>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
    Wrap3::template Get<RowT, ColT + 1>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT + 1, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
    Wrap3::template Get<RowT, ColT + 2>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT + 2, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT + 2 < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool OneMatrixIsDiagonal =
    std::is_same<LayoutT1, vtkMatrixUtilities::Layout::Diag>::value ||
    std::is_same<LayoutT2, vtkMatrixUtilities::Layout::Diag>::value;
};

//=============================================================================
// Specialization for a chunk of size 2
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 2>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT3>::value_type;
    using Wrap3 = vtkMatrixUtilities::Wrapper<RowsT, ColsT, MatrixT3>;

    Wrap3::template Get<RowT, ColT>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
    Wrap3::template Get<RowT, ColT + 1>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT + 1, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT + 1 < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool OneMatrixIsDiagonal =
    std::is_same<LayoutT1, vtkMatrixUtilities::Layout::Diag>::value ||
    std::is_same<LayoutT2, vtkMatrixUtilities::Layout::Diag>::value;
};

//=============================================================================
// Specialization for a chunk of size 1
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 1>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT3>::value_type;
    using Wrap3 = vtkMatrixUtilities::Wrapper<RowsT, ColsT, MatrixT3>;

    Wrap3::template Get<RowT, ColT>(std::forward<MatrixT3>(M3)) =
      ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
        OneMatrixIsDiagonal>::Type::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2));
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool OneMatrixIsDiagonal =
    std::is_same<LayoutT1, vtkMatrixUtilities::Layout::Diag>::value ||
    std::is_same<LayoutT2, vtkMatrixUtilities::Layout::Diag>::value;
};

//=============================================================================
// Specialization when both input matrices are diagonal for a chunk of size 4
template <int SizeT, int IdxT>
class MultiplyMatrix<SizeT, SizeT, SizeT, vtkMatrixUtilities::Layout::Diag,
  vtkMatrixUtilities::Layout::Diag, IdxT, IdxT, 4>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    M3[IdxT] = M1[IdxT] * M2[IdxT];
    M3[IdxT + 1] = M1[IdxT + 1] * M2[IdxT + 1];
    M3[IdxT + 2] = M1[IdxT + 2] * M2[IdxT + 2];
    M3[IdxT + 3] = M1[IdxT + 3] * M2[IdxT + 3];
  }
};

//=============================================================================
// Specialization when both input matrices are diagonal for a chunk of size 3
template <int SizeT, int IdxT>
class MultiplyMatrix<SizeT, SizeT, SizeT, vtkMatrixUtilities::Layout::Diag,
  vtkMatrixUtilities::Layout::Diag, IdxT, IdxT, 3>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    M3[IdxT] = M1[IdxT] * M2[IdxT];
    M3[IdxT + 1] = M1[IdxT + 1] * M2[IdxT + 1];
    M3[IdxT + 2] = M1[IdxT + 2] * M2[IdxT + 2];
  }
};

//=============================================================================
// Specialization when both input matrices are diagonal for a chunk of size 2
template <int SizeT, int IdxT>
class MultiplyMatrix<SizeT, SizeT, SizeT, vtkMatrixUtilities::Layout::Diag,
  vtkMatrixUtilities::Layout::Diag, IdxT, IdxT, 2>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    M3[IdxT] = M1[IdxT] * M2[IdxT];
    M3[IdxT + 1] = M1[IdxT + 1] * M2[IdxT + 1];
  }
};

//=============================================================================
// Specialization when both input matrices are diagonal for a chunk of size 1
template <int SizeT, int IdxT>
class MultiplyMatrix<SizeT, SizeT, SizeT, vtkMatrixUtilities::Layout::Diag,
  vtkMatrixUtilities::Layout::Diag, IdxT, IdxT, 1>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    M3[IdxT] = M1[IdxT] * M2[IdxT];
  }
};
} // namespace detail

//=============================================================================
// Multiply matrices such that M3 = M1 * M2.
// Template parameters LayoutT1 and LayoutT2 respectively reindex
// input matrices M1 and M2 following MatrixLayout options
// Hence, if LayoutT1 == vtkMatrixUtilities::Layout::Transpose, then M3 = M1^T * M2, and so on.
// M1 (or M1^T if LayoutT1 == vtkMatrixUtilities::Layout::Transpose) is a matrix of RowsT   x
// MidDimT M2 (or M2^T if LayoutT2 == vtkMatrixUtilities::Layout::Transpose) is a matrix of MidDimT
// x ColsDimT
//
// RemainingRowSizeT should be disregarded when first instantiating this class. It is
// the number of remaining elements in the current row to process.
//
// To compute the multiplication, each component of the output matrix
// is computed chunk by chunk (of size VTK_MATH_PRIVATE_PACK_SIZE),
// starting at the top left, sweeping the rows one by one.
template <int RowsT, int MidDimT, int ColsT, class LayoutT1 = vtkMatrixUtilities::Layout::Identity,
  class LayoutT2 = vtkMatrixUtilities::Layout::Identity, int NextRowT = 1, int NextColT = 1,
  int RemainingRowSizeT = ColsT>
class MultiplyMatrix
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    detail::MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, Row, Col, Shift>::Compute(
      std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2), std::forward<MatrixT3>(M3));
    MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, ForwardRow, ForwardCol,
      ForwardRemainingRowSize>::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2),
      std::forward<MatrixT3>(M3));
  }

private:
  static constexpr int Row = NextRowT - 1;
  static constexpr int Col = NextColT - 1;

  // We go to a new row when the last chunk is lower or equal to
  // VTK_MATH_PRIVATE_PACK_SIZE
  static constexpr bool GoToNewRow = !((ColsT - Col - 1) / VTK_MATH_PRIVATE_PACK_SIZE);

  // This is the chunk size, which is from what we should shift in recursive
  // calls
  static constexpr int Shift = GoToNewRow ? ColsT - Col : VTK_MATH_PRIVATE_PACK_SIZE;

  // Row / col / pack size for the next recursive call, depending on if we
  // change row or not.
  static constexpr int ForwardRow = GoToNewRow ? NextRowT + 1 : NextRowT;
  static constexpr int ForwardCol = GoToNewRow ? 1 : NextColT + Shift;
  static constexpr int ForwardRemainingRowSize = GoToNewRow ? ColsT : RemainingRowSizeT - Shift;

  static constexpr bool ROW_OUT_OF_BOUNDS = Row < RowsT && Row >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = Col < ColsT && Col >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// Specialization for when both input matrices are diagonal
// Warning: RowsT, MidDimT and ColsT MUST match
template <int RowsT, int MidDimT, int ColsT, int NextIdxT, int RemainingRowSizeT>
class MultiplyMatrix<RowsT, MidDimT, ColsT, vtkMatrixUtilities::Layout::Diag,
  vtkMatrixUtilities::Layout::Diag, NextIdxT, NextIdxT, RemainingRowSizeT>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)
  {
    detail::MultiplyMatrix<RowsT, MidDimT, ColsT, vtkMatrixUtilities::Layout::Diag,
      vtkMatrixUtilities::Layout::Diag, NextIdxT - 1, NextIdxT - 1,
      Shift>::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2),
      std::forward<MatrixT3>(M3));
    MultiplyMatrix<RowsT, MidDimT, ColsT, vtkMatrixUtilities::Layout::Diag,
      vtkMatrixUtilities::Layout::Diag, NextIdxT + Shift, NextIdxT + Shift,
      RemainingRowSizeT - Shift>::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2),
      std::forward<MatrixT3>(M3));
  }

private:
  static constexpr bool Shift = VTK_MATH_PRIVATE_PACK_SIZE;

  static constexpr bool DIAGONAL_MATRICES_DIMENSIONS_DONT_MATCH =
    RowsT == MidDimT && RowsT == ColsT;
  static_assert(DIAGONAL_MATRICES_DIMENSIONS_DONT_MATCH, "There must be RowsT = MidDimT = colsT");
  static constexpr bool ROW_OUT_OF_BOUNDS = NextIdxT <= RowsT && NextIdxT > 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = NextIdxT <= ColsT && NextIdxT > 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// Macro defining the specialization for when both input matrices are diagonal,
// implementing the last chunk to compute. They should be specialized
// for integers below VTK_MATH_PRIVATE_PACK_SIZE
#define vtkEndForBothDiagonalMultiplyMatrixSpecializationMacro(RemainingRowSize)                   \
  template <int RowsT, int MidDimT, int ColsT, int NextIdxT>                                       \
  class MultiplyMatrix<RowsT, MidDimT, ColsT, vtkMatrixUtilities::Layout::Diag,                    \
    vtkMatrixUtilities::Layout::Diag, NextIdxT, NextIdxT, RemainingRowSize>                        \
  {                                                                                                \
  public:                                                                                          \
    template <class MatrixT1, class MatrixT2, class MatrixT3>                                      \
    static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)                               \
    {                                                                                              \
      detail::MultiplyMatrix<RowsT, MidDimT, ColsT, vtkMatrixUtilities::Layout::Diag,              \
        vtkMatrixUtilities::Layout::Diag, NextIdxT - 1, NextIdxT - 1,                              \
        RemainingRowSize>::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2),         \
        std::forward<MatrixT3>(M3));                                                               \
    }                                                                                              \
                                                                                                   \
  private:                                                                                         \
    static constexpr bool DIAGONAL_MATRICES_DIMENSIONS_DONT_MATCH =                                \
      RowsT == MidDimT && RowsT == ColsT;                                                          \
    static_assert(                                                                                 \
      DIAGONAL_MATRICES_DIMENSIONS_DONT_MATCH, "There must be RowsT = MidDimT = colsT");           \
    static constexpr bool ROW_OUT_OF_BOUNDS = NextIdxT <= RowsT && NextIdxT > 0;                   \
    static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");                                     \
    static constexpr bool COL_OUT_OF_BOUNDS = NextIdxT <= ColsT && NextIdxT > 0;                   \
    static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");                                     \
  };

//=============================================================================
vtkEndForBothDiagonalMultiplyMatrixSpecializationMacro(1);
vtkEndForBothDiagonalMultiplyMatrixSpecializationMacro(2);
vtkEndForBothDiagonalMultiplyMatrixSpecializationMacro(3);
vtkEndForBothDiagonalMultiplyMatrixSpecializationMacro(4);

#undef vtkEndForBothDiagonalMultiplyMatrixSpecializationMacro

//=============================================================================
// Macro defining specialization for the last chunk to compute. They should be
// specialized for integers below VTK_MATH_PRIVATE_PACK_SIZE
#define vtkLastRowMultiplyMatrixSpecializationMacro(RemainingRowSize)                              \
  template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int NextColT>       \
  class MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowsT, NextColT,                 \
    RemainingRowSize>                                                                              \
  {                                                                                                \
  public:                                                                                          \
    template <class MatrixT1, class MatrixT2, class MatrixT3>                                      \
    static void Compute(MatrixT1&& M1, MatrixT2&& M2, MatrixT3&& M3)                               \
    {                                                                                              \
      detail::MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowsT - 1, NextColT - 1,   \
        RemainingRowSize>::Compute(std::forward<MatrixT1>(M1), std::forward<MatrixT2>(M2),         \
        std::forward<MatrixT3>(M3));                                                               \
    }                                                                                              \
                                                                                                   \
  private:                                                                                         \
    static constexpr bool COL_OUT_OF_BOUNDS = NextColT - 1 < ColsT && NextColT >= 1;               \
    static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");                                     \
  };

//=============================================================================
vtkLastRowMultiplyMatrixSpecializationMacro(1);
vtkLastRowMultiplyMatrixSpecializationMacro(2);
vtkLastRowMultiplyMatrixSpecializationMacro(3);
vtkLastRowMultiplyMatrixSpecializationMacro(4);

#undef vtkLastRowMultiplyMatrixSpecializationMacro

//=============================================================================
// Class computing the determinant of square matrices SizeT x SizeT.
// The template parameter LayoutT is a struct embedded in MatrixLayout
template <int SizeT, class LayoutT = vtkMatrixUtilities::Layout::Identity>
class Determinant;

//=============================================================================
// Specialization for diagonal 3x3 matrices of size
template <>
class Determinant<3, vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Compute(MatrixT&& M)
  {
    return M[0] * M[1] * M[3];
  }
};

//=============================================================================
// Specialization for diagonal 2x2 matrices
template <>
class Determinant<2, vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Compute(MatrixT&& M)
  {
    return M[0] * M[1];
  }
};

//=============================================================================
// Specialization for 1x1 matrices
template <class LayoutT>
class Determinant<1, LayoutT>
{
public:
  template <class MatrixT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Compute(MatrixT&& M)
  {
    return M[0];
  }
};

//=============================================================================
// Specialization for 2x2 non-diagonal matrices
template <class LayoutT>
class Determinant<2, LayoutT>
{
public:
  template <class MatrixT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Compute(MatrixT&& M)
  {
    using Wrap = vtkMatrixUtilities::Wrapper<2, 2, MatrixT, LayoutT>;

    return Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) -
      Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
      Wrap::template Get<0, 1>(std::forward<MatrixT>(M));
  }

private:
};

//=============================================================================
// Specialization for 3x3 non-diagonal matrices
template <class LayoutT>
class Determinant<3, LayoutT>
{
public:
  template <class MatrixT>
  static typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type Compute(MatrixT&& M)
  {
    using Wrap = vtkMatrixUtilities::Wrapper<3, 3, MatrixT, LayoutT>;

    return Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) *
      Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) +
      Wrap::template Get<0, 1>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 2>(std::forward<MatrixT>(M)) *
      Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) +
      Wrap::template Get<0, 2>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
      Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) -
      Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 2>(std::forward<MatrixT>(M)) *
      Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) -
      Wrap::template Get<0, 1>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
      Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
      Wrap::template Get<0, 2>(std::forward<MatrixT>(M)) *
      Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) *
      Wrap::template Get<2, 0>(std::forward<MatrixT>(M));
  }
};

//=============================================================================
// Class inverting square matrices SizeT x SizeT.
// The template parameter LayoutT is a struct embedded in MatrixLayout
template <int SizeT, class LayoutT = vtkMatrixUtilities::Layout::Identity>
class InvertMatrix;

//=============================================================================
// Specialization for 2x2 non-diagonal matrices
template <class LayoutT>
class InvertMatrix<2, LayoutT>
{
public:
  template <class MatrixT1, class MatrixT2>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT2>::value_type;
    using Wrap1 = vtkMatrixUtilities::Wrapper<2, 2, MatrixT1, LayoutT>;
    using Wrap2 = vtkMatrixUtilities::Wrapper<2, 2, MatrixT2>;

    Scalar detInv = 1.0 / Determinant<2, LayoutT>::Compute(std::forward<MatrixT1>(M1));
    Wrap2::template Get<0, 0>(std::forward<MatrixT2>(M2)) =
      detInv * Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1));
    Wrap2::template Get<1, 0>(std::forward<MatrixT2>(M2)) =
      -detInv * Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1));
    Wrap2::template Get<0, 1>(std::forward<MatrixT2>(M2)) =
      -detInv * Wrap1::template Get<0, 1>(std::forward<MatrixT1>(M1));
    Wrap2::template Get<1, 1>(std::forward<MatrixT2>(M2)) =
      detInv * Wrap1::template Get<0, 0>(std::forward<MatrixT1>(M1));
  }
};

//=============================================================================
// Specialization for 3x3 non-diagonal matrices
template <class LayoutT>
class InvertMatrix<3, LayoutT>
{
public:
  template <class MatrixT1, class MatrixT2>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT2>::value_type;
    using Wrap1 = vtkMatrixUtilities::Wrapper<3, 3, MatrixT1, LayoutT>;
    using Wrap2 = vtkMatrixUtilities::Wrapper<3, 3, MatrixT2>;

    Scalar detInv = 1.0 /
      (Wrap1::template Get<0, 0>(std::forward<MatrixT1>(M1)) *
          (Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1)) *
              Wrap1::template Get<2, 2>(std::forward<MatrixT1>(M1)) -
            Wrap1::template Get<2, 1>(std::forward<MatrixT1>(M1)) *
              Wrap1::template Get<1, 2>(std::forward<MatrixT1>(M1))) -
        Wrap1::template Get<0, 1>(std::forward<MatrixT1>(M1)) *
          (Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1)) *
              Wrap1::template Get<2, 2>(std::forward<MatrixT1>(M1)) -
            Wrap1::template Get<2, 0>(std::forward<MatrixT1>(M1)) *
              Wrap1::template Get<1, 2>(std::forward<MatrixT1>(M1))) +
        Wrap1::template Get<0, 2>(std::forward<MatrixT1>(M1)) *
          (Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1)) *
              Wrap1::template Get<2, 1>(std::forward<MatrixT1>(M1)) -
            Wrap1::template Get<2, 0>(std::forward<MatrixT1>(M1)) *
              Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1))));

    Wrap2::template Get<0, 0>(std::forward<MatrixT2>(M2)) = detInv *
      (Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<2, 2>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<2, 1>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<1, 2>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<1, 0>(std::forward<MatrixT2>(M2)) = -detInv *
      (Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<2, 2>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<2, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<1, 2>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<2, 0>(std::forward<MatrixT2>(M2)) = detInv *
      (Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<2, 1>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<2, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<0, 1>(std::forward<MatrixT2>(M2)) = -detInv *
      (Wrap1::template Get<0, 1>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<2, 2>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<2, 1>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<0, 2>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<1, 1>(std::forward<MatrixT2>(M2)) = detInv *
      (Wrap1::template Get<0, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<2, 2>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<2, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<0, 2>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<2, 1>(std::forward<MatrixT2>(M2)) = -detInv *
      (Wrap1::template Get<0, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<2, 1>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<2, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<0, 1>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<0, 2>(std::forward<MatrixT2>(M2)) = detInv *
      (Wrap1::template Get<0, 1>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<1, 2>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<0, 2>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<1, 2>(std::forward<MatrixT2>(M2)) = -detInv *
      (Wrap1::template Get<0, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<1, 2>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<0, 2>(std::forward<MatrixT1>(M1)));
    Wrap2::template Get<2, 2>(std::forward<MatrixT2>(M2)) = detInv *
      (Wrap1::template Get<0, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<1, 1>(std::forward<MatrixT1>(M1)) -
        Wrap1::template Get<1, 0>(std::forward<MatrixT1>(M1)) *
          Wrap1::template Get<0, 1>(std::forward<MatrixT1>(M1)));
  }
};

//=============================================================================
// Specialization for 1x1 matrices
template <class LayoutT>
class InvertMatrix<1, LayoutT>
{
public:
  template <class MatrixT1, class MatrixT2>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    M2[0] = 1.0 / M1[0];
  }
};

//=============================================================================
// Specialization for 2x2 diagonal matrices
template <>
class InvertMatrix<2, vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT1, class MatrixT2>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    M2[0] = 1.0 / M1[0];
    M2[1] = 1.0 / M1[1];
  }
};

//=============================================================================
// Specialization for 3x3 diagonal matrices
template <>
class InvertMatrix<3, vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT1, class MatrixT2>
  static void Compute(MatrixT1&& M1, MatrixT2&& M2)
  {
    M2[0] = 1.0 / M1[0];
    M2[1] = 1.0 / M1[1];
    M2[2] = 1.0 / M1[2];
  }
};

//=============================================================================
// Class solving systems M*y = x for square matrices RowsT x ColsT.
// The template parameter LayoutT is a struct embedded in MatrixLayout
// This class is currently specialized for 1x1, 2x2 and 3x3 matrices
template <int RowsT, int ColsT, class LayoutT = vtkMatrixUtilities::Layout::Identity>
class LinearSolve;

//=============================================================================
// Specialization for 1x1 matrices
template <class LayoutT>
class LinearSolve<1, 1, LayoutT>
{
public:
  template <class MatrixT, class VectorT1, class VectorT2>
  static void Compute(MatrixT&& M, VectorT1&& x, VectorT2&& y)
  {
    y[0] = x[0] / M[0];
  }
};

//=============================================================================
// Specialization for 2x2 matrices
template <class LayoutT>
class LinearSolve<2, 2, LayoutT>
{
public:
  template <class MatrixT, class VectorT1, class VectorT2>
  static void Compute(MatrixT&& M, VectorT1&& x, VectorT2&& y)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type;
    using Wrap = vtkMatrixUtilities::Wrapper<2, 2, MatrixT, LayoutT>;

    Scalar detInv = 1.0 / Determinant<2, LayoutT>::Compute(std::forward<MatrixT>(M));
    y[0] = (x[0] * Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) -
             x[1] * Wrap::template Get<0, 1>(std::forward<MatrixT>(M))) *
      detInv;
    y[1] = (-x[0] * Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) +
             x[1] * Wrap::template Get<0, 0>(std::forward<MatrixT>(M))) *
      detInv;
  }
};

//=============================================================================
// Specialization for 3x3 matrices
template <class LayoutT>
class LinearSolve<3, 3, LayoutT>
{
public:
  template <class MatrixT, class VectorT1, class VectorT2>
  static void Compute(MatrixT&& M, VectorT1&& x, VectorT2&& y)
  {
    using Scalar = typename vtkMatrixUtilities::ScalarTypeExtractor<MatrixT>::value_type;
    using Wrap = vtkMatrixUtilities::Wrapper<3, 3, MatrixT, LayoutT>;

    Scalar detInv = 1.0 /
      (Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
          (Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 2>(std::forward<MatrixT>(M))) -
        Wrap::template Get<0, 1>(std::forward<MatrixT>(M)) *
          (Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 2>(std::forward<MatrixT>(M))) +
        Wrap::template Get<0, 2>(std::forward<MatrixT>(M)) *
          (Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 1>(std::forward<MatrixT>(M))));

    y[0] = detInv *
      (x[0] *
          (Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 2>(std::forward<MatrixT>(M))) -
        x[1] *
          (Wrap::template Get<0, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<0, 2>(std::forward<MatrixT>(M))) +
        x[2] *
          (Wrap::template Get<0, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) *
              Wrap::template Get<0, 2>(std::forward<MatrixT>(M))));
    y[1] = detInv *
      (-x[0] *
          (Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 2>(std::forward<MatrixT>(M))) +
        x[1] *
          (Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<0, 2>(std::forward<MatrixT>(M))) -
        x[2] *
          (Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 2>(std::forward<MatrixT>(M)) -
            Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<0, 2>(std::forward<MatrixT>(M))));
    y[2] = detInv *
      (x[0] *
          (Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 1>(std::forward<MatrixT>(M))) -
        x[1] *
          (Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<2, 1>(std::forward<MatrixT>(M)) -
            Wrap::template Get<2, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<0, 1>(std::forward<MatrixT>(M))) +
        x[2] *
          (Wrap::template Get<0, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<1, 1>(std::forward<MatrixT>(M)) -
            Wrap::template Get<1, 0>(std::forward<MatrixT>(M)) *
              Wrap::template Get<0, 1>(std::forward<MatrixT>(M))));
  }
};

//=============================================================================
// Specialization for 2x2 diagonal matrices
template <>
class LinearSolve<2, 2, vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT, class VectorT1, class VectorT2>
  static void Compute(MatrixT&& M, VectorT1&& x, VectorT2&& y)
  {
    y[0] = x[0] / M[0];
    y[1] = x[1] / M[1];
  }
};

//=============================================================================
// Specialization for 3x3 diagonal matrices
template <>
class LinearSolve<3, 3, vtkMatrixUtilities::Layout::Diag>
{
public:
  template <class MatrixT, class VectorT1, class VectorT2>
  static void Compute(MatrixT&& M, VectorT1&& x, VectorT2&& y)
  {
    y[0] = x[0] / M[0];
    y[1] = x[1] / M[1];
    y[2] = x[2] / M[2];
  }
};
VTK_ABI_NAMESPACE_END
} // namespace vtkMathPrivate
#endif
