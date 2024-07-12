// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTableBasedClipCases.h"

VTK_ABI_NAMESPACE_BEGIN
constexpr bool vtkTableBasedClipCasesBase::SupportedCellTypes[VTK_NUMBER_OF_CELL_TYPES];

constexpr uint8_t vtkTableBasedClipCasesBase::CellMaxCase[9];

constexpr uint8_t vtkTableBasedClipCasesBase::CellEdges[NUM_CELL_TYPES][MAX_NUM_EDGES][2];

constexpr int16_t vtkTableBasedClipCasesBase::CellCasesStartIndexLookUp[NUM_CELL_TYPES];

constexpr std::array<uint8_t, 26665> vtkTableBasedClipCases<false>::CellCases;

constexpr std::array<uint16_t, 670> vtkTableBasedClipCases<false>::StartCellCases;

constexpr std::array<uint8_t, 23879> vtkTableBasedClipCases<true>::CellCases;

constexpr std::array<uint16_t, 670> vtkTableBasedClipCases<true>::StartCellCases;
VTK_ABI_NAMESPACE_END
