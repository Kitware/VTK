/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LSDynaFamily.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "LSDynaFamily.h"
//#include "vtksys/SystemTools.hxx"

#include <errno.h>
#include <ctype.h>
#include <cassert>

#include <string>
#include <set>
#include <vector>
#include <algorithm>
#include <map>

namespace
{
//Documentation on why the exemption
#define USE_STAT_64 VTK_SIZEOF_ID_TYPE==8 && !defined _DARWIN_FEATURE_64_BIT_INODE && !defined __FreeBSD__
//OS X and FreeBSD use stat instead of stat64
#if (USE_STAT_64)
//64bit
#ifndef WIN32
int LS_DYNA_STAT(const char* fname, struct stat64& s)
{
  //POSIX
  return stat64(fname,&s);
}
#else
int LS_DYNA_STAT(const char* fname, struct __stat64& s)
{
  //windows
#if defined(_MSC_VER) && _MSC_VER < 1400
  return _stat64(fname, &s);
#else
  return __stat64(fname, &s);
#endif
}
#endif

#else
//32bit
int LS_DYNA_STAT(const char* fname, struct stat& s)
{
  //POSIX
  return stat(fname,&s);
}
#endif

vtkLSDynaFile_t VTK_LSDYNA_OPENFILE(const char* fname)
{
#ifndef WIN32
  vtkLSDynaFile_t f = open(fname, O_RDONLY);
  return f;
#else
  vtkLSDynaFile_t f = fopen(fname, "rb");
  setvbuf(f,NULL,_IONBF,0); //disable buffer
  return f;
#endif
}

  std::string vtkLSGetFamilyFileName( const char* basedir, const std::string& dbname, int adaptationLvl, int number )
  {
    std::string blorb;

    blorb = basedir + dbname;

    if ( adaptationLvl > 0 )
      {
      // convert adaptationLvl from an integer to "aa", "ab", "ac", ...
      // and tack it onto the end of our blorb.
      std::string slvl;
      int a = adaptationLvl - 1;
      while ( a )
        {
        slvl += char(97 + (a % 26) );
        a = a / 26;
        }
      while ( slvl.size() < 2 )
        {
        slvl += 'a';
        }
      std::reverse( slvl.begin(), slvl.end() );
      blorb += slvl;
      }

    if ( number > 0 )
      {
      char n[4];
      sprintf(n, "%02d", number);
      blorb += n;
      }

    return blorb;
  }
}

const char* LSDynaFamily::SectionTypeNames[] =
{
  "ControlSection",
  "StaticSection",
  "TimeStepSection",
  "MaterialTypeData",
  "FluidMaterialIdData",
  "SPHElementData",
  "GeometryData",
  "UserIdData",
  "AdaptedParentData",
  "SPHNodeData",
  "RigidSurfaceData",
  "EndOfStaticSection",
  "ElementDeletionState",
  "SPHNodeState",
  "RigidSurfaceState"
};

const float LSDynaFamily::EOFMarker = -999999.0f;

struct LSDynaFamily::BufferingInfo
{
    BufferingInfo():
      numWordsToRead(0),
      loopTimes(0),
      leftOver(0),
      size(1048576)
      {};
    vtkIdType numWordsToRead;
    vtkIdType loopTimes;
    vtkIdType leftOver;
    const vtkIdType size; //deafult buffer size
private:
  LSDynaFamily::BufferingInfo& operator=( const LSDynaFamily::BufferingInfo & );
};

//-----------------------------------------------------------------------------
LSDynaFamily::LSDynaFamily()
    {
    this->FD = VTK_LSDYNA_BADFILE; // No file open
    this->FAdapt = -1; // Invalid adaptation
    this->FNum = -1; // No files in filelist
    this->FWord = 0; // At start of file

    this->SwapEndian = -1; // No endian-ness determined
    this->WordSize = 0; // No word size determined

    this->TimeStep = 0; // Initial time step
    this->StateSize = 0; // Time steps take up no room on disk

    this->AdaptationsMarkers.push_back( LSDynaFamilyAdaptLevel() );
    this->Chunk = NULL;
    this->ChunkWord = 0;
    this->ChunkAlloc = 0;

    this->FileHandlesClosed = false;

    this->BufferInfo = new LSDynaFamily::BufferingInfo();
    }

//-----------------------------------------------------------------------------
LSDynaFamily::~LSDynaFamily()
    {
    if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
      {
      VTK_LSDYNA_CLOSEFILE(this->FD);
      }

    if ( this->Chunk )
      {
      delete [] this->Chunk;
      }

    delete this->BufferInfo;
    }

//-----------------------------------------------------------------------------
void LSDynaFamily::SetDatabaseDirectory( std::string dd )
  {
  this->DatabaseDirectory = dd;
  }
std::string LSDynaFamily::GetDatabaseDirectory()
  {
  return this->DatabaseDirectory;
  }

//-----------------------------------------------------------------------------
void LSDynaFamily::SetDatabaseBaseName( std::string bn )
  {
  this->DatabaseBaseName = bn;
  }
std::string LSDynaFamily::GetDatabaseBaseName()
  {
  return this->DatabaseBaseName;
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::ScanDatabaseDirectory()
  {
  // FIXME: None of this need be cleared if we are trying to track a
  // simulation in progress.  But it won't hurt to redo the scan from the
  // beginning... it will just take longer.
  this->Files.clear();
  this->FileSizes.clear();
  this->FileAdaptLevels.clear();
  this->TimeAdaptLevels.clear();
  this->Adaptations.clear();
  this->TimeStepMarks.clear();

  std::string tmpFile;
  int filenum = 0;
  int adaptLevel = 0;
  int tryAdapt = 0; // don't try an adaptive step unless we have one good file at the current level.
  bool adapted = true; // true when advancing over a mesh adaptation.
#if defined (WIN32) && VTK_SIZEOF_ID_TYPE==8
  struct __stat64 st;
#elif USE_STAT_64
  struct stat64 st;
#else
  struct stat st;
#endif
  while ( tryAdapt >= 0 )
    {
    tmpFile = vtkLSGetFamilyFileName( this->DatabaseDirectory.c_str(),
                                      this->DatabaseBaseName,
                                      adaptLevel,
                                      filenum );
    if ( LS_DYNA_STAT( tmpFile.c_str(), st) == 0 )
      {
      if ( adapted )
        {
        this->Adaptations.push_back( (int)this->Files.size() );
        adapted = false;
        }
      this->Files.push_back( tmpFile );
      this->FileSizes.push_back( st.st_size );
      this->FileAdaptLevels.push_back( adaptLevel );
      tryAdapt = 1;
      ++filenum;
      }
    else
      {
      --tryAdapt;
      ++adaptLevel;
      filenum = 0;
      adapted = true;
      }
    }
    return this->Files.size() == 0;
  }

//-----------------------------------------------------------------------------
const char* LSDynaFamily::SectionTypeToString( SectionType s )
  {
  return SectionTypeNames[s];
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::SkipToWord( SectionType sType, vtkIdType sId, vtkIdType wordNumber )
  {
  LSDynaFamilySectionMark mark;
  if ( sType != TimeStepSection && sType < ElementDeletionState )
    {
    assert( sId < (int)this->Adaptations.size() );
    if ( sId < 0 )
      sId = 0;
    mark = this->AdaptationsMarkers[sId].Marks[ sType ];
    mark.Offset += wordNumber;
    }
  else
    {
    // NOTE: SkipToWord cannot jump outside of the current adaptation level!
    // You must use SetTimeStep() to do that -- it will call ReadHeaderInformation().
    mark = this->AdaptationsMarkers[this->FAdapt].Marks[ sType ];
    mark.Offset += wordNumber;
    if ( sId >= (vtkIdType) this->TimeStepMarks.size() )
      {
      return 1;
      }
    mark.FileNumber = this->TimeStepMarks[ sId ].FileNumber;
    mark.Offset = this->TimeStepMarks[ sId ].Offset +
      ( this->AdaptationsMarkers[this->FAdapt].Marks[sType].Offset -
        this->AdaptationsMarkers[this->FAdapt].Marks[TimeStepSection].Offset ) +
      wordNumber;
    }

  // if the skip is too big for one file, advance to the correct file
  while ( (mark.FileNumber < (vtkIdType) this->Files.size()) && (mark.Offset > this->FileSizes[ mark.FileNumber ]) )
    {
    mark.Offset -= this->FileSizes[ mark.FileNumber ];
    mark.FileNumber++;
    }

  if ( mark.FileNumber > (vtkIdType) this->Files.size() )
    {
    // when stepping past the end of the entire database (as opposed
    // to a single file), return a different value
    return 2;
    }

  if ( this->FNum < 0 || (this->FNum != mark.FileNumber) )
    {
    if ( this->FNum >= 0 )
      {
      if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
        {
        VTK_LSDYNA_CLOSEFILE(this->FD);
        }
      }
    this->FD = VTK_LSDYNA_OPENFILE(this->Files[mark.FileNumber].c_str());
    if ( VTK_LSDYNA_ISBADFILE(this->FD) )
      {
      return errno;
      }
    this->FNum = mark.FileNumber;
    this->FAdapt = this->FileAdaptLevels[ this->FNum ];
    }
  vtkLSDynaOff_t offset = mark.Offset * this->WordSize;
  // FIXME: Handle case where wordNumber + mark.Offset > (7=factor)*512*512
  if ( VTK_LSDYNA_SEEKTELL(this->FD,offset,SEEK_SET) != offset )
    {
    return errno;
    }
  this->FWord = mark.Offset;
  return 0;
  }

// FIXME: Assumes there is a valid file open and that
// lseek will return the byte just past the time value word.
// the BufferChunks buffer).
//-----------------------------------------------------------------------------
int LSDynaFamily::MarkTimeStep()
  {
  LSDynaFamilySectionMark mark;
  mark.FileNumber = this->FNum;
  mark.Offset = VTK_LSDYNA_TELL( this->FD ) / this->GetWordSize() - 1;
  this->TimeStepMarks.push_back( mark );
  this->TimeAdaptLevels.push_back( this->FAdapt );
  return 0;
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::SkipWords( vtkIdType numWords )
  {
  if(numWords<=0)
    {
    return 0;
    }

  if ( this->FNum < 0 || VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    return -1;
    }
  vtkIdType offset = numWords*this->WordSize;

  //determine where we are currently in the file
  vtkIdType currentPos = VTK_LSDYNA_TELL(this->FD);

  VTK_LSDYNA_SEEK(this->FD, offset, SEEK_CUR);
  vtkIdType amountMoved = VTK_LSDYNA_TELL(this->FD) - currentPos;

  offset -= amountMoved;
  if(offset>0)
    {
    // try advancing to next file
    VTK_LSDYNA_CLOSEFILE(this->FD);

    // if the skip is too big for one file, advance to the correct file
    ++this->FNum;
    while ( (this->FNum < (vtkIdType) this->Files.size()) && (offset > this->FileSizes[ this->FNum ]) )
      {
      offset -= this->FileSizes[ this->FNum ];
      ++this->FNum;
      }

    this->FD = VTK_LSDYNA_OPENFILE(this->Files[this->FNum].c_str());
    this->FWord = 0;
    if ( VTK_LSDYNA_ISBADFILE(this->FD) )
      { // bad file (permissions, deleted) or request (too big)
      this->FNum = -1;
      this->FAdapt = -1;
      return errno;
      }

    //seek into the file the current offset amount
    VTK_LSDYNA_SEEK(this->FD, offset, SEEK_CUR);
    amountMoved = VTK_LSDYNA_TELL(this->FD);
    offset -= amountMoved;
    }

  this->FWord = VTK_LSDYNA_TELL(this->FD);
  return 0;
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::BufferChunk( WordType wType, vtkIdType chunkSizeInWords )
  {
  if ( chunkSizeInWords == 0 )
    return 0;

  if ( this->ChunkAlloc < chunkSizeInWords )
    {
    if ( this->Chunk )
      {
      delete [] this->Chunk;
      }
    this->ChunkAlloc = chunkSizeInWords;
    this->Chunk = new unsigned char[ this->ChunkAlloc*this->WordSize ];
    }

  this->FWord = VTK_LSDYNA_TELL(this->FD);

  // Eventually, we must check the return value and see if the read
  // came up short (EOF). If it did, then we must advance to the next
  // file.
  vtkIdType bytesLeft = chunkSizeInWords*this->WordSize;
  vtkIdType bytesRead;
  unsigned char* buf = this->Chunk;
  this->ChunkValid = 0;
  this->ChunkWord = 0;
  while ( bytesLeft )
    {
    bytesRead = VTK_LSDYNA_READ(this->FD,(void*) buf,bytesLeft);
    this->ChunkValid += bytesRead;
    if ( bytesRead < bytesLeft )
      {
      std::cout << "bytesRead < bytesLeft" << std::endl;
      std::cout << "bytesRead " << bytesRead << " bytesLeft" << bytesLeft << std::endl;
      if ( bytesRead <= 0 )
        { // try advancing to next file
        VTK_LSDYNA_CLOSEFILE(this->FD);
        if ( ++this->FNum == (vtkIdType) this->Files.size() )
          { // no more files to read. Oops.
          this->FNum = -1;
          this->FAdapt = -1;
          return 1;
          }
        this->FD = VTK_LSDYNA_OPENFILE(this->Files[this->FNum].c_str());
        this->FWord = 0;
        if ( VTK_LSDYNA_ISBADFILE(this->FD) )
          { // bad file (permissions, deleted) or request (too big)
          this->FNum = -1;
          this->FAdapt = -1;
          return errno;
          }
        }
      }
    bytesLeft -= bytesRead;
    buf += bytesRead;
    }
  this->FWord = VTK_LSDYNA_TELL(this->FD);

  if ( this->SwapEndian && wType != LSDynaFamily::Char )
    {
    unsigned char tmp[4];
    vtkIdType i;
    unsigned char* cur = this->Chunk;

    // Currently, wType is unused, but if I ever have to support cray
    // floating point types, this will need to be different
    switch (this->WordSize)
      {
    case 4:
      for (i=0; i<chunkSizeInWords; ++i)
        {
        tmp[0] = cur[0];
        tmp[1] = cur[1];
        cur[0] = cur[3];
        cur[1] = cur[2];
        cur[2] = tmp[1];
        cur[3] = tmp[0];
        cur += this->WordSize;
        }
      break;
    case 8:
    default:
      for (i=0; i<chunkSizeInWords; ++i)
        {
        tmp[0] = cur[0];
        tmp[1] = cur[1];
        tmp[2] = cur[2];
        tmp[3] = cur[3];
        cur[0] = cur[7];
        cur[1] = cur[6];
        cur[2] = cur[5];
        cur[3] = cur[4];
        cur[4] = tmp[3];
        cur[5] = tmp[2];
        cur[6] = tmp[1];
        cur[7] = tmp[0];
        cur += this->WordSize;
        }
      break;
      }
    }

  return 0;
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::ClearBuffer()
{
  if ( this->Chunk )
    {
    this->ChunkAlloc = 0;
    this->ChunkWord = 0;
    this->ChunkValid = 0;
    delete [] this->Chunk;
    this->Chunk = NULL;
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType LSDynaFamily::InitPartialChunkBuffering( const vtkIdType& numTuples,
  const vtkIdType& numComps )
{
  if(numTuples==0)
    {
    this->BufferInfo->numWordsToRead = 0;
    this->BufferInfo->leftOver = 0;
    this->BufferInfo->loopTimes = 0;
    }
  else
    {
    const vtkIdType size(this->BufferInfo->size);
    this->BufferInfo->numWordsToRead = (size * numComps);
    this->BufferInfo->leftOver=(numTuples%size) * numComps;
    this->BufferInfo->loopTimes=(numTuples/size);
    }
  int numChunks = this->BufferInfo->loopTimes;
  if(this->BufferInfo->leftOver>0)
    {
    ++numChunks;
    }
  return numChunks;
}

//-----------------------------------------------------------------------------
vtkIdType LSDynaFamily::GetNextChunk( const WordType& wType)
{
  vtkIdType size;
  if(this->BufferInfo->loopTimes > 0)
    {
    size = this->BufferInfo->numWordsToRead;
    }
  else if(this->BufferInfo->loopTimes == 0)
    {
    size = this->BufferInfo->leftOver;
    }
  else
    {
    size = 0;
    }

  this->BufferChunk(wType,size);
  --this->BufferInfo->loopTimes;
  return size;
}


//-----------------------------------------------------------------------------
int LSDynaFamily::AdvanceFile()
  {
  if ( this->FNum < 0 && VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    if ( this->Files.size() > 0 )
      {
      this->FNum = 0;
      this->FAdapt = 0;
      return 0;
      }
    else
      {
      return 1;
      }
    }
  if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    VTK_LSDYNA_CLOSEFILE(this->FD);
    //this->FD = VTK_LSDYNA_BADFILE;
    }
  this->FWord = 0;
  this->ChunkValid = 0;
  if ( this->FNum + 1 < (vtkIdType) this->Files.size() )
    {
    this->FNum++;
    this->FAdapt = this->FileAdaptLevels[ this->FNum ];
    }
  else
    {
    this->FD = VTK_LSDYNA_BADFILE;
    return 1;
    }
  this->FD = VTK_LSDYNA_OPENFILE(this->Files[this->FNum].c_str());
  if ( VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    return errno;
    }
  return 0;
  }

void LSDynaFamily::MarkSectionStart( int adaptLevel, SectionType m )
  {
  vtkIdType myWord;

  if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    myWord = VTK_LSDYNA_TELL(this->FD) / this->WordSize;
    }
  else
    {
    myWord = 0;
    }

  // OK, mark it.
  LSDynaFamilySectionMark mark;

  mark.FileNumber = this->FNum;
  mark.Offset = myWord;
  while ( adaptLevel >= (int) this->AdaptationsMarkers.size() )
    {
    this->AdaptationsMarkers.push_back( LSDynaFamilyAdaptLevel() );
    }
  this->AdaptationsMarkers[adaptLevel].Marks[m] = mark;

  //fprintf( stderr, "Mark \"%s\" is (%d,%d)\n", SectionTypeToString(m), mark.FileNumber, mark.Offset );
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::JumpToMark( SectionType m )
  {
  return this->SkipToWord( m, this->TimeStep, 0 );
  }

//-----------------------------------------------------------------------------
int LSDynaFamily::DetermineStorageModel()
  {
  double test;
  this->WordSize = 4;
  this->SwapEndian = 0;
  this->JumpToMark( ControlSection ); // opens file 0, since marks are all zeroed
  this->BufferChunk( Float, 128 ); // does no swapping, buffers enough for 64 8-byte words
  this->ChunkWord = 14;
  test = this->GetNextWordAsFloat();
  if ( test > 900. && test < 1000. )
    {
    this->JumpToMark( ControlSection ); // seek to start of file
    return 0;
    }
  this->ChunkWord = 14;
  this->WordSize = 8;
  test = this->GetNextWordAsFloat();
  if ( test > 900. && test < 1000. )
    {
    this->JumpToMark( ControlSection ); // seek to start of file
    return 0;
    }
  // OK, try swapping endianness
  this->SwapEndian = 1;
  this->WordSize = 4;
  this->JumpToMark( ControlSection ); // seek to start of file
  this->BufferChunk( Float, 128 );
  this->ChunkWord = 14;
  test = this->GetNextWordAsFloat();
  if ( test > 900. && test < 1000. )
    {
    this->JumpToMark( ControlSection ); // seek to start of file
    return 0;
    }
  this->ChunkWord = 14;
  this->WordSize = 8;
  test = this->GetNextWordAsFloat();
  if ( test > 900. && test < 1000. )
    {
    this->JumpToMark( ControlSection ); // seek to start of file
    return 0;
    }

  // Oops, couldn't identify storage model
  VTK_LSDYNA_CLOSEFILE(this->FD);
  this->FNum = -1;
  this->FAdapt = -1;
  return 1;
  }

//-----------------------------------------------------------------------------
void LSDynaFamily::SetStateSize( vtkIdType sz )
  {
  this->StateSize = sz;
  }

//-----------------------------------------------------------------------------
vtkIdType LSDynaFamily::GetStateSize() const
  {
  return this->StateSize;
  }

//-----------------------------------------------------------------------------
vtkIdType LSDynaFamily::GetNumberOfFiles()
  {
  return this->Files.size();
  }

//-----------------------------------------------------------------------------
std::string LSDynaFamily::GetFileName( int i )
  {
  return this->Files[i];
  }

//-----------------------------------------------------------------------------
vtkIdType LSDynaFamily::GetFileSize( int i )
{
  return this->FileSizes[i];
}

//-----------------------------------------------------------------------------
int LSDynaFamily::GetWordSize() const
  {
  return this->WordSize;
  }

// Reset erases all information about the current database.
// It does not free memory allocated for the current chunk.
//-----------------------------------------------------------------------------
void LSDynaFamily::Reset()
{
  if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    VTK_LSDYNA_CLOSEFILE(this->FD);
    this->FD = VTK_LSDYNA_BADFILE;
    }

  this->DatabaseDirectory = "";
  this->DatabaseBaseName = "";
  this->Files.clear();
  this->FileSizes.clear();
  this->Adaptations.clear();
  this->FileAdaptLevels.clear();
  this->TimeStepMarks.clear();
  this->TimeAdaptLevels.clear();
  this->FNum = -1;
  this->FAdapt = -1;
  this->FWord = 0;
  this->TimeStep = -1;
  this->ChunkValid = 0;
  this->FileHandlesClosed = false;

  delete this->BufferInfo;
  this->BufferInfo = new LSDynaFamily::BufferingInfo();
}

//-----------------------------------------------------------------------------
void LSDynaFamily::DumpMarks( std::ostream& os )
{
  int i, j;
  os << "Files: " << std::endl;
  for ( i = 0; i < (int) this->Files.size(); ++i )
    {
    os << i << ": " << this->Files[i]
       << " [" << this->FileAdaptLevels[i] << "] "
       << this->FileSizes[i] << std::endl;
    }
  os << std::endl;

  os << "Adaptation levels:" << std::endl;
  for ( i = 0; i < (int) this->Adaptations.size(); ++i )
    {
    os << this->Adaptations[i] << ":" << std::endl;
    for ( j = 0; j < LSDynaFamily::NumberOfSectionTypes; ++j )
      {
      os << "  " << LSDynaFamily::SectionTypeToString( (LSDynaFamily::SectionType)j ) << " = "
         << this->AdaptationsMarkers[i].Marks[j].FileNumber << "/"
         << this->AdaptationsMarkers[i].Marks[j].Offset << std::endl;
      }
    }
  os << std::endl;

  os << "State section marks:" << std::endl;
  for ( i = 0; i < (int) this->TimeStepMarks.size(); ++i )
    {
    os << i << ": "
       << this->TimeStepMarks[i].FileNumber << "/"
       << this->TimeStepMarks[i].Offset
       << std::endl;
    }
}

//-----------------------------------------------------------------------------
void LSDynaFamily::CloseFileHandles()
{
  if (!VTK_LSDYNA_ISBADFILE(this->FD) && !this->FileHandlesClosed)
    {
    VTK_LSDYNA_CLOSEFILE(this->FD);
    this->FD = VTK_LSDYNA_BADFILE;
    this->ClearBuffer();
    this->FileHandlesClosed=true;
    }
}

//-----------------------------------------------------------------------------
void LSDynaFamily::OpenFileHandles()
{
  if (VTK_LSDYNA_ISBADFILE(this->FD) && this->FileHandlesClosed)
    {
    this->FD = VTK_LSDYNA_OPENFILE(this->Files[this->FNum].c_str());
    VTK_LSDYNA_SEEK(this->FD,this->FWord,SEEK_SET);
    this->FileHandlesClosed=false;
    }
}
