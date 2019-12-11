/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMReXGridReaderInternal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAMReXGridReaderInternal.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIndent.h"
#include "vtkObject.h"
#include "vtkSetGet.h"

#include <sstream>
#include <vector>
#include <vtksys/FStream.hxx>

namespace
{
std::string ReadFile(const std::string& filename)
{
  std::string contents;
  vtksys::ifstream stream(filename.c_str(), std::ios::binary);
  if (stream)
  {
    stream.seekg(0, std::ios::end);
    int flength = static_cast<int>(stream.tellg());
    stream.seekg(0, std::ios::beg);
    std::vector<char> data(flength + 1 + (flength + 1) % 8); // padded for better alignment.
    stream.read(data.data(), flength);
    data[flength] = '\0';
    contents = data.data();
  }
  return contents;
}
}

RealDescriptor::RealDescriptor() {}

RealDescriptor::RealDescriptor(const long* fr_, const int* ord_, int ordl_)
  : fr(fr_, fr_ + 8)
  , ord(ord_, ord_ + ordl_)
{
}

const long* RealDescriptor::format() const&
{
  return &fr[0];
}

const std::vector<long>& RealDescriptor::formatarray() const&
{
  return fr;
}

const int* RealDescriptor::order() const&
{
  return &ord[0];
}

const std::vector<int>& RealDescriptor::orderarray() const&
{
  return ord;
}

int RealDescriptor::numBytes() const
{
  return (fr[0] + 7) >> 3;
}

bool RealDescriptor::operator==(const RealDescriptor& rd) const
{
  return fr == rd.fr && ord == rd.ord;
}

#define AMREX_PRINT(os, indent, var) os << indent << #var << ": " << var << endl

vtkAMReXGridHeader::vtkAMReXGridHeader()
  : versionName()
  , variableNamesSize(0)
  , variableNames()
  , dim(0)
  , time(0)
  , finestLevel(0)
  , problemDomainLoEnd()
  , problemDomainHiEnd()
  , refinementRatio()
  , levelDomains()
  , levelSteps()
  , geometryCoord(0)
  , magicZero(0)
  , levelSize()
  , levelCells()
  , levelPrefix()
  , multiFabPrefix()
  , debugHeader(false)
{
}

void vtkAMReXGridHeader::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->PrintSelfGenericHeader(os, indent);
}

void vtkAMReXGridHeader::PrintSelfGenericHeader(ostream& os, vtkIndent indent)
{
  AMREX_PRINT(os, indent, versionName);
  AMREX_PRINT(os, indent, variableNamesSize);
  os << indent << "variableNames: " << endl;
  for (const auto& name : this->variableNames)
  {
    os << indent.GetNextIndent() << name << endl;
  }
  AMREX_PRINT(os, indent, dim);
  AMREX_PRINT(os, indent, time);
  AMREX_PRINT(os, indent, finestLevel);
  os << indent << "problemDomainLoEnd: " << endl << indent.GetNextIndent();
  for (const auto& lo : this->problemDomainLoEnd)
  {
    os << lo << " ";
  }
  os << endl;
  os << indent << "problemDomainHiEnd: " << endl << indent.GetNextIndent();
  for (const auto& hi : this->problemDomainHiEnd)
  {
    os << hi << " ";
  }
  os << endl;
  os << indent << "refinementRatio: " << endl << indent.GetNextIndent();
  for (const auto& ratio : this->refinementRatio)
  {
    os << ratio << " ";
  }
  os << endl;
  os << indent << "levelDomains: " << endl << indent.GetNextIndent();
  for (int level = 0; level < this->finestLevel + 1; ++level)
  {
    os << "(";
    os << "(";
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->levelDomains[level][0][space];
      if (space < (this->dim - 1))
        os << ",";
    }
    os << ") ";
    os << "(";
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->levelDomains[level][1][space];
      if (space < (this->dim - 1))
        os << ",";
    }
    os << ") ";
    os << "(";
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->levelDomains[level][2][space];
      if (space < (this->dim - 1))
        os << ",";
    }
    os << ")";
    if (level < this->finestLevel)
      os << ") ";
    else
      os << ")";
  }
  os << endl;
  os << indent << "levelSteps: " << endl << indent.GetNextIndent();
  for (const auto& steps : this->levelSteps)
  {
    os << steps << " ";
  }
  os << endl;
  os << indent << "cellSize: " << endl << indent.GetNextIndent();
  for (int level = 0; level < this->finestLevel + 1; ++level)
  {
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->cellSize[level][space];
    }
    if (level < this->finestLevel)
      os << endl << indent.GetNextIndent();
    else
      os << endl;
  }
  AMREX_PRINT(os, indent, geometryCoord);
  AMREX_PRINT(os, indent, magicZero);
  os << indent << "levelCells: " << endl << indent.GetNextIndent();
  for (int level = 0; level <= this->finestLevel; ++level)
  {
    os << level << " " << this->levelSize[level] << " " << this->time << endl
       << indent.GetNextIndent();
    os << this->levelSteps[level] << endl << indent.GetNextIndent();
    for (int size = 0; size < this->levelSize[level]; ++size)
    {
      for (int space = 0; space < this->dim; ++space)
      {
        for (int bound = 0; bound <= 1; ++bound)
        {
          os << this->levelCells[level][size][space][bound] << " ";
        }
        os << endl << indent.GetNextIndent();
      }
    }
    os << this->levelPrefix[level] << "/" << this->multiFabPrefix[level] << endl
       << indent.GetNextIndent();
  }
  os << "Generic Header Complete" << endl;
}

bool vtkAMReXGridHeader::Parse(const std::string& headerData)
{
  this->ParseGenericHeader(headerData);
  if (debugHeader)
  {
    std::ostream* os;
    os = &std::cout;
    vtkIndent indent;
    this->PrintSelf(*os, indent);
  }
  return true;
}

bool vtkAMReXGridHeader::ParseGenericHeader(const std::string& headerData)
{
  char c;
  std::istringstream hstream(headerData);
  hstream >> this->versionName;
  if (this->versionName.empty())
  {
    vtkGenericWarningMacro("Failed to read versionName string.");
    return false;
  }
  hstream >> this->variableNamesSize;
  this->variableNames.resize(this->variableNamesSize);
  for (int cc = 0; cc < this->variableNamesSize; ++cc)
  {
    hstream >> this->variableNames[cc];
  }
  hstream >> this->dim;
  if (this->dim != 1 && this->dim != 2 && this->dim != 3)
  {
    vtkGenericWarningMacro("dim must be 1, 2, or 3.");
    return false;
  }
  hstream >> this->time;
  hstream >> this->finestLevel;
  if (this->finestLevel < 0)
  {
    vtkGenericWarningMacro("finestLevel must be >= 0");
    return false;
  }
  this->problemDomainLoEnd.resize(this->dim);
  for (int cc = 0; cc < this->dim; ++cc)
  {
    hstream >> this->problemDomainLoEnd[cc];
  }
  this->problemDomainHiEnd.resize(this->dim);
  for (int cc = 0; cc < this->dim; ++cc)
  {
    hstream >> this->problemDomainHiEnd[cc];
  }
  this->refinementRatio.resize(this->finestLevel);
  for (int cc = 0; cc < this->finestLevel; ++cc)
  {
    hstream >> this->refinementRatio[cc];
  }
  this->levelDomains.resize(this->finestLevel + 1);
  for (int cc = 0; cc <= this->finestLevel; ++cc)
  {
    this->levelDomains[cc].resize(3);
    for (int dd = 0; dd < 3; ++dd)
    {
      this->levelDomains[cc][dd].resize(this->dim);
    }
  }
  for (int cc = 0; cc <= this->finestLevel; ++cc)
  {
    hstream >> c; // read '('
    for (int dd = 0; dd < 3; ++dd)
    {
      hstream >> c; // read '('
      for (int ee = 0; ee < this->dim; ++ee)
      {
        hstream >> this->levelDomains[cc][dd][ee];
        if (ee < (this->dim - 1))
        {
          hstream >> c; // read ','
        }
        else
        {
          hstream >> c; // read ')'
        }
      }
    }
    hstream >> c; // read ')'
  }
  this->levelSteps.resize(this->finestLevel + 1);
  for (int cc = 0; cc < this->finestLevel + 1; ++cc)
  {
    hstream >> this->levelSteps[cc];
  }
  this->cellSize.resize(this->finestLevel + 1);
  for (int cc = 0; cc <= this->finestLevel; ++cc)
  {
    this->cellSize[cc].resize(this->dim);
  }
  for (int cc = 0; cc <= this->finestLevel; ++cc)
  {
    for (int dd = 0; dd < this->dim; ++dd)
    {
      hstream >> this->cellSize[cc][dd];
    }
  }
  hstream >> this->geometryCoord;
  hstream >> this->magicZero;
  int tmpLevel;
  this->levelSize.resize(this->finestLevel + 1);
  double tmpTime;
  int tmpLevelSteps;
  this->levelCells.resize(this->finestLevel + 1);
  std::string pathName;
  std::string deliminator = "/";
  this->levelPrefix.resize(this->finestLevel + 1);
  this->multiFabPrefix.resize(this->finestLevel + 1);
  for (int level = 0; level <= this->finestLevel; ++level)
  {
    hstream >> tmpLevel;
    hstream >> this->levelSize[level];
    hstream >> tmpTime;
    hstream >> tmpLevelSteps;
    this->levelCells[level].resize(this->levelSize[level]);
    for (int size = 0; size < this->levelSize[level]; ++size)
    {
      this->levelCells[level][size].resize(this->dim);
      for (int space = 0; space < this->dim; ++space)
      {
        this->levelCells[level][size][space].resize(2);
      }
    }
    for (int size = 0; size < this->levelSize[level]; ++size)
    {
      for (int space = 0; space < this->dim; ++space)
      {
        for (int bound = 0; bound <= 1; ++bound)
        {
          hstream >> this->levelCells[level][size][space][bound];
        }
      }
    }
    hstream >> pathName;
    size_t pos = 0;
    pos = pathName.find(deliminator);
    this->levelPrefix[level] = pathName.substr(0, pos);
    pathName.erase(0, pos + deliminator.length());
    this->multiFabPrefix[level] = pathName.substr(0, pathName.length());
  }
  return true;
}

vtkAMReXGridLevelHeader::vtkAMReXGridLevelHeader()
  : level(0)
  , dim(0)
  , levelVersion(0)
  , levelHow(0)
  , levelNumberOfComponents(0)
  , levelNumberOfGhostCells(0)
  , levelBoxArraySize(0)
  , levelMagicZero(0)
  , levelBoxArrays()
  , levelNumberOfFABOnDisk(0)
  , levelFabOnDiskPrefix()
  , levelFABFile()
  , levelFileOffset()
  , levelMinimumsFAB()
  , levelMaximumsFAB()
  , levelFABArrayMinimum()
  , levelFABArrayMaximum()
  , levelRealNumberOfBytes(0)
  , levelRealOrder(0)
  , debugLevelHeader(false)
{
}

void vtkAMReXGridLevelHeader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->PrintSelfLevelHeader(os, indent);
}

void vtkAMReXGridLevelHeader::PrintSelfLevelHeader(ostream& os, vtkIndent indent)
{
  AMREX_PRINT(os, indent, level);
  AMREX_PRINT(os, indent, levelVersion);
  AMREX_PRINT(os, indent, levelHow);
  AMREX_PRINT(os, indent, levelNumberOfComponents);
  AMREX_PRINT(os, indent, levelNumberOfGhostCells);
  os << "BoxArray Size and MagicZero:" << endl << indent.GetNextIndent();
  os << "(" << this->levelBoxArraySize << " " << this->levelMagicZero << endl
     << indent.GetNextIndent();
  os << indent << "levelDomains: " << endl << indent.GetNextIndent();
  for (int cc = 0; cc < this->levelBoxArraySize; ++cc)
  {
    os << "(";
    os << "(";
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->levelBoxArrays[cc][0][space];
      if (space < (this->dim - 1))
        os << ",";
    }
    os << ") ";
    os << "(";
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->levelBoxArrays[cc][1][space];
      if (space < (this->dim - 1))
        os << ",";
    }
    os << ") ";
    os << "(";
    for (int space = 0; space < this->dim; ++space)
    {
      os << this->levelBoxArrays[cc][2][space];
      if (space < (this->dim - 1))
        os << ",";
    }
    os << ")";
    if (cc < (this->levelBoxArraySize - 1))
      os << ")" << endl << indent.GetNextIndent();
    else
      os << ")" << endl;
  }
  os << ")" << endl;
  AMREX_PRINT(os, indent, levelNumberOfFABOnDisk);
  os << indent << "FABsOnDisk: " << endl << indent.GetNextIndent();
  for (int cc = 0; cc < this->levelNumberOfFABOnDisk; ++cc)
  {
    os << this->levelFabOnDiskPrefix << ' ' << this->levelFABFile[cc] << ' '
       << this->levelFileOffset[cc] << endl
       << indent.GetNextIndent();
  }
  os << endl;
  if (this->levelVersion == Version_v1 || this->levelVersion == NoFabHeaderMinMax_v1)
  {
    std::ios::fmtflags oflags = os.flags();
    os.setf(std::ios::floatfield, std::ios::scientific);
    int oldPrec(os.precision(16));
    os << indent << "Minimums and Maximums of FABs: " << endl << indent.GetNextIndent();
    os << this->levelNumberOfFABOnDisk << "," << this->levelNumberOfComponents << endl
       << indent.GetNextIndent();
    for (int cc = 0; cc < this->levelNumberOfFABOnDisk; ++cc)
    {
      for (int dd = 0; dd < this->levelNumberOfComponents; ++dd)
      {
        os << this->levelMinimumsFAB[cc][dd] << "," << endl << indent.GetNextIndent();
      }
    }
    os << endl << indent.GetNextIndent();
    os << this->levelNumberOfFABOnDisk << "," << this->levelNumberOfComponents << endl
       << indent.GetNextIndent();
    for (int cc = 0; cc < this->levelNumberOfFABOnDisk; ++cc)
    {
      for (int dd = 0; dd < this->levelNumberOfComponents; ++dd)
      {
        os << this->levelMaximumsFAB[cc][dd];
        if (cc < (this->levelNumberOfFABOnDisk - 1))
          os << "," << endl << indent.GetNextIndent();
        else
          os << endl << indent.GetNextIndent();
      }
    }
    os << endl << indent.GetNextIndent();
    os.flags(oflags);
    os.precision(oldPrec);
  }
  if (this->levelVersion == NoFabHeaderFAMinMax_v1)
  {
    os << indent << "Minimums and Maximums of FABArray: " << endl << indent.GetNextIndent();
    for (int cc = 0; cc < this->levelNumberOfComponents; ++cc)
    {
      os << this->levelFABArrayMinimum[cc] << ",";
    }
    os << endl << indent.GetNextIndent();
    for (int cc = 0; cc < this->levelNumberOfComponents; ++cc)
    {
      os << this->levelFABArrayMaximum[cc] << ",";
    }
    os << endl << indent.GetNextIndent();
  }
  if (this->levelVersion == NoFabHeader_v1 || this->levelVersion == NoFabHeaderMinMax_v1 ||
    this->levelVersion == NoFabHeaderFAMinMax_v1)
  {
    os << indent << "Real Format: " << endl << indent.GetNextIndent();
    os << "(" << this->levelRealNumberOfBytes << "," << this->levelRealOrder << ")" << endl
       << indent.GetNextIndent();
  }
  os << "Level " << this->level << " Header Complete" << endl;
}

bool vtkAMReXGridLevelHeader::Parse(int _level, int _dim, const std::string& headerData)
{
  this->ParseLevelHeader(_level, _dim, headerData);
  if (debugLevelHeader)
  {
    std::ostream* os;
    os = &std::cout;
    vtkIndent indent;
    this->PrintSelf(*os, indent);
  }
  return true;
}

bool vtkAMReXGridLevelHeader::ParseLevelHeader(int _level, int _dim, const std::string& headerData)
{
  std::istringstream hstream(headerData);
  this->level = _level;
  this->dim = _dim;
  hstream >> this->levelVersion;
  hstream >> this->levelHow;
  hstream >> this->levelNumberOfComponents;
  hstream >> this->levelNumberOfGhostCells;
  char c;
  hstream >> c; // read '(' Begin BoxArray writeon()
  hstream >> this->levelBoxArraySize;
  hstream >> this->levelMagicZero;
  this->levelBoxArrays.resize(this->levelBoxArraySize);
  for (int cc = 0; cc < this->levelBoxArraySize; ++cc)
  {
    this->levelBoxArrays[cc].resize(3);
    for (int dd = 0; dd < 3; ++dd)
    {
      this->levelBoxArrays[cc][dd].resize(this->dim);
    }
  }
  for (int cc = 0; cc < this->levelBoxArraySize; ++cc)
  {
    hstream >> c; // read '('
    for (int dd = 0; dd < 3; ++dd)
    {
      hstream >> c; // read '('
      for (int ee = 0; ee < this->dim; ++ee)
      {
        hstream >> this->levelBoxArrays[cc][dd][ee];
        if (ee < (this->dim - 1))
        {
          hstream >> c; // read ','
        }
        else
        {
          hstream >> c; // read ')'
        }
      }
    }
    hstream >> c; // read ')'
  }
  hstream >> c; // read ')' End BoxArray writeon()
  hstream >> this->levelNumberOfFABOnDisk;
  this->levelFABFile.resize(this->levelNumberOfFABOnDisk);
  this->levelFileOffset.resize(this->levelNumberOfFABOnDisk);
  for (int cc = 0; cc < this->levelNumberOfFABOnDisk; ++cc)
  {
    hstream >> this->levelFabOnDiskPrefix; // Prefix
    hstream >> this->levelFABFile[cc];     // File
    hstream >> this->levelFileOffset[cc];  // Offset
  }
  if (this->levelVersion == Version_v1 || this->levelVersion == NoFabHeaderMinMax_v1)
  {
    int tmpLevelNumberOfFABOnDisk;
    int tmpLevelNumberOfComponents;
    hstream >> tmpLevelNumberOfFABOnDisk;
    hstream >> c; // read ','
    hstream >> tmpLevelNumberOfComponents;
    this->levelMinimumsFAB.resize(this->levelNumberOfFABOnDisk);
    for (int cc = 0; cc < this->levelNumberOfFABOnDisk; ++cc)
    {
      this->levelMinimumsFAB[cc].resize(this->levelNumberOfComponents);
      for (int dd = 0; dd < this->levelNumberOfComponents; ++dd)
      {
        hstream >> this->levelMinimumsFAB[cc][dd];
        hstream >> c;
      }
    }
    hstream >> tmpLevelNumberOfFABOnDisk;
    hstream >> c; // read ','
    hstream >> tmpLevelNumberOfComponents;
    this->levelMaximumsFAB.resize(this->levelNumberOfFABOnDisk);
    for (int cc = 0; cc < this->levelNumberOfFABOnDisk; ++cc)
    {
      this->levelMaximumsFAB[cc].resize(this->levelNumberOfComponents);
      for (int dd = 0; dd < this->levelNumberOfComponents; ++dd)
      {
        hstream >> this->levelMaximumsFAB[cc][dd];
        hstream >> c; // read ','
      }
    }
  }
  if (this->levelVersion == NoFabHeaderFAMinMax_v1)
  {
    this->levelFABArrayMinimum.resize(this->levelNumberOfComponents);
    for (int cc = 0; cc < this->levelNumberOfComponents; ++cc)
    {
      hstream >> this->levelFABArrayMinimum[cc];
      hstream >> c; // read ','
    }
    this->levelFABArrayMaximum.resize(this->levelNumberOfComponents);
    for (int cc = 0; cc < this->levelNumberOfComponents; ++cc)
    {
      hstream >> this->levelFABArrayMaximum[cc];
      hstream >> c; // read ','
    }
  }
  if (this->levelVersion == NoFabHeader_v1 || this->levelVersion == NoFabHeaderMinMax_v1 ||
    this->levelVersion == NoFabHeaderFAMinMax_v1)
  {
    hstream >> c; // read '('
    hstream >> this->levelRealNumberOfBytes;
    hstream >> c; // read ','
    hstream >> this->levelRealOrder;
    hstream >> c; // read ')'
  }
  return true;
}

vtkAMReXGridReaderInternal::vtkAMReXGridReaderInternal()
  : FileName()
  , LevelHeader()
{
  this->headersAreRead = false;
  this->debugReader = false;
  this->Header = nullptr;
}

vtkAMReXGridReaderInternal::~vtkAMReXGridReaderInternal()
{
  this->DestroyHeader();
  this->DestroyLevelHeader();
}

void vtkAMReXGridReaderInternal::DestroyHeader()
{
  delete this->Header;
  this->Header = nullptr;
}

void vtkAMReXGridReaderInternal::DestroyLevelHeader()
{
  for (unsigned int i = 0; i < this->LevelHeader.size(); ++i)
  {
    delete this->LevelHeader[i];
    this->LevelHeader[i] = nullptr;
  }
}

void vtkAMReXGridReaderInternal::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << this->FileName << endl;
  if (this->Header)
  {
    os << indent << "Header: " << endl;
    this->Header->PrintSelf(os, indent.GetNextIndent());
    os << indent << "LevelHeader(s): " << endl;
    for (int cc = 0; cc < this->Header->finestLevel + 1; ++cc)
    {
      this->LevelHeader[cc]->PrintSelfLevelHeader(os, indent.GetNextIndent());
    }
  }
  else
  {
    os << indent << "Header: nullptr" << endl;
  }
}

void vtkAMReXGridReaderInternal::SetFileName(char* fName)
{
  const std::string filename(fName == nullptr ? "" : fName);
  this->FileName = filename;
  this->headersAreRead = false;
}

//-----------------------------------------------------------------------------
void vtkAMReXGridReaderInternal::ReadMetaData()
{
  if (!this->headersAreRead)
  {
    if (!this->FileName.empty())
    {
      if (this->ReadHeader())
      {
        this->headersAreRead = this->ReadLevelHeader();
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkAMReXGridReaderInternal::ReadHeader()
{
  this->DestroyHeader();

  const std::string headerFileName = this->FileName + "/Header";
  const auto headerData = ::ReadFile(headerFileName);
  if (headerData.empty())
  {
    return false;
  }

  auto headerPtr = new vtkAMReXGridHeader();
  if (!headerPtr->Parse(headerData))
  {
    delete headerPtr;
    return false;
  }

  this->Header = headerPtr;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAMReXGridReaderInternal::ReadLevelHeader()
{
  this->DestroyLevelHeader();

  this->LevelHeader.resize(this->Header->finestLevel + 1);

  for (int level = 0; level <= this->Header->finestLevel; ++level)
  {
    const std::string levelHeaderFileName = this->FileName + "/" +
      this->Header->levelPrefix[level] + "/" + this->Header->multiFabPrefix[level] + "_H";

    const auto headerData = ::ReadFile(levelHeaderFileName);
    if (headerData.empty())
    {
      return false;
    }

    auto headerPtr = new vtkAMReXGridLevelHeader();
    if (!headerPtr->Parse(level, this->Header->dim, headerData))
    {
      delete headerPtr;
      return false;
    }

    this->LevelHeader[level] = headerPtr;
  }
  return true;
}

int vtkAMReXGridReaderInternal::GetNumberOfLevels()
{
  return this->headersAreRead ? this->Header->finestLevel : -1;
}

int vtkAMReXGridReaderInternal::GetBlockLevel(const int blockIdx)
{
  if (this->headersAreRead)
  {
    int numberOfLevels = this->GetNumberOfLevels() + 1;
    int levelBlocksLo = 0;
    int levelBlocksHi = 0;
    for (int cc = 0; cc < numberOfLevels; ++cc)
    {
      levelBlocksHi += this->LevelHeader[cc]->levelBoxArraySize;
      if (blockIdx >= levelBlocksLo && blockIdx < levelBlocksHi)
      {
        return cc;
      }
      levelBlocksLo = levelBlocksHi;
    }
  }
  return (-1);
}

int vtkAMReXGridReaderInternal::GetNumberOfBlocks()
{
  if (this->headersAreRead)
  {
    int numberOfLevels = this->GetNumberOfLevels() + 1;
    int numberOfBlocks = 0;
    for (int i = 0; i < numberOfLevels; ++i)
    {
      numberOfBlocks += this->Header->levelSize[i];
    }
    return numberOfBlocks;
  }
  return (-1);
}

int vtkAMReXGridReaderInternal::GetBlockIndexWithinLevel(int blockIdx, int level)
{
  if (this->headersAreRead)
  {
    int blockIndexWithinLevel = blockIdx;
    for (int i = 0; i < level; ++i)
    {
      blockIndexWithinLevel -= this->Header->levelSize[i];
    }
    return blockIndexWithinLevel;
  }
  return (-1);
}

void vtkAMReXGridReaderInternal::GetBlockAttribute(
  const char* attribute, int blockIdx, vtkDataSet* pDataSet)
{
  if (this->headersAreRead)
  {
    if (attribute == nullptr || blockIdx < 0 || pDataSet == nullptr ||
      blockIdx >= this->GetNumberOfBlocks())
    {
      return;
    }

    //
    // orders.
    //
    // int big_float_order[] = { 1, 2, 3, 4 };
    int little_float_order[] = { 4, 3, 2, 1 };
    // int mid_float_order_2[] = { 2, 1, 4, 3 };
    // int big_double_order[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int little_double_order[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    // int mid_double_order_2[] = { 2, 1, 4, 3, 6, 5, 8, 7 };
    //
    // formats.
    //
    long ieee_float[] = { 32L, 8L, 23L, 0L, 1L, 9L, 0L, 0x7FL };
    long ieee_double[] = { 64L, 11L, 52L, 0L, 1L, 12L, 0L, 0x3FFL };

    int offsetOfAttribute = this->GetOffsetOfAttribute(attribute);
    int theLevel = this->GetBlockLevel(blockIdx);
    int blockIdxWithinLevel = this->GetBlockIndexWithinLevel(blockIdx, theLevel);
    if (debugReader)
      std::cout << "blockIdx " << blockIdx << " attribute " << attribute << " offset of attribute "
                << offsetOfAttribute << " Level " << theLevel << " blockIdx within Level "
                << blockIdxWithinLevel << std::endl;
    const std::string FABFileName = this->FileName + "/" + this->Header->levelPrefix[theLevel] +
      "/" + this->LevelHeader[theLevel]->levelFABFile[blockIdxWithinLevel];
    if (debugReader)
      std::cout << "FABFile " << FABFileName << " Offset "
                << this->LevelHeader[theLevel]->levelFileOffset[blockIdxWithinLevel] << std::endl;
    std::filebuf fb;
    if (fb.open(FABFileName, std::ios::binary | std::ios::in))
    {
      std::istream is(&fb);
      is.seekg(this->LevelHeader[theLevel]->levelFileOffset[blockIdxWithinLevel]);
      //
      // Read FAB Header
      //
      this->ReadFAB(is);
      // int version =
      this->ReadVersion(is);
      int dimension = this->Header->dim;
      RealDescriptor* ird = this->ReadRealDescriptor(is);
      std::vector<int> boxArray(3 * dimension);
      std::vector<int> boxArrayDim(dimension);
      int numberOfPoints = ReadBoxArray(is, boxArray.data(), boxArrayDim.data());
      // int numberOfAttributes =
      this->ReadNumberOfAttributes(is);

      //
      // Skip the Line Feed (linefeed+1)
      // Jump to the desired attribute (offsetOfAttribute*(numberOfPoints*ird->numBytes()))
      // - Patrick O'Leary
      //
      int linefeed = is.tellg();
      is.seekg((linefeed + 1) + (offsetOfAttribute * (numberOfPoints * ird->numBytes())));

      if (debugReader)
      {
        for (int i = 0; i < dimension; ++i)
          std::cout << boxArrayDim[i] << " ";
        std::cout << std::endl;
      }

      if (ird->numBytes() == 4)
      {
        vtkNew<vtkFloatArray> dataArray;
        dataArray->SetName(attribute);
        dataArray->SetNumberOfTuples(numberOfPoints);
        float* arrayPtr = static_cast<float*>(dataArray->GetPointer(0));
        std::vector<char> buffer(numberOfPoints * ird->numBytes());
        this->ReadBlockAttribute(is, numberOfPoints, ird->numBytes(), buffer.data());
        RealDescriptor* ord =
          new RealDescriptor(ieee_float, little_float_order, 4); // we desire ieee little endian
        this->Convert(arrayPtr, buffer.data(), numberOfPoints, *ord, *ird);
        pDataSet->GetCellData()->AddArray(dataArray);
        delete ird;
        delete ord;
      }
      else
      {
        vtkNew<vtkDoubleArray> dataArray;
        dataArray->SetName(attribute);
        dataArray->SetNumberOfTuples(numberOfPoints);
        double* arrayPtr = static_cast<double*>(dataArray->GetPointer(0));
        std::vector<char> buffer(numberOfPoints * ird->numBytes());
        this->ReadBlockAttribute(is, numberOfPoints, ird->numBytes(), buffer.data());
        RealDescriptor* ord =
          new RealDescriptor(ieee_double, little_double_order, 8); // we desire ieee little endian
        this->Convert(arrayPtr, buffer.data(), numberOfPoints, *ord, *ird);
        pDataSet->GetCellData()->AddArray(dataArray);
        arrayPtr = nullptr;
        delete ird;
        delete ord;
      }
      if (debugReader)
      {
        std::cout << is.tellg() << " "
                  << this->LevelHeader[theLevel]->levelFileOffset[blockIdxWithinLevel] << " "
                  << numberOfPoints << std::endl;
      }
      fb.close();
    }
  }
}

int vtkAMReXGridReaderInternal::GetOffsetOfAttribute(const char* attribute)
{
  int i = 0, position = 0;
  bool found = false;

  while (i < this->Header->variableNamesSize && !found)
  {
    if (strcmp(this->Header->variableNames[i].c_str(), attribute) == 0)
    {
      found = true;
      position = i;
    }
    ++i;
  }
  if (found)
  {
    return position;
  }
  else
    return (-1);
}

void vtkAMReXGridReaderInternal::ReadFAB(std::istream& is)
{
  char f, a, b;
  is >> f;
  is >> a;
  is >> b;
  if (debugReader)
    std::cout << f << a << b;
}

int vtkAMReXGridReaderInternal::ReadVersion(std::istream& is)
{
  char colon;
  is >> colon;
  if (colon == ':')
  {
    if (debugReader)
      std::cout << colon << "!" << std::endl;

    return 0;
  }
  else
  {
    is.putback(colon);
    if (debugReader)
      std::cout << " ";
    return 1;
  }
}

void vtkAMReXGridReaderInternal::ReadOrder(std::istream& is, std::vector<int>& ar)
{
  char c;
  is >> c; // '('
  int size;
  is >> size;
  is >> c; // ','
  is >> c; // '('
  ar.resize(size);
  for (int i = 0; i < size; ++i)
    is >> ar[i];
  is >> c; // ')'
  is >> c; // ')'
}

void vtkAMReXGridReaderInternal::PrintOrder(std::vector<int>& ar)
{
  size_t size = ar.size();
  std::cout << "(" << size << ", (";
  for (size_t i = 0; i < size; ++i)
  {
    std::cout << ar[i];
    if (i < size - 1)
      std::cout << " ";
  }
  std::cout << "))";
}

void vtkAMReXGridReaderInternal::ReadFormat(std::istream& is, std::vector<long>& ar)
{
  char c;
  is >> c; // '('
  int size;
  is >> size;
  is >> c; // ','
  is >> c; // '('
  ar.resize(size);
  for (int i = 0; i < size; ++i)
    is >> ar[i];
  is >> c; // ')'
  is >> c; // ')'
}

void vtkAMReXGridReaderInternal::PrintFormat(std::vector<long>& ar)
{
  size_t size = ar.size();
  std::cout << "(" << size << ", (";
  for (size_t i = 0; i < size; ++i)
  {
    std::cout << ar[i];
    if (i < size - 1)
      std::cout << " ";
  }
  std::cout << "))";
}

RealDescriptor* vtkAMReXGridReaderInternal::ReadRealDescriptor(std::istream& is)
{
  std::vector<long> fmt;
  std::vector<int> ord;
  char c;
  is >> c; // '('
  if (debugReader)
    std::cout << c;
  this->ReadFormat(is, fmt);
  if (debugReader)
    this->PrintFormat(fmt);
  is >> c; // ','
  if (debugReader)
    std::cout << c;
  this->ReadOrder(is, ord);
  if (debugReader)
    this->PrintOrder(ord);
  is >> c; // ')'
  if (debugReader)
    std::cout << c;
  //
  // ord.size() is either 4 or 8 for float or double respectively - cast to int is safe
  //
  return new RealDescriptor(&fmt[0], &ord[0], static_cast<int>(ord.size()));
}

int vtkAMReXGridReaderInternal::ReadBoxArray(std::istream& is, int* boxArray, int* boxArrayDim)
{
  char c;
  is >> c; // read '('
  for (int dd = 0; dd < 3; ++dd)
  {
    is >> c; // read '('
    for (int ee = 0; ee < this->Header->dim; ++ee)
    {
      is >> boxArray[this->Header->dim * dd + ee];
      if (ee < (this->Header->dim - 1))
      {
        is >> c; // read ','
      }
      else
      {
        is >> c; // read ')'
      }
    }
  }
  is >> c; // read ')'

  //
  // block dimension - '(hi - lo + 1)' is the number of cells '+ 1' is the number of points
  //
  int numberOfPoints = 1;
  for (int i = 0; i < this->Header->dim; ++i)
  {
    boxArrayDim[i] =
      ((boxArray[this->Header->dim * 1 + i] - boxArray[this->Header->dim * 0 + i]) + 1);
    numberOfPoints *= boxArrayDim[i];
  }
  if (debugReader)
    this->PrintBoxArray(boxArray);
  return numberOfPoints;
}

void vtkAMReXGridReaderInternal::PrintBoxArray(int* boxArray)
{
  std::cout << "(";
  std::cout << "(";
  for (int space = 0; space < this->Header->dim; ++space)
  {
    std::cout << boxArray[0 * this->Header->dim + space];
    if (space < (this->Header->dim - 1))
      std::cout << ",";
  }
  std::cout << ") ";
  std::cout << "(";
  for (int space = 0; space < this->Header->dim; ++space)
  {
    std::cout << boxArray[1 * this->Header->dim + space];
    if (space < (this->Header->dim - 1))
      std::cout << ",";
  }
  std::cout << ") ";
  std::cout << "(";
  for (int space = 0; space < this->Header->dim; ++space)
  {
    std::cout << boxArray[2 * this->Header->dim + space];
    if (space < (this->Header->dim - 1))
      std::cout << ",";
  }
  std::cout << ")";
  std::cout << ")";
}

int vtkAMReXGridReaderInternal::ReadNumberOfAttributes(std::istream& is)
{
  int numberOfAttributes;
  is >> numberOfAttributes;
  if (debugReader)
    std::cout << " " << numberOfAttributes << std::endl;
  return numberOfAttributes;
}

void vtkAMReXGridReaderInternal::ReadBlockAttribute(
  std::istream& is, int numberOfPoints, int size, char* buffer)
{
  is.read(buffer, numberOfPoints * size);
}

void vtkAMReXGridReaderInternal::Convert(
  void* out, const void* in, long nitems, const RealDescriptor& ord, const RealDescriptor& ird)
{
  if (ord == ird)
  {
    size_t n = size_t(nitems);
    memcpy(out, in, n * ord.numBytes());
  }
  else if (ord.formatarray() == ird.formatarray())
  {
    this->PermuteOrder(out, in, nitems, ord.order(), ird.order(), ord.numBytes());
  }
  else
  {
    //
    // We don't handle changing real formats
    //
  }
}

void vtkAMReXGridReaderInternal::PermuteOrder(
  void* out, const void* in, long nitems, const int* outord, const int* inord, int REALSIZE)
{
  char const* pin = static_cast<char const*>(in);
  char* pout = static_cast<char*>(out);

  pin--;
  pout--;

  for (; nitems > 0; nitems--, pin += REALSIZE, pout += REALSIZE)
  {
    for (int i = 0; i < REALSIZE; i++)
      pout[outord[i]] = pin[inord[i]];
  }
}
