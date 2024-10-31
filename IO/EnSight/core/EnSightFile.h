// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef EnSightFile_h
#define EnSightFile_h

#include "vtkByteSwap.h"

#include "vtksys/FStream.hxx"

#include <cassert>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ensight_gold
{
VTK_ABI_NAMESPACE_BEGIN

constexpr int MAX_LINE_LENGTH = 80;
// This is half the precision of an int.
constexpr int MAXIMUM_PART_ID = 65536;

enum class FileType
{
  Unknown,
  ASCII,
  CBinary,
  FBinary
};

enum class Endianness
{
  Unknown,
  Little,
  Big
};

struct TimeSetInfo
{
  int NumberOfSteps;
  std::vector<int> FileNameNumbers;
  std::vector<double> TimeValues;
};

using TimeSetInfoMapType = std::map<int, std::shared_ptr<TimeSetInfo>>;

struct FileSetInfo
{
  std::vector<int> NumberOfSteps;
  std::vector<int> FileNameIndex;
  std::vector<int> TimeStepIndexInFile;
};

using FileSetInfoMapType = std::map<int, std::shared_ptr<FileSetInfo>>;

/**
 * EnSightFile performs processing on a single file, whether it's a case file,
 * geometry, etc. It also works on ASCII, C binary, and Fortran binary files.
 */
struct EnSightFile
{
  FileType Format = FileType::Unknown;
  Endianness ByteOrder = Endianness::Unknown;
  int TimeSet = -1;
  int FileSet = -1;
  bool InBlockRead = false;

  EnSightFile();
  ~EnSightFile();

  /**
   * Set the filename. isCaseFile should be set to true for case files, or other similar
   * type of metadata files (like a filename numbers file). When true, it will immediately
   * open that file. Returns true that the filename was set and in the case of casefiles,
   * returns true if OpenFile was successful
   */
  bool SetFileNamePattern(const std::string& filename, bool isCaseFile = false);

  /**
   * Set the time and file set ids
   */
  void SetTimeAndFileSetInfo(int timeSet, int fileSet);

  /**
   * Set the time set info
   */
  void SetTimeSetInfo(std::shared_ptr<TimeSetInfo> info);
  std::shared_ptr<TimeSetInfo> GetTimeSetInfo() { return this->TimeInfo; }

  /**
   * Set the file set info
   */
  void SetFileSetInfo(std::shared_ptr<FileSetInfo> info);

  /**
   * Set the time step to read. If data is not transient, will just open the file if it's not
   * already.
   * For transient data, it will make sure the correct file is open, and go to the correct time
   * step in the file.
   * If it returns false, that means some file open or seek operation failed.
   * See output messages for details on the failure.
   */
  bool SetTimeStepToRead(double ts);

  /**
   * Checks if this file has multiple time steps or not. If there's a wildcard in the
   * FileNamePattern, return true, otherwise it will check for the existence of the BEGIN TIME STEP
   * line.
   */
  bool CheckForMultipleTimeSteps();

  /**
   * Checks for a BEGIN TIME STEP line and ensures the file is at the correct position
   * to continue reading.
   */
  void CheckForBeginTimeStepLine();

  /**
   * Checks for a END TIME STEP line. Returns true if found and sets the position to be just after
   * that line. Returns false if that line isn't found and resets the position to the place the file
   * was at before this call.
   */
  bool CheckForEndTimeStepLine();

  /**
   * For ASCII files, reads the next line while skipping lines that contain only whitespace
   * or a comment. For binary files, just calls ReadLine().
   */
  std::pair<bool, std::string> ReadNextLine(int size = MAX_LINE_LENGTH);

  /**
   * Reads the next line up to size characters (ASCII) or size characters (binary)
   */
  std::pair<bool, std::string> ReadLine(int size = MAX_LINE_LENGTH);

  /** Ignore the the next characters until either the line end delimiter is met or size characters
   * have been ignored (if provided).
   * For binary formats, ignore the next size characters + padding (but you should probably use
   * another method)*/
  void SkipLine(vtkTypeInt64 size = std::numeric_limits<std::streamsize>::max());

  /**
   * Skip the specified number of non-numeric lines when reading.
   * WARNING: Should only be used for non-numeric lines, even in ASCII mode!
   * Some sections in ASCII will contain multiple numbers per line, in which
   * case the MAX_LINE_LENGTH limit will probably cut off the line.
   */
  void SkipNLines(vtkIdType n);

  /**
   * Skip the specified number of numbers when reading.
   * For binary files, moves the read position the appropriate number of bytes
   * (numsPerLine arg is irrelevant).
   * For ASCII files, one or more numbers can be contained on a line, depending on
   * the section, so numsPerLine should be specified if it's not 1.
   */
  template <typename T>
  void SkipNNumbers(vtkIdType n, int numsPerLine = 1);

  /**
   * Move the read position of the file stream back by MAX_LINE_LENGTH characters.
   */
  void GoBackOneLine();

  /**
   * Attempts to determine the byte order given an int read from the file.
   */
  bool DetectByteOrder(int* result);

  /**
   * Read a number from file and store it in result
   */
  template <typename T>
  bool ReadNumber(T* result, bool padBeginning = true, bool padEnd = true);

  /**
   * Read an array of size n.
   * singleLine applies only to ASCII files (ignored for binary). Set true if all numbers in the
   * array to be read are written on the same line, false otherwise.
   * padBeginning and padEnd applies only to Fortran binary files. If the array to be read is a
   * full fortran write (i.e., there is the 4 padding bytes on both sides), then both will be true.
   * Setting one or both to false enables partial arrays to be read (such as when reading cells).
   */
  template <typename T>
  bool ReadArray(
    T* result, vtkIdType n, bool singleLine = false, bool padBeginning = true, bool padEnd = true);

  /**
   * Move the read position ahead n bytes.
   */
  void MoveReadPosition(vtkTypeInt64 numBytes);

  /**
   * Get current position of reader in stream.
   */
  std::streampos GetCurrentPosition();

  /**
   * This is used when change_coords_only is set, for determining if the file we currently have
   * open is the file that contains the connectivity.
   */
  int GetCurrentOpenTimeStep();

  /**
   * Opens the file and performs some processing to determine the format of the file.
   * Appropriately resets the position of the file stream depending on the type of file.
   */
  bool OpenFile(const std::string& filename, bool isCaseFile = false);
  bool OpenFile(bool isCaseFile = false);

private:
  std::string FileNamePattern;
  std::string CurrentOpenFileName;
  std::shared_ptr<TimeSetInfo> TimeInfo;
  std::shared_ptr<FileSetInfo> FileInfo;

  // This map keeps track of the positions in the file
  // where time steps begin
  // file index to the vector of positions for that file
  std::map<int, std::vector<std::streampos>> TimeStepBeginPositions;
  int CurrentFileIndex = -1;

  int TimeStepIndex = -1;

  vtksys::ifstream* Stream;
  bool HasBinaryHeader = false;
  int FortranSkipBytes = 0;

  /**
   * Resets the read position of the file.
   */
  void ResetFile();

  /**
   * Closes the file, if open.
   */
  void CloseFile();

  /**
   * Move the read position ahead to position pos.
   */
  void MoveToPosition(std::streampos pos);
};

template <typename T>
int getNumChars();

template <typename T>
bool stringTo(const std::string& input, T& output);

//------------------------------------------------------------------------------
template <typename T>
void EnSightFile::SkipNNumbers(vtkIdType n, int numsPerLine /* = 1 */)
{
  // for ascii, n is number of numeric lines to skip
  if (this->Format == FileType::ASCII)
  {
    // this->SkipNLines(n);
    //  so this format has max of 10 digits for integers
    //  for float, 12 characters total
    //  there's also white space allowed between numbers
    int size = getNumChars<T>() * numsPerLine + 10 * numsPerLine;
    vtkIdType lineIdx = 0;
    while (lineIdx < n)
    {
      auto result = this->ReadLine(size);
      lineIdx++;
      if (!result.first)
      {
        vtkGenericWarningMacro("SkipNNumbers() the full ascii line was not read");
      }
    }
  }
  else
  {
    vtkTypeInt64 numBytes = n * sizeof(T) + this->FortranSkipBytes * 2;
    this->MoveReadPosition(numBytes);
  }
}

//------------------------------------------------------------------------------
template <typename T>
bool EnSightFile::ReadNumber(T* result, bool padBeginning /* = true*/, bool padEnd /* = true*/)
{
  if (this->Format == FileType::ASCII)
  {
    auto line = this->ReadNextLine();
    stringTo(line.second, *result);
  }
  else
  {
    if (padBeginning)
    {
      assert(!this->InBlockRead);
      this->MoveReadPosition(this->FortranSkipBytes);
    }
    if (!this->Stream->read((char*)result, sizeof(T)))
    {
      vtkGenericWarningMacro("read failed");
      return false;
    }
    if (padEnd)
    {
      assert(!this->InBlockRead);
      this->MoveReadPosition(this->FortranSkipBytes);
    }
    if (this->ByteOrder == Endianness::Little)
    {
      vtkByteSwap::Swap4LE(result);
    }
    else if (this->ByteOrder == Endianness::Big)
    {
      vtkByteSwap::Swap4BE(result);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
template <typename T>
bool EnSightFile::ReadArray(T* result, vtkIdType n, bool singleLine /* = false*/,
  bool padBeginning /* = true*/, bool padEnd /* = true*/)
{
  if (n <= 0)
  {
    return true;
  }

  if (this->Format == FileType::ASCII)
  {
    if (!singleLine)
    {
      for (int i = 0; i < n; i++)
      {
        this->ReadNumber(&result[i]);
      }
    }
    else
    {
      int size = getNumChars<T>() * n + 10 * n;
      auto lineResult = this->ReadLine(size);
      if (!lineResult.first)
      {
        vtkGenericWarningMacro("ReadArray() the full ascii line was not read");
      }

      std::stringstream ss(lineResult.second);
      for (int i = 0; i < n; i++)
      {
        ss >> result[i];
      }
    }
  }
  else
  {
    // In some cases we want to read everything in a single fortran
    // read into a single array, but sometimes we don't want to, so
    // we have to handle the skip bytes appropriately
    if (padBeginning && this->FortranSkipBytes > 0)
    {
      this->MoveReadPosition(this->FortranSkipBytes);
    }
    if (!this->Stream->read((char*)result, sizeof(T) * n))
    {
      vtkGenericWarningMacro("read array failed");
      return false;
    }
    if (padEnd && this->FortranSkipBytes > 0)
    {
      this->MoveReadPosition(this->FortranSkipBytes);
    }
    if (this->ByteOrder == Endianness::Little)
    {
      vtkByteSwap::Swap4LERange(result, n);
    }
    else if (this->ByteOrder == Endianness::Big)
    {
      vtkByteSwap::Swap4BERange(result, n);
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
} // end namespace ensight_gold

#endif // end EnSightFile_h
