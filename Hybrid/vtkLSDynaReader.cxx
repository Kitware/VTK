/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLSDynaReader.cxx

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

// NOTE TO DEVELOPERS: ========================================================
//
// This is a really big file.
// It contains several private classes:
// - vtkLSDynaFamily:
//    A class to abstract away I/O from families of output files.
//    This performs the actual reads and writes plus any required byte swapping.
//    Also contains a subclass, vtkLSDynaFamilyAdaptLevel, used to store 
//    file+offset
//    information for each mesh adaptation's state info.
// - vtkLSDynaReaderPrivate:
//    A class to hold metadata about a particular file (such as time steps, 
//    the start of state information for each time step, the number of 
//    adaptive remeshes, and the large collection of constants that determine 
//    the available attributes). It contains an vtkLSDynaFamily instance.
// - vtkLSDynaReaderXMLParser:
//    A class to parse XML summary files containing part names and their IDs.
//    This class is used by vtkLSDynaReader::ReadInputDeckXML().
// - vtkLSDynaReader:
//    The implementation of the actual public VTK interface.
// These classes are preceded by some file-static constants and utility routines.

#include <vtkConfigure.h>
#include "vtkLSDynaReader.h"

#include "vtksys/SystemTools.hxx"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
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
#else // WIN32
#  include <stdio.h>
typedef long vtkLSDynaOff_t; // insanity
typedef FILE* vtkLSDynaFile_t;
#  define VTK_LSDYNA_BADFILE 0
#  define VTK_LSDYNA_TELL(fid) ftell( fid )
#  define VTK_LSDYNA_SEEK(fid,off,whence) fseek( fid, off, whence )
#  define VTK_LSDYNA_SEEKTELL(fid,off,whence) fseek( fid, off, whence ), ftell( fid )
#  define VTK_LSDYNA_READ(fid,ptr,cnt) fread(ptr,1,cnt,fid)
#  define VTK_LSDYNA_ISBADFILE(fid) (fid == 0)
#endif
#ifdef VTKSNL_HAVE_ERRNO_H
#  include <errno.h>
#endif

#include <vtkstd/string>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/algorithm>
#include <vtkstd/map>

#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkInformation.h>
#include <vtkInformationDoubleVectorKey.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiThreshold.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkSystemIncludes.h>
#include <vtkThreshold.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLParser.h>

// Define this to print out information on the structure of the reader's output
#undef VTK_LSDYNA_DBG_MULTIBLOCK

#ifdef VTK_LSDYNA_DBG_MULTIBLOCK
#include <vtkMultiGroupDataInformation.h>
#endif // VTK_LSDYNA_DBG_MULTIBLOCK

vtkStandardNewMacro(vtkLSDynaReader);

// Names of vtkDataArrays provided with grid:
#define LS_ARRAYNAME_USERID             "UserID"
#define LS_ARRAYNAME_MATERIAL           "Material"
#define LS_ARRAYNAME_DEATH              "Death"
#define LS_ARRAYNAME_SPECIES_BLNK       "SpeciesXX"
#define LS_ARRAYNAME_SPECIES_FMT        "Species%02d"
#define LS_ARRAYNAME_SPECIES_01         "Species01"
#define LS_ARRAYNAME_SPECIES_02         "Species02"
#define LS_ARRAYNAME_SPECIES_03         "Species03"
#define LS_ARRAYNAME_SPECIES_04         "Species04"
#define LS_ARRAYNAME_SPECIES_05         "Species05"
#define LS_ARRAYNAME_SPECIES_06         "Species06"
#define LS_ARRAYNAME_SPECIES_07         "Species07"
#define LS_ARRAYNAME_SPECIES_08         "Species08"
#define LS_ARRAYNAME_SPECIES_09         "Species09"
#define LS_ARRAYNAME_SPECIES_10         "Species10"
#define LS_ARRAYNAME_TEMPERATURE        "Temperature"
#define LS_ARRAYNAME_DEFLECTION         "Deflection"
#define LS_ARRAYNAME_VELOCITY           "Velocity"
#define LS_ARRAYNAME_ACCELERATION       "Acceleration"
#define LS_ARRAYNAME_PRESSURE           "Pressure"
#define LS_ARRAYNAME_VORTICITY          "Vorticity"
#define LS_ARRAYNAME_RESULTANTVORTICITY "ResVorticity"
#define LS_ARRAYNAME_ENSTROPHY          "Enstrophy"
#define LS_ARRAYNAME_HELICITY           "Helicity"
#define LS_ARRAYNAME_STREAMFUNCTION     "StreamFunc"
#define LS_ARRAYNAME_ENTHALPY           "Enthalpy" 
#define LS_ARRAYNAME_DENSITY            "Density"
#define LS_ARRAYNAME_TURBULENTKE        "TurbulentKE"
#define LS_ARRAYNAME_DISSIPATION        "Dissipation"
#define LS_ARRAYNAME_EDDYVISCOSITY      "EddyVisc"
#define LS_ARRAYNAME_RADIUSOFINFLUENCE  "InfluenceRadius"
#define LS_ARRAYNAME_NUMNEIGHBORS       "NumberOfNeighbors"
#define LS_ARRAYNAME_SEGMENTID          "SegmentID"
#define LS_ARRAYNAME_STRAIN             "Strain"
#define LS_ARRAYNAME_STRESS             "Stress"
#define LS_ARRAYNAME_EPSTRAIN           "EffPlastStrn"
#define LS_ARRAYNAME_INTEGRATIONPOINT   "IntPtData"
#define LS_ARRAYNAME_RESULTANTS         "Resultants"
#define LS_ARRAYNAME_ELEMENTMISC        "ElementMisc"
#define LS_ARRAYNAME_INTERNALENERGY     "InternalEnergy"
#define LS_ARRAYNAME_AXIALFORCE         "AxialForce"
#define LS_ARRAYNAME_SHEARRESULTANT     "ShearResultant"
#define LS_ARRAYNAME_BENDINGRESULTANT   "BendingResultant"
#define LS_ARRAYNAME_TORSIONRESULTANT   "TorsionResultant"
#define LS_ARRAYNAME_NORMALRESULTANT    "NormalResultant"
#define LS_ARRAYNAME_AXIALSTRAIN        "AxialStrain"
#define LS_ARRAYNAME_AXIALSTRESS        "AxialStress"
#define LS_ARRAYNAME_SHEARSTRAIN        "ShearStrain"
#define LS_ARRAYNAME_SHEARSTRESS        "ShearStress"
#define LS_ARRAYNAME_PLASTICSTRAIN      "PlasticStrain"
#define LS_ARRAYNAME_THICKNESS          "Thickness"

// Possible material deletion options
#define LS_MDLOPT_NONE 0
#define LS_MDLOPT_POINT 1
#define LS_MDLOPT_CELL 2

#ifdef VTK_LSDYNA_DBG_MULTIBLOCK
static void vtkDebugMultiBlockStructure( vtkIndent indent, vtkMultiGroupDataSet* mbds );
#endif // VTK_LSDYNA_DBG_MULTIBLOCK

static const char* vtkLSDynaCellTypes[] =
{
  "Point",
  "Beam",
  "Shell",
  "Thick Shell",
  "Solid",
  "Rigid Body",
  "Road Surface"
};

static void vtkLSGetLine( ifstream& deck, vtkstd::string& line )
{
#if !defined(_WIN32) && !defined(WIN32) && !defined(_MSC_VER) && !defined(__BORLANDC__)
  // One line implementation for everyone but Windows (MSVC6 and BCC32 are the troublemakers):
  vtkstd::getline( deck, line, '\n' );
#else
  // Feed Windows its food cut up into little pieces
  int linechar;
  line = "";
  while ( deck.good() )
    {
    linechar = deck.get();
    if ( linechar == '\r' || linechar == '\n' )
      return;
    line += linechar;
    }
#endif // !defined(_WIN32) || !defined(_MSC_VER) || (_MSC_VER < 1200) || (_MSC_VER >= 1300)
}

// Read in lines until one that's
// - not empty, and
// - not a comment
// is encountered. Return with that text stored in \a line.
// If an error or EOF is hit, return 0. Otherwise, return 1.
static int vtkLSNextSignificantLine( ifstream& deck, vtkstd::string& line )
{
  while ( deck.good() )
    {
    vtkLSGetLine( deck, line );
    if ( ! line.empty() && line[0] != '$' )
      {
      return 1;
      }
    }
  return 0;
}

static void vtkLSTrimWhitespace( vtkstd::string& line )
{
  vtkstd::string::size_type llen = line.length();
  while ( llen &&
    ( line[llen - 1] == ' ' ||
      line[llen - 1] == '\t' ||
      line[llen - 1] == '\r' ||
      line[llen - 1] == '\n' ) )
    {
    --llen;
    }

  vtkstd::string::size_type nameStart = 0;
  while ( nameStart < llen &&
    ( line[nameStart] == ' ' ||
      line[nameStart] == '\t' ) )
    {
    ++nameStart;
    }

  line = line.substr( nameStart, llen - nameStart );
}

static void vtkLSDowncaseFirstWord( vtkstd::string& downcased, const vtkstd::string& line )
{
  vtkstd::string::size_type i;
  vtkstd::string::value_type chr;
  int leadingSpace = 0;
  downcased = "";
  for ( i = 0; i < line.length(); ++i )
    {
    chr = tolower( line[i] );
    if ( chr == ' ' || chr == '\t' )
      {
      if ( leadingSpace )
        { // We've trimmed leading whitespace already, so we're done with the word.
        return;
        }
      }
    else
      {
      leadingSpace = 1;
      if ( chr == ',' )
        { // We're at a separator (other than whitespace). No need to continue.
        return;
        }
      }
    downcased += chr;
    }
}

void vtkLSSplitString( vtkstd::string& input, vtkstd::vector<vtkstd::string>& splits, const char* separators )
{
  vtkstd::string::size_type posBeg = 0;
  vtkstd::string::size_type posEnd;
  do {
    posEnd = input.find_first_of( separators, posBeg );
    if ( posEnd > posBeg )
      {
      // don't include empty entries in splits.
      // NOTE: This means ",comp,1, ,3" with separators ", " yields "comp","1","3", not "","comp","1","","","3".
      splits.push_back( input.substr( posBeg, posEnd - posBeg ) );
      }
    posBeg = input.find_first_not_of( separators, posEnd );
  } while ( posBeg != vtkstd::string::npos );
}

vtkstd::string vtkLSGetFamilyFileName( const char* basedir, const vtkstd::string& dbname, int adaptationLvl, int number )
{
  vtkstd::string blorb;

  blorb = basedir + dbname;

  if ( adaptationLvl > 0 )
    {
    // convert adaptationLvl from an integer to "aa", "ab", "ac", ...
    // and tack it onto the end of our blorb.
    vtkstd::string slvl;
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
    vtkstd::reverse( slvl.begin(), slvl.end() );
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

struct vtkLSDynaFamilySectionMark
{
  vtkIdType FileNumber;
  vtkIdType Offset;
};

//******************************************************************
//******************************************************************
//******************************************************************

class vtkLSDynaFamily
{
public:
  vtkLSDynaFamily();
  ~vtkLSDynaFamily();

  void SetDatabaseDirectory( vtkstd::string dd );
  vtkstd::string GetDatabaseDirectory();

  void SetDatabaseBaseName( vtkstd::string bn );
  vtkstd::string GetDatabaseBaseName();

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

  class vtkLSDynaFamilyAdaptLevel
  {
  public:
    vtkLSDynaFamilySectionMark Marks[NumberOfSectionTypes];

    vtkLSDynaFamilyAdaptLevel()
      {
      vtkLSDynaFamilySectionMark mark;
      mark.FileNumber = 0;
      mark.Offset = 0;
      for ( int i=0; i<vtkLSDynaFamily::NumberOfSectionTypes; ++i )
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

  inline char* GetNextWordAsChars();
  inline double GetNextWordAsFloat();
  inline vtkIdType GetNextWordAsInt();

  // Not needed (yet):
  // void GetCurrentWord( SectionType& stype, vtkIdType& sId, vtkIdType& wN );
  int AdvanceFile();
  void MarkSectionStart( int adapteLevel, SectionType m );

  int JumpToMark( SectionType m );
  int DetermineStorageModel();

  void SetStateSize( vtkIdType sz );
  vtkIdType GetStateSize() const;

  vtkIdType GetNumberOfFiles();
  vtkstd::string GetFileName( int i );

  int GetCurrentAdaptLevel() const { return this->FAdapt; }
  int TimeAdaptLevel( int i ) const { return this->TimeAdaptLevels[i]; }

  /// FIXME: Remove this when done debugging.
  vtkIdType GetCurrentFWord() const { return this->FWord; }

  int GetWordSize() const;
  // Reset erases all information about the current database.
  // It does not free memory allocated for the current chunk.
  void Reset();

  /// Print all adaptation and time step marker information.
  void DumpMarks( ostream& os );

protected:
  /// The directory containing d3plot files
  vtkstd::string DatabaseDirectory;
  /// The name (title string) of the database. This is the first 10 words
  /// (40 or 80 bytes) of the first file.
  vtkstd::string DatabaseBaseName;
  /// The list of files that make up the database.
  vtkstd::vector<vtkstd::string> Files;
  /// The size of each file in the database. Note that they can be padded,
  /// so this is >= the amount of data in each file.
  vtkstd::vector<vtkLSDynaOff_t> FileSizes;
  /// The adaptation level associated with each file.
  vtkstd::vector<int> FileAdaptLevels;
  /// Which files mark the start of a new mesh adaptation. There is at
  /// least one entry and the first entry is always 0.
  vtkstd::vector<int> Adaptations;
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
  //vtkstd::vector<double> TimeValues;
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
  vtkstd::vector<vtkLSDynaFamilyAdaptLevel> AdaptationsMarkers;
  /// An array of bookmarks pointing to the start of state information for
  /// each timestep.
  vtkstd::vector<vtkLSDynaFamilySectionMark> TimeStepMarks;
  /// The adaptation level associated with each time step.
  vtkstd::vector<int> TimeAdaptLevels;
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
};

const char* vtkLSDynaFamily::SectionTypeNames[] =
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

const float vtkLSDynaFamily::EOFMarker = -999999.0f;

vtkLSDynaFamily::vtkLSDynaFamily()
    {
    this->FD = VTK_LSDYNA_BADFILE; // No file open
    this->FAdapt = -1; // Invalid adaptation
    this->FNum = -1; // No files in filelist
    this->FWord = 0; // At start of file

    this->SwapEndian = -1; // No endian-ness determined
    this->WordSize = 0; // No word size determined

    this->TimeStep = 0; // Initial time step
    this->StateSize = 0; // Time steps take up no room on disk

    this->AdaptationsMarkers.push_back( vtkLSDynaFamilyAdaptLevel() );
    this->Chunk = 0;
    this->ChunkWord = 0;
    this->ChunkAlloc = 0;
    }

vtkLSDynaFamily::~vtkLSDynaFamily()
    {
    if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
      {
#ifndef WIN32
      close( this->FD );
#else
      fclose( this->FD );
#endif // WIN32
      }

    if ( this->Chunk )
      {
      delete [] this->Chunk;
      }
    }

void vtkLSDynaFamily::SetDatabaseDirectory( vtkstd::string dd )
  {
  this->DatabaseDirectory = dd;
  }
vtkstd::string vtkLSDynaFamily::GetDatabaseDirectory()
  {
  return this->DatabaseDirectory;
  }

void vtkLSDynaFamily::SetDatabaseBaseName( vtkstd::string bn )
  {
  this->DatabaseBaseName = bn;
  }
vtkstd::string vtkLSDynaFamily::GetDatabaseBaseName()
  {
  return this->DatabaseBaseName;
  }

int vtkLSDynaFamily::ScanDatabaseDirectory()
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

  vtkstd::string tmpFile;
  int filenum = 0;
  int adaptLevel = 0;
  int tryAdapt = 0; // don't try an adaptive step unless we have one good file at the current level.
  bool adapted = true; // true when advancing over a mesh adaptation.
  struct stat st;
  while ( tryAdapt >= 0 )
    {
    tmpFile = vtkLSGetFamilyFileName( this->DatabaseDirectory.c_str(),
                                      this->DatabaseBaseName,
                                      adaptLevel,
                                      filenum );
    if ( stat( tmpFile.c_str(), &st ) == 0 )
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

const char* vtkLSDynaFamily::SectionTypeToString( SectionType s )
  {
  return SectionTypeNames[s];
  }


int vtkLSDynaFamily::SkipToWord( SectionType sType, vtkIdType sId, vtkIdType wordNumber )
  {
  vtkLSDynaFamilySectionMark mark;
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
#ifndef WIN32
        close( this->FD );
#else
        fclose( this->FD );
#endif // WIN32
        }
      }
#ifndef WIN32
    this->FD = open( this->Files[ mark.FileNumber ].c_str(), O_RDONLY );
#else
    this->FD = fopen( this->Files[ mark.FileNumber ].c_str(), "rb" );
#endif // WIN32
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
int vtkLSDynaFamily::MarkTimeStep()
  {
  vtkLSDynaFamilySectionMark mark;
  mark.FileNumber = this->FNum;
  mark.Offset = VTK_LSDYNA_TELL( this->FD ) / this->GetWordSize() - 1;
  this->TimeStepMarks.push_back( mark );
  this->TimeAdaptLevels.push_back( this->FAdapt );
  return 0;
  }

// FIXME: Assumes you never skip past EOF
int vtkLSDynaFamily::SkipWords( vtkIdType numWords )
  {
  if ( this->FNum < 0 || VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    return -1;
    }
  vtkIdType offset = numWords*this->WordSize;
  if ( VTK_LSDYNA_SEEKTELL(this->FD,offset,SEEK_CUR) != offset )
    {
    return errno;
    }
  this->FWord = VTK_LSDYNA_TELL(this->FD);
  return 0;
  }

int vtkLSDynaFamily::BufferChunk( WordType wType, vtkIdType chunkSizeInWords )
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
      if ( bytesRead <= 0 )
        { // try advancing to next file
#ifndef WIN32
        close( this->FD );
#else
        fclose( this->FD );
#endif // WIN32
        if ( ++this->FNum == (vtkIdType) this->Files.size() )
          { // no more files to read. Oops.
          this->FNum = -1;
          this->FAdapt = -1;
          return 1;
          }
#ifndef WIN32
        this->FD = open( this->Files[ this->FNum ].c_str(), O_RDONLY );
#else
        this->FD = fopen( this->Files[ this->FNum ].c_str(), "rb" );
#endif // WIN32
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

  if ( this->SwapEndian && wType != vtkLSDynaFamily::Char )
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

inline char* vtkLSDynaFamily::GetNextWordAsChars()
  {
  if ( this->ChunkWord >= this->ChunkValid ) fprintf( stderr, "Read char past end of buffer\n" );
  return (char*) (&this->Chunk[ (this->ChunkWord++)*this->WordSize ]);
  }
inline double vtkLSDynaFamily::GetNextWordAsFloat()
  {
  if ( this->ChunkWord >= this->ChunkValid ) fprintf( stderr, "Read float past end of buffer\n" );
  switch (this->WordSize)
    {
  case 4:
    return double( *(float*)(&this->Chunk[ this->ChunkWord++ << 2 ]) );
  case 8:
  default:
    return *(double*)(&this->Chunk[ this->ChunkWord++ << 3 ]);
    }
  }
inline vtkIdType vtkLSDynaFamily::GetNextWordAsInt()
  {
  if ( this->ChunkWord >= this->ChunkValid )
    {
    fprintf( stderr, "Read int past end of buffer\n" );
    }
  switch (this->WordSize)
    {
  case 4:
    return vtkIdType( *(int*)(&this->Chunk[ this->ChunkWord++ << 2 ]) );
  case 8:
  default:
    return *(vtkIdType*)(&this->Chunk[ this->ChunkWord++ << 3 ]);
    }
  }

int vtkLSDynaFamily::AdvanceFile()
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
#ifndef WIN32
    close( this->FD );
#else
    fclose( this->FD );
#endif // WIN32
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
#ifndef WIN32
  this->FD = open( this->Files[ this->FNum ].c_str(), O_RDONLY );
#else
  this->FD = fopen( this->Files[ this->FNum ].c_str(), "rb" );
#endif // WIN32
  if ( VTK_LSDYNA_ISBADFILE(this->FD) )
    {
    return errno;
    }
  return 0;
  }

void vtkLSDynaFamily::MarkSectionStart( int adaptLevel, SectionType m )
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
  vtkLSDynaFamilySectionMark mark;

  mark.FileNumber = this->FNum;
  mark.Offset = myWord;
  while ( adaptLevel >= (int) this->AdaptationsMarkers.size() )
    {
    this->AdaptationsMarkers.push_back( vtkLSDynaFamilyAdaptLevel() );
    }
  this->AdaptationsMarkers[adaptLevel].Marks[m] = mark;

  //fprintf( stderr, "Mark \"%s\" is (%d,%d)\n", SectionTypeToString(m), mark.FileNumber, mark.Offset );
  }

int vtkLSDynaFamily::JumpToMark( SectionType m )
  {
  return this->SkipToWord( m, this->TimeStep, 0 );
  }

int vtkLSDynaFamily::DetermineStorageModel()
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
#ifndef WIN32
  close( this->FD );
#else
  fclose( this->FD );
#endif // WIN32
  this->FNum = -1;
  this->FAdapt = -1;
  return 1;
  }

void vtkLSDynaFamily::SetStateSize( vtkIdType sz )
  {
  this->StateSize = sz;
  }

vtkIdType vtkLSDynaFamily::GetStateSize() const
  {
  return this->StateSize;
  }

vtkIdType vtkLSDynaFamily::GetNumberOfFiles()
  {
  return this->Files.size();
  }

vtkstd::string vtkLSDynaFamily::GetFileName( int i )
  {
  return this->Files[i];
  }

int vtkLSDynaFamily::GetWordSize() const
  {
  return this->WordSize;
  }

// Reset erases all information about the current database.
// It does not free memory allocated for the current chunk.
void vtkLSDynaFamily::Reset()
{
  if ( ! VTK_LSDYNA_ISBADFILE(this->FD) )
    {
#ifndef WIN32
    close( this->FD );
#else
    fclose( this->FD );
#endif // WIN32
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
}

void vtkLSDynaFamily::DumpMarks( ostream& os )
{
  int i, j;

  os << "Files: " << endl;
  for ( i = 0; i < (int) this->Files.size(); ++i )
    {
    os << i << ": " << this->Files[i]
       << " [" << this->FileAdaptLevels[i] << "] "
       << this->FileSizes[i] << endl;
    }
  os << endl;

  os << "Adaptation levels:" << endl;
  for ( i = 0; i < (int) this->Adaptations.size(); ++i )
    {
    os << this->Adaptations[i] << ":" << endl;
    for ( j = 0; j < vtkLSDynaFamily::NumberOfSectionTypes; ++j )
      {
      os << "  " << vtkLSDynaFamily::SectionTypeToString( (vtkLSDynaFamily::SectionType)j ) << " = "
         << this->AdaptationsMarkers[i].Marks[j].FileNumber << "/"
         << this->AdaptationsMarkers[i].Marks[j].Offset << endl;
      }
    }
  os << endl;

  os << "State section marks:" << endl;
  for ( i = 0; i < (int) this->TimeStepMarks.size(); ++i )
    {
    os << i << ": "
       << this->TimeStepMarks[i].FileNumber << "/"
       << this->TimeStepMarks[i].Offset
       << endl;
    }
}

//******************************************************************
//******************************************************************
//******************************************************************

// ================================================= Private state of the reader
class vtkLSDynaReaderPrivate
{
public:
  // If this is 0, the rest of the members have undefined
  // values (although "derived-value" arrays will be
  // initialized to NULL)
  int FileIsValid;
  int FileSizeFactor; // scale factor used to compute MaxFileLength
  vtkIdType MaxFileLength; // Maximum size of any file (data too big is split into multiple files)

  vtkLSDynaFamily Fam; // file family I/O aggregator

  char  Title[41];
  int Dimensionality;
  vtkIdType CurrentState; // time step
  vtkIdType NumberOfNodes;
  vtkIdType NumberOfCells[vtkLSDynaReader::NUM_CELL_TYPES];
  int AnyDeletedCells[vtkLSDynaReader::NUM_CELL_TYPES]; // Are any cells of this type deleted in the current time step? 0=no, 1=yes
  int ReadRigidRoadMvmt; // Are some of the quads rigid? (eliminating a lot of state)
  int ConnectivityUnpacked; // Is the connectivity packed, 3 to a word?
  vtkstd::map<vtkstd::string,vtkIdType> Dict;

  /// List of material IDs that indicate the associated shell element is rigid (and has no state data)
  vtkstd::set<int> RigidMaterials;
  /// List of material IDs that indicate the associated solid element represents an Eulerian or ALE fluid.
  vtkstd::set<int> FluidMaterials;

  vtkstd::vector<vtkstd::string> PointArrayNames;
  vtkstd::vector<int> PointArrayComponents;
  vtkstd::vector<int> PointArrayStatus;

  vtkstd::map<int, vtkstd::vector<vtkstd::string> > CellArrayNames;
  vtkstd::map<int, vtkstd::vector<int> > CellArrayComponents;
  vtkstd::map<int, vtkstd::vector<int> > CellArrayStatus;

  vtkstd::vector<vtkstd::string> PartNames;
  vtkstd::vector<int> PartIds;
  vtkstd::vector<int> PartMaterials;
  vtkstd::vector<int> PartStatus;

  vtkstd::vector<int> MaterialsOrdered;
  vtkstd::vector<int> MaterialsUnordered;
  vtkstd::vector<int> MaterialsLookup;

  vtkstd::vector<vtkIdType> RigidSurfaceSegmentSizes;
  vtkstd::vector<double> TimeValues;

  // For the current time value, what file contains this state (0=d3plot,1=d3plot01, ...)
  vtkIdType FileNumberThisState;
  // For the current time value, what is the byte offset of the state in file FileNumberThisState?
  vtkIdType FileOffsetThisState;
  // Size of all data that appears before first state
  vtkIdType PreStateSize;
  // Number of bytes required to store a single timestep
  vtkIdType StateSize;

  vtkLSDynaReaderPrivate()
    {
    this->FileIsValid = 0;
    this->Dimensionality=0;
    this->NumberOfNodes=0;
    this->FileSizeFactor = 7;
    this->MaxFileLength = this->FileSizeFactor*512*512*8;

    this->Title[0] = '\0';
    this->PreStateSize = 0;
    this->StateSize = 0;
    this->CurrentState = 0;

    vtkstd::vector<vtkstd::string> blankNames;
    vtkstd::vector<int> blankNumbers;
    for ( int cellType = 0; cellType < vtkLSDynaReader::NUM_CELL_TYPES; ++cellType )
      {
      this->NumberOfCells[cellType] = 0;
      this->AnyDeletedCells[cellType] = 0;
      this->CellArrayNames[cellType] = blankNames;
      this->CellArrayComponents[cellType] = blankNumbers;
      this->CellArrayStatus[cellType] = blankNumbers;
      }
    }

  bool AddPointArray( vtkstd::string name, int numComponents, int status )
    {
    for ( unsigned i = 0; i < this->PointArrayNames.size(); ++i )
      {
      if ( this->PointArrayNames[i] == name )
        {
        if ( this->PointArrayComponents[i] != numComponents )
          {
          vtkGenericWarningMacro( "You tried to add a duplicate of point array "
            << name.c_str() << " with " << numComponents << " components instead of the original "
            << this->PointArrayComponents[i] << "!" );
          }
        return false;
        }
      }
    this->PointArrayNames.push_back( name );
    this->PointArrayComponents.push_back( numComponents );
    this->PointArrayStatus.push_back( status );

    return true;
    }

  bool AddCellArray( int cellType, vtkstd::string name, int numComponents, int status )
    {
    for ( unsigned i = 0; i < this->CellArrayNames[cellType].size(); ++i )
      {
      if ( this->CellArrayNames[cellType][i] == name )
        {
        if ( this->CellArrayComponents[cellType][i] != numComponents )
          {
          vtkGenericWarningMacro( "You tried to add a duplicate of cell array "
            << name.c_str() << " with " << numComponents << " components instead of the original "
            << this->CellArrayComponents[cellType][i] << "!" );
          }
        return false;
        }
      }
    this->CellArrayNames[cellType].push_back( name );
    this->CellArrayComponents[cellType].push_back( numComponents );
    this->CellArrayStatus[cellType].push_back( status );

    return true;
    }

  int GetTotalMaterialCount()
    {
    return
      this->Dict["NUMMAT8"] + this->Dict["NUMMATT"] + this->Dict["NUMMAT4"] +
      this->Dict["NUMMAT2"] + this->Dict["NGPSPH"] + this->Dict["NSURF"];
    // Dict["NUMMAT"] is the subset of Dict["NUMMAT4"] materials that are rigid body materials
    // FIXME: Should NSURF be in here at all? I don't have any datasets w/ NSURF > 0, so I can't test.
    }

  void Reset()
    {
    this->FileIsValid = 0;
    this->FileSizeFactor = 7;
    this->MaxFileLength = this->FileSizeFactor*512*512*8;

    this->Title[0] = '\0';
    this->PreStateSize = 0;
    this->StateSize = 0;
    this->CurrentState = 0;

    this->Dict.clear();
    this->Fam.Reset();

    this->PointArrayNames.clear();
    this->PointArrayComponents.clear();
    this->PointArrayStatus.clear();

    for ( int cellType = 0; cellType < vtkLSDynaReader::NUM_CELL_TYPES; ++cellType )
      {
      this->CellArrayNames[cellType].clear();
      this->CellArrayComponents[cellType].clear();
      this->CellArrayStatus[cellType].clear();
      this->AnyDeletedCells[cellType] = 0;
      }

    this->PartNames.clear();
    this->PartIds.clear();
    this->PartMaterials.clear();
    this->PartStatus.clear();

    this->MaterialsOrdered.clear();
    this->MaterialsUnordered.clear();
    this->MaterialsLookup.clear();

    this->RigidSurfaceSegmentSizes.clear();
    this->TimeValues.clear();
    }

  /// Dump the dictionary of Dyna keywords and their values.
  void DumpDict( ostream& os );
  /// Dump the file/offset marks, adaptation levels, and state size.
  void DumpMarks( ostream& os );
};

void vtkLSDynaReaderPrivate::DumpDict( ostream& os )
{
  os << "LSDynaReader Dictionary" << endl;
  for ( vtkstd::map<vtkstd::string,vtkIdType>::iterator it = this->Dict.begin(); it != this->Dict.end(); ++it )
    {
    os << "\t" << it->first.c_str() << ": " << it->second << endl;
    }
}

void vtkLSDynaReaderPrivate::DumpMarks( ostream& os )
{
  os << "State Size: " << this->StateSize << endl;

  this->Fam.DumpMarks( os );
}

// ============================================== End Private state of the reader

// ============================================ Start of XML Summary reader class
class vtkXMLDynaSummaryParser : public vtkXMLParser
{
public:
  static vtkXMLDynaSummaryParser* New();
  vtkTypeMacro(vtkXMLDynaSummaryParser,vtkXMLParser);

  /// Must be set before calling Parse();
  vtkLSDynaReaderPrivate* P;

protected:
  vtkstd::string PartName;
  int PartId;
  int PartStatus;
  int PartMaterial;
  int InPart;
  int InDyna;
  int InName;

  vtkXMLDynaSummaryParser()
    {
    this->P = 0;
    this->PartId = -1;
    this->InDyna = 0;
    this->InPart = 0;
    this->InName = 0;
    };
  virtual ~vtkXMLDynaSummaryParser() { };

  virtual void StartElement(const char* name, const char** atts)
    {
    int i;
    if ( ! strcmp( name, "part" ) )
      {
      if ( ! this->InDyna || this->InPart )
        { // can't have loner parts or parts that contain parts
        this->ReportUnknownElement( name );
        }
      else
        {
        this->InPart = 1;
        this->PartName = "";

        this->PartId = -1;
        this->PartStatus = 1;
        this->PartMaterial = -1;
        for ( i = 0; atts[i] != 0; i += 2 )
          {
          if ( ! strcmp( atts[i], "id" ) )
            {
            if ( sscanf( atts[i+1], "%d", &this->PartId ) <= 0 )
              {
              this->PartId = -1;
              this->ReportBadAttribute( name, atts[i], atts[i+1] );
              }
            }
          else if ( ! strcmp( atts[i], "material" ) )
            {
            if ( sscanf( atts[i+1], "%d", &this->PartMaterial ) <= 0 )
              {
              this->PartMaterial = -1;
              this->ReportBadAttribute( name, atts[i], atts[i+1] );
              }
            }
          else if ( ! strcmp( atts[i], "status" ) )
            {
            if ( sscanf( atts[i+1], "%d", &this->PartStatus ) <= 0 )
              {
              this->PartStatus = 1;
              this->ReportBadAttribute( name, atts[i], atts[i+1] );
              }
            }
          }
        if ( this->PartId < 0 )
          {
          this->ReportMissingAttribute( name, "id" );
          }
        }
      }
    else if ( ! strcmp( name, "name" ) )
      {
      if ( ! this->InDyna || ! this->InPart )
        { // name must be inside a part
        this->ReportUnknownElement( name );
        }
      else
        {
        this->InName = 1;
        this->PartName = "";
        }
      }
    else if ( ! strcmp( name, "database" ) )
      { // database must be inside the lsdyna tag, but not inside a part or name
      if ( ! this->InDyna || this->InPart || this->InName )
        {
        this->ReportUnknownElement( name );
        }
      else
        {
        const char* dbpath = 0;
        const char* dbname = 0;
        for ( i = 0; atts[i] != 0; i += 2 )
          {
          if ( ! strcmp( atts[i], "path" ) )
            {
            dbpath = atts[i+1];
            }
          else if ( ! strcmp( atts[i], "name" ) )
            {
            dbname = atts[i+1];
            }
          }
        if ( dbpath && dbname )
          {
          this->P->Fam.SetDatabaseDirectory( dbpath );
          this->P->Fam.SetDatabaseBaseName( dbname );
          }
        else
          {
          this->ReportXmlParseError();
          }
        }
      }
    else if ( ! strcmp( name, "lsdyna" ) )
      {
      if ( this->InPart || this->InName || this->InDyna )
        { // dyna must be outermost tag
        this->ReportUnknownElement( name );
        }
      else
        {
        this->InDyna = 1;
        }
      }
    }
  virtual void EndElement(const char* name)
    {
    if ( ! strcmp( name, "part" ) )
      {
      this->InPart = this->InName = 0;
      if ( this->PartName.empty() || this->PartId <= 0 || this->PartId > (int) this->P->PartNames.size() )
        { // missing a name or an id
        this->ReportXmlParseError();
        }
      else
        {
        vtkLSTrimWhitespace( this->PartName );
        this->P->PartNames[this->PartId - 1] = this->PartName;
        this->P->PartIds[this->PartId - 1] = this->PartId;
        this->P->PartMaterials[this->PartId - 1] = this->PartMaterial;
        this->P->PartStatus[this->PartId - 1] = this->PartStatus;
        }
      }
    else if ( ! strcmp( name, "name" ) )
      {
      this->InName = 0;
      }
    else if ( ! strcmp( name, "lsdyna" ) )
      {
      this->InDyna = this->InPart = this->InName = 0;
      }
    }
  virtual void CharacterDataHandler(const char* data, int length)
    {
    if ( ! this->InName )
      {
      return;
      }
    // skip leading whitespace
    int i = 0;
    while ( this->PartName.empty() && i < length && this->IsSpace( data[i] ) )
      ++i;

    if ( i < length )
      this->PartName.append( data + i, length - i );
    }
private:
  vtkXMLDynaSummaryParser( const vtkXMLDynaSummaryParser& ); // Not implemented.
  void operator = ( const vtkXMLDynaSummaryParser& ); // Not implemented.
};

vtkStandardNewMacro(vtkXMLDynaSummaryParser);
// ============================================== End of XML Summary reader class




// =================================================== Start of public interface
vtkLSDynaReader::vtkLSDynaReader()
{
  this->P = new vtkLSDynaReaderPrivate;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->DeformedMesh = 1;
  this->RemoveDeletedCells = 1;
  this->SplitByMaterialId = 0;
  this->InputDeck = 0;

  this->OutputParticles = 0;
  this->OutputBeams = 0;
  this->OutputShell = 0;
  this->OutputThickShell = 0;
  this->OutputSolid = 0;
  this->OutputRigidBody = 0;
  this->OutputRoadSurface = 0;
}

vtkLSDynaReader::~vtkLSDynaReader()
{
  this->SetInputDeck(0);
  delete this->P;
  this->P = 0;
}

void vtkLSDynaReader::PrintSelf( ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Title: \"" << this->GetTitle() << "\"" << endl;
  os << indent << "InputDeck: " << (this->InputDeck ? this->InputDeck : "(null)") << endl;
  os << indent << "DeformedMesh: " << (this->DeformedMesh ? "On" : "Off") << endl;
  os << indent << "RemoveDeletedCells: " << (this->RemoveDeletedCells ? "On" : "Off") << endl;
  os << indent << "SplitByMaterialId: " << (this->SplitByMaterialId ? "On" : "Off") << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1] << endl;

  if (this->P)
    {
    os << indent << "PrivateData: " << this->P << endl;
    }
  else
    {
    os << indent << "PrivateData: (none)" << endl;
    }

  if (this->OutputParticles)
    {
    os << indent << "OutputParticles: " << this->OutputParticles << endl;
    this->OutputParticles->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputParticles: (none)" << endl;
    }

  if (this->OutputBeams)
    {
    os << indent << "OutputBeams: " << this->OutputBeams << endl;
    this->OutputBeams->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputBeams: (none)" << endl;
    }

  if (this->OutputShell)
    {
    os << indent << "OutputShell: " << this->OutputShell << endl;
    this->OutputShell->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputShell: (none)" << endl;
    }

  if (this->OutputThickShell)
    {
    os << indent << "OutputThickShell: " << this->OutputThickShell << endl;
    this->OutputThickShell->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputThickShell: (none)" << endl;
    }

  if (this->OutputSolid)
    {
    os << indent << "OutputSolid: " << this->OutputSolid << endl;
    this->OutputSolid->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputSolid: (none)" << endl;
    }

  if (this->OutputRigidBody)
    {
    os << indent << "OutputRigidBody: " << this->OutputRigidBody << endl;
    this->OutputRigidBody->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputRigidBody: (none)" << endl;
    }

  if (this->OutputRoadSurface)
    {
    os << indent << "OutputRoadSurface: " << this->OutputRoadSurface << endl;
    this->OutputRoadSurface->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "OutputRoadSurface: (none)" << endl;
    }

  os << indent << "Dimensionality: " << this->GetDimensionality() << endl;
  os << indent << "Nodes: " << this->GetNumberOfNodes() << endl;
  os << indent << "Cells: " << this->GetNumberOfCells() << endl;

  os << indent << "PointArrays: ";
  for ( int i=0; i<this->GetNumberOfPointArrays(); ++i )
    {
    os << this->GetPointArrayName( i ) << " ";
    }
  os << endl;

  os << "CellArrays: " << endl;
  for ( int ct = 0; ct < vtkLSDynaReader::NUM_CELL_TYPES; ++ct )
    {
    os << vtkLSDynaCellTypes[ct] << ":" << endl;
    for ( int i = 0; i < this->GetNumberOfCellArrays( ct ); ++i )
      {
      os << this->GetCellArrayName( ct, i ) << " ";
      }
    os << endl;
    }
  os << endl;

  os << indent << "Time Steps: " << this->GetNumberOfTimeSteps() << endl;
  for ( int j=0; j<this->GetNumberOfTimeSteps(); ++j )
    {
    os.precision(5);
    os.width(12);
    os << this->GetTimeValue(j) ;
    if ( (j+1) % 8 == 0 && j != this->GetNumberOfTimeSteps()-1 )
      {
      os << endl << indent;
      }
    else
      {
      os << " ";
      }
    }
  os << endl;
}

void vtkLSDynaReader::Dump( ostream& os )
{
  vtkIndent indent;
  os << indent << "Title: \"" << this->GetTitle() << "\"" << endl
     << indent << "DeformedMesh: " << (this->DeformedMesh ? "On" : "Off") << endl
     << indent << "RemoveDeletedCells: " << (this->RemoveDeletedCells ? "On" : "Off") << endl
     << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1] << endl
     << indent << "PrivateData: " << this->P << endl
     << indent << "Dimensionality: " << this->GetDimensionality() << endl
     << indent << "Nodes: " << this->GetNumberOfNodes() << endl
     << indent << "Cells: " << this->GetNumberOfCells() << endl
     << indent << "PointArrays:    ";
  for ( int i=0; i<this->GetNumberOfPointArrays(); ++i )
    {
    os << this->GetPointArrayName( i ) << " ";
    }
  os << endl
     << "CellArrays:" << endl;
  for ( int ct = 0; ct < vtkLSDynaReader::NUM_CELL_TYPES; ++ct )
    {
    os << vtkLSDynaCellTypes[ct] << ":" << endl;
    for ( int i = 0; i < this->GetNumberOfCellArrays( ct ); ++i )
      {
      os << this->GetCellArrayName( ct, i ) << " ";
      }
    os << endl;
    }
  os << endl;

  os << indent << "Time Steps:       " << this->GetNumberOfTimeSteps() << endl;
  for ( int j=0; j<this->GetNumberOfTimeSteps(); ++j )
    {
    os.precision(5);
    os.width(12);
    os << this->GetTimeValue(j) ;
    if ( (j+1) % 8 == 0 && j != this->GetNumberOfTimeSteps()-1 )
      {
      os << endl << indent;
      }
    else
      {
      os << " ";
      }
    }
  os << endl;

  this->P->DumpDict( os );
  this->P->DumpMarks( os );
}

void vtkLSDynaReader::DebugDump()
{
  this->Dump( cout );
}

int vtkLSDynaReader::CanReadFile( const char* fname )
{
  if ( ! fname )
    return 0;

  vtkstd::string dbDir = vtksys::SystemTools::GetFilenamePath( fname );
  vtkstd::string dbName = vtksys::SystemTools::GetFilenameName( fname );
  vtkstd::string dbExt;
  vtkstd::string::size_type dot;
  vtkLSDynaReaderPrivate* p = new vtkLSDynaReaderPrivate;
  int result = 0;

  // GetFilenameExtension doesn't look for the rightmost "." ... do it ourselves.
  dot = dbName.rfind( '.' );
  if ( dot != vtkstd::string::npos )
    {
    dbExt = dbName.substr( dot );
    }
  else
    {
    dbExt = "";
    }

  p->Fam.SetDatabaseDirectory( dbDir );

  if ( dbExt == ".k" || dbExt == ".lsdyna" )
    {
    p->Fam.SetDatabaseBaseName( "/d3plot" );
    }
  else
    {
    struct stat st;
    if ( stat( fname, &st ) == 0 )
      {
      dbName.insert( 0, "/" );
      p->Fam.SetDatabaseBaseName( dbName.c_str() );
      }
    else
      {
      p->Fam.SetDatabaseBaseName( "/d3plot" );
      }
    }
  // If the time step is set before RequestInformation is called, we must
  // read the header information immediately in order to determine whether
  // the timestep that's been passed is valid. If it's not, we ignore it.
  if ( ! p->FileIsValid )
    {
    if ( p->Fam.GetDatabaseDirectory().empty() )
      {
      result = -1;
      }
    else
      {
      if ( p->Fam.GetDatabaseBaseName().empty() )
        {
        p->Fam.SetDatabaseBaseName( "/d3plot" ); // not a bad assumption.
        }
      p->Fam.ScanDatabaseDirectory();
      if ( p->Fam.GetNumberOfFiles() < 1 )
        {
        result = -1;
        }
      else
        {
        if ( p->Fam.DetermineStorageModel() != 0 )
          result = 0;
        else
          result = 1;
        }
      }
    }
  delete p;

  return result > 0; // -1 and 0 are both problems, 1 indicates success.
}

void vtkLSDynaReader::SetDatabaseDirectory( const char* f )
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting DatabaseDirectory to " << f );
  if ( ! f )
    {
    if ( ! this->P->Fam.GetDatabaseDirectory().empty() )
      { // no string => no database directory
      this->P->Reset();
      this->SetInputDeck( 0 );
      this->Modified();
      }
    return;
    }
  if ( strcmp(this->P->Fam.GetDatabaseDirectory().c_str(), f) )
    {
    this->P->Reset();
    this->SetInputDeck( 0 );
    this->P->Fam.SetDatabaseDirectory( vtkstd::string(f) );
    this->Modified();
    }
}

const char* vtkLSDynaReader::GetDatabaseDirectory()
{
  return this->P->Fam.GetDatabaseDirectory().c_str();
}

int vtkLSDynaReader::IsDatabaseValid()
{
  return this->P->FileIsValid;
}

void vtkLSDynaReader::SetFileName( const char* f )
{
  vtkstd::string dbDir = vtksys::SystemTools::GetFilenamePath( f );
  vtkstd::string dbName = vtksys::SystemTools::GetFilenameName( f );
  vtkstd::string dbExt;
  vtkstd::string::size_type dot;

  // GetFilenameExtension doesn't look for the rightmost "." ... do it ourselves.
  dot = dbName.rfind( '.' );
  if ( dot != vtkstd::string::npos )
    {
    dbExt = dbName.substr( dot );
    }
  else
    {
    dbExt = "";
    }

  this->SetDatabaseDirectory( dbDir.c_str() );

  if ( dbExt == ".k" || dbExt == ".lsdyna" )
    {
    this->SetInputDeck( f );
    this->P->Fam.SetDatabaseBaseName( "/d3plot" );
    }
  else
    {
    struct stat st;
    if ( stat( f, &st ) == 0 )
      {
      dbName.insert( 0, "/" );
      this->P->Fam.SetDatabaseBaseName( dbName.c_str() );
      }
    else
      {
      this->P->Fam.SetDatabaseBaseName( "/d3plot" );
      }
    }
}

const char* vtkLSDynaReader::GetFileName()
{
  // This is completely thread UNsafe. But what to do?
  static vtkstd::string filenameSurrogate;
  filenameSurrogate = this->P->Fam.GetDatabaseDirectory() + "/d3plot";
  return filenameSurrogate.c_str();
}

char* vtkLSDynaReader::GetTitle()
{
  return this->P->Title;
}

int vtkLSDynaReader::GetDimensionality()
{
  return this->P->Dimensionality;
}

void vtkLSDynaReader::SetTimeStep( vtkIdType t )
{
  vtkLSDynaReaderPrivate* p = this->P;
  if ( p->CurrentState == t )
    {
    return;
    }

  // If the time step is set before RequestInformation is called, we must
  // read the header information immediately in order to determine whether
  // the timestep that's been passed is valid. If it's not, we ignore it.
  if ( ! p->FileIsValid )
    {
    if ( p->Fam.GetDatabaseDirectory().empty() )
      {
      vtkErrorMacro( "You haven't set the LS-Dyna database directory!" );
      return;
      }

    p->Fam.SetDatabaseBaseName( "/d3plot" ); // force this for now.
    p->Fam.ScanDatabaseDirectory();
    if ( p->Fam.GetNumberOfFiles() < 1 )
      {
      p->FileIsValid = 0;
      return;
      }
    p->Fam.DetermineStorageModel();
    p->MaxFileLength = p->FileSizeFactor*512*512*p->Fam.GetWordSize();
    p->FileIsValid = 1;

    // OK, now we have a list of files. Next, determine the length of the
    // state vector (#bytes of data stored per time step):
    this->ReadHeaderInformation( 0 );

    // Finally, we can loop through and determine where all the state
    // vectors start for each time step.
    this->ScanDatabaseTimeSteps();
    }

  // Now, make sure we update the dictionary to contain information
  // relevant to the adaptation level that matches the requested timestep.
  if ( t >= 0 && t < (int) p->TimeValues.size() )
    {
    if ( p->Fam.GetCurrentAdaptLevel() != p->Fam.TimeAdaptLevel( t ) )
      {
      if ( this->ReadHeaderInformation( p->Fam.TimeAdaptLevel( t ) ) == 0 )
        {
        // unable to read the header information for the adaptation level corresponding
        // to the requested time step
        return;
        }
      }
    }

  p->CurrentState = t;
  this->Modified();
}

vtkIdType vtkLSDynaReader::GetTimeStep()
{
  return this->P->CurrentState;
}

vtkIdType vtkLSDynaReader::GetNumberOfTimeSteps()
{
  return (vtkIdType) this->P->TimeValues.size();
}

double vtkLSDynaReader::GetTimeValue( vtkIdType s )
{
  if ( s < 0 || s >= (vtkIdType) this->P->TimeValues.size() )
    {
    return -1.0;
    }

  return this->P->TimeValues[s];
}

vtkIdType vtkLSDynaReader::GetNumberOfNodes()
{
  return this->P->NumberOfNodes;
}

vtkIdType vtkLSDynaReader::GetNumberOfCells()
{
  vtkIdType tmp=0;
  for ( int c=0; c<vtkLSDynaReader::NUM_CELL_TYPES; ++c )
    {
    tmp += this->P->NumberOfCells[c];
    }
  return tmp;
}

vtkIdType vtkLSDynaReader::GetNumberOfSolidCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::SOLID];
}

vtkIdType vtkLSDynaReader::GetNumberOfThickShellCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::THICK_SHELL];
}

vtkIdType vtkLSDynaReader::GetNumberOfShellCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::SHELL];
}

vtkIdType vtkLSDynaReader::GetNumberOfRigidBodyCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::RIGID_BODY];
}

vtkIdType vtkLSDynaReader::GetNumberOfRoadSurfaceCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::ROAD_SURFACE];
}

vtkIdType vtkLSDynaReader::GetNumberOfBeamCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::BEAM];
}

vtkIdType vtkLSDynaReader::GetNumberOfParticleCells()
{
  return this->P->NumberOfCells[vtkLSDynaReader::PARTICLE];
}

vtkIdType vtkLSDynaReader::GetNumberOfContinuumCells()
{
  vtkIdType tmp=0;
  for ( int c=vtkLSDynaReader::PARTICLE+1; c<vtkLSDynaReader::NUM_CELL_TYPES; ++c )
    {
    tmp += this->P->NumberOfCells[c];
    }
  return tmp;
}

// =================================== Point array queries
int vtkLSDynaReader::GetNumberOfPointArrays()
{
  return (int) this->P->PointArrayNames.size();
}

const char* vtkLSDynaReader::GetPointArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->PointArrayNames.size() )
    return 0;

  return this->P->PointArrayNames[a].c_str();
}

int vtkLSDynaReader::GetPointArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->PointArrayStatus.size() )
    return 0;

  return this->P->PointArrayStatus[a];
}

void vtkLSDynaReader::SetPointArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->PointArrayStatus.size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->PointArrayStatus[a] )
    return;

  this->P->PointArrayStatus[a] = stat;
  this->Modified();
}

int vtkLSDynaReader::GetNumberOfComponentsInPointArray( int a )
{
  if ( a < 0 || a >= (int) this->P->PointArrayStatus.size() )
    return 0;

  return this->P->PointArrayComponents[a];
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfCellArrays( int ct )
{
  return (int) this->P->CellArrayNames[ct].size();
}

const char* vtkLSDynaReader::GetCellArrayName( int ct, int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[ct].size() )
    return 0;

  return this->P->CellArrayNames[ct][a].c_str();
}

int vtkLSDynaReader::GetCellArrayStatus( int ct, int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[ct].size() )
    return 0;

  return this->P->CellArrayStatus[ct][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInCellArray( int ct, int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[ct].size() )
    return 0;

  return this->P->CellArrayComponents[ct][a];
}

void vtkLSDynaReader::SetCellArrayStatus( int ct, int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[ct].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[ct][a] )
    return;

  this->P->CellArrayStatus[ct][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfSolidArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::SOLID].size();
}

const char* vtkLSDynaReader::GetSolidArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::SOLID].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::SOLID][a].c_str();
}

int vtkLSDynaReader::GetSolidArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::SOLID].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::SOLID][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInSolidArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::SOLID].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::SOLID][a];
}

void vtkLSDynaReader::SetSolidArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::SOLID].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::SOLID][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::SOLID][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfThickShellArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::THICK_SHELL].size();
}

const char* vtkLSDynaReader::GetThickShellArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::THICK_SHELL].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::THICK_SHELL][a].c_str();
}

int vtkLSDynaReader::GetThickShellArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::THICK_SHELL].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::THICK_SHELL][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInThickShellArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::THICK_SHELL].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::THICK_SHELL][a];
}

void vtkLSDynaReader::SetThickShellArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::THICK_SHELL].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::THICK_SHELL][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::THICK_SHELL][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfShellArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::SHELL].size();
}

const char* vtkLSDynaReader::GetShellArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::SHELL].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::SHELL][a].c_str();
}

int vtkLSDynaReader::GetShellArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::SHELL].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::SHELL][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInShellArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::SHELL].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::SHELL][a];
}

void vtkLSDynaReader::SetShellArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::SHELL].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::SHELL][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::SHELL][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfRigidBodyArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::RIGID_BODY].size();
}

const char* vtkLSDynaReader::GetRigidBodyArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::RIGID_BODY].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::RIGID_BODY][a].c_str();
}

int vtkLSDynaReader::GetRigidBodyArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::RIGID_BODY].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::RIGID_BODY][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInRigidBodyArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::RIGID_BODY].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::RIGID_BODY][a];
}

void vtkLSDynaReader::SetRigidBodyArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::RIGID_BODY].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::RIGID_BODY][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::RIGID_BODY][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfRoadSurfaceArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::ROAD_SURFACE].size();
}

const char* vtkLSDynaReader::GetRoadSurfaceArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::ROAD_SURFACE].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::ROAD_SURFACE][a].c_str();
}

int vtkLSDynaReader::GetRoadSurfaceArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::ROAD_SURFACE].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::ROAD_SURFACE][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInRoadSurfaceArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::ROAD_SURFACE].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::ROAD_SURFACE][a];
}

void vtkLSDynaReader::SetRoadSurfaceArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::ROAD_SURFACE].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::ROAD_SURFACE][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::ROAD_SURFACE][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfBeamArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::BEAM].size();
}

const char* vtkLSDynaReader::GetBeamArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::BEAM].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::BEAM][a].c_str();
}

int vtkLSDynaReader::GetBeamArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::BEAM].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::BEAM][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInBeamArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::BEAM].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::BEAM][a];
}

void vtkLSDynaReader::SetBeamArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::BEAM].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::BEAM][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::BEAM][a] = stat;
  this->Modified();
}

// =================================== Cell array queries
int vtkLSDynaReader::GetNumberOfParticleArrays()
{
  return (int) this->P->CellArrayNames[vtkLSDynaReader::PARTICLE].size();
}

const char* vtkLSDynaReader::GetParticleArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayNames[vtkLSDynaReader::PARTICLE].size() )
    return 0;

  return this->P->CellArrayNames[vtkLSDynaReader::PARTICLE][a].c_str();
}

int vtkLSDynaReader::GetParticleArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::PARTICLE].size() )
    return 0;

  return this->P->CellArrayStatus[vtkLSDynaReader::PARTICLE][a];
}

int vtkLSDynaReader::GetNumberOfComponentsInParticleArray( int a )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::PARTICLE].size() )
    return 0;

  return this->P->CellArrayComponents[vtkLSDynaReader::PARTICLE][a];
}

void vtkLSDynaReader::SetParticleArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->CellArrayStatus[vtkLSDynaReader::PARTICLE].size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->CellArrayStatus[vtkLSDynaReader::PARTICLE][a] )
    return;

  this->P->CellArrayStatus[vtkLSDynaReader::PARTICLE][a] = stat;
  this->Modified();
}

// =================================== Part queries
int vtkLSDynaReader::GetNumberOfPartArrays()
{
  return (int) this->P->PartNames.size();
}

const char* vtkLSDynaReader::GetPartArrayName( int a )
{
  if ( a < 0 || a >= (int) this->P->PartNames.size() )
    return 0;

  return this->P->PartNames[a].c_str();
}

int vtkLSDynaReader::GetPartArrayStatus( int a )
{
  if ( a < 0 || a >= (int) this->P->PartStatus.size() )
    return 0;

  return this->P->PartStatus[a];
}

void vtkLSDynaReader::SetPartArrayStatus( int a, int stat )
{
  if ( a < 0 || a >= (int) this->P->PartStatus.size() )
    {
    vtkWarningMacro( "Cannot set status of non-existent point array " << a );
    return;
    }

  if ( stat == this->P->PartStatus[a] )
    return;

  this->P->PartStatus[a] = stat;
  this->Modified();
}

// =================================== Read the control word header for the current adaptation level
int vtkLSDynaReader::ReadHeaderInformation( int curAdapt )
{
  vtkLSDynaReaderPrivate* p = this->P;

  // =================================== Control Word Section
  p->Fam.SkipToWord( vtkLSDynaFamily::ControlSection, curAdapt /*timestep*/, 0 );
  p->Fam.BufferChunk( vtkLSDynaFamily::Char, 10 );
  memcpy( p->Title, p->Fam.GetNextWordAsChars(), 40*sizeof(char) );
  p->Title[40] = '\0';

  p->Fam.SkipToWord( vtkLSDynaFamily::ControlSection, curAdapt /*timestep*/, 13 );
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 1 );
  p->Dict["Code"] = p->Fam.GetNextWordAsInt();
  p->Fam.BufferChunk( vtkLSDynaFamily::Float, 1 );
  p->Dict["Version"] = vtkIdType(p->Fam.GetNextWordAsFloat());
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 49 );
  p->Dict[    "NDIM"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NUMNP"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "ICODE"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NGLBV"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IT"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IU"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IV"] = p->Fam.GetNextWordAsInt();
  p->Dict[      "IA"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NEL8"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMAT8"] = p->Fam.GetNextWordAsInt();
  p->Fam.GetNextWordAsInt(); // BLANK
  p->Fam.GetNextWordAsInt(); // BLANK
  p->Dict[    "NV3D"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NEL2"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMAT2"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NV1D"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NEL4"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMAT4"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NV2D"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NEIPH"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NEIPS"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "MAXINT"] = p->Fam.GetNextWordAsInt();
  // do MDLOPT here?
  p->Dict[   "NMSPH"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "EDLOPT"] = p->Dict["NMSPH"]; // EDLOPT is not standard
  p->Dict[  "NGPSPH"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NARBS"] = p->Fam.GetNextWordAsInt();
  p->Dict[    "NELT"] = p->Fam.GetNextWordAsInt();
  p->Dict[ "NUMMATT"] = p->Fam.GetNextWordAsInt();
  p->Dict[   "NV3DT"] = p->Fam.GetNextWordAsInt();
  p->Dict["IOSHL(1)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict["IOSHL(2)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict["IOSHL(3)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict["IOSHL(4)"] = p->Fam.GetNextWordAsInt() == 1000 ? 1 : 0;
  p->Dict[ "IALEMAT"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "NCFDV1"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "NCFDV2"] = p->Fam.GetNextWordAsInt();
  p->Dict[  "NADAPT"] = p->Fam.GetNextWordAsInt();
  p->Fam.GetNextWordAsInt(); // BLANK

  // Compute the derived values in this->P
  // =========================================== Control Word Section Processing
  int itmp;
  vtkIdType iddtmp;
  char ctmp[128]; // temp space for generating keywords (i.e. isphfg) and attribute names (i.e., StressIntPt3)

  // --- Initialize some values
  p->ReadRigidRoadMvmt = 0;
  p->PreStateSize = 64*p->Fam.GetWordSize();
  p->StateSize = p->Fam.GetWordSize(); // Account for "time word"
  p->Dimensionality = p->Dict["NDIM"];
  switch (p->Dimensionality)
    {
  case 2:
  case 3:
    p->Dict["MATTYP"] = 0;
    p->ConnectivityUnpacked = 0;
    break;
  case 7:
    p->ReadRigidRoadMvmt = 1;
  case 5:
    p->Dict["MATTYP"] = 1;
    p->ConnectivityUnpacked = 1;
    p->Dimensionality = 3;
    break;
  case 4:
    p->ConnectivityUnpacked = 1;
    p->Dict["MATTYP"] = 0;
    p->Dimensionality = 3;
    break;
  default:
    vtkErrorMacro("Unknown Dimensionality " << p->Dimensionality << " encountered" );
    p->FileIsValid = 0;
    return 0;
    break;
    }

  // FIXME Are these marks valid since we are marking the word past the end of the chunk?
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::StaticSection );
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::MaterialTypeData );
  if ( p->Dict["MATTYP"] != 0 )
    {
    p->Fam.BufferChunk( vtkLSDynaFamily::Int, 2 );
    p->Dict["NUMRBE"] = p->Fam.GetNextWordAsInt();
    p->Dict["NUMMAT"] = p->Fam.GetNextWordAsInt();
    }
  else
    {
    p->Dict["NUMRBE"] = 0;
    p->Dict["NUMMAT"] = 0;
    }
  p->NumberOfNodes = p->Dict["NUMNP"];

  p->NumberOfCells[vtkLSDynaReader::RIGID_BODY] = p->Dict["NUMRBE"];
  p->NumberOfCells[vtkLSDynaReader::SOLID] = p->Dict["NEL8"];
  p->NumberOfCells[vtkLSDynaReader::THICK_SHELL] = p->Dict["NELT"];
  p->NumberOfCells[vtkLSDynaReader::SHELL] = p->Dict["NEL4"];
  p->NumberOfCells[vtkLSDynaReader::BEAM] = p->Dict["NEL2"];
  p->NumberOfCells[vtkLSDynaReader::PARTICLE] = p->Dict["NMSPH"];

  p->StateSize += p->Dict["NGLBV"]*p->Fam.GetWordSize();

  if ( p->Dict["IT"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_TEMPERATURE, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    }
  if ( p->Dict["IU"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_DEFLECTION, p->Dimensionality, 1 );
    p->StateSize += p->NumberOfNodes * p->Dimensionality * p->Fam.GetWordSize();
    }
  if ( p->Dict["IV"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_VELOCITY, p->Dimensionality, 1 );
    p->StateSize += p->NumberOfNodes * p->Dimensionality * p->Fam.GetWordSize();
    }
  if ( p->Dict["IA"] != 0 )
    {
    p->AddPointArray( LS_ARRAYNAME_ACCELERATION, p->Dimensionality, 1 );
    p->StateSize += p->NumberOfNodes * p->Dimensionality * p->Fam.GetWordSize();
    }
  p->Dict["cfdPressure"] = 0;
  p->Dict["cfdVort"] = 0;
  p->Dict["cfdXVort"] = 0;
  p->Dict["cfdYVort"] = 0;
  p->Dict["cfdZVort"] = 0;
  p->Dict["cfdRVort"] = 0;
  p->Dict["cfdEnstrophy"] = 0;
  p->Dict["cfdHelicity"] = 0;
  p->Dict["cfdStream"] = 0;
  p->Dict["cfdEnthalpy"] = 0;
  p->Dict["cfdDensity"] = 0;
  p->Dict["cfdTurbKE"] = 0;
  p->Dict["cfdDiss"] = 0;
  p->Dict["cfdEddyVisc"] = 0;
  itmp = p->Dict["NCFDV1"];
  if ( itmp & 2 )
    {
    p->AddPointArray( LS_ARRAYNAME_PRESSURE, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdPressure"] = 1;
    }
  if ( (itmp & 28) == 28 )
    {
    p->AddPointArray( LS_ARRAYNAME_VORTICITY, 3, 1 );
    p->StateSize += p->NumberOfNodes * 3 * p->Fam.GetWordSize();
    p->Dict["cfdVort"] = 1;
    p->Dict["cfdXVort"] = 1;
    p->Dict["cfdYVort"] = 1;
    p->Dict["cfdZVort"] = 1;
    }
  else
    { // OK, we don't have all the vector components... maybe we have some of them?
    if ( itmp & 4 )
      {
      p->AddPointArray( LS_ARRAYNAME_VORTICITY "_X", 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      p->Dict["cfdXVort"] = 1;
      }
    if ( itmp & 8 )
      {
      p->AddPointArray( LS_ARRAYNAME_VORTICITY "_Y", 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      p->Dict["cfdYVort"] = 1;
      }
    if ( itmp & 16 )
      {
      p->AddPointArray( LS_ARRAYNAME_VORTICITY "_Z", 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      p->Dict["cfdZVort"] = 1;
      }
    }
  if ( itmp & 32 )
    {
    p->AddPointArray( LS_ARRAYNAME_RESULTANTVORTICITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdRVort"] = 1;
  if ( itmp & 64 )
    {
    p->AddPointArray( LS_ARRAYNAME_ENSTROPHY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdEnstrophy"] = 1;
    }
  if ( itmp & 128 )
    {
    p->AddPointArray( LS_ARRAYNAME_HELICITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdHelicity"] = 1;
    }
  if ( itmp & 256 )
    {
    p->AddPointArray( LS_ARRAYNAME_STREAMFUNCTION, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdStream"] = 1;
    }
  if ( itmp & 512 )
    {
    p->AddPointArray( LS_ARRAYNAME_ENTHALPY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdEnthalpy"] = 1;
    }
  if ( itmp & 1024 )
    {
    p->AddPointArray( LS_ARRAYNAME_DENSITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdDensity"] = 1;
    }
  if ( itmp & 2048 )
    {
    p->AddPointArray( LS_ARRAYNAME_TURBULENTKE, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdTurbKE"] = 1;
    }
  if ( itmp & 4096 )
    {
    p->AddPointArray( LS_ARRAYNAME_DISSIPATION, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdDiss"] = 1;
    }
  if ( itmp & 1040384 )
    {
    p->AddPointArray( LS_ARRAYNAME_EDDYVISCOSITY, 1, 1 );
    p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
    p->Dict["cfdEddyVisc"] = 1;
    }
    }

  char sname[]=LS_ARRAYNAME_SPECIES_BLNK;
  iddtmp = p->Dict["NCFDV2"];
  for ( itmp=1; itmp<11; ++itmp )
    {
    if ( iddtmp & (vtkIdType)(1<<itmp) )
      {
      sprintf( sname, LS_ARRAYNAME_SPECIES_FMT, itmp );
      p->AddPointArray( sname, 1, 1 );
      p->StateSize += p->NumberOfNodes * p->Fam.GetWordSize();
      sprintf( sname, "cfdSpec%02d", itmp );
      p->Dict[ sname ] = 1;
      }
    else
      {
      sprintf( sname, "cfdSpec%02d", itmp );
      p->Dict[ sname ] = 0;
      }
    }

  int mdlopt;
  int intpts2;
  mdlopt = p->Dict["MAXINT"];
  if ( mdlopt >= 0 )
    {
    intpts2 = mdlopt;
    mdlopt = LS_MDLOPT_NONE;
    }
  else if ( mdlopt < -10000 )
    {
    intpts2 = -mdlopt -10000;
    mdlopt = LS_MDLOPT_CELL;

    // WARNING: This needs NumberOfCells[vtkLSDynaReader::RIGID_BODY] set, which relies on NUMRBE
    p->StateSize += this->GetNumberOfContinuumCells() * p->Fam.GetWordSize();
    }
  else
    {
    intpts2 = -mdlopt;
    mdlopt = LS_MDLOPT_POINT;
    p->AddPointArray( LS_ARRAYNAME_DEATH, 1, 1 );
    p->StateSize += this->GetNumberOfNodes() * p->Fam.GetWordSize();
    }
  p->Dict["MDLOPT"] = mdlopt;
  p->Dict["_MAXINT_"] = intpts2;
  if ( p->Dict["NEL4"] > 0 )
    {
    if ( p->Dict["NV2D"]-(p->Dict["_MAXINT_"]*(6*p->Dict["IOSHL(1)"]+p->Dict["IOSHL(2)"]+p->Dict["NEIPS"])+8*p->Dict["IOSHL(3)"]+4*p->Dict["IOSHL(4)"]) > 1 )
      {
      p->Dict["ISTRN"] = 1;
      }
    else
      {
      p->Dict["ISTRN"] = 0;
      }
    }
  else if ( p->Dict["NELT"] > 0 )
    {
    if ( p->Dict["NV3D"] - p->Dict["_MAXINT_"]*(6*p->Dict["IOSHL(1)"]+p->Dict["IOSHL(2)"]+p->Dict["NEIPS"]) > 1 )
      {
      p->Dict["ISTRN"] = 1;
      }
    else
      {
      p->Dict["ISTRN"] = 0;
      }
    }
  else
    {
    p->Dict["ISTRN"] = 0;
    }
  // Solid element state size   FIXME: 7 + NEIPH should really be NV3D (in case things change)
  p->StateSize += (7+p->Dict["NEIPH"])*p->NumberOfCells[vtkLSDynaReader::SOLID]*p->Fam.GetWordSize();
  // Thick shell state size
  p->StateSize += p->Dict["NV3DT"]*p->NumberOfCells[vtkLSDynaReader::THICK_SHELL]*p->Fam.GetWordSize();
  // (Thin) shell state size (we remove rigid body shell element state below)
  p->StateSize += p->Dict["NV2D"]*p->NumberOfCells[vtkLSDynaReader::SHELL]*p->Fam.GetWordSize();
  // Beam state size
  p->StateSize += p->Dict["NV1D"]*p->NumberOfCells[vtkLSDynaReader::BEAM]*p->Fam.GetWordSize();

  // OK, we are done processing the header (control) section.

  // ================================================ Static Information Section
  // This is marked above so we can read NUMRBE in time to do StateSize calculations
  // ================================================ Material Type Data Section
  // This is marked above so we can read NUMRBE in time to do StateSize calculations
  if ( p->Dict["MATTYP"] != 0 )
    {
    // If there are rigid body elements, they won't have state data and we must
    // reduce the state size
    p->StateSize -= p->Dict["NV2D"] * p->NumberOfCells[ vtkLSDynaReader::RIGID_BODY ];

    p->Fam.BufferChunk( vtkLSDynaFamily::Int, p->Dict["NUMMAT"] );
    for ( itmp = 0; itmp < p->Dict["NUMMAT"]; ++itmp )
      {
      p->RigidMaterials.insert( p->Fam.GetNextWordAsInt() );
      }
    p->PreStateSize += (2 + p->Dict["NUMMAT"])*p->Fam.GetWordSize();

    //p->Fam.SkipToWord( vtkLSDynaFamily::MaterialTypeData, curAdapt, 2 + p->Dict["NUMMAT"] );
    }

  // ============================================ Fluid Material ID Data Section
  // IALEMAT offset
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::FluidMaterialIdData );
  p->PreStateSize += p->Dict["IALEMAT"];
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, p->Dict["IALEMAT"] );
  for ( itmp = 0; itmp < p->Dict["IALEMAT"]; ++itmp )
    {
    p->FluidMaterials.insert( p->Fam.GetNextWordAsInt() );
    }
  //p->Fam.SkipToWord( vtkLSDynaFamily::FluidMaterialIdData, curAdapt, p->Dict["IALEMAT"] );

  // =======================  Smooth Particle Hydrodynamics Element Data Section
  // Only when NMSPH > 0
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::SPHElementData );
  if ( p->NumberOfCells[vtkLSDynaReader::PARTICLE] > 0 )
    {
    p->Fam.BufferChunk( vtkLSDynaFamily::Int, 1 );
    vtkIdType sphAttributes = p->Fam.GetNextWordAsInt();
    p->Dict["isphfg(1)"] = sphAttributes;
    if ( sphAttributes >= 9 )
      {
      p->Fam.BufferChunk( vtkLSDynaFamily::Int, sphAttributes - 1 ); // should be 9
      // Dyna docs call statePerParticle "NUM_SPH_DATA":
      int statePerParticle = 1; // start at 1 because we always have material ID of particle.
      for ( itmp = 2; itmp <= sphAttributes; ++itmp )
        {
        int numComponents = p->Fam.GetNextWordAsInt();
        sprintf( ctmp, "isphfg(%d)", itmp );
        p->Dict[ ctmp ] = numComponents;
        statePerParticle += numComponents;
        }
      p->Dict["NUM_SPH_DATA"] = statePerParticle;
      p->StateSize += statePerParticle * p->NumberOfCells[vtkLSDynaReader::PARTICLE] * p->Fam.GetWordSize();
      }
    else
      {
      p->FileIsValid = 0;
      return 0;
      }
    p->Fam.SkipToWord( vtkLSDynaFamily::SPHElementData, curAdapt, p->Dict["isphfg(1)"] );
    p->PreStateSize += p->Dict["isphfg(1)"]*p->Fam.GetWordSize();
    }

  // ===================================================== Geometry Data Section
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::GeometryData );
  iddtmp = this->GetNumberOfNodes()*p->Dimensionality*p->Fam.GetWordSize(); // Size of nodes on disk
  iddtmp += p->NumberOfCells[vtkLSDynaReader::SOLID]*9*p->Fam.GetWordSize(); // Size of hexes on disk
  iddtmp += p->NumberOfCells[vtkLSDynaReader::THICK_SHELL]*9*p->Fam.GetWordSize(); // Size of thick shells on disk
  iddtmp += p->NumberOfCells[vtkLSDynaReader::SHELL]*5*p->Fam.GetWordSize(); // Size of quads on disk
  iddtmp += p->NumberOfCells[vtkLSDynaReader::BEAM]*6*p->Fam.GetWordSize(); // Size of beams on disk
  p->PreStateSize += iddtmp;
  p->Fam.SkipToWord( vtkLSDynaFamily::GeometryData, curAdapt, iddtmp/p->Fam.GetWordSize() ); // Skip to end of geometry

  // =========== User Material, Node, And Element Identification Numbers Section
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::UserIdData );
  if ( p->Dict["NARBS"] != 0 )
    {
    p->Fam.BufferChunk( vtkLSDynaFamily::Int, 10 );
    p->PreStateSize += 10*p->Fam.GetWordSize();
    p->Dict["NSORT"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRH"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRB"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRS"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRT"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSORTD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRHD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRBD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRSD"] = p->Fam.GetNextWordAsInt();
    p->Dict["NSRTD"] = p->Fam.GetNextWordAsInt();
    //iddtmp = (this->GetNumberOfNodes() + this->GetNumberOfContinuumCells())*p->Fam.GetWordSize();
    if ( p->Dict["NSORT"] < 0 )
      {
      p->Fam.BufferChunk( vtkLSDynaFamily::Int, 6 );
      p->PreStateSize += 6*p->Fam.GetWordSize();
      p->Dict["NSRMA"] = p->Fam.GetNextWordAsInt();
      p->Dict["NSRMU"] = p->Fam.GetNextWordAsInt();
      p->Dict["NSRMP"] = p->Fam.GetNextWordAsInt();
      p->Dict["NSRTM"] = p->Fam.GetNextWordAsInt();
      p->Dict["NUMRBS"] = p->Fam.GetNextWordAsInt();
      p->Dict["NMMAT"] = p->Fam.GetNextWordAsInt();
      iddtmp += 3*p->Dict["NMMAT"]*p->Fam.GetWordSize();
      }
    // FIXME!!! Why is NARBS larger than 10+NUMNP+NEL8+NEL2+NEL4+NELT?
    // Obviously, NARBS is definitive, but what are the extra numbers at the end?
    //iddtmp += (p->Dict["NARBS"]*p->Fam.GetWordSize() - iddtmp - 10*p->Fam.GetWordSize() - (p->Dict["NSORT"]<0 ? 6*p->Fam.GetWordSize() : 0));
    //p->PreStateSize += iddtmp;
    p->PreStateSize += p->Dict["NARBS"] * p->Fam.GetWordSize();
    // should just skip forward iddtmp bytes here, but no easy way to do that with the fam
    p->Fam.SkipToWord( vtkLSDynaFamily::UserIdData, curAdapt, p->Dict["NARBS"] );
    }
  else
    {
    p->Dict["NSORT"] = 0;
    }
  // Break from convention and read in actual array values (as opposed to just summary information)
  // about the material IDs. This is required because the reader must present part names after
  // RequestInformation is called and that cannot be done without a list of material IDs.
  this->ReadUserMaterialIds();

  // ======================================= Adapted Element Parent List Section
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::AdaptedParentData );
  p->Fam.SkipToWord( vtkLSDynaFamily::AdaptedParentData, curAdapt, 2*p->Dict["NADAPT"] );
  iddtmp = 2*p->Dict["NADAPT"]*p->Fam.GetWordSize();
  p->PreStateSize += iddtmp;

  // ============== Smooth Particle Hydrodynamics Node And Material List Section
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::SPHNodeData );
  iddtmp = 2*p->NumberOfCells[vtkLSDynaReader::PARTICLE]*p->Fam.GetWordSize();
  p->PreStateSize += iddtmp;
  p->Fam.SkipToWord( vtkLSDynaFamily::SPHNodeData, curAdapt, 2*p->NumberOfCells[vtkLSDynaReader::PARTICLE] );

  // =========================================== Rigid Road Surface Data Section
  p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::RigidSurfaceData );
  if ( p->Dict["NDIM"] > 5 )
    {
    p->Fam.BufferChunk( vtkLSDynaFamily::Int, 4 );
    p->PreStateSize += 4*p->Fam.GetWordSize();
    p->Dict["NNODE"]  = p->Fam.GetNextWordAsInt();
    p->Dict["NSEG"]   = p->Fam.GetNextWordAsInt();
    p->Dict["NSURF"]  = p->Fam.GetNextWordAsInt();
    p->Dict["MOTION"] = p->Fam.GetNextWordAsInt();
    iddtmp = 4*p->Dict["NNODE"]*p->Fam.GetWordSize();
    p->PreStateSize += iddtmp;
    p->Fam.SkipWords( 4*p->Dict["NNODE"] );

    for ( itmp = 0; itmp < p->Dict["NSURF"]; ++itmp )
      {
      p->Fam.BufferChunk( vtkLSDynaFamily::Int, 2 );
      p->Fam.GetNextWordAsInt(); // Skip SURFID
      iddtmp = p->Fam.GetNextWordAsInt(); // Read SURFNSEG[SURFID]
      p->RigidSurfaceSegmentSizes.push_back( iddtmp );
      p->PreStateSize += (2 + 4*iddtmp)*p->Fam.GetWordSize();
      p->Fam.SkipWords( 4*iddtmp );
      }

    if ( p->Dict["NSEG"] > 0 )
      {
      p->AddCellArray( vtkLSDynaReader::ROAD_SURFACE, LS_ARRAYNAME_SEGMENTID, 1, 1 );
      //FIXME: p->AddRoadPointArray( vtkLSDynaReader::ROAD_SURFACE, LS_ARRAYNAME_USERID, 1, 1 );
      }

    if ( p->Dict["MOTION"] )
      {
      p->StateSize += 6*p->Dict["NSURF"]*p->Fam.GetWordSize();
      }
    }

  //if ( curAdapt == 0 )
    {
    p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::EndOfStaticSection );
    p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::TimeStepSection );
    }
  p->Fam.SetStateSize( p->StateSize / p->Fam.GetWordSize() );


  // ==========================================================================
  // Now that we've read the header, create a list of the cell arrays for
  // each output mesh.

  // Currently, the LS-Dyna reader only gives users a few knobs to control which cell variables are loaded.
  // It is a difficult problem since many attributes only occur on some element types, there are many
  // dyna flags that conditionally omit results, and some quantities are repeated over differing numbers
  // of points for different types of cells.
  // Given the complexity, we punt by defining some knobs for "types" of data and define related fields.
  // In a perfect world, finer-grained control would be available.
  //
  // As an example: if there are any
  // - 3-D cells, OR
  // - non-rigid 2-D cells with IOSHL(1)==1, OR
  // - beam cells with NV1D > 6, OR
  // - SPH cells with isphfg(4)==6
  // then there will be stresses present

  // Every cell always has a material type
  // FIXME: Is this true? Rigid bodies may be an exception, in which 
  // case we need to check that the number of cells in the other 5 meshes sum to >0

  if ( p->Dict["NARBS"] )
    {
    p->AddPointArray( LS_ARRAYNAME_USERID, 1, 1 );
    }

  if ( p->NumberOfCells[ vtkLSDynaReader::PARTICLE ] )
    {
    p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_MATERIAL, 1, 1 );
    p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_DEATH, 1, 1 );
    if ( p->Dict["isphfg(2)"] == 1 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_RADIUSOFINFLUENCE, 1, 1 );
      }
    if ( p->Dict["isphfg(3)"] == 1 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_PRESSURE, 1, 1 );
      }
    if ( p->Dict["isphfg(4)"] == 6 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_STRESS, 6, 1 );
      }
    if ( p->Dict["isphfg(5)"] == 1 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
      }
    if ( p->Dict["isphfg(6)"] == 1 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_DENSITY, 1, 1 );
      }
    if ( p->Dict["isphfg(7)"] == 1 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_INTERNALENERGY, 1, 1 );
      }
    if ( p->Dict["isphfg(8)"] == 1 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_NUMNEIGHBORS, 1, 1 );
      }
    if ( p->Dict["isphfg(9)"] == 6 )
      {
      p->AddCellArray( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_STRAIN, 6, 1 );
      }
    }

  if ( p->NumberOfCells[ vtkLSDynaReader::BEAM ] )
    {
    p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_MATERIAL, 1, 1 );
    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
      {
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_DEATH, 1, 1 );
      }

    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_USERID, 1, 1 );
      }
    if ( p->Dict["NV1D"] >= 6 )
      {
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_AXIALFORCE, 1, 1 );
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_SHEARRESULTANT, 2, 1 );
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_BENDINGRESULTANT, 2, 1 );
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_TORSIONRESULTANT, 1, 1 );
      }
    if ( p->Dict["NV1D"] > 6 )
      {
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_SHEARSTRESS, 2, 1 );
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_AXIALSTRESS, 1, 1 );
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_AXIALSTRAIN, 1, 1 );
      p->AddCellArray( vtkLSDynaReader::BEAM, LS_ARRAYNAME_PLASTICSTRAIN, 1, 1 );
      }
    }

  if ( p->NumberOfCells[ vtkLSDynaReader::SHELL ] )
    {
    p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_MATERIAL, 1, 1 );
    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
      {
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_DEATH, 1, 1 );
      }
    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_USERID, 1, 1 );
      }
    if ( p->Dict["IOSHL(1)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_STRESS, 6, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_STRESS "InnerSurf", 6, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_STRESS "OuterSurf", 6, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["IOSHL(2)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_EPSTRAIN "InnerSurf", 1, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_EPSTRAIN "OuterSurf", 1, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, ctmp, 1, 1 );
        }
      }
    if ( p->Dict["IOSHL(3)"] )
      {
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_NORMALRESULTANT, 3, 1 );
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_SHEARRESULTANT, 2, 1 );
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_BENDINGRESULTANT, 3, 1 );
      }
    if ( p->Dict["IOSHL(4)"] )
      {
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_THICKNESS, 1, 1 );
      p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_ELEMENTMISC, 2, 1 );
      }
    if ( p->Dict["NEIPS"] )
      {
      int neips = p->Dict["NEIPS"];
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf", neips, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf", neips, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["ISTRN"] )
      {
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_STRAIN "InnerSurf", 6, 1 );
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_STRAIN "OuterSurf", 6, 1 );
      }
    if ( ! p->Dict["ISTRN"] || (p->Dict["ISTRN"] && p->Dict["NV2D"] >= 45) )
      {
        p->AddCellArray( vtkLSDynaReader::SHELL, LS_ARRAYNAME_INTERNALENERGY, 1, 1 );
      }
    }

  if ( p->NumberOfCells[ vtkLSDynaReader::THICK_SHELL ] )
    {
    p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_MATERIAL, 1, 1 );
    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
      {
      p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_DEATH, 1, 1 );
      }
    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_USERID, 1, 1 );
      }
    if ( p->Dict["IOSHL(1)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_STRESS, 6, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_STRESS "InnerSurf", 6, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_STRESS "OuterSurf", 6, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["IOSHL(2)"] )
      {
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_EPSTRAIN "InnerSurf", 1, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_EPSTRAIN "OuterSurf", 1, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, ctmp, 1, 1 );
        }
      }
    if ( p->Dict["NEIPS"] )
      {
      int neips = p->Dict["NEIPS"];
      if ( p->Dict["_MAXINT_"] >= 3 )
        {
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT, neips, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf", neips, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf", neips, 1 );
        }
      for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
        {
        sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, ctmp, 6, 1 );
        }
      }
    if ( p->Dict["ISTRN"] )
      {
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_STRAIN "InnerSurf", 6, 1 );
        p->AddCellArray( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_STRAIN "OuterSurf", 6, 1 );
      }
    }

  if ( p->NumberOfCells[ vtkLSDynaReader::SOLID ] )
    {
    p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_MATERIAL, 1, 1 );
    if ( p->Dict["MDLOPT"] == LS_MDLOPT_CELL )
      {
      p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_DEATH, 1, 1 );
      }
    if ( p->Dict["NARBS"] != 0 )
      {
      p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_USERID, 1, 1 );
      }
    p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_STRESS, 6, 1 );
    p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_EPSTRAIN, 1, 1 );
    if ( p->Dict["ISTRN"] )
      {
      p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_STRAIN, 6, 1 );
      }
    if ( p->Dict["NEIPH"] > 0 )
      {
      p->AddCellArray( vtkLSDynaReader::SOLID, LS_ARRAYNAME_INTEGRATIONPOINT, p->Dict["NEIPH"], 1 );
      }
    }

  // Only try reading the keyword file if we don't have part names. 
  if ( curAdapt == 0 && p->PartNames.size() == 0 )
    {
    this->ReadInputDeck();
    }

  return -1;
}

int vtkLSDynaReader::ScanDatabaseTimeSteps()
{
  vtkLSDynaReaderPrivate* p = this->P;

  // ======================================================= State Data Sections
  // The 2 lines below are now in ReadHeaderInformation:
  // p->Fam.MarkSectionStart( curAdapt, vtkLSDynaFamily::TimeStepSection );
  // p->Fam.SetStateSize( p->StateSize / p->Fam.GetWordSize() );
  // It may be useful to call
  // p->JumpToMark( vtkLSDynaFamily::TimeStepSection );
  // here.
  if ( p->Fam.GetStateSize() <= 0 )
    {
    vtkErrorMacro( "Database has bad state size (" << p->Fam.GetStateSize() << ")." );
    return 1;
    }

  // Discover the number of states and record the time value for each.
  int ntimesteps = 0;
  double time;
  int itmp = 1;
  int lastAdapt = 0;
  do {
    if ( p->Fam.BufferChunk( vtkLSDynaFamily::Float, 1 ) == 0 )
      {
      time = p->Fam.GetNextWordAsFloat();
      if ( time != vtkLSDynaFamily::EOFMarker )
        {
        p->Fam.MarkTimeStep();
        p->TimeValues.push_back( time );
        //fprintf( stderr, "%d %f\n", (int) p->TimeValues.size() - 1, time ); fflush(stderr);
        if ( p->Fam.SkipToWord( vtkLSDynaFamily::TimeStepSection, ntimesteps++, p->Fam.GetStateSize() ) )
          {
          itmp = 0;
          }
        }
      else
        {
        if ( p->Fam.AdvanceFile() )
          {
          itmp = 0;
          }
        else
          {
          if ( ntimesteps == 0 )
            {
            // First time step was an EOF marker... move the marker to the
            // beginning of the first real time step...
            p->Fam.MarkSectionStart( lastAdapt, vtkLSDynaFamily::TimeStepSection );
            }
          }
        int nextAdapt = p->Fam.GetCurrentAdaptLevel();
        if ( nextAdapt != lastAdapt )
          {
          // Read the next static header section... state size has changed.
          p->Fam.MarkSectionStart( nextAdapt, vtkLSDynaFamily::ControlSection );
          this->ReadHeaderInformation( nextAdapt );
          //p->Fam.SkipToWord( vtkLSDynaFamily::EndOfStaticSection, nextAdapt, 0 );
          lastAdapt = nextAdapt;
          }
        }
      }
    else
      {
      itmp = 0;
      }
  } while (itmp);

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = ntimesteps ? ntimesteps - 1 : 0;

  return -1;
}

// =================================== Provide information about the database to the pipeline
int vtkLSDynaReader::RequestInformation( vtkInformation* vtkNotUsed(request),
                                         vtkInformationVector** vtkNotUsed(iinfo),
                                         vtkInformationVector* oinfo )
{
  vtkLSDynaReaderPrivate* p = this->P;
  // If the time step is set before RequestInformation is called, we must
  // read the header information immediately in order to determine whether
  // the timestep that's been passed is valid. If it's not, we ignore it.
  if ( ! p->FileIsValid )
    {
    if ( p->Fam.GetDatabaseDirectory().empty() )
      {
      // fail silently for CanReadFile()'s sake.
      //vtkErrorMacro( "You haven't set the LS-Dyna database directory!" );
      return 1;
      }

    if ( p->Fam.GetDatabaseBaseName().empty() )
      {
      p->Fam.SetDatabaseBaseName( "/d3plot" ); // not a bad assumption.
      }
    p->Fam.ScanDatabaseDirectory();
    if ( p->Fam.GetNumberOfFiles() < 1 )
      {
      p->FileIsValid = 0;
      return 1;
      }
    p->Fam.DetermineStorageModel();
    p->MaxFileLength = p->FileSizeFactor*512*512*p->Fam.GetWordSize();
    p->FileIsValid = 1;

    // OK, now we have a list of files. Next, determine the length of the
    // state vector (#bytes of data stored per time step):
    this->ReadHeaderInformation( 0 );

    // Finally, we can loop through and determine where all the state
    // vectors start for each time step.
    // This will call ReadHeaderInformation when it encounters any
    // mesh adaptations (so that it can get the new state vector size).
    this->ScanDatabaseTimeSteps();
    }

  if ( p->TimeValues.size() == 0 )
    {
    vtkErrorMacro( "No valid time steps in the LS-Dyna database" );
    return 0;
    }

  // Clamp timestep to be valid here.
  if ( p->CurrentState < 0 )
    {
    p->CurrentState = 0;
    }
  else if ( p->CurrentState >= (int) p->TimeValues.size() )
    {
    p->CurrentState = p->TimeValues.size() - 1;
    }

  int newAdaptLevel = p->Fam.TimeAdaptLevel( p->CurrentState );
  if ( p->Fam.GetCurrentAdaptLevel() !=  newAdaptLevel )
    {
    // Requested time step has a different mesh adaptation than current one.
    // Update the header information so that queries like GetNumberOfCells() work properly.
    int result;
    result = this->ReadHeaderInformation( newAdaptLevel );
    if ( result >= 0 )
      {
      return result;
      }
    }

  // Every output object has all the time steps.
  vtkInformation* outInfo = oinfo->GetInformationObject(0);
  outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &p->TimeValues[0], (int)p->TimeValues.size() );
  double timeRange[2];
  timeRange[0] = p->TimeValues[0];
  timeRange[1] = p->TimeValues[p->TimeValues.size() - 1];
  outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2 );

  // Currently, this is a serial reader.
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);

  return 1;
}

// ============================================================= Section parsing
int vtkLSDynaReader::ReadNodes()
{
  vtkLSDynaReaderPrivate* p = this->P;

  vtkPoints* pts = vtkPoints::New();
  if ( ! pts )
    {
    return 1;
    }

  if ( p->Fam.GetWordSize() == 4 )
    {
    pts->SetDataTypeToFloat();
    }
  else
    {
    pts->SetDataTypeToDouble();
    }

  this->OutputParticles->SetPoints( pts );
  this->OutputBeams->SetPoints( pts );
  this->OutputShell->SetPoints( pts );
  this->OutputThickShell->SetPoints( pts );
  this->OutputSolid->SetPoints( pts );
  this->OutputRigidBody->SetPoints( pts );
  // Not this->OutputRoadSurface because RoadurfaceData subsection specifies nodal coords.
  pts->Delete();
  pts->SetNumberOfPoints( p->NumberOfNodes );

  // Skip reading coordinates if we are deflecting the mesh... they would be replaced anyway.
  // Note that we still have to read the rigid road coordinates.
  double pt[3];
  if ( ! this->DeformedMesh || ! this->GetPointArrayStatus( LS_ARRAYNAME_DEFLECTION ) )
    {
    p->Fam.SkipToWord( vtkLSDynaFamily::GeometryData, p->Fam.GetCurrentAdaptLevel(), 0 );
    p->Fam.BufferChunk( vtkLSDynaFamily::Float, p->NumberOfNodes * p->Dimensionality );

    if ( p->Dimensionality == 3 )
      {
      for ( vtkIdType i=0; i<p->NumberOfNodes; ++i )
        {
        pt[0] = p->Fam.GetNextWordAsFloat();
        pt[1] = p->Fam.GetNextWordAsFloat();
        pt[2] = p->Fam.GetNextWordAsFloat();
        pts->SetPoint( i, pt );
        }
      }
    else
      {
      pt[2] = 0.;
      for ( vtkIdType i=0; i<p->NumberOfNodes; ++i )
        {
        pt[0] = p->Fam.GetNextWordAsFloat();
        pt[1] = p->Fam.GetNextWordAsFloat();
        pts->SetPoint( i, pt );
        }
      }
    }

  if ( p->ReadRigidRoadMvmt )
    {
    pts = vtkPoints::New();
    if ( ! pts )
      {
      return 1;
      }

    if ( p->Fam.GetWordSize() == 4 )
      {
      pts->SetDataTypeToFloat();
      }
    else
      {
      pts->SetDataTypeToDouble();
      }
    this->OutputRoadSurface->SetPoints( pts );
    pts->Delete();

    vtkIdType nnode = p->Dict["NNODE"];
    pts->SetNumberOfPoints( nnode );
    if ( this->GetPointArrayStatus( LS_ARRAYNAME_USERID ) )
      {
      p->Fam.SkipToWord( vtkLSDynaFamily::RigidSurfaceData, p->Fam.GetCurrentAdaptLevel(), 4 );
      p->Fam.BufferChunk( vtkLSDynaFamily::Int, nnode );
      vtkIdTypeArray* ids = vtkIdTypeArray::New();
      ids->SetNumberOfComponents( 1 );
      ids->SetNumberOfTuples( nnode );
      ids->SetName( LS_ARRAYNAME_USERID );
      this->OutputRoadSurface->GetPointData()->AddArray( ids );
      ids->Delete();
      for ( vtkIdType i=0; i<nnode; ++i )
        {
        ids->SetTuple1( i, p->Fam.GetNextWordAsInt() );
        }
      }
    else
      {
      p->Fam.SkipToWord( vtkLSDynaFamily::RigidSurfaceData, p->Fam.GetCurrentAdaptLevel(), 4 + nnode );
      }
    p->Fam.BufferChunk( vtkLSDynaFamily::Float, nnode*3 ); // These are always 3-D
    for ( vtkIdType i=0; i<nnode; ++i )
      {
      pt[0] = p->Fam.GetNextWordAsFloat();
      pt[1] = p->Fam.GetNextWordAsFloat();
      pt[2] = p->Fam.GetNextWordAsFloat();
      pts->SetPoint( i, pt );
      }
    }

  return 0;
}

int vtkLSDynaReader::ReadConnectivityAndMaterial()
{
  vtkLSDynaReaderPrivate* p = this->P;
  if ( p->ConnectivityUnpacked == 0 )
    {
    // FIXME
    vtkErrorMacro( "Packed connectivity isn't supported yet." );
    return 1;
    }

  vtkIdType nc;
  vtkIntArray* matl = 0;
  vtkIdType conn[8];
  vtkIdType matlId;
  vtkIdType c, t, i;
  int matlStatus;
  c = 0;

  nc = p->NumberOfCells[ vtkLSDynaReader::PARTICLE ];
  this->OutputParticles->Allocate( nc );
  matlStatus = this->GetCellArrayStatus( vtkLSDynaReader::PARTICLE, LS_ARRAYNAME_MATERIAL );
  if ( matlStatus )
    {
    matl = vtkIntArray::New();
    matl->SetNumberOfComponents( 1 );
    matl->SetNumberOfTuples( nc );
    matl->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputParticles->GetCellData()->AddArray( matl );
    matl->FastDelete();
    }
  t = p->Dict["NMSPH"];
  p->Fam.SkipToWord( vtkLSDynaFamily::SPHNodeData, p->Fam.GetCurrentAdaptLevel(), 0 );
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 2 * t );
  for ( i = 0; i < t; ++i )
    {
    conn[0] = p->Fam.GetNextWordAsInt() - 1;
    matlId = p->Fam.GetNextWordAsInt();
    this->OutputParticles->InsertNextCell( VTK_VERTEX, 1, conn );
    if ( matlStatus ) matl->SetTuple1( i, p->MaterialsOrdered[matlId - 1] );
    }

  p->Fam.SkipToWord( vtkLSDynaFamily::GeometryData, p->Fam.GetCurrentAdaptLevel(), p->NumberOfNodes*p->Dimensionality );

  nc = p->NumberOfCells[ vtkLSDynaReader::SOLID ];
  this->OutputSolid->Allocate( nc );
  matlStatus = this->GetCellArrayStatus( vtkLSDynaReader::SOLID, LS_ARRAYNAME_MATERIAL );
  if ( matlStatus )
    {
    matl = vtkIntArray::New();
    matl->SetNumberOfComponents( 1 );
    matl->SetNumberOfTuples( nc );
    matl->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputSolid->GetCellData()->AddArray( matl );
    matl->FastDelete();
    }
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 9*p->NumberOfCells[vtkLSDynaReader::SOLID] );
  for ( t=0; t<p->NumberOfCells[vtkLSDynaReader::SOLID]; ++t )
    {
    for ( i=0; i<8; ++i )
      {
      conn[i] = p->Fam.GetNextWordAsInt() - 1;
      }
    matlId = p->Fam.GetNextWordAsInt();
    // Detect repeated connectivity entries to determine element type
    if ( conn[7] == conn[6] )
      { // conn[6] == conn[5] is implied since there are no 7-node elements
      if ( conn[5] == conn[4] )
        {
        if ( conn[4] == conn[3] )
          {
          this->OutputSolid->InsertNextCell( VTK_TETRA, 4, conn );
          }
        else
          {
          this->OutputSolid->InsertNextCell( VTK_PYRAMID, 5, conn );
          }
        }
      else
        {
        this->OutputSolid->InsertNextCell( VTK_WEDGE, 6, conn );
        }
      }
    else
      {
      this->OutputSolid->InsertNextCell( VTK_HEXAHEDRON, 8, conn );
      }
    if ( matlStatus )
      {
      matl->SetTuple1( t, p->MaterialsOrdered[matlId - 1] );
      }
    }

  nc = p->NumberOfCells[ vtkLSDynaReader::THICK_SHELL ];
  this->OutputThickShell->Allocate( nc );
  matlStatus = this->GetCellArrayStatus( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_MATERIAL );
  if ( matlStatus )
    {
    matl = vtkIntArray::New();
    matl->SetNumberOfComponents( 1 );
    matl->SetNumberOfTuples( nc );
    matl->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputThickShell->GetCellData()->AddArray( matl );
    matl->FastDelete();
    }
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 9*p->NumberOfCells[vtkLSDynaReader::THICK_SHELL] );
  for ( t=0; t<p->NumberOfCells[vtkLSDynaReader::THICK_SHELL]; ++t )
    {
    for ( i=0; i<8; ++i )
      {
      conn[i] = p->Fam.GetNextWordAsInt() - 1;
      }
    matlId = p->Fam.GetNextWordAsInt();
    this->OutputThickShell->InsertNextCell( VTK_QUADRATIC_QUAD, 8, conn );
    if ( matlStatus ) matl->SetTuple1( t, p->MaterialsOrdered[matlId - 1] );
    }

  nc = p->NumberOfCells[ vtkLSDynaReader::BEAM ];
  this->OutputBeams->Allocate( nc );
  matlStatus = this->GetCellArrayStatus( vtkLSDynaReader::BEAM, LS_ARRAYNAME_MATERIAL );
  if ( matlStatus )
    {
    matl = vtkIntArray::New();
    matl->SetNumberOfComponents( 1 );
    matl->SetNumberOfTuples( nc );
    matl->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputBeams->GetCellData()->AddArray( matl );
    matl->FastDelete();
    }
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 6*p->NumberOfCells[vtkLSDynaReader::BEAM] );
  for ( t=0; t<p->NumberOfCells[vtkLSDynaReader::BEAM]; ++t )
    {
    for ( i=0; i<5; ++i )
      {
      conn[i] = p->Fam.GetNextWordAsInt() - 1;
      }
    matlId = p->Fam.GetNextWordAsInt();
    this->OutputBeams->InsertNextCell( VTK_LINE, 2, conn );
    if ( matlStatus ) matl->SetTuple1( t, p->MaterialsOrdered[matlId - 1] );
    }


  nc = p->NumberOfCells[ vtkLSDynaReader::SHELL ];
  this->OutputShell->Allocate( nc );
  bool haveRigidMaterials = (p->Dict["MATTYP"] != 0) && p->RigidMaterials.size();

  matlStatus = this->GetCellArrayStatus( vtkLSDynaReader::SHELL, LS_ARRAYNAME_MATERIAL );
  if ( matlStatus )
    {
    matl = vtkIntArray::New();
    matl->SetNumberOfComponents( 1 );
    matl->SetNumberOfTuples( nc );
    matl->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputShell->GetCellData()->AddArray( matl );
    matl->FastDelete();
    }

  nc = p->NumberOfCells[ vtkLSDynaReader::RIGID_BODY ];
  this->OutputRigidBody->Allocate( nc );
  int rmatStatus = this->GetCellArrayStatus( vtkLSDynaReader::RIGID_BODY, LS_ARRAYNAME_MATERIAL );
  vtkIntArray* rmat = 0;
  if ( rmatStatus )
    {
    rmat = vtkIntArray::New();
    rmat->SetNumberOfComponents( 1 );
    rmat->SetNumberOfTuples( nc );
    rmat->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputRigidBody->GetCellData()->AddArray( rmat );
    rmat->FastDelete();
    }
  vtkIdType nrFound = 0;
  vtkIdType nsFound = 0;

  // FIXME: Should this include p->NumberOfCells[ vtkLSDynaReader::RIGID_BODY ] or should matl->SetNumberOfTuples() use different number?
  p->Fam.BufferChunk( vtkLSDynaFamily::Int, 5*p->NumberOfCells[vtkLSDynaReader::SHELL] );
  for ( t=0; t<p->NumberOfCells[vtkLSDynaReader::SHELL]; ++t )
    {
    for ( i=0; i<4; ++i )
      {
      conn[i] = p->Fam.GetNextWordAsInt() - 1;
      }
    matlId = p->Fam.GetNextWordAsInt();
    if ( haveRigidMaterials && p->RigidMaterials.find( matlId ) == p->RigidMaterials.end() )
      {
      this->OutputRigidBody->InsertNextCell( VTK_QUAD, 4, conn );
      if ( rmatStatus ) rmat->SetTuple1( nrFound++, p->MaterialsOrdered[matlId - 1] );
      }
    else
      {
      this->OutputShell->InsertNextCell( VTK_QUAD, 4, conn );
      if ( matlStatus ) matl->SetTuple1( nsFound++, p->MaterialsOrdered[matlId - 1] );
      }
    }
  //fprintf( stdout, "haveRigid: %d nrFound: %d nsFound: %d numRBE: %d\n", haveRigidMaterials, (int) nrFound, (int) nsFound, (int) p->Dict["NUMRBE"] );

  // Always call allocate so that cell array is created.
  nc = p->NumberOfCells[ vtkLSDynaReader::ROAD_SURFACE ];
  this->OutputRoadSurface->Allocate( nc );
  if ( p->ReadRigidRoadMvmt )
    {
    /* FIXME: There is no material, just segment ID, for road surfaces?
    matl = vtkIntArray::New();
    matl->SetNumberOfComponents( 1 );
    matl->SetNumberOfTuples( nc );
    matl->SetName( LS_ARRAYNAME_MATERIAL );
    this->OutputRoadSurface->GetCellData()->AddArray( matl );
    matl->FastDelete();
    */

    vtkIntArray* segn = 0;
    if ( this->GetCellArrayStatus( vtkLSDynaReader::ROAD_SURFACE, LS_ARRAYNAME_SEGMENTID ) )
      {
      segn = vtkIntArray::New();
      segn->SetNumberOfComponents( 1 );
      segn->SetNumberOfTuples( nc );
      segn->SetName( LS_ARRAYNAME_SEGMENTID );
      this->OutputRoadSurface->GetCellData()->AddArray( segn );
      segn->FastDelete();

      // FIXME: We're skipping road surface node ids
      p->Fam.SkipToWord( vtkLSDynaFamily::RigidSurfaceData, p->Fam.GetCurrentAdaptLevel(), 4 + 4*p->Dict["NNODE"] );
      for ( c=0; c<p->Dict["NSURF"]; ++c )
        {
        p->Fam.BufferChunk( vtkLSDynaFamily::Int, 2 );
        vtkIdType segId = p->Fam.GetNextWordAsInt();
        vtkIdType segSz = p->Fam.GetNextWordAsInt();
        p->Fam.BufferChunk( vtkLSDynaFamily::Int, 4*segSz );
        for ( t=0; t<segSz; ++t )
          {
          for ( i=0; i<4; ++i )
            {
            conn[i] = p->Fam.GetNextWordAsInt() - 1;
            }
          this->OutputRoadSurface->InsertNextCell( VTK_QUAD, 4, conn );
          }
        for ( t=0; t<segSz; ++t )
          {
          segn->SetTuple1( t, segId );
          }
        }
      }
    else
      {
      p->Fam.SkipToWord( vtkLSDynaFamily::RigidSurfaceData, p->Fam.GetCurrentAdaptLevel(),
        4 + 4 * p->Dict["NNODE"] + 2 * p->Dict["NSEG"] + 4 * p->Dict["NSURF"] );
      }
    }

  return 0;
}

int vtkLSDynaReader::ReadUserIds()
{
  vtkLSDynaReaderPrivate* p = this->P;
  if ( p->Dict["NARBS"] <= 0 )
    {
    return 0; // Nothing to do
    }

  // Below here is code that runs when user node or element numbers are present
  int arbitraryMaterials = p->Dict["NSORT"] < 0 ? 1 : 0;
  vtkIdType isz = this->GetNumberOfNodes();
  if ( arbitraryMaterials )
    {
    p->Fam.SkipToWord( vtkLSDynaFamily::UserIdData, p->Fam.GetCurrentAdaptLevel(), 16 );
    }
  else
    {
    p->Fam.SkipToWord( vtkLSDynaFamily::UserIdData, p->Fam.GetCurrentAdaptLevel(), 10 );
    }

  // Node numbers
  vtkIdTypeArray* userNodeIds = 0;
  int nodeIdStatus = this->GetPointArrayStatus( LS_ARRAYNAME_USERID );
  if ( nodeIdStatus )
    {
    userNodeIds = vtkIdTypeArray::New();
    userNodeIds->SetNumberOfComponents(1);
    userNodeIds->SetNumberOfTuples( isz );
    userNodeIds->SetName( LS_ARRAYNAME_USERID );
    // all outputs except OutputRoadSurface share the same set of nodes:
    this->OutputSolid->GetPointData()->AddArray( userNodeIds );
    this->OutputThickShell->GetPointData()->AddArray( userNodeIds );
    this->OutputShell->GetPointData()->AddArray( userNodeIds );
    this->OutputRigidBody->GetPointData()->AddArray( userNodeIds );
    //this->OutputRoadSurface->GetPointData()->AddArray( userNodeIds );
    this->OutputBeams->GetPointData()->AddArray( userNodeIds );
    userNodeIds->Delete();
    }

  // Element numbers
  vtkIdTypeArray* userElemIds[vtkLSDynaReader::NUM_CELL_TYPES];
  int eleIdStatus[vtkLSDynaReader::NUM_CELL_TYPES];

#define VTK_LS_READCELLUSERID(mesh,celltype) \
  eleIdStatus[celltype] = this->GetCellArrayStatus( celltype, LS_ARRAYNAME_USERID ); \
  if ( eleIdStatus[celltype] ) \
    { \
    userElemIds[celltype] = vtkIdTypeArray::New(); \
    userElemIds[celltype]->SetNumberOfComponents(1); \
    userElemIds[celltype]->SetNumberOfTuples( this->P->NumberOfCells[celltype] ); \
    userElemIds[celltype]->SetName( LS_ARRAYNAME_USERID ); \
    mesh->GetCellData()->AddArray( userElemIds[celltype] ); \
    userElemIds[celltype]->Delete(); \
    }

  VTK_LS_READCELLUSERID(this->OutputSolid,vtkLSDynaReader::SOLID);
  VTK_LS_READCELLUSERID(this->OutputThickShell,vtkLSDynaReader::THICK_SHELL);
  VTK_LS_READCELLUSERID(this->OutputShell,vtkLSDynaReader::SHELL);
  VTK_LS_READCELLUSERID(this->OutputRigidBody,vtkLSDynaReader::RIGID_BODY);
  VTK_LS_READCELLUSERID(this->OutputBeams,vtkLSDynaReader::BEAM);

  eleIdStatus[vtkLSDynaReader::PARTICLE] = false;
  userElemIds[vtkLSDynaReader::PARTICLE] = 0;

  eleIdStatus[vtkLSDynaReader::ROAD_SURFACE] = false;
  userElemIds[vtkLSDynaReader::ROAD_SURFACE] = 0;

#undef VTK_LS_READCELLUSERID

  vtkIdType c;
  vtkIdType e;

  c = 0;
  e = 0;

  if ( nodeIdStatus )
    {
    p->Fam.BufferChunk( vtkLSDynaFamily::Int, isz );
    for ( c=0; c<isz; ++c )
      {
      userNodeIds->SetTuple1( c, p->Fam.GetNextWordAsInt() );
      }
    }
  else
    {
    p->Fam.SkipWords( isz );
    }

  // FIXME: This loop won't work if Rigid Body and Shell elements are interleaved (which I now believe they are)
  for ( int s=vtkLSDynaReader::PARTICLE; s<vtkLSDynaReader::NUM_CELL_TYPES; ++s )
    {
    vtkIdTypeArray* ueids = userElemIds[s];
    if ( (! eleIdStatus[s]) || (! ueids) )
      {
      p->Fam.SkipWords( this->P->NumberOfCells[s] );
      continue; // skip arrays the user doesn't want to load
      }

    p->Fam.BufferChunk( vtkLSDynaFamily::Int, this->P->NumberOfCells[s] );
    for ( e=0; e<this->P->NumberOfCells[s]; ++e )
      {
      ueids->SetTuple1( e, p->Fam.GetNextWordAsInt() );
      }
    }

  return 0;
}

int vtkLSDynaReader::ReadDeletion()
{
  int errnum = 0;
  int tmp;
  vtkLSDynaReaderPrivate* p = this->P;
  vtkDataArray* death;
  switch ( p->Dict["MDLOPT"] )
    {
  case LS_MDLOPT_POINT:
    if ( this->GetPointArrayStatus( LS_ARRAYNAME_DEATH ) )
      {
      p->Fam.SkipWords( this->GetNumberOfNodes() );
      return 0;
      }
    death =  p->Fam.GetWordSize() == 4 ?
            (vtkDataArray*) vtkFloatArray::New() :
            (vtkDataArray*) vtkDoubleArray::New();
    death->SetName( LS_ARRAYNAME_DEATH );
    death->SetNumberOfComponents( 1 );
    death->SetNumberOfTuples( this->GetNumberOfNodes() );
    errnum = this->ReadDeletionArray( death, tmp /*dummy*/ );
    if ( ! errnum )
      {
      this->OutputBeams->GetPointData()->AddArray( death );
      // Intentionally omitting this->OutputRigidBody.
      this->OutputShell->GetPointData()->AddArray( death );
      this->OutputThickShell->GetPointData()->AddArray( death );
      this->OutputSolid->GetPointData()->AddArray( death );
      }
    death->Delete();
    break;
  case LS_MDLOPT_CELL:
    if ( this->GetCellArrayStatus( vtkLSDynaReader::SOLID, LS_ARRAYNAME_DEATH ) == 0 )
      {
      p->Fam.SkipWords( this->GetNumberOfSolidCells() );
      }
    else
      {
      death =  p->Fam.GetWordSize() == 4 ?
        (vtkDataArray*) vtkFloatArray::New() :
          (vtkDataArray*) vtkDoubleArray::New();
      death->SetName( LS_ARRAYNAME_DEATH );
      death->SetNumberOfComponents( 1 );
      death->SetNumberOfTuples( p->NumberOfCells[ vtkLSDynaReader::SOLID ] );
      errnum += (tmp = this->ReadDeletionArray( death, p->AnyDeletedCells[ vtkLSDynaReader::SOLID ] ));
      if ( ! tmp )
        {
        this->OutputSolid->GetCellData()->AddArray( death );
        }
      death->Delete();
      }

    if ( this->GetCellArrayStatus( vtkLSDynaReader::THICK_SHELL, LS_ARRAYNAME_DEATH ) == 0 )
      {
      p->Fam.SkipWords( this->GetNumberOfThickShellCells() );
      }
    else
      {
      death =  p->Fam.GetWordSize() == 4 ?
        (vtkDataArray*) vtkFloatArray::New() :
          (vtkDataArray*) vtkDoubleArray::New();
      death->SetName( LS_ARRAYNAME_DEATH );
      death->SetNumberOfComponents( 1 );
      death->SetNumberOfTuples( p->NumberOfCells[ vtkLSDynaReader::THICK_SHELL ] );
      errnum += (tmp = this->ReadDeletionArray( death, p->AnyDeletedCells[ vtkLSDynaReader::THICK_SHELL ] ));
      if ( ! tmp )
        {
        this->OutputThickShell->GetCellData()->AddArray( death );
        }
      death->Delete();
      }

    if ( this->GetCellArrayStatus( vtkLSDynaReader::SHELL, LS_ARRAYNAME_DEATH ) == 0 )
      {
      p->Fam.SkipWords( this->GetNumberOfShellCells() );
      }
    else
      {
      death =  p->Fam.GetWordSize() == 4 ?
        (vtkDataArray*) vtkFloatArray::New() :
          (vtkDataArray*) vtkDoubleArray::New();
      death->SetName( LS_ARRAYNAME_DEATH );
      death->SetNumberOfComponents( 1 );
      death->SetNumberOfTuples( p->NumberOfCells[ vtkLSDynaReader::SHELL ] );
      errnum += (tmp = this->ReadDeletionArray( death, p->AnyDeletedCells[ vtkLSDynaReader::SHELL ] ));
      if ( ! tmp )
        {
        this->OutputShell->GetCellData()->AddArray( death );
        }
      death->Delete();
      }

    if ( this->GetCellArrayStatus( vtkLSDynaReader::BEAM, LS_ARRAYNAME_DEATH ) == 0 )
      {
      p->Fam.SkipWords( this->GetNumberOfBeamCells() );
      }
    else
      {
      death =  p->Fam.GetWordSize() == 4 ?
        (vtkDataArray*) vtkFloatArray::New() :
          (vtkDataArray*) vtkDoubleArray::New();
      death->SetName( LS_ARRAYNAME_DEATH );
      death->SetNumberOfComponents( 1 );
      death->SetNumberOfTuples( p->NumberOfCells[ vtkLSDynaReader::BEAM ] );
      errnum += (tmp = this->ReadDeletionArray( death, p->AnyDeletedCells[ vtkLSDynaReader::BEAM ] ));
      if ( ! tmp )
        {
        this->OutputBeams->GetCellData()->AddArray( death );
        }
      death->Delete();
      }

    // vtkLSDynaReader::PARTICLE deletion states are read by ReadSPHState() along with
    // other SPH state information.

    break;
  case LS_MDLOPT_NONE:
  default:
    // do nothing.
    errnum = 0;
    }
  return errnum;
}

int vtkLSDynaReader::ReadDeletionArray( vtkDataArray* array, int& anyZeros )
{
  double val;
  anyZeros = 0;
  vtkIdType n = array->GetNumberOfTuples();
  vtkLSDynaReaderPrivate* p = this->P;
  p->Fam.BufferChunk( vtkLSDynaFamily::Float, n );
  for ( vtkIdType i=0; i<n; ++i )
    {
    val = p->Fam.GetNextWordAsFloat();
    if ( val == 0. )
      anyZeros = 1;
    array->SetTuple1( i, val );
    }
  return 0;
}

int vtkLSDynaReader::ReadState( vtkIdType step )
{
  vtkLSDynaReaderPrivate* p = this->P;
  // Skip global variables for now
  p->Fam.SkipToWord( vtkLSDynaFamily::TimeStepSection, step, 1 + p->Dict["NGLBV"] );

  // Read nodal data ===========================================================
  vtkDataArray* var;
  vtkstd::vector<vtkDataArray*> vars;
  vtkstd::vector<int> cmps;
  // Important: push_back in the order these are interleaved on disk
  // Note that temperature and deflection are swapped relative to the order they
  // are specified in the header section.
  const char * aNames[] = {
    LS_ARRAYNAME_DEFLECTION,
    LS_ARRAYNAME_TEMPERATURE,
    LS_ARRAYNAME_VELOCITY,
    LS_ARRAYNAME_ACCELERATION,
    LS_ARRAYNAME_PRESSURE,
    LS_ARRAYNAME_VORTICITY "_X",
    LS_ARRAYNAME_VORTICITY "_Y",
    LS_ARRAYNAME_VORTICITY "_Z",
    LS_ARRAYNAME_RESULTANTVORTICITY,
    LS_ARRAYNAME_ENSTROPHY,
    LS_ARRAYNAME_HELICITY,
    LS_ARRAYNAME_STREAMFUNCTION,
    LS_ARRAYNAME_ENTHALPY,
    LS_ARRAYNAME_DENSITY,
    LS_ARRAYNAME_TURBULENTKE,
    LS_ARRAYNAME_DISSIPATION,
    LS_ARRAYNAME_EDDYVISCOSITY,
    LS_ARRAYNAME_SPECIES_01,
    LS_ARRAYNAME_SPECIES_02,
    LS_ARRAYNAME_SPECIES_03,
    LS_ARRAYNAME_SPECIES_04,
    LS_ARRAYNAME_SPECIES_05,
    LS_ARRAYNAME_SPECIES_06,
    LS_ARRAYNAME_SPECIES_07,
    LS_ARRAYNAME_SPECIES_08,
    LS_ARRAYNAME_SPECIES_09,
    LS_ARRAYNAME_SPECIES_10
  };
  const char* aDictNames[] = {
    "IU",
    "IT",
    "IV",
    "IA",
    "cfdPressure",
    "cfdXVort",
    "cfdYVort",
    "cfdZVort",
    "cfdRVort",
    "cfdEnstrophy",
    "cfdHelicity",
    "cfdStream",
    "cfdEnthalpy",
    "cfdDensity",
    "cfdTurbKE",
    "cfdDiss",
    "cfdEddyVisc",
    "cfdSpec01",
    "cfdSpec02",
    "cfdSpec03",
    "cfdSpec04",
    "cfdSpec05",
    "cfdSpec06",
    "cfdSpec07",
    "cfdSpec08",
    "cfdSpec09",
    "cfdSpec10"
  };
  int aComponents[] = {
    -1, 1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
  };
  int vppt = 0; // values per point
  int allVortPresent = p->Dict["cfdXVort"] && p->Dict["cfdYVort"] && p->Dict["cfdZVort"];

  for ( int nvnum = 0; nvnum < (int) (sizeof(aComponents)/sizeof(aComponents[0])); ++nvnum )
    {
    if ( p->Dict[ aDictNames[nvnum] ] )
      {
      if ( allVortPresent && ! strncmp( LS_ARRAYNAME_VORTICITY, aNames[ nvnum ], sizeof(LS_ARRAYNAME_VORTICITY) ) )
        {
        // turn the vorticity components from individual scalars into one vector (with a hack)
        if ( nvnum < 7 )
          continue;
        aComponents[ nvnum ] = 3;
        aNames[ nvnum ] = LS_ARRAYNAME_VORTICITY;
        }
      var = p->Fam.GetWordSize() == 4 ? (vtkDataArray*) vtkFloatArray::New() : (vtkDataArray*) vtkDoubleArray::New();
      var->SetName( aNames[ nvnum ] );
      var->SetNumberOfComponents( aComponents[ nvnum ] == -1 ? 3 : aComponents[ nvnum ] ); // Always make vectors length 3, even for 2D data
      vars.push_back( var );
      cmps.push_back( aComponents[ nvnum ] == -1 ? p->Dimensionality : aComponents[ nvnum ] );
      vppt += cmps.back();
      }
    }

  if ( vppt != 0 )
    {
    vtkstd::vector<int>::iterator arc = cmps.begin();
    for ( vtkstd::vector<vtkDataArray*>::iterator arr=vars.begin(); arr != vars.end(); ++arr, ++arc )
      {
      if ( this->GetPointArrayStatus( (*arr)->GetName() ) == 0 )
        { // don't read arrays the user didn't request, just delete them
        (*arr)->Delete();
        p->Fam.SkipWords( p->NumberOfNodes*(*arc) );
        }
      else
        {
        (*arr)->SetNumberOfTuples( p->NumberOfNodes );
        this->OutputParticles->GetPointData()->AddArray( *arr );
        this->OutputBeams->GetPointData()->AddArray( *arr );
        this->OutputShell->GetPointData()->AddArray( *arr );
        this->OutputThickShell->GetPointData()->AddArray( *arr );
        this->OutputSolid->GetPointData()->AddArray( *arr );
        (*arr)->FastDelete();
        p->Fam.BufferChunk( vtkLSDynaFamily::Float, p->NumberOfNodes*(*arc) );
        vtkIdType pt;
        double tuple[3] = { 0., 0., 0. };
        for ( pt=0; pt<p->NumberOfNodes; ++pt )
          {
          for ( int c=0; c<*arc; ++c )
            {
            tuple[c] = p->Fam.GetNextWordAsFloat();
            }
          (*arr)->SetTuple( pt, tuple );
          }
        if ( this->DeformedMesh && ! strcmp( (*arr)->GetName(), LS_ARRAYNAME_DEFLECTION) )
          {
          // Replace point coordinates with deflection (don't add to points).
          // The name "deflection" is misleading.
          this->OutputParticles->GetPoints()->SetData( *arr );
          this->OutputBeams->GetPoints()->SetData( *arr );
          this->OutputShell->GetPoints()->SetData( *arr );
          this->OutputThickShell->GetPoints()->SetData( *arr );
          this->OutputSolid->GetPoints()->SetData( *arr );
          }
        }
      }
    }

  // Read element data==========================================================

  // The element data is unfortunately interleaved so that all arrays for a single element
  // are lumped together. This makes reading in a selected subset of arrays difficult.
  // These macros greatly reduce the amount of code to read.
#define VTK_LS_CELLARRAY(cond,mesh,celltype,arrayname,components)\
  if ( cond ) \
    { \
    if ( this->GetCellArrayStatus( celltype, arrayname ) ) \
      { \
      var = p->Fam.GetWordSize() == 4 ? (vtkDataArray*) vtkFloatArray::New() : (vtkDataArray*) vtkDoubleArray::New(); \
      var->SetName( arrayname ); \
      var->SetNumberOfComponents( components ); \
      var->SetNumberOfTuples( p->NumberOfCells[ celltype ] ); \
      this->mesh->GetCellData()->AddArray( var ); \
      var->FastDelete(); \
      vars.push_back( var ); \
      cmps.push_back( vppt ); \
      } \
    vppt += components; \
    }

#define VTK_LS_READCELLS(numtuples,celltype)\
  ts = numtuples; \
  if ( vars.size() != 0 ) \
    { \
    double* tuple = new double[ ts ]; \
    memset( tuple, 0, ts*sizeof(double*) ); \
    \
    for ( vtkIdType e=0; e<p->NumberOfCells[ celltype ]; ++e ) \
      { \
      p->Fam.BufferChunk( vtkLSDynaFamily::Float, ts ); \
      for ( int i=0; i<ts; ++i ) \
        { \
        tuple[i] = p->Fam.GetNextWordAsFloat(); \
        } \
      vtkstd::vector<int>::iterator arc = cmps.begin(); \
      for ( vtkstd::vector<vtkDataArray*>::iterator arr=vars.begin(); arr != vars.end(); ++arr, ++arc ) \
        { \
        (*arr)->SetTuple( e, tuple + *arc ); \
        } \
      } \
    delete [] tuple; \
    } \
  else \
    { \
    p->Fam.SkipWords( p->NumberOfCells[ celltype ] * ts ); \
    }

  // 3D element data=======================================
  vppt = 0;
  vars.clear();
  cmps.clear();

  VTK_LS_CELLARRAY(1,OutputSolid,vtkLSDynaReader::SOLID,LS_ARRAYNAME_STRESS,6);
  VTK_LS_CELLARRAY(1,OutputSolid,vtkLSDynaReader::SOLID,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NEIPH" ] > 0,OutputSolid,vtkLSDynaReader::SOLID,LS_ARRAYNAME_INTEGRATIONPOINT,p->Dict["NEIPH"]);
  VTK_LS_CELLARRAY(p->Dict["ISTRN" ],OutputSolid,vtkLSDynaReader::SOLID,LS_ARRAYNAME_STRAIN,6);

  int ts;
  VTK_LS_READCELLS(p->Dict["NV3D"],vtkLSDynaReader::SOLID);

  // Thick shell element data==============================
  vppt = 0;
  vars.clear();
  cmps.clear();

  // Mid-surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_STRESS,6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_INTEGRATIONPOINT,p->Dict["NEIPS"]);

  // Inner surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_STRESS "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_EPSTRAIN "InnerSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf",p->Dict["NEIPS"]);

  // Outer surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_STRESS "OuterSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_EPSTRAIN "OuterSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf",p->Dict["NEIPS"]);

  VTK_LS_CELLARRAY(p->Dict["ISTRN"],OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_STRAIN "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["ISTRN"],OutputThickShell,vtkLSDynaReader::THICK_SHELL,LS_ARRAYNAME_STRAIN "OuterSurf",6);

  // If _MAXINT_ > 3, there will be additional fields. They are other
  // integration point values. There are (_MAXINT_ - 3) extra
  // integration points, each of which has a stress (6 vals),
  // an effective plastic strain (1 val), and extra integration
  // point values (NEIPS vals).
  //vppt += ( p->Dict["_MAXINT_"] - 3 ) * ( 6 * p->Dict["IOSHL(1)"] + p->Dict["IOSHL(1)"] + p->Dict["NEIPS"] );
  int itmp;
  char ctmp[128];
  for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
    {
    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,ctmp,6);

    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,ctmp,1);

    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputThickShell,vtkLSDynaReader::THICK_SHELL,ctmp,p->Dict["NEIPS"]);
    }

  VTK_LS_READCELLS(p->Dict["NV3DT"],vtkLSDynaReader::THICK_SHELL);

  // Beam element data=====================================
  vppt = 0;
  vars.clear();
  cmps.clear();

  VTK_LS_CELLARRAY(1,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_AXIALFORCE,1);
  VTK_LS_CELLARRAY(1,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_SHEARRESULTANT,2);
  VTK_LS_CELLARRAY(1,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_BENDINGRESULTANT,2);
  VTK_LS_CELLARRAY(1,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_TORSIONRESULTANT,2);

  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_SHEARSTRESS,2);
  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_AXIALSTRESS,1);
  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_AXIALSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NV1D"] > 6,OutputBeams,vtkLSDynaReader::BEAM,LS_ARRAYNAME_PLASTICSTRAIN,1);

  VTK_LS_READCELLS(p->Dict["NV1D"],vtkLSDynaReader::BEAM);

  // Shell element data====================================
  vppt = 0;
  vars.clear();
  cmps.clear();

  // Mid-surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_STRESS,6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_INTEGRATIONPOINT,p->Dict["NEIPS"]);

  // Inner surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_STRESS "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_EPSTRAIN "InnerSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "InnerSurf",p->Dict["NEIPS"]);

  // Outer surface data
  VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_STRESS "OuterSurf",6);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_EPSTRAIN "OuterSurf",1);
  VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_INTEGRATIONPOINT "OuterSurf",p->Dict["NEIPS"]);

  // If _MAXINT_ > 3, there will be additional fields. They are other
  // integration point values. There are (_MAXINT_ - 3) extra
  // integration points, each of which has a stress (6 vals),
  // an effective plastic strain (1 val), and extra integration
  // point values (NEIPS vals).
  //vppt += ( p->Dict["_MAXINT_"] - 3 ) * ( 6 * p->Dict["IOSHL(1)"] + p->Dict["IOSHL(2)"] + p->Dict["NEIPS"] );
  for ( itmp = 3; itmp < p->Dict["_MAXINT_"]; ++itmp )
    {
    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_STRESS, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["IOSHL(1)"] != 0,OutputShell,vtkLSDynaReader::SHELL,ctmp,6);

    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_EPSTRAIN, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["IOSHL(2)"] != 0,OutputShell,vtkLSDynaReader::SHELL,ctmp,1);

    sprintf( ctmp, "%sIntPt%d", LS_ARRAYNAME_INTEGRATIONPOINT, itmp + 1 );
    VTK_LS_CELLARRAY(p->Dict["NEIPS"] > 0,OutputShell,vtkLSDynaReader::SHELL,ctmp,p->Dict["NEIPS"]);
    }

  VTK_LS_CELLARRAY(p->Dict["IOSHL(3)"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_BENDINGRESULTANT,3); // Bending Mx, My, Mxy
  VTK_LS_CELLARRAY(p->Dict["IOSHL(3)"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_SHEARRESULTANT,2); // Shear Qx, Qy
  VTK_LS_CELLARRAY(p->Dict["IOSHL(3)"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_NORMALRESULTANT,3); // Normal Nx, Ny, Nxy

  VTK_LS_CELLARRAY(p->Dict["IOSHL(4)"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_THICKNESS,1);
  VTK_LS_CELLARRAY(p->Dict["IOSHL(4)"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_ELEMENTMISC,2);

  VTK_LS_CELLARRAY(p->Dict["ISTRN"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_STRAIN "InnerSurf",6);
  VTK_LS_CELLARRAY(p->Dict["ISTRN"],OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_STRAIN "OuterSurf",6);
  VTK_LS_CELLARRAY(! p->Dict["ISTRN"] || (p->Dict["ISTRN"] && p->Dict["NV2D"] >= 45),
    OutputShell,vtkLSDynaReader::SHELL,LS_ARRAYNAME_INTERNALENERGY,1);

  VTK_LS_READCELLS(p->Dict["NV2D"],vtkLSDynaReader::SHELL);

#undef VTK_LS_CELLARRAY
#undef VTK_LS_READCELLS

  return 0;
}

int vtkLSDynaReader::ReadSPHState( vtkIdType vtkNotUsed(step) )
{
  vtkLSDynaReaderPrivate* p = this->P;

  // WARNING!!!!
  // This routine assumes that the file's read head is positioned at the beginning of the state data.
 
  // Read nodal data ===========================================================
  vtkDataArray* var;
  vtkstd::vector<vtkDataArray*> vars;
  vtkstd::vector<int> cmps;
  int vppt = 0; // values per point
  int ts;

  // The data is unfortunately interleaved so that all arrays for a single
  // element are lumped together. This makes reading in a selected subset
  // of arrays difficult.  These macros greatly reduce the amount of code
  // to read.
#define VTK_LS_SPHARRAY(cond,mesh,celltype,arrayname,components)\
  if ( cond ) \
    { \
    if ( this->GetCellArrayStatus( celltype, arrayname ) ) \
      { \
      var = p->Fam.GetWordSize() == 4 ? (vtkDataArray*) vtkFloatArray::New() : (vtkDataArray*) vtkDoubleArray::New(); \
      var->SetName( arrayname ); \
      var->SetNumberOfComponents( components ); \
      var->SetNumberOfTuples( p->NumberOfCells[ celltype ] ); \
      this->mesh->GetCellData()->AddArray( var ); \
      var->FastDelete(); \
      vars.push_back( var ); \
      cmps.push_back( vppt ); \
      } \
    vppt += components; \
    }

  VTK_LS_SPHARRAY(                   1,OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_DEATH,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(2)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_RADIUSOFINFLUENCE,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(3)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_PRESSURE,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(4)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_STRESS,6);
  VTK_LS_SPHARRAY(p->Dict["isphfg(5)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_EPSTRAIN,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(6)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_DENSITY,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(7)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_INTERNALENERGY,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(8)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_NUMNEIGHBORS,1);
  VTK_LS_SPHARRAY(p->Dict["isphfg(9)"],OutputParticles,vtkLSDynaReader::PARTICLE,LS_ARRAYNAME_STRAIN,6);

  p->AnyDeletedCells[ vtkLSDynaReader::PARTICLE ] = 0;
  ts = p->Dict["NUM_SPH_DATA"];
  if ( vars.size() != 0 )
    {
    double* tuple = new double[ ts ];
    memset( tuple, 0, ts*sizeof(double*) );
   
    for ( vtkIdType e=0; e<p->NumberOfCells[ vtkLSDynaReader::PARTICLE ]; ++e )
      {
      p->Fam.BufferChunk( vtkLSDynaFamily::Float, ts );
      for ( int i=0; i<ts; ++i )
        {
        tuple[i] = p->Fam.GetNextWordAsFloat();
        }
      if ( tuple[0] == 0. )
        p->AnyDeletedCells[ vtkLSDynaReader::PARTICLE ] = 1;
      vtkstd::vector<int>::iterator arc = cmps.begin();
      for ( vtkstd::vector<vtkDataArray*>::iterator arr=vars.begin(); arr != vars.end(); ++arr, ++arc )
        {
        (*arr)->SetTuple( e, tuple + *arc );
        }
      }
    delete [] tuple;
    }

  return 0;
}

int vtkLSDynaReader::ReadUserMaterialIds()
{
  vtkLSDynaReaderPrivate* p = this->P;
  vtkIdType m, msz;

  p->MaterialsOrdered.clear();
  p->MaterialsUnordered.clear();
  p->MaterialsLookup.clear();
  // Does the file contain arbitrary material IDs?
  if ( (p->Dict["NARBS"] > 0) && (p->Dict["NSORT"] < 0) )
    { // Yes, it does. Read them.

    // Skip over arbitrary node and element IDs:
    vtkIdType skipIds = p->Dict["NUMNP"] + p->Dict["NEL8"] + p->Dict["NEL2"] + p->Dict["NEL4"] + p->Dict["NELT"];
    p->Fam.SkipToWord( vtkLSDynaFamily::UserIdData, p->Fam.GetCurrentAdaptLevel(), 16 + skipIds );
    msz  = p->Dict["NMMAT"];

    // Read in material ID lists:
    p->Fam.BufferChunk( vtkLSDynaFamily::Int, msz );
    for ( m=0; m<msz; ++m )
      {
      p->MaterialsOrdered.push_back( p->Fam.GetNextWordAsInt() );
      }

    p->Fam.BufferChunk( vtkLSDynaFamily::Int, msz );
    for ( m=0; m<msz; ++m )
      {
      p->MaterialsUnordered.push_back( p->Fam.GetNextWordAsInt() );
      }

    p->Fam.BufferChunk( vtkLSDynaFamily::Int, msz );
    for ( m=0; m<msz; ++m )
      {
      p->MaterialsLookup.push_back( p->Fam.GetNextWordAsInt() );
      }

    }
  else
    { // No, it doesn't. Fabricate a list of sequential IDs
    msz = p->Dict["NUMMAT8"] + p->Dict["NUMMATT"] + p->Dict["NUMMAT4"] + p->Dict["NUMMAT2"] + p->Dict["NGPSPH"];
    // construct the (trivial) material lookup tables
    for ( m = 1; m <= msz; ++m )
      {
      p->MaterialsOrdered.push_back( m );
      p->MaterialsUnordered.push_back( m );
      p->MaterialsLookup.push_back( m );
      }
    }
  return 0;
}

int vtkLSDynaReader::ReadInputDeck()
{
  vtkLSDynaReaderPrivate* p = this->P;

  p->PartNames.clear();
  p->PartIds.clear();
  p->PartMaterials.clear();
  p->PartStatus.clear();

  // Create simple part names as place holders
  int mat = 1;
  int i;
  int N;
  char partLabel[64];
  int arbitraryMaterials = p->Dict["NMMAT"];

#define VTK_LSDYNA_PARTLABEL(dict,fmt) \
  N = p->Dict[dict]; \
  for ( i = 0; i < N; ++i, ++mat ) \
    { \
    if ( arbitraryMaterials ) \
      sprintf( partLabel, fmt " (Matl%d)", mat, p->MaterialsOrdered[mat - 1] ); \
    else \
      sprintf( partLabel, fmt, mat ); \
    p->PartNames.push_back( partLabel ); \
    p->PartIds.push_back( arbitraryMaterials ? p->MaterialsOrdered[mat - 1] : mat ); \
    p->PartMaterials.push_back( mat ); /* PartMaterials currently unused, so this is irrevelant */ \
    p->PartStatus.push_back( 1 ); \
    }

  VTK_LSDYNA_PARTLABEL("NUMMAT8","Part%d"); // was "PartSolid%d
  VTK_LSDYNA_PARTLABEL("NUMMATT","Part%d"); // was "PartThickShell%d
  VTK_LSDYNA_PARTLABEL("NUMMAT4","Part%d"); // was "PartShell%d
  VTK_LSDYNA_PARTLABEL("NUMMAT2","Part%d"); // was "PartBeam%d
  VTK_LSDYNA_PARTLABEL("NGPSPH", "Part%d"); // was "PartParticle%d
  VTK_LSDYNA_PARTLABEL("NSURF",  "Part%d"); // was "PartRoadSurface%d
  VTK_LSDYNA_PARTLABEL("NUMMAT", "Part%d"); // was "PartRigidBody%d

#undef VTK_LSDYNA_PARTLABEL

  if ( ! this->InputDeck )
    {
    // nothing more we can do
    return 0;
    }

  ifstream deck( this->InputDeck, ios::in );
  if ( ! deck.good() )
    {
    return 0;
    }

  vtkstd::string header;
  vtkLSGetLine( deck, header );
  deck.seekg( 0, ios::beg );
  int retval;
  if ( vtksys::SystemTools::StringStartsWith( header.c_str(), "<?xml" ) )
    {
    retval = this->ReadInputDeckXML( deck );
    }
  else
    {
    retval = this->ReadInputDeckKeywords( deck );
    }

  return retval;
}

int vtkLSDynaReader::ReadInputDeckXML( ifstream& deck )
{
  vtkXMLDynaSummaryParser* parser = vtkXMLDynaSummaryParser::New();
  parser->P = this->P;
  parser->SetStream( &deck );
  // We must be able to parse the file and end up with 1 part per material ID
  if ( ! parser->Parse() || this->P->GetTotalMaterialCount() != (int)this->P->PartNames.size() )
    {
    // We had a problem identifying a part, give up and start over,
    // pretending that InputDeck was NULL so as to get the automatically
    // generated part names.
    char* inputDeckTmp = this->InputDeck;
    this->InputDeck = 0;
    this->ReadInputDeck();
    this->InputDeck = inputDeckTmp;
    }
  parser->Delete();

  return 0;
}

int vtkLSDynaReader::ReadInputDeckKeywords( ifstream& deck )
{
  int success = 1;
  vtkstd::map<vtkstd::string,int> parameters;
  vtkstd::string line;
  vtkstd::string lineLowercase;
  vtkstd::string partName;
  int partMaterial;
  int partId;
  int curPart = 0;

  while ( deck.good() && vtkLSNextSignificantLine( deck, line ) && curPart < (int)this->P->PartNames.size() )
    {
    if ( line[0] == '*' )
      {
      vtkLSDowncaseFirstWord( lineLowercase, line.substr( 1 ) );
      if ( vtksys::SystemTools::StringStartsWith( lineLowercase.c_str(), "part" ) )
        {
        // found a part
        // ... read the next non-comment line as the part name
        if ( vtkLSNextSignificantLine( deck, line ) )
          {
          // Get rid of leading and trailing newlines, whitespace, etc.
          vtkLSTrimWhitespace( line );
          partName = line;
          }
        else
          {
          partName = "";
          }
        // ... read the next non-comment line as the part id or a reference to it.
        if ( vtkLSNextSignificantLine( deck, line ) )
          {
          vtkstd::vector<vtkstd::string> splits;
          vtkLSSplitString( line, splits, "& ,\t\n\r" );
          if ( line[0] == '&' )
            {
            // found a reference. look it up.
            partId = splits.size() ? parameters[splits[0]] : -1;
            }
          else
            {
            if ( splits.size() < 1 || sscanf( splits[0].c_str(), "%d", &partId ) <= 0 )
              {
              partId = -1;
              }
            }
          if ( splits.size() < 3 )
            {
            partMaterial = -1;
            }
          else
            {
            if ( splits[2][0] == '&' )
              {
              partMaterial = parameters[splits[2]];
              }
            else
              {
              if ( sscanf( splits[2].c_str(), "%d", &partMaterial ) <= 0 )
                {
                partMaterial = -1;
                }
              }
            }
          } // read the part id or reference
        else
          {
          partId = -1;
          partMaterial = -1;
          }
        // Comment on next line: partId values need not be consecutive. FIXME: ... or even positive?
        if ( ! partName.empty() && partId >= 0 )
          {
          this->P->PartNames[curPart] = partName;
          this->P->PartIds[curPart] = partId;
          this->P->PartMaterials[curPart] = partMaterial;
          this->P->PartStatus[curPart] = 1;
          //fprintf( stderr, "%2d: Part: \"%s\" Id: %d\n", curPart, partName.c_str(), partId );
          ++curPart;
          }
        else
          {
          success = 0;
          }
        }
      else if ( vtksys::SystemTools::StringStartsWith( lineLowercase.c_str(), "parameter" ) )
        {
        // found a reference
        // ... read the next non-comment line to decode the reference
        if ( vtkLSNextSignificantLine( deck, line ) )
          {
          vtkstd::string paramName;
          int paramIntVal;
          // Look for "^[IiRr]\s*(\w+)\s+([\w\.-]+)" and set parameters[\2]=\1
          if ( line[0] == 'I' || line[0] == 'i' )
            { // We found an integer parameter. Those are the only ones we care about.
            line = line.substr( 1 );
            vtkstd::string::size_type paramStart = line.find_first_not_of( " \t," );
            if ( paramStart == vtkstd::string::npos )
              { // ignore a bad parameter line
              continue;
              }
            vtkstd::string::size_type paramEnd = line.find_first_of( " \t,", paramStart );
            if ( paramEnd == vtkstd::string::npos )
              { // found the parameter name, but no value after it
              continue;
              }
            paramName = line.substr( paramStart, paramEnd - paramStart );
            if ( sscanf( line.substr( paramEnd + 1 ).c_str(), "%d", &paramIntVal ) <= 0 )
              { // unable to read id
              continue;
              }
            parameters[ paramName ] = paramIntVal;
            }
          }
        else
          {
          // no valid line after "*parameter" keyword. Silently ignore it.
          }
        } // "parameter line"
      } // line starts with "*"
    } // while ( deck.good() )

  if ( success )
    {
    // Save a summary file if possible. The user can open the summary file next
    // time and not be forced to parse the entire input deck to get part IDs.
    vtkstd::string deckDir = vtksys::SystemTools::GetFilenamePath( this->InputDeck );
    vtkstd::string deckName = vtksys::SystemTools::GetFilenameName( this->InputDeck );
    vtkstd::string deckExt;
    vtkstd::string::size_type dot;
    vtkstd::string xmlSummary;

    // GetFilenameExtension doesn't look for the rightmost "." ... do it ourselves.
    dot = deckName.rfind( '.' );
    if ( dot != vtkstd::string::npos )
      {
      deckExt = deckName.substr( dot );
      deckName = deckName.substr( 0, dot );
      }
    else
      {
      deckExt = "";
      }
#ifndef WIN32
    xmlSummary = deckDir + "/" + deckName + ".lsdyna";
#else
    xmlSummary = deckDir + "\\" + deckName + ".lsdyna";
#endif // WIN32
    // As long as we don't kill the input deck, write the summary XML:
    if ( xmlSummary != this->InputDeck )
      {
      this->WriteInputDeckSummary( xmlSummary.c_str() );
      }
    }
  else
    {
    // We had a problem identifying a part, give up and start over, pretending
    // that InputDeck was NULL so as to get the automatically generated part names.
    char* inputDeckTmp = this->InputDeck;
    this->InputDeck = 0;
    this->ReadInputDeck();
    this->InputDeck = inputDeckTmp;
    }

  return ! success;
}

int vtkLSDynaReader::WriteInputDeckSummary( const char* fname )
{
  ofstream xmlSummary( fname, ios::out | ios::trunc );
  if ( ! xmlSummary.good() )
    {
    return 1;
    }

  xmlSummary
    << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
    << "<lsdyna>" << endl;

  vtkstd::string dbDir = this->P->Fam.GetDatabaseDirectory();
  vtkstd::string dbName = this->P->Fam.GetDatabaseBaseName();
  if ( this->IsDatabaseValid() && ! dbDir.empty() && ! dbName.empty() )
    {
#ifndef WIN32
    if ( dbDir[0] == '/' )
#else
    if ( dbDir[0] == '\\' )
#endif // WIN32
      {
      // OK, we have an absolute path, so it should be safe to write it out.
      xmlSummary
        << "  <database path=\"" << dbDir.c_str()
        << "\" name=\"" << dbName.c_str() << "\"/>" << endl;
      }
    }

  for ( unsigned p = 0; p < this->P->PartNames.size(); ++p )
    {
    xmlSummary
      << "  <part id=\"" << this->P->PartIds[p]
      << "\" material_id=\"" << this->P->PartMaterials[p]
      << "\" status=\"" << this->P->PartStatus[p]
      << "\"><name>" << this->P->PartNames[p].c_str()
      << "</name></part>" << endl;
    }

  xmlSummary
    << "</lsdyna>" << endl;

  return 0;
}

void vtkLSDynaReader::PartFilter( vtkMultiBlockDataSet* mbds, int celltype )
{
  vtkLSDynaReaderPrivate* p = this->P;
  vtkDataSet* target = 0;
  switch ( celltype )
    {
  case vtkLSDynaReader::PARTICLE:
    target = this->OutputParticles;
    break;
  case vtkLSDynaReader::BEAM:
    target = this->OutputBeams;
    break;
  case vtkLSDynaReader::SHELL:
    target = this->OutputShell;
    break;
  case vtkLSDynaReader::THICK_SHELL:
    target = this->OutputThickShell;
    break;
  case vtkLSDynaReader::SOLID:
    target = this->OutputSolid;
    break;
  case vtkLSDynaReader::RIGID_BODY:
    target = this->OutputRigidBody;
    break;
  case vtkLSDynaReader::ROAD_SURFACE:
    target = this->OutputRoadSurface;
    break;
  default:
    vtkErrorMacro( "Unknown cell type " << celltype << " passed to PartFilter." );
    return; // nothing we can do.
    }

  if ( p->NumberOfCells[celltype] == 0 )
    {
    // no work to do, just add the dataset as-is.
    mbds->SetBlock ( celltype, target );
    return;
    }

  // We may not have any work to do if we're only removing deleted cells:
  if ( ! this->SplitByMaterialId )
    {
    if ( celltype == vtkLSDynaReader::RIGID_BODY || celltype == vtkLSDynaReader::ROAD_SURFACE )
      {
      // no deletion data for these cell types, just add the dataset as-is.
      mbds->SetBlock( celltype, target );
      return;
      }
    }

  const char* attribName = this->RemoveDeletedCells ? LS_ARRAYNAME_DEATH : LS_ARRAYNAME_MATERIAL;
  int sequentialIds = this->RemoveDeletedCells ? 1 : 0;

  int m;
  vtkMultiThreshold* thresh = vtkMultiThreshold::New();
  vtkUnstructuredGrid* temp = vtkUnstructuredGrid::New();
  temp->ShallowCopy( target );

  thresh->SetInput( temp );
  vtkstd::vector<int> partSetIds;
  int partSetId;
  for ( m = 0; m < (int) p->MaterialsOrdered.size(); ++m )
    {
    int matlId = p->MaterialsOrdered[m];
    vtkstd::vector<int>::iterator partId = vtkstd::find( p->PartIds.begin(), p->PartIds.end(), matlId );
    if ( partId == p->PartIds.end() || ! p->PartStatus[partId - p->PartIds.begin()] )
      {
      //vtkWarningMacro( "Skipping material " << m << " (" << matlId << ") because there's no associated part." );
      continue; // Skip parts with status "off" or materials without an associated part.
      }

    // Create a list of "notch" intervals, one for each part with status "on". If RemoveDeletedCells is true, attribName == Death.
    double notch = sequentialIds ? m + 1 : matlId;
    partSetId = thresh->AddBandpassIntervalSet( notch, notch, vtkDataObject::FIELD_ASSOCIATION_CELLS, attribName, 0, 1 );
    if ( this->SplitByMaterialId )
      {
      thresh->OutputSet( partSetId );
      }
    else
      {
      partSetIds.push_back( partSetId );
      }
    }

  if ( ! this->SplitByMaterialId )
    {
    partSetId = thresh->AddBooleanSet( vtkMultiThreshold::OR, (int)partSetIds.size(), &partSetIds[0] );
    thresh->OutputSet( partSetId );
    }
  thresh->Update();
  temp->Delete();

  mbds->SetBlock( celltype, thresh->GetOutput() );
  thresh->Delete();
}

// ================================================== OK Already! Read the file!
int vtkLSDynaReader::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(iinfo),
  vtkInformationVector* oinfo )
{
  vtkLSDynaReaderPrivate* p = this->P;

  if ( ! p->FileIsValid )
    {
    // This should have been set in RequestInformation()
    return 0;
    }

  vtkMultiBlockDataSet* mbds = 0;
  vtkInformation* oi = oinfo->GetInformationObject(0);
  if ( ! oi )
    {
    return 0;
    }

  if ( oi->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() ) )
    {
    // Only return single time steps for now.
    double* requestedTimeSteps = oi->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() );
    int timeStepLen = oi->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    double* timeSteps = oi->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );

    int cnt = 0;
    while ( cnt < timeStepLen - 1 && timeSteps[cnt] < requestedTimeSteps[0] )
      {
      ++cnt;
      }
    this->SetTimeStep( cnt );

    oi->Set( vtkDataObject::DATA_TIME_STEPS(), &p->TimeValues[ p->CurrentState ], 1 );
    }

  mbds = vtkMultiBlockDataSet::SafeDownCast( oi->Get(vtkDataObject::DATA_OBJECT()) );
  if ( ! mbds )
    {
    return 0;
    }

  mbds->SetNumberOfBlocks( 1 );

#define VTK_LSDYNA_PREPDATASET(mds,x,m,n,mtype) \
    x = mtype::New();

    VTK_LSDYNA_PREPDATASET(mbds,this->OutputSolid,      0,0,vtkUnstructuredGrid);
    VTK_LSDYNA_PREPDATASET(mbds,this->OutputThickShell, 0,1,vtkUnstructuredGrid);
    VTK_LSDYNA_PREPDATASET(mbds,this->OutputShell,      0,2,vtkUnstructuredGrid);
    VTK_LSDYNA_PREPDATASET(mbds,this->OutputRigidBody,  0,3,vtkUnstructuredGrid);
    VTK_LSDYNA_PREPDATASET(mbds,this->OutputRoadSurface,0,4,vtkUnstructuredGrid);
    VTK_LSDYNA_PREPDATASET(mbds,this->OutputBeams,      0,5,vtkUnstructuredGrid);
    VTK_LSDYNA_PREPDATASET(mbds,this->OutputParticles,  0,6,vtkUnstructuredGrid);
#undef VTK_LSDYNA_PREPDATASET

  this->UpdateProgress( 0.01 );

  if ( p->Dict["MATTYP"] )
    {
    // Do something with material type data
    }
  this->UpdateProgress( 0.05 );

  if ( p->Dict["IALEMAT"] )
    {
    // Do something with fluid material ID data
    }
  this->UpdateProgress( 0.10 );

  if ( p->Dict["NMSPH"] )
    {
    // Do something with smooth partical hydrodynamics element data
    }
  this->UpdateProgress( 0.15 );

  // Always read nodes
  if ( this->ReadNodes() )
    {
    vtkErrorMacro( "Could not read nodal coordinates." );
    return 1;
    }
  this->UpdateProgress( 0.25 );

  // Do something with user-specified node/element/material numbering
  if ( this->ReadUserIds() )
    {
    vtkErrorMacro( "Could not read user node/element IDs." );
    return 1;
    }

  // Always read connectivity info
  if ( this->ReadConnectivityAndMaterial() )
    {
    vtkErrorMacro( "Could not read connectivity." );
    return 1;
    }
  this->UpdateProgress( 0.5 );

  // Adapted element parent list
  // This isn't even implemented by LS-Dyna yet

  // Smooth Particle Hydrodynamics Node and Material List are handled in ReadConnectivityAndMaterial()

  // Rigid Road Surface Data
  if ( p->ReadRigidRoadMvmt )
    {
    vtkErrorMacro( "Rigid surfaces not implemented." );
    return 1;
    }
  this->UpdateProgress( 0.6 );

  // Debug sanity check:
  //p->DumpDict( cout );

  // Start of state data ===================
  // I. Node and Cell State
  if ( this->ReadState( p->CurrentState ) )
    {
    vtkErrorMacro( "Problem reading state data for time step " << p->CurrentState );
    return 1;
    }

  // II. Cell Death State
  if ( this->ReadDeletion() )
    {
    vtkErrorMacro( "Problem reading deletion state." );
    return 1;
    }

  // III. SPH Node State
  if ( this->GetNumberOfParticleCells() )
    {
    if ( this->ReadSPHState( p->CurrentState ) )
      {
      vtkErrorMacro( "Problem reading smooth particle hydrodynamics state." );
      return 1;
      }
    }

  // IV. Rigid Cell Motion State
  if ( p->ReadRigidRoadMvmt )
    {
    vtkErrorMacro( "Rigid surfaces not implemented." );
    return 1;
    }

  // Now a superset of the data has been read and there is enough information present
  // to subset and/or partition the mesh based on:
  // 1. Deleted cells
  // 2. Material ID
  int anyButNotAllPartsSelected = 0;
  unsigned int pid;
  for ( pid = 0; pid < p->PartStatus.size(); ++pid )
    {
    if ( p->PartStatus[pid] )
      anyButNotAllPartsSelected |= 2;
    else
      anyButNotAllPartsSelected |= 1;
    if ( anyButNotAllPartsSelected == 3 )
      break; // we have at least one part turned on and at least one part turned off.
    }
  int needToRunPartFilter;
  switch ( anyButNotAllPartsSelected )
    {
  case 0: // no parts exist
  case 1: // all parts are turned off
    needToRunPartFilter = -1;
    break;
  case 2: // all parts are turned on
    needToRunPartFilter = RemoveDeletedCells || this->SplitByMaterialId;
    break;
  case 3:
  default:
    needToRunPartFilter = 1;
    break;
    }

  if ( needToRunPartFilter > 0 )
    {
    for ( int ct = vtkLSDynaReader::PARTICLE; ct < vtkLSDynaReader::NUM_CELL_TYPES; ++ct )
      {
      this->PartFilter( mbds, ct );
      }
    }
  else if ( needToRunPartFilter == 0 )
    {
#define VTK_LSDYNA_SETBLOCK(mds,x,m,mtype) \
  if ( ! x ) \
    { \
    mtype* tmpDS = mtype::New(); \
    mds->SetBlock(m, tmpDS);               \
    tmpDS->FastDelete(); \
    } \
  else \
    { \
    mds->SetBlock(m, x);                   \
    }

    VTK_LSDYNA_SETBLOCK(mbds,this->OutputSolid,      0,vtkUnstructuredGrid);
    VTK_LSDYNA_SETBLOCK(mbds,this->OutputThickShell, 1,vtkUnstructuredGrid);
    VTK_LSDYNA_SETBLOCK(mbds,this->OutputShell,      2,vtkUnstructuredGrid);
    VTK_LSDYNA_SETBLOCK(mbds,this->OutputRigidBody,  3,vtkUnstructuredGrid);
    VTK_LSDYNA_SETBLOCK(mbds,this->OutputRoadSurface,4,vtkUnstructuredGrid);
    VTK_LSDYNA_SETBLOCK(mbds,this->OutputBeams,      5,vtkUnstructuredGrid);
    VTK_LSDYNA_SETBLOCK(mbds,this->OutputParticles,  6,vtkUnstructuredGrid);

#undef VTK_LSDYNA_SETBLOCK
    }

  this->OutputSolid->Delete();
  this->OutputThickShell->Delete();
  this->OutputShell->Delete();
  this->OutputRigidBody->Delete();
  this->OutputRoadSurface->Delete();
  this->OutputBeams->Delete();
  this->OutputParticles->Delete();

#ifdef VTK_LSDYNA_DBG_MULTIBLOCK
  // Print the hierarchy of meshes that is about to be returned as output.
  vtkIndent indent;
  vtkDebugMultiBlockStructure( indent, mbds );
#endif // VTK_LSDYNA_DBG_MULTIBLOCK

  this->UpdateProgress( 1.0 );
  return 1;
}

#ifdef VTK_LSDYNA_DBG_MULTIBLOCK
static void vtkDebugMultiBlockStructure( vtkIndent indent, vtkMultiGroupDataSet* mbds )
{
  vtkMultiGroupDataInformation* info;
  vtkDataObject* child;

  if ( ! mbds )
    return;

  info = mbds->GetMultiGroupDataInformation();
  if ( ! info )
    {
    cerr << "xxx" << mbds->GetClassName() << " had NULL group information.\n";
    return;
    }

  int ngroup = info->GetNumberOfGroups();
  cout << indent << mbds->GetClassName() << " with " << ngroup << " groups. (" << mbds << ")\n";
  for ( int g = 0; g < ngroup; ++g )
    {
    vtkIndent l2indent = indent.GetNextIndent();
    int nchildren = mbds->GetNumberOfDataSets( g );
    cout << l2indent << "Group " << g << " has " << nchildren << " children:\n";
    vtkIndent l3indent = l2indent.GetNextIndent();
    for ( int c = 0; c < nchildren; ++c )
      {
      child = mbds->GetDataSet( g, c );
      if ( ! child )
        {
        cout << l3indent << "NULL entry\n";
        }
      else if ( child->IsA( "vtkMultiGroupDataSet" ) )
        {
        vtkMultiGroupDataSet* gchild = vtkMultiGroupDataSet::SafeDownCast(child);
        vtkDebugMultiBlockStructure( l3indent, gchild );
        }
      else if ( child->IsA( "vtkPointSet" ) )
        {
        vtkPointSet* pchild = vtkPointSet::SafeDownCast(child);
        cout << l3indent << pchild->GetClassName() << " with " << pchild->GetNumberOfPoints() << " points and "
          << pchild->GetNumberOfCells() << " cells.\n";
        }
      else
        {
        cout << l3indent << child->GetClassName() << "\n";
        }
      }
    }
}
#endif // VTK_LSDYNA_DBG_MULTIBLOCK
