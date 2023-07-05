// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPartitionedDataSetWriter.h"

#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkXMLCompositeDataSetWriterHelper.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataWriterHelper.h"

#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <map>
#include <memory>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPartitionedDataSetWriter);
//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetWriter::vtkXMLPartitionedDataSetWriter() = default;

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetWriter::~vtkXMLPartitionedDataSetWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetWriter::SetInputData(vtkPartitionedDataSet* pd)
{
  this->SetInputDataObject(pd);
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetWriter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkLogScopeF(TRACE, "RequestData ('%s')", this->FileName);
  this->SetErrorCode(vtkErrorCode::UnknownError);

  if (this->WriteToOutputString)
  {
    vtkErrorMacro("This writer does not support writing to string yet.");
    return 0;
  }

  if (!this->FileName || this->FileName[0] == '\0')
  {
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    vtkErrorMacro("Filename cannot be empty!");
    return 0;
  }

  auto controller = this->GetController();

  auto inputPDS = vtkPartitionedDataSet::GetData(inputVector[0], 0);
  assert(inputPDS != nullptr);

  this->UpdateProgress(0.0);

  std::string path, filename, artifactsDir;
  std::tie(path, filename, artifactsDir) = vtkXMLWriter2::SplitFileName(this->FileName);
  vtkLogF(TRACE, "Filename components(path='%s', filename='%s', artifactsDir='%s')", path.c_str(),
    filename.c_str(), artifactsDir.c_str());
  if (!this->MakeDirectory(path))
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    vtkErrorMacro("Failed to create directory '" << path << "'.");
    return 0;
  }

  // we intentionally don't add path as an artifact to cleanup if write fails.
  // this->AddArtifact(path, true);

  const auto absoluteArtifactsDir = path + "/" + artifactsDir;
  if (!this->MakeDirectory(absoluteArtifactsDir))
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    vtkErrorMacro("Failed to create directory '" << absoluteArtifactsDir << "'.");
    return 0;
  }
  this->AddRootArtifact(absoluteArtifactsDir, /*isDir*/ true);

  // note: localDataSets may have nullptrs.
  auto localDataSets =
    vtkCompositeDataSet::GetDataSets<vtkDataObject>(inputPDS, /*preserveNull=*/true);
  const int localOffset =
    vtkXMLWriter2::ExclusiveScanSum(controller, static_cast<int>(localDataSets.size()));

  // note: localFilenames may have empty strings.
  std::vector<std::string> localFilenames;

  vtkNew<vtkXMLCompositeDataSetWriterHelper> helper;
  helper->SetWriter(this);
  const auto filenameNoExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
  for (size_t cc = 0; cc < localDataSets.size(); ++cc)
  {
    const auto prefix = artifactsDir + "/" + filenameNoExt + "_" + std::to_string(localOffset + cc);
    auto fname = helper->WriteDataSet(path, prefix, localDataSets[cc]);
    localFilenames.push_back(fname);
    if (!fname.empty())
    {
      this->AddArtifact(fname);
    }
  }

  // pass written filenames to root node. allFilenames is non empty only on root node.
  std::vector<std::string> allFilenames = vtkXMLWriter2::Gather(controller, localFilenames, 0);

  // Now write the summary XML on the root node.
  bool success = true;
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    success = this->WriteSummaryXML(inputPDS, allFilenames);
  }
  if (controller != nullptr && controller->GetNumberOfProcesses() > 1)
  {
    int message[2] = { success ? 1 : 0, static_cast<int>(this->GetErrorCode()) };
    controller->Broadcast(message, 2, 0);
    success = (message[0] == 1);
    this->SetErrorCode(message[1]);
  }
  else if (success)
  {
    this->SetErrorCode(vtkErrorCode::NoError);
  }
  this->UpdateProgress(1.0);
  vtkLogF(TRACE, "success: %d", (int)success);
  return success ? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkXMLPartitionedDataSetWriter::WriteSummaryXML(
  vtkPartitionedDataSet* input, const std::vector<std::string>& allFilenames)
{
  vtkNew<vtkXMLDataWriterHelper> helper;
  helper->SetWriter(this);
  helper->SetDataSetVersion(this->GetDataSetMajorVersion(), this->GetDataSetMinorVersion());
  helper->SetDataSetName(input->GetClassName());
  if (!helper->OpenFile())
  {
    return false;
  }
  this->AddArtifact(this->FileName);

  if (!helper->BeginWriting())
  {
    return false;
  }

  // build and serialize the DOM.
  vtkNew<vtkXMLDataElement> root;
  root->SetName(input->GetClassName());
  int index = 0;
  for (auto& fname : allFilenames)
  {
    if (!fname.empty()) // fname will be empty for null nodes in the input.
    {
      vtkNew<vtkXMLDataElement> child;
      child->SetName("DataSet");
      child->SetIntAttribute("index", index);
      child->SetAttribute("file", fname.c_str());
      root->AddNestedElement(child);
    }
    ++index;
  }

  helper->AddXML(root);
  helper->AddGlobalFieldData(input);
  return helper->EndWriting() != 0;
}

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
