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
#include "vtkDataAssemblyVisitor.h"
#include "vtkFileResourceStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkResourceParser.h"

#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

#include <fstream>
#include <limits>

#include <sstream>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
enum BaseNodes
{
  DEVICES = 0,
  HRR = 1,
  SLICES = 2,
  BOUNDARIES = 3
};

const std::vector<std::string> BASE_NODES = { "Devices", "HRR", "Slices", "Boundaries" };
}

struct vtkFDSReader::vtkInternals
{
  // Maps used to retrieve filename(s) relative to
  // a given "leaf" in the data assembly
  std::map<int, std::string> HRRFiles;
  std::map<int, std::string> DevcFiles;
  std::map<int, std::set<std::string>> SliceFiles;
  std::map<int, std::set<std::string>> BoundaryFiles;
};

// TODO : find the right place for this
// TODO : on visitor by task ? or give it a function to execute ?
class vtkFDSDeviceVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSDeviceVisitor* New();
  vtkTypeMacro(vtkFDSDeviceVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (this->Internals->DevcFiles.count(nodeId) == 0)
    {
      return;
    }

    // Is a valid leaf
    std::string fileName = this->Internals->DevcFiles.at(nodeId);
    cout << "Node " << nodeId << " : " << fileName << endl;
  }

  // TODO : is it relevant to store it like this ?
  std::shared_ptr<vtkFDSReader::vtkInternals> Internals;

protected:
  vtkFDSDeviceVisitor() = default;
  ~vtkFDSDeviceVisitor() override = default;

private:
  vtkFDSDeviceVisitor(const vtkFDSDeviceVisitor&) = delete;
  void operator=(const vtkFDSDeviceVisitor&) = delete;
};
vtkStandardNewMacro(vtkFDSDeviceVisitor);

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFDSReader)

  //------------------------------------------------------------------------------
  vtkFDSReader::vtkFDSReader()
  : Internals(new vtkInternals())
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
  cout << "REQUEST INFO" << endl;

  vtkSmartPointer<vtkResourceStream> stream = this->Open();
  if (!stream)
  {
    vtkErrorMacro(<< "Request information : failed to open stream");
    return 0;
  }

  // Fill base structure
  this->Assembly->Initialize();
  const auto baseNodes = this->Assembly->AddNodes(BASE_NODES);

  if (!this->FileName.empty())
  {
    std::string rootNodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    this->Assembly->SetNodeName(vtkDataAssembly::GetRootNode(), rootNodeName.c_str());
  }

  vtkNew<vtkResourceParser> parser;
  parser->SetStream(stream);
  parser->StopOnNewLineOn();

  int lineNumber = 0;  // current line
  std::string keyWord; // storage for parsed "keyWord"

  // Main parsing loop
  vtkParseResult result = vtkParseResult::EndOfLine;
  while (result == vtkParseResult::EndOfLine)
  {
    lineNumber++;

    result = parser->Parse(keyWord);
    if (result != vtkParseResult::Ok)
    {
      continue;
    }

    if (keyWord == "CSVF")
    {
      result = parser->DiscardLine();
      if (result != vtkParseResult::EndOfLine)
      {
        continue;
      }
      lineNumber++;

      // Parse CSV file type
      // Possible values are : hrr, devc
      std::string fileType;
      result = parser->Parse(fileType);
      if (result != vtkParseResult::Ok)
      {
        vtkWarningMacro(<< "Line " << lineNumber << " : unable to parse CSV file type.");
        continue;
      }
      if (fileType == "devc")
      {
        result = parser->DiscardLine();
        if (result != vtkParseResult::EndOfLine)
        {
          continue;
        }
        lineNumber++;

        // Parse devc file path
        std::string fileName;
        result = parser->Parse(fileName);
        if (result != vtkParseResult::Ok)
        {
          vtkWarningMacro(<< "Line " << lineNumber << " : unable to parse devc file path.");
          continue;
        }

        // TODO : check validity of file path
        std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

        // Register file path and fill assembly
        const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[DEVICES]);
        this->Internals->DevcFiles.emplace(idx, fileName);
      }
      else if (fileType == "hrr")
      {
        result = parser->DiscardLine();
        if (result != vtkParseResult::EndOfLine)
        {
          continue;
        }
        lineNumber++;

        // Parse hrr file path
        std::string fileName;
        result = parser->Parse(fileName);
        if (result != vtkParseResult::Ok)
        {
          vtkWarningMacro(<< "Line " << lineNumber << " : unable to parse hrr file path.");
          continue;
        }

        // TODO : check validity of file path
        std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

        // Register file path and fill assembly
        const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[HRR]);
        this->Internals->HRRFiles.emplace(idx, fileName);
      }
      else
      {
        vtkWarningMacro(<< "Line " << lineNumber << " : unknown CSV file type.");
      }
    }
    else if (keyWord == "SLCF" || keyWord == "SLCC")
    {
      // TODO : check dimension

      result = parser->DiscardLine();
      if (result != vtkParseResult::EndOfLine)
      {
        continue;
      }
      lineNumber++;

      // Parse sf file path
      std::string fileName;
      result = parser->Parse(fileName);
      if (result != vtkParseResult::Ok)
      {
        vtkWarningMacro(<< "Line " << lineNumber << " : unable to parse sf file path.");
        continue;
      }

      // TODO : check validity of file path
      std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

      // TODO : take account of dimension
      // TODO : find a good way to store slices

      const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[SLICES]);
      std::set<std::string>& filesAtIdx = this->Internals->SliceFiles[idx];
      filesAtIdx.emplace(fileName);
    }
    else if (keyWord == "BNDF")
    {
      result = parser->DiscardLine();
      if (result != vtkParseResult::EndOfLine)
      {
        continue;
      }
      lineNumber++;

      // Parse sf file path
      std::string fileName;
      result = parser->Parse(fileName);
      if (result != vtkParseResult::Ok)
      {
        vtkWarningMacro(<< "Line " << lineNumber << " : unable to parse sf file path.");
        continue;
      }

      // TODO : check validity of file path
      std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

      const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[BOUNDARIES]);
      std::set<std::string>& filesAtIdx = this->Internals->BoundaryFiles[idx];
      filesAtIdx.emplace(fileName);
    }

    result = parser->DiscardLine();
  }

  // The last result that ended the loop
  if (result != vtkParseResult::EndOfStream)
  {
    vtkErrorMacro(<< "Error during parsing of SMV file at line" << lineNumber);
    return 0;
  }

  // Update Assembly widget
  this->AssemblyTag += 1;

  return 1;
}

// ----------------------------------------------------------------------------
int vtkFDSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  cout << "REQUEST DATA" << endl;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outInfo);

  if (!output)
  {
    vtkErrorMacro("Unable to retrieve the output !");
    return 0;
  }

  vtkSmartPointer<vtkResourceStream> stream = this->Open();
  if (!stream)
  {
    vtkErrorMacro(<< "Request data: failed to open stream");
    return 0;
  }

  // vtkNew<vtkResourceParser> parser;
  // parser->SetStream(stream);
  // parser->StopOnNewLineOn();

  // int lineNumber = 0; // current line
  // std::string keyWord; // the keyWord

  // const auto flushLine = [this, &parser, &lineNumber]() {
  //   std::string remaining;

  //   auto result = parser->Parse(remaining);
  //   if (result != vtkParseResult::EndOfLine)
  //   {
  //     vtkWarningMacro(<< "unexpected data at end of line in .smv file at line " << lineNumber);
  //     result = parser->DiscardLine();
  //   }

  //   return result;
  // };

  // vtkParseResult result = vtkParseResult::Ok;
  // while (result == vtkParseResult::Ok || result == vtkParseResult::EndOfLine)
  // {
  //   ++lineNumber;

  //   result = parser->Parse(keyWord);
  //   if (keyWord == "GRID")
  //   {
  //     cout << lineNumber << " : " << keyWord << " encountered" << endl;
  //     result = flushLine();

  //     // Parse grid dimensions
  //     std::array<double, 3> dims;
  //     for (std::size_t i = 0; i < 3; ++i)
  //     {
  //       result = parser->Parse(dims[i]);
  //       if (result != vtkParseResult::Ok)
  //       {
  //         vtkErrorMacro(<< "Failed to parse " << i << "th grid dimension at line " <<
  //         lineNumber); return 0;
  //       }
  //       else
  //       {
  //         cout << dims[i] << endl;
  //       }
  //     }

  //     result = flushLine();
  //   }
  // }

  const std::vector<std::string> selectors(this->Selectors.begin(), this->Selectors.end());
  const auto selectedNodes = this->Assembly->SelectNodes(selectors);

  vtkNew<vtkDataAssembly> outAssembly;
  outAssembly->SubsetCopy(this->Assembly, selectedNodes);
  output->SetDataAssembly(outAssembly);

  int devicesIdx =
    outAssembly->FindFirstNodeWithName("Devices", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (devicesIdx != -1)
  {
    cout << "Devices found" << endl;
    vtkNew<vtkFDSDeviceVisitor> visitor;
    visitor->Internals = this->Internals;
    outAssembly->Visit(devicesIdx, visitor);
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkResourceStream> vtkFDSReader::Open()
{
  if (this->Stream)
  {
    if (this->Stream->SupportSeek())
    {
      this->Stream->Seek(0, vtkResourceStream::SeekDirection::Begin);
    }

    return this->Stream;
  }

  auto fileStream = vtkSmartPointer<vtkFileResourceStream>::New();
  if (this->FileName.empty() || !fileStream->Open(this->FileName.c_str()))
  {
    vtkErrorMacro(<< "Failed to open file: "
                  << (this->FileName.empty() ? this->FileName : "No file name set"));
    return nullptr;
  }

  return fileStream;
}

VTK_ABI_NAMESPACE_END
