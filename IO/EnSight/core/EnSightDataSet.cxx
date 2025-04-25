// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "EnSightDataSet.h"

#include "core/EnSightFile.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTransformFilter.h"
#include "vtkTypeInt32Array.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace ensight_gold
{

namespace
{
constexpr int MAX_CASE_LINE_LENGTH = 1024;

// used for the first part of a case file line (e.g. model:, measured:, etc)
const std::regex& GetLineTypeRegEx()
{
  static const std::regex lineTypeRegEx(R"((?:^|\s)([[:alpha:]_\s]+:)(?=$|\s))");
  return lineTypeRegEx;
}

// integers
const std::regex& GetIntRegEx()
{
  static const std::regex intRegEx(R"(^(?:\s+)(\d+)(?=$|\s))");
  return intRegEx;
}

// floating point
const std::regex& GetNumRegEx()
{
  static const std::regex numRegEx(R"((?:^|\s)([-]?\d*\.?\d*e?[+-]?\d*[^\s])(?=$|\s))");
  return numRegEx;
}

// filenames or other cases where it's not determining the type  (e.g., change_coords_only)
const std::regex& GetFileNameRegEx()
{
  static const std::regex fileNameRegEx(R"((?:^|\s)([[:alnum:]/_.*-]+)(?=$|\s))");
  return fileNameRegEx;
}

template <typename T>
bool extractLinePart(const std::regex& rx, std::string& line, T& value)
{
  std::smatch sm;
  if (std::regex_search(line, sm, rx))
  {
    stringTo(sm.str(1), value);
    line = sm.suffix().str();
    return true;
  }
  return false;
}

bool extractFileName(std::string& line, std::string& filename)
{
  // need to handle:
  // space in the filename, in which case it must be surrounded by quotes
  // removing trailing whitespace
  // note we may also have some kind of option present after the filename in line
  char quotes = '\"';
  auto quoteBegin = line.find(quotes);
  if (quoteBegin == std::string::npos)
  {
    // no quotes - filename cannot contain spaces, so we can use regex
    return extractLinePart(GetFileNameRegEx(), line, filename);
  }

  // we have quotes, we know where the filename starts and ends
  auto quoteEnd = line.find(quotes, quoteBegin + 1);
  if (quoteEnd == std::string::npos)
  {
    vtkGenericWarningMacro("when extracting filename, unmatched quotes were found");
    return false;
  }

  filename = line.substr(quoteBegin + 1, quoteEnd - quoteBegin - 1);
  line = line.substr(quoteEnd + 1);

  return true;
}

void sanitize(std::string& str)
{
  char quotes = '\"';
  size_t found = str.find(quotes);
  if (found != std::string::npos)
  {
    str.erase(std::remove(str.begin(), str.end(), quotes), str.end());
  }

  // remove whitespace at the end of the string and before the string
  std::string whitespaces(" \t\n\r");
  found = str.find_last_not_of(whitespaces);
  if (found != std::string::npos)
  {
    str.erase(found + 1);
  }
  found = str.find_first_not_of(whitespaces);
  if (found != std::string::npos)
  {
    str.erase(0, found);
  }
}

// copied from the old reader to make sure tensor components are correct
int getDestinationComponent(int srcComponent, int numComponents)
{
  if (numComponents == 6)
  {
    // for 6 component tensors, the symmetric tensor components XZ and YZ are interchanged
    // see #10637.
    switch (srcComponent)
    {
      case 4:
        return 5;

      case 5:
        return 4;
    }
  }
  return srcComponent;
}

void setPointDataScalarsVectors(vtkDataSet* dataset, vtkDataArray* array)
{
  if (array->GetNumberOfComponents() == 1 && !dataset->GetPointData()->GetScalars())
  {
    dataset->GetPointData()->SetScalars(array);
  }
  else if (array->GetNumberOfComponents() == 3 && !dataset->GetPointData()->GetVectors())
  {
    dataset->GetPointData()->SetVectors(array);
  }
}

void setCellDataScalarsVectors(vtkDataSet* dataset, vtkDataArray* array)
{
  if (array->GetNumberOfComponents() == 1 && !dataset->GetCellData()->GetScalars())
  {
    dataset->GetCellData()->SetScalars(array);
  }
  else if (array->GetNumberOfComponents() == 3 && !dataset->GetCellData()->GetVectors())
  {
    dataset->GetCellData()->SetVectors(array);
  }
}

ElementType getElementTypeFromString(std::string line)
{
  static std::unordered_map<std::string, ElementType> eTypeMap = { { "point", ElementType::Point },
    { "bar2", ElementType::Bar2 }, { "bar3", ElementType::Bar3 }, { "tria3", ElementType::Tria3 },
    { "tria6", ElementType::Tria6 }, { "quad4", ElementType::Quad4 },
    { "quad8", ElementType::Quad8 }, { "tetra4", ElementType::Tetra4 },
    { "tetra10", ElementType::Tetra10 }, { "pyramid5", ElementType::Pyramid5 },
    { "pyramid13", ElementType::Pyramid13 }, { "penta6", ElementType::Penta6 },
    { "penta15", ElementType::Penta15 }, { "hexa8", ElementType::Hexa8 },
    { "hexa20", ElementType::Hexa20 }, { "nsided", ElementType::NSided },
    { "nfaced", ElementType::NFaced }, { "g_point", ElementType::GPoint },
    { "g_bar2", ElementType::GBar2 }, { "g_bar3", ElementType::GBar3 },
    { "g_tria3", ElementType::GTria3 }, { "g_tria6", ElementType::GTria6 },
    { "g_quad4", ElementType::GQuad4 }, { "g_quad8", ElementType::GQuad8 },
    { "g_tetra4", ElementType::GTetra4 }, { "g_tetra10", ElementType::GTetra10 },
    { "g_pyramid5", ElementType::GPyramid5 }, { "g_pyramid13", ElementType::GPyramid13 },
    { "g_penta6", ElementType::GPenta6 }, { "g_penta15", ElementType::GPenta15 },
    { "g_hexa8", ElementType::GHexa8 }, { "g_hexa20", ElementType::GHexa20 },
    { "g_nsided", ElementType::GNSided }, { "g_nfaced", ElementType::GNFaced } };

  std::regex regEx("^([^ ]+)");
  std::smatch sm;
  if (!std::regex_search(line, sm, regEx))
  {
    return ElementType::Unknown;
  }
  std::string str = sm.str(0);

  // remove whitespace from string
  str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
  auto it = eTypeMap.find(str);
  if (it == eTypeMap.end())
  {
    return ElementType::Unknown;
  }
  return it->second;
}

bool isValidCellSectionHeader(const std::string& line)
{
  if (line.find("block") != std::string::npos)
  {
    return true;
  }
  return getElementTypeFromString(line) != ElementType::Unknown;
}

int getNumComponents(VariableType type)
{
  switch (type)
  {
    case VariableType::ScalarPerNode:
    case VariableType::ScalarPerMeasuredNode:
    case VariableType::ScalarPerElement:
    case VariableType::ComplexScalarPerNode:
    case VariableType::ComplexScalarPerElement:
      return 1;
    case VariableType::VectorPerNode:
    case VariableType::VectorPerMeasuredNode:
    case VariableType::VectorPerElement:
    case VariableType::ComplexVectorPerNode:
    case VariableType::ComplexVectorPerElement:
      return 3;
    case VariableType::TensorSymmPerNode:
    case VariableType::TensorSymmPerElement:
      return 6;
    case VariableType::TensorAsymPerNode:
    case VariableType::TensorAsymPerElement:
      return 9;
    default:
      return 0;
  }
}

VariableType getVariableTypeFromString(const std::string& str)
{
  static std::unordered_map<std::string, VariableType> typeMap = {
    { "constant per case:", VariableType::ConstantPerCase },
    { "constant per case file:", VariableType::ConstantPerCaseFile },
    { "constant per part:", VariableType::ConstantPerPart },
    { "scalar per node:", VariableType::ScalarPerNode },
    { "scalar per measured node:", VariableType::ScalarPerMeasuredNode },
    { "vector per node:", VariableType::VectorPerNode },
    { "vector per measured node:", VariableType::VectorPerMeasuredNode },
    { "tensor symm per node:", VariableType::TensorSymmPerNode },
    { "tensor asym per node:", VariableType::TensorAsymPerNode },
    { "complex scalar per node:", VariableType::ComplexScalarPerNode },
    { "complex vector per node:", VariableType::ComplexVectorPerNode },
    { "scalar per element:", VariableType::ScalarPerElement },
    { "vector per element:", VariableType::VectorPerElement },
    { "tensor symm per element:", VariableType::TensorSymmPerElement },
    { "tensor asym per element:", VariableType::TensorAsymPerElement },
    { "complex scalar per element:", VariableType::ComplexScalarPerElement },
    { "complex vector per element:", VariableType::ComplexVectorPerElement }
  };
  auto it = typeMap.find(str);
  if (it == typeMap.end())
  {
    return VariableType::Unknown;
  }
  return it->second;
}

std::pair<int, int> getVTKCellType(ElementType e)
{
  static std::map<ElementType, std::pair<int, int>> vtkTypeMap = {
    { ElementType::Point, { VTK_VERTEX, 1 } }, { ElementType::Bar2, { VTK_LINE, 2 } },
    { ElementType::Bar3, { VTK_QUADRATIC_EDGE, 3 } }, { ElementType::Tria3, { VTK_TRIANGLE, 3 } },
    { ElementType::Tria6, { VTK_QUADRATIC_TRIANGLE, 6 } }, { ElementType::Quad4, { VTK_QUAD, 4 } },
    { ElementType::Quad8, { VTK_QUADRATIC_QUAD, 8 } }, { ElementType::Tetra4, { VTK_TETRA, 4 } },
    { ElementType::Tetra10, { VTK_QUADRATIC_TETRA, 10 } },
    { ElementType::Pyramid5, { VTK_PYRAMID, 5 } },
    { ElementType::Pyramid13, { VTK_QUADRATIC_PYRAMID, 13 } },
    { ElementType::Penta6, { VTK_WEDGE, 6 } },
    { ElementType::Penta15, { VTK_QUADRATIC_WEDGE, 15 } },
    { ElementType::Hexa8, { VTK_HEXAHEDRON, 8 } },
    { ElementType::Hexa20, { VTK_QUADRATIC_HEXAHEDRON, 20 } },
    { ElementType::NSided, { VTK_POLYGON, 0 } },    // will need to set num points
    { ElementType::NFaced, { VTK_POLYHEDRON, 0 } }, // will need to set num points
    { ElementType::GPoint, { VTK_VERTEX, 1 } }, { ElementType::GBar2, { VTK_LINE, 2 } },
    { ElementType::GBar3, { VTK_QUADRATIC_EDGE, 3 } }, { ElementType::GTria3, { VTK_TRIANGLE, 3 } },
    { ElementType::GTria6, { VTK_QUADRATIC_TRIANGLE, 6 } },
    { ElementType::GQuad4, { VTK_QUAD, 4 } }, { ElementType::GQuad8, { VTK_QUADRATIC_QUAD, 8 } },
    { ElementType::GTetra4, { VTK_TETRA, 4 } },
    { ElementType::GTetra10, { VTK_QUADRATIC_TETRA, 10 } },
    { ElementType::GPyramid5, { VTK_PYRAMID, 5 } },
    { ElementType::GPyramid13, { VTK_QUADRATIC_PYRAMID, 13 } },
    { ElementType::GPenta6, { VTK_WEDGE, 6 } },
    { ElementType::GPenta15, { VTK_QUADRATIC_WEDGE, 15 } },
    { ElementType::GHexa8, { VTK_HEXAHEDRON, 8 } },
    { ElementType::GHexa20, { VTK_QUADRATIC_HEXAHEDRON, 20 } },
    { ElementType::GNSided, { VTK_POLYGON, 0 } },   // will need to set num points
    { ElementType::GNFaced, { VTK_POLYHEDRON, 0 } } // will need to set num points
  };
  auto it = vtkTypeMap.find(e);
  if (it == vtkTypeMap.end())
  {
    return std::make_pair(-1, 0);
  }
  return it->second;
}

// evaluates a single option from a "block" line
void evaluateOption(const std::string& option, GridOptions& opts)
{
  if (option == "block")
  {
    // if nothing else is specified after "block", curvilinear is default
    // if it is specified, it will get updated on a future call
    opts.Type = GridType::Curvilinear;
  }
  else if (option == "coordinates")
  {
    opts.Type = GridType::Unstructured;
  }
  else if (option == "curvilinear")
  {
    opts.Type = GridType::Curvilinear;
  }
  else if (option == "rectilinear")
  {
    opts.Type = GridType::Rectilinear;
  }
  else if (option == "uniform")
  {
    opts.Type = GridType::Uniform;
  }
  else if (option == "iblanked")
  {
    opts.IBlanked = true;
  }
  else if (option == "with_ghost")
  {
    opts.WithGhost = true;
  }
  else if (option == "range")
  {
    opts.HasRange = true;
  }
}

// parse a "block" line to determine relevant options
GridOptions getGridOptions(std::string line)
{
  GridOptions opts;

  std::regex optRegEx(R"((?:^|\s)([[:alpha:]_]+)(?=$|\s))");
  std::string part;
  while (extractLinePart(optRegEx, line, part))
  {
    evaluateOption(part, opts);
  }

  return opts;
}

template <typename T>
void readCaseFileValues(EnSightFile& file, std::string& line, std::vector<T>& values)
{
  // time values may not all be on one line, and they may not even start
  // on the same line as 'time values:'
  T val;
  bool continueReading = true;
  while (continueReading)
  {
    while (extractLinePart(GetNumRegEx(), line, val))
    {
      values.push_back(val);
    }

    // Once we process a line, we need
    // to read the next to see if it contains time values
    // or if we should move on with processing the rest of the file
    auto result = file.ReadNextLine(MAX_CASE_LINE_LENGTH);
    continueReading = result.first;
    if (continueReading)
    {
      line = result.second;
      if (!std::all_of(line.begin(), line.end(),
            [](char c) -> bool
            { return isdigit(c) || isspace(c) || c == '.' || c == 'e' || c == '+' || c == '-'; }))
      {
        // The current line is not more time step values, so reset
        // this line so we can continue processing.
        file.GoBackOneLine();
        continueReading = false;
        // break;
      }
    }
  }
}

template <typename T>
void readFileValues(EnSightFile& file, std::vector<T>& values)
{
  auto result = file.ReadNextLine(MAX_CASE_LINE_LENGTH);
  while (result.first)
  {
    T val;
    while (extractLinePart(GetNumRegEx(), result.second, val))
    {
      values.push_back(val);
    }
    result = file.ReadNextLine(MAX_CASE_LINE_LENGTH);
  }
}

template <typename T>
void readFileValues(const std::string& filename, std::vector<T>& values)
{
  EnSightFile file;
  file.Format = FileType::ASCII;
  if (!file.SetFileNamePattern(filename, true))
  {
    vtkGenericWarningMacro("File " << filename << " could not be opened");
    return;
  }

  readFileValues(file, values);
}

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
bool EnSightDataSet::CheckVersion(const char* casefilename)
{
  if (!this->CaseFile.SetFileNamePattern(casefilename, true))
  {
    vtkGenericWarningMacro("Casefile " << casefilename << " could not be opened");
    return false;
  }
  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  do
  {
    if (result.second.find("FORMAT") != std::string::npos)
    {
      return this->ParseFormatSection();
    }
    result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  } while (result.first);
  return false;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ParseCaseFile(const char* casefilename)
{
  // need to reset since this means that RequestInformation has been called
  this->MeasuredPartitionId = -1;

  // has 5 sections: FORMAT, GEOMETRY, VARIABLE, TIME, FILE
  if (!this->CaseFile.SetFileNamePattern(casefilename, true))
  {
    vtkGenericWarningMacro("Casefile " << casefilename << " could not be opened");
    return false;
  }
  auto parentDir = vtksys::SystemTools::GetParentDirectory(casefilename);
  vtksys::SystemTools::SplitPath(parentDir, this->FilePath);

  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  while (result.first)
  {
    std::string& line = result.second;
    if (line.find("FORMAT") != std::string::npos)
    {
      if (!this->ParseFormatSection())
      {
        vtkGenericWarningMacro("This reader handles only EnSight Gold files");
        return false;
      }
    }
    else if (line.find("GEOMETRY") != std::string::npos)
    {
      this->ParseGeometrySection();
    }
    else if (line.find("VARIABLE") != std::string::npos)
    {
      this->ParseVariableSection();
    }
    else if (line.find("TIME") != std::string::npos)
    {
      this->ParseTimeSection();
    }
    else if (line.find("FILE") != std::string::npos)
    {
      this->ParseFileSection();
    }
    else if (line.find("MATERIAL") != std::string::npos ||
      line.find("BLOCK_CONTINUATION") != std::string::npos ||
      line.find("SCRIPTS") != std::string::npos)
    {
      vtkGenericWarningMacro("Skipping case file section: " << line);
      result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
      while (result.first && !this->IsSectionHeader(result.second))
      {
        result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
      }
      continue;
    }
    else
    {
      vtkGenericWarningMacro("ParseCaseFile: invalid line - " << line);
    }
    result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  }

  if (this->GeometryFile.TimeSet != -1)
  {
    auto it = this->TimeSetInfoMap.find(this->GeometryFile.TimeSet);
    if (it == this->TimeSetInfoMap.end())
    {
      // we earlier set the default time set to 1, but it turns out no time sets exist, so reset
      // back to -1
      this->GeometryFile.TimeSet = -1;
    }
    else
    {
      this->GeometryFile.SetTimeSetInfo(it->second);
    }
  }
  if (this->GeometryFile.FileSet != -1)
  {
    auto it = this->FileSetInfoMap.find(this->GeometryFile.FileSet);
    if (it == this->FileSetInfoMap.end())
    {
      vtkGenericWarningMacro("couldn't find file set with id " << this->GeometryFile.FileSet);
      return false;
    }
    this->GeometryFile.SetFileSetInfo(it->second);
  }

  if (this->MeasuredFile.TimeSet != -1)
  {
    auto it = this->TimeSetInfoMap.find(this->MeasuredFile.TimeSet);
    if (it == this->TimeSetInfoMap.end())
    {
      // we earlier set the default time set to 1, but it turns out no time sets exist, so reset
      // back to -1
      this->MeasuredFile.TimeSet = -1;
    }
    else
    {
      this->MeasuredFile.SetTimeSetInfo(it->second);
    }
  }
  if (this->MeasuredFile.FileSet != -1)
  {
    auto it = this->FileSetInfoMap.find(this->MeasuredFile.FileSet);
    if (it == this->FileSetInfoMap.end())
    {
      vtkGenericWarningMacro("couldn't find file set with id " << this->MeasuredFile.FileSet);
      return false;
    }
    this->MeasuredFile.SetFileSetInfo(it->second);
  }

  // add timeset/fileset info to all variables
  for (auto& var : this->Variables)
  {
    if (var.File.FileSet != -1)
    {
      auto it = this->FileSetInfoMap.find(var.File.FileSet);
      if (it == this->FileSetInfoMap.end())
      {
        vtkGenericWarningMacro("couldn't find file set with id " << var.File.FileSet);
        return false;
      }
      var.File.SetFileSetInfo(it->second);
    }

    if (var.File.TimeSet != -1)
    {
      auto it = this->TimeSetInfoMap.find(var.File.TimeSet);
      if (it == this->TimeSetInfoMap.end())
      {
        // we earlier set the default time set to 1, but it turns out no time sets exist, so reset
        // back to -1
        var.File.TimeSet = -1;
      }
      else
      {
        var.File.SetTimeSetInfo(it->second);
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ParseFormatSection()
{
  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  auto& line = result.second;
  if (line.find("ensight gold") != std::string::npos)
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void EnSightDataSet::ParseGeometrySection()
{
  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  while (result.first)
  {
    std::string line = result.second;
    if (this->IsSectionHeader(line))
    {
      this->CaseFile.GoBackOneLine();
      break;
    }

    // break the line into its parts
    // e.g. model ts fs filename
    std::string lineType, option, fileName;
    int timeSet = -1, fileSet = -1;

    if (!extractLinePart(GetLineTypeRegEx(), line, lineType))
    {
      vtkGenericWarningMacro("could not extract the line type from " << result.second);
    }
    extractLinePart(GetIntRegEx(), line, timeSet);

    extractLinePart(GetIntRegEx(), line, fileSet);
    if (!extractFileName(line, fileName))
    {
      vtkGenericWarningMacro("could not extract file name from " << result.second);
    }

    if (lineType == "model:")
    {
      this->GeometryFileName = this->GetFullPath(fileName);
      this->GeometryFile.SetFileNamePattern(this->GeometryFileName);
      extractLinePart(GetFileNameRegEx(), line, option);

      // option can be empty, 'change_coords_only', 'change_coords_only cstep', or
      // 'changing_geometry_per_part'. changing_geometry_per_part signals that part lines will have
      // a mandatory additional option in the part lines of the geometry file
      this->GeometryChangeCoordsOnly = option == "change_coords_only";
      if (this->GeometryChangeCoordsOnly)
      {
        // change_coords_only indicates that only coords change in geometry, otherwise connectivity
        // changes too. cstep means the zero-based time step that contains the connectivity
        extractLinePart(GetIntRegEx(), line, this->GeometryCStep);
      }

      // check to see if we do indeed have a static geometry
      if (timeSet == -1 && this->GeometryFile.CheckForMultipleTimeSteps())
      {
        // old reader seems to just automatically have a time set id 1 even if it's not specified
        // I have run into some customer data that used wildcards in filenames, but did not
        // specify the time set
        timeSet = 1;
      }
      this->GeometryFile.SetTimeAndFileSetInfo(timeSet, fileSet);

      if (this->GeometryFile.TimeSet == -1)
      {
        this->IsStaticGeometry = true;
      }

      if (this->IsStaticGeometry || this->GeometryChangeCoordsOnly)
      {
        this->MeshCache = vtkSmartPointer<vtkDataObjectMeshCache>::New();
      }
    }
    else if (lineType == "measured:")
    {
      this->MeasuredFile.SetTimeAndFileSetInfo(timeSet, fileSet);
      this->MeasuredFileName = this->GetFullPath(fileName);
      this->MeasuredFile.SetFileNamePattern(this->MeasuredFileName);
      extractLinePart(GetFileNameRegEx(), line, option);
    }
    else if (lineType == "match:")
    {
      vtkGenericWarningMacro("match files not supported yet");
    }
    else if (lineType == "boundary:")
    {
      vtkGenericWarningMacro("boundary files not supported yet");
    }
    else if (lineType == "rigid_body:")
    {
      this->RigidBodyFileName = this->GetFullPath(fileName);
      // it's technically possible to have a static mesh, but have rigid body transforms to apply
      // at each time step. In this case, ApplyRigidBodyTransforms ends up altering the mesh, and so
      // the transforms aren't applied to the original geometry, messing things up. The simple fix
      // is to not cache in this case.
      this->IsStaticGeometry = false;
    }
    else if (lineType == "Vector_glyphs:")
    {
      vtkGenericWarningMacro("Vector glyphs files not supported yet");
    }
    else
    {
      vtkGenericWarningMacro("ParseGeometrySection: invalid line - " << result.second);
    }

    result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ParseVariableSection()
{
  this->Variables.clear();
  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  while (result.first)
  {
    std::string& line = result.second;
    if (this->IsSectionHeader(line))
    {
      this->CaseFile.GoBackOneLine();
      break;
    }

    std::string varType, fileName;
    VariableOptions opts;

    extractLinePart(GetLineTypeRegEx(), line, varType);
    opts.Type = getVariableTypeFromString(varType);
    if (opts.Type == VariableType::Unknown)
    {
      // error
      vtkGenericWarningMacro("could not determine type of variable!");
      continue;
    }

    extractLinePart(GetIntRegEx(), line, opts.File.TimeSet);
    if (opts.Type == VariableType::ConstantPerCase)
    {
      extractLinePart(GetFileNameRegEx(), line, opts.Name);
      readCaseFileValues(this->CaseFile, line, opts.Constants);
    }
    else if (opts.Type == VariableType::ConstantPerCaseFile)
    {
      extractLinePart(GetFileNameRegEx(), line, opts.Name);
      if (!extractFileName(line, fileName))
      {
        vtkGenericWarningMacro("could not extract file name from " << result.second);
      }
      opts.File.SetFileNamePattern(this->GetFullPath(fileName));
    }
    else if (opts.Type == VariableType::ConstantPerPart)
    {
      vtkGenericWarningMacro("Constant per part not yet supported");
    }
    else
    {
      if (opts.File.TimeSet == -1)
      {
        // old reader seems to just automatically have a time set id 1 even if it's not specified
        // I have run into some customer data that used wildcards in filenames, but did not
        // specify the time set
        opts.File.TimeSet = 1;
      }

      extractLinePart(GetIntRegEx(), line, opts.File.FileSet);
      extractLinePart(GetFileNameRegEx(), line, opts.Name);

      if (!extractFileName(line, fileName))
      {
        vtkGenericWarningMacro("could not extract file name from " << result.second);
      }
      opts.File.SetFileNamePattern(this->GetFullPath(fileName));

      if (varType.find("complex") != std::string::npos)
      {
        // need to grab remaining info for complex var types
        extractLinePart(GetFileNameRegEx(), line, fileName);
        opts.ImaginaryFile.SetFileNamePattern(this->GetFullPath(fileName));
        opts.ImaginaryFile.TimeSet = opts.File.TimeSet;
        opts.ImaginaryFile.FileSet = opts.File.FileSet;
        extractLinePart(GetNumRegEx(), line, opts.Frequency);
      }
    }

    this->Variables.emplace_back(opts);
    result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ParseTimeSection()
{
  bool moreTimeSets = true;
  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  while (moreTimeSets && result.first)
  {
    std::shared_ptr<TimeSetInfo> tsInfo(new TimeSetInfo);
    int timeSet, startNum = -1, increment = -1;

    while (result.first)
    {
      std::string& line = result.second;
      if (this->IsSectionHeader(line))
      {
        this->CaseFile.GoBackOneLine();
        break;
      }

      std::string lineType;
      extractLinePart(GetLineTypeRegEx(), line, lineType);
      if (lineType == "time set:")
      {
        moreTimeSets = false;
        extractLinePart(GetIntRegEx(), line, timeSet);
      }
      else if (lineType == "number of steps:")
      {
        extractLinePart(GetIntRegEx(), line, tsInfo->NumberOfSteps);
      }
      else if (lineType == "filename start number:")
      {
        extractLinePart(GetIntRegEx(), line, startNum);
      }
      else if (lineType == "filename increment:")
      {
        extractLinePart(GetIntRegEx(), line, increment);
      }
      else if (lineType == "time values:")
      {
        readCaseFileValues(this->CaseFile, line, tsInfo->TimeValues);
        this->AllTimeSteps.insert(
          this->AllTimeSteps.end(), tsInfo->TimeValues.begin(), tsInfo->TimeValues.end());
      }
      else if (lineType == "filename numbers:")
      {
        readCaseFileValues(this->CaseFile, line, tsInfo->FileNameNumbers);
      }
      else if (lineType == "filename numbers file:")
      {
        std::string filename;
        extractFileName(line, filename);
        readFileValues(this->GetFullPath(filename), tsInfo->FileNameNumbers);
      }
      else if (lineType == "time values file:")
      {
        std::string filename;
        extractFileName(line, filename);
        readFileValues(this->GetFullPath(filename), tsInfo->TimeValues);
        this->AllTimeSteps.insert(
          this->AllTimeSteps.end(), tsInfo->TimeValues.begin(), tsInfo->TimeValues.end());
      }
      else if (lineType == "maximum time steps:")
      {
        // this line can just be ignored
      }

      result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
      if (!moreTimeSets && result.second.find("time set") != std::string::npos)
      {
        moreTimeSets = true;
        break;
      }
    }

    if (startNum >= 0 && increment > 0)
    {
      tsInfo->FileNameNumbers.resize(tsInfo->NumberOfSteps);
      tsInfo->FileNameNumbers[0] = startNum;
      for (unsigned int i = 1; i < tsInfo->FileNameNumbers.size(); ++i)
      {
        tsInfo->FileNameNumbers[i] = tsInfo->FileNameNumbers[i - 1] + increment;
      }
    }

    if (static_cast<size_t>(tsInfo->NumberOfSteps) != tsInfo->TimeValues.size())
    {
      vtkGenericWarningMacro("Parsing time section, found "
        << tsInfo->NumberOfSteps << " steps, but only " << tsInfo->TimeValues.size()
        << " time values");
    }
    this->TimeSetInfoMap.insert(std::make_pair(timeSet, tsInfo));
  }

  // make sure this->AllTimeSteps has only unique values and is sorted.
  std::sort(this->AllTimeSteps.begin(), this->AllTimeSteps.end());
  auto uniqueVals = std::unique(this->AllTimeSteps.begin(), this->AllTimeSteps.end());
  this->AllTimeSteps.erase(uniqueVals, this->AllTimeSteps.end());
}

//------------------------------------------------------------------------------
void EnSightDataSet::ParseFileSection()
{
  bool moreFileSets = true;
  auto result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
  while (moreFileSets && result.first)
  {
    std::shared_ptr<FileSetInfo> fsInfo(new FileSetInfo);
    int fileSet = -1, numSteps = -1, fileIndex = -1;
    moreFileSets = false;

    while (result.first)
    {
      std::string& line = result.second;
      if (this->IsSectionHeader(line))
      {
        this->CaseFile.GoBackOneLine();
        break;
      }

      std::string lineType;
      extractLinePart(GetLineTypeRegEx(), line, lineType);
      if (lineType == "file set:")
      {
        extractLinePart(GetIntRegEx(), line, fileSet);
      }
      else if (lineType == "number of steps:")
      {
        extractLinePart(GetIntRegEx(), line, numSteps);
        fsInfo->NumberOfSteps.push_back(numSteps);
      }
      else if (lineType == "filename index:")
      {
        extractLinePart(GetIntRegEx(), line, fileIndex);
        fsInfo->FileNameIndex.push_back(fileIndex);
      }

      result = this->CaseFile.ReadNextLine(MAX_CASE_LINE_LENGTH);
      if (result.second.find("file set") != std::string::npos)
      {
        moreFileSets = true;
        break;
      }
    }
    this->FileSetInfoMap.insert(std::make_pair(fileSet, fsInfo));
  }
}

//------------------------------------------------------------------------------
std::string EnSightDataSet::GetFullPath(const std::string& fname)
{
  this->FilePath.emplace_back(fname);
  auto fileName = vtksys::SystemTools::JoinPath(this->FilePath);
  this->FilePath.pop_back();
  return fileName;
}

//------------------------------------------------------------------------------
void EnSightDataSet::SetVariableFileFormat()
{
  auto format = this->GeometryFile.Format;
  if (format == FileType::ASCII)
  {
    return; // ASCII is default
  }
  for (auto& var : this->Variables)
  {
    var.File.Format = format;
    var.File.ByteOrder = this->GeometryFile.ByteOrder;
    var.ImaginaryFile.Format = format;
  }
}

//------------------------------------------------------------------------------
std::vector<double> EnSightDataSet::GetTimeSteps()
{
  return this->AllTimeSteps;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::IsSectionHeader(std::string line)
{
  std::unordered_set<std::string> headerSet = { "FORMAT", "GEOMETRY", "VARIABLE", "TIME", "FILE",
    "MATERIAL", "BLOCK_CONTINUATION", "SCRIPTS" };
  line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
  line.erase(std::remove(line.begin(), line.end(), ':'), line.end());
  auto it = headerSet.find(line);
  return (it == headerSet.end() ? false : true);
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ReadGeometry(vtkPartitionedDataSetCollection* output,
  vtkDataArraySelection* selection, bool outputStructureOnly)
{
  vtkLogScopeFunction(TRACE);
  if ((this->IsStaticGeometry || this->GeometryChangeCoordsOnly) && !this->MeshCache)
  {
    vtkGenericWarningMacro("Cache is null when it should not be");
    return false;
  }

  if (this->IsStaticGeometry)
  {
    auto cacheStatus = this->MeshCache->GetStatus();
    if (cacheStatus.CacheDefined)
    {
      this->MeshCache->CopyCacheToDataObject(output);
      // nothing changes, no need to read anything
      return true;
    }
  }
  else if (this->GeometryChangeCoordsOnly)
  {
    if (this->MeshCache->GetStatus().CacheDefined)
    {
      this->MeshCache->CopyCacheToDataObject(output);
      // only the coords change, we still need to read that
      return true;
    }
  }

  if (!this->GeometryFile.SetTimeStepToRead(this->ActualTimeValue))
  {
    vtkGenericWarningMacro("couldn't correctly set time step to read. Aborting");
    return false;
  }

  this->GeometryFile.CheckForBeginTimeStepLine();
  this->GeometryFile.SkipNLines(4);

  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("extents") != std::string::npos)
  {
    if (this->GeometryFile.Format == FileType::ASCII)
    {
      // two values per line in ASCII case
      this->GeometryFile.SkipNLines(3);
    }
    else
    {
      this->GeometryFile.MoveReadPosition(6 * sizeof(float));
    }
    result = this->GeometryFile.ReadNextLine(); // "part"
  }

  while (result.first && result.second.find("part") != std::string::npos)
  {
    int partId = this->ReadPartId(this->GeometryFile);
    partId--; // EnSight starts counts at 1

    auto it = this->PartInfoMap.find(partId);
    if (it == this->PartInfoMap.end())
    {
      vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
      return false;
    }

    auto& partInfo = it->second;

    result = this->GeometryFile.ReadNextLine(); // part description line
    auto partName = result.second;

    bool readPart = false;
    bool addToPDC = false;
    if (selection->ArrayIsEnabled(partName.c_str()))
    {
      readPart = true;
      addToPDC = true;
    }
    if (outputStructureOnly)
    {
      // in this case, this rank is not responsible for reading this part, but it still needs to
      // provide a PDS for it in the output, so the structure of the PDC matches across ranks.
      // So we only change readPart to false, so we can skip to the correct part of the file
      readPart = false;
    }

    vtkSmartPointer<vtkDataSet> grid = nullptr;

    result = this->GeometryFile.ReadNextLine();
    auto opts = getGridOptions(result.second);
    if (readPart)
    {
      switch (opts.Type)
      {
        case GridType::Uniform:
          if (!grid)
          {
            grid = vtkSmartPointer<vtkUniformGrid>::New();
          }
          this->CreateUniformGridOutput(opts, vtkUniformGrid::SafeDownCast(grid));
          break;
        case GridType::Rectilinear:
          if (!grid)
          {
            grid = vtkSmartPointer<vtkRectilinearGrid>::New();
          }
          this->CreateRectilinearGridOutput(opts, vtkRectilinearGrid::SafeDownCast(grid));
          break;
        case GridType::Curvilinear:
          if (!grid)
          {
            grid = vtkSmartPointer<vtkStructuredGrid>::New();
          }
          this->CreateStructuredGridOutput(opts, vtkStructuredGrid::SafeDownCast(grid));
          break;
        case GridType::Unstructured:
          if (!grid)
          {
            grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
          }
          this->CreateUnstructuredGridOutput(opts, vtkUnstructuredGrid::SafeDownCast(grid));
          break;
        default:
          vtkGenericWarningMacro("Grid type not correctly specified");
          return false;
      }
      if (grid)
      {
        this->ApplyRigidBodyTransforms(partId, partName, grid);
      }
    }
    else
    {
      switch (opts.Type)
      {
        case GridType::Uniform:
          this->PassThroughUniformGrid(opts, partId);
          break;
        case GridType::Rectilinear:
          this->PassThroughRectilinearGrid(opts, partId);
          break;
        case GridType::Curvilinear:
          this->PassThroughStructuredGrid(opts, partId);
          break;
        case GridType::Unstructured:
          this->PassThroughUnstructuredGrid(opts, partId);
          break;
        default:
          vtkGenericWarningMacro("Grid type not correctly specified");
          return false;
      }
    }

    if (addToPDC)
    {
      if (!this->PartOfSOSFile)
      {
        // In this case, we don't need to worry about the coordination of PDCIndex info
        // across casefiles, so we can just assign this part the next id in the  PDC
        partInfo.PDCIndex = output->GetNumberOfPartitionedDataSets();
      }
      vtkLog(TRACE,
        "part id " << partId << " " << partName << " will be added as PDS # " << partInfo.PDCIndex);
      vtkNew<vtkPartitionedDataSet> pds;
      if (grid)
      {
        pds->SetNumberOfPartitions(1);
        pds->SetPartition(0, grid);
      }
      else
      {
        pds->SetNumberOfPartitions(0);
      }
      output->SetPartitionedDataSet(partInfo.PDCIndex, pds);
      output->GetMetaData(partInfo.PDCIndex)->Set(vtkCompositeDataSet::NAME(), partName);

      auto assembly = output->GetDataAssembly();
      auto validName = vtkDataAssembly::MakeValidNodeName(partName.c_str());
      auto node = assembly->AddNode(validName.c_str());
      assembly->AddDataSetIndex(node, partInfo.PDCIndex);
    }

    if (this->GeometryFile.CheckForEndTimeStepLine())
    {
      break;
    }
    result = this->GeometryFile.ReadNextLine();
  }

  // We only create vtkPartitionedDataSets for parts that are being read. If we're only reading a
  // single casefile, there should be nothing else to do here. If this is being read as part of an
  // SOS file, then we need to make sure the number of vtkPartitionedDataSets is correct in the
  // output, and that we set the metadata and assembly info for empty vtkPartitionedDataSets.
  auto assembly = output->GetDataAssembly();
  if (this->PartOfSOSFile)
  {
    output->SetNumberOfPartitionedDataSets(this->NumberOfLoadedParts);
    for (unsigned int i = 0; i < output->GetNumberOfPartitionedDataSets(); i++)
    {
      vtkSmartPointer<vtkPartitionedDataSet> pds = output->GetPartitionedDataSet(i);
      if (!pds)
      {
        pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
        output->SetPartitionedDataSet(i, pds);
      }
      if (!output->GetMetaData(i)->Has(vtkCompositeDataSet::NAME()))
      {
        auto name = this->LoadedPartNames->GetValue(i);
        output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name.c_str());
        auto validName = vtkDataAssembly::MakeValidNodeName(name.c_str());
        auto node = assembly->AddNode(validName.c_str());
        assembly->AddDataSetIndex(node, i);
      }
    }
  }

  if (this->IsStaticGeometry || this->GeometryChangeCoordsOnly)
  {
    this->MeshCache->UpdateCache(output);
  }

  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ReadMeasuredGeometry(vtkPartitionedDataSetCollection* output,
  vtkDataArraySelection* selection, bool outputStructureOnly)
{
  vtkLogScopeFunction(TRACE);
  bool addToPDC = selection->ArrayIsEnabled(this->MeasuredPartName.c_str());
  if (addToPDC && (outputStructureOnly || this->MeasuredFileName.empty()))
  {
    // we don't need to read anything in this case, just need to make sure we have a PDS for this
    // so all ranks will match
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(0);
    if (this->MeasuredPartitionId == -1)
    {
      this->MeasuredPartitionId = output->GetNumberOfPartitionedDataSets();
    }
    vtkLog(TRACE, "Adding an empty PDS for measured data at index " << this->MeasuredPartitionId);
    output->SetPartitionedDataSet(this->MeasuredPartitionId, pds);
    output->GetMetaData(this->MeasuredPartitionId)
      ->Set(vtkCompositeDataSet::NAME(), this->MeasuredPartName);

    auto assembly = output->GetDataAssembly();
    auto validName = vtkDataAssembly::MakeValidNodeName(this->MeasuredPartName.c_str());
    auto node = assembly->AddNode(validName.c_str());
    assembly->AddDataSetIndex(node, this->MeasuredPartitionId);
    return true;
  }

  if (outputStructureOnly || !addToPDC)
  {
    vtkLog(TRACE, "Not reading measured data and NOT adding an empty PDS");
    return true;
  }

  if (!this->MeasuredFile.SetTimeStepToRead(this->ActualTimeValue))
  {
    vtkGenericWarningMacro("couldn't correctly set time step to read. Aborting");
    return false;
  }
  this->MeasuredFile.CheckForBeginTimeStepLine();

  // description line
  this->MeasuredFile.SkipNLines(1);

  auto result = this->MeasuredFile.ReadNextLine();
  if (result.second.find("particle coordinates") == std::string::npos)
  {
    vtkGenericWarningMacro("second line doesn't contain 'particle coordinates'");
    return false;
  }

  int numParticles;
  this->MeasuredFile.ReadNumber(&numParticles);

  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(numParticles);
  vtkNew<vtkCellArray> vertices;
  vertices->AllocateEstimate(numParticles, 1);

  vtkNew<vtkTypeInt32Array> idArray;
  idArray->SetNumberOfTuples(numParticles);
  idArray->SetName("Node Ids");

  // according to the old reader, it seems that the rest of the file is formatted differently
  // depending on whether it's ASCII or binary
  // for ASCII:
  // rest of file is numParticles lines of id, x, y, z
  // for binary:
  // the point ids are stored first, then the 3d coordinates stored in a tuple-by-tuple manner
  if (this->MeasuredFile.Format == FileType::ASCII)
  {
    for (int i = 0; i < numParticles; i++)
    {
      result = this->MeasuredFile.ReadNextLine();
      if (!result.first)
      {
        break;
      }

      std::stringstream ss(result.second);
      int id;
      ss >> id;
      idArray->SetValue(i, id);

      float coords[3];
      for (int j = 0; j < 3; j++)
      {
        ss >> coords[j];
      }
      points->SetPoint(i, coords);
      vtkIdType ptId = i;
      vertices->InsertNextCell(1, &ptId);
    }
    polydata->SetPoints(points);
    polydata->SetVerts(vertices);
  }
  else
  {
    this->MeasuredFile.ReadArray(idArray->WritePointer(0, numParticles), numParticles);

    vtkNew<vtkFloatArray> coords;
    coords->SetNumberOfComponents(3);
    coords->SetNumberOfTuples(numParticles);
    // This is different than what the old binary reader does, but if I'm understanding that
    // correctly this should be a more efficient equivalent.
    this->MeasuredFile.ReadArray(coords->WritePointer(0, numParticles * 3), numParticles * 3);
    points->SetData(coords);
    polydata->SetPoints(points);
    for (int i = 0; i < numParticles; i++)
    {
      vtkIdType ptId = i;
      vertices->InsertNextCell(1, &ptId);
    }
    polydata->SetVerts(vertices);
  }
  polydata->GetPointData()->SetGlobalIds(idArray);
  this->MeasuredFile.CheckForEndTimeStepLine();

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(1);
  pds->SetPartition(0, polydata);
  if (this->MeasuredPartitionId == -1)
  {
    this->MeasuredPartitionId = output->GetNumberOfPartitionedDataSets();
  }
  vtkLog(TRACE, "Adding PDS for measured data at index " << this->MeasuredPartitionId);
  output->SetPartitionedDataSet(this->MeasuredPartitionId, pds);
  output->GetMetaData(this->MeasuredPartitionId)
    ->Set(vtkCompositeDataSet::NAME(), this->MeasuredPartName);

  auto assembly = output->GetDataAssembly();
  auto validName = vtkDataAssembly::MakeValidNodeName(this->MeasuredPartName.c_str());
  auto node = assembly->AddNode(validName.c_str());
  assembly->AddDataSetIndex(node, this->MeasuredPartitionId);
  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::GetPartInfo(vtkDataArraySelection* partSelection,
  vtkDataArraySelection* pointArraySelection, vtkDataArraySelection* cellArraySelection,
  vtkDataArraySelection* fieldArraySelection, vtkStringArray* partNames)
{
  // Since we just want to get info on all the parts, we'll just look at the first time step
  if (!this->GeometryFile.SetTimeStepToRead(0.0))
  {
    vtkGenericWarningMacro("couldn't correctly set time step to read. Aborting");
    return false;
  }

  partNames->Initialize();

  // now that geometry file has been opened and file type detected, set format for all variables
  this->SetVariableFileFormat();

  this->GeometryFile.CheckForBeginTimeStepLine();
  this->GeometryFile.SkipNLines(2);

  // read node id, which can be off/given/assign/ignore
  auto result = this->GeometryFile.ReadNextLine();
  std::regex nodeRegEx(R"((?:^|\s)(off|given|assign|ignore)(?=$|\s))");
  std::smatch sm;
  if (result.first && std::regex_search(result.second, sm, nodeRegEx))
  {
    if (sm[1] == "given" || sm[1] == "ignore")
    {
      this->NodeIdsListed = true;
    }
  }

  // similarly for element id
  result = this->GeometryFile.ReadNextLine();
  if (result.first && std::regex_search(result.second, sm, nodeRegEx))
  {
    if (sm[1] == "given" || sm[1] == "ignore")
    {
      this->ElementIdsListed = true;
    }
  }

  result = this->GeometryFile.ReadNextLine();
  if (result.second.find("extents") != std::string::npos)
  {
    if (this->GeometryFile.Format == FileType::ASCII)
    {
      // two values per line in ASCII case
      this->GeometryFile.SkipNLines(3);
    }
    else
    {
      this->GeometryFile.MoveReadPosition(6 * sizeof(float));
    }
    result = this->GeometryFile.ReadNextLine(); // "part"
  }

  while (result.first && result.second.find("part") != std::string::npos)
  {
    int partId = this->ReadPartId(this->GeometryFile);
    partId--; // EnSight starts counts at 1
    auto retVal = this->PartInfoMap.insert(std::make_pair(partId, PartInfo()));
    auto& partInfo = retVal.first->second;

    result = this->GeometryFile.ReadNextLine(); // part description line
    partInfo.Name = result.second;
    partSelection->AddArray(partInfo.Name.c_str());
    partNames->InsertValue(partId, partInfo.Name);

    result = this->GeometryFile.ReadNextLine();
    auto opts = getGridOptions(result.second);
    switch (opts.Type)
    {
      case GridType::Uniform:
        this->PassThroughUniformGrid(opts, partId);
        break;
      case GridType::Rectilinear:
        this->PassThroughRectilinearGrid(opts, partId);
        break;
      case GridType::Curvilinear:
        this->PassThroughStructuredGrid(opts, partId);
        break;
      case GridType::Unstructured:
        this->PassThroughUnstructuredGrid(opts, partId);
        break;
      default:
        vtkGenericWarningMacro("Grid type not correctly specified");
        return false;
    }
    if (this->GeometryFile.CheckForEndTimeStepLine())
    {
      break;
    }
    result = this->GeometryFile.ReadNextLine();
  }

  if (!this->MeasuredFileName.empty())
  {
    partSelection->AddArray(this->MeasuredPartName.c_str());
    partNames->InsertNextValue(this->MeasuredPartName.c_str());
  }

  for (auto& var : this->Variables)
  {
    switch (var.Type)
    {
      case VariableType::ScalarPerNode:
      case VariableType::VectorPerNode:
      case VariableType::TensorSymmPerNode:
      case VariableType::TensorAsymPerNode:
      case VariableType::ScalarPerMeasuredNode:
      case VariableType::VectorPerMeasuredNode:
      case VariableType::ComplexScalarPerNode:
      case VariableType::ComplexVectorPerNode:
        pointArraySelection->AddArray(var.Name.c_str());
        break;

      case VariableType::ScalarPerElement:
      case VariableType::VectorPerElement:
      case VariableType::TensorSymmPerElement:
      case VariableType::TensorAsymPerElement:
      case VariableType::ComplexScalarPerElement:
      case VariableType::ComplexVectorPerElement:
        cellArraySelection->AddArray(var.Name.c_str());
        break;
      case VariableType::ConstantPerCase:
      case VariableType::ConstantPerCaseFile:
      case VariableType::ConstantPerPart:
        fieldArraySelection->AddArray(var.Name.c_str());
        break;
      default:
        vtkGenericWarningMacro("invalid variable type found: " << static_cast<int>(var.Type));
        break;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ReadVariables(vtkPartitionedDataSetCollection* output,
  vtkDataArraySelection* partSelection, vtkDataArraySelection* pointArraySelection,
  vtkDataArraySelection* cellArraySelection, vtkDataArraySelection* fieldArraySelection)
{
  for (auto& var : this->Variables)
  {
    switch (var.Type)
    {
      case VariableType::ScalarPerNode:
      case VariableType::VectorPerNode:
      case VariableType::TensorSymmPerNode:
      case VariableType::TensorAsymPerNode:
        if (pointArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableNodes(
            var.File, var.Name, getNumComponents(var.Type), output, partSelection);
        }
        break;

      case VariableType::ScalarPerMeasuredNode:
      case VariableType::VectorPerMeasuredNode:
        if (pointArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableMeasuredNodes(
            var.File, var.Name, getNumComponents(var.Type), output, partSelection);
        }
        break;

      case VariableType::ComplexScalarPerNode:
        if (pointArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableNodes(
            var.File, var.Name, getNumComponents(var.Type), output, partSelection, true, true);
          this->ReadVariableNodes(var.ImaginaryFile, var.Name, getNumComponents(var.Type), output,
            partSelection, true, false);
        }
        break;

      case VariableType::ComplexVectorPerNode:
        if (pointArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableNodes(
            var.File, var.Name + "_r", getNumComponents(var.Type), output, partSelection);
          this->ReadVariableNodes(
            var.ImaginaryFile, var.Name + "_i", getNumComponents(var.Type), output, partSelection);
        }
        break;

      case VariableType::ScalarPerElement:
      case VariableType::VectorPerElement:
      case VariableType::TensorSymmPerElement:
      case VariableType::TensorAsymPerElement:
        if (cellArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableElements(
            var.File, var.Name, getNumComponents(var.Type), output, partSelection);
        }
        break;

      case VariableType::ComplexScalarPerElement:
        if (cellArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableElements(
            var.File, var.Name, getNumComponents(var.Type), output, partSelection, true, true);
          this->ReadVariableElements(var.ImaginaryFile, var.Name, getNumComponents(var.Type),
            output, partSelection, true, false);
        }
        break;

      case VariableType::ComplexVectorPerElement:
        if (cellArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableElements(
            var.File, var.Name + "_r", getNumComponents(var.Type), output, partSelection);
          this->ReadVariableElements(
            var.ImaginaryFile, var.Name + "_i", getNumComponents(var.Type), output, partSelection);
        }
        break;

      case VariableType::ConstantPerCase:
      case VariableType::ConstantPerCaseFile:
        if (fieldArraySelection->ArrayIsEnabled(var.Name.c_str()))
        {
          this->ReadVariableConstantCase(var, output);
        }
        break;

      case VariableType::ConstantPerPart:
        // TODO:
        vtkGenericWarningMacro("constant per part not yet supported.");
        break;

      default:
        vtkGenericWarningMacro("Variable type is unknown");
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadVariableNodes(EnSightFile& file, const std::string& arrayName,
  int numComponents, vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection,
  bool isComplex /*=false*/, bool isReal /*=true*/)
{
  if (!file.SetTimeStepToRead(this->ActualTimeValue))
  {
    vtkGenericWarningMacro("couldn't correctly set time step to read. Aborting");
    return;
  }
  file.CheckForBeginTimeStepLine();

  // skip description line
  file.SkipNLines(1);
  auto result = file.ReadNextLine();
  while (result.first && result.second.find("part") != std::string::npos)
  {
    int partId = this->ReadPartId(file);
    partId--;

    // next line should be either coordinates or block
    // however it may or may not be there if there is an empty part.
    // we'll test for it and if it doesn't say coordinates or block, we'll assume
    // it's an empty part and move on
    result = file.ReadNextLine();
    auto sectionHeader = result.second;
    if (sectionHeader.find("coordinates") == std::string::npos &&
      sectionHeader.find("block") == std::string::npos)
    {
      continue;
    }

    auto it = this->PartInfoMap.find(partId);
    if (it == this->PartInfoMap.end())
    {
      vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
      return;
    }
    auto& partInfo = it->second;
    bool readPart = false;
    if (selection->ArrayIsEnabled(partInfo.Name.c_str()))
    {
      readPart = true;
    }

    if (readPart)
    {
      vtkPartitionedDataSet* pds = output->GetPartitionedDataSet(partInfo.PDCIndex);
      vtkDataSet* ds = pds->GetPartition(0);
      auto numPts = ds->GetNumberOfPoints();
      if (numPts > 0)
      {
        // Because the old reader puts the real and imaginary components into a single array
        // with 2 components in the case of scalars, we will copy that functionality here, so
        // users of the old reader can expect to have the same variable names with this reader.
        // When numComponents > 1, the real and imaginary components are always put into their
        // own vtkDataArray.
        if (isComplex && numComponents == 1)
        {
          auto tmpArray = this->ReadVariableArray(file, sectionHeader, numPts, numComponents);
          if (isReal)
          {
            vtkNew<vtkFloatArray> array;
            array->SetNumberOfComponents(2);
            array->SetNumberOfTuples(numPts);
            array->CopyComponent(0, tmpArray, 0);
            array->SetName(arrayName.c_str());
            ds->GetPointData()->AddArray(array);
            setPointDataScalarsVectors(ds, array);
          }
          else
          {
            auto array =
              vtkDataArray::SafeDownCast(ds->GetPointData()->GetAbstractArray(arrayName.c_str()));
            if (!array)
            {
              vtkGenericWarningMacro(
                "Couldn't find real component of array " << arrayName << " in part " << partId);
              return;
            }
            array->CopyComponent(1, tmpArray, 0);
          }
        }
        else
        {
          auto array = this->ReadVariableArray(file, sectionHeader, numPts, numComponents);
          array->SetName(arrayName.c_str());
          ds->GetPointData()->AddArray(array);
          setPointDataScalarsVectors(ds, array);
        }
      }
    }
    else
    {
      file.SkipNNumbers<float>(numComponents * partInfo.NumNodes);
    }
    if (file.CheckForEndTimeStepLine())
    {
      break;
    }
    result = file.ReadNextLine();
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadVariableMeasuredNodes(EnSightFile& file, const std::string& arrayName,
  int numComponents, vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection)
{
  if (!selection->ArrayIsEnabled(this->MeasuredPartName.c_str()))
  {
    return;
  }
  if (!file.SetTimeStepToRead(this->ActualTimeValue))
  {
    vtkGenericWarningMacro("couldn't correctly set time step to read. Aborting");
    return;
  }
  file.CheckForBeginTimeStepLine();

  // skip description line
  file.SkipNLines(1);

  vtkPartitionedDataSet* pds = output->GetPartitionedDataSet(this->MeasuredPartitionId);
  vtkDataSet* ds = pds->GetPartition(0);
  auto numPts = ds->GetNumberOfPoints();
  if (numPts <= 0)
  {
    return;
  }

  vtkNew<vtkFloatArray> array;
  array->SetNumberOfComponents(numComponents);
  array->SetNumberOfTuples(numPts);
  array->SetName(arrayName.c_str());

  if (file.Format == FileType::ASCII)
  {
    int ptsPerLine = 6 / numComponents;
    int numLines = numPts / ptsPerLine;
    int moreValues = numPts % ptsPerLine;
    int startPos = 0;

    for (int i = 0; i < numLines; i++)
    {
      file.ReadArray(array->WritePointer(startPos, 6), 6, true);
      startPos += 6;
    }
    if (moreValues > 0)
    {
      file.ReadArray(array->WritePointer(startPos, moreValues * numComponents),
        moreValues * numComponents, true);
    }
  }
  else
  {
    file.ReadArray(array->WritePointer(0, numPts * numComponents), numPts * numComponents);
  }
  file.CheckForEndTimeStepLine();
  ds->GetPointData()->AddArray(array);
  setPointDataScalarsVectors(ds, array);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkFloatArray> EnSightDataSet::ReadVariableArray(
  EnSightFile& file, const std::string& sectionHeader, vtkIdType numElements, int numComponents)
{
  // handles reading float arrays for variables
  // handles partial and undefined values
  // undef gets converted to NaN because that is what VTK/ParaView uses
  std::regex regEx("^[^ ]+ ([^ ]+)");
  std::smatch sm;
  const bool match = std::regex_search(sectionHeader, sm, regEx);
  const bool hasUndef = (match && sm.str(1) == "undef");
  const bool hasPartial = (match && sm.str(1) == "partial");

  float undefValue = 0;
  if (hasUndef)
  {
    file.ReadNumber(&undefValue);
  }

  vtkNew<vtkIdList> partialIndices;
  if (hasPartial)
  {
    int count;
    file.ReadNumber(&count);

    std::vector<int> buffer(count);
    file.ReadArray(&buffer.front(), count);

    partialIndices->SetNumberOfIds(count);
    std::transform(buffer.begin(), buffer.end(), partialIndices->WritePointer(0, count),
      [](vtkIdType val)
      {
        return val - 1; /* since ensight indices start with 1*/
      });
  }

  // replace undefined values with "internal undef" which in ParaView is NaN
  auto replaceUndef = [&](vtkFloatArray* farray)
  {
    if (hasUndef)
    {
      for (vtkIdType cc = 0; cc < numElements; ++cc)
      {
        if (farray->GetTypedComponent(cc, 0) == undefValue)
        {
          farray->SetTypedComponent(cc, 0, std::nanf("1"));
        }
      }
    }
  };

  auto readComponent = [&](vtkIdType count)
  {
    vtkNew<vtkFloatArray> buffer;
    buffer->SetNumberOfTuples(count);
    if (hasPartial)
    {
      // fill with NaNs
      buffer->FillValue(std::nanf("1"));

      vtkNew<vtkFloatArray> pbuffer;
      pbuffer->SetNumberOfTuples(partialIndices->GetNumberOfIds());
      file.ReadArray(pbuffer->WritePointer(0, partialIndices->GetNumberOfIds()),
        static_cast<int>(partialIndices->GetNumberOfIds()));

      // now copy the tuples over from pbuffer to buffer.
      vtkNew<vtkIdList> srcIds;
      srcIds->SetNumberOfIds(partialIndices->GetNumberOfIds());
      std::iota(srcIds->begin(), srcIds->end(), 0);
      buffer->InsertTuples(partialIndices, srcIds, pbuffer);
    }
    else
    {
      file.ReadArray(buffer->WritePointer(0, count), count);
      replaceUndef(buffer);
    }
    return buffer;
  };

  if (numComponents == 1)
  {
    return readComponent(numElements);
  }
  else if (numComponents > 1)
  {
    vtkNew<vtkFloatArray> array;
    array->SetNumberOfComponents(numComponents);
    array->SetNumberOfTuples(numElements);
    for (int comp = 0; comp < numComponents; ++comp)
    {
      const int destComponent = getDestinationComponent(comp, numComponents);
      auto buffer = readComponent(numElements);
      array->CopyComponent(destComponent, buffer, 0);
    }
    return array;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadVariableElements(EnSightFile& file, const std::string& arrayName,
  int numComponents, vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection,
  bool isComplex /*=false*/, bool isReal /*=true*/)
{
  if (!file.SetTimeStepToRead(this->ActualTimeValue))
  {
    vtkGenericWarningMacro("couldn't correctly set time step to read. Aborting");
    return;
  }
  file.CheckForBeginTimeStepLine();

  // skip description line
  file.SkipNLines(1);
  auto result = file.ReadNextLine();
  bool continueReading = result.first;
  while (continueReading && result.second.find("part") != std::string::npos)
  {
    int partId = this->ReadPartId(file);
    partId--;

    auto it = this->PartInfoMap.find(partId);
    if (it == this->PartInfoMap.end())
    {
      vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
      return;
    }
    auto& partInfo = it->second;
    bool readPart = false;
    if (selection->ArrayIsEnabled(partInfo.Name.c_str()))
    {
      readPart = true;
    }

    // next line either says block or has an element type
    result = file.ReadNextLine();
    continueReading = result.first;
    while (continueReading && isValidCellSectionHeader(result.second))
    {
      if (result.second.find("block") != std::string::npos)
      {
        if (readPart)
        {
          vtkPartitionedDataSet* pds = output->GetPartitionedDataSet(partInfo.PDCIndex);
          vtkDataSet* ds = pds->GetPartition(0);
          auto numCells = ds->GetNumberOfCells();
          // Because the old reader puts the real and imaginary components into a single array
          // with 2 components in the case of scalars, we will copy that functionality here, so
          // users of the old reader can expect to have the same variable names with this reader.
          // When numComponents > 1, the real and imaginary components are always put into their
          // own vtkDataArray.
          if (isComplex && numComponents == 1)
          {
            auto tmpArray = this->ReadVariableArray(file, result.second, numCells, numComponents);
            if (isReal)
            {
              vtkNew<vtkFloatArray> array;
              array->SetNumberOfComponents(2);
              array->SetNumberOfTuples(numCells);
              array->CopyComponent(0, tmpArray, 0);
              array->SetName(arrayName.c_str());
              ds->GetCellData()->AddArray(array);
              setCellDataScalarsVectors(ds, array);
            }
            else
            {
              auto array =
                vtkDataArray::SafeDownCast(ds->GetCellData()->GetAbstractArray(arrayName.c_str()));
              if (!array)
              {
                vtkGenericWarningMacro(
                  "Couldn't find real component of array " << arrayName << " in part " << partId);
                return;
              }
              array->CopyComponent(1, tmpArray, 0);
            }
          }
          else
          {
            auto array = this->ReadVariableArray(file, result.second, numCells, numComponents);
            array->SetName(arrayName.c_str());
            ds->GetCellData()->AddArray(array);
            setCellDataScalarsVectors(ds, array);
          }
        }
        else
        {
          file.SkipNNumbers<float>(numComponents * partInfo.NumNodes);
        }
        if (file.CheckForEndTimeStepLine())
        {
          continueReading = false;
          break;
        }
        result = file.ReadNextLine();
        continueReading = result.first;
      }
      else
      {
        if (readPart)
        {
          // so we need to know how many cells of each element type exist
          // the variable file doesn't specify, but the geometry file does
          vtkPartitionedDataSet* pds = output->GetPartitionedDataSet(partInfo.PDCIndex);
          vtkDataSet* ds = pds->GetPartition(0);
          auto numCells = ds->GetNumberOfCells();
          // This could be much simpler, but is made more complex by trying to match functionality
          // of the old reader. Because the old reader puts the real and imaginary components into a
          // single array with 2 components in the case of scalars, we will copy that functionality
          // here, so users of the old reader can expect to have the same variable names with this
          // reader. When numComponents > 1, the real and imaginary components are always put into
          // their own vtkDataArray.
          vtkSmartPointer<vtkFloatArray> array;
          if (isComplex && numComponents == 1 && !isReal)
          {
            array =
              vtkFloatArray::SafeDownCast(ds->GetCellData()->GetAbstractArray(arrayName.c_str()));
            if (!array)
            {
              vtkGenericWarningMacro(
                "Couldn't find real component of array " << arrayName << " in part " << partId);
              return;
            }
          }
          else if (isComplex && numComponents == 1 && isReal)
          {
            array = vtkSmartPointer<vtkFloatArray>::New();
            array->SetNumberOfComponents(2);
            array->SetNumberOfTuples(numCells);
            array->SetName(arrayName.c_str());
          }
          else
          {
            array = vtkSmartPointer<vtkFloatArray>::New();
            array->SetNumberOfComponents(numComponents);
            array->SetNumberOfTuples(numCells);
            array->SetName(arrayName.c_str());
          }

          int cellPos = 0;
          auto elementType = getElementTypeFromString(result.second);
          while (continueReading && elementType != ElementType::Unknown)
          {
            auto numElementCells = it->second.NumElementsPerType[static_cast<int>(elementType)];
            if (isComplex && numComponents == 1)
            {
              auto tmpSubarray =
                this->ReadVariableArray(file, result.second, numElementCells, numComponents);
              vtkNew<vtkFloatArray> subarray;
              subarray->SetNumberOfComponents(2);
              subarray->SetNumberOfTuples(numElementCells);
              if (isReal)
              {
                // We always read the real component first, so in this case, we'll copy the 0th
                // component into the subarray
                subarray->CopyComponent(0, tmpSubarray, 0);
              }
              else
              {
                // now that we're reading the imaginary component, we can get the tuples (which only
                // contains the real component at this point) into subarray, copy our array into the
                // next component. then we can set those in the actual full array.

                // GetTuples API says the second id is inclusive
                array->GetTuples(cellPos, cellPos + numElementCells - 1, subarray);
                subarray->CopyComponent(1, tmpSubarray, 0);
              }
              array->InsertTuples(cellPos, subarray->GetNumberOfTuples(), 0, subarray);
            }
            else
            {
              auto subarray =
                this->ReadVariableArray(file, result.second, numElementCells, numComponents);
              array->InsertTuples(cellPos, numElementCells, 0, subarray);
            }
            cellPos += numElementCells;

            if (file.CheckForEndTimeStepLine())
            {
              continueReading = false;
              break;
            }
            result = file.ReadNextLine();
            continueReading = result.first;
            elementType = getElementTypeFromString(result.second);
            if (!isComplex || numComponents != 1 || isReal)
            {
              ds->GetCellData()->AddArray(array);
              setCellDataScalarsVectors(ds, array);
            }
          }
          // result = file.ReadNextLine();
        }
        else
        {
          auto elementType = getElementTypeFromString(result.second);
          while (continueReading && elementType != ElementType::Unknown)
          {
            auto numElementCells = it->second.NumElementsPerType[static_cast<int>(elementType)];
            file.SkipNNumbers<float>(numComponents * numElementCells);
            if (file.CheckForEndTimeStepLine())
            {
              continueReading = false;
              break;
            }
            result = file.ReadNextLine();
            continueReading = result.first;
            elementType = getElementTypeFromString(result.second);
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadVariableConstantCase(
  VariableOptions& var, vtkPartitionedDataSetCollection* output)
{
  // in this case we may have already read the values, since they're in the case file
  // but they may also be in a separate file, in which case we'll read them in here
  // this is one value for the dataset per time step
  if (var.Type == VariableType::ConstantPerCaseFile && var.Constants.empty())
  {
    // we'll read these in the first time we call this, and just keep it cached
    var.File.OpenFile(true);
    readFileValues(var.File, var.Constants);
  }

  if (var.Constants.empty())
  {
    vtkGenericWarningMacro(
      "Variable " << var.Name << "  is a constant per case, but no values were found");
    return;
  }

  int idx = 0;
  if (var.File.TimeSet != -1)
  {
    auto info = var.File.GetTimeSetInfo();
    double timeVal = info->TimeValues[0];
    for (size_t i = 1; i < info->TimeValues.size(); ++i)
    {
      double newTime = info->TimeValues[i];
      if (newTime <= this->ActualTimeValue && newTime > timeVal)
      {
        timeVal = newTime;
        idx++;
      }
    }
  }
  vtkNew<vtkFloatArray> array;
  array->SetName(var.Name.c_str());
  array->SetNumberOfTuples(1);
  array->SetValue(0, var.Constants[idx]);
  output->GetFieldData()->AddArray(array);
}

//------------------------------------------------------------------------------
void EnSightDataSet::CreateUniformGridOutput(const GridOptions& opts, vtkUniformGrid* output)
{
  int dimensions[3];
  int numPts, numCells;

  this->ReadDimensions(opts.HasRange, dimensions, numPts, numCells);
  output->SetDimensions(dimensions);

  float origin[3];
  this->GeometryFile.ReadArray(origin, 3);
  output->SetOrigin(origin[0], origin[1], origin[2]);

  float delta[3];
  this->GeometryFile.ReadArray(delta, 3);
  output->SetSpacing(delta[0], delta[1], delta[2]);

  if (opts.IBlanked)
  {
    std::vector<int> data(numPts, 0);
    this->ReadOptionalValues(numPts, data.data());
    for (size_t i = 0; i < data.size(); i++)
    {
      if (!data[i])
      {
        output->BlankPoint(i);
      }
    }
  }

  if (opts.WithGhost)
  {
    this->ProcessGhostCells(numCells, output);
  }

  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("node_ids") != std::string::npos)
  {
    this->ProcessNodeIds(numPts, output);
    result = this->GeometryFile.ReadNextLine();
  }

  if (result.second.find("element_ids") != std::string::npos)
  {
    this->ProcessElementIds(numCells, output);
  }
  else
  {
    this->GeometryFile.GoBackOneLine();
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::PassThroughUniformGrid(const GridOptions& opts, int partId)
{
  int dimensions[3];
  auto it = this->PartInfoMap.find(partId);
  if (it == this->PartInfoMap.end())
  {
    // shouldn't happen but just in case
    vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
    return;
  }
  auto& numPts = it->second.NumNodes;
  auto& numCells = it->second.NumElements;
  this->ReadDimensions(opts.HasRange, dimensions, numPts, numCells);

  if (numPts == 0)
  {
    return;
  }

  this->GeometryFile.SkipNNumbers<float>(6);

  this->PassThroughOptionalSections(opts, numPts, numCells);
}

//------------------------------------------------------------------------------
void EnSightDataSet::CreateRectilinearGridOutput(
  const GridOptions& opts, vtkRectilinearGrid* output)
{
  int dimensions[3];
  int numPts, numCells;
  this->ReadDimensions(opts.HasRange, dimensions, numPts, numCells);
  output->SetDimensions(dimensions);

  vtkNew<vtkFloatArray> xCoords;
  vtkNew<vtkFloatArray> yCoords;
  vtkNew<vtkFloatArray> zCoords;
  xCoords->SetNumberOfTuples(dimensions[0]);
  yCoords->SetNumberOfTuples(dimensions[1]);
  zCoords->SetNumberOfTuples(dimensions[2]);

  this->GeometryFile.ReadArray(xCoords->WritePointer(0, dimensions[0]), dimensions[0]);
  this->GeometryFile.ReadArray(yCoords->WritePointer(0, dimensions[1]), dimensions[1]);
  this->GeometryFile.ReadArray(zCoords->WritePointer(0, dimensions[2]), dimensions[2]);

  output->SetXCoordinates(xCoords);
  output->SetYCoordinates(yCoords);
  output->SetZCoordinates(zCoords);

  if (opts.IBlanked)
  {
    vtkGenericWarningMacro("VTK does not handle blanking for rectilinear grids");
    std::vector<int> data(numPts, 0);
    this->ReadOptionalValues(numPts, data.data());
  }

  if (opts.WithGhost)
  {
    this->ProcessGhostCells(numCells, output);
  }

  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("node_ids") != std::string::npos)
  {
    this->ProcessNodeIds(numPts, output);
    result = this->GeometryFile.ReadNextLine();
  }

  if (result.second.find("element_ids") != std::string::npos)
  {
    this->ProcessElementIds(numCells, output);
  }
  else
  {
    this->GeometryFile.GoBackOneLine();
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::PassThroughRectilinearGrid(const GridOptions& opts, int partId)
{
  int dimensions[3];
  auto it = this->PartInfoMap.find(partId);
  if (it == this->PartInfoMap.end())
  {
    // shouldn't happen but just in case
    vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
    return;
  }
  auto& numPts = it->second.NumNodes;
  auto& numCells = it->second.NumElements;
  this->ReadDimensions(opts.HasRange, dimensions, numPts, numCells);

  // skip x, y, and z coords
  this->GeometryFile.SkipNNumbers<float>(dimensions[0] + dimensions[1] + dimensions[2]);

  this->PassThroughOptionalSections(opts, numPts, numCells);
}

//------------------------------------------------------------------------------
void EnSightDataSet::CreateStructuredGridOutput(const GridOptions& opts, vtkStructuredGrid* output)
{
  int dimensions[3];
  int numPts, numCells;
  this->ReadDimensions(opts.HasRange, dimensions, numPts, numCells);
  output->SetDimensions(dimensions);

  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(numPts);

  auto readComponent = [&](vtkIdType count)
  {
    vtkNew<vtkFloatArray> buffer;
    buffer->SetNumberOfTuples(count);
    this->GeometryFile.ReadArray(buffer->WritePointer(0, count), count);
    return buffer;
  };

  vtkNew<vtkFloatArray> ptsArray;
  ptsArray->SetNumberOfComponents(3);
  ptsArray->SetNumberOfTuples(numPts);

  for (int i = 0; i < 3; i++)
  {
    auto buffer = readComponent(numPts);
    ptsArray->CopyComponent(i, buffer, 0);
  }
  points->SetData(ptsArray);
  output->SetPoints(points);

  if (opts.IBlanked)
  {
    std::vector<int> data(numPts, 0);
    this->ReadOptionalValues(numPts, data.data());
    for (int i = 0; i < numPts; i++)
    {
      if (!data[i])
      {
        output->BlankPoint(i);
      }
    }
  }

  if (opts.WithGhost)
  {
    this->ProcessGhostCells(numCells, output);
  }

  // it's not clear in the user manual if it is required for the node id section to be preceded by
  // 'node_ids'. The old reader makes this assumption
  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("node_ids") != std::string::npos)
  {
    this->ProcessNodeIds(numPts, output);
    result = this->GeometryFile.ReadNextLine();
  }

  if (result.second.find("element_ids") != std::string::npos)
  {
    this->ProcessElementIds(numCells, output);
  }
  else
  {
    this->GeometryFile.GoBackOneLine();
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::PassThroughStructuredGrid(const GridOptions& opts, int partId)
{
  int dimensions[3];
  auto it = this->PartInfoMap.find(partId);
  if (it == this->PartInfoMap.end())
  {
    // shouldn't happen but just in case
    vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
    return;
  }
  auto& numPts = it->second.NumNodes;
  auto& numCells = it->second.NumElements;
  this->ReadDimensions(opts.HasRange, dimensions, numPts, numCells);

  this->GeometryFile.SkipNNumbers<float>(numPts * 3);

  this->PassThroughOptionalSections(opts, numPts, numCells);
}

bool EnSightDataSet::CurrentGeometryFileContainsConnectivity()
{
  if (this->GeometryCStep == -1)
  {
    // if CStep isn't set, then the connectivity is in every time step
    return true;
  }

  // now check to see if Cstep is the current file
  return this->GeometryFile.GetCurrentOpenTimeStep() == this->GeometryCStep;
}

//------------------------------------------------------------------------------
void EnSightDataSet::CreateUnstructuredGridOutput(
  const GridOptions& vtkNotUsed(opts), vtkUnstructuredGrid* output)
{
  int numPts;
  this->GeometryFile.ReadNumber(&numPts);

  if (this->NodeIdsListed)
  {
    this->ProcessNodeIds(numPts, output);
  }

  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(numPts);

  auto readComponent = [&](vtkIdType count)
  {
    vtkNew<vtkFloatArray> buffer;
    buffer->SetNumberOfTuples(count);
    this->GeometryFile.ReadArray(buffer->WritePointer(0, count), count);
    return buffer;
  };

  vtkNew<vtkFloatArray> ptsArray;
  ptsArray->SetNumberOfComponents(3);
  ptsArray->SetNumberOfTuples(numPts);

  for (int i = 0; i < 3; i++)
  {
    auto buffer = readComponent(numPts);
    ptsArray->CopyComponent(i, buffer, 0);
  }
  points->SetData(ptsArray);
  output->SetPoints(points);

  // it sounds like its possible that change_coords_only could be set, but if there is no
  // CStep set, then all time steps contain the connectivity, it just doesn't change and doesn't
  // need to be read on every step.
  // now process element(s)
  // at this point, if the geometry was change_coords_only, we may be reading from a file that
  // doesn't have the connectivity in it, in which case we'll need to use GeometryCStep to read
  // the connectivity (if we haven't already cached it)
  if (this->GeometryChangeCoordsOnly && this->MeshCache->GetStatus().CacheDefined)
  {
    // so we've already cached data in a previous step and now
    // we've updated the coordinates.
    // we should just be able to return here
    return;
  }

  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("part") != std::string::npos)
  {
    // reset this so part reading is correct when we leave this method
    this->GeometryFile.GoBackOneLine();
    return;
  }
  auto elementType = getElementTypeFromString(result.second);
  while (result.first && elementType != ElementType::Unknown)
  {
    if (elementType == ElementType::NSided || elementType == ElementType::GNSided)
    {
      int numCells;
      this->ReadNSidedSection(numCells, output);
    }
    else if (elementType == ElementType::NFaced || elementType == ElementType::GNFaced)
    {
      int numCells;
      this->ReadNFacedSection(numCells, output);
    }
    else
    {
      int numCells;
      this->GeometryFile.ReadNumber(&numCells);

      if (this->ElementIdsListed)
      {
        this->GeometryFile.SkipNNumbers<int>(numCells);
      }

      bool padBegin = true, padEnd = false;
      for (int i = 0; i < numCells; i++)
      {
        if (i == numCells - 1)
        {
          padEnd = true;
        }
        this->ReadCell(elementType, output, padBegin, padEnd);
        padBegin = false;
      }
    }
    result = this->GeometryFile.ReadNextLine();
    if (result.second.find("part") != std::string::npos)
    {
      // reset this so part reading is correct when we leave this method
      this->GeometryFile.GoBackOneLine();
      break;
    }
    if (this->GeometryFile.CheckForEndTimeStepLine())
    {
      this->GeometryFile.GoBackOneLine();
      return;
    }
    elementType = getElementTypeFromString(result.second);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::PassThroughUnstructuredGrid(const GridOptions& vtkNotUsed(opts), int partId)
{
  auto it = this->PartInfoMap.find(partId);
  if (it == this->PartInfoMap.end())
  {
    // shouldn't happen but just in case
    vtkGenericWarningMacro("Part Id " << partId << " could not be found in PartInfoMap");
    return;
  }
  auto& numPts = it->second.NumNodes;
  auto& numCellsPerType = it->second.NumElementsPerType;
  this->GeometryFile.ReadNumber(&numPts);

  if (this->NodeIdsListed)
  {
    this->GeometryFile.SkipNNumbers<int>(numPts);
  }

  // because of the way fortran binary files are, we have
  // call SkipNNumbers for each set of coordinates
  this->GeometryFile.SkipNNumbers<float>(numPts);
  this->GeometryFile.SkipNNumbers<float>(numPts);
  this->GeometryFile.SkipNNumbers<float>(numPts);

  // skip cell info
  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("part") != std::string::npos)
  {
    // reset this so part reading is correct when we leave this method
    this->GeometryFile.GoBackOneLine();
    return;
  }
  auto elementType = getElementTypeFromString(result.second);
  while (result.first && elementType != ElementType::Unknown)
  {
    if (elementType == ElementType::NSided)
    {
      this->SkipNSidedSection(numCellsPerType[static_cast<int>(elementType)]);
    }
    else if (elementType == ElementType::NFaced)
    {
      this->SkipNFacedSection(numCellsPerType[static_cast<int>(elementType)]);
    }
    else
    {
      this->GeometryFile.ReadNumber(&numCellsPerType[static_cast<int>(elementType)]);

      if (this->ElementIdsListed)
      {
        this->GeometryFile.SkipNNumbers<int>(numCellsPerType[static_cast<int>(elementType)]);
      }
      auto cellInfo = getVTKCellType(elementType);
      if (this->GeometryFile.Format == FileType::ASCII)
      {
        this->GeometryFile.SkipNNumbers<float>(
          numCellsPerType[static_cast<int>(elementType)], cellInfo.second);
      }
      else
      {
        this->GeometryFile.SkipNNumbers<float>(
          numCellsPerType[static_cast<int>(elementType)] * cellInfo.second);
      }
    }

    if (this->GeometryFile.CheckForEndTimeStepLine())
    {
      this->GeometryFile.GoBackOneLine();
      return;
    }
    result = this->GeometryFile.ReadNextLine();
    if (result.second.find("part") != std::string::npos)
    {
      // reset this so part reading is correct when we leave this method
      this->GeometryFile.GoBackOneLine();
      break;
    }
    elementType = getElementTypeFromString(result.second);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::PassThroughOptionalSections(const GridOptions& opts, int numPts, int numCells)
{
  if (opts.IBlanked)
  {
    this->GeometryFile.SkipNNumbers<int>(numPts);
  }

  if (opts.WithGhost)
  {
    this->CheckForOptionalHeader("ghost_flags");
    this->GeometryFile.SkipNNumbers<int>(numCells);
  }

  // some test files specify node id given or element id given, but then actually
  // don't contain those ids
  auto result = this->GeometryFile.ReadNextLine();
  if (result.second.find("node_ids") != std::string::npos)
  {
    this->GeometryFile.SkipNNumbers<int>(numPts);
    result = this->GeometryFile.ReadNextLine();
  }

  if (result.second.find("element_ids") != std::string::npos)
  {
    this->GeometryFile.SkipNNumbers<int>(numCells);
  }
  else
  {
    this->GeometryFile.GoBackOneLine();
  }
}

//------------------------------------------------------------------------------
int EnSightDataSet::ReadPartId(EnSightFile& file)
{
  int partId;
  file.ReadNumber(&partId);
  if (file.Format != FileType::ASCII && file.ByteOrder == Endianness::Unknown)
  {
    file.DetectByteOrder(&partId);
  }
  return partId;
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadDimensions(bool hasRange, int dimensions[3], int& numPts, int& numCells)
{
  if (this->GeometryFile.Format == FileType::ASCII)
  {
    auto result = this->GeometryFile.ReadNextLine();
    for (int i = 0; i < 3; i++)
    {
      extractLinePart(GetNumRegEx(), result.second, dimensions[i]);
    }
  }
  else
  {
    this->GeometryFile.ReadArray(dimensions, 3);
  }

  if (hasRange)
  {
    int range[6];
    this->ReadRange(range);
    // range contains: imin, imax, jmin, jmax, kmin, kmax
    dimensions[0] = range[1] - range[0] + 1;
    dimensions[1] = range[3] - range[2] + 1;
    dimensions[2] = range[5] - range[4] + 1;
  }

  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  if (numPts == 0)
  {
    numCells = 0;
    return;
  }

  numCells = 1;
  for (int i = 0; i < 3; i++)
  {
    if (dimensions[i] > 1)
    {
      numCells *= (dimensions[i] - 1);
    }
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadRange(int range[6])
{
  if (this->GeometryFile.Format == FileType::ASCII)
  {
    auto result = this->GeometryFile.ReadNextLine();
    for (int i = 0; i < 6; i++)
    {
      extractLinePart(GetNumRegEx(), result.second, range[i]);
    }
  }
  else
  {
    this->GeometryFile.ReadArray(range, 6);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadOptionalValues(int numVals, int* array, std::string sectionName)
{
  this->CheckForOptionalHeader(sectionName);
  this->GeometryFile.ReadArray(array, numVals);
}

//------------------------------------------------------------------------------
void EnSightDataSet::CheckForOptionalHeader(const std::string& sectionName)
{
  // some data has an optional string before it. e.g., for ghost flags,
  // there may be a string "ghost_flags" preceding it
  if (!sectionName.empty())
  {
    auto result = this->GeometryFile.ReadNextLine();
    if (result.second.find(sectionName) == std::string::npos)
    {
      this->GeometryFile.GoBackOneLine();
    }
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadCell(ElementType eType, vtkUnstructuredGrid* output,
  bool padBegin /* = false*/, bool padEnd /* = false*/)
{
  auto retVal = getVTKCellType(eType);
  this->ReadCell(retVal.first, retVal.second, output, padBegin, padEnd);
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadCell(int cellType, int numNodes, vtkUnstructuredGrid* output,
  bool padBegin /* = false*/, bool padEnd /* = false*/)
{
  if (cellType == -1)
  {
    vtkGenericWarningMacro("ReadCell: not a valid vtk cell type");
    return;
  }
  if (numNodes == 0)
  {
    vtkGenericWarningMacro("This cell type has not been implemented yet");
    return;
  }

  int* tempNodeIds = new int[numNodes];
  this->GeometryFile.ReadArray(tempNodeIds, numNodes, true, padBegin, padEnd);

  if (output)
  {
    // we can pass nullptr, in which case we just want to pass through
    vtkIdType* nodeIds = new vtkIdType[numNodes];
    for (int i = 0; i < numNodes; i++)
    {
      nodeIds[i] = tempNodeIds[i] - 1;
    }
    if (cellType == VTK_POLYHEDRON)
    {
      vtkGenericWarningMacro("ReadCell should not be called for polyhedron");
    }
    else
    {
      output->InsertNextCell(cellType, numNodes, nodeIds);
    }
    delete[] nodeIds;
  }
  delete[] tempNodeIds;
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadNSidedSection(int& numElements, vtkUnstructuredGrid* output)
{
  this->GeometryFile.ReadNumber(&numElements);

  if (this->ElementIdsListed)
  {
    this->GeometryFile.SkipNNumbers<int>(numElements);
  }

  int* numNodesPerElement = new int[numElements];
  this->GeometryFile.ReadArray(numNodesPerElement, numElements);

  auto cellInfo = getVTKCellType(ElementType::NSided);
  bool padBegin = true, padEnd = false;
  for (int elem = 0; elem < numElements; elem++)
  {
    if (elem == numElements - 1)
    {
      padEnd = true;
    }
    auto numNodes = numNodesPerElement[elem];
    this->ReadCell(cellInfo.first, numNodes, output, padBegin, padEnd);
    padBegin = false; // should only be true on 1st iteration
  }
  delete[] numNodesPerElement;
}

//------------------------------------------------------------------------------
void EnSightDataSet::SkipNFacedSection(int& numElements)
{
  this->GeometryFile.ReadNumber(&numElements);

  // (optional) Element IDs
  if (this->ElementIdsListed)
  {
    this->GeometryFile.SkipNNumbers<int>(numElements);
  }

  // Number of faces per element
  std::vector<int> numFacesPerElement(numElements, 0);
  this->GeometryFile.ReadArray(numFacesPerElement.data(), numElements);

  vtkIdType totalNumFaces = std::accumulate(
    numFacesPerElement.begin(), numFacesPerElement.end(), static_cast<vtkIdType>(0));

  if (this->GeometryFile.Format == FileType::ASCII)
  {
    for (vtkIdType i = 0; i < totalNumFaces; ++i)
    {
      // Skip 2 lines: number of point per face per element, face connectivity
      this->GeometryFile.SkipLine();
      this->GeometryFile.SkipLine();
    }
  }
  else
  {
    std::vector<int> numNodesPerFacePerElement(totalNumFaces);
    this->GeometryFile.ReadArray(numNodesPerFacePerElement.data(), totalNumFaces);

    vtkIdType totalNumNodes = std::accumulate(numNodesPerFacePerElement.begin(),
      numNodesPerFacePerElement.end(), static_cast<vtkIdType>(0));

    this->GeometryFile.SkipNNumbers<int>(totalNumNodes);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::SkipNSidedSection(int& numElements)
{
  this->GeometryFile.ReadNumber(&numElements);

  if (this->ElementIdsListed)
  {
    this->GeometryFile.SkipNNumbers<int>(numElements);
  }

  if (this->GeometryFile.Format == FileType::ASCII)
  {
    // Skip 2 lines per element: number of nodes, node numbers for this element
    for (int elementIdx = 0; elementIdx < numElements; ++elementIdx)
    {
      this->GeometryFile.SkipLine();
      this->GeometryFile.SkipLine();
    }
  }
  else
  {
    std::vector<int> numNodesPerElement(numElements);
    this->GeometryFile.ReadArray(numNodesPerElement.data(), numElements);

    vtkIdType totalNumNodes = std::accumulate(
      numNodesPerElement.begin(), numNodesPerElement.end(), static_cast<vtkIdType>(0));
    this->GeometryFile.SkipNNumbers<int>(totalNumNodes);
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ReadNFacedSection(int& numElements, vtkUnstructuredGrid* output)
{
  vtkLogScopeFunction(TRACE);

  // Number of elements
  this->GeometryFile.ReadNumber(&numElements);

  // (optional) Element IDs
  if (this->ElementIdsListed)
  {
    this->GeometryFile.SkipNNumbers<int>(numElements);
  }

  // Number of faces per element
  std::vector<int> numFacesPerElement(numElements, 0);
  this->GeometryFile.ReadArray(numFacesPerElement.data(), numElements);

  // Read the whole block in one go
  vtkIdType totalNumFaces = std::accumulate(
    numFacesPerElement.begin(), numFacesPerElement.end(), static_cast<vtkIdType>(0));

  std::vector<int> numNodesPerFacePerElement(totalNumFaces);
  this->GeometryFile.ReadArray(numNodesPerFacePerElement.data(), totalNumFaces);

  vtkIdType totalNumNodes = std::accumulate(
    numNodesPerFacePerElement.begin(), numNodesPerFacePerElement.end(), static_cast<vtkIdType>(0));
  std::vector<int> faceNodesBuffer(totalNumNodes);

  vtkIdType offset = 0;
  for (vtkIdType i = 0; i < totalNumFaces; ++i)
  {
    this->GeometryFile.ReadArray(
      faceNodesBuffer.data() + offset, numNodesPerFacePerElement[i], true);
    offset += numNodesPerFacePerElement[i];
  }

  // Now build the actual cells
  auto cellInfo = getVTKCellType(ElementType::NFaced);

  auto numNodesInFaceIt = numNodesPerFacePerElement.begin();
  auto nodeIt = faceNodesBuffer.begin();

  // Break through all loops if numNodesInFaceIt or nodeIt reach the end of vector
  bool endReached = false;
  vtkNew<vtkCellArray> faceStream;

  for (int elemIdx = 0; elemIdx < numElements; elemIdx++)
  {
    const int numFacesInElement = numFacesPerElement[elemIdx];
    /// @note: we could save that value from the earlier "total" computation. It's not significant
    // compared to the read time though
    const vtkIdType numNodesInElement = std::accumulate(
      numNodesInFaceIt, numNodesInFaceIt + numFacesInElement, static_cast<vtkIdType>(0));

    std::vector<vtkIdType> uniqueCellIDs;
    uniqueCellIDs.reserve(numNodesInElement);

    faceStream->Reset();
    faceStream->AllocateExact(numFacesInElement, numNodesInElement);

    for (int faceIdx = 0; faceIdx < numFacesInElement; ++faceIdx)
    {
      const int numNodesInFace = *numNodesInFaceIt;
      faceStream->InsertNextCell(numNodesInFace);

      for (int i = 0; i < numNodesInFace; ++i)
      {
        vtkIdType correctedId = (*nodeIt) - 1; // Ensight node IDs are 1-based
        faceStream->InsertCellPoint(correctedId);

        /// @note: We use an unsorted, unique vector instead of a set because:
        // 1) This is a per-cell unique point list; we expect it to be relatively small
        // 2) It allows us to use the insertNextCell call below which expects a contiguous container
        if (std::find(uniqueCellIDs.begin(), uniqueCellIDs.end(), correctedId) ==
          std::end(uniqueCellIDs))
        {
          uniqueCellIDs.push_back(correctedId);
        }

        ++nodeIt;
        if (nodeIt == faceNodesBuffer.end())
        {
          endReached = true;
          break;
        }
      }
      ++numNodesInFaceIt;
      if (endReached || numNodesInFaceIt == numNodesPerFacePerElement.end())
      {
        endReached = true;
        break;
      }
    }

    if (output)
    {
      output->InsertNextCell(
        cellInfo.first, uniqueCellIDs.size(), uniqueCellIDs.data(), faceStream);
    }
    if (endReached)
    {
      break;
    }
  }
}

//------------------------------------------------------------------------------
void EnSightDataSet::ProcessNodeIds(int numPts, vtkDataSet* output)
{
  vtkNew<vtkTypeInt32Array> array;
  array->SetNumberOfTuples(numPts);
  array->SetName("Node Ids");
  this->ReadOptionalValues(numPts, array->WritePointer(0, numPts), "node_ids");
  vtkPointData* pointData = output->GetPointData();
  pointData->SetGlobalIds(array);
}

//------------------------------------------------------------------------------
void EnSightDataSet::ProcessElementIds(int numCells, vtkDataSet* output)
{
  vtkNew<vtkTypeInt32Array> array;
  array->SetNumberOfTuples(numCells);
  array->SetName("Element Ids");
  this->ReadOptionalValues(numCells, array->WritePointer(0, numCells), "element_ids");
  vtkCellData* cellData = output->GetCellData();
  cellData->SetGlobalIds(array);
}

//------------------------------------------------------------------------------
void EnSightDataSet::ProcessGhostCells(int numCells, vtkDataSet* output)
{
  // ensight stores as int, so we'll have to read into vector and then copy over
  // to the actual array
  std::vector<int> ghostFlags(numCells, 0);
  this->ReadOptionalValues(numCells, ghostFlags.data(), "ghost_flags");
  vtkNew<vtkUnsignedCharArray> cellGhostArray;
  cellGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
  cellGhostArray->SetNumberOfComponents(1);
  cellGhostArray->SetNumberOfTuples(numCells);

  for (vtkIdType i = 0; i < numCells; i++)
  {
    cellGhostArray->SetValue(i, ghostFlags[i] ? vtkDataSetAttributes::DUPLICATECELL : 0);
  }
  output->GetCellData()->AddArray(cellGhostArray);
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ReadRigidBodyGeometryFile()
{
  if (!this->RigidBodyFile.SetFileNamePattern(this->RigidBodyFileName, true))
  {
    vtkGenericWarningMacro("Rigid body file " << this->RigidBodyFileName << " could not be opened");
    return false;
  }

  this->RigidBodyTransforms.clear();
  this->EulerTransformsMap.clear();
  this->UseEulerTimeSteps = false;
  this->EulerTimeSteps.clear();

  auto result = this->RigidBodyFile.ReadNextLine();
  if (!result.first || result.second.find("EnSight Rigid Body") == std::string::npos)
  {
    vtkGenericWarningMacro("The first line " << result.second << " is not 'EnSight Rigid Body'.");
    return false;
  }

  // read the version now
  result = this->RigidBodyFile.ReadNextLine();
  if (!result.first || result.second.find("version") == std::string::npos)
  {
    vtkGenericWarningMacro("The first line " << result.second << " is not 'EnSight Rigid Body'.");
    return false;
  }

  float version;
  extractLinePart(GetNumRegEx(), result.second, version);
  if (version != 2.0)
  {
    vtkGenericWarningMacro("currently only version 2.0 of the rigid body format is supported.");
    return false;
  }

  // read "names" or "numbers"
  result = this->RigidBodyFile.ReadNextLine();
  if (!result.first)
  {
    vtkGenericWarningMacro("There was an issue reading the names/numbers line");
    return false;
  }
  this->UsePartNamesRB = result.second.find("names") != std::string::npos;

  int numParts;
  this->RigidBodyFile.ReadNumber(&numParts);

  // read the number of following part names / part numbers
  result = this->RigidBodyFile.ReadNextLine(); // either a part name or number
  int idx = 0;
  while (result.first && idx < numParts)
  {
    // handle line which is either a part name or number
    int partId;
    std::string partName = result.second;
    sanitize(partName);
    if (!this->UsePartNamesRB)
    {
      // Need to make sure that we remove any quotes from the partId
      partId = std::stoi(partName) - 1; // EnSight starts #ing at 1.
      partName = std::to_string(partId);
    }

    // num of transformations
    int numTransformations;
    this->RigidBodyFile.ReadNumber(&numTransformations);
    vtkLog(TRACE,
      "reading transforms for part " << partName << ", which has " << numTransformations
                                     << " transformations");

    if (this->RigidBodyTransforms.count(partName))
    {
      vtkGenericWarningMacro("Parts should only be listed once in the rigid body file, but part "
        << partName << " has already been read.");
      return false;
    }
    this->RigidBodyTransforms.insert(std::make_pair(partName, PartTransforms()));
    auto& currentPartTransform = this->RigidBodyTransforms[partName];

    // now loop through transformations
    int transIdx = 0;
    bool pretransform = true;
    std::regex regEx("^[^ ]+ ([^ ]+)");
    result = this->RigidBodyFile.ReadNextLine();
    while (result.first && transIdx < numTransformations)
    {
      auto lineParts = vtksys::SystemTools::SplitString(result.second, ':');
      if (lineParts.size() != 2)
      {
        vtkGenericWarningMacro("line " << result.second << " could not be read properly");
        return false;
      }
      std::string line = lineParts[1];
      std::string lineType = lineParts[0];
      sanitize(lineType);
      if (lineType == "Eul")
      {
        // we'll handle reading this file when we finish reading this
        std::string fileName;
        if (!extractFileName(line, fileName))
        {
          vtkGenericWarningMacro("could not extract file name from " << line);
        }
        sanitize(line);
        currentPartTransform.EETFilename = fileName;
        sanitize(currentPartTransform.EETFilename);
        currentPartTransform.EETTransTitle = line;
        pretransform = false;
        transIdx++;
        result = this->RigidBodyFile.ReadNextLine();
        continue;
      }

      vtkTransform* transform;
      if (pretransform)
      {
        currentPartTransform.PreTransforms.emplace_back(vtkSmartPointer<vtkTransform>::New());
        transform = currentPartTransform.PreTransforms.back();
      }
      else
      {
        currentPartTransform.PostTransforms.emplace_back(vtkSmartPointer<vtkTransform>::New());
        transform = currentPartTransform.PostTransforms.back();
      }
      transform->PostMultiply();
      bool applyToVectors = false;

      if (lineType == "M" || lineType == "Mv")
      {
        // M matrices applied only to geometry
        // Mv matrices applied to geometry and vectors
        if (!this->ReadRigidBodyMatrixLines(line, lineType, transform, applyToVectors))
        {
          // some error happened reading the matrix lines
          return false;
        }
      }
      else
      {
        // other possibilities are all single values
        // rotations and scaling should be applied to geometry and vectors
        // translations are only applied to geometry
        sanitize(line);
        double value;
        try
        {
          value = std::stod(line);
        }
        catch (std::invalid_argument&)
        {
          vtkGenericWarningMacro("Couldn't convert line " << line << " to a double");
          return false;
        }
        vtkLog(TRACE, "Found transformation " << lineType << ", with value of " << value);
        if (lineType == "Tx")
        {
          transform->Translate(value, 0, 0);
          applyToVectors = false;
        }
        else if (lineType == "Ty")
        {
          transform->Translate(0, value, 0);
          applyToVectors = false;
        }
        else if (lineType == "Tz")
        {
          transform->Translate(0, 0, value);
          applyToVectors = false;
        }
        else if (lineType == "Sx")
        {
          transform->Scale(value, 1, 1);
          applyToVectors = true;
        }
        else if (lineType == "Sy")
        {
          transform->Scale(1, value, 1);
          applyToVectors = true;
        }
        else if (lineType == "Sz")
        {
          transform->Scale(1, 1, value);
          applyToVectors = true;
        }
        else
        {
          // everything else should be rotation
          // lineType should be one of 'Rx', 'Ry', or 'Rz' if the value is in degrees
          // or 'Rxr', 'Ryr', or 'Rzr' if the value is in radians
          if (lineType[0] != 'R')
          {
            vtkGenericWarningMacro("the transform string " << lineType << " is not valid.");
            return false;
          }
          applyToVectors = true;

          if (lineType.size() == 3 && lineType[2] == 'r')
          {
            // convert radians to degrees
            value = vtkMath::DegreesFromRadians(value);
          }

          switch (lineType[1])
          {
            case 'x':
              transform->RotateX(value);
              break;
            case 'y':
              transform->RotateY(value);
              break;
            case 'z':
              transform->RotateZ(value);
              break;
            default:
              vtkGenericWarningMacro("couldn't determine rotation type");
          }
        }
      }

      if (pretransform)
      {
        currentPartTransform.PreTransformsApplyToVectors.push_back(applyToVectors);
      }
      else
      {
        currentPartTransform.PostTransformsApplyToVectors.push_back(applyToVectors);
      }

      transIdx++;
      result = this->RigidBodyFile.ReadNextLine();
    }

    if (currentPartTransform.EETFilename.empty() || currentPartTransform.EETTransTitle.empty())
    {
      vtkGenericWarningMacro("Every part in a rigid body file must have an 'Eul:' line");
      return false;
    }

    idx++;
    if (!result.first)
    {
      // last read was EOF
      break;
    }
  }

  // It's possible that these files could be stored in a different directory from the
  // case file. the erb file will have a path relative to the case file, while the
  // eet file has a path relative to the erb. for example with the following directory:
  // - output.case
  // - data/output.erb
  // - data/output.eet
  // So in the case file, the path to the erb file will say 'data/output.erb'
  // while in the erb file, the eet file will just say 'output.eet'.
  std::vector<std::string> filenameComponents;
  vtksys::SystemTools::SplitPath(this->RigidBodyFileName, filenameComponents);
  auto path =
    vtksys::SystemTools::JoinPath(filenameComponents.begin(), filenameComponents.end() - 1);
  return this->ReadRigidBodyEulerParameterFile(path);
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ReadRigidBodyMatrixLines(
  std::string& line, const std::string& transType, vtkTransform* transform, bool& applyToVectors)
{
  // reads all 4 matrix lines into a vtkMatrix4x4 and concatenates it into transform
  if (transType[0] != 'M')
  {
    vtkGenericWarningMacro("The transform type " << transType << " should be a matrix");
    return false;
  }

  applyToVectors = vtksys::SystemTools::StringStartsWith(transType, "Mv");

  vtkNew<vtkMatrix4x4> matrix;
  std::stringstream ss(line);
  for (int row = 0; row < 4; ++row)
  {
    if (row != 0)
    {
      auto result = this->RigidBodyFile.ReadNextLine();
      ss.str(result.second);
    }

    for (int col = 0; col < 4; ++col)
    {
      double value;
      ss >> value;
      // based on the example in the EnSight user manual, it seems we need to do the
      // transform of the matrix as its given in the erb file
      matrix->SetElement(col, row, value);
    }
  }
  transform->Concatenate(matrix);
  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ReadRigidBodyEulerParameterFile(const std::string& path)
{
  // according to EnSight User manual, although the format technically allows for different
  // .eet files for different parts, EnSight can only handle one per model, so we'll just grab
  // the file name info from the first part in this->RigidBodyTransforms.
  // If this changes in a future version, we can update this to read multiple eet files.
  auto filename = this->RigidBodyTransforms.begin()->second.EETFilename;

  if (filename.empty())
  {
    vtkGenericWarningMacro("An euler parameter file must be specified in the rigid body file.");
    return false;
  }

  auto fullFileName = path + "/" + filename;
  if (!this->EETFile.SetFileNamePattern(fullFileName, true))
  {
    vtkGenericWarningMacro("the file " << fullFileName << " could not be opened");
    return false;
  }

  // first line should be "Ens_Euler"
  auto result = this->EETFile.ReadNextLine();
  if (!result.first || result.second.find("Ens_Euler") == std::string::npos)
  {
    vtkGenericWarningMacro("The first line " << result.second << " is not 'Ens_Euler'.");
    return false;
  }

  result = this->EETFile.ReadNextLine();
  if (!result.first || result.second.find("NumTimes:") == std::string::npos)
  {
    vtkGenericWarningMacro("The second line " << result.second << " is not 'NumTimes:'.");
    return false;
  }

  // line should contain the number of time steps in the file
  int numTimes;
  if (!this->EETFile.ReadNumber(&numTimes))
  {
    vtkGenericWarningMacro("Unable to read number of time steps in eet file");
    return false;
  }
  vtkLog(TRACE, "number of timesteps: " << numTimes);

  // if we don't have any time info from regular time sets, then we'll create time steps
  // using the euler transformations
  this->UseEulerTimeSteps = this->TimeSetInfoMap.empty();
  if (this->UseEulerTimeSteps)
  {
    this->EulerTimeSteps.resize(numTimes);
  }

  result = this->EETFile.ReadNextLine();
  if (!result.first || result.second.find("NumTrans:") == std::string::npos)
  {
    vtkGenericWarningMacro("The line " << result.second << " should be 'NumTrans:'.");
    return false;
  }

  int numTrans;
  if (!this->EETFile.ReadNumber(&numTrans))
  {
    vtkGenericWarningMacro("Unable to read number of time steps in eet file");
    return false;
  }
  vtkLog(TRACE, "number of transformations: " << numTrans);

  result = this->EETFile.ReadNextLine();
  if (!result.first || result.second.find("Titles:") == std::string::npos)
  {
    vtkGenericWarningMacro("The line " << result.second << " should be 'Titles:'.");
    return false;
  }

  std::vector<std::string> titles;
  for (int i = 0; i < numTrans; ++i)
  {
    result = this->EETFile.ReadNextLine();
    if (!result.first)
    {
      vtkGenericWarningMacro("Unable to read correct number of titles");
      return false;
    }
    // sanitize the title name just in case of any trailing whitespace or quotes
    sanitize(result.second);
    titles.emplace_back(result.second);
    this->EulerTransformsMap[result.second] = TimeToEulerTransMapType();
  }

  // rest of file is Time Step sections
  result = this->EETFile.ReadNextLine();
  int timeIdx = 0;
  while (result.first && timeIdx < numTimes)
  {
    if (result.second.find("Time Step:") == std::string::npos)
    {
      vtkGenericWarningMacro("The line " << result.second << " should be 'Time Step:'");
      return false;
    }

    double time;
    this->EETFile.ReadNumber(&time);
    if (this->UseEulerTimeSteps)
    {
      this->EulerTimeSteps[timeIdx] = time;
    }

    for (int transIdx = 0; transIdx < numTrans; ++transIdx)
    {
      const auto& title = titles[transIdx];
      if (this->EulerTransformsMap.count(title) == 0)
      {
        vtkGenericWarningMacro(
          "The EulerTransformsMap for title " << title << " could not be found");
        return false;
      }
      auto& titleMap = this->EulerTransformsMap[title];

      float values[7];
      if (!this->EETFile.ReadArray(values, 7, true))
      {
        vtkGenericWarningMacro("Unable to read line containing euler parameters");
        return false;
      }

      // each line should have 7 floats:
      // 3 translations in x, y, z and 4 euler parameters
      double tx = values[0];
      double ty = values[1];
      double tz = values[2];
      double e0 = values[3];
      double e1 = values[4];
      double e2 = values[5];
      double e3 = values[6];

      vtkNew<vtkTransform> transform;
      transform->PostMultiply();
      vtkNew<vtkMatrix4x4> eulerRotation;
      eulerRotation->Identity();
      // see https://mathworld.wolfram.com/EulerParameters.html
      // for details. the elements in the matrix are eqns 18-26
      eulerRotation->SetElement(0, 0, e0 * e0 + e1 * e1 - e2 * e2 - e3 * e3);
      eulerRotation->SetElement(0, 1, 2 * (e1 * e2 + e0 * e3));
      eulerRotation->SetElement(0, 2, 2 * (e1 * e3 - e0 * e2));
      eulerRotation->SetElement(1, 0, 2 * (e1 * e2 - e0 * e3));
      eulerRotation->SetElement(1, 1, e0 * e0 - e1 * e1 + e2 * e2 - e3 * e3);
      eulerRotation->SetElement(1, 2, 2 * (e2 * e3 + e0 * e1));
      eulerRotation->SetElement(2, 0, 2 * (e1 * e3 + e0 * e2));
      eulerRotation->SetElement(2, 1, 2 * (e2 * e3 - e0 * e1));
      eulerRotation->SetElement(2, 2, e0 * e0 - e1 * e1 - e2 * e2 + e3 * e3);
      transform->Concatenate(eulerRotation);
      // translations should be done after the euler rotation
      transform->Translate(tx, ty, tz);

      titleMap[time] = transform;
    }

    result = this->EETFile.ReadNextLine();
    timeIdx++;
  }
  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::ApplyRigidBodyTransforms(int partId, std::string partName, vtkDataSet* output)
{
  if (!this->UsePartNamesRB)
  {
    // need to first convert part id to a string and use that as the partName
    partName = std::to_string(partId);
  }
  if (this->RigidBodyTransforms.find(partName) == this->RigidBodyTransforms.end())
  {
    // this isn't an error, we just don't have a transform to apply to this part
    return true;
  }

  // first we need to concatenate pretransforms, euler transforms, and post transforms
  // We have to apply some transforms with TransformAllInputVectors on and some with it off.
  const auto& partTransforms = this->RigidBodyTransforms[partName];

  std::vector<vtkSmartPointer<vtkTransformFilter>> transformPipeline;
  // first check to see if we have any pretransforms
  for (unsigned int i = 0; i < partTransforms.PreTransforms.size(); i++)
  {
    transformPipeline.push_back(vtkSmartPointer<vtkTransformFilter>::New());
    vtkTransformFilter* filter = transformPipeline.back();

    if (i == 0)
    {
      filter->SetInputData(output);
    }
    else
    {
      filter->SetInputConnection(transformPipeline[i - 1]->GetOutputPort(0));
    }

    filter->SetTransform(partTransforms.PreTransforms[i]);

    if (partTransforms.PreTransformsApplyToVectors[i])
    {
      filter->TransformAllInputVectorsOn();
    }
  }

  // now find the correct euler transform
  auto eulerTitle = partTransforms.EETTransTitle;
  // need to make sure we don't have quotes or trailing whitespace even though it's not a filename
  sanitize(eulerTitle);
  if (this->EulerTransformsMap.find(eulerTitle) == this->EulerTransformsMap.end())
  {
    vtkGenericWarningMacro("could not find '" << eulerTitle << "' in the EulerTransformsMap.");
    return false;
  }

  auto& titleMap = this->EulerTransformsMap[eulerTitle];
  if (titleMap.find(this->ActualTimeValue) == titleMap.end())
  {
    vtkGenericWarningMacro("could not find time step " << this->ActualTimeValue
                                                       << " in the euler transformations map"
                                                          " for part '"
                                                       << partName << "' with title '" << eulerTitle
                                                       << "'");
    return false;
  }

  auto eulerTransform = this->EulerTransformsMap[eulerTitle][this->ActualTimeValue];
  transformPipeline.push_back(vtkSmartPointer<vtkTransformFilter>::New());
  vtkTransformFilter* filter = transformPipeline.back();
  if (transformPipeline.size() > 1)
  {
    filter->SetInputConnection(transformPipeline[transformPipeline.size() - 2]->GetOutputPort(0));
  }
  else
  {
    filter->SetInputData(output);
  }
  filter->SetTransform(eulerTransform);

  // now handle any post transforms
  for (unsigned int i = 0; i < partTransforms.PostTransforms.size(); i++)
  {
    // there's always at least 1 transform in the pipeline at this point
    auto prevTransFilter = transformPipeline.back();

    transformPipeline.push_back(vtkSmartPointer<vtkTransformFilter>::New());
    vtkTransformFilter* curFilter = transformPipeline.back();

    curFilter->SetInputConnection(prevTransFilter->GetOutputPort(0));
    curFilter->SetTransform(partTransforms.PostTransforms[i]);

    if (partTransforms.PostTransformsApplyToVectors[i])
    {
      curFilter->TransformAllInputVectorsOn();
    }
  }

  transformPipeline.back()->Update();
  output->ShallowCopy(transformPipeline.back()->GetOutput());
  return true;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::HasRigidBodyFile()
{
  return !this->RigidBodyFileName.empty();
}

//------------------------------------------------------------------------------
bool EnSightDataSet::UseRigidBodyTimeSteps()
{
  return this->UseEulerTimeSteps;
}

//------------------------------------------------------------------------------
std::vector<double> EnSightDataSet::GetEulerTimeSteps()
{
  return this->EulerTimeSteps;
}

//------------------------------------------------------------------------------
void EnSightDataSet::SetActualTimeValue(double time)
{
  this->ActualTimeValue = time;
}

//------------------------------------------------------------------------------
bool EnSightDataSet::UseStaticMeshCache() const
{
  return this->IsStaticGeometry || this->GeometryChangeCoordsOnly;
}

//------------------------------------------------------------------------------
vtkDataObjectMeshCache* EnSightDataSet::GetMeshCache()
{
  return this->MeshCache;
}

//------------------------------------------------------------------------------
void EnSightDataSet::SetPartOfSOSFile(bool partOfSOS)
{
  this->PartOfSOSFile = partOfSOS;
}

//------------------------------------------------------------------------------
void EnSightDataSet::SetPDCInfoForLoadedParts(
  vtkSmartPointer<vtkIdTypeArray> indices, vtkSmartPointer<vtkStringArray> names)
{
  for (int i = 0; i < indices->GetNumberOfValues(); i++)
  {
    auto index = indices->GetValue(i);
    if (index != -1)
    {
      this->NumberOfLoadedParts++;
      if (this->PartInfoMap.count(i))
      {
        auto& partInfo = this->PartInfoMap[i];
        partInfo.PDCIndex = index;
      }
      else
      {
        // in this case, this casefile didn't find any info on this part during GetPartInfo()
        // so we'll just update PartInfoMap with it.
        auto retval = this->PartInfoMap.insert(std::make_pair(i, PartInfo()));
        auto& partInfo = retval.first->second;
        partInfo.PDCIndex = index;
        partInfo.Name = names->GetValue(partInfo.PDCIndex);
      }
      if (names->GetValue(index) == this->MeasuredPartName)
      {
        this->MeasuredPartitionId = index;
        vtkLog(TRACE, "Setting measured partition id to " << this->MeasuredPartitionId);
      }
    }
  }
  this->LoadedPartNames = names;
}

VTK_ABI_NAMESPACE_END
} // end namespace ensight_gold
