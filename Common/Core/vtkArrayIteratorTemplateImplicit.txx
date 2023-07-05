// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This file is used to stop dependency tracking from including a
// dependence on the .txx file from the .h file when implicit
// instantiation is not needed.  It just includes the corresponding
// .txx file, so only the name is important.

#ifndef vtkArrayIteratorTemplateImplicit_txx
#define vtkArrayIteratorTemplateImplicit_txx

#include "vtkArrayIteratorTemplate.txx"

#endif
