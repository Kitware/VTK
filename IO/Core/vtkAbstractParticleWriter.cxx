// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractParticleWriter.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Construct with no start and end write methods or arguments.
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractParticleWriter::vtkAbstractParticleWriter()
{
  this->TimeStep = 0;
  this->TimeValue = 0.0;
  this->FileName = nullptr;
  this->CollectiveIO = 0;
}
//------------------------------------------------------------------------------
vtkAbstractParticleWriter::~vtkAbstractParticleWriter()
{
  delete[] this->FileName;
  this->FileName = nullptr;
}
//------------------------------------------------------------------------------
void vtkAbstractParticleWriter::SetWriteModeToCollective()
{
  this->SetCollectiveIO(1);
}
//------------------------------------------------------------------------------
void vtkAbstractParticleWriter::SetWriteModeToIndependent()
{
  this->SetCollectiveIO(0);
}
//------------------------------------------------------------------------------
void vtkAbstractParticleWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "CollectiveIO: " << this->CollectiveIO << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NONE") << endl;
}
VTK_ABI_NAMESPACE_END
