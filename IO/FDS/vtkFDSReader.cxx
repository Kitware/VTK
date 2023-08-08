// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFDSReader.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDoubleArray.h"
#include "vtkFileResourceStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkRectilinearGrid.h"
#include "vtkResourceParser.h"
#include "vtkWeakPointer.h"

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
  GRIDS = 0,
  DEVICES = 1,
  HRR = 2,
  SLICES = 3,
  BOUNDARIES = 4
};

const std::vector<std::string> BASE_NODES = { "Grids", "Devices", "HRR", "Slices", "Boundaries" };
const std::vector<std::string> DIM_KEYWORDS = { "TRNX", "TRNY", "TRNZ" };

struct FDSParser
{
  void Init(vtkResourceStream* stream)
  {
    this->Parser->Reset();
    this->Parser->SetStream(stream);
    this->Parser->StopOnNewLineOn();
    this->LineNumber = 0;
    this->Result = vtkParseResult::EndOfLine;
  }

  bool Parse(std::string& output)
  {
    this->Result = this->Parser->Parse(output);
    if (this->Result != vtkParseResult::Ok)
    {
      return false;
    }
    return true;
  }

  // TODO : template
  bool Parse(int& output)
  {
    this->Result = this->Parser->Parse(output);
    if (this->Result != vtkParseResult::Ok)
    {
      // TODO: increment line number for EOL ?
      return false;
    }
    return true;
  }

  // TODO : template
  bool Parse(double& output)
  {
    this->Result = this->Parser->Parse(output);
    if (this->Result != vtkParseResult::Ok)
    {
      // TODO: increment line number for EOL ?
      return false;
    }
    return true;
  }

  // TODO: add ParseKeyWord

  bool DiscardLine()
  {
    this->Result = this->Parser->DiscardLine();
    if (this->Result != vtkParseResult::EndOfLine)
    {
      return false;
    }
    this->LineNumber++;
    return true;
  }

  vtkNew<vtkResourceParser> Parser;
  vtkParseResult Result = vtkParseResult::EndOfLine;
  int LineNumber = 0; // current line
};

struct GridData
{
  vtkSmartPointer<vtkRectilinearGrid> Geometry;
  unsigned int GridNb;
};

struct SliceData
{
  vtkSmartPointer<vtkRectilinearGrid> Geometry;
  std::set<std::string> FileNames;
};
}

struct vtkFDSReader::vtkInternals
{
  // Maps used to retrieve filename(s) relative to
  // a given "leaf" in the data assembly
  std::map<int, ::GridData> Grids;
  std::map<int, std::string> HRRFiles;
  std::map<int, std::string> DevcFiles;
  std::map<int, ::SliceData> Slices;
  std::map<int, std::set<std::string>> BoundaryFiles;

  unsigned int MaxNbOfPartitions = 0;
  unsigned int GridCount = 0;
};

// TODO : find the right place for this
// TODO : on visitor by task ? or give it a function to execute ?
class vtkFDSGridVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkFDSGridVisitor* New();
  vtkTypeMacro(vtkFDSGridVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeId) override
  {
    if (this->Internals->Grids.count(nodeId) == 0)
    {
      return;
    }

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetPartition(0, this->Internals->Grids.at(nodeId).Geometry);
    this->OutputPDSC->SetPartitionedDataSet(nodeId, pds);
  }

  // TODO : is it relevant to store it like this ?
  std::shared_ptr<vtkFDSReader::vtkInternals> Internals;
  vtkWeakPointer<vtkPartitionedDataSetCollection> OutputPDSC;

protected:
  vtkFDSGridVisitor() = default;
  ~vtkFDSGridVisitor() override = default;

private:
  vtkFDSGridVisitor(const vtkFDSGridVisitor&) = delete;
  void operator=(const vtkFDSGridVisitor&) = delete;
};
vtkStandardNewMacro(vtkFDSGridVisitor);

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

  // TODO : remove if we can decorellate assembly and partitionned dataset collection
  this->Internals->MaxNbOfPartitions = 0;
  this->Internals->GridCount = 0;

  FDSParser parser;
  parser.Init(stream);

  // Main parsing loop
  do
  {
    std::string keyWord;
    if (!parser.Parse(keyWord))
    {
      // Let the loop handle the parsing result
      continue;
    }

    if (keyWord == "GRID")
    {
      // Retrieve grid name
      std::string gridName;
      if (!parser.Parse(gridName))
      {
        continue;
      }

      // Skip the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      vtkNew<vtkRectilinearGrid> grid;

      // Parse grid dimensions
      int dimensions[3] = { 0, 0, 0 };
      for (std::size_t i = 0; i < 3; ++i)
      {
        if (!parser.Parse(dimensions[i]))
        {
          vtkErrorMacro(<< "Failed to parse the grid dimension " << i << " at line "
                        << parser.LineNumber);
          return 0;
        }
        dimensions[i] += 1;
      }

      grid->SetDimensions(dimensions);

      // Discard the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Discard empty line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Expected : PDIM keyword
      if (!parser.Parse(keyWord))
      {
        continue;
      }

      if (keyWord != "PDIM")
      {
        vtkErrorMacro(<< "Expected a PDIM keyword at line " << parser.LineNumber
                      << ", but none was found.");
        return 0;
      }

      // Discard the rest of the line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // TODO retrieve data (grid extent)
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Discard empty line
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse X/Y/Z coordinates
      for (int dim = 0; dim < 3; dim++)
      {
        if (!parser.Parse(keyWord))
        {
          return 0; // TODO
        }

        // We should have TRNX, TRNY or TRNZ
        if (keyWord != ::DIM_KEYWORDS[dim])
        {
          vtkErrorMacro(<< "Expected a " << ::DIM_KEYWORDS[dim] << " keyword at line "
                        << parser.LineNumber << ", but none was found.");
          return 0;
        }

        // Discard the rest of the line
        if (!parser.DiscardLine())
        {
          continue;
        }

        // Discard line (contains unused info)
        if (!parser.DiscardLine())
        {
          continue;
        }

        vtkNew<vtkDoubleArray> coordArray;
        coordArray->SetNumberOfComponents(1);
        coordArray->SetNumberOfTuples(dimensions[dim]);

        // Iterate over all coordinates along current axis
        for (int id = 0; id < dimensions[dim]; id++)
        {
          // Parse X/Y/Z index
          int i = 0;
          if (!parser.Parse(i))
          {
            return 0; // TODO
          }

          if (i != id)
          {
            vtkErrorMacro(<< "Wrong dimention found. Expected " << id << ", got " << i << ".");
            return 0;
          }

          // Parse X/Y/Z coordinates value
          double d = 0.;
          if (!parser.Parse(d))
          {
            return 0; // TODO
          }

          coordArray->SetValue(id, d);

          // Discard the rest of the line
          if (!parser.DiscardLine())
          {
            continue;
          }
        }

        // TODO : need keep pointer
        switch (dim)
        {
          case 0:
            grid->SetXCoordinates(coordArray);
            break;
          case 1:
            grid->SetYCoordinates(coordArray);
            break;
          case 2:
            grid->SetZCoordinates(coordArray);
            break;
          default:
            break;
        }

        // Discard empty line
        if (!parser.DiscardLine())
        {
          continue;
        }
      }

      // Register grid and fill assembly
      const int idx = this->Assembly->AddNode(gridName.c_str(), baseNodes[GRIDS]);
      this->Assembly->AddDataSetIndex(idx, idx); // TODO : change ?
      ::GridData gridData;
      gridData.GridNb = ++this->Internals->GridCount;
      gridData.Geometry = grid;
      this->Internals->Grids.emplace(idx, gridData);
      this->Internals->MaxNbOfPartitions++;
    }
    else if (keyWord == "CSVF")
    {
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse CSV file type
      // Possible values are : hrr, devc
      std::string fileType;
      if (!parser.Parse(fileType))
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse CSV file type.");
        continue;
      }
      if (fileType == "devc")
      {
        if (!parser.DiscardLine())
        {
          continue;
        }

        // Parse devc file path
        std::string fileName;
        if (!parser.Parse(fileName))
        {
          vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse devc file path.");
          continue;
        }

        // TODO : check validity of file path
        std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

        // Register file path and fill assembly
        const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[DEVICES]);
        this->Internals->DevcFiles.emplace(idx, fileName);
        this->Internals->MaxNbOfPartitions++;
      }
      else if (fileType == "hrr")
      {
        if (!parser.DiscardLine())
        {
          continue;
        }

        // Parse hrr file path
        std::string fileName;
        if (!parser.Parse(fileName))
        {
          vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse hrr file path.");
          continue;
        }

        // TODO : check validity of file path
        std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

        // Register file path and fill assembly
        const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[HRR]);
        this->Assembly->AddDataSetIndex(idx, idx); // TODO : change ?
        this->Internals->HRRFiles.emplace(idx, fileName);
        this->Internals->MaxNbOfPartitions++;
      }
      else
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unknown CSV file type.");
      }
    }
    else if (keyWord == "SLCF" || keyWord == "SLCC")
    {
      vtkNew<vtkRectilinearGrid> grid;

      // Parse grid ID
      std::string gridNb;
      if (!parser.Parse(gridNb))
      {
        continue;
      }

      // Search for dimensions
      // We can have a specified slice ID before that but it's not mandatory
      std::string token;
      do
      {
        if (!parser.Parse(token))
        {
          return 0;
        }
        cout << token << endl;
      } while (token != "&");

      int dimensions[3] = { 0, 0, 0 };
      for (int dim = 0; dim < 3; dim++)
      {
        int start = 0;
        if (!parser.Parse(start))
        {
          vtkErrorMacro(<< "Unable to parse slice start id at line " << parser.LineNumber);
          return 0;
        }

        int end = 0;
        if (!parser.Parse(end))
        {
          vtkErrorMacro(<< "Unable to parse slice end id at line " << parser.LineNumber);
          return 0;
        }

        int dimension = end - start + 1;
        if (dimension < 1)
        {
          vtkErrorMacro(
            "Slice dimension " << dim << " invalid (0 or negative) at line " << parser.LineNumber);
          return 0;
        }
        dimensions[dim] = dimension;
        cout << dimension;
      }
      cout << endl;

      grid->SetDimensions(dimensions);

      // Generate X/Y/Z coordinates from associated grid
      for (int dim = 0; dim < 3; dim++)
      {
      }

      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse .sf file path
      std::string fileName;
      if (!parser.Parse(fileName))
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse sf file path.");
        continue;
      }

      // TODO : check validity of file path
      std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

      // TODO : take account of dimension
      // TODO : find a good way to store slices

      const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[SLICES]);
      this->Assembly->AddDataSetIndex(idx, idx); // TODO : change ?
      std::set<std::string>& filesAtIdx = this->Internals->Slices[idx].FileNames;
      filesAtIdx.emplace(fileName);
      this->Internals->MaxNbOfPartitions++;
    }
    else if (keyWord == "BNDF")
    {
      if (!parser.DiscardLine())
      {
        continue;
      }

      // Parse sf file path
      std::string fileName;
      if (!parser.Parse(fileName))
      {
        vtkWarningMacro(<< "Line " << parser.LineNumber << " : unable to parse sf file path.");
        continue;
      }

      // TODO : check validity of file path
      std::string nodeName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);

      const int idx = this->Assembly->AddNode(nodeName.c_str(), baseNodes[BOUNDARIES]);
      this->Assembly->AddDataSetIndex(idx, idx); // TODO : change ?
      std::set<std::string>& filesAtIdx = this->Internals->BoundaryFiles[idx];
      filesAtIdx.emplace(fileName);
      this->Internals->MaxNbOfPartitions++;
    }

    if (!parser.DiscardLine())
    {
      continue;
    }
  } while (parser.Result == vtkParseResult::EndOfLine);

  // The last result that ended the loop
  if (parser.Result != vtkParseResult::EndOfStream)
  {
    cout << static_cast<int>(parser.Result) << endl;
    vtkErrorMacro(<< "Error during parsing of SMV file at line " << parser.LineNumber);
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

  const std::vector<std::string> selectors(this->Selectors.begin(), this->Selectors.end());
  const auto selectedNodes = this->Assembly->SelectNodes(selectors);

  vtkNew<vtkDataAssembly> outAssembly;
  outAssembly->SubsetCopy(this->Assembly, selectedNodes);
  output->SetDataAssembly(outAssembly);
  output->SetNumberOfPartitionedDataSets(this->Internals->MaxNbOfPartitions);

  int gridIdx =
    outAssembly->FindFirstNodeWithName("Grids", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (gridIdx != -1)
  {
    vtkNew<vtkFDSGridVisitor> gridVisitor;
    gridVisitor->Internals = this->Internals;
    gridVisitor->OutputPDSC = output;
    outAssembly->Visit(gridIdx, gridVisitor);
  }

  int devicesIdx =
    outAssembly->FindFirstNodeWithName("Devices", vtkDataAssembly::TraversalOrder::BreadthFirst);
  if (devicesIdx != -1)
  {
    vtkNew<vtkFDSDeviceVisitor> deviceVisitor;
    deviceVisitor->Internals = this->Internals;
    outAssembly->Visit(devicesIdx, deviceVisitor);
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
