// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIndent.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkIndent* vtkIndent::New()
{
  return new vtkIndent; // not a VTK object, don't use object factory macros
}

constexpr int VTK_STD_INDENT = 2;
constexpr int VTK_NUMBER_OF_BLANKS = 40;

static const char blanks[VTK_NUMBER_OF_BLANKS + 1] = "                                        ";

// Determine the next indentation level. Keep indenting by two until the
// max of forty.
vtkIndent vtkIndent::GetNextIndent()
{
  int indent = this->Indent + VTK_STD_INDENT;
  if (indent > VTK_NUMBER_OF_BLANKS)
  {
    indent = VTK_NUMBER_OF_BLANKS;
  }
  return vtkIndent(indent);
}

// Print out the indentation. Basically output a bunch of spaces.
ostream& operator<<(ostream& os, const vtkIndent& ind)
{
  os << blanks + (VTK_NUMBER_OF_BLANKS - ind.Indent);
  return os;
}
VTK_ABI_NAMESPACE_END
