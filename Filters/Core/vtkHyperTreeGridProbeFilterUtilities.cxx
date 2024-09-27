// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridProbeFilterUtilities.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkStringArray.h"
#include "vtkType.h"

namespace vtkHyperTreeGridProbeFilterUtilities
{
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void FillDefaultArray(vtkAbstractArray* array)
{
  // This function could be improved with array dispatch
  vtkStringArray* strArray = vtkStringArray::SafeDownCast(array);
  vtkDataArray* dataArray = vtkArrayDownCast<vtkDataArray>(array);

  if (strArray)
  {
    vtkSMPTools::For(0, strArray->GetNumberOfValues(),
      [strArray](vtkIdType start, vtkIdType end)
      {
        for (vtkIdType i = start; i < end; ++i)
        {
          strArray->SetValue(i, "");
        }
      });
  }
  else if (dataArray)
  {
    // Use type comparison to consider both SOA & AOS memory layouts
    if (dataArray->GetDataType() == VTK_FLOAT || dataArray->GetDataType() == VTK_DOUBLE)
    {
      dataArray->Fill(vtkMath::Nan());
    }
    else
    {
      dataArray->Fill(0);
    }
  }
  else
  {
    vtkGenericWarningMacro("Array is not a vtkDataArray nor is it a vtkStringArray and will not be "
                           "filled with default values.");
  }
}

VTK_ABI_NAMESPACE_END
} // end of namespace.
