/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPartitionedDataSetCollectionWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPartitionedDataSetCollectionWriter.h"

#include "vtkBase64Utilities.h"
#include "vtkDataAssembly.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
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
#include <numeric>

vtkStandardNewMacro(vtkXMLPartitionedDataSetCollectionWriter);
//------------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionWriter::vtkXMLPartitionedDataSetCollectionWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionWriter::~vtkXMLPartitionedDataSetCollectionWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetCollectionWriter::SetInputData(vtkPartitionedDataSetCollection* pdc)
{
  this->SetInputDataObject(pdc);
}

//------------------------------------------------------------------------------
int vtkXMLPartitionedDataSetCollectionWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetCollectionWriter::RequestData(
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

  auto inputPDC = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
  assert(inputPDC != nullptr);

  this->UpdateProgress(0.0);

  std::string path, filename, artifactsDir;
  std::tie(path, filename, artifactsDir) = vtkXMLWriter2::SplitFileName(this->FileName);
  vtkLogF(TRACE, "Filename components(path='%s', filename='%s', artifactsDir='%s')", path.c_str(),
    filename.c_str(), artifactsDir.c_str());
  if (!this->MakeDirectory(path))
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    vtkErrorMacro("Failed to create directory '" << path.c_str() << "'.");
    return 0;
  }

  // we intentionally don't add path as an artifact to cleanup if write fails.
  // this->AddArtifact(path, true);
  const auto absoluteArtifactsDir = path + "/" + artifactsDir;
  if (!this->MakeDirectory(absoluteArtifactsDir))
  {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    vtkErrorMacro("Failed to create directory '" << absoluteArtifactsDir.c_str() << "'.");
    return 0;
  }
  this->AddRootArtifact(absoluteArtifactsDir, /*isDir*/ true);

  auto controller = this->GetController();

  vtkNew<vtkXMLCompositeDataSetWriterHelper> helper;
  helper->SetWriter(this);

  const auto filenameNoExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
  std::vector<std::vector<std::string>> allFilenames(inputPDC->GetNumberOfPartitionedDataSets());

  // Write individual files for each dataset in every partitioned-dataset
  // and build the list of filenames to write the summary file.
  for (unsigned int pidx = 0, max = inputPDC->GetNumberOfPartitionedDataSets(); pidx < max; ++pidx)
  {
    auto pds = inputPDC->GetPartitionedDataSet(pidx);

    // note: localDataSets may have nullptrs.
    auto localDataSets =
      vtkCompositeDataSet::GetDataSets<vtkDataObject>(pds, /*preserveNull=*/true);
    int localOffset =
      vtkXMLWriter2::ExclusiveScanSum(controller, static_cast<int>(localDataSets.size()));

    for (auto& dataset : localDataSets)
    {
      const auto prefix = artifactsDir + "/" + filenameNoExt + "_" + std::to_string(pidx) + "_" +
        std::to_string(localOffset);
      const auto fname = helper->WriteDataSet(path, prefix, dataset);
      allFilenames[pidx].push_back(fname);
      if (!fname.empty())
      {
        this->AddArtifact(fname);
      }
      ++localOffset;
    }

    // pass written filenames to root node. Returned valie is non empty only on root node.
    allFilenames[pidx] = vtkXMLWriter2::Gather(controller, allFilenames[pidx], 0);
  }

  // Now write the summary XML on the root node.
  bool success = true;
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    success = this->WriteSummaryXML(inputPDC, allFilenames);
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

//------------------------------------------------------------------------------
bool vtkXMLPartitionedDataSetCollectionWriter::WriteSummaryXML(
  vtkPartitionedDataSetCollection* input, const std::vector<std::vector<std::string>>& allFilenames)
{
  assert(static_cast<unsigned int>(allFilenames.size()) == input->GetNumberOfPartitionedDataSets());

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
  int pindex = 0;
  for (const auto& partitionNames : allFilenames)
  {
    int dindex = 0;
    vtkNew<vtkXMLDataElement> parent;
    parent->SetName("Partitions");
    parent->SetIntAttribute("index", pindex);
    for (const auto& fname : partitionNames)
    {
      if (!fname.empty()) // fname will be empty for null nodes in the input.
      {
        vtkNew<vtkXMLDataElement> child;
        child->SetName("DataSet");
        child->SetIntAttribute("index", dindex);
        child->SetAttribute("file", fname.c_str());
        parent->AddNestedElement(child);
      }
      ++dindex;
    }

    const bool hasName =
      input->HasMetaData(pindex) && input->GetMetaData(pindex)->Has(vtkCompositeDataSet::NAME());

    // skip empty partitions, however do preserve name, if present.
    if (parent->GetNumberOfNestedElements() > 0 || hasName)
    {
      if (hasName)
      {
        parent->SetAttribute("name", input->GetMetaData(pindex)->Get(vtkCompositeDataSet::NAME()));
      }
      root->AddNestedElement(parent);
    }
    ++pindex;
  }

  // add DataAssembly
  if (auto da = input->GetDataAssembly())
  {
    vtkNew<vtkXMLDataElement> child;
    child->SetName("DataAssembly");
    child->SetAttribute("encoding", "base64");

    // As a first pass, we'll encode the XML and add it as char data. In
    // reality, we should be able to add the XML simply as a nested element,
    // however `vtkXMLDataParser`'s inability to read from a string makes it
    // unnecessarily hard and hence we leave that for now.
    auto xml = da->SerializeToXML(vtkIndent().GetNextIndent());
    unsigned char* encoded_buffer = new unsigned char[xml.size() * 2];
    auto encoded_buffer_size =
      vtkBase64Utilities::Encode(reinterpret_cast<const unsigned char*>(xml.c_str()),
        static_cast<unsigned long>(xml.size()), encoded_buffer);
    child->SetCharacterData(
      reinterpret_cast<char*>(encoded_buffer), static_cast<int>(encoded_buffer_size));
    delete[] encoded_buffer;
    root->AddNestedElement(child);
  }

  helper->AddXML(root);
  helper->AddGlobalFieldData(input);
  if (!helper->EndWriting())
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkXMLPartitionedDataSetCollectionWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
