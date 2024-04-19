// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright @ 2002 US Army Research Laboratory
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-4-Clause-Modif
#ifndef vtkXdmfDataArray_h
#define vtkXdmfDataArray_h

#include "vtkIOXdmf2Module.h" // For export macro
#include "vtkObject.h"

namespace xdmf2
{
class XdmfArray;
}

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;

class VTKIOXDMF2_EXPORT vtkXdmfDataArray : public vtkObject
{
public:
  static vtkXdmfDataArray* New();
  vtkTypeMacro(vtkXdmfDataArray, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkDataArray* FromArray();

  char* ToArray();

  vtkDataArray* FromXdmfArray(char* ArrayName = nullptr, int CopyShape = 1, int rank = 1,
    int Components = 1, int MakeCopy = 1);

  char* ToXdmfArray(vtkDataArray* DataArray = nullptr, int CopyShape = 1);

  void SetArray(char* TagName);

  char* GetArray();

  void SetVtkArray(vtkDataArray* array);

  vtkDataArray* GetVtkArray();

protected:
  vtkXdmfDataArray();

private:
  vtkDataArray* vtkArray;
  xdmf2::XdmfArray* Array;
  vtkXdmfDataArray(const vtkXdmfDataArray&) = delete;
  void operator=(const vtkXdmfDataArray&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkXdmfDataArray_h */
