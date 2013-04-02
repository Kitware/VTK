/*=========================================================================

  Program:   Visualization Toolkit
  Module:    LSDynaFamily.h

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

// .NAME LSDynaFamily
// .SECTION Description
//    A class to abstract away I/O from families of output files.
//    This performs the actual reads and writes plus any required byte swapping.
//    Also contains a subclass, LSDynaFamilyAdaptLevel, used to store
//    file+offset
//    information for each mesh adaptation's state info.

#ifndef __LSDynaFamily_h
#define __LSDynaFamily_h

#include "vtkType.h"
#include "LSDynaExport.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//this is needs to be moved over to fseekpos and ftellpos
//in the future
#ifndef WIN32
#   include <unistd.h>
typedef off_t vtkLSDynaOff_t; // sanity
typedef int vtkLSDynaFile_t;
#  define VTK_LSDYNA_BADFILE -1
#  define VTK_LSDYNA_TELL(fid) lseek( fid, 0, SEEK_CUR )
#  define VTK_LSDYNA_SEEK(fid,off,whence) lseek( fid, off, whence )
#  define VTK_LSDYNA_SEEKTELL(fid,off,whence) lseek( fid, off, whence )
#  define VTK_LSDYNA_READ(fid,ptr,cnt) read(fid,ptr,cnt)
#  define VTK_LSDYNA_ISBADFILE(fid) (fid < 0)
#  define VTK_LSDYNA_CLOSEFILE(fid) close(fid)
#else // WIN32
typedef long vtkLSDynaOff_t; // insanity
typedef FILE* vtkLSDynaFile_t;
#  define VTK_LSDYNA_BADFILE 0
#  define VTK_LSDYNA_TELL(fid) ftell( fid )
#  define VTK_LSDYNA_SEEK(fid,off,whence) fseek( fid, off, whence )
#  define VTK_LSDYNA_SEEKTELL(fid,off,whence) fseek( fid, off, whence ), ftell( fid )
#  define VTK_LSDYNA_READ(fid,ptr,cnt) fread(ptr,1,cnt,fid)
#  define VTK_LSDYNA_ISBADFILE(fid) (fid == 0)
#  define VTK_LSDYNA_CLOSEFILE(fid) fclose(fid)
#endif
#ifdef VTKSNL_HAVE_ERRNO_H
#  include <errno.h>
#endif

class LSDynaFamily
{
public:
  LSDynaFamily();
  ~LSDynaFamily();

  struct LSDynaFamilySectionMark
    {
    vtkIdType FileNumber;
    vtkIdType Offset;
    };

  void SetDatabaseDirectory( std::string dd );
  std::string GetDatabaseDirectory();

  void SetDatabaseBaseName( std::string bn );
  std::string GetDatabaseBaseName();

  int ScanDatabaseDirectory();

  enum SectionType {
    // These are the "section" marks:
    // They are absolute (independent of current timestep).
    ControlSection=0,
    StaticSection,
    TimeStepSection,
    // These are the "subsection" marks:
    // == ControlSection has no subsections
    // == StaticSection has these "absolute" marks:
    MaterialTypeData,
    FluidMaterialIdData,
    SPHElementData,
    GeometryData,
    UserIdData,
    AdaptedParentData,
    SPHNodeData,
    RigidSurfaceData,
    EndOfStaticSection,
    // == TimeStepSection has these marks, relative to timestep 0 (so they are
    //    not valid for an arbitrary timestep, but may easily be used to compute
    //    an offset for any time step by adding a multiple of the state size):
    ElementDeletionState,
    SPHNodeState,
    RigidSurfaceState,
    // THIS MUST BE LAST
    NumberOfSectionTypes
  };

  class LSDynaFamilyAdaptLevel
  {
  public:
    LSDynaFamilySectionMark Marks[NumberOfSectionTypes];

    LSDynaFamilyAdaptLevel()
      {
      LSDynaFamilySectionMark mark;
      mark.FileNumber = 0;
      mark.Offset = 0;
      for ( int i=0; i<LSDynaFamily::NumberOfSectionTypes; ++i )
        {
        this->Marks[i] = mark;
        }
      }
  };

  static const char* SectionTypeNames[];

  enum WordType {
    Char,
    Float,
    Int
  };

  static const float EOFMarker;
  static const char* SectionTypeToString( SectionType s );

  int SkipToWord( SectionType sType, vtkIdType sId, vtkIdType wordNumber );
  int MarkTimeStep();
  int SkipWords( vtkIdType numWords );
  int BufferChunk( WordType wType, vtkIdType chunkSizeInWords );
  int ClearBuffer();

  //Description:
  //Setup reading of a number of words to be split across multiple
  //bufferChunk. This is used to read really large buffer sections
  //in more reasonable sizes. The paramters are used to specify the total buffer
  //size. The buffer size will always be evenly divisable by numComps and total
  //word size of all buffers will be numTuples*numComps
  vtkIdType InitPartialChunkBuffering(const vtkIdType& numTuples, const vtkIdType& numComps );
  vtkIdType GetNextChunk( const WordType& wType);

  inline char* GetNextWordAsChars();
  inline double GetNextWordAsFloat();
  inline vtkIdType GetNextWordAsInt();

  //Get the raw chunk buffer as a buffer of type T
  template<typename T>
  T* GetBufferAs();

  // Not needed (yet):
  // void GetCurrentWord( SectionType& stype, vtkIdType& sId, vtkIdType& wN );
  int AdvanceFile();
  void MarkSectionStart( int adapteLevel, SectionType m );

  int JumpToMark( SectionType m );
  int DetermineStorageModel();

  void SetStateSize( vtkIdType sz );
  vtkIdType GetStateSize() const;

  vtkIdType GetNumberOfFiles();
  std::string GetFileName( int i );
  vtkIdType GetFileSize( int i );

  int GetCurrentAdaptLevel() const { return this->FAdapt; }
  int TimeAdaptLevel( int i ) const { return this->TimeAdaptLevels[i]; }

  vtkIdType GetCurrentFWord() const { return this->FWord; }

  int GetWordSize() const;
  // Reset erases all information about the current database.
  // It does not free memory allocated for the current chunk.
  void Reset();

  /// Print all adaptation and time step marker information.
  void DumpMarks( std::ostream& os );

  //Closes the current file descripter. This is called after
  //we are done reading in request data
  void CloseFileHandles();

  void OpenFileHandles();

protected:
  /// The directory containing d3plot files
  std::string DatabaseDirectory;
  /// The name (title string) of the database. This is the first 10 words
  /// (40 or 80 bytes) of the first file.
  std::string DatabaseBaseName;
  /// The list of files that make up the database.
  std::vector<std::string> Files;
  /// The size of each file in the database. Note that they can be padded,
  /// so this is >= the amount of data in each file.
  std::vector<vtkIdType> FileSizes;
  /// The adaptation level associated with each file.
  std::vector<int> FileAdaptLevels;
  /// Which files mark the start of a new mesh adaptation. There is at
  /// least one entry and the first entry is always 0.
  std::vector<int> Adaptations;
  /// The currently open file descriptor
  vtkLSDynaFile_t FD;
  /// The index of currently open file descriptor into list of files
  vtkIdType FNum;
  /// The current adaptation level. This is checked whenever a file is
  /// advanced so we can skip its control+geometry headers.
  int FAdapt;
  /// The offset of Chunk in currently open file
  vtkIdType FWord;
  /// A comprehensive list of all time values across all files (and mesh
  /// adaptations)
  //std::vector<double> TimeValues;
  /// The current timestep
  vtkIdType TimeStep;
  /// Whether files are reverse endian-ness of architecture
  int SwapEndian;
  /// Whether words are 4 or 8 bytes
  int WordSize;
  /// How many words is a timestep on disk?
  vtkIdType StateSize;
  /// A vector of arrays of offsets to various header information sections
  /// (that do not vary with timestep), one for each mesh adaptation.
  std::vector<LSDynaFamilyAdaptLevel> AdaptationsMarkers;
  /// An array of bookmarks pointing to the start of state information for
  /// each timestep.
  std::vector<LSDynaFamilySectionMark> TimeStepMarks;
  /// The adaptation level associated with each time step.
  std::vector<int> TimeAdaptLevels;
  /// A buffer containing file contents of file FNum starting with word FWord.
  unsigned char* Chunk;
  /// A pointer to the next word in Chunk that will be returned when the
  /// reader requests a word.
  vtkIdType ChunkWord;
  // How much of the the allocated space is filled with valid data (assert
  // ChunkValid <= ChunkAlloc).
  vtkIdType ChunkValid;
  /// The allocated size (in words) of Chunk.
  vtkIdType ChunkAlloc;

  bool FileHandlesClosed;
  struct BufferingInfo;
  BufferingInfo* BufferInfo;
};

//-----------------------------------------------------------------------------
inline char* LSDynaFamily::GetNextWordAsChars()
  {
  if ( this->ChunkWord >= this->ChunkValid ) fprintf( stderr, "Read char past end of buffer\n" );
  return (char*) (&this->Chunk[ (this->ChunkWord++)*this->WordSize ]);
  }

//-----------------------------------------------------------------------------
inline double LSDynaFamily::GetNextWordAsFloat()
  {
  if ( this->ChunkWord >= this->ChunkValid ) fprintf( stderr, "Read float past end of buffer\n" );
  switch (this->WordSize)
    {
  case 4:
    {
    vtkTypeFloat32 value;
    memcpy(&value, &this->Chunk[ this->ChunkWord++ << 2 ], sizeof(value));
    return value;
    }
  case 8:
  default:
    {
    vtkTypeFloat64 value;
    memcpy(&value, &this->Chunk[ this->ChunkWord++ << 3 ], sizeof(value));
    return value;
    }
    }
  }

//-----------------------------------------------------------------------------
inline vtkIdType LSDynaFamily::GetNextWordAsInt()
  {
  if ( this->ChunkWord >= this->ChunkValid )
    {
    fprintf( stderr, "Read int past end of buffer\n" );
    }
  switch (this->WordSize)
    {
  case 4:
    {
    vtkTypeInt32 value;
    memcpy(&value, &this->Chunk[ this->ChunkWord++ << 2 ], sizeof(value));
    return value;
    }
  case 8:
  default:
    {
    vtkIdType value;
    memcpy(&value, &this->Chunk[ this->ChunkWord++ << 3 ], sizeof(value));
    return value;
    }
    }
  }

//-----------------------------------------------------------------------------
template<typename T>
inline T* LSDynaFamily::GetBufferAs()
{
  return (T*)this->Chunk;
}


#endif // __LSDynaFamily_h
