// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStdString.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
ostream& operator<<(ostream& os, const vtkStdString& s)
{
  return os << s.c_str();
}
VTK_ABI_NAMESPACE_END
