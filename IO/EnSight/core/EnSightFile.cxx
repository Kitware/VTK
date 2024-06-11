// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "EnSightFile.h"

#include <regex>

#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/core.h)
// clang-format on

namespace ensight_gold
{
VTK_ABI_NAMESPACE_BEGIN

template <>
int getNumChars<int>()
{
  return 10;
}

template <>
int getNumChars<float>()
{
  return 12;
}

template <>
bool stringTo(const std::string& input, std::string& output)
{
  output = input;
  return true;
}

template <>
bool stringTo(const std::string& input, int& output)
{
  try
  {
    output = std::stoi(input);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

template <>
bool stringTo(const std::string& input, float& output)
{
  try
  {
    output = std::stof(input);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

template <>
bool stringTo(const std::string& input, double& output)
{
  try
  {
    output = std::stod(input);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

int getFileNameNumberIndex(double actualTimeValue, std::shared_ptr<TimeSetInfo> info)
{
  int idx = 0;

  double timeVal = info->TimeValues[0];
  for (size_t i = 1; i < info->TimeValues.size(); ++i)
  {
    double newTime = info->TimeValues[i];
    if (newTime <= actualTimeValue && newTime > timeVal)
    {
      timeVal = newTime;
      idx++;
    }
  }
  return idx;
}

int getFileSetIndex(int tsIdx, std::shared_ptr<FileSetInfo> info)
{
  int currentSum = 0;
  for (size_t i = 0; i < info->NumberOfSteps.size(); i++)
  {
    currentSum += info->NumberOfSteps[i];
    if (tsIdx < currentSum)
    {
      return info->FileNameIndex[i];
    }
  }
  return -1;
}

std::string replaceWildcards(const std::string& pattern, int num)
{
  std::regex exp("\\*+");
  std::smatch sm;
  std::string filename = pattern;
  if (std::regex_search(pattern, sm, exp))
  {
    auto numWildcards = sm.length(0);
    filename = fmt::format(
      "{}{:0{}d}{}", std::string(sm.prefix()), num, numWildcards, std::string(sm.suffix()));
  }

  return filename;
}

//------------------------------------------------------------------------------
EnSightFile::EnSightFile()
{
  this->Stream = nullptr;
}

//------------------------------------------------------------------------------
EnSightFile::~EnSightFile()
{
  if (this->Stream)
  {
    this->CloseFile();
  }
}

//------------------------------------------------------------------------------
bool EnSightFile::SetFileNamePattern(const std::string& filename, bool isCaseFile /* = false*/)
{
  this->FileNamePattern = filename;
  if (isCaseFile)
  {
    return this->OpenFile(filename, true);
  }
  return true;
}

//------------------------------------------------------------------------------
void EnSightFile::SetTimeAndFileSetInfo(int timeSet, int fileSet)
{
  this->TimeSet = timeSet;
  this->FileSet = fileSet;
}

//------------------------------------------------------------------------------
void EnSightFile::SetTimeSetInfo(std::shared_ptr<TimeSetInfo> info)
{
  this->TimeInfo = info;
}

//------------------------------------------------------------------------------
void EnSightFile::SetFileSetInfo(std::shared_ptr<FileSetInfo> info)
{
  this->FileInfo = info;
  if (!this->FileInfo->FileNameIndex.empty())
  {
    if (this->FileInfo->NumberOfSteps.size() != this->FileInfo->FileNameIndex.size())
    {
      vtkGenericWarningMacro("For a file set, the number of steps and the number of file name "
                             "indices should be the same");
      return;
    }
    for (size_t i = 0; i < this->FileInfo->NumberOfSteps.size(); i++)
    {
      for (int j = 0; j < this->FileInfo->NumberOfSteps[i]; j++)
      {
        this->FileInfo->TimeStepIndexInFile.push_back(j);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool EnSightFile::SetTimeStepToRead(double ts)
{
  if ((this->FileSet == -1 && this->TimeSet == -1) || !this->TimeInfo)
  {
    // non transient data
    return this->OpenFile(this->FileNamePattern);
  }

  if (this->TimeInfo->TimeValues.empty())
  {
    vtkGenericWarningMacro(
      "Time sets are used, but some error has caused the TimeValues to be empty");
    return false;
  }

  auto tsIdx = getFileNameNumberIndex(ts, this->TimeInfo);
  if (this->TimeSet != -1 && this->FileSet == -1)
  {
    // only using time sets
    if (tsIdx != this->TimeStepIndex)
    {
      this->TimeStepIndex = tsIdx;
      this->CloseFile();
      std::string filename = this->FileNamePattern;

      // check if the filename contains wildcards
      std::regex exp("\\*+");
      std::smatch sm;
      if (std::regex_search(this->FileNamePattern, sm, exp))
      {
        if (!this->TimeInfo)
        {
          vtkGenericWarningMacro("TimeSet is " << this->TimeSet << ", but TimeInfo was not set");
          return false;
        }
        filename = replaceWildcards(filename, this->TimeInfo->FileNameNumbers[this->TimeStepIndex]);
      }
      if (!this->OpenFile(filename))
      {
        vtkGenericWarningMacro("the file " << filename << " could not be opened");
        return false;
      }
    }
    this->ResetFile();
  }
  else if (this->TimeSet != -1 && this->FileSet != -1)
  {
    std::streampos posToRead;
    int fileIndex = -1;
    int tsIndexForFile = tsIdx;
    if (!this->FileInfo->FileNameIndex.empty())
    {
      // in this case we may have to switch files, depending on what
      // time step we're on
      // otherwise we just stay in the file we already have opened
      fileIndex = getFileSetIndex(tsIdx, this->FileInfo);
      tsIndexForFile = this->FileInfo->TimeStepIndexInFile[tsIdx];
      this->CurrentFileIndex = fileIndex;
    }
    std::string filename = this->FileNamePattern;
    if (fileIndex != -1)
    {
      filename = replaceWildcards(filename, fileIndex);
    }

    if (!this->OpenFile(filename))
    {
      vtkGenericWarningMacro("the file " << filename << " could not be opened");
      return false;
    }

    auto& beginPositions = this->TimeStepBeginPositions[fileIndex];
    if (static_cast<size_t>(tsIndexForFile) < beginPositions.size())
    {
      posToRead = beginPositions[tsIndexForFile];
    }
    else
    {
      // so regardless of if beginPositions is empty or not, we need to add positions to the vector
      // to get the the ts index we need
      std::streampos priorPos = beginPositions.empty() ? std::streampos(0) : beginPositions.back();
      this->MoveToPosition(priorPos);
      int i = static_cast<int>(beginPositions.size());
      auto result = this->ReadNextLine();
      while (i <= tsIndexForFile && result.first)
      {
        if (result.second.find("BEGIN TIME STEP") != std::string::npos)
        {
          beginPositions.push_back(this->GetCurrentPosition());
          i++;
        }
        result = this->ReadNextLine();
      }
      posToRead = beginPositions[tsIndexForFile];
    }
    this->MoveToPosition(posToRead);
  }
  else
  {
    vtkGenericWarningMacro("Time sets aren't being used, but file sets are, which is invalid");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void EnSightFile::CheckForBeginTimeStepLine()
{
  // When file sets are used, multiple time steps are in a single file.
  // each time step is between lines saying BEGIN TIME STEP and END TIME STEP
  if (this->FileSet == -1)
  {
    return;
  }

  auto result = this->ReadNextLine();
  if (result.second.find("BEGIN TIME STEP") == std::string::npos)
  {
    // This isn't an error situation. We track positions of time steps starting just
    // after BEGIN TIME STEP. So if the line doesn't end up containing that, reset back
    // to the previous line so we don't mess up processing.
    this->GoBackOneLine();
    return;
  }

  // Adding positions to TimeStepBeginPositions should always happen in SetTimeStepToRead,
  // but just incase it doesn't, we can add it here.
  auto& beginPositions = this->TimeStepBeginPositions[this->CurrentFileIndex];
  if (std::find(beginPositions.begin(), beginPositions.end(), this->GetCurrentPosition()) ==
    beginPositions.end())
  {
    beginPositions.push_back(this->GetCurrentPosition());
  }
}

//------------------------------------------------------------------------------
bool EnSightFile::CheckForEndTimeStepLine()
{
  // When file sets are used, multiple time steps are in a single file.
  // each time step is between lines saying END TIME STEP and END TIME STEP
  if (this->FileSet == -1)
  {
    return false;
  }

  auto result = this->ReadNextLine();
  if (result.second.find("END TIME STEP") != std::string::npos)
  {
    return true;
  }
  this->GoBackOneLine();
  return false;
}

//------------------------------------------------------------------------------
void EnSightFile::ResetFile()
{
  std::streampos pos = 0;
  if (this->Format == FileType::CBinary)
  {
    pos = this->HasBinaryHeader ? MAX_LINE_LENGTH : 0;
  }
  else if (this->Format == FileType::FBinary)
  {
    pos = this->HasBinaryHeader ? MAX_LINE_LENGTH + this->FortranSkipBytes * 2 : 0;
  }
  this->MoveToPosition(pos);
}

//------------------------------------------------------------------------------
bool EnSightFile::OpenFile(bool isCaseFile /* = false*/)
{
  return this->OpenFile(this->FileNamePattern, isCaseFile);
}

//------------------------------------------------------------------------------
// assumes that if you're trying to open the file, but it's already open, that you
// also want to reset the read position
// in the case of binary files, check to see if it starts with 'C/Fortran binary'
// and if so set read position to just after that
bool EnSightFile::OpenFile(const std::string& filename, bool isCaseFile /* = false*/)
{
  if (this->Stream && this->CurrentOpenFileName == filename)
  {
    this->ResetFile();
    return true;
  }
  else if (this->Stream && this->CurrentOpenFileName != filename)
  {
    this->CloseFile();
  }

  this->Stream = new vtksys::ifstream(filename.c_str(), ios::binary);
  if (this->Stream->fail())
  {
    vtkGenericWarningMacro("opening file " << filename << " failed!");
    delete this->Stream;
    this->Stream = nullptr;
    return false;
  }
  this->CurrentOpenFileName = filename;

  if (isCaseFile)
  {
    this->Format = FileType::ASCII;
    return true;
  }

  if (this->Format == FileType::CBinary || this->Format == FileType::ASCII)
  {
    // That means we've already set the file type on this file,
    // so don't bother checking
    this->ResetFile();
    return true;
  }

  // read the first line to check the format
  auto result = this->ReadLine();
  auto& header = result.second;
  std::transform(
    header.begin(), header.end(), header.begin(), [](unsigned char c) { return std::tolower(c); });
  if (header.find("c binary") != std::string::npos)
  {
    this->Format = FileType::CBinary;
    this->Stream->seekg(MAX_LINE_LENGTH, ios::beg);
    this->HasBinaryHeader = true;
  }
  else
  {
    // wasn't C Binary, check for Fortran Binary
    // Fortan files have 4 bytes on each side of each read
    this->Stream->seekg(0, ios::beg);
    result = this->ReadLine(88);
    auto& fline = result.second;

    // the 4 bytes starting each read in little endian and big endian
    char le_len[4] = { 0x50, 0x00, 0x00, 0x00 };
    char be_len[4] = { 0x00, 0x00, 0x00, 0x50 };
    bool le_isFortran = true;
    bool be_isFortran = true;
    for (int c = 0; c < 4; c++)
    {
      le_isFortran = le_isFortran && (fline[c] == le_len[c]) && (fline[c + 84] == le_len[c]);
      be_isFortran = be_isFortran && (fline[c] == be_len[c]) && (fline[c + 84] == be_len[c]);
    }

    if (le_isFortran || be_isFortran)
    {
      this->Format = FileType::FBinary;
      this->FortranSkipBytes = 4;
      this->ByteOrder = le_isFortran ? Endianness::Little : Endianness::Big;
      if (fline.find("Fortran Binary") != std::string::npos)
      {
        this->HasBinaryHeader = true;
      }
    }
    else
    {
      this->Format = FileType::ASCII;
      this->Stream->seekg(0, ios::beg);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void EnSightFile::CloseFile()
{
  if (this->Stream)
  {
    if (this->Stream->is_open())
    {
      this->Stream->close();
    }
    delete this->Stream;
    this->Stream = nullptr;
  }
  this->CurrentOpenFileName.clear();
}

//------------------------------------------------------------------------------
std::pair<bool, std::string> EnSightFile::ReadNextLine(int size /* = MAX_LINE_LENGTH*/)
{
  if (this->Format != FileType::ASCII)
  {
    return this->ReadLine(size);
  }

  bool isComment = true;
  bool lineRead = true;

  std::pair<bool, std::string> result;
  while (isComment && lineRead)
  {
    result = this->ReadLine(size);
    lineRead = result.first;
    auto& line = result.second;
    if (lineRead && line[0] != '#')
    {
      if (!std::all_of(line.begin(), line.end(), isspace))
      {
        // The line was not empty, not beginning by '#' and not composed
        // of only white space, this is not a comment
        isComment = false;
      }
    }
  }

  if (lineRead)
  { // remove any trailing comments from the line
    char comment = '#';
    size_t found = result.second.find(comment);
    if (found != std::string::npos)
    {
      result.second.erase(found);
    }
  }

  return result;
}

//------------------------------------------------------------------------------
std::pair<bool, std::string> EnSightFile::ReadLine(int size /* = MAX_LINE_LENGTH*/)
{
  std::vector<char> line(size);
  if (this->Format == FileType::ASCII)
  {
    this->Stream->getline(line.data(), size);
  }
  else
  {
    this->MoveReadPosition(this->FortranSkipBytes);
    this->Stream->read(line.data(), size);
    line[size - 1] = '\0';
    this->MoveReadPosition(this->FortranSkipBytes);
  }

  bool retVal = true;
  if (this->Stream->fail())
  {
    // Reset the error flag before returning. This way, we can keep working
    // if we handle the error downstream.
    this->Stream->clear();
    retVal = false;
  }
  else if (this->Stream->eof())
  {
    // it seems on some builds, when eof, the output of tellg is -1, which can
    // mess up what EnSightDataSet is expecting (e.g. if GoBackOneLine is called).
    // Resetting the flags in this case appears to fix that issue.
    this->Stream->clear();
  }
  if (this->Format == FileType::FBinary || this->Format == FileType::Unknown)
  {
    // in this case we can't truncate at \0 because we have (or are trying to figure out)
    // a fortran binary file and it has some bytes on the end that we expect to be there
    return std::make_pair(retVal, std::string(line.data(), size));
  }
  // for ASCII/C binary let the string get truncated with a \0 char
  return std::make_pair(retVal, std::string(line.data()));
}

//------------------------------------------------------------------------------
void EnSightFile::SkipNLines(vtkIdType n)
{
  if (this->Format == FileType::ASCII)
  {
    for (vtkIdType i = 0; i < n; i++)
    {
      this->ReadNextLine();
    }
  }
  else
  {
    auto numBytes = n * (MAX_LINE_LENGTH + this->FortranSkipBytes * 2);
    this->MoveReadPosition(numBytes);
  }
}

//------------------------------------------------------------------------------
void EnSightFile::GoBackOneLine()
{
  auto pos = this->Stream->tellg();
  pos -= this->Stream->gcount();
  pos -= (this->FortranSkipBytes * 2);
  this->Stream->seekg(pos, ios::beg);
}

//------------------------------------------------------------------------------
bool EnSightFile::DetectByteOrder(int* result)
{
  if (this->ByteOrder == Endianness::Unknown)
  {
    int tmpLE = *result;
    int tmpBE = *result;
    vtkByteSwap::Swap4LE(&tmpLE);
    vtkByteSwap::Swap4BE(&tmpBE);

    if (tmpLE >= 0 && tmpLE < MAXIMUM_PART_ID)
    {
      this->ByteOrder = Endianness::Little;
      *result = tmpLE;
      return true;
    }
    if (tmpBE >= 0 && tmpBE < MAXIMUM_PART_ID)
    {
      this->ByteOrder = Endianness::Big;
      *result = tmpBE;
      return true;
    }
    vtkGenericWarningMacro("Byte order could not be determined.");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void EnSightFile::MoveReadPosition(int numBytes)
{
  auto pos = this->Stream->tellg();
  pos += numBytes;
  this->Stream->seekg(pos, ios::beg);
}

//------------------------------------------------------------------------------
void EnSightFile::MoveToPosition(std::streampos pos)
{
  this->Stream->seekg(pos, ios::beg);
}

//------------------------------------------------------------------------------
std::streampos EnSightFile::GetCurrentPosition()
{
  return this->Stream->tellg();
}

//------------------------------------------------------------------------------
int EnSightFile::GetCurrentOpenTimeStep()
{
  if (this->CurrentOpenFileName.empty() || this->TimeSet == -1)
  {
    return -1;
  }

  return this->TimeStepIndex;
}

VTK_ABI_NAMESPACE_END
} // end namespace ensight_gold
