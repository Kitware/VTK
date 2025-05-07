// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAvmeshReader.h"
#include "AvmeshInternals.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAvmeshReader);

vtkAvmeshReader::vtkAvmeshReader()
  : SurfaceOnly(false)
  , BuildConnectivityIteratively(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkAvmeshReader::~vtkAvmeshReader() = default;

int vtkAvmeshReader::CanReadFile(VTK_FILEPATH const char* filename)
{
  return vtksys::SystemTools::TestFileAccess(filename, vtksys::TEST_FILE_READ);
}

int vtkAvmeshReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  try
  {
    ReadAvmesh(output, this->FileName, this->SurfaceOnly, this->BuildConnectivityIteratively);
  }
  catch (AvmeshError const& ex)
  {
    vtkErrorMacro(<< ex.what());
    return 0;
  }

  return 1;
}

void vtkAvmeshReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << this->FileName << "\n";
  os << indent << "SurfaceOnly: " << this->SurfaceOnly << "\n";
  os << indent << "BuildConnectivityIteratively: " << this->BuildConnectivityIteratively << "\n";
}
VTK_ABI_NAMESPACE_END
