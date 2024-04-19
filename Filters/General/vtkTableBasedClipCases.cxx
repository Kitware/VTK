// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTableBasedClipCases.h"

VTK_ABI_NAMESPACE_BEGIN
template <bool TInsideOut>
constexpr bool vtkTableBasedClipCases<TInsideOut>::SupportedCellTypes[VTK_NUMBER_OF_CELL_TYPES];

template <bool TInsideOut>
constexpr uint8_t vtkTableBasedClipCases<TInsideOut>::CellMaxCase[9];

template <bool TInsideOut>
constexpr uint8_t vtkTableBasedClipCases<TInsideOut>::CellEdges[NUM_CELL_TYPES][MAX_NUM_EDGES][2];

template <bool TInsideOut>
constexpr uint8_t vtkTableBasedClipCases<TInsideOut>::CellCases[];

template <bool TInsideOut>
constexpr uint16_t vtkTableBasedClipCases<TInsideOut>::StartCellCases[];

template <bool TInsideOut>
constexpr uint8_t vtkTableBasedClipCases<TInsideOut>::CellCasesInsideOut[];

template <bool TInsideOut>
constexpr uint16_t vtkTableBasedClipCases<TInsideOut>::StartCellCasesInsideOut[];

template <bool TInsideOut>
constexpr int16_t vtkTableBasedClipCases<TInsideOut>::CellCasesStartIndexLookUp[NUM_CELL_TYPES];

// Explicit instantiation
template class VTKFILTERSGENERAL_EXPORT vtkTableBasedClipCases<false>;
template class VTKFILTERSGENERAL_EXPORT vtkTableBasedClipCases<true>;
VTK_ABI_NAMESPACE_END
