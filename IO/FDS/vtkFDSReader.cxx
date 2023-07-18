/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFDSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFDSReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"

#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

// ----------------------------------------------------------------------------
struct vtkFDSReader::vtkInternals
{
  bool OpenFile(const std::string& fileName);

  vtksys::ifstream File;
};

//-----------------------------------------------------------------------------
bool vtkFDSReader::vtkInternals::OpenFile(const std::string& fileName)
{
  vtksys::SystemTools::Stat_t fs;
  if (!vtksys::SystemTools::Stat(fileName.c_str(), &fs))
  {
    this->File.open(fileName.c_str(), ios::in);
  }

  if (this->File.fail())
  {
    vtkErrorWithObjectMacro(nullptr, "Could not open file: " << fileName);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFDSReader)

  //------------------------------------------------------------------------------
  vtkFDSReader::vtkFDSReader()
  : Internals(new vtkFDSReader::vtkInternals())
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkFDSReader::~vtkFDSReader() = default;

//------------------------------------------------------------------------------
void vtkFDSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkFDSReader::AddSelector(const char* selector)
{
  if (selector && this->Selectors.insert(selector).second)
  {
    this->Modified();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkFDSReader::ClearSelectors()
{
  if (!this->Selectors.empty())
  {
    this->Selectors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkFDSReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->FileName.empty())
  {
    vtkErrorMacro("Requires valid input file name.");
    return 0;
  }

  if (!this->Internals->OpenFile(this->FileName))
  {
    return 0;
  }

  std::string rootNodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);

  // Setup dummy assembly
  this->Assembly->SetNodeName(vtkDataAssembly::GetRootNode(), rootNodeName.c_str());

  // Level 0
  const auto base = this->Assembly->AddNodes({ "Devices", "Slices", "Boundaries" });
  // Level 1
  const auto devices = this->Assembly->AddNodes({ "device_1", "device_2" }, base[0]);
  const auto slices =
    this->Assembly->AddNodes({ "slice_3D_1", "slice_3D_2", "Slices_1D", "Slices_2D" }, base[1]);
  const auto boundaries = this->Assembly->AddNodes({ "boundary_1", "boundary_2" }, base[2]);
  // Level 2
  const auto slices1D = this->Assembly->AddNodes({ "slice_1D_1", "slice_1D_2" }, slices[2]);
  const auto slices2D = this->Assembly->AddNodes({ "slice_2D_1", "slice_2D_2" }, slices[3]);

  // Update Assembly widget
  this->AssemblyTag += 1;

  return 1;
}

// ----------------------------------------------------------------------------
int vtkFDSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outInfo);

  if (!output)
  {
    vtkErrorMacro("Unable to retrieve the output !");
    return 0;
  }

  const std::vector<std::string> selectors(this->Selectors.begin(), this->Selectors.end());
  const auto selectedNodes = this->Assembly->SelectNodes(selectors);

  vtkNew<vtkDataAssembly> outAssembly;
  outAssembly->SubsetCopy(this->Assembly, selectedNodes);
  output->SetDataAssembly(outAssembly);

  return 1;
}

VTK_ABI_NAMESPACE_END
