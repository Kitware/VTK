// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMReXGridReaderInternal
 * @brief   Consists of the low-level AMReX Reader used by the vtkAMReXGridReader.
 *
 * @sa
 *  vtkAMReXGridReader
 */

#ifndef vtkAMReXGridReaderInternal_h
#define vtkAMReXGridReaderInternal_h

#include <map>
#include <string>
#include <vector>

#include "vtkDataSet.h"
#include "vtkSOADataArrayTemplate.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIndent;

//==============================================================================
//            I N T E R N A L   A M R e X     R E A D E R
//==============================================================================

// ----------------------------------------------------------------------------
//                     Class  RealDecriptor (begin)
// ----------------------------------------------------------------------------

/*
   floating point format specification (fmt):
    -   fmt[0] = # of bits per number
    -   fmt[1] = # of bits in exponent
    -   fmt[2] = # of bits in mantissa
    -   fmt[3] = start bit of sign
    -   fmt[4] = start bit of exponent
    -   fmt[5] = start bit of mantissa
    -   fmt[6] = high order mantissa bit (CRAY needs this)
    -   fmt[7] = bias of exponent

    64 11 52 0 1 12 0 1023 - IEEE Double

   byte order (ord) handles endianness (and defines size such as float or double)
    -   ord[0] = byte in 1st byte
    -   ord[1] = byte in 2nd byte
    -   ord[2] = byte in 3rd byte
    -   ord[3] = byte in 4th byte
    -   ...
*/

class RealDescriptor
{
public:
  RealDescriptor();
  RealDescriptor(const long* format, const int* order, int order_length);
  const long* format() const&;
  const std::vector<long>& formatarray() const&;
  const int* order() const&;
  const std::vector<int>& orderarray() const&;
  long numBytes() const;
  bool operator==(const RealDescriptor& rd) const;

private:
  std::vector<long> fr;
  std::vector<int> ord;
};

// ----------------------------------------------------------------------------
//                     Class  RealDescriptor ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  vtkAMReXGridHeader (begin)
// ----------------------------------------------------------------------------

class vtkAMReXGridHeader
{
public:
  std::string versionName;
  int variableNamesSize;
  std::vector<std::string> variableNames;

  // prefix string indicating a variable is a vector component
  // Note: this prefix will be removed from any variable name
  // whether or not the variable name is a properly formed
  // vector variable name (contains a proper postfix)
  std::string vectorNamePrefix = "amrexvec";

  // delimiter must be the same after prefix and before postfix
  char nameDelim = '_';

  // variableNames map to (potentially a collection of) variableNames indices
  std::map<std::string, std::vector<int>> parsedVariableNames;
  std::map<std::string, std::vector<int>> extraMultiFabParsedVarNames;
  std::map<std::string, int> extraMultiFabParsedVarMap;

  int dim;
  double time;
  int finestLevel;
  std::vector<double> problemDomainLoEnd;
  std::vector<double> problemDomainHiEnd;
  std::vector<int> refinementRatio;
  std::vector<std::vector<std::vector<int>>> levelDomains;
  std::vector<int> levelSteps;
  std::vector<std::vector<double>> cellSize;
  int geometryCoord;
  int magicZero;
  std::vector<int> levelSize;
  std::vector<std::vector<std::vector<std::vector<double>>>> levelCells;
  std::vector<std::string> levelPrefix;
  std::vector<std::string> multiFabPrefix;

  // use this to store the prefixes for extra multifabs appended to the end of the main header
  int extraMultiFabCount;
  // only allow one topology per multifab. 0 == vertex data, 3 == cell data
  // edge and face data not supported
  std::vector<int> extraMultiFabVarTopology;
  // prefix for each multifab on each level [fab][level]
  std::vector<std::vector<std::string>> extraMultiFabPrefixes;
  // vector of variables stored on each fab [fab][variable]
  std::vector<std::vector<std::string>> extraMultiFabVariables;

  bool debugHeader;

  vtkAMReXGridHeader();

  void PrintSelf(std::ostream& os, vtkIndent indent);
  void PrintSelfGenericHeader(std::ostream& os, vtkIndent indent);
  bool Parse(const std::string& headerData);
  bool ParseGenericHeader(const std::string& headerData);

  void SetVectorNamePrefix(const std::string& prefix);
  void SetNameDelimiter(char delim);

private:
  // if the vectorNamePrefix is detected at the beginning of the name,
  // remove it along with the expected x/y/z postfix. Otherwise, return
  // the original string
  std::string GetBaseVariableName(const std::string& name);

  // returns 0 if postfix is x, 1 for y and 2 for z. returns -1 otherwise
  int CheckComponent(const std::string& name);

  // check if name has the vectorNamePrefix
  bool HasVectorPrefix(const std::string& name);
};

// ----------------------------------------------------------------------------
//                     Class  vtkAMReXGridHeader ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  vtkAMReXGridLevelHeader (begin)
// ----------------------------------------------------------------------------

class vtkAMReXGridLevelHeader
{
  enum Version
  {
    Undefined_v1 = 0,          // ---- undefined
    Version_v1 = 1,            // ---- auto converting version with headers
                               // ---- for each fab in the data files and
                               // ---- min and max values for each fab in the header
    NoFabHeader_v1 = 2,        // ---- no fab headers, no fab mins or maxes
    NoFabHeaderMinMax_v1 = 3,  // ---- no fab headers,
                               // ---- min and max values for each fab in the header
    NoFabHeaderFAMinMax_v1 = 4 // ---- no fab headers, no fab mins or maxes,
                               // ---- min and max values for each FabArray in the header
  };
  enum Ordering
  {
    NormalOrder = 1,
    ReverseOrder = 2
  };

public:
  int level;
  int dim;
  int levelVersion;
  int levelHow;
  int levelNumberOfComponents;
  int levelNumberOfGhostCells;
  int levelBoxArraySize;
  int levelMagicZero;
  std::vector<std::vector<std::vector<int>>> levelBoxArrays;
  int levelNumberOfFABOnDisk;
  std::string levelFabOnDiskPrefix;
  std::vector<std::string> levelFABFile;
  std::vector<long> levelFileOffset;
  std::vector<std::vector<double>> levelMinimumsFAB;
  std::vector<std::vector<double>> levelMaximumsFAB;
  std::vector<double> levelFABArrayMinimum;
  std::vector<double> levelFABArrayMaximum;
  int levelRealNumberOfBytes;
  int levelRealOrder;
  bool debugLevelHeader;

  vtkAMReXGridLevelHeader();
  void PrintSelf(std::ostream& os, vtkIndent indent);
  void PrintSelfLevelHeader(std::ostream& os, vtkIndent indent);
  bool Parse(int _level, int _dim, const std::string& headerData);
  bool ParseLevelHeader(int _level, int _dim, const std::string& headerData);
};

// ----------------------------------------------------------------------------
//                     Class  vtkAMReXGridLevelHeader ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  vtkAMReXGridReaderInternal (begin)
// ----------------------------------------------------------------------------

class vtkAMReXGridReaderInternal
{
public:
  vtkAMReXGridReaderInternal();
  ~vtkAMReXGridReaderInternal();
  void DestroyHeader();
  void DestroyLevelHeader();
  void PrintSelf(std::ostream& os, vtkIndent indent);
  void SetFileName(char* fName);

  void ReadMetaData();
  bool ReadHeader();
  bool ReadLevelHeader();
  bool ReadExtraFabHeader();
  int GetNumberOfLevels();
  int GetBlockLevel(int blockIdx);
  int GetNumberOfBlocks();
  int GetBlockIndexWithinLevel(int blockIdx, int level);
  void GetBlockAttribute(const char* attribute, int blockIdx, vtkDataSet* pDataSet);
  void GetExtraMultiFabBlockAttribute(const char* attribute, int blockIdx, vtkDataSet* pDataSet);
  int GetOffsetOfAttribute(const char* attribute);
  int GetAttributeOffsetExtraMultiFab(const char* attribute, int fabIndex);
  int GetExtraMultiFabIndex(const char* attribute);
  void ReadFAB(std::istream& is);
  int ReadVersion(std::istream& is);
  void ReadOrder(std::istream& is, std::vector<int>& ar);
  void PrintOrder(std::vector<int>& ar);
  void ReadFormat(std::istream& is, std::vector<long>& ar);
  void PrintFormat(std::vector<long>& ar);
  RealDescriptor* ReadRealDescriptor(std::istream& is);
  long ReadBoxArray(std::istream& is, int* boxArray, int* boxArrayDim);
  void PrintBoxArray(int* boxArray);
  int ReadNumberOfAttributes(std::istream& is);
  void ReadBlockAttribute(std::istream& is, long numberOfPoints, long size, char* buffer);
  void Convert(
    void* out, const void* in, long nitems, const RealDescriptor& ord, const RealDescriptor& ird);
  void PermuteOrder(
    void* out, const void* in, long nitems, const int* outord, const int* inord, int REALSIZE);

  template <typename T>
  void CreateVTKAttributeArray(vtkAOSDataArrayTemplate<T>* dataArray, const RealDescriptor* ord,
    const RealDescriptor* ird, const std::vector<std::vector<char>>& buffers, int numberOfPoints,
    const std::string& attribute);

  bool headersAreRead;
  bool extraMultiFabHeadersAreRead;
  bool debugReader;
  std::string FileName;
  vtkAMReXGridHeader* Header;
  friend class vtkAMReXGridHeader;
  std::vector<vtkAMReXGridLevelHeader*> LevelHeader;
  std::vector<std::vector<vtkAMReXGridLevelHeader*>> ExtraMultiFabHeader;
  friend class vtkAMReXGridLeveHeader;
};

template <typename T>
void vtkAMReXGridReaderInternal::CreateVTKAttributeArray(vtkAOSDataArrayTemplate<T>* dataArray,
  const RealDescriptor* ord, const RealDescriptor* ird,
  const std::vector<std::vector<char>>& buffers, int numberOfPoints, const std::string& attribute)
{
  int nComps = static_cast<int>(this->Header->parsedVariableNames[attribute].size());
  if (nComps == 0) // check if the variable is in an extra fab
  {
    nComps = static_cast<int>(this->Header->extraMultiFabParsedVarNames[attribute].size());
  }
  if (nComps == 0)
  {
    return;
  }
  dataArray->SetName(attribute.c_str());
  dataArray->SetNumberOfComponents(nComps);
  dataArray->SetNumberOfTuples(numberOfPoints);
  T* arrayPtr = new T[numberOfPoints];
  for (int j = 0; j < nComps; ++j)
  {
    this->Convert(arrayPtr, buffers[j].data(), numberOfPoints, *ord, *ird);

    // Copy to data array component
    for (int i = 0; i < numberOfPoints; ++i)
    {
      dataArray->SetTypedComponent(i, j, arrayPtr[i]);
    }
  }
  delete[] arrayPtr;
}

// ----------------------------------------------------------------------------
//                     Class  vtkAMReXGridReaderInternal ( end )
// ----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_END
#endif /* vtkAMReXGridReaderInternal_h */
// VTK-HeaderTest-Exclude: vtkAMReXGridReaderInternal.h
