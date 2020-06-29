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
 * vtkMathPrivate provides meta-classes helpers that are used in some vtkMath
 * methods.
 * @sa
 * vtkMath
 */

#ifndef vtkMathPrivate_hxx
#define vtkMathPrivate_hxx

#include "vtkCommonCoreModule.h" //required for correct implementation

#include <type_traits>

namespace vtkMathPrivate
{
//=============================================================================
// Private class to extract the value type of both standard arrays
// and containers having a value_type typedef.
template <int ContainerTypeT, class ContainerT>
struct ScalarTypeExtractorImpl
{
  typedef typename ContainerT::value_type value_type;
};

//=============================================================================
template <class ContainerT>
struct ScalarTypeExtractorImpl<1, ContainerT>
{
  typedef typename std::remove_all_extents<ContainerT>::type value_type;
};

//=============================================================================
template <class ContainerT>
struct ScalarTypeExtractorImpl<2, ContainerT>
{
  typedef typename std::remove_pointer<ContainerT>::type value_type;
};

//=============================================================================
template <class ContainerT>
struct ScalarTypeExtractor
{
  typedef typename ScalarTypeExtractorImpl<
    std::is_array<ContainerT>::value ? 1 : (std::is_pointer<ContainerT>::value ? 2 : 0),
    ContainerT>::value_type value_type;
};

static constexpr int VTK_MATH_PRIVATE_PACK_SIZE = 4;

//=============================================================================
// This struct determines a prior transform to input matrices, chaging the
// way they are indexed
struct MatrixLayout
{
  struct Identity;  // Input matrix is unchanged
  struct Transpose; // Input matrix is transposed when calculating
  struct Diag;      // Input matrix is considered diagonal, and value at index idx points
                    // to component of coordinates (idx, idx) in the diagonal matrix.
};

//=============================================================================
// This class computes the dot product between row RowT of matrix M1
// and column ColT of matrix M2.
// The template parameter IdxT is on index ahead from computation.
// Template parameters LayoutT1 and LayoutT2 respectively reindex
// input matrices M1 and M2 according to MatrixLayout enumeration
// M1 (or M1^T if LayoutT1 == MatrixLayout::Transpose) is a matrix of RowsT   x MidDimT
// M2 (or M2^T if LayoutT2 == MatrixLayout::Transpose) is a matrix of MidDimT x ColsDimT
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT,
  class LayoutT1 = MatrixLayout::Identity, class LayoutT2 = MatrixLayout::Identity, int IdxT = 0,
  int PackSizeT = MidDimT>
class ContractRowWithCol
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT,
             Shift>::Compute(M1, M2) +
      ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
        IdxT + Shift, PackSizeT - Shift>::Compute(M1, M2);
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
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[IndicesM1_0] * M2[IndicesM2_0] + M1[IndicesM1_1] * M2[IndicesM2_1] +
      M1[IndicesM1_2] * M2[IndicesM2_2] + M1[IndicesM1_3] * M2[IndicesM2_3];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool Transpose1 = std::is_same<LayoutT1, MatrixLayout::Transpose>::value;
  static constexpr bool Transpose2 = std::is_same<LayoutT2, MatrixLayout::Transpose>::value;

  static constexpr int IndicesM1_0 = Transpose1 ? RowT + RowsT * IdxT : MidDimT * RowT + IdxT;
  static constexpr int IndicesM1_1 =
    Transpose1 ? RowT + RowsT * (IdxT + 1) : MidDimT * RowT + IdxT + 1;
  static constexpr int IndicesM1_2 =
    Transpose1 ? RowT + RowsT * (IdxT + 2) : MidDimT * RowT + IdxT + 2;
  static constexpr int IndicesM1_3 =
    Transpose1 ? RowT + RowsT * (IdxT + 3) : MidDimT * RowT + IdxT + 3;

  static constexpr int IndicesM2_0 = Transpose2 ? MidDimT * ColT + IdxT : ColT + ColsT * IdxT;
  static constexpr int IndicesM2_1 =
    Transpose2 ? MidDimT * ColT + IdxT + 1 : ColT + ColsT * (IdxT + 1);
  static constexpr int IndicesM2_2 =
    Transpose2 ? MidDimT * ColT + IdxT + 2 : ColT + ColsT * (IdxT + 2);
  static constexpr int IndicesM2_3 =
    Transpose2 ? MidDimT * ColT + IdxT + 3 : ColT + ColsT * (IdxT + 3);
};

//=============================================================================
// Specialization for when there are 3 components left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 3>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[IndicesM1_0] * M2[IndicesM2_0] + M1[IndicesM1_1] * M2[IndicesM2_1] +
      M1[IndicesM1_2] * M2[IndicesM2_2];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool Transpose1 = std::is_same<LayoutT1, MatrixLayout::Transpose>::value;
  static constexpr bool Transpose2 = std::is_same<LayoutT2, MatrixLayout::Transpose>::value;

  static constexpr int IndicesM1_0 = Transpose1 ? RowT + RowsT * IdxT : MidDimT * RowT + IdxT;
  static constexpr int IndicesM1_1 =
    Transpose1 ? RowT + RowsT * (IdxT + 1) : MidDimT * RowT + IdxT + 1;
  static constexpr int IndicesM1_2 =
    Transpose1 ? RowT + RowsT * (IdxT + 2) : MidDimT * RowT + IdxT + 2;

  static constexpr int IndicesM2_0 = Transpose2 ? MidDimT * ColT + IdxT : ColT + ColsT * IdxT;
  static constexpr int IndicesM2_1 =
    Transpose2 ? MidDimT * ColT + IdxT + 1 : ColT + ColsT * (IdxT + 1);
  static constexpr int IndicesM2_2 =
    Transpose2 ? MidDimT * ColT + IdxT + 2 : ColT + ColsT * (IdxT + 2);
};

//=============================================================================
// Specialization for when there are 2 components left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 2>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[IndicesM1_0] * M2[IndicesM2_0] + M1[IndicesM1_1] * M2[IndicesM2_1];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool Transpose1 = std::is_same<LayoutT1, MatrixLayout::Transpose>::value;
  static constexpr bool Transpose2 = std::is_same<LayoutT2, MatrixLayout::Transpose>::value;

  static constexpr int IndicesM1_0 = Transpose1 ? RowT + RowsT * IdxT : MidDimT * RowT + IdxT;
  static constexpr int IndicesM1_1 =
    Transpose1 ? RowT + RowsT * (IdxT + 1) : MidDimT * RowT + IdxT + 1;

  static constexpr int IndicesM2_0 = Transpose2 ? MidDimT * ColT + IdxT : ColT + ColsT * IdxT;
  static constexpr int IndicesM2_1 =
    Transpose2 ? MidDimT * ColT + IdxT + 1 : ColT + ColsT * (IdxT + 1);
};

//=============================================================================
// Specialization for when there is 1 component left to compute
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, int IdxT>
class ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2, IdxT, 1>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[IdxM1] * M2[IdxM2];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool Transpose1 = std::is_same<LayoutT1, MatrixLayout::Transpose>::value;
  static constexpr bool Transpose2 = std::is_same<LayoutT2, MatrixLayout::Transpose>::value;

  static constexpr int IdxM1 = Transpose1 ? RowT + RowsT * IdxT : MidDimT * RowT + IdxT;
  static constexpr int IdxM2 = Transpose2 ? MidDimT * ColT + IdxT : ColT + ColsT * IdxT;
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
class DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, MatrixLayout::Diag,
  LayoutT2>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[RowT] * M2[IdxM2];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool Transpose2 = std::is_same<LayoutT2, MatrixLayout::Transpose>::value;
  static constexpr int IdxM2 = Transpose2 ? MidDimT * ColT + RowT : ColT + ColsT * RowT;
};

//=============================================================================
// Specialization for when M2 is diagonal
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1>
class DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1,
  MatrixLayout::Diag>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[IdxM1] * M2[ColT];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT >= 0 && ColT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool Transpose1 = std::is_same<LayoutT1, MatrixLayout::Transpose>::value;
  static constexpr int IdxM1 = Transpose1 ? RowT + RowsT * ColT : MidDimT * RowT + ColT;
};

//=============================================================================
// Specialization for when both input matrices are diagonal
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT>
class DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, RowT, MatrixLayout::Diag,
  MatrixLayout::Diag>
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1& M1, const MatrixT2& M2)
  {
    return M1[RowT] * M2[RowT];
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT >= 0 && RowT < RowsT;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = RowT >= 0 && RowT < ColsT;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");
};

//=============================================================================
// This class returns zero. Is is used for rectangular diagonal matrices, when
// lines / rows are filled with zeros.
template <class ScalarT>
class NullContractRowWithCol
{
public:
  template <class MatrixT1, class MatrixT2>
  static ScalarT Compute(const MatrixT1&, const MatrixT2&)
  {
    return ScalarT(0);
  }
};

//=============================================================================
// Helper class to choose between regular contraction class vs diagonal
// handling contraction class.
// By default, Type is an instance of ContractRowWithCol
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2, bool HasAtLeatOneDiagonalMatrixT = false>
struct ContractRowWithColSwitch
{
  typedef ContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2> Type;

  static constexpr bool NO_INPUT_MATRIX_CAN_BE_DIAGONAL =
    !std::is_same<LayoutT1, MatrixLayout::Diag>::value &&
    !std::is_same<LayoutT2, MatrixLayout::Diag>::value;
  static_assert(NO_INPUT_MATRIX_CAN_BE_DIAGONAL,
    "LayoutT1 and LayoutT2 cannot equal MatrixLayout::Diag in this setup");
};

//=============================================================================
// Specialization for diagonal matrices.
// Rectangular diagonal matrices are handled using NullContractRowWithCol.
template <class ScalarT, int RowsT, int MidDimT, int ColsT, int RowT, int ColT, class LayoutT1,
  class LayoutT2>
struct ContractRowWithColSwitch<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2,
  true>
{
  static constexpr bool IsDiagonal1 = std::is_same<LayoutT1, MatrixLayout::Diag>::value;
  static constexpr bool IsDiagonal2 = std::is_same<LayoutT2, MatrixLayout::Diag>::value;

  // If on of the diagonal matrix is rectangular and we are out of the diagonal
  // length, all remaning values are null.
  static constexpr bool UseNullContractRowWithCol =
    (IsDiagonal1 && RowT >= MidDimT) || (IsDiagonal2 && ColT >= MidDimT);

  typedef typename std::conditional<UseNullContractRowWithCol, NullContractRowWithCol<ScalarT>,
    DiagContractRowWithCol<ScalarT, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1, LayoutT2>>::type
    Type;

  static constexpr bool NEEDS_AT_LEAST_ONE_DIAGONAL_INPUT_MATRIX = IsDiagonal1 || IsDiagonal2;
  static_assert(NEEDS_AT_LEAST_ONE_DIAGONAL_INPUT_MATRIX,
    "LayoutT1 or LayoutT2 must equal MatrixLayout::Diag in this setup");
};

//=============================================================================
// Class in charge for actually multiplying 2 matrices.
// This method is called by MultiplyMatrix::Compute for chunks of size
// VTK_MATH_PRIVATE_PACK_SIZE at most, specified in ColPackSizeT.
// This class mostly consists on explicitly onfold computation for those chunks.
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT,
  int ColPackSizeT>
class MultiplyMatrixImpl;

//=============================================================================
// Specialization for a chunk of size 4
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrixImpl<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 4>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)
  {
    using Scalar = typename ScalarTypeExtractor<MatrixT3>::value_type;

    M3[OutputIndices_0] = ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT,
      LayoutT1, LayoutT2, AtLeastOneMatrixIsDiagonal>::Type::Compute(M1, M2);
    M3[OutputIndices_1] = ContractRowWithColSwitch < Scalar, RowsT, MidDimT, ColsT,
    BothMatricesAreDiagonal ? RowT + 1 : RowT, ColT + 1, LayoutT1, LayoutT2,
    AtLeastOneMatrixIsDiagonal > ::Type::Compute(M1, M2);
    M3[OutputIndices_2] = ContractRowWithColSwitch < Scalar, RowsT, MidDimT, ColsT,
    BothMatricesAreDiagonal ? RowT + 2 : RowT, ColT + 2, LayoutT1, LayoutT2,
    AtLeastOneMatrixIsDiagonal > ::Type::Compute(M1, M2);
    M3[OutputIndices_3] = ContractRowWithColSwitch < Scalar, RowsT, MidDimT, ColsT,
    BothMatricesAreDiagonal ? RowT + 3 : RowT, ColT + 3, LayoutT1, LayoutT2,
    AtLeastOneMatrixIsDiagonal > ::Type::Compute(M1, M2);
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT + 3 < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool AtLeastOneMatrixIsDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value ||
    std::is_same<LayoutT2, MatrixLayout::Diag>::value;

  static constexpr bool BothMatricesAreDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value &&
    std::is_same<LayoutT2, MatrixLayout::Diag>::value;
  static constexpr bool ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL =
    !BothMatricesAreDiagonal || ColT == RowT;
  static_assert(ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL, "RowT must equal ColT");

  static constexpr int OutputIndices_0 = BothMatricesAreDiagonal ? RowT : ColT + RowT * ColsT;
  static constexpr int OutputIndices_1 =
    BothMatricesAreDiagonal ? RowT + 1 : ColT + 1 + RowT * ColsT;
  static constexpr int OutputIndices_2 =
    BothMatricesAreDiagonal ? RowT + 2 : ColT + 2 + RowT * ColsT;
  static constexpr int OutputIndices_3 =
    BothMatricesAreDiagonal ? RowT + 3 : ColT + 3 + RowT * ColsT;
};

//=============================================================================
// Specialization for a chunk of size 3
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrixImpl<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 3>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)
  {
    using Scalar = typename ScalarTypeExtractor<MatrixT3>::value_type;

    M3[OutputIndices_0] = ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT,
      LayoutT1, LayoutT2, AtLeastOneMatrixIsDiagonal>::Type::Compute(M1, M2);
    M3[OutputIndices_1] = ContractRowWithColSwitch < Scalar, RowsT, MidDimT, ColsT,
    BothMatricesAreDiagonal ? RowT + 1 : RowT, ColT + 1, LayoutT1, LayoutT2,
    AtLeastOneMatrixIsDiagonal > ::Type::Compute(M1, M2);
    M3[OutputIndices_2] = ContractRowWithColSwitch < Scalar, RowsT, MidDimT, ColsT,
    BothMatricesAreDiagonal ? RowT + 2 : RowT, ColT + 2, LayoutT1, LayoutT2,
    AtLeastOneMatrixIsDiagonal > ::Type::Compute(M1, M2);
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT + 2 < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool AtLeastOneMatrixIsDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value ||
    std::is_same<LayoutT2, MatrixLayout::Diag>::value;

  static constexpr bool BothMatricesAreDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value && std::is_same<LayoutT1, LayoutT2>::value;
  static constexpr bool ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL =
    !BothMatricesAreDiagonal || ColT == RowT;
  static_assert(ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL, "RowT must equal ColT");

  static constexpr int OutputIndices_0 = BothMatricesAreDiagonal ? RowT : ColT + RowT * ColsT;
  static constexpr int OutputIndices_1 =
    BothMatricesAreDiagonal ? RowT + 1 : ColT + 1 + RowT * ColsT;
  static constexpr int OutputIndices_2 =
    BothMatricesAreDiagonal ? RowT + 2 : ColT + 2 + RowT * ColsT;
};

//=============================================================================
// Specialization for a chunk of size 2
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrixImpl<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 2>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)
  {
    using Scalar = typename ScalarTypeExtractor<MatrixT3>::value_type;

    M3[OutputIndices_0] = ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT,
      LayoutT1, LayoutT2, AtLeastOneMatrixIsDiagonal>::Type::Compute(M1, M2);
    M3[OutputIndices_1] = ContractRowWithColSwitch < Scalar, RowsT, MidDimT, ColsT,
    BothMatricesAreDiagonal ? RowT + 1 : RowT, ColT + 1, LayoutT1, LayoutT2,
    AtLeastOneMatrixIsDiagonal > ::Type::Compute(M1, M2);
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT + 1 < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool AtLeastOneMatrixIsDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value ||
    std::is_same<LayoutT2, MatrixLayout::Diag>::value;

  static constexpr bool BothMatricesAreDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value && std::is_same<LayoutT1, LayoutT2>::value;
  static constexpr bool ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL =
    !BothMatricesAreDiagonal || ColT == RowT;
  static_assert(ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL, "RowT must equal ColT");

  static constexpr int OutputIndices_0 = BothMatricesAreDiagonal ? RowT : ColT + RowT * ColsT;
  static constexpr int OutputIndices_1 =
    BothMatricesAreDiagonal ? RowT + 1 : ColT + 1 + RowT * ColsT;
};

//=============================================================================
// Specialization for a chunk of size 1
template <int RowsT, int MidDimT, int ColsT, class LayoutT1, class LayoutT2, int RowT, int ColT>
class MultiplyMatrixImpl<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowT, ColT, 1>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)
  {
    using Scalar = typename ScalarTypeExtractor<MatrixT3>::value_type;

    M3[OutputIdx] = ContractRowWithColSwitch<Scalar, RowsT, MidDimT, ColsT, RowT, ColT, LayoutT1,
      LayoutT2, AtLeastOneMatrixIsDiagonal>::Type::Compute(M1, M2);
  }

private:
  static constexpr bool ROW_OUT_OF_BOUNDS = RowT < RowsT && RowT >= 0;
  static_assert(ROW_OUT_OF_BOUNDS, "RowT is out of bounds");
  static constexpr bool COL_OUT_OF_BOUNDS = ColT < ColsT && ColT >= 0;
  static_assert(COL_OUT_OF_BOUNDS, "ColT is out of bounds");

  static constexpr bool AtLeastOneMatrixIsDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value ||
    std::is_same<LayoutT2, MatrixLayout::Diag>::value;

  static constexpr bool BothMatricesAreDiagonal =
    std::is_same<LayoutT1, MatrixLayout::Diag>::value &&
    std::is_same<LayoutT2, MatrixLayout::Diag>::value;
  static constexpr bool ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL =
    !BothMatricesAreDiagonal || ColT == RowT;
  static_assert(ROWT_AND_COLT_MUST_MATCH_WHEN_BOTH_MATRICES_ARE_DIAGONAL, "RowT must equal ColT");

  static constexpr int OutputIdx = BothMatricesAreDiagonal ? RowT : ColT + RowT * ColsT;
};

//=============================================================================
// Multiply matrices such that M3 = M1 * M2.
// Template parameters LayoutT1 and LayoutT2 respectively reindex
// input matrices M1 and M2 following MatrixLayout options
// Hence, if LayoutT1 == MatrixLayout::Transpose, then M3 = M1^T * M2, and so on.
// M1 (or M1^T if LayoutT1 == MatrixLayout::Transpose) is a matrix of RowsT   x MidDimT
// M2 (or M2^T if LayoutT2 == MatrixLayout::Transpose) is a matrix of MidDimT x ColsDimT
//
// RemainingRowSizeT should be disregarded when first instantiating this class. It is
// the number of remaning elements in the current row to process.
//
// To compute the multiplication, each component of the output matrix
// is computed chunk by chunk (of size VTK_MATH_PRIVATE_PACK_SIZE),
// starting at the top left, sweeping the rows one by one.
template <int RowsT, int MidDimT, int ColsT, class LayoutT1 = MatrixLayout::Identity,
  class LayoutT2 = MatrixLayout::Identity, int NextRowT = 1, int NextColT = 1,
  int RemainingRowSizeT = ColsT>
class MultiplyMatrix
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)
  {
    MultiplyMatrixImpl<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, Row, Col, Shift>::Compute(
      M1, M2, M3);
    MultiplyMatrix<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, ForwardRow, ForwardCol,
      ForwardRemainingRowSize>::Compute(M1, M2, M3);
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
class MultiplyMatrix<RowsT, MidDimT, ColsT, MatrixLayout::Diag, MatrixLayout::Diag, NextIdxT,
  NextIdxT, RemainingRowSizeT>
{
public:
  template <class MatrixT1, class MatrixT2, class MatrixT3>
  static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)
  {
    MultiplyMatrixImpl<RowsT, MidDimT, ColsT, MatrixLayout::Diag, MatrixLayout::Diag, NextIdxT - 1,
      NextIdxT - 1, Shift>::Compute(M1, M2, M3);
    MultiplyMatrix<RowsT, MidDimT, ColsT, MatrixLayout::Diag, MatrixLayout::Diag, NextIdxT + Shift,
      NextIdxT + Shift, RemainingRowSizeT - Shift>::Compute(M1, M2, M3);
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
  class MultiplyMatrix<RowsT, MidDimT, ColsT, MatrixLayout::Diag, MatrixLayout::Diag, NextIdxT,    \
    NextIdxT, RemainingRowSize>                                                                    \
  {                                                                                                \
  public:                                                                                          \
    template <class MatrixT1, class MatrixT2, class MatrixT3>                                      \
    static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)                      \
    {                                                                                              \
      MultiplyMatrixImpl<RowsT, MidDimT, ColsT, MatrixLayout::Diag, MatrixLayout::Diag,            \
        NextIdxT - 1, NextIdxT - 1, RemainingRowSize>::Compute(M1, M2, M3);                        \
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
    static void Compute(const MatrixT1& M1, const MatrixT2& M2, MatrixT3& M3)                      \
    {                                                                                              \
      MultiplyMatrixImpl<RowsT, MidDimT, ColsT, LayoutT1, LayoutT2, RowsT - 1, NextColT - 1,       \
        RemainingRowSize>::Compute(M1, M2, M3);                                                    \
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
} // namespace vtkMathPrivate
#endif
