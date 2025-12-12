// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCompositer.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCompositer);

//------------------------------------------------------------------------------
vtkCompositer::vtkCompositer()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->NumberOfProcesses = 1;
  if (this->Controller)
  {
    this->Controller->Register(this);
    this->NumberOfProcesses = this->Controller->GetNumberOfProcesses();
  }
}

//------------------------------------------------------------------------------
vtkCompositer::~vtkCompositer()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkCompositer::SetController(vtkMultiProcessController* mpc)
{
  vtkSetObjectBodyMacro(Controller, vtkMultiProcessController, mpc);

  if (mpc)
  {
    this->NumberOfProcesses = mpc->GetNumberOfProcesses();
  }
}

//------------------------------------------------------------------------------
void vtkCompositer::CompositeBuffer(
  vtkDataArray* pBuf, vtkFloatArray* zBuf, vtkDataArray* pTmp, vtkFloatArray* zTmp)
{
  (void)pBuf;
  (void)zBuf;
  (void)pTmp;
  (void)zTmp;
}

//------------------------------------------------------------------------------
void vtkCompositer::ResizeFloatArray(vtkFloatArray* fa, int numComp, vtkIdType size)
{
  fa->SetNumberOfComponents(numComp);
  fa->SetNumberOfTuples(size);
}

void vtkCompositer::ResizeUnsignedCharArray(vtkUnsignedCharArray* uca, int numComp, vtkIdType size)
{
  uca->SetNumberOfComponents(numComp);
  uca->SetNumberOfTuples(size);
}

void vtkCompositer::DeleteArray(vtkDataArray* da)
{
  da->Delete();
}

//------------------------------------------------------------------------------
void vtkCompositer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}
VTK_ABI_NAMESPACE_END
