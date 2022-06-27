/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFLUENTReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Brian W. Dotson & Terry E. Jordan (Department of Energy, National
// Energy Technology Laboratory) & Douglas McCorkle (Iowa State University)
// who developed this class.
//
// Please address all comments to Brian Dotson (brian.dotson@netl.doe.gov) &
// Terry Jordan (terry.jordan@sa.netl.doe.gov)
// & Doug McCorkle (mccdo@iastate.edu)

#include "vtkFLUENTReader.h"
#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConvexPointSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkEndian.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkHexahedron.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkWedge.h"
#include "vtksys/Encoding.hxx"
#include "vtksys/FStream.hxx"

#include "fstream"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <cctype>
#include <sys/stat.h>

vtkStandardNewMacro(vtkFLUENTReader);

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

// Structures
struct vtkFLUENTReader::Cell
{
  int type;
  int zone;
  std::vector<int> faces;
  int parent;
  int child;
  std::vector<int> nodes;
};

struct vtkFLUENTReader::Face
{
  int type;
  unsigned int zone;
  std::vector<int> nodes;
  int c0;
  int c1;
  int periodicShadow;
  int parent;
  int child;
  int interfaceFaceParent;
  int interfaceFaceChild;
  int ncgParent;
  int ncgChild;
};

struct vtkFLUENTReader::ScalarDataChunk
{
  int subsectionId;
  vtkIdType zoneId;
  std::vector<double> scalarData;
};

struct vtkFLUENTReader::VectorDataChunk
{
  int subsectionId;
  vtkIdType zoneId;
  std::vector<double> iComponentData;
  std::vector<double> jComponentData;
  std::vector<double> kComponentData;
};

struct vtkFLUENTReader::stdString
{
  std::string value;
};
struct vtkFLUENTReader::intVector
{
  std::vector<int> value;
};
struct vtkFLUENTReader::doubleVector
{
  std::vector<double> value;
};
struct vtkFLUENTReader::stringVector
{
  std::vector<std::string> value;
};
struct vtkFLUENTReader::cellVector
{
  std::vector<Cell> value;
};
struct vtkFLUENTReader::faceVector
{
  std::vector<Face> value;
};
struct vtkFLUENTReader::stdMap
{
  std::map<size_t, std::string> value;
};
struct vtkFLUENTReader::scalarDataVector
{
  std::vector<ScalarDataChunk> value;
};
struct vtkFLUENTReader::vectorDataVector
{
  std::vector<VectorDataChunk> value;
};
struct vtkFLUENTReader::intVectorVector
{
  std::vector<std::vector<int>> value;
};

//------------------------------------------------------------------------------
vtkFLUENTReader::vtkFLUENTReader()
{
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->FileName = nullptr;
  this->NumberOfCells = 0;
  this->NumberOfCellArrays = 0;

  this->FluentCaseFile = nullptr;
  this->FluentDataFile = nullptr;
  this->CaseBuffer = new stdString;
  this->DataBuffer = new stdString;

  this->Points = vtkPoints::New();
  this->Triangle = vtkTriangle::New();
  this->Tetra = vtkTetra::New();
  this->Quad = vtkQuad::New();
  this->Hexahedron = vtkHexahedron::New();
  this->Pyramid = vtkPyramid::New();
  this->Wedge = vtkWedge::New();
  this->ConvexPointSet = vtkConvexPointSet::New();

  this->Cells = new cellVector;
  this->Faces = new faceVector;
  this->VariableNames = new stdMap;
  this->CellZones = new intVector;
  this->ScalarDataChunks = new scalarDataVector;
  this->VectorDataChunks = new vectorDataVector;

  this->SubSectionZones = new intVectorVector;
  this->SubSectionIds = new intVector;
  this->SubSectionSize = new intVector;

  this->ScalarVariableNames = new stringVector;
  this->ScalarSubSectionIds = new intVector;
  this->VectorVariableNames = new stringVector;
  this->VectorSubSectionIds = new intVector;

  this->SwapBytes = 0;
  this->GridDimension = 0;
  this->DataPass = 0;
  this->NumberOfScalars = 0;
  this->NumberOfVectors = 0;

  this->SetNumberOfInputPorts(0);
  this->SetDataByteOrderToLittleEndian();
}

//------------------------------------------------------------------------------
vtkFLUENTReader::~vtkFLUENTReader()
{
  this->Points->Delete();
  this->Triangle->Delete();
  this->Tetra->Delete();
  this->Quad->Delete();
  this->Hexahedron->Delete();
  this->Pyramid->Delete();
  this->Wedge->Delete();
  this->ConvexPointSet->Delete();

  delete this->CaseBuffer;
  delete this->DataBuffer;
  delete this->Cells;
  delete this->Faces;
  delete this->VariableNames;
  delete this->CellZones;
  delete this->ScalarDataChunks;
  delete this->VectorDataChunks;
  delete this->SubSectionZones;
  delete this->SubSectionIds;
  delete this->SubSectionSize;
  delete this->ScalarVariableNames;
  delete this->ScalarSubSectionIds;
  delete this->VectorVariableNames;
  delete this->VectorSubSectionIds;
  delete this->FluentCaseFile;
  delete this->FluentDataFile;

  this->CellDataArraySelection->Delete();

  delete[] this->FileName;
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

  output->SetNumberOfBlocks(static_cast<unsigned int>(this->CellZones->value.size()));
  // vtkUnstructuredGrid *Grid[CellZones.size()];

  std::vector<vtkUnstructuredGrid*> grid;
  grid.resize(this->CellZones->value.size());

  for (size_t test = 0; test < this->CellZones->value.size(); test++)
  {
    grid[test] = vtkUnstructuredGrid::New();
  }

  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    size_t location = std::find(this->CellZones->value.begin(), this->CellZones->value.end(),
                        this->Cells->value[i].zone) -
      this->CellZones->value.begin();
    if (this->Cells->value[i].type == 1)
    {
      for (int j = 0; j < 3; j++)
      {
        this->Triangle->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Triangle->GetCellType(), this->Triangle->GetPointIds());
    }
    else if (this->Cells->value[i].type == 2)
    {
      for (int j = 0; j < 4; j++)
      {
        this->Tetra->GetPointIds()->SetId(j, Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Tetra->GetCellType(), this->Tetra->GetPointIds());
    }
    else if (this->Cells->value[i].type == 3)
    {
      for (int j = 0; j < 4; j++)
      {
        this->Quad->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Quad->GetCellType(), this->Quad->GetPointIds());
    }
    else if (this->Cells->value[i].type == 4)
    {
      for (int j = 0; j < 8; j++)
      {
        this->Hexahedron->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(
        this->Hexahedron->GetCellType(), this->Hexahedron->GetPointIds());
    }
    else if (this->Cells->value[i].type == 5)
    {
      for (int j = 0; j < 5; j++)
      {
        this->Pyramid->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Pyramid->GetCellType(), this->Pyramid->GetPointIds());
    }
    else if (this->Cells->value[i].type == 6)
    {
      for (int j = 0; j < 6; j++)
      {
        this->Wedge->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Wedge->GetCellType(), this->Wedge->GetPointIds());
    }
    else if (this->Cells->value[i].type == 7)
    {
      this->ConvexPointSet->GetPointIds()->SetNumberOfIds(
        static_cast<vtkIdType>(this->Cells->value[i].nodes.size()));
      for (size_t j = 0; j < this->Cells->value[i].nodes.size(); j++)
      {
        this->ConvexPointSet->GetPointIds()->SetId(
          static_cast<vtkIdType>(j), this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(
        this->ConvexPointSet->GetCellType(), this->ConvexPointSet->GetPointIds());
    }
  }
  //  this->Cells->value.clear();

  // Scalar Data
  for (size_t l = 0; l < this->ScalarDataChunks->value.size(); l++)
  {
    size_t location = std::find(this->CellZones->value.begin(), this->CellZones->value.end(),
                        this->ScalarDataChunks->value[l].zoneId) -
      this->CellZones->value.begin();

    vtkDoubleArray* v = vtkDoubleArray::New();
    for (size_t m = 0; m < this->ScalarDataChunks->value[l].scalarData.size(); m++)
    {
      v->InsertValue(static_cast<vtkIdType>(m), this->ScalarDataChunks->value[l].scalarData[m]);
    }
    // v->SetName(this->ScalarVariableNames->
    //           value[l/this->CellZones->value.size()].c_str());
    v->SetName(this->VariableNames->value[this->ScalarDataChunks->value[l].subsectionId].c_str());
    grid[location]->GetCellData()->AddArray(v);
    v->Delete();
  }
  this->ScalarDataChunks->value.clear();

  // Vector Data
  for (size_t l = 0; l < this->VectorDataChunks->value.size(); l++)
  {
    size_t location = std::find(this->CellZones->value.begin(), this->CellZones->value.end(),
                        this->VectorDataChunks->value[l].zoneId) -
      this->CellZones->value.begin();
    vtkDoubleArray* v = vtkDoubleArray::New();
    v->SetNumberOfComponents(3);
    for (size_t m = 0; m < this->VectorDataChunks->value[l].iComponentData.size(); m++)
    {
      v->InsertComponent(
        static_cast<vtkIdType>(m), 0, this->VectorDataChunks->value[l].iComponentData[m]);
      v->InsertComponent(
        static_cast<vtkIdType>(m), 1, this->VectorDataChunks->value[l].jComponentData[m]);
      v->InsertComponent(
        static_cast<vtkIdType>(m), 2, this->VectorDataChunks->value[l].kComponentData[m]);
    }
    // v->SetName(this->VectorVariableNames->
    //           value[l/this->CellZones->value.size()].c_str());
    v->SetName(this->VariableNames->value[this->VectorDataChunks->value[l].subsectionId].c_str());
    grid[location]->GetCellData()->AddArray(v);
    v->Delete();
  }
  this->VectorDataChunks->value.clear();

  for (size_t addTo = 0; addTo < this->CellZones->value.size(); addTo++)
  {
    grid[addTo]->SetPoints(Points);
    output->SetBlock(static_cast<unsigned int>(addTo), grid[addTo]);
    grid[addTo]->Delete();
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (!this->OpenCaseFile(this->FileName))
  {
    vtkErrorMacro("Unable to open cas file.");
    return 0;
  }

  int dat_file_opened = this->OpenDataFile(this->FileName);
  if (!dat_file_opened)
  {
    vtkWarningMacro("Unable to open dat file.");
  }

  this->LoadVariableNames();
  this->ParseCaseFile(); // Reads Necessary Information from the .cas file.
  this->CleanCells();    //  Removes unnecessary faces from the cells.
  this->PopulateCellNodes();
  this->GetNumberOfCellZones();
  this->NumberOfScalars = 0;
  this->NumberOfVectors = 0;
  if (dat_file_opened)
  {
    this->ParseDataFile();
  }
  for (size_t i = 0; i < this->SubSectionIds->value.size(); i++)
  {
    if (this->SubSectionSize->value[i] == 1)
    {
      this->CellDataArraySelection->AddArray(
        this->VariableNames->value[this->SubSectionIds->value[i]].c_str());
      this->ScalarVariableNames->value.push_back(
        this->VariableNames->value[this->SubSectionIds->value[i]]);
      this->ScalarSubSectionIds->value.push_back(this->SubSectionIds->value[i]);
    }
    else if (this->SubSectionSize->value[i] == 3)
    {
      this->CellDataArraySelection->AddArray(
        this->VariableNames->value[this->SubSectionIds->value[i]].c_str());
      this->VectorVariableNames->value.push_back(
        this->VariableNames->value[this->SubSectionIds->value[i]]);
      this->VectorSubSectionIds->value.push_back(this->SubSectionIds->value[i]);
    }
  }
  this->NumberOfCells = static_cast<vtkIdType>(this->Cells->value.size());
  return 1;
}

//------------------------------------------------------------------------------
bool vtkFLUENTReader::OpenCaseFile(const char* filename)
{
  std::ios_base::openmode mode = ios::in;
#ifdef _WIN32
  mode |= ios::binary;
#endif
  this->FluentCaseFile = new vtksys::ifstream(filename, mode);
  return !this->FluentCaseFile->fail();
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkFLUENTReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
bool vtkFLUENTReader::OpenDataFile(const char* filename)
{
  std::string dfilename(filename);
  dfilename.erase(dfilename.length() - 3, 3);
  dfilename.append("dat");

  std::ios_base::openmode mode = ios::in;
#ifdef _WIN32
  mode |= ios::binary;
#endif
  this->FluentDataFile = new vtksys::ifstream(dfilename.c_str(), mode);
  if (this->FluentDataFile->fail())
  {
    vtkErrorMacro("Could not open data file "
      << dfilename << "associated with cas file " << filename
      << ". Please verify the cas and dat files have the same base name.");
    return false;
  }
  else
  {
    return true;
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetCaseChunk()
{
  this->CaseBuffer->value = ""; // Clear buffer

  //
  // Look for beginning of chunk
  //
  while (this->FluentCaseFile->peek() != '(')
  {
    this->FluentCaseFile->get();
    if (this->FluentCaseFile->eof())
    {
      return 0;
    }
  }

  //
  // Figure out whether this is a binary or ascii chunk.
  // If the index is 3 digits or more, then binary, otherwise ascii.
  //
  std::string index;
  while (this->FluentCaseFile->peek() != ' ')
  {
    // index.push_back(this->FluentCaseFile->peek());
    index += static_cast<char>(this->FluentCaseFile->peek());
    // this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
    this->CaseBuffer->value += static_cast<char>(this->FluentCaseFile->get());
    if (this->FluentCaseFile->eof())
    {
      return 0;
    }
  }

  index.erase(0, 1); // Get rid of the "("

  //
  //  Grab the chunk and put it in buffer.
  //  You have to look for the end of section std::string if it is
  //  a binary chunk.
  //

  if (index.size() > 2)
  { // Binary Chunk
    char end[120];
    strcpy(end, "End of Binary Section   ");
    strcat(end, index.c_str());
    strcat(end, ")");
    size_t len = strlen(end);

    // Load the case buffer enough to start comparing to the end std::string.
    while (this->CaseBuffer->value.size() < len)
    {
      // this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
      this->CaseBuffer->value += static_cast<char>(this->FluentCaseFile->get());
    }

    // while (CaseBuffer.compare(CaseBuffer.size()-strlen(end),
    // strlen(end), end))
    while (
      strcmp(this->CaseBuffer->value.c_str() + (this->CaseBuffer->value.size() - len), end) != 0)
    {
      // this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
      this->CaseBuffer->value += static_cast<char>(this->FluentCaseFile->get());
    }
  }
  else
  { // Ascii Chunk
    int level = 0;
    while ((this->FluentCaseFile->peek() != ')') || (level != 0))
    {
      // this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
      this->CaseBuffer->value += static_cast<char>(this->FluentCaseFile->get());
      if (this->CaseBuffer->value.at(this->CaseBuffer->value.length() - 1) == '(')
      {
        level++;
      }
      if (this->CaseBuffer->value.at(this->CaseBuffer->value.length() - 1) == ')')
      {
        level--;
      }
      if (this->FluentCaseFile->eof())
      {
        return 0;
      }
    }
    // this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
    this->CaseBuffer->value += static_cast<char>(this->FluentCaseFile->get());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetCaseIndex()
{
  std::string sindex;

  int i = 1;
  while (this->CaseBuffer->value.at(i) != ' ')
  {
    // sindex.push_back(this->CaseBuffer->value.at(i++));
    sindex += this->CaseBuffer->value.at(i++);
  }
  return atoi(sindex.c_str());
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetNumberOfCellZones()
{
  int match;

  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    if (this->CellZones->value.empty())
    {
      this->CellZones->value.push_back(this->Cells->value[i].zone);
    }
    else
    {
      match = 0;
      for (size_t j = 0; j < this->CellZones->value.size(); j++)
      {
        if (this->CellZones->value[j] == this->Cells->value[i].zone)
        {
          match = 1;
        }
      }
      if (match == 0)
      {
        this->CellZones->value.push_back(this->Cells->value[i].zone);
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetDataIndex()
{
  std::string sindex;

  int i = 1;
  while (this->DataBuffer->value.at(i) != ' ')
  {
    // sindex.push_back(this->DataBuffer->value.at(i++));
    sindex += this->DataBuffer->value.at(i++);
  }
  return atoi(sindex.c_str());
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetDataChunk()
{
  this->DataBuffer->value = ""; // Clear buffer
  //
  // Look for beginning of chunk
  //
  while (this->FluentDataFile->peek() != '(')
  {
    this->FluentDataFile->get();
    if (this->FluentDataFile->eof())
    {
      return 0;
    }
  }

  //
  // Figure out whether this is a binary or ascii chunk.
  // If the index is 3 digits or more, then binary, otherwise ascii.
  //
  std::string index;
  while (this->FluentDataFile->peek() != ' ')
  {
    // index.push_back(this->FluentDataFile->peek());
    index += static_cast<char>(this->FluentDataFile->peek());
    // this->DataBuffer->value.push_back(this->FluentDataFile->get());
    this->DataBuffer->value += static_cast<char>(this->FluentDataFile->get());
    if (this->FluentDataFile->eof())
    {
      return 0;
    }
  }

  index.erase(0, 1); // Get rid of the "("

  //
  //  Grab the chunk and put it in buffer.
  //  You have to look for the end of section std::string if it is
  //  a binary chunk.
  //
  if (index.size() > 3)
  { // Binary Chunk
    // it may be in our best interest to do away with the index portion of the
    //"end" string - we have found a dataset, that although errant, does work
    // fine in ensight and the index does not match - maybe just an end string
    // that contains "End of Binary Section" and and a search to relocate the
    // file pointer to the "))" entry.
    char end[120];
    strcpy(end, "End of Binary Section   ");
    // strcat(end, index.c_str());
    // strcat(end, ")");
    size_t len = strlen(end);

    // Load the data buffer enough to start comparing to the end std::string.
    while (this->DataBuffer->value.size() < len)
    {
      // this->DataBuffer->value.push_back(this->FluentDataFile->get());
      this->DataBuffer->value += static_cast<char>(this->FluentDataFile->get());
    }

    // while (DataBuffer.compare(DataBuffer.size()-strlen(end),
    // strlen(end), end))
    while (
      strcmp(this->DataBuffer->value.c_str() + (this->DataBuffer->value.size() - len), end) != 0)
    {
      // this->DataBuffer->value.push_back(this->FluentDataFile->get());
      this->DataBuffer->value += static_cast<char>(this->FluentDataFile->get());
    }
  }
  else
  { // Ascii Chunk
    int level = 0;
    while ((this->FluentDataFile->peek() != ')') || (level != 0))
    {
      // this->DataBuffer->value.push_back(this->FluentDataFile->get());
      this->DataBuffer->value += static_cast<char>(this->FluentDataFile->get());
      if (this->DataBuffer->value.at(this->DataBuffer->value.length() - 1) == '(')
      {
        level++;
      }
      if (this->DataBuffer->value.at(this->DataBuffer->value.length() - 1) == ')')
      {
        level--;
      }
      if (this->FluentDataFile->eof())
      {
        return 0;
      }
    }
    // this->DataBuffer->value.push_back(this->FluentDataFile->get());
    this->DataBuffer->value += static_cast<char>(this->FluentDataFile->get());
  }

  return 1;
}

struct
{
  size_t index;
  const char* name;
} variable_info[] = {
  //
  { 1, "PRESSURE" },
  { 2, "MOMENTUM" },
  { 3, "TEMPERATURE" },
  { 4, "ENTHALPY" },
  { 5, "TKE" },
  { 6, "TED" },
  { 7, "SPECIES" },
  { 8, "G" },
  { 9, "WSWIRL" },
  { 10, "DPMS_MASS" },
  { 11, "DPMS_MOM" },
  { 12, "DPMS_ENERGY" },
  { 13, "DPMS_SPECIES" },
  { 14, "DVOLUME_DT" },
  { 15, "BODY_FORCES" },
  { 16, "FMEAN" },
  { 17, "FVAR" },
  { 18, "MASS_FLUX" },
  { 19, "WALL_SHEAR" },
  { 20, "BOUNDARY_HEAT_FLUX" },
  { 21, "BOUNDARY_RAD_HEAT_FLUX" },
  { 22, "OLD_PRESSURE" },
  { 23, "POLLUT" },
  { 24, "DPMS_P1_S" },
  { 25, "DPMS_P1_AP" },
  { 26, "WALL_GAS_TEMPERATURE" },
  { 27, "DPMS_P1_DIFF" },
  { 28, "DR_SURF" },
  { 29, "W_M1" },
  { 30, "W_M2" },
  { 31, "DPMS_BURNOUT" },

  //
  { 32, "DPMS_CONCENTRATION" },
  { 33, "PDF_MW" },
  { 34, "DPMS_WSWIRL" },
  { 35, "YPLUS" },
  { 36, "YPLUS_UTAU" },
  { 37, "WALL_SHEAR_SWIRL" },
  { 38, "WALL_T_INNER" },
  { 39, "POLLUT0" },
  { 40, "POLLUT1" },
  { 41, "WALL_G_INNER" },
  { 42, "PREMIXC" },
  { 43, "PREMIXC_T" },
  { 44, "PREMIXC_RATE" },
  { 45, "POLLUT2" },
  { 46, "POLLUT3" },
  { 47, "MASS_FLUX_M1" },
  { 48, "MASS_FLUX_M2" },
  { 49, "GRID_FLUX" },
  { 50, "DO_I" },
  { 51, "DO_RECON_I" },
  { 52, "DO_ENERGY_SOURCE" },
  { 53, "DO_IRRAD" },
  { 54, "DO_QMINUS" },
  { 55, "DO_IRRAD_OLD" },
  { 56, "DO_IWX=56" },
  { 57, "DO_IWY" },
  { 58, "DO_IWZ" },
  { 59, "MACH" },
  { 60, "SLIP_U" },
  { 61, "SLIP_V" },
  { 62, "SLIP_W" },
  { 63, "SDR" },
  { 64, "SDR_M1" },
  { 65, "SDR_M2" },
  { 66, "POLLUT4" },
  { 67, "GRANULAR_TEMPERATURE" },
  { 68, "GRANULAR_TEMPERATURE_M1" },
  { 69, "GRANULAR_TEMPERATURE_M2" },
  { 70, "VFLUX" },
  { 80, "VFLUX_M1" },
  { 90, "VFLUX_M2" },
  { 91, "DO_QNET" },
  { 92, "DO_QTRANS" },
  { 93, "DO_QREFL" },
  { 94, "DO_QABS" },
  { 95, "POLLUT5" },
  { 96, "WALL_DIST" },
  { 97, "SOLAR_SOURCE" },
  { 98, "SOLAR_QREFL" },
  { 99, "SOLAR_QABS" },
  { 100, "SOLAR_QTRANS" },
  { 101, "DENSITY" },
  { 102, "MU_LAM" },
  { 103, "MU_TURB" },
  { 104, "CP" },
  { 105, "KTC" },
  { 106, "VGS_DTRM" },
  { 107, "VGF_DTRM" },
  { 108, "RSTRESS" },
  { 109, "THREAD_RAD_FLUX" },
  { 110, "SPE_Q" },
  { 111, "X_VELOCITY" },
  { 112, "Y_VELOCITY" },
  { 113, "Z_VELOCITY" },
  { 114, "WALL_VELOCITY" },
  { 115, "X_VELOCITY_M1" },
  { 116, "Y_VELOCITY_M1" },
  { 117, "Z_VELOCITY_M1" },
  { 118, "PHASE_MASS" },
  { 119, "TKE_M1" },
  { 120, "TED_M1" },
  { 121, "POLLUT6" },
  { 122, "X_VELOCITY_M2" },
  { 123, "Y_VELOCITY_M2" },
  { 124, "Z_VELOCITY_M2" },
  { 126, "TKE_M2" },
  { 127, "TED_M2" },
  { 128, "RUU" },
  { 129, "RVV" },
  { 130, "RWW" },
  { 131, "RUV" },
  { 132, "RVW" },
  { 133, "RUW" },
  { 134, "DPMS_EROSION" },
  { 135, "DPMS_ACCRETION" },
  { 136, "FMEAN2" },
  { 137, "FVAR2" },
  { 138, "ENTHALPY_M1" },
  { 139, "ENTHALPY_M2" },
  { 140, "FMEAN_M1" },
  { 141, "FMEAN_M2" },
  { 142, "FVAR_M1" },
  { 143, "FVAR_M2" },
  { 144, "FMEAN2_M1" },
  { 145, "FMEAN2_M2" },
  { 146, "FVAR2_M1" },
  { 147, "FVAR2_M2" },
  { 148, "PREMIXC_M1" },
  { 149, "PREMIXC_M2" },
  { 150, "VOF" },
  { 151, "VOF_1" },
  { 152, "VOF_2" },
  { 153, "VOF_3" },
  { 154, "VOF_4" },
  { 160, "VOF_M1" },
  { 161, "VOF_1_M1" },
  { 162, "VOF_2_M1" },
  { 163, "VOF_3_M1" },
  { 164, "VOF_4_M1" },
  { 170, "VOF_M2" },
  { 171, "VOF_1_M2" },
  { 172, "VOF_2_M2" },
  { 173, "VOF_3_M2" },
  { 174, "VOF_4_M2" },
  { 180, "VOLUME_M2" },
  { 181, "WALL_GRID_VELOCITY" },
  { 182, "POLLUT7" },
  { 183, "POLLUT8" },
  { 184, "POLLUT9" },
  { 185, "POLLUT10" },
  { 186, "POLLUT11" },
  { 187, "POLLUT12" },
  { 188, "POLLUT13" },
  { 190, "SV_T_AUX" },
  { 191, "SV_T_AP_AUX" },
  { 192, "TOTAL_PRESSURE" },
  { 193, "TOTAL_TEMPERATURE" },
  { 194, "NRBC_DC" },
  { 195, "DP_TMFR" },

  // Y_*
  { 200, "Y_00" },
  { 201, "Y_01" },
  { 202, "Y_02" },
  { 203, "Y_03" },
  { 204, "Y_04" },
  { 205, "Y_05" },
  { 206, "Y_06" },
  { 207, "Y_07" },
  { 208, "Y_08" },
  { 209, "Y_09" },
  { 210, "Y_10" },
  { 211, "Y_11" },
  { 212, "Y_12" },
  { 213, "Y_13" },
  { 214, "Y_14" },
  { 215, "Y_15" },
  { 216, "Y_16" },
  { 217, "Y_17" },
  { 218, "Y_18" },
  { 219, "Y_19" },
  { 220, "Y_20" },
  { 221, "Y_21" },
  { 222, "Y_22" },
  { 223, "Y_23" },
  { 224, "Y_24" },
  { 225, "Y_25" },
  { 226, "Y_26" },
  { 227, "Y_27" },
  { 228, "Y_28" },
  { 229, "Y_29" },
  { 230, "Y_30" },
  { 231, "Y_31" },
  { 232, "Y_32" },
  { 233, "Y_33" },
  { 234, "Y_34" },
  { 235, "Y_35" },
  { 236, "Y_36" },
  { 237, "Y_37" },
  { 238, "Y_38" },
  { 239, "Y_39" },
  { 240, "Y_40" },
  { 241, "Y_41" },
  { 242, "Y_42" },
  { 243, "Y_43" },
  { 244, "Y_44" },
  { 245, "Y_45" },
  { 246, "Y_46" },
  { 247, "Y_47" },
  { 248, "Y_48" },
  { 249, "Y_49" },

  // Y_M1_*
  { 250, "Y_M1_00" },
  { 251, "Y_M1_01" },
  { 252, "Y_M1_02" },
  { 253, "Y_M1_03" },
  { 254, "Y_M1_04" },
  { 255, "Y_M1_05" },
  { 256, "Y_M1_06" },
  { 257, "Y_M1_07" },
  { 258, "Y_M1_08" },
  { 259, "Y_M1_09" },
  { 260, "Y_M1_10" },
  { 261, "Y_M1_11" },
  { 262, "Y_M1_12" },
  { 263, "Y_M1_13" },
  { 264, "Y_M1_14" },
  { 265, "Y_M1_15" },
  { 266, "Y_M1_16" },
  { 267, "Y_M1_17" },
  { 268, "Y_M1_18" },
  { 269, "Y_M1_19" },
  { 270, "Y_M1_20" },
  { 271, "Y_M1_21" },
  { 272, "Y_M1_22" },
  { 273, "Y_M1_23" },
  { 274, "Y_M1_24" },
  { 275, "Y_M1_25" },
  { 276, "Y_M1_26" },
  { 277, "Y_M1_27" },
  { 278, "Y_M1_28" },
  { 279, "Y_M1_29" },
  { 280, "Y_M1_30" },
  { 281, "Y_M1_31" },
  { 282, "Y_M1_32" },
  { 283, "Y_M1_33" },
  { 284, "Y_M1_34" },
  { 285, "Y_M1_35" },
  { 286, "Y_M1_36" },
  { 287, "Y_M1_37" },
  { 288, "Y_M1_38" },
  { 289, "Y_M1_39" },
  { 290, "Y_M1_40" },
  { 291, "Y_M1_41" },
  { 292, "Y_M1_42" },
  { 293, "Y_M1_43" },
  { 294, "Y_M1_44" },
  { 295, "Y_M1_45" },
  { 296, "Y_M1_46" },
  { 297, "Y_M1_47" },
  { 298, "Y_M1_48" },
  { 299, "Y_M1_49" },

  // Y_M2_*
  { 300, "Y_M2_00" },
  { 301, "Y_M2_01" },
  { 302, "Y_M2_02" },
  { 303, "Y_M2_03" },
  { 304, "Y_M2_04" },
  { 305, "Y_M2_05" },
  { 306, "Y_M2_06" },
  { 307, "Y_M2_07" },
  { 308, "Y_M2_08" },
  { 309, "Y_M2_09" },
  { 310, "Y_M2_10" },
  { 311, "Y_M2_11" },
  { 312, "Y_M2_12" },
  { 313, "Y_M2_13" },
  { 314, "Y_M2_14" },
  { 315, "Y_M2_15" },
  { 316, "Y_M2_16" },
  { 317, "Y_M2_17" },
  { 318, "Y_M2_18" },
  { 319, "Y_M2_19" },
  { 320, "Y_M2_20" },
  { 321, "Y_M2_21" },
  { 322, "Y_M2_22" },
  { 323, "Y_M2_23" },
  { 324, "Y_M2_24" },
  { 325, "Y_M2_25" },
  { 326, "Y_M2_26" },
  { 327, "Y_M2_27" },
  { 328, "Y_M2_28" },
  { 329, "Y_M2_29" },
  { 330, "Y_M2_30" },
  { 331, "Y_M2_31" },
  { 332, "Y_M2_32" },
  { 333, "Y_M2_33" },
  { 334, "Y_M2_34" },
  { 335, "Y_M2_35" },
  { 336, "Y_M2_36" },
  { 337, "Y_M2_37" },
  { 338, "Y_M2_38" },
  { 339, "Y_M2_39" },
  { 340, "Y_M2_40" },
  { 341, "Y_M2_41" },
  { 342, "Y_M2_42" },
  { 343, "Y_M2_43" },
  { 344, "Y_M2_44" },
  { 345, "Y_M2_45" },
  { 346, "Y_M2_46" },
  { 347, "Y_M2_47" },
  { 348, "Y_M2_48" },
  { 349, "Y_M2_49" },

  // DR_SURF_*
  { 350, "DR_SURF_00" },
  { 351, "DR_SURF_01" },
  { 352, "DR_SURF_02" },
  { 353, "DR_SURF_03" },
  { 354, "DR_SURF_04" },
  { 355, "DR_SURF_05" },
  { 356, "DR_SURF_06" },
  { 357, "DR_SURF_07" },
  { 358, "DR_SURF_08" },
  { 359, "DR_SURF_09" },
  { 360, "DR_SURF_10" },
  { 361, "DR_SURF_11" },
  { 362, "DR_SURF_12" },
  { 363, "DR_SURF_13" },
  { 364, "DR_SURF_14" },
  { 365, "DR_SURF_15" },
  { 366, "DR_SURF_16" },
  { 367, "DR_SURF_17" },
  { 368, "DR_SURF_18" },
  { 369, "DR_SURF_19" },
  { 370, "DR_SURF_20" },
  { 371, "DR_SURF_21" },
  { 372, "DR_SURF_22" },
  { 373, "DR_SURF_23" },
  { 374, "DR_SURF_24" },
  { 375, "DR_SURF_25" },
  { 376, "DR_SURF_26" },
  { 377, "DR_SURF_27" },
  { 378, "DR_SURF_28" },
  { 379, "DR_SURF_29" },
  { 380, "DR_SURF_30" },
  { 381, "DR_SURF_31" },
  { 382, "DR_SURF_32" },
  { 383, "DR_SURF_33" },
  { 384, "DR_SURF_34" },
  { 385, "DR_SURF_35" },
  { 386, "DR_SURF_36" },
  { 387, "DR_SURF_37" },
  { 388, "DR_SURF_38" },
  { 389, "DR_SURF_39" },
  { 390, "DR_SURF_40" },
  { 391, "DR_SURF_41" },
  { 392, "DR_SURF_42" },
  { 393, "DR_SURF_43" },
  { 394, "DR_SURF_44" },
  { 395, "DR_SURF_45" },
  { 396, "DR_SURF_46" },
  { 397, "DR_SURF_47" },
  { 398, "DR_SURF_48" },
  { 399, "DR_SURF_49" },

  //
  { 400, "PRESSURE_MEAN" },
  { 401, "PRESSURE_RMS" },
  { 402, "X_VELOCITY_MEAN" },
  { 403, "X_VELOCITY_RMS" },
  { 404, "Y_VELOCITY_MEAN" },
  { 405, "Y_VELOCITY_RMS" },
  { 406, "Z_VELOCITY_MEAN" },
  { 407, "Z_VELOCITY_RMS" },
  { 408, "TEMPERATURE_MEAN" },
  { 409, "TEMPERATURE_RMS" },
  { 410, "VOF_MEAN" },
  { 411, "VOF_RMS" },
  { 412, "PRESSURE_M1" },
  { 413, "PRESSURE_M2" },
  { 414, "GRANULAR_TEMPERATURE_MEAN" },
  { 415, "GRANULAR_TEMPERATURE_RMS" },

  // DPMS_Y_*
  { 450, "DPMS_Y_00" },
  { 451, "DPMS_Y_01" },
  { 452, "DPMS_Y_02" },
  { 453, "DPMS_Y_03" },
  { 454, "DPMS_Y_04" },
  { 455, "DPMS_Y_05" },
  { 456, "DPMS_Y_06" },
  { 457, "DPMS_Y_07" },
  { 458, "DPMS_Y_08" },
  { 459, "DPMS_Y_09" },
  { 460, "DPMS_Y_10" },
  { 461, "DPMS_Y_11" },
  { 462, "DPMS_Y_12" },
  { 463, "DPMS_Y_13" },
  { 464, "DPMS_Y_14" },
  { 465, "DPMS_Y_15" },
  { 466, "DPMS_Y_16" },
  { 467, "DPMS_Y_17" },
  { 468, "DPMS_Y_18" },
  { 469, "DPMS_Y_19" },
  { 470, "DPMS_Y_20" },
  { 471, "DPMS_Y_21" },
  { 472, "DPMS_Y_22" },
  { 473, "DPMS_Y_23" },
  { 474, "DPMS_Y_24" },
  { 475, "DPMS_Y_25" },
  { 476, "DPMS_Y_26" },
  { 477, "DPMS_Y_27" },
  { 478, "DPMS_Y_28" },
  { 479, "DPMS_Y_29" },
  { 480, "DPMS_Y_30" },
  { 481, "DPMS_Y_31" },
  { 482, "DPMS_Y_32" },
  { 483, "DPMS_Y_33" },
  { 484, "DPMS_Y_34" },
  { 485, "DPMS_Y_35" },
  { 486, "DPMS_Y_36" },
  { 487, "DPMS_Y_37" },
  { 488, "DPMS_Y_38" },
  { 489, "DPMS_Y_39" },
  { 490, "DPMS_Y_40" },
  { 491, "DPMS_Y_41" },
  { 492, "DPMS_Y_42" },
  { 493, "DPMS_Y_43" },
  { 494, "DPMS_Y_44" },
  { 495, "DPMS_Y_45" },
  { 496, "DPMS_Y_46" },
  { 497, "DPMS_Y_47" },
  { 498, "DPMS_Y_48" },
  { 499, "DPMS_Y_49" },

  //
  { 500, "NUT" },
  { 501, "NUT_M1" },
  { 502, "NUT_M2" },
  { 503, "RUU_M1" },
  { 504, "RVV_M1" },
  { 505, "RWW_M1" },
  { 506, "RUV_M1" },
  { 507, "RVW_M1" },
  { 508, "RUW_M1" },
  { 509, "RUU_M2" },
  { 510, "RVV_M2" },
  { 511, "RWW_M2" },
  { 512, "RUV_M2" },
  { 513, "RVW_M2" },
  { 514, "RUW_M2" },
  { 515, "ENERGY_M1" },
  { 516, "ENERGY_M2" },
  { 517, "DENSITY_M1" },
  { 518, "DENSITY_M2" },
  { 519, "DPMS_PDF_1" },
  { 520, "DPMS_PDF_2" },
  { 521, "V2" },
  { 522, "V2_M1" },
  { 523, "V2_M2" },
  { 524, "FEL" },
  { 525, "FEL_M1" },
  { 526, "FEL_M2" },
  { 527, "LKE" },
  { 528, "LKE_M1" },
  { 529, "LKE_M2" },
  { 530, "SHELL_CELL_T" },
  { 531, "SHELL_FACE_T" },
  { 532, "SHELL_CELL_ENERGY_M1" },
  { 533, "SHELL_CELL_ENERGY_M2" },
  { 540, "DPMS_TKE" },
  { 541, "DPMS_D" },
  { 542, "DPMS_O" },
  { 543, "DPMS_TKE_RUU" },
  { 544, "DPMS_TKE_RVV" },
  { 545, "DPMS_TKE_RWW" },
  { 546, "DPMS_TKE_RUV" },
  { 547, "DPMS_TKE_RVW" },
  { 548, "DPMS_TKE_RUW" },
  { 549, "DPMS_DS_MASS" },
  { 550, "DPMS_DS_ENERGY" },
  { 551, "DPMS_DS_TKE" },
  { 552, "DPMS_DS_D" },
  { 553, "DPMS_DS_O" },
  { 554, "DPMS_DS_TKE_RUU" },
  { 555, "DPMS_DS_TKE_RVV" },
  { 556, "DPMS_DS_TKE_RWW" },
  { 557, "DPMS_DS_TKE_RUV" },
  { 558, "DPMS_DS_TKE_RVW" },
  { 559, "DPMS_DS_TKE_RUW" },
  { 560, "DPMS_DS_PDF_1" },
  { 561, "DPMS_DS_PDF_2" },
  { 562, "DPMS_DS_EMISS" },
  { 563, "DPMS_DS_ABS" },
  { 564, "DPMS_DS_SCAT" },
  { 565, "DPMS_DS_BURNOUT" },
  { 566, "DPMS_DS_MOM" },
  { 567, "DPMS_DS_WSWIRL" },
  { 580, "MU_TURB_L" },
  { 581, "MU_TURB_S" },
  { 582, "TKE_TRANS" },
  { 583, "TKE_TRANS_M1" },
  { 584, "TKE_TRANS_M2" },
  { 585, "MU_TURB_W" },
  { 600, "DELH" },
  { 601, "DPMS_MOM_AP" },
  { 602, "DPMS_WSWIRL_AP" },
  { 603, "X_PULL" },
  { 604, "Y_PULL" },
  { 605, "Z_PULL" },
  { 606, "LIQF" },
  { 610, "PDFT_QBAR" },
  { 611, "PDFT_PHI" },
  { 612, "PDFT_Q_TA" },
  { 613, "PDFT_SVOL_TA" },
  { 614, "PDFT_MASS_TA" },
  { 615, "PDFT_T4_TA" },
  { 620, "MICRO_MIX_FVAR1 " },
  { 621, "MICRO_MIX_FVAR2 " },
  { 622, "MICRO_MIX_FVAR3 " },
  { 623, "MICRO_MIX_FVAR1_M1 " },
  { 624, "MICRO_MIX_FVAR2_M1 " },
  { 625, "MICRO_MIX_FVAR3_M1 " },
  { 626, "MICRO_MIX_FVAR1_M2 " },
  { 627, "MICRO_MIX_FVAR2_M2 " },
  { 628, "MICRO_MIX_FVAR3_M2 " },
  { 630, "SCAD_LES " },
  { 635, "UFLA_Y    " },
  { 636, "UFLA_Y_M1 " },
  { 637, "UFLA_Y_M2 " },
  { 645, "CREV_MASS" },
  { 646, "CREV_ENRG" },
  { 647, "CREV_MOM" },
  { 650, "ACOUSTICS_MODEL" },
  { 651, "AC_RECEIVERS_DATA" },
  { 652, "SV_DPDT_RMS" },
  { 653, "SV_PRESSURE_M1" },
  { 654, "AC_PERIODIC_INDEX" },
  { 655, "AC_PERIODIC_PS" },
  { 656, "AC_F_NORMAL" },
  { 657, "AC_F_CENTROID" },
  { 660, "IGNITE" },
  { 661, "IGNITE_M1" },
  { 662, "IGNITE_M2" },
  { 663, "IGNITE_RATE" },

  // *_MEAN
  { 680, "WALL_SHEAR_MEAN" },
  { 681, "UV_MEAN" },
  { 682, "UW_MEAN" },
  { 683, "VW_MEAN" },
  { 684, "UT_MEAN" },
  { 685, "VT_MEAN" },
  { 686, "WT_MEAN" },
  { 687, "BOUNDARY_HEAT_FLUX_MEAN" },

  // UDS_*
  { 700, "UDS_00" },
  { 701, "UDS_01" },
  { 702, "UDS_02" },
  { 703, "UDS_03" },
  { 704, "UDS_04" },
  { 705, "UDS_05" },
  { 706, "UDS_06" },
  { 707, "UDS_07" },
  { 708, "UDS_08" },
  { 709, "UDS_09" },
  { 710, "UDS_10" },
  { 711, "UDS_11" },
  { 712, "UDS_12" },
  { 713, "UDS_13" },
  { 714, "UDS_14" },
  { 715, "UDS_15" },
  { 716, "UDS_16" },
  { 717, "UDS_17" },
  { 718, "UDS_18" },
  { 719, "UDS_19" },
  { 720, "UDS_20" },
  { 721, "UDS_21" },
  { 722, "UDS_22" },
  { 723, "UDS_23" },
  { 724, "UDS_24" },
  { 725, "UDS_25" },
  { 726, "UDS_26" },
  { 727, "UDS_27" },
  { 728, "UDS_28" },
  { 729, "UDS_29" },
  { 730, "UDS_30" },
  { 731, "UDS_31" },
  { 732, "UDS_32" },
  { 733, "UDS_33" },
  { 734, "UDS_34" },
  { 735, "UDS_35" },
  { 736, "UDS_36" },
  { 737, "UDS_37" },
  { 738, "UDS_38" },
  { 739, "UDS_39" },
  { 740, "UDS_40" },
  { 741, "UDS_41" },
  { 742, "UDS_42" },
  { 743, "UDS_43" },
  { 744, "UDS_44" },
  { 745, "UDS_45" },
  { 746, "UDS_46" },
  { 747, "UDS_47" },
  { 748, "UDS_48" },
  { 749, "UDS_49" },

  // UDS_M1_*
  { 750, "UDS_M1_00" },
  { 751, "UDS_M1_01" },
  { 752, "UDS_M1_02" },
  { 753, "UDS_M1_03" },
  { 754, "UDS_M1_04" },
  { 755, "UDS_M1_05" },
  { 756, "UDS_M1_06" },
  { 757, "UDS_M1_07" },
  { 758, "UDS_M1_08" },
  { 759, "UDS_M1_09" },
  { 760, "UDS_M1_10" },
  { 761, "UDS_M1_11" },
  { 762, "UDS_M1_12" },
  { 763, "UDS_M1_13" },
  { 764, "UDS_M1_14" },
  { 765, "UDS_M1_15" },
  { 766, "UDS_M1_16" },
  { 767, "UDS_M1_17" },
  { 768, "UDS_M1_18" },
  { 769, "UDS_M1_19" },
  { 770, "UDS_M1_20" },
  { 771, "UDS_M1_21" },
  { 772, "UDS_M1_22" },
  { 773, "UDS_M1_23" },
  { 774, "UDS_M1_24" },
  { 775, "UDS_M1_25" },
  { 776, "UDS_M1_26" },
  { 777, "UDS_M1_27" },
  { 778, "UDS_M1_28" },
  { 779, "UDS_M1_29" },
  { 780, "UDS_M1_30" },
  { 781, "UDS_M1_31" },
  { 782, "UDS_M1_32" },
  { 783, "UDS_M1_33" },
  { 784, "UDS_M1_34" },
  { 785, "UDS_M1_35" },
  { 786, "UDS_M1_36" },
  { 787, "UDS_M1_37" },
  { 788, "UDS_M1_38" },
  { 789, "UDS_M1_39" },
  { 790, "UDS_M1_40" },
  { 791, "UDS_M1_41" },
  { 792, "UDS_M1_42" },
  { 793, "UDS_M1_43" },
  { 794, "UDS_M1_44" },
  { 795, "UDS_M1_45" },
  { 796, "UDS_M1_46" },
  { 797, "UDS_M1_47" },
  { 798, "UDS_M1_48" },
  { 799, "UDS_M1_49" },

  // UDS_M2_*
  { 800, "UDS_M2_00" },
  { 801, "UDS_M2_01" },
  { 802, "UDS_M2_02" },
  { 803, "UDS_M2_03" },
  { 804, "UDS_M2_04" },
  { 805, "UDS_M2_05" },
  { 806, "UDS_M2_06" },
  { 807, "UDS_M2_07" },
  { 808, "UDS_M2_08" },
  { 809, "UDS_M2_09" },
  { 810, "UDS_M2_10" },
  { 811, "UDS_M2_11" },
  { 812, "UDS_M2_12" },
  { 813, "UDS_M2_13" },
  { 814, "UDS_M2_14" },
  { 815, "UDS_M2_15" },
  { 816, "UDS_M2_16" },
  { 817, "UDS_M2_17" },
  { 818, "UDS_M2_18" },
  { 819, "UDS_M2_19" },
  { 820, "UDS_M2_20" },
  { 821, "UDS_M2_21" },
  { 822, "UDS_M2_22" },
  { 823, "UDS_M2_23" },
  { 824, "UDS_M2_24" },
  { 825, "UDS_M2_25" },
  { 826, "UDS_M2_26" },
  { 827, "UDS_M2_27" },
  { 828, "UDS_M2_28" },
  { 829, "UDS_M2_29" },
  { 830, "UDS_M2_30" },
  { 831, "UDS_M2_31" },
  { 832, "UDS_M2_32" },
  { 833, "UDS_M2_33" },
  { 834, "UDS_M2_34" },
  { 835, "UDS_M2_35" },
  { 836, "UDS_M2_36" },
  { 837, "UDS_M2_37" },
  { 838, "UDS_M2_38" },
  { 839, "UDS_M2_39" },
  { 840, "UDS_M2_40" },
  { 841, "UDS_M2_41" },
  { 842, "UDS_M2_42" },
  { 843, "UDS_M2_43" },
  { 844, "UDS_M2_44" },
  { 845, "UDS_M2_45" },
  { 846, "UDS_M2_46" },
  { 847, "UDS_M2_47" },
  { 848, "UDS_M2_48" },
  { 849, "UDS_M2_49" },

  // DPMS_DS_Y_*
  { 850, "DPMS_DS_Y_00" },
  { 851, "DPMS_DS_Y_01" },
  { 852, "DPMS_DS_Y_02" },
  { 853, "DPMS_DS_Y_03" },
  { 854, "DPMS_DS_Y_04" },
  { 855, "DPMS_DS_Y_05" },
  { 856, "DPMS_DS_Y_06" },
  { 857, "DPMS_DS_Y_07" },
  { 858, "DPMS_DS_Y_08" },
  { 859, "DPMS_DS_Y_09" },
  { 860, "DPMS_DS_Y_10" },
  { 861, "DPMS_DS_Y_11" },
  { 862, "DPMS_DS_Y_12" },
  { 863, "DPMS_DS_Y_13" },
  { 864, "DPMS_DS_Y_14" },
  { 865, "DPMS_DS_Y_15" },
  { 866, "DPMS_DS_Y_16" },
  { 867, "DPMS_DS_Y_17" },
  { 868, "DPMS_DS_Y_18" },
  { 869, "DPMS_DS_Y_19" },
  { 870, "DPMS_DS_Y_20" },
  { 871, "DPMS_DS_Y_21" },
  { 872, "DPMS_DS_Y_22" },
  { 873, "DPMS_DS_Y_23" },
  { 874, "DPMS_DS_Y_24" },
  { 875, "DPMS_DS_Y_25" },
  { 876, "DPMS_DS_Y_26" },
  { 877, "DPMS_DS_Y_27" },
  { 878, "DPMS_DS_Y_28" },
  { 879, "DPMS_DS_Y_29" },
  { 880, "DPMS_DS_Y_30" },
  { 881, "DPMS_DS_Y_31" },
  { 882, "DPMS_DS_Y_32" },
  { 883, "DPMS_DS_Y_33" },
  { 884, "DPMS_DS_Y_34" },
  { 885, "DPMS_DS_Y_35" },
  { 886, "DPMS_DS_Y_36" },
  { 887, "DPMS_DS_Y_37" },
  { 888, "DPMS_DS_Y_38" },
  { 889, "DPMS_DS_Y_39" },
  { 890, "DPMS_DS_Y_40" },
  { 891, "DPMS_DS_Y_41" },
  { 892, "DPMS_DS_Y_42" },
  { 893, "DPMS_DS_Y_43" },
  { 894, "DPMS_DS_Y_44" },
  { 895, "DPMS_DS_Y_45" },
  { 896, "DPMS_DS_Y_46" },
  { 897, "DPMS_DS_Y_47" },
  { 898, "DPMS_DS_Y_48" },
  { 899, "DPMS_DS_Y_49" },

  //
  { 910, "GRANULAR_PRESSURE" },
  { 911, "DPMS_DS_P1_S" },
  { 912, "DPMS_DS_P1_AP" },
  { 913, "DPMS_DS_P1_DIFF" },

  // DPMS_DS_SURFACE_SPECIES_*
  { 920, "DPMS_DS_SURFACE_SPECIES_00" },
  { 921, "DPMS_DS_SURFACE_SPECIES_01" },
  { 922, "DPMS_DS_SURFACE_SPECIES_02" },
  { 923, "DPMS_DS_SURFACE_SPECIES_03" },
  { 924, "DPMS_DS_SURFACE_SPECIES_04" },
  { 925, "DPMS_DS_SURFACE_SPECIES_05" },
  { 926, "DPMS_DS_SURFACE_SPECIES_06" },
  { 927, "DPMS_DS_SURFACE_SPECIES_07" },
  { 928, "DPMS_DS_SURFACE_SPECIES_08" },
  { 929, "DPMS_DS_SURFACE_SPECIES_09" },
  { 930, "DPMS_DS_SURFACE_SPECIES_10" },
  { 931, "DPMS_DS_SURFACE_SPECIES_11" },
  { 932, "DPMS_DS_SURFACE_SPECIES_12" },
  { 933, "DPMS_DS_SURFACE_SPECIES_13" },
  { 934, "DPMS_DS_SURFACE_SPECIES_14" },
  { 935, "DPMS_DS_SURFACE_SPECIES_15" },
  { 936, "DPMS_DS_SURFACE_SPECIES_16" },
  { 937, "DPMS_DS_SURFACE_SPECIES_17" },
  { 938, "DPMS_DS_SURFACE_SPECIES_18" },
  { 939, "DPMS_DS_SURFACE_SPECIES_19" },
  { 940, "DPMS_DS_SURFACE_SPECIES_20" },
  { 941, "DPMS_DS_SURFACE_SPECIES_21" },
  { 942, "DPMS_DS_SURFACE_SPECIES_22" },
  { 943, "DPMS_DS_SURFACE_SPECIES_23" },
  { 944, "DPMS_DS_SURFACE_SPECIES_24" },
  { 945, "DPMS_DS_SURFACE_SPECIES_25" },
  { 946, "DPMS_DS_SURFACE_SPECIES_26" },
  { 947, "DPMS_DS_SURFACE_SPECIES_27" },
  { 948, "DPMS_DS_SURFACE_SPECIES_28" },
  { 949, "DPMS_DS_SURFACE_SPECIES_29" },
  { 950, "DPMS_DS_SURFACE_SPECIES_30" },
  { 951, "DPMS_DS_SURFACE_SPECIES_31" },
  { 952, "DPMS_DS_SURFACE_SPECIES_32" },
  { 953, "DPMS_DS_SURFACE_SPECIES_33" },
  { 954, "DPMS_DS_SURFACE_SPECIES_34" },
  { 955, "DPMS_DS_SURFACE_SPECIES_35" },
  { 956, "DPMS_DS_SURFACE_SPECIES_36" },
  { 957, "DPMS_DS_SURFACE_SPECIES_37" },
  { 958, "DPMS_DS_SURFACE_SPECIES_38" },
  { 959, "DPMS_DS_SURFACE_SPECIES_39" },
  { 960, "DPMS_DS_SURFACE_SPECIES_40" },
  { 961, "DPMS_DS_SURFACE_SPECIES_41" },
  { 962, "DPMS_DS_SURFACE_SPECIES_42" },
  { 963, "DPMS_DS_SURFACE_SPECIES_43" },
  { 964, "DPMS_DS_SURFACE_SPECIES_44" },
  { 965, "DPMS_DS_SURFACE_SPECIES_45" },
  { 966, "DPMS_DS_SURFACE_SPECIES_46" },
  { 967, "DPMS_DS_SURFACE_SPECIES_47" },
  { 968, "DPMS_DS_SURFACE_SPECIES_48" },
  { 969, "DPMS_DS_SURFACE_SPECIES_49" },
  { 970, "UDM_I" },

  // Y_MEAN_*
  { 1000, "Y_MEAN_00" },
  { 1001, "Y_MEAN_01" },
  { 1002, "Y_MEAN_02" },
  { 1003, "Y_MEAN_03" },
  { 1004, "Y_MEAN_04" },
  { 1005, "Y_MEAN_05" },
  { 1006, "Y_MEAN_06" },
  { 1007, "Y_MEAN_07" },
  { 1008, "Y_MEAN_08" },
  { 1009, "Y_MEAN_09" },
  { 1010, "Y_MEAN_10" },
  { 1011, "Y_MEAN_11" },
  { 1012, "Y_MEAN_12" },
  { 1013, "Y_MEAN_13" },
  { 1014, "Y_MEAN_14" },
  { 1015, "Y_MEAN_15" },
  { 1016, "Y_MEAN_16" },
  { 1017, "Y_MEAN_17" },
  { 1018, "Y_MEAN_18" },
  { 1019, "Y_MEAN_19" },
  { 1020, "Y_MEAN_20" },
  { 1021, "Y_MEAN_21" },
  { 1022, "Y_MEAN_22" },
  { 1023, "Y_MEAN_23" },
  { 1024, "Y_MEAN_24" },
  { 1025, "Y_MEAN_25" },
  { 1026, "Y_MEAN_26" },
  { 1027, "Y_MEAN_27" },
  { 1028, "Y_MEAN_28" },
  { 1029, "Y_MEAN_29" },
  { 1030, "Y_MEAN_30" },
  { 1031, "Y_MEAN_31" },
  { 1032, "Y_MEAN_32" },
  { 1033, "Y_MEAN_33" },
  { 1034, "Y_MEAN_34" },
  { 1035, "Y_MEAN_35" },
  { 1036, "Y_MEAN_36" },
  { 1037, "Y_MEAN_37" },
  { 1038, "Y_MEAN_38" },
  { 1039, "Y_MEAN_39" },
  { 1040, "Y_MEAN_40" },
  { 1041, "Y_MEAN_41" },
  { 1042, "Y_MEAN_42" },
  { 1043, "Y_MEAN_43" },
  { 1044, "Y_MEAN_44" },
  { 1045, "Y_MEAN_45" },
  { 1046, "Y_MEAN_46" },
  { 1047, "Y_MEAN_47" },
  { 1048, "Y_MEAN_48" },
  { 1049, "Y_MEAN_49" },

  // Y_RMS_*
  { 1050, "Y_RMS_00" },
  { 1051, "Y_RMS_01" },
  { 1052, "Y_RMS_02" },
  { 1053, "Y_RMS_03" },
  { 1054, "Y_RMS_04" },
  { 1055, "Y_RMS_05" },
  { 1056, "Y_RMS_06" },
  { 1057, "Y_RMS_07" },
  { 1058, "Y_RMS_08" },
  { 1059, "Y_RMS_09" },
  { 1060, "Y_RMS_10" },
  { 1061, "Y_RMS_11" },
  { 1062, "Y_RMS_12" },
  { 1063, "Y_RMS_13" },
  { 1064, "Y_RMS_14" },
  { 1065, "Y_RMS_15" },
  { 1066, "Y_RMS_16" },
  { 1067, "Y_RMS_17" },
  { 1068, "Y_RMS_18" },
  { 1069, "Y_RMS_19" },
  { 1070, "Y_RMS_20" },
  { 1071, "Y_RMS_21" },
  { 1072, "Y_RMS_22" },
  { 1073, "Y_RMS_23" },
  { 1074, "Y_RMS_24" },
  { 1075, "Y_RMS_25" },
  { 1076, "Y_RMS_26" },
  { 1077, "Y_RMS_27" },
  { 1078, "Y_RMS_28" },
  { 1079, "Y_RMS_29" },
  { 1080, "Y_RMS_30" },
  { 1081, "Y_RMS_31" },
  { 1082, "Y_RMS_32" },
  { 1083, "Y_RMS_33" },
  { 1084, "Y_RMS_34" },
  { 1085, "Y_RMS_35" },
  { 1086, "Y_RMS_36" },
  { 1087, "Y_RMS_37" },
  { 1088, "Y_RMS_38" },
  { 1089, "Y_RMS_39" },
  { 1090, "Y_RMS_40" },
  { 1091, "Y_RMS_41" },
  { 1092, "Y_RMS_42" },
  { 1093, "Y_RMS_43" },
  { 1094, "Y_RMS_44" },
  { 1095, "Y_RMS_45" },
  { 1096, "Y_RMS_46" },
  { 1097, "Y_RMS_47" },
  { 1098, "Y_RMS_48" },
  { 1099, "Y_RMS_49" },

  // SITE_F_*
  { 1200, "SITE_F_00" },
  { 1201, "SITE_F_01" },
  { 1202, "SITE_F_02" },
  { 1203, "SITE_F_03" },
  { 1204, "SITE_F_04" },
  { 1205, "SITE_F_05" },
  { 1206, "SITE_F_06" },
  { 1207, "SITE_F_07" },
  { 1208, "SITE_F_08" },
  { 1209, "SITE_F_09" },
  { 1210, "SITE_F_10" },
  { 1211, "SITE_F_11" },
  { 1212, "SITE_F_12" },
  { 1213, "SITE_F_13" },
  { 1214, "SITE_F_14" },
  { 1215, "SITE_F_15" },
  { 1216, "SITE_F_16" },
  { 1217, "SITE_F_17" },
  { 1218, "SITE_F_18" },
  { 1219, "SITE_F_19" },
  { 1220, "SITE_F_20" },
  { 1221, "SITE_F_21" },
  { 1222, "SITE_F_22" },
  { 1223, "SITE_F_23" },
  { 1224, "SITE_F_24" },
  { 1225, "SITE_F_25" },
  { 1226, "SITE_F_26" },
  { 1227, "SITE_F_27" },
  { 1228, "SITE_F_28" },
  { 1229, "SITE_F_29" },
  { 1230, "SITE_F_30" },
  { 1231, "SITE_F_31" },
  { 1232, "SITE_F_32" },
  { 1233, "SITE_F_33" },
  { 1234, "SITE_F_34" },
  { 1235, "SITE_F_35" },
  { 1236, "SITE_F_36" },
  { 1237, "SITE_F_37" },
  { 1238, "SITE_F_38" },
  { 1239, "SITE_F_39" },
  { 1240, "SITE_F_40" },
  { 1241, "SITE_F_41" },
  { 1242, "SITE_F_42" },
  { 1243, "SITE_F_43" },
  { 1244, "SITE_F_44" },
  { 1245, "SITE_F_45" },
  { 1246, "SITE_F_46" },
  { 1247, "SITE_F_47" },
  { 1248, "SITE_F_48" },
  { 1249, "SITE_F_49" },

  // CREV_Y_*
  { 1250, "CREV_Y_00" },
  { 1251, "CREV_Y_01" },
  { 1252, "CREV_Y_02" },
  { 1253, "CREV_Y_03" },
  { 1254, "CREV_Y_04" },
  { 1255, "CREV_Y_05" },
  { 1256, "CREV_Y_06" },
  { 1257, "CREV_Y_07" },
  { 1258, "CREV_Y_08" },
  { 1259, "CREV_Y_09" },
  { 1260, "CREV_Y_10" },
  { 1261, "CREV_Y_11" },
  { 1262, "CREV_Y_12" },
  { 1263, "CREV_Y_13" },
  { 1264, "CREV_Y_14" },
  { 1265, "CREV_Y_15" },
  { 1266, "CREV_Y_16" },
  { 1267, "CREV_Y_17" },
  { 1268, "CREV_Y_18" },
  { 1269, "CREV_Y_19" },
  { 1270, "CREV_Y_20" },
  { 1271, "CREV_Y_21" },
  { 1272, "CREV_Y_22" },
  { 1273, "CREV_Y_23" },
  { 1274, "CREV_Y_24" },
  { 1275, "CREV_Y_25" },
  { 1276, "CREV_Y_26" },
  { 1277, "CREV_Y_27" },
  { 1278, "CREV_Y_28" },
  { 1279, "CREV_Y_29" },
  { 1280, "CREV_Y_30" },
  { 1281, "CREV_Y_31" },
  { 1282, "CREV_Y_32" },
  { 1283, "CREV_Y_33" },
  { 1284, "CREV_Y_34" },
  { 1285, "CREV_Y_35" },
  { 1286, "CREV_Y_36" },
  { 1287, "CREV_Y_37" },
  { 1288, "CREV_Y_38" },
  { 1289, "CREV_Y_39" },
  { 1290, "CREV_Y_40" },
  { 1291, "CREV_Y_41" },
  { 1292, "CREV_Y_42" },
  { 1293, "CREV_Y_43" },
  { 1294, "CREV_Y_44" },
  { 1295, "CREV_Y_45" },
  { 1296, "CREV_Y_46" },
  { 1297, "CREV_Y_47" },
  { 1298, "CREV_Y_48" },
  { 1299, "CREV_Y_49" },

  //
  { 1301, "WSB" },
  { 1302, "WSN" },
  { 1303, "WSR" },
  { 1304, "WSB_M1" },
  { 1305, "WSB_M2" },
  { 1306, "WSN_M1" },
  { 1307, "WSN_M2" },
  { 1308, "WSR_M1" },
  { 1309, "WSR_M2" },
  { 1310, "MASGEN" },
  { 1311, "NUCRAT" },
  { 1330, "TEMPERATURE_M1" },
  { 1331, "TEMPERATURE_M2" },

  // SURF_F_*
  { 1350, "SURF_F_00" },
  { 1351, "SURF_F_01" },
  { 1352, "SURF_F_02" },
  { 1353, "SURF_F_03" },
  { 1354, "SURF_F_04" },
  { 1355, "SURF_F_05" },
  { 1356, "SURF_F_06" },
  { 1357, "SURF_F_07" },
  { 1358, "SURF_F_08" },
  { 1359, "SURF_F_09" },
  { 1360, "SURF_F_10" },
  { 1361, "SURF_F_11" },
  { 1362, "SURF_F_12" },
  { 1363, "SURF_F_13" },
  { 1364, "SURF_F_14" },
  { 1365, "SURF_F_15" },
  { 1366, "SURF_F_16" },
  { 1367, "SURF_F_17" },
  { 1368, "SURF_F_18" },
  { 1369, "SURF_F_19" },
  { 1370, "SURF_F_20" },
  { 1371, "SURF_F_21" },
  { 1372, "SURF_F_22" },
  { 1373, "SURF_F_23" },
  { 1374, "SURF_F_24" },
  { 1375, "SURF_F_25" },
  { 1376, "SURF_F_26" },
  { 1377, "SURF_F_27" },
  { 1378, "SURF_F_28" },
  { 1379, "SURF_F_29" },
  { 1380, "SURF_F_30" },
  { 1381, "SURF_F_31" },
  { 1382, "SURF_F_32" },
  { 1383, "SURF_F_33" },
  { 1384, "SURF_F_34" },
  { 1385, "SURF_F_35" },
  { 1386, "SURF_F_36" },
  { 1387, "SURF_F_37" },
  { 1388, "SURF_F_38" },
  { 1389, "SURF_F_39" },
  { 1390, "SURF_F_40" },
  { 1391, "SURF_F_41" },
  { 1392, "SURF_F_42" },
  { 1393, "SURF_F_43" },
  { 1394, "SURF_F_44" },
  { 1395, "SURF_F_45" },
  { 1396, "SURF_F_46" },
  { 1397, "SURF_F_47" },
  { 1398, "SURF_F_48" },
  { 1399, "SURF_F_49" },

  // PB_DISC_*
  { 7700, "PB_DISC_00" },
  { 7701, "PB_DISC_01" },
  { 7702, "PB_DISC_02" },
  { 7703, "PB_DISC_03" },
  { 7704, "PB_DISC_04" },
  { 7705, "PB_DISC_05" },
  { 7706, "PB_DISC_06" },
  { 7707, "PB_DISC_07" },
  { 7708, "PB_DISC_08" },
  { 7709, "PB_DISC_09" },
  { 7710, "PB_DISC_10" },
  { 7711, "PB_DISC_11" },
  { 7712, "PB_DISC_12" },
  { 7713, "PB_DISC_13" },
  { 7714, "PB_DISC_14" },
  { 7715, "PB_DISC_15" },
  { 7716, "PB_DISC_16" },
  { 7717, "PB_DISC_17" },
  { 7718, "PB_DISC_18" },
  { 7719, "PB_DISC_19" },
  { 7720, "PB_DISC_20" },
  { 7721, "PB_DISC_21" },
  { 7722, "PB_DISC_22" },
  { 7723, "PB_DISC_23" },
  { 7724, "PB_DISC_24" },
  { 7725, "PB_DISC_25" },
  { 7726, "PB_DISC_26" },
  { 7727, "PB_DISC_27" },
  { 7728, "PB_DISC_28" },
  { 7729, "PB_DISC_29" },
  { 7730, "PB_DISC_30" },
  { 7731, "PB_DISC_31" },
  { 7732, "PB_DISC_32" },
  { 7733, "PB_DISC_33" },
  { 7734, "PB_DISC_34" },
  { 7735, "PB_DISC_35" },
  { 7736, "PB_DISC_36" },
  { 7737, "PB_DISC_37" },
  { 7738, "PB_DISC_38" },
  { 7739, "PB_DISC_39" },
  { 7740, "PB_DISC_40" },
  { 7741, "PB_DISC_41" },
  { 7742, "PB_DISC_42" },
  { 7743, "PB_DISC_43" },
  { 7744, "PB_DISC_44" },
  { 7745, "PB_DISC_45" },
  { 7746, "PB_DISC_46" },
  { 7747, "PB_DISC_47" },
  { 7748, "PB_DISC_48" },
  { 7749, "PB_DISC_49" },

  // PB_DISC_M1_*
  { 7750, "PB_DISC_M1_00" },
  { 7751, "PB_DISC_M1_01" },
  { 7752, "PB_DISC_M1_02" },
  { 7753, "PB_DISC_M1_03" },
  { 7754, "PB_DISC_M1_04" },
  { 7755, "PB_DISC_M1_05" },
  { 7756, "PB_DISC_M1_06" },
  { 7757, "PB_DISC_M1_07" },
  { 7758, "PB_DISC_M1_08" },
  { 7759, "PB_DISC_M1_09" },
  { 7760, "PB_DISC_M1_10" },
  { 7761, "PB_DISC_M1_11" },
  { 7762, "PB_DISC_M1_12" },
  { 7763, "PB_DISC_M1_13" },
  { 7764, "PB_DISC_M1_14" },
  { 7765, "PB_DISC_M1_15" },
  { 7766, "PB_DISC_M1_16" },
  { 7767, "PB_DISC_M1_17" },
  { 7768, "PB_DISC_M1_18" },
  { 7769, "PB_DISC_M1_19" },
  { 7770, "PB_DISC_M1_20" },
  { 7771, "PB_DISC_M1_21" },
  { 7772, "PB_DISC_M1_22" },
  { 7773, "PB_DISC_M1_23" },
  { 7774, "PB_DISC_M1_24" },
  { 7775, "PB_DISC_M1_25" },
  { 7776, "PB_DISC_M1_26" },
  { 7777, "PB_DISC_M1_27" },
  { 7778, "PB_DISC_M1_28" },
  { 7779, "PB_DISC_M1_29" },
  { 7780, "PB_DISC_M1_30" },
  { 7781, "PB_DISC_M1_31" },
  { 7782, "PB_DISC_M1_32" },
  { 7783, "PB_DISC_M1_33" },
  { 7784, "PB_DISC_M1_34" },
  { 7785, "PB_DISC_M1_35" },
  { 7786, "PB_DISC_M1_36" },
  { 7787, "PB_DISC_M1_37" },
  { 7788, "PB_DISC_M1_38" },
  { 7789, "PB_DISC_M1_39" },
  { 7790, "PB_DISC_M1_40" },
  { 7791, "PB_DISC_M1_41" },
  { 7792, "PB_DISC_M1_42" },
  { 7793, "PB_DISC_M1_43" },
  { 7794, "PB_DISC_M1_44" },
  { 7795, "PB_DISC_M1_45" },
  { 7796, "PB_DISC_M1_46" },
  { 7797, "PB_DISC_M1_47" },
  { 7798, "PB_DISC_M1_48" },
  { 7799, "PB_DISC_M1_49" },

  // PB_DISC_M2_*
  { 7800, "PB_DISC_M2_00" },
  { 7801, "PB_DISC_M2_01" },
  { 7802, "PB_DISC_M2_02" },
  { 7803, "PB_DISC_M2_03" },
  { 7804, "PB_DISC_M2_04" },
  { 7805, "PB_DISC_M2_05" },
  { 7806, "PB_DISC_M2_06" },
  { 7807, "PB_DISC_M2_07" },
  { 7808, "PB_DISC_M2_08" },
  { 7809, "PB_DISC_M2_09" },
  { 7810, "PB_DISC_M2_10" },
  { 7811, "PB_DISC_M2_11" },
  { 7812, "PB_DISC_M2_12" },
  { 7813, "PB_DISC_M2_13" },
  { 7814, "PB_DISC_M2_14" },
  { 7815, "PB_DISC_M2_15" },
  { 7816, "PB_DISC_M2_16" },
  { 7817, "PB_DISC_M2_17" },
  { 7818, "PB_DISC_M2_18" },
  { 7819, "PB_DISC_M2_19" },
  { 7820, "PB_DISC_M2_20" },
  { 7821, "PB_DISC_M2_21" },
  { 7822, "PB_DISC_M2_22" },
  { 7823, "PB_DISC_M2_23" },
  { 7824, "PB_DISC_M2_24" },
  { 7825, "PB_DISC_M2_25" },
  { 7826, "PB_DISC_M2_26" },
  { 7827, "PB_DISC_M2_27" },
  { 7828, "PB_DISC_M2_28" },
  { 7829, "PB_DISC_M2_29" },
  { 7830, "PB_DISC_M2_30" },
  { 7831, "PB_DISC_M2_31" },
  { 7832, "PB_DISC_M2_32" },
  { 7833, "PB_DISC_M2_33" },
  { 7834, "PB_DISC_M2_34" },
  { 7835, "PB_DISC_M2_35" },
  { 7836, "PB_DISC_M2_36" },
  { 7837, "PB_DISC_M2_37" },
  { 7838, "PB_DISC_M2_38" },
  { 7839, "PB_DISC_M2_39" },
  { 7840, "PB_DISC_M2_40" },
  { 7841, "PB_DISC_M2_41" },
  { 7842, "PB_DISC_M2_42" },
  { 7843, "PB_DISC_M2_43" },
  { 7844, "PB_DISC_M2_44" },
  { 7845, "PB_DISC_M2_45" },
  { 7846, "PB_DISC_M2_46" },
  { 7847, "PB_DISC_M2_47" },
  { 7848, "PB_DISC_M2_48" },
  { 7849, "PB_DISC_M2_49" },

  // PB_QMOM_*
  { 7850, "PB_QMOM_00" },
  { 7851, "PB_QMOM_01" },
  { 7852, "PB_QMOM_02" },
  { 7853, "PB_QMOM_03" },
  { 7854, "PB_QMOM_04" },
  { 7855, "PB_QMOM_05" },
  { 7856, "PB_QMOM_06" },
  { 7857, "PB_QMOM_07" },
  { 7858, "PB_QMOM_08" },
  { 7859, "PB_QMOM_09" },
  { 7860, "PB_QMOM_10" },
  { 7861, "PB_QMOM_11" },
  { 7862, "PB_QMOM_12" },
  { 7863, "PB_QMOM_13" },
  { 7864, "PB_QMOM_14" },
  { 7865, "PB_QMOM_15" },
  { 7866, "PB_QMOM_16" },
  { 7867, "PB_QMOM_17" },
  { 7868, "PB_QMOM_18" },
  { 7869, "PB_QMOM_19" },
  { 7870, "PB_QMOM_20" },
  { 7871, "PB_QMOM_21" },
  { 7872, "PB_QMOM_22" },
  { 7873, "PB_QMOM_23" },
  { 7874, "PB_QMOM_24" },
  { 7875, "PB_QMOM_25" },
  { 7876, "PB_QMOM_26" },
  { 7877, "PB_QMOM_27" },
  { 7878, "PB_QMOM_28" },
  { 7879, "PB_QMOM_29" },
  { 7880, "PB_QMOM_30" },
  { 7881, "PB_QMOM_31" },
  { 7882, "PB_QMOM_32" },
  { 7883, "PB_QMOM_33" },
  { 7884, "PB_QMOM_34" },
  { 7885, "PB_QMOM_35" },
  { 7886, "PB_QMOM_36" },
  { 7887, "PB_QMOM_37" },
  { 7888, "PB_QMOM_38" },
  { 7889, "PB_QMOM_39" },
  { 7890, "PB_QMOM_40" },
  { 7891, "PB_QMOM_41" },
  { 7892, "PB_QMOM_42" },
  { 7893, "PB_QMOM_43" },
  { 7894, "PB_QMOM_44" },
  { 7895, "PB_QMOM_45" },
  { 7896, "PB_QMOM_46" },
  { 7897, "PB_QMOM_47" },
  { 7898, "PB_QMOM_48" },
  { 7899, "PB_QMOM_49" },

  // PB_QMOM_M1_*
  { 7900, "PB_QMOM_M1_00" },
  { 7901, "PB_QMOM_M1_01" },
  { 7902, "PB_QMOM_M1_02" },
  { 7903, "PB_QMOM_M1_03" },
  { 7904, "PB_QMOM_M1_04" },
  { 7905, "PB_QMOM_M1_05" },
  { 7906, "PB_QMOM_M1_06" },
  { 7907, "PB_QMOM_M1_07" },
  { 7908, "PB_QMOM_M1_08" },
  { 7909, "PB_QMOM_M1_09" },
  { 7910, "PB_QMOM_M1_10" },
  { 7911, "PB_QMOM_M1_11" },
  { 7912, "PB_QMOM_M1_12" },
  { 7913, "PB_QMOM_M1_13" },
  { 7914, "PB_QMOM_M1_14" },
  { 7915, "PB_QMOM_M1_15" },
  { 7916, "PB_QMOM_M1_16" },
  { 7917, "PB_QMOM_M1_17" },
  { 7918, "PB_QMOM_M1_18" },
  { 7919, "PB_QMOM_M1_19" },
  { 7920, "PB_QMOM_M1_20" },
  { 7921, "PB_QMOM_M1_21" },
  { 7922, "PB_QMOM_M1_22" },
  { 7923, "PB_QMOM_M1_23" },
  { 7924, "PB_QMOM_M1_24" },
  { 7925, "PB_QMOM_M1_25" },
  { 7926, "PB_QMOM_M1_26" },
  { 7927, "PB_QMOM_M1_27" },
  { 7928, "PB_QMOM_M1_28" },
  { 7929, "PB_QMOM_M1_29" },
  { 7930, "PB_QMOM_M1_30" },
  { 7931, "PB_QMOM_M1_31" },
  { 7932, "PB_QMOM_M1_32" },
  { 7933, "PB_QMOM_M1_33" },
  { 7934, "PB_QMOM_M1_34" },
  { 7935, "PB_QMOM_M1_35" },
  { 7936, "PB_QMOM_M1_36" },
  { 7937, "PB_QMOM_M1_37" },
  { 7938, "PB_QMOM_M1_38" },
  { 7939, "PB_QMOM_M1_39" },
  { 7940, "PB_QMOM_M1_40" },
  { 7941, "PB_QMOM_M1_41" },
  { 7942, "PB_QMOM_M1_42" },
  { 7943, "PB_QMOM_M1_43" },
  { 7944, "PB_QMOM_M1_44" },
  { 7945, "PB_QMOM_M1_45" },
  { 7946, "PB_QMOM_M1_46" },
  { 7947, "PB_QMOM_M1_47" },
  { 7948, "PB_QMOM_M1_48" },
  { 7949, "PB_QMOM_M1_49" },

  // PB_QMOM_M2_*
  { 7950, "PB_QMOM_M2_00" },
  { 7951, "PB_QMOM_M2_01" },
  { 7952, "PB_QMOM_M2_02" },
  { 7953, "PB_QMOM_M2_03" },
  { 7954, "PB_QMOM_M2_04" },
  { 7955, "PB_QMOM_M2_05" },
  { 7956, "PB_QMOM_M2_06" },
  { 7957, "PB_QMOM_M2_07" },
  { 7958, "PB_QMOM_M2_08" },
  { 7959, "PB_QMOM_M2_09" },
  { 7960, "PB_QMOM_M2_10" },
  { 7961, "PB_QMOM_M2_11" },
  { 7962, "PB_QMOM_M2_12" },
  { 7963, "PB_QMOM_M2_13" },
  { 7964, "PB_QMOM_M2_14" },
  { 7965, "PB_QMOM_M2_15" },
  { 7966, "PB_QMOM_M2_16" },
  { 7967, "PB_QMOM_M2_17" },
  { 7968, "PB_QMOM_M2_18" },
  { 7969, "PB_QMOM_M2_19" },
  { 7970, "PB_QMOM_M2_20" },
  { 7971, "PB_QMOM_M2_21" },
  { 7972, "PB_QMOM_M2_22" },
  { 7973, "PB_QMOM_M2_23" },
  { 7974, "PB_QMOM_M2_24" },
  { 7975, "PB_QMOM_M2_25" },
  { 7976, "PB_QMOM_M2_26" },
  { 7977, "PB_QMOM_M2_27" },
  { 7978, "PB_QMOM_M2_28" },
  { 7979, "PB_QMOM_M2_29" },
  { 7980, "PB_QMOM_M2_30" },
  { 7981, "PB_QMOM_M2_31" },
  { 7982, "PB_QMOM_M2_32" },
  { 7983, "PB_QMOM_M2_33" },
  { 7984, "PB_QMOM_M2_34" },
  { 7985, "PB_QMOM_M2_35" },
  { 7986, "PB_QMOM_M2_36" },
  { 7987, "PB_QMOM_M2_37" },
  { 7988, "PB_QMOM_M2_38" },
  { 7989, "PB_QMOM_M2_39" },
  { 7990, "PB_QMOM_M2_40" },
  { 7991, "PB_QMOM_M2_41" },
  { 7992, "PB_QMOM_M2_42" },
  { 7993, "PB_QMOM_M2_43" },
  { 7994, "PB_QMOM_M2_44" },
  { 7995, "PB_QMOM_M2_45" },
  { 7996, "PB_QMOM_M2_46" },
  { 7997, "PB_QMOM_M2_47" },
  { 7998, "PB_QMOM_M2_48" },
  { 7999, "PB_QMOM_M2_49" },

  // PB_SMM_*
  { 8000, "PB_SMM_00" },
  { 8001, "PB_SMM_01" },
  { 8002, "PB_SMM_02" },
  { 8003, "PB_SMM_03" },
  { 8004, "PB_SMM_04" },
  { 8005, "PB_SMM_05" },
  { 8006, "PB_SMM_06" },
  { 8007, "PB_SMM_07" },
  { 8008, "PB_SMM_08" },
  { 8009, "PB_SMM_09" },
  { 8010, "PB_SMM_10" },
  { 8011, "PB_SMM_11" },
  { 8012, "PB_SMM_12" },
  { 8013, "PB_SMM_13" },
  { 8014, "PB_SMM_14" },
  { 8015, "PB_SMM_15" },
  { 8016, "PB_SMM_16" },
  { 8017, "PB_SMM_17" },
  { 8018, "PB_SMM_18" },
  { 8019, "PB_SMM_19" },
  { 8020, "PB_SMM_20" },
  { 8021, "PB_SMM_21" },
  { 8022, "PB_SMM_22" },
  { 8023, "PB_SMM_23" },
  { 8024, "PB_SMM_24" },
  { 8025, "PB_SMM_25" },
  { 8026, "PB_SMM_26" },
  { 8027, "PB_SMM_27" },
  { 8028, "PB_SMM_28" },
  { 8029, "PB_SMM_29" },
  { 8030, "PB_SMM_30" },
  { 8031, "PB_SMM_31" },
  { 8032, "PB_SMM_32" },
  { 8033, "PB_SMM_33" },
  { 8034, "PB_SMM_34" },
  { 8035, "PB_SMM_35" },
  { 8036, "PB_SMM_36" },
  { 8037, "PB_SMM_37" },
  { 8038, "PB_SMM_38" },
  { 8039, "PB_SMM_39" },
  { 8040, "PB_SMM_40" },
  { 8041, "PB_SMM_41" },
  { 8042, "PB_SMM_42" },
  { 8043, "PB_SMM_43" },
  { 8044, "PB_SMM_44" },
  { 8045, "PB_SMM_45" },
  { 8046, "PB_SMM_46" },
  { 8047, "PB_SMM_47" },
  { 8048, "PB_SMM_48" },
  { 8049, "PB_SMM_49" },

  // PB_SMM_M1_*
  { 8050, "PB_SMM_M1_00" },
  { 8051, "PB_SMM_M1_01" },
  { 8052, "PB_SMM_M1_02" },
  { 8053, "PB_SMM_M1_03" },
  { 8054, "PB_SMM_M1_04" },
  { 8055, "PB_SMM_M1_05" },
  { 8056, "PB_SMM_M1_06" },
  { 8057, "PB_SMM_M1_07" },
  { 8058, "PB_SMM_M1_08" },
  { 8059, "PB_SMM_M1_09" },
  { 8060, "PB_SMM_M1_10" },
  { 8061, "PB_SMM_M1_11" },
  { 8062, "PB_SMM_M1_12" },
  { 8063, "PB_SMM_M1_13" },
  { 8064, "PB_SMM_M1_14" },
  { 8065, "PB_SMM_M1_15" },
  { 8066, "PB_SMM_M1_16" },
  { 8067, "PB_SMM_M1_17" },
  { 8068, "PB_SMM_M1_18" },
  { 8069, "PB_SMM_M1_19" },
  { 8070, "PB_SMM_M1_20" },
  { 8071, "PB_SMM_M1_21" },
  { 8072, "PB_SMM_M1_22" },
  { 8073, "PB_SMM_M1_23" },
  { 8074, "PB_SMM_M1_24" },
  { 8075, "PB_SMM_M1_25" },
  { 8076, "PB_SMM_M1_26" },
  { 8077, "PB_SMM_M1_27" },
  { 8078, "PB_SMM_M1_28" },
  { 8079, "PB_SMM_M1_29" },
  { 8080, "PB_SMM_M1_30" },
  { 8081, "PB_SMM_M1_31" },
  { 8082, "PB_SMM_M1_32" },
  { 8083, "PB_SMM_M1_33" },
  { 8084, "PB_SMM_M1_34" },
  { 8085, "PB_SMM_M1_35" },
  { 8086, "PB_SMM_M1_36" },
  { 8087, "PB_SMM_M1_37" },
  { 8088, "PB_SMM_M1_38" },
  { 8089, "PB_SMM_M1_39" },
  { 8090, "PB_SMM_M1_40" },
  { 8091, "PB_SMM_M1_41" },
  { 8092, "PB_SMM_M1_42" },
  { 8093, "PB_SMM_M1_43" },
  { 8094, "PB_SMM_M1_44" },
  { 8095, "PB_SMM_M1_45" },
  { 8096, "PB_SMM_M1_46" },
  { 8097, "PB_SMM_M1_47" },
  { 8098, "PB_SMM_M1_48" },
  { 8099, "PB_SMM_M1_49" },

  // PB_SMM_M2_*
  { 8100, "PB_SMM_M2_00" },
  { 8101, "PB_SMM_M2_01" },
  { 8102, "PB_SMM_M2_02" },
  { 8103, "PB_SMM_M2_03" },
  { 8104, "PB_SMM_M2_04" },
  { 8105, "PB_SMM_M2_05" },
  { 8106, "PB_SMM_M2_06" },
  { 8107, "PB_SMM_M2_07" },
  { 8108, "PB_SMM_M2_08" },
  { 8109, "PB_SMM_M2_09" },
  { 8110, "PB_SMM_M2_10" },
  { 8111, "PB_SMM_M2_11" },
  { 8112, "PB_SMM_M2_12" },
  { 8113, "PB_SMM_M2_13" },
  { 8114, "PB_SMM_M2_14" },
  { 8115, "PB_SMM_M2_15" },
  { 8116, "PB_SMM_M2_16" },
  { 8117, "PB_SMM_M2_17" },
  { 8118, "PB_SMM_M2_18" },
  { 8119, "PB_SMM_M2_19" },
  { 8120, "PB_SMM_M2_20" },
  { 8121, "PB_SMM_M2_21" },
  { 8122, "PB_SMM_M2_22" },
  { 8123, "PB_SMM_M2_23" },
  { 8124, "PB_SMM_M2_24" },
  { 8125, "PB_SMM_M2_25" },
  { 8126, "PB_SMM_M2_26" },
  { 8127, "PB_SMM_M2_27" },
  { 8128, "PB_SMM_M2_28" },
  { 8129, "PB_SMM_M2_29" },
  { 8130, "PB_SMM_M2_30" },
  { 8131, "PB_SMM_M2_31" },
  { 8132, "PB_SMM_M2_32" },
  { 8133, "PB_SMM_M2_33" },
  { 8134, "PB_SMM_M2_34" },
  { 8135, "PB_SMM_M2_35" },
  { 8136, "PB_SMM_M2_36" },
  { 8137, "PB_SMM_M2_37" },
  { 8138, "PB_SMM_M2_38" },
  { 8139, "PB_SMM_M2_39" },
  { 8140, "PB_SMM_M2_40" },
  { 8141, "PB_SMM_M2_41" },
  { 8142, "PB_SMM_M2_42" },
  { 8143, "PB_SMM_M2_43" },
  { 8144, "PB_SMM_M2_44" },
  { 8145, "PB_SMM_M2_45" },
  { 8146, "PB_SMM_M2_46" },
  { 8147, "PB_SMM_M2_47" },
  { 8148, "PB_SMM_M2_48" },
  { 8149, "PB_SMM_M2_49" },
};

void vtkFLUENTReader::LoadVariableNames()
{
  for (auto const& variable : variable_info)
  {
    this->VariableNames->value[variable.index] = variable.name;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::ParseCaseFile()
{
  this->FluentCaseFile->clear();
  this->FluentCaseFile->seekg(0, ios::beg);

  while (this->GetCaseChunk())
  {

    int index = this->GetCaseIndex();
    switch (index)
    {
      case 0:
        break;
      case 1:
        break;
      case 2:
        this->GridDimension = this->GetDimension();
        break;
      case 4:
        this->GetLittleEndianFlag();
        break;
      case 10:
        this->GetNodesAscii();
        break;
      case 12:
        this->GetCellsAscii();
        break;
      case 13:
        this->GetFacesAscii();
        break;
      case 18:
        this->GetPeriodicShadowFacesAscii();
        break;
      case 37:
        this->GetSpeciesVariableNames();
        break;
      case 38:
        break;
      case 39:
        break;
      case 40:
        break;
      case 41:
        break;
      case 45:
        break;
      case 58:
        this->GetCellTreeAscii();
        break;
      case 59:
        this->GetFaceTreeAscii();
        break;
      case 61:
        this->GetInterfaceFaceParentsAscii();
        break;
      case 62:
        this->GetNonconformalGridInterfaceFaceInformationAscii();
        break;
      case 63:
        break;
      case 64:
        break;
      case 2010:
        this->GetNodesSinglePrecision();
        break;
      case 3010:
        this->GetNodesDoublePrecision();
        break;
      case 2012:
        this->GetCellsBinary();
        break;
      case 3012:
        this->GetCellsBinary(); // Should be the same as single precision..
                                // only grabbing ints.
        break;
      case 2013:
        this->GetFacesBinary();
        break;
      case 3013:
        this->GetFacesBinary();
        break;
      case 2018:
        this->GetPeriodicShadowFacesBinary();
        break;
      case 3018:
        this->GetPeriodicShadowFacesBinary();
        break;
      case 2040:
        break;
      case 3040:
        break;
      case 2041:
        break;
      case 3041:
        break;
      case 2058:
        this->GetCellTreeBinary();
        break;
      case 3058:
        this->GetCellTreeBinary();
        break;
      case 2059:
        this->GetFaceTreeBinary();
        break;
      case 3059:
        this->GetFaceTreeBinary();
        break;
      case 2061:
        this->GetInterfaceFaceParentsBinary();
        break;
      case 3061:
        this->GetInterfaceFaceParentsBinary();
        break;
      case 2062:
        this->GetNonconformalGridInterfaceFaceInformationBinary();
        break;
      case 3062:
        this->GetNonconformalGridInterfaceFaceInformationBinary();
        break;
      case 2063:
        break;
      case 3063:
        break;
      default:
        // cout << "Undefined Section = " << index << endl;
        break;
    }
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetDimension()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  std::string info = this->CaseBuffer->value.substr(start + 4, 1);
  return atoi(info.c_str());
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetLittleEndianFlag()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  int flag;
  sscanf(info.c_str(), "%d", &flag);

  if (flag == 60)
  {
    this->SetDataByteOrderToLittleEndian();
  }
  else
  {
    this->SetDataByteOrderToBigEndian();
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetNodesAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int zoneId, firstIndex, lastIndex;
  int type, nd;
  sscanf(info.c_str(), "%x %x %x %d %d", &zoneId, &firstIndex, &lastIndex, &type, &nd);

  if (this->CaseBuffer->value.at(5) == '0')
  {
    this->Points->Allocate(lastIndex);
  }
  else
  {
    size_t dstart = this->CaseBuffer->value.find('(', 5);
    size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
    std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
    std::stringstream pdatastream(pdata);

    double x, y, z;
    if (this->GridDimension == 3)
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        pdatastream >> x;
        pdatastream >> y;
        pdatastream >> z;
        this->Points->InsertPoint(i - 1, x, y, z);
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        pdatastream >> x;
        pdatastream >> y;
        this->Points->InsertPoint(i - 1, x, y, 0.0);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetNodesSinglePrecision()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int zoneId, firstIndex, lastIndex;
  int type;
  sscanf(info.c_str(), "%x %x %x %d", &zoneId, &firstIndex, &lastIndex, &type);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  double x, y, z;
  if (this->GridDimension == 3)
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      x = this->GetCaseBufferFloat(static_cast<int>(ptr));
      ptr = ptr + 4;

      y = this->GetCaseBufferFloat(static_cast<int>(ptr));
      ptr = ptr + 4;

      z = this->GetCaseBufferFloat(static_cast<int>(ptr));
      ptr = ptr + 4;
      this->Points->InsertPoint(i - 1, x, y, z);
    }
  }
  else
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      x = this->GetCaseBufferFloat(static_cast<int>(ptr));
      ptr = ptr + 4;

      y = this->GetCaseBufferFloat(static_cast<int>(ptr));
      ptr = ptr + 4;

      z = 0.0;

      this->Points->InsertPoint(i - 1, x, y, z);
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetNodesDoublePrecision()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int zoneId, firstIndex, lastIndex;
  int type;
  sscanf(info.c_str(), "%x %x %x %d", &zoneId, &firstIndex, &lastIndex, &type);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  if (this->GridDimension == 3)
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      double x = this->GetCaseBufferDouble(static_cast<int>(ptr));
      ptr = ptr + 8;

      double y = this->GetCaseBufferDouble(static_cast<int>(ptr));
      ptr = ptr + 8;

      double z = this->GetCaseBufferDouble(static_cast<int>(ptr));
      ptr = ptr + 8;
      this->Points->InsertPoint(i - 1, x, y, z);
    }
  }
  else
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      double x = this->GetCaseBufferDouble(static_cast<int>(ptr));
      ptr = ptr + 8;

      double y = this->GetCaseBufferDouble(static_cast<int>(ptr));
      ptr = ptr + 8;

      this->Points->InsertPoint(i - 1, x, y, 0.0);
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetCellsAscii()
{
  if (this->CaseBuffer->value.at(5) == '0')
  { // Cell Info
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')', 1);
    std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
    unsigned int zoneId, firstIndex, lastIndex;
    int type;
    sscanf(info.c_str(), "%x %x %x %d", &zoneId, &firstIndex, &lastIndex, &type);
    this->Cells->value.resize(lastIndex);
  }
  else
  { // Cell Definitions
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')', 1);
    std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
    unsigned int zoneId, firstIndex, lastIndex;
    int type, elementType;
    sscanf(info.c_str(), "%x %x %x %d %d", &zoneId, &firstIndex, &lastIndex, &type, &elementType);

    if (elementType == 0)
    {
      size_t dstart = this->CaseBuffer->value.find('(', 5);
      size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
      std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
      std::stringstream pdatastream(pdata);
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        pdatastream >> this->Cells->value[i - 1].type;
        this->Cells->value[i - 1].zone = zoneId;
        this->Cells->value[i - 1].parent = 0;
        this->Cells->value[i - 1].child = 0;
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        this->Cells->value[i - 1].type = elementType;
        this->Cells->value[i - 1].zone = zoneId;
        this->Cells->value[i - 1].parent = 0;
        this->Cells->value[i - 1].child = 0;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetCellsBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int zoneId, firstIndex, lastIndex, type, elementType;
  sscanf(info.c_str(), "%x %x %x %x %x", &zoneId, &firstIndex, &lastIndex, &type, &elementType);

  if (elementType == 0)
  {
    size_t dstart = this->CaseBuffer->value.find('(', 7);
    size_t ptr = dstart + 1;
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      this->Cells->value[i - 1].type = this->GetCaseBufferInt(static_cast<int>(ptr));
      ptr = ptr + 4;
      this->Cells->value[i - 1].zone = zoneId;
      this->Cells->value[i - 1].parent = 0;
      this->Cells->value[i - 1].child = 0;
    }
  }
  else
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      this->Cells->value[i - 1].type = elementType;
      this->Cells->value[i - 1].zone = zoneId;
      this->Cells->value[i - 1].parent = 0;
      this->Cells->value[i - 1].child = 0;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetFacesAscii()
{

  if (this->CaseBuffer->value.at(5) == '0')
  { // Face Info
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')', 1);
    std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
    unsigned int zoneId, firstIndex, lastIndex, bcType;
    sscanf(info.c_str(), "%x %x %x %x", &zoneId, &firstIndex, &lastIndex, &bcType);

    this->Faces->value.resize(lastIndex);
  }
  else
  { // Face Definitions
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')', 1);
    std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
    unsigned int zoneId, firstIndex, lastIndex, bcType, faceType;
    sscanf(info.c_str(), "%x %x %x %x %x", &zoneId, &firstIndex, &lastIndex, &bcType, &faceType);

    size_t dstart = this->CaseBuffer->value.find('(', 7);
    size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
    std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
    std::stringstream pdatastream(pdata);

    int numberOfNodesInFace = 0;
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      if (faceType == 0 || faceType == 5)
      {
        pdatastream >> numberOfNodesInFace;
      }
      else
      {
        numberOfNodesInFace = faceType;
      }
      this->Faces->value[i - 1].nodes.resize(numberOfNodesInFace);
      for (int j = 0; j < numberOfNodesInFace; j++)
      {
        pdatastream >> hex >> this->Faces->value[i - 1].nodes[j];
        this->Faces->value[i - 1].nodes[j]--;
      }
      pdatastream >> hex >> this->Faces->value[i - 1].c0;
      pdatastream >> hex >> this->Faces->value[i - 1].c1;
      this->Faces->value[i - 1].c0--;
      this->Faces->value[i - 1].c1--;
      this->Faces->value[i - 1].type = numberOfNodesInFace;
      this->Faces->value[i - 1].zone = zoneId;
      this->Faces->value[i - 1].periodicShadow = 0;
      this->Faces->value[i - 1].parent = 0;
      this->Faces->value[i - 1].child = 0;
      this->Faces->value[i - 1].interfaceFaceParent = 0;
      this->Faces->value[i - 1].ncgParent = 0;
      this->Faces->value[i - 1].ncgChild = 0;
      this->Faces->value[i - 1].interfaceFaceChild = 0;
      if (this->Faces->value[i - 1].c0 >= 0)
      {
        this->Cells->value[this->Faces->value[i - 1].c0].faces.push_back(i - 1);
      }
      if (this->Faces->value[i - 1].c1 >= 0)
      {
        this->Cells->value[this->Faces->value[i - 1].c1].faces.push_back(i - 1);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetFacesBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int zoneId, firstIndex, lastIndex, bcType, faceType;
  sscanf(info.c_str(), "%x %x %x %x %x", &zoneId, &firstIndex, &lastIndex, &bcType, &faceType);
  size_t dstart = this->CaseBuffer->value.find('(', 7);
  int numberOfNodesInFace = 0;
  size_t ptr = dstart + 1;
  for (unsigned int i = firstIndex; i <= lastIndex; i++)
  {
    if ((faceType == 0) || (faceType == 5))
    {
      numberOfNodesInFace = this->GetCaseBufferInt(static_cast<int>(ptr));
      ptr = ptr + 4;
    }
    else
    {
      numberOfNodesInFace = faceType;
    }

    this->Faces->value[i - 1].nodes.resize(numberOfNodesInFace);

    for (int k = 0; k < numberOfNodesInFace; k++)
    {
      this->Faces->value[i - 1].nodes[k] = this->GetCaseBufferInt(static_cast<int>(ptr));
      this->Faces->value[i - 1].nodes[k]--;
      ptr = ptr + 4;
    }

    this->Faces->value[i - 1].c0 = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    this->Faces->value[i - 1].c1 = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    this->Faces->value[i - 1].c0--;
    this->Faces->value[i - 1].c1--;
    this->Faces->value[i - 1].type = numberOfNodesInFace;
    this->Faces->value[i - 1].zone = zoneId;
    this->Faces->value[i - 1].periodicShadow = 0;
    this->Faces->value[i - 1].parent = 0;
    this->Faces->value[i - 1].child = 0;
    this->Faces->value[i - 1].interfaceFaceParent = 0;
    this->Faces->value[i - 1].ncgParent = 0;
    this->Faces->value[i - 1].ncgChild = 0;
    this->Faces->value[i - 1].interfaceFaceChild = 0;
    if (this->Faces->value[i - 1].c0 >= 0)
    {
      this->Cells->value[this->Faces->value[i - 1].c0].faces.push_back(i - 1);
    }
    if (this->Faces->value[i - 1].c1 >= 0)
    {
      this->Cells->value[this->Faces->value[i - 1].c1].faces.push_back(i - 1);
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetPeriodicShadowFacesAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int firstIndex, lastIndex, periodicZone, shadowZone;
  sscanf(info.c_str(), "%x %x %x %x", &firstIndex, &lastIndex, &periodicZone, &shadowZone);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
  std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
  std::stringstream pdatastream(pdata);

  int faceIndex1, faceIndex2;
  for (unsigned int i = firstIndex; i <= lastIndex; i++)
  {
    pdatastream >> hex >> faceIndex1;
    pdatastream >> hex >> faceIndex2;
    this->Faces->value[faceIndex1].periodicShadow = 1;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetPeriodicShadowFacesBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int firstIndex, lastIndex, periodicZone, shadowZone;
  sscanf(info.c_str(), "%x %x %x %x", &firstIndex, &lastIndex, &periodicZone, &shadowZone);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  // int faceIndex1, faceIndex2;
  for (unsigned int i = firstIndex; i <= lastIndex; i++)
  {
    // faceIndex1 = this->GetCaseBufferInt(ptr);
    this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    // faceIndex2 = this->GetCaseBufferInt(ptr);
    this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetCellTreeAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int cellId0, cellId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &cellId0, &cellId1, &parentZoneId, &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
  std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
  std::stringstream pdatastream(pdata);

  int numberOfKids, kid;
  for (unsigned int i = cellId0; i <= cellId1; i++)
  {
    this->Cells->value[i - 1].parent = 1;
    pdatastream >> hex >> numberOfKids;
    for (int j = 0; j < numberOfKids; j++)
    {
      pdatastream >> hex >> kid;
      this->Cells->value[kid - 1].child = 1;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetCellTreeBinary()
{

  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int cellId0, cellId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &cellId0, &cellId1, &parentZoneId, &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int numberOfKids, kid;
  for (unsigned int i = cellId0; i <= cellId1; i++)
  {
    this->Cells->value[i - 1].parent = 1;
    numberOfKids = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    for (int j = 0; j < numberOfKids; j++)
    {
      kid = this->GetCaseBufferInt(static_cast<int>(ptr));
      ptr = ptr + 4;
      this->Cells->value[kid - 1].child = 1;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetFaceTreeAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int faceId0, faceId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &faceId0, &faceId1, &parentZoneId, &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
  std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
  std::stringstream pdatastream(pdata);

  int numberOfKids, kid;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    this->Faces->value[i - 1].parent = 1;
    pdatastream >> hex >> numberOfKids;
    for (int j = 0; j < numberOfKids; j++)
    {
      pdatastream >> hex >> kid;
      this->Faces->value[kid - 1].child = 1;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetFaceTreeBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int faceId0, faceId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &faceId0, &faceId1, &parentZoneId, &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int numberOfKids, kid;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    this->Faces->value[i - 1].parent = 1;
    numberOfKids = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    for (int j = 0; j < numberOfKids; j++)
    {
      kid = this->GetCaseBufferInt(static_cast<int>(ptr));
      ptr = ptr + 4;
      this->Faces->value[kid - 1].child = 1;
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetInterfaceFaceParentsAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int faceId0, faceId1;
  sscanf(info.c_str(), "%x %x", &faceId0, &faceId1);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
  std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
  std::stringstream pdatastream(pdata);

  int parentId0, parentId1;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    pdatastream >> hex >> parentId0;
    pdatastream >> hex >> parentId1;
    this->Faces->value[parentId0 - 1].interfaceFaceParent = 1;
    this->Faces->value[parentId1 - 1].interfaceFaceParent = 1;
    this->Faces->value[i - 1].interfaceFaceChild = 1;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetInterfaceFaceParentsBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  unsigned int faceId0, faceId1;
  sscanf(info.c_str(), "%x %x", &faceId0, &faceId1);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int parentId0, parentId1;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    parentId0 = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    parentId1 = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    this->Faces->value[parentId0 - 1].interfaceFaceParent = 1;
    this->Faces->value[parentId1 - 1].interfaceFaceParent = 1;
    this->Faces->value[i - 1].interfaceFaceChild = 1;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetNonconformalGridInterfaceFaceInformationAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = this->CaseBuffer->value.substr(start + 1, end - start - 1);
  int kidId, parentId, numberOfFaces;
  sscanf(info.c_str(), "%d %d %d", &kidId, &parentId, &numberOfFaces);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart + 1);
  std::string pdata = this->CaseBuffer->value.substr(dstart + 1, dend - start - 1);
  std::stringstream pdatastream(pdata);

  int child, parent;
  for (int i = 0; i < numberOfFaces; i++)
  {
    pdatastream >> hex >> child;
    pdatastream >> hex >> parent;
    this->Faces->value[child - 1].ncgChild = 1;
    this->Faces->value[parent - 1].ncgParent = 1;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetNonconformalGridInterfaceFaceInformationBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')', 1);
  std::string info = CaseBuffer->value.substr(start + 1, end - start - 1);
  int kidId, parentId, numberOfFaces;
  sscanf(info.c_str(), "%d %d %d", &kidId, &parentId, &numberOfFaces);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int child, parent;
  for (int i = 0; i < numberOfFaces; i++)
  {
    child = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    parent = this->GetCaseBufferInt(static_cast<int>(ptr));
    ptr = ptr + 4;
    this->Faces->value[child - 1].ncgChild = 1;
    this->Faces->value[parent - 1].ncgParent = 1;
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::CleanCells()
{

  std::vector<int> t;
  for (size_t i = 0; i < Cells->value.size(); i++)
  {

    if (((this->Cells->value[i].type == 1) && (this->Cells->value[i].faces.size() != 3)) ||
      ((this->Cells->value[i].type == 2) && (this->Cells->value[i].faces.size() != 4)) ||
      ((this->Cells->value[i].type == 3) && (this->Cells->value[i].faces.size() != 4)) ||
      ((this->Cells->value[i].type == 4) && (this->Cells->value[i].faces.size() != 6)) ||
      ((this->Cells->value[i].type == 5) && (this->Cells->value[i].faces.size() != 5)) ||
      ((this->Cells->value[i].type == 6) && (this->Cells->value[i].faces.size() != 5)))
    {

      // Copy faces
      t.clear();
      for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
      {
        t.push_back(this->Cells->value[i].faces[j]);
      }

      // Clear Faces
      this->Cells->value[i].faces.clear();

      // Copy the faces that are not flagged back into the cell
      for (size_t j = 0; j < t.size(); j++)
      {
        if ((this->Faces->value[t[j]].child == 0) && (this->Faces->value[t[j]].ncgChild == 0) &&
          (this->Faces->value[t[j]].interfaceFaceChild == 0))
        {
          this->Cells->value[i].faces.push_back(t[j]);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulateCellNodes()
{
  for (size_t i = 0; i < this->Cells->value.size(); i++)
  {
    const vtkIdType id = static_cast<vtkIdType>(i);
    switch (this->Cells->value[i].type)
    {
      case 1: // Triangle
        this->PopulateTriangleCell(id);
        break;

      case 2: // Tetrahedron
        this->PopulateTetraCell(id);
        break;

      case 3: // Quadrilateral
        this->PopulateQuadCell(id);
        break;

      case 4: // Hexahedral
        this->PopulateHexahedronCell(id);
        break;

      case 5: // Pyramid
        this->PopulatePyramidCell(id);
        break;

      case 6: // Wedge
        this->PopulateWedgeCell(id);
        break;

      case 7: // Polyhedron
        this->PopulatePolyhedronCell(id);
        break;
    }
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetCaseBufferInt(int ptr)
{
  union mix_i {
    int i;
    char c[4];
  } mi = { 1 };

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mi.c[3 - j] = this->CaseBuffer->value.at(ptr + j);
    }
    else
    {
      mi.c[j] = this->CaseBuffer->value.at(ptr + j);
    }
  }
  return mi.i;
}

//------------------------------------------------------------------------------
float vtkFLUENTReader::GetCaseBufferFloat(int ptr)
{
  union mix_f {
    float f;
    char c[4];
  } mf = { 1.0 };

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mf.c[3 - j] = this->CaseBuffer->value.at(ptr + j);
    }
    else
    {
      mf.c[j] = this->CaseBuffer->value.at(ptr + j);
    }
  }
  return mf.f;
}

//------------------------------------------------------------------------------
double vtkFLUENTReader::GetCaseBufferDouble(int ptr)
{
  union mix_i {
    double d;
    char c[8];
  } md = { 1.0 };

  for (int j = 0; j < 8; j++)
  {
    if (this->GetSwapBytes())
    {
      md.c[7 - j] = this->CaseBuffer->value.at(ptr + j);
    }
    else
    {
      md.c[j] = this->CaseBuffer->value.at(ptr + j);
    }
  }
  return md.d;
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulateTriangleCell(int i)
{
  this->Cells->value[i].nodes.resize(3);
  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }

  if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
      this->Cells->value[i].nodes[0] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] != this->Cells->value[i].nodes[1])
  {
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
  }
  else
  {
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulateTetraCell(int i)
{
  this->Cells->value[i].nodes.resize(4);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[2];
  }
  else
  {
    this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[2];
  }

  if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
      this->Cells->value[i].nodes[0] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] != this->Cells->value[i].nodes[1] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] != this->Cells->value[i].nodes[2])
  {
    this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
  }
  else if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
      this->Cells->value[i].nodes[0] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] != this->Cells->value[i].nodes[1] &&
    this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] != this->Cells->value[i].nodes[2])
  {
    this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[2];
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulateQuadCell(int i)
{
  this->Cells->value[i].nodes.resize(4);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[1] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[0] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }

  if ((this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
          this->Cells->value[i].nodes[0] &&
        this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
          this->Cells->value[i].nodes[1]) &&
    (this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
        this->Cells->value[i].nodes[0] &&
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
        this->Cells->value[i].nodes[1]))
  {
    if (this->Faces->value[this->Cells->value[i].faces[1]].c0 == i)
    {
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
    }
  }
  else if ((this->Faces->value[this->Cells->value[i].faces[2]].nodes[0] !=
               this->Cells->value[i].nodes[0] &&
             this->Faces->value[this->Cells->value[i].faces[2]].nodes[0] !=
               this->Cells->value[i].nodes[1]) &&
    (this->Faces->value[this->Cells->value[i].faces[2]].nodes[1] !=
        this->Cells->value[i].nodes[0] &&
      this->Faces->value[this->Cells->value[i].faces[2]].nodes[1] !=
        this->Cells->value[i].nodes[1]))
  {
    if (this->Faces->value[this->Cells->value[i].faces[2]].c0 == i)
    {
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[0];
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[0];
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[2]].nodes[1];
    }
  }
  else
  {
    if (this->Faces->value[this->Cells->value[i].faces[3]].c0 == i)
    {
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[0];
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[0];
      this->Cells->value[i].nodes[2] = this->Faces->value[this->Cells->value[i].faces[3]].nodes[1];
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulateHexahedronCell(int i)
{
  this->Cells->value[i].nodes.resize(8);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    for (int j = 0; j < 4; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[this->Cells->value[i].faces[0]].nodes[j];
    }
  }
  else
  {
    for (int j = 3; j >= 0; j--)
    {
      this->Cells->value[i].nodes[3 - j] =
        this->Faces->value[this->Cells->value[i].faces[0]].nodes[j];
    }
  }

  //  Look for opposite face of hexahedron
  for (int j = 1; j < 6; j++)
  {
    int flag = 0;
    for (int k = 0; k < 4; k++)
    {
      if ((this->Cells->value[i].nodes[0] ==
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
        (this->Cells->value[i].nodes[1] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
        (this->Cells->value[i].nodes[2] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
        (this->Cells->value[i].nodes[3] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]))
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c1 == i)
      {
        for (int k = 4; k < 8; k++)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k - 4];
        }
      }
      else
      {
        for (int k = 7; k >= 4; k--)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[7 - k];
        }
      }
    }
  }

  //  Find the face with points 0 and 1 in them.
  int f01[4] = { -1, -1, -1, -1 };
  for (int j = 1; j < 6; j++)
  {
    int flag0 = 0;
    int flag1 = 0;
    for (int k = 0; k < 4; k++)
    {
      if (this->Cells->value[i].nodes[0] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells->value[i].nodes[1] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }
    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          f01[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 3; k >= 0; k--)
        {
          f01[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  //  Find the face with points 0 and 3 in them.
  int f03[4] = { -1, -1, -1, -1 };
  for (int j = 1; j < 6; j++)
  {
    int flag0 = 0;
    int flag1 = 0;
    for (int k = 0; k < 4; k++)
    {
      if (this->Cells->value[i].nodes[0] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells->value[i].nodes[3] ==
        this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }

    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          f03[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 3; k >= 0; k--)
        {
          f03[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // What point is in f01 and f03 besides 0 ... this is point 4
  int p4 = 0;
  for (int k = 0; k < 4; k++)
  {
    if (f01[k] != this->Cells->value[i].nodes[0])
    {
      for (int n = 0; n < 4; n++)
      {
        if (f01[k] == f03[n])
        {
          p4 = f01[k];
        }
      }
    }
  }

  // Since we know point 4 now we check to see if points
  //  4, 5, 6, and 7 are in the correct positions.
  int t[8];
  t[4] = this->Cells->value[i].nodes[4];
  t[5] = this->Cells->value[i].nodes[5];
  t[6] = this->Cells->value[i].nodes[6];
  t[7] = this->Cells->value[i].nodes[7];
  if (p4 == this->Cells->value[i].nodes[5])
  {
    this->Cells->value[i].nodes[5] = t[6];
    this->Cells->value[i].nodes[6] = t[7];
    this->Cells->value[i].nodes[7] = t[4];
    this->Cells->value[i].nodes[4] = t[5];
  }
  else if (p4 == Cells->value[i].nodes[6])
  {
    this->Cells->value[i].nodes[5] = t[7];
    this->Cells->value[i].nodes[6] = t[4];
    this->Cells->value[i].nodes[7] = t[5];
    this->Cells->value[i].nodes[4] = t[6];
  }
  else if (p4 == Cells->value[i].nodes[7])
  {
    this->Cells->value[i].nodes[5] = t[4];
    this->Cells->value[i].nodes[6] = t[5];
    this->Cells->value[i].nodes[7] = t[6];
    this->Cells->value[i].nodes[4] = t[7];
  }
  // else point 4 was lined up so everything was correct.
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulatePyramidCell(int i)
{
  this->Cells->value[i].nodes.resize(5);
  //  The quad face will be the base of the pyramid
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Faces->value[this->Cells->value[i].faces[j]].nodes.size() == 4)
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells->value[i].nodes[3 - k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // Just need to find point 4
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Faces->value[this->Cells->value[i].faces[j]].nodes.size() == 3)
    {
      for (int k = 0; k < 3; k++)
      {
        if ((this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
              this->Cells->value[i].nodes[0]) &&
          (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
            this->Cells->value[i].nodes[1]) &&
          (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
            this->Cells->value[i].nodes[2]) &&
          (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
            this->Cells->value[i].nodes[3]))
        {
          this->Cells->value[i].nodes[4] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulateWedgeCell(int i)
{
  this->Cells->value[i].nodes.resize(6);

  //  Find the first triangle face and make it the base.
  int base = 0;
  int first = 0;
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if ((this->Faces->value[this->Cells->value[i].faces[j]].type == 3) && (first == 0))
    {
      base = this->Cells->value[i].faces[j];
      first = 1;
    }
  }

  //  Find the second triangle face and make it the top.
  int top = 0;
  int second = 0;
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if ((this->Faces->value[this->Cells->value[i].faces[j]].type == 3) && (second == 0) &&
      (this->Cells->value[i].faces[j] != base))
    {
      top = this->Cells->value[i].faces[j];
      second = 1;
    }
  }

  // Load Base nodes into the nodes std::vector
  if (this->Faces->value[base].c0 == i)
  {
    for (int j = 0; j < 3; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[base].nodes[j];
    }
  }
  else
  {
    for (int j = 2; j >= 0; j--)
    {
      this->Cells->value[i].nodes[2 - j] = this->Faces->value[base].nodes[j];
    }
  }
  // Load Top nodes into the nodes std::vector
  if (this->Faces->value[top].c1 == i)
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[top].nodes[j - 3];
    }
  }
  else
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[top].nodes[5 - j];
    }
  }

  //  Find the quad face with points 0 and 1 in them.
  int w01[4] = { -1, -1, -1, -1 };
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Cells->value[i].faces[j] != base && this->Cells->value[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf1 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells->value[i].nodes[0] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells->value[i].nodes[1] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf1 = 1;
        }
        if ((wf0 == 1) && (wf1 == 1))
        {
          for (int n = 0; n < 4; n++)
          {
            w01[n] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  //  Find the quad face with points 0 and 2 in them.
  int w02[4] = { -1, -1, -1, -1 };
  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    if (this->Cells->value[i].faces[j] != base && this->Cells->value[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf2 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells->value[i].nodes[0] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells->value[i].nodes[2] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf2 = 1;
        }
        if ((wf0 == 1) && (wf2 == 1))
        {
          for (int n = 0; n < 4; n++)
          {
            w02[n] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  // Point 3 is the point that is in both w01 and w02

  // What point is in f01 and f02 besides 0 ... this is point 3
  int p3 = 0;
  for (int k = 0; k < 4; k++)
  {
    if (w01[k] != this->Cells->value[i].nodes[0])
    {
      for (int n = 0; n < 4; n++)
      {
        if (w01[k] == w02[n])
        {
          p3 = w01[k];
        }
      }
    }
  }

  // Since we know point 3 now we check to see if points
  //  3, 4, and 5 are in the correct positions.
  int t[6];
  t[3] = this->Cells->value[i].nodes[3];
  t[4] = this->Cells->value[i].nodes[4];
  t[5] = this->Cells->value[i].nodes[5];
  if (p3 == this->Cells->value[i].nodes[4])
  {
    this->Cells->value[i].nodes[3] = t[4];
    this->Cells->value[i].nodes[4] = t[5];
    this->Cells->value[i].nodes[5] = t[3];
  }
  else if (p3 == this->Cells->value[i].nodes[5])
  {
    this->Cells->value[i].nodes[3] = t[5];
    this->Cells->value[i].nodes[4] = t[3];
    this->Cells->value[i].nodes[5] = t[4];
  }
  // else point 3 was lined up so everything was correct.
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::PopulatePolyhedronCell(int i)
{
  //  We can't set the size on the nodes std::vector because we
  //  are not sure how many we are going to have.
  //  All we have to do here is add the nodes from the faces into
  //  nodes std::vector within the cell.  All we have to check for is
  //  duplicate nodes.
  //
  // cout << "number of faces in cell = " << Cells[i].faces.size() << endl;

  for (size_t j = 0; j < this->Cells->value[i].faces.size(); j++)
  {
    // cout << "number of nodes in face = " <<
    // Faces[Cells[i].faces[j]].nodes.size() << endl;
    for (size_t k = 0; k < this->Faces->value[this->Cells->value[i].faces[j]].nodes.size(); k++)
    {
      int flag;
      flag = 0;
      // Is the node already in the cell?
      for (size_t n = 0; n < Cells->value[i].nodes.size(); n++)
      {
        if (this->Cells->value[i].nodes[n] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          flag = 1;
        }
      }
      if (flag == 0)
      {
        // No match - insert node into cell.
        this->Cells->value[i].nodes.push_back(
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::ParseDataFile()
{
  while (this->GetDataChunk())
  {
    int index = this->GetDataIndex();
    switch (index)
    {
      case 0:
        // cout << "Comment Section" << endl;
        break;

      case 4:
        // cout << "Machine Configuration Section" << endl;
        break;

      case 33:
        // cout << "Grid Size Section" << endl;
        break;

      case 37:
        // cout << "Variables Section" << endl;
        break;

      case 300:
        // cout << "Data Section" << endl;
        GetData(1);
        break;

      case 301:
        // cout << "Residuals Section" << endl;
        break;

      case 302:
        // cout << "Residuals Section" << endl;
        break;

      case 2300:
        // cout << "Single Precision Data Section" << endl;
        GetData(2);
        break;

      case 2301:
        // cout << "Single Precision Residuals Section" << endl;
        break;

      case 2302:
        // cout << "Single Precision Residuals Section" << endl;
        break;

      case 3300:
        // cout << "Single Precision Data Section" << endl;
        GetData(3);
        break;

      case 3301:
        // cout << "Single Precision Residuals Section" << endl;
        break;

      case 3302:
        // cout << "Single Precision Residuals Section" << endl;
        break;

      default:
        // cout << "Data Undefined Section = " << index << endl;
        break;
    }
  }
}

//------------------------------------------------------------------------------
int vtkFLUENTReader::GetDataBufferInt(int ptr)
{
  union mix_i {
    int i;
    char c[4];
  } mi = { 1 };

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mi.c[3 - j] = this->DataBuffer->value.at(ptr + j);
    }
    else
    {
      mi.c[j] = this->DataBuffer->value.at(ptr + j);
    }
  }
  return mi.i;
}

//------------------------------------------------------------------------------
float vtkFLUENTReader::GetDataBufferFloat(int ptr)
{
  union mix_f {
    float f;
    char c[4];
  } mf = { 1.0 };

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mf.c[3 - j] = this->DataBuffer->value.at(ptr + j);
    }
    else
    {
      mf.c[j] = this->DataBuffer->value.at(ptr + j);
    }
  }
  return mf.f;
}

//------------------------------------------------------------------------------
double vtkFLUENTReader::GetDataBufferDouble(int ptr)
{
  union mix_i {
    double d;
    char c[8];
  } md = { 1.0 };

  for (int j = 0; j < 8; j++)
  {
    if (this->GetSwapBytes())
    {
      md.c[7 - j] = this->DataBuffer->value.at(ptr + j);
    }
    else
    {
      md.c[j] = this->DataBuffer->value.at(ptr + j);
    }
  }
  return md.d;
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetData(int dataType)
{
  size_t start = this->DataBuffer->value.find('(', 1);
  size_t end = this->DataBuffer->value.find(')', 1);
  std::string info = this->DataBuffer->value.substr(start + 1, end - start - 1);
  std::stringstream infostream(info);
  int subSectionId, zoneId, size, nTimeLevels, nPhases, firstId, lastId;
  infostream >> subSectionId >> zoneId >> size >> nTimeLevels >> nPhases >> firstId >> lastId;

  // Is this a cell zone?
  int zmatch = 0;
  for (size_t i = 0; i < this->CellZones->value.size(); i++)
  {
    if (this->CellZones->value[i] == zoneId)
    {
      zmatch = 1;
    }
  }

  if (zmatch)
  {

    // Set up stream or pointer to data
    size_t dstart = this->DataBuffer->value.find('(', 7);
    size_t dend = this->DataBuffer->value.find(')', dstart + 1);
    std::string pdata = this->DataBuffer->value.substr(dstart + 1, dend - dstart - 2);
    std::stringstream pdatastream(pdata);
    size_t ptr = dstart + 1;

    // Is this a new variable?
    int match = 0;
    for (size_t i = 0; i < this->SubSectionIds->value.size(); i++)
    {
      if (subSectionId == this->SubSectionIds->value[i])
      {
        match = 1;
      }
    }

    if ((match == 0) && (size < 4))
    { // new variable
      this->SubSectionIds->value.push_back(subSectionId);
      this->SubSectionSize->value.push_back(size);
      this->SubSectionZones->value.resize(this->SubSectionZones->value.size() + 1);
      this->SubSectionZones->value[this->SubSectionZones->value.size() - 1].push_back(zoneId);
    }

    if (size == 1)
    {
      this->NumberOfScalars++;
      this->ScalarDataChunks->value.resize(this->ScalarDataChunks->value.size() + 1);
      this->ScalarDataChunks->value[this->ScalarDataChunks->value.size() - 1].subsectionId =
        subSectionId;
      this->ScalarDataChunks->value[this->ScalarDataChunks->value.size() - 1].zoneId = zoneId;
      for (int i = firstId; i <= lastId; i++)
      {
        double temp;
        if (dataType == 1)
        {
          pdatastream >> temp;
        }
        else if (dataType == 2)
        {
          temp = this->GetDataBufferFloat(static_cast<int>(ptr));
          ptr = ptr + 4;
        }
        else
        {
          temp = this->GetDataBufferDouble(static_cast<int>(ptr));
          ptr = ptr + 8;
        }
        this->ScalarDataChunks->value[this->ScalarDataChunks->value.size() - 1]
          .scalarData.push_back(temp);
      }
    }
    else if (size == 3)
    {
      this->NumberOfVectors++;
      this->VectorDataChunks->value.resize(this->VectorDataChunks->value.size() + 1);
      this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1].subsectionId =
        subSectionId;
      this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1].zoneId = zoneId;
      for (int i = firstId; i <= lastId; i++)
      {
        double tempx, tempy, tempz;

        if (dataType == 1)
        {
          pdatastream >> tempx;
          pdatastream >> tempy;
          pdatastream >> tempz;
        }
        else if (dataType == 2)
        {
          tempx = this->GetDataBufferFloat(static_cast<int>(ptr));
          ptr = ptr + 4;
          tempy = this->GetDataBufferFloat(static_cast<int>(ptr));
          ptr = ptr + 4;
          tempz = this->GetDataBufferFloat(static_cast<int>(ptr));
          ptr = ptr + 4;
        }
        else
        {
          tempx = this->GetDataBufferDouble(static_cast<int>(ptr));
          ptr = ptr + 8;
          tempy = this->GetDataBufferDouble(static_cast<int>(ptr));
          ptr = ptr + 8;
          tempz = this->GetDataBufferDouble(static_cast<int>(ptr));
          ptr = ptr + 8;
        }
        this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
          .iComponentData.push_back(tempx);
        this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
          .jComponentData.push_back(tempy);
        this->VectorDataChunks->value[this->VectorDataChunks->value.size() - 1]
          .kComponentData.push_back(tempz);
      }
    }
    else
    {
      // cout << "Weird Variable Size = " << size << endl;
    }
  }
}
//------------------------------------------------------------------------------
void vtkFLUENTReader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}
//------------------------------------------------------------------------------
void vtkFLUENTReader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}
//------------------------------------------------------------------------------
void vtkFLUENTReader::SetDataByteOrder(int byteOrder)
{
  if (byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN)
  {
    this->SetDataByteOrderToBigEndian();
  }
  else
  {
    this->SetDataByteOrderToLittleEndian();
  }
}
//------------------------------------------------------------------------------
int vtkFLUENTReader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if (this->SwapBytes)
  {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
  else
  {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
#else
  if (this->SwapBytes)
  {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
  else
  {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
#endif
}
//------------------------------------------------------------------------------
const char* vtkFLUENTReader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if (this->SwapBytes)
  {
    return "LittleEndian";
  }
  else
  {
    return "BigEndian";
  }
#else
  if (this->SwapBytes)
  {
    return "BigEndian";
  }
  else
  {
    return "LittleEndian";
  }
#endif
}
//------------------------------------------------------------------------------
void vtkFLUENTReader::GetSpeciesVariableNames()
{
  // Locate the "(species (names" entry
  std::string variables = this->CaseBuffer->value;
  size_t startPos = variables.find("(species (names (") + 17;
  if (startPos != std::string::npos)
  {
    variables.erase(0, startPos);

    size_t endPos = variables.find(')');
    variables.erase(endPos);

    std::stringstream tokenizer(variables);

    int iterator = 0;

    while (!tokenizer.eof())
    {
      std::string temp;
      tokenizer >> temp;

      this->VariableNames->value[200 + iterator] = temp;
      this->VariableNames->value[250 + iterator] = "M1_" + temp;
      this->VariableNames->value[300 + iterator] = "M2_" + temp;
      this->VariableNames->value[450 + iterator] = "DPMS_" + temp;
      this->VariableNames->value[850 + iterator] = "DPMS_DS_" + temp;
      this->VariableNames->value[1000 + iterator] = "MEAN_" + temp;
      this->VariableNames->value[1050 + iterator] = "RMS_" + temp;
      this->VariableNames->value[1250 + iterator] = "CREV_" + temp;

      iterator++;
    }
  }
}
