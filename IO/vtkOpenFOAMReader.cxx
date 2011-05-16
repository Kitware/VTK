/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenFOAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Terry Jordan of SAIC at the National Energy
// Technology Laboratory who developed this class.
// Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov)
//
// Token-based FoamFile format lexer/parser,
// performance/stability/compatibility enhancements, gzipped file
// support, lagrangian field support, variable timestep support,
// builtin cell-to-point filter, pointField support, polyhedron
// decomposition support, OF 1.5 extended format support, multiregion
// support, old mesh format support, parallelization support for
// decomposed cases in conjunction with vtkPOpenFOAMReader, et. al. by
// Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp)
//
// * GUI Based selection of mesh regions and fields available in the case
// * Minor bug fixes / Strict memory allocation checks
// * Minor performance enhancements
// by Philippose Rajan (sarith@rocketmail.com)

// Hijack the CRC routine of zlib to omit CRC check for gzipped files
// (on OSes other than Windows where the mechanism doesn't work due
// to pre-bound DLL symbols) if set to 1, or not (set to 0). Affects
// performance by about 3% - 4%.
#define VTK_FOAMFILE_OMIT_CRCCHECK 0

// The input/output buffer sizes for zlib in bytes.
#define VTK_FOAMFILE_INBUFSIZE (16384)
#define VTK_FOAMFILE_OUTBUFSIZE (131072)
#define VTK_FOAMFILE_INCLUDE_STACK_SIZE (10)

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#if VTK_FOAMFILE_OMIT_CRCCHECK
#define ZLIB_INTERNAL
#endif

// for possible future extension of linehead-aware directives
#define VTK_FOAMFILE_RECOGNIZE_LINEHEAD 0

#include "vtkOpenFOAMReader.h"

#include <vtkstd/vector>
#include "vtksys/DateStamp.h"
#include "vtksys/SystemTools.hxx"
#include <vtksys/ios/sstream>
#include "vtk_zlib.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCollection.h"
#include "vtkConvexPointSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkHexahedron.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkSortDataArray.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"
#include "vtkWedge.h"

#if !(defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__))
// for getpwnam() / getpwuid()
#include <sys/types.h>
#include <pwd.h>
// for getuid()
#include <unistd.h>
#endif
// for fabs()
#include <math.h>
// for isalnum() / isspace() / isdigit()
#include <ctype.h>

#if VTK_FOAMFILE_OMIT_CRCCHECK
uLong ZEXPORT crc32(uLong, const Bytef *, uInt)
{ return 0; }
#endif

vtkStandardNewMacro(vtkOpenFOAMReader);

// forward declarations
template <typename T> struct vtkFoamArrayVector
  : public vtkstd::vector<T *>
{
private:
  typedef vtkstd::vector<T *> Superclass;

public:
  ~vtkFoamArrayVector()
  {
    for(size_t arrayI = 0; arrayI < Superclass::size(); arrayI++)
      {
      if(Superclass::operator[](arrayI))
        {
        Superclass::operator[](arrayI)->Delete();
        }
      }
  }
};

typedef vtkFoamArrayVector<vtkIntArray> vtkFoamIntArrayVector;
typedef vtkFoamArrayVector<vtkFloatArray> vtkFoamFloatArrayVector;
struct vtkFoamIntVectorVector;

struct vtkFoamError;
struct vtkFoamToken;
struct vtkFoamFileStack;
struct vtkFoamFile;
struct vtkFoamIOobject;
template <typename T> struct vtkFoamReadValue;
struct vtkFoamEntryValue;
struct vtkFoamEntry;
struct vtkFoamDict;

//-----------------------------------------------------------------------------
// class vtkOpenFOAMReaderPrivate
// the reader core of vtkOpenFOAMReader
class VTK_IO_EXPORT vtkOpenFOAMReaderPrivate : public vtkObject
{
public:
  static vtkOpenFOAMReaderPrivate *New();
  vtkTypeMacro(vtkOpenFOAMReaderPrivate, vtkObject);

  vtkDoubleArray *GetTimeValues()
    {return this->TimeValues;}
  vtkGetMacro(TimeStep, int);
  vtkSetMacro(TimeStep, int);
  const vtkStdString &GetRegionName() const
    {return this->RegionName;}

  // gather timestep information
  bool MakeInformationVector(const vtkStdString &, const vtkStdString &,
      const vtkStdString &, vtkOpenFOAMReader *);
  // read mesh/fields and create dataset
  int RequestData(vtkMultiBlockDataSet *, bool, bool, bool);
  void SetTimeValue(const double);
  int MakeMetaDataAtTimeStep(vtkStringArray *, vtkStringArray *,
      vtkStringArray *, const bool);
  void SetupInformation(const vtkStdString &, const vtkStdString &,
      const vtkStdString &, vtkOpenFOAMReaderPrivate *);

private:
  struct vtkFoamBoundaryEntry
    {
    enum bt
      {
      PHYSICAL = 1,   // patch, wall
      PROCESSOR = 2,  // processor
      GEOMETRICAL = 0 // symmetryPlane, wedge, cyclic, empty, etc.
      };
    vtkStdString BoundaryName;
    int NFaces, StartFace, AllBoundariesStartFace;
    bool IsActive;
    bt BoundaryType;
    };

  struct vtkFoamBoundaryDict : public vtkstd::vector<vtkFoamBoundaryEntry>
    {
    // we need to keep the path to time directory where the current mesh
    // is read from, since boundaryDict may be accessed multiple times
    // at a timestep for patch selections
    vtkStdString TimeDir;
    };

  vtkOpenFOAMReader *Parent;

  // case and region
  vtkStdString CasePath;
  vtkStdString RegionName;
  vtkStdString ProcessorName;

  // time information
  vtkDoubleArray *TimeValues;
  int TimeStep;
  int TimeStepOld;
  vtkStringArray *TimeNames;

  int InternalMeshSelectionStatus;
  int InternalMeshSelectionStatusOld;

  // filenames / directories
  vtkStringArray *VolFieldFiles;
  vtkStringArray *PointFieldFiles;
  vtkStringArray *LagrangianFieldFiles;
  vtkStringArray *PolyMeshPointsDir;
  vtkStringArray *PolyMeshFacesDir;

  // for mesh construction
  vtkIdType NumCells;
  vtkIdType NumPoints;
  vtkIntArray *FaceOwner;

  // for cell-to-point interpolation
  vtkPolyData *AllBoundaries;
  vtkIntArray *AllBoundariesPointMap;
  vtkIntArray *InternalPoints;

  // for caching mesh
  vtkUnstructuredGrid *InternalMesh;
  vtkMultiBlockDataSet *BoundaryMesh;
  vtkFoamIntArrayVector *BoundaryPointMap;
  vtkFoamBoundaryDict BoundaryDict;
  vtkMultiBlockDataSet *PointZoneMesh;
  vtkMultiBlockDataSet *FaceZoneMesh;
  vtkMultiBlockDataSet *CellZoneMesh;

  // for polyhedra handling
  int NumTotalAdditionalCells;
  vtkIntArray *AdditionalCellIds;
  vtkIntArray *NumAdditionalCells;
  vtkFoamIntArrayVector *AdditionalCellPoints;

  // constructor and destructor are kept private
  vtkOpenFOAMReaderPrivate();
  ~vtkOpenFOAMReaderPrivate();

  // not implemented.
  vtkOpenFOAMReaderPrivate(const vtkOpenFOAMReaderPrivate &);
  void operator=(const vtkOpenFOAMReaderPrivate &);

  // clear mesh construction
  void ClearInternalMeshes();
  void ClearBoundaryMeshes();
  void ClearMeshes();

  vtkStdString RegionPath() const
    {return (this->RegionName == "" ? "" : "/") + this->RegionName;}
  vtkStdString TimePath(const int timeI) const
    {return this->CasePath + this->TimeNames->GetValue(timeI);}
  vtkStdString TimeRegionPath(const int timeI) const
    {return this->TimePath(timeI) + this->RegionPath();}
  vtkStdString CurrentTimePath() const
    {return this->TimePath(this->TimeStep);}
  vtkStdString CurrentTimeRegionPath() const
    {return this->TimeRegionPath(this->TimeStep);}
  vtkStdString CurrentTimeRegionMeshPath(vtkStringArray *dir) const
    {return this->CasePath + dir->GetValue(this->TimeStep) + this->RegionPath()
    + "/polyMesh/";}
  vtkStdString RegionPrefix() const
    {return this->RegionName + (this->RegionName == "" ? "" : "/");}

  // search time directories for mesh
  void AppendMeshDirToArray(vtkStringArray *, const vtkStdString &, const int);
  void PopulatePolyMeshDirArrays();

  // search a time directory for field objects
  void GetFieldNames(const vtkStdString &, const bool, vtkStringArray *,
      vtkStringArray *);
  void SortFieldFiles(vtkStringArray *, vtkStringArray *, vtkStringArray *);
  void LocateLagrangianClouds(vtkStringArray *, const vtkStdString &);

  // read controlDict
  bool ListTimeDirectoriesByControlDict(vtkFoamDict *dict);
  bool ListTimeDirectoriesByInstances();

  // read mesh files
  vtkFloatArray* ReadPointsFile();
  vtkFoamIntVectorVector* ReadFacesFile (const vtkStdString &);
  vtkFoamIntVectorVector* ReadOwnerNeighborFiles(const vtkStdString &,
      vtkFoamIntVectorVector *);
  bool CheckFacePoints(vtkFoamIntVectorVector *);

  // create mesh
  void InsertCellsToGrid(vtkUnstructuredGrid *, const vtkFoamIntVectorVector *,
      const vtkFoamIntVectorVector *, vtkFloatArray *, vtkIdTypeArray *,
      vtkIntArray *);
  vtkUnstructuredGrid *MakeInternalMesh(const vtkFoamIntVectorVector *,
      const vtkFoamIntVectorVector *, vtkFloatArray *);
  void InsertFacesToGrid(vtkPolyData *, const vtkFoamIntVectorVector *, int,
      int, vtkIntArray *, vtkIdList *, vtkIntArray *, const bool);
  template <typename T1, typename T2> bool ExtendArray(T1 *, const int);
  vtkMultiBlockDataSet* MakeBoundaryMesh(const vtkFoamIntVectorVector *,
      vtkFloatArray *);
  void SetBlockName(vtkMultiBlockDataSet *, unsigned int, const char *);
  void TruncateFaceOwner();

  // move additional points for decomposed cells
  vtkPoints *MoveInternalMesh(vtkUnstructuredGrid *, vtkFloatArray *);
  void MoveBoundaryMesh(vtkMultiBlockDataSet *, vtkFloatArray *);

  // cell-to-point interpolator
  void InterpolateCellToPoint(vtkFloatArray *, vtkFloatArray *, vtkPointSet *,
      vtkIntArray *, const int);

  // read and create cell/point fields
  void ConstructDimensions(vtkStdString *, vtkFoamDict *);
  bool ReadFieldFile(vtkFoamIOobject *, vtkFoamDict *, const vtkStdString &,
      vtkDataArraySelection *);
  vtkFloatArray *FillField(vtkFoamEntry *, int, vtkFoamIOobject *,
      const vtkStdString &);
  void GetVolFieldAtTimeStep(vtkUnstructuredGrid *, vtkMultiBlockDataSet *,
      const vtkStdString &);
  void GetPointFieldAtTimeStep(vtkUnstructuredGrid *, vtkMultiBlockDataSet *,
      const vtkStdString &);
  void AddArrayToFieldData(vtkDataSetAttributes *, vtkDataArray *,
      const vtkStdString &);

  // create lagrangian mesh/fields
  vtkMultiBlockDataSet *MakeLagrangianMesh();

  // create point/face/cell zones
  vtkFoamDict *GatherBlocks(const char *, bool);
  bool GetPointZoneMesh(vtkMultiBlockDataSet *, vtkPoints *);
  bool GetFaceZoneMesh(vtkMultiBlockDataSet *, const vtkFoamIntVectorVector *,
      vtkPoints *);
  bool GetCellZoneMesh(vtkMultiBlockDataSet *, const vtkFoamIntVectorVector *,
      const vtkFoamIntVectorVector *, vtkPoints *);
};

vtkStandardNewMacro(vtkOpenFOAMReaderPrivate);

//-----------------------------------------------------------------------------
// struct vtkFoamIntVectorVector
struct vtkFoamIntVectorVector
{
private:
  vtkIntArray *Indices, *Body;

public:
  ~vtkFoamIntVectorVector()
  {
    this->Indices->Delete();
    this->Body->Delete();
  }

  vtkFoamIntVectorVector(const vtkFoamIntVectorVector &ivv) :
    Indices(ivv.Indices), Body(ivv.Body)
  {
    this->Indices->Register(0); // vtkDataArrays do not have ShallowCopy
    this->Body->Register(0);
  }
  vtkFoamIntVectorVector() :
    Indices(vtkIntArray::New()), Body(vtkIntArray::New())
  {
  }
  vtkFoamIntVectorVector(const int nElements, const int bodyLength) :
    Indices(vtkIntArray::New()), Body(vtkIntArray::New())
  {
    this->Indices->SetNumberOfValues(nElements + 1);
    this->Body->SetNumberOfValues(bodyLength);
  }

  // note that vtkIntArray::Resize() allocates (current size + new
  // size) bytes if current size < new size until 2010-06-27
  // cf. commit c869c3d5875f503e757b64f2fd1ec349aee859bf
  void ResizeBody(const int bodyLength)
  {
    this->Body->Resize(bodyLength);
  }
  int *WritePointer(const int i, const int bodyI, const int number)
  {
    return this->Body->WritePointer(*this->Indices->GetPointer(i) = bodyI,
        number);
  }
  int *SetIndex(const int i, const int bodyI)
  {
    return this->Body->GetPointer(*this->Indices->GetPointer(i) = bodyI);
  }
  void SetValue(const int bodyI, int value)
  {
    this->Body->SetValue(bodyI, value);
  }
  void InsertValue(const int bodyI, int value)
  {
    this->Body->InsertValue(bodyI, value);
  }
  const int *operator[](const int i) const
  {
    return this->Body->GetPointer(this->Indices->GetValue(i));
  }
  int GetSize(const int i) const
  {
    return this->Indices->GetValue(i + 1) - this->Indices->GetValue(i);
  }
  int GetNumberOfElements() const
  {
    return this->Indices->GetNumberOfTuples() - 1;
  }
  vtkIntArray *GetIndices()
  {
    return this->Indices;
  }
  vtkIntArray *GetBody()
  {
    return this->Body;
  }
};

//-----------------------------------------------------------------------------
// class vtkFoamError
// class for exception-carrying object
struct vtkFoamError : public vtkStdString
{
private:
  typedef vtkStdString Superclass;

public:
  vtkFoamError() :
    vtkStdString()
  {
  }
  vtkFoamError(const vtkFoamError& e) :
    vtkStdString(e)
  {
  }
  ~vtkFoamError()
  {
  }
  // a super-easy way to make use of operator<<()'s defined in
  // vtksys_ios::ostringstream class
  template <class T> vtkFoamError& operator<<(const T& t)
  {
    vtksys_ios::ostringstream os;
    os << t;
    this->Superclass::operator+=(os.str());
    return *this;
  }
};

//-----------------------------------------------------------------------------
// class vtkFoamToken
// token class which also works as container for list types
// - a word token is treated as a string token for simplicity
// - handles only atomic types. Handling of list types are left to the
//   derived classes.
struct vtkFoamToken
{
public:
  enum tokenType
    {
    // undefined type
    UNDEFINED,
    // atomic types
    PUNCTUATION, LABEL, SCALAR, STRING, IDENTIFIER,
    // vtkObject-derived list types
    STRINGLIST, LABELLIST, SCALARLIST, VECTORLIST,
    // original list types
    LABELLISTLIST, ENTRYVALUELIST, EMPTYLIST, DICTIONARY,
    // error state
    TOKEN_ERROR
    };

protected:
  tokenType Type;
  union
  {
    char Char;
    int Int;
    double Double;
    vtkStdString* String;
    vtkObjectBase *VtkObjectPtr;
    // vtkObject-derived list types
    vtkIntArray *LabelListPtr;
    vtkFloatArray *ScalarListPtr, *VectorListPtr;
    vtkStringArray *StringListPtr;
    // original list types
    vtkFoamIntVectorVector *LabelListListPtr;
    vtkstd::vector<vtkFoamEntryValue*> *EntryValuePtrs;
    vtkFoamDict *DictPtr;
  };

  void Clear()
  {
    if (this->Type == STRING || this->Type == IDENTIFIER)
      {
      delete this->String;
      }
  }

  void AssignData(const vtkFoamToken& value)
  {
    switch (value.Type)
      {
      case PUNCTUATION:
        this->Char = value.Char;
        break;
      case LABEL:
        this->Int = value.Int;
        break;
      case SCALAR:
        this->Double = value.Double;
        break;
      case STRING:
      case IDENTIFIER:
        this->String = new vtkStdString(*value.String);
        break;
        // required to suppress the 'enumeration value not handled' warning by
        // g++ when compiled with -Wall
      default:
        break;
      }
  }

public:
  vtkFoamToken() :
    Type(UNDEFINED)
  {
  }
  vtkFoamToken(const vtkFoamToken& value) :
    Type(value.Type)
  {
    this->AssignData(value);
  }
  ~vtkFoamToken()
  {
    this->Clear();
  }

  tokenType GetType() const
  {
    return this->Type;
  }

  template <typename T> bool Is() const;
  template <typename T> T To() const;
#if defined(_MSC_VER)
  // workaround for Win32-64ids-nmake70
  VTK_TEMPLATE_SPECIALIZE bool Is<int>() const;
  VTK_TEMPLATE_SPECIALIZE bool Is<float>() const;
  VTK_TEMPLATE_SPECIALIZE bool Is<double>() const;
  VTK_TEMPLATE_SPECIALIZE int To<int>() const;
  VTK_TEMPLATE_SPECIALIZE float To<float>() const;
  VTK_TEMPLATE_SPECIALIZE double To<double>() const;
#endif

  // workaround for SunOS-CC5.6-dbg
  int ToInt() const
  {
    return this->Int;
  }

  // workaround for SunOS-CC5.6-dbg
  float ToFloat() const
  {
    return this->Type == LABEL ? this->Int : this->Double;
  }

  const vtkStdString ToString() const
  {
    return *this->String;
  }
  const vtkStdString ToIdentifier() const
  {
    return *this->String;
  }

  void SetBad()
  {
    this->Clear();
    this->Type = TOKEN_ERROR;
  }
  void SetIdentifier(const vtkStdString& idString)
  {
    this->operator=(idString);
    this->Type = IDENTIFIER;
  }

  void operator=(const char value)
  {
    this->Clear();
    this->Type = PUNCTUATION;
    this->Char = value;
  }
  void operator=(const int value)
  {
    this->Clear();
    this->Type = LABEL;
    this->Int = value;
  }
  void operator=(const double value)
  {
    this->Clear();
    this->Type = SCALAR;
    this->Double = value;
  }
  void operator=(const char *value)
  {
    this->Clear();
    this->Type = STRING;
    this->String = new vtkStdString(value);
  }
  void operator=(const vtkStdString& value)
  {
    this->Clear();
    this->Type = STRING;
    this->String = new vtkStdString(value);
  }
  void operator=(const vtkFoamToken& value)
  {
    this->Clear();
    this->Type = value.Type;
    this->AssignData(value);
  }
  bool operator==(const char value) const
  {
    return this->Type == PUNCTUATION && this->Char == value;
  }
  bool operator==(const int value) const
  {
    return this->Type == LABEL && this->Int == value;
  }
  bool operator==(const vtkStdString& value) const
  {
    return this->Type == STRING && *this->String == value;
  }
  bool operator!=(const vtkStdString& value) const
  {
    return this->Type != STRING || *this->String != value;
  }
  bool operator!=(const char value) const
  {
    return !this->operator==(value);
  }

  friend vtksys_ios::ostringstream& operator<<(vtksys_ios::ostringstream& str,
      const vtkFoamToken& value)
  {
    switch (value.GetType())
      {
      case TOKEN_ERROR:
        str << "badToken (an unexpected EOF?)";
        break;
      case PUNCTUATION:
        str << value.Char;
        break;
      case LABEL:
        str << value.Int;
        break;
      case SCALAR:
        str << value.Double;
        break;
      case STRING:
      case IDENTIFIER:
        str << *value.String;
        break;
        // required to suppress the 'enumeration value not handled' warning by
        // g++ when compiled with -Wall
      default:
        break;
      }
    return str;
  }
};

VTK_TEMPLATE_SPECIALIZE inline bool vtkFoamToken::Is<int>() const
{
  return this->Type == LABEL;
}

VTK_TEMPLATE_SPECIALIZE inline bool vtkFoamToken::Is<float>() const
{
  return this->Type == LABEL || this->Type == SCALAR;
}

VTK_TEMPLATE_SPECIALIZE inline bool vtkFoamToken::Is<double>() const
{
  return this->Type == SCALAR;
}

VTK_TEMPLATE_SPECIALIZE inline int vtkFoamToken::To<int>() const
{
  return this->Int;
}

VTK_TEMPLATE_SPECIALIZE inline float vtkFoamToken::To<float>() const
{
  return this->Type == LABEL ? this->Int : this->Double;
}

VTK_TEMPLATE_SPECIALIZE inline double vtkFoamToken::To<double>() const
{
  return this->Type == LABEL ? this->Int : this->Double;
}

//-----------------------------------------------------------------------------
// class vtkFoamFileStack
// list of variables that have to be saved when a file is included.
struct vtkFoamFileStack
{
protected:
  vtkStdString FileName;
  FILE *File;
  bool IsCompressed;
  z_stream Z;
  int ZStatus;
  int LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
  bool WasNewline;
#endif

  // buffer pointers. using raw pointers for performance reason.
  unsigned char *Inbuf;
  unsigned char *Outbuf;
  unsigned char *BufPtr;
  unsigned char *BufEndPtr;

  vtkFoamFileStack() :
    FileName(), File(NULL), IsCompressed(false), ZStatus(Z_OK), LineNumber(0),
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        WasNewline(true),
#endif
        Inbuf(NULL), Outbuf(NULL), BufPtr(NULL), BufEndPtr(NULL)
  {
    this->Z.zalloc = Z_NULL;
    this->Z.zfree = Z_NULL;
    this->Z.opaque = Z_NULL;
  }

  void Reset()
  {
    // this->FileName = "";
    this->File = NULL;
    this->IsCompressed = false;
    // this->ZStatus = Z_OK;
    this->Z.zalloc = Z_NULL;
    this->Z.zfree = Z_NULL;
    this->Z.opaque = Z_NULL;
    // this->LineNumber = 0;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
    this->WasNewline = true;
#endif

    this->Inbuf = NULL;
    this->Outbuf = NULL;
    // this->BufPtr = NULL;
    // this->BufEndPtr = NULL;
  }

public:
  const vtkStdString& GetFileName() const
  {
    return this->FileName;
  }
  int GetLineNumber() const
  {
    return this->LineNumber;
  }
};

//-----------------------------------------------------------------------------
// class vtkFoamFile
// read and tokenize the input.
struct vtkFoamFile : public vtkFoamFileStack
{
private:
  typedef vtkFoamFileStack Superclass;

public:
  // #inputMode values
  enum inputModes
    {
        INPUT_MODE_MERGE,
        INPUT_MODE_OVERWRITE,
        INPUT_MODE_PROTECT,
        INPUT_MODE_WARN,
        INPUT_MODE_ERROR
    };

private:
  bool Is13Positions;
  inputModes InputMode;

  // inclusion handling
  vtkFoamFileStack *Stack[VTK_FOAMFILE_INCLUDE_STACK_SIZE];
  int StackI;
  vtkStdString CasePath;

  // declare and define as private
  vtkFoamFile();
  bool InflateNext(unsigned char *buf, int requestSize);
  int NextTokenHead();
  // hacks to keep exception throwing / recursive codes out-of-line to make
  // putBack(), getc() and readExpecting() inline expandable
  void ThrowDuplicatedPutBackException();
  void ThrowUnexpectedEOFException();
  void ThrowUnexpectedNondigitCharExecption(const int c);
  void ThrowUnexpectedTokenException(const char, const int c);
  int ReadNext();

  void PutBack(const int c)
  {
    if (--this->Superclass::BufPtr < this->Superclass::Outbuf)
      {
      this->ThrowDuplicatedPutBackException();
      }
    *this->Superclass::BufPtr = c;
  }

  // get a character
  int Getc()
  {
    return this->Superclass::BufPtr == this->Superclass::BufEndPtr ? this->ReadNext()
        : *this->Superclass::BufPtr++;
  }

  vtkFoamError StackString()
  {
    vtksys_ios::ostringstream os;
    if (this->StackI > 0)
      {
      os << "\n included";

      for (int stackI = this->StackI - 1; stackI >= 0; stackI--)
        {
        os << " from line " << this->Stack[stackI]->GetLineNumber() << " of "
            << this->Stack[stackI]->GetFileName() << "\n";
        }
      os << ": ";
      }
    return vtkFoamError() << os.str();
  }

  bool CloseIncludedFile()
  {
    if (this->StackI == 0)
      {
      return false;
      }
    this->Clear();
    this->StackI--;
    // use the default bitwise assignment operator
    this->Superclass::operator=(*this->Stack[this->StackI]);
    delete this->Stack[this->StackI];
    return true;
  }

  void Clear()
  {
    if (this->Superclass::IsCompressed)
      {
      inflateEnd(&this->Superclass::Z);
      }

    delete [] this->Superclass::Inbuf;
    delete [] this->Superclass::Outbuf;
    this->Superclass::Inbuf = this->Superclass::Outbuf = NULL;

    if (this->Superclass::File)
      {
      fclose(this->Superclass::File);
      this->Superclass::File = NULL;
      }
    // don't reset the line number so that the last line number is
    // retained after close
    // lineNumber_ = 0;
  }

  //! Return file name (part beyond last /)
  vtkStdString ExtractName(const vtkStdString& path) const
  {
#if defined(_WIN32)
    const vtkStdString pathFindSeparator = "/\\", pathSeparator = "\\";
#else
    const vtkStdString pathFindSeparator = "/", pathSeparator = "/";
#endif
    vtkStdString::size_type pos = path.find_last_of(pathFindSeparator);
    if (pos == vtkStdString::npos)
      {
      // no slash
      return path;
      }
    else if (pos+1 == path.size())
      {
      // final trailing slash
      vtkStdString::size_type endPos = pos;
      pos = path.find_last_of(pathFindSeparator, pos-1);
      if (pos == vtkStdString::npos)
        {
        // no further slash
        return path.substr(0, endPos);
        }
      else
        {
        return path.substr(pos + 1, endPos - pos - 1);
        }
      }
    else
      {
      return path.substr(pos + 1, vtkStdString::npos);
      }
  }

  //! Return directory path name (part before last /)
  vtkStdString ExtractPath(const vtkStdString& path) const
  {
#if defined(_WIN32)
    const vtkStdString pathFindSeparator = "/\\", pathSeparator = "\\";
#else
    const vtkStdString pathFindSeparator = "/", pathSeparator = "/";
#endif
    const vtkStdString::size_type pos = path.find_last_of(pathFindSeparator);
    return pos == vtkStdString::npos
        ? vtkStdString(".") + pathSeparator
        : path.substr(0, pos + 1);
  }


public:
  vtkFoamFile(const vtkStdString& casePath) :
    vtkFoamFileStack(), Is13Positions(false), InputMode(INPUT_MODE_ERROR),
        StackI(0), CasePath(casePath)
  {
  }
  ~vtkFoamFile()
  {
    this->Close();
  }

  void SetIs13Positions(const bool is13Positions)
  {
    this->Is13Positions = is13Positions;
  }
  bool GetIs13Positions() const
  {
    return this->Is13Positions;
  }
  inputModes GetInputMode() const
  {
    return this->InputMode;
  }
  const vtkStdString GetCasePath() const
  {
    return this->CasePath;
  }
  const vtkStdString GetFilePath() const
  {
    return this->ExtractPath(this->FileName);
  }

  vtkStdString ExpandPath(const vtkStdString& pathIn,
      const vtkStdString& defaultPath)
  {
    vtkStdString expandedPath;
    bool isExpanded = false, wasPathSeparator = true;
    const size_t nChars = pathIn.length();
    for (size_t charI = 0; charI < nChars;)
      {
      char c = pathIn[charI];
      switch (c)
        {
        case '$': // $-variable expansion
          {
          vtkStdString variable;
          while (++charI < nChars && (isalnum(pathIn[charI]) || pathIn[charI]
              == '_'))
            {
            variable += pathIn[charI];
            }
          if (variable == "FOAM_CASE") // discard path until the variable
            {
            expandedPath = this->CasePath;
            wasPathSeparator = true;
            isExpanded = true;
            }
          else if (variable == "FOAM_CASENAME")
            {
            // FOAM_CASENAME is the final directory name from CasePath
            expandedPath += this->ExtractName(this->CasePath);
            wasPathSeparator = false;
            isExpanded = true;
            }
          else
            {
            const char *value = getenv(variable.c_str());
            if (value != NULL)
              {
              expandedPath += value;
              }
            const vtkStdString::size_type len = expandedPath.length();
            if (len > 0)
              {
              const char c2 = expandedPath[len - 1];
              wasPathSeparator = (c2 == '/' || c2 == '\\');
              }
            else
              {
              wasPathSeparator = false;
              }
            }
          }
          break;
        case '~': // home directory expansion
          // not using vtksys::SystemTools::ConvertToUnixSlashes() for
          // a bit better handling of "~"
          if (wasPathSeparator)
            {
            vtkStdString userName;
            while (++charI < nChars && (pathIn[charI] != '/' && pathIn[charI]
                != '\\') && pathIn[charI] != '$')
              {
              userName += pathIn[charI];
              }
            if (userName == "")
              {
              const char *homePtr = getenv("HOME");
              if (homePtr == NULL)
                {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
                expandedPath = "";
#else
                const struct passwd *pwentry = getpwuid(getuid());
                if (pwentry == NULL)
                  {
                  throw this->StackString() << "Home directory path not found";
                  }
                expandedPath = pwentry->pw_dir;
#endif
                }
              else
                {
                expandedPath = homePtr;
                }
              }
            else
              {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
              const char *homePtr = getenv("HOME");
              expandedPath
              = this->ExtractPath(homePtr ? homePtr : "") + userName;
#else
              if (userName == "OpenFOAM")
                {
                // so far only "~/.OpenFOAM" expansion is supported
                const char *homePtr = getenv("HOME");
                if (homePtr == NULL)
                  {
                  expandedPath = "";
                  }
                else
                  {
                  expandedPath = vtkStdString(homePtr) + "/.OpenFOAM";
                  }
                }
              else
                {
                const struct passwd *pwentry = getpwnam(userName.c_str());
                if (pwentry == NULL)
                  {
                  throw this->StackString() << "Home directory for user "
                  << userName.c_str() << " not found";
                  }
                expandedPath = pwentry->pw_dir;
                }
#endif
              }
            wasPathSeparator = false;
            isExpanded = true;
            break;
            }
          // fall through
        default:
          wasPathSeparator = (c == '/' || c == '\\');
          expandedPath += c;
          charI++;
        }
      }
    if (isExpanded || expandedPath.substr(0, 1) == "/" || expandedPath.substr(
        0, 1) == "\\")
      {
      return expandedPath;
      }
    else
      {
      return defaultPath + expandedPath;
      }
  }

  void IncludeFile(const vtkStdString& includedFileName,
      const vtkStdString& defaultPath)
  {
    if (this->StackI >= VTK_FOAMFILE_INCLUDE_STACK_SIZE)
      {
      throw this->StackString() << "Exceeded maximum #include recursions of "
      << VTK_FOAMFILE_INCLUDE_STACK_SIZE;
      }
    // use the default bitwise copy constructor
    this->Stack[this->StackI++] = new vtkFoamFileStack(*this);
    this->Superclass::Reset();

    this->Open(this->ExpandPath(includedFileName, defaultPath));
  }

  // the tokenizer
  // returns true if success, false if encountered EOF
  bool Read(vtkFoamToken& token)
  {
    // expanded the outermost loop in nextTokenHead() for performance
    int c;
    while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
      {
      if (c == '\n')
        {
        ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
        }
      }
    if (c == 47) // '/' == 47
      {
      this->PutBack(c);
      c = this->NextTokenHead();
      }
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
    if(c != '#')
      {
      this->Superclass::WasNewline = false;
      }
#endif

    const int MAXLEN = 1024;
    char buf[MAXLEN + 1];
    int charI = 0;
    switch (c)
      {
      case '(':
      case ')':
        // high-priority punctuation token
        token = static_cast<char>(c);
        return true;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '0':
      case '-':
        // undetermined number token
        do
          {
          buf[charI++] = c;
          } while (isdigit(c = this->Getc()) && charI < MAXLEN);
        if (c != '.' && c != 'e' && c != 'E' && charI < MAXLEN && c != EOF)
          {
          // label token
          buf[charI] = '\0';
          token = static_cast<int>(strtol(buf, NULL, 10));
          this->PutBack(c);
          return true;
          }
        // fall through
      case '.':
        // scalar token
        if (c == '.' && charI < MAXLEN)
          {
          // read decimal fraction part
          buf[charI++] = c;
          while (isdigit(c = this->Getc()) && charI < MAXLEN)
            {
            buf[charI++] = c;
            }
          }
        if ((c == 'e' || c == 'E') && charI < MAXLEN)
          {
          // read exponent part
          buf[charI++] = c;
          if (((c = this->Getc()) == '+' || c == '-') && charI < MAXLEN)
            {
            buf[charI++] = c;
            c = this->Getc();
            }
          while (isdigit(c) && charI < MAXLEN)
            {
            buf[charI++] = c;
            c = this->Getc();
            }
          }
        if (charI == 1 && buf[0] == '-')
          {
          token = '-';
          this->PutBack(c);
          return true;
          }
        buf[charI] = '\0';
        token = strtod(buf, NULL);
        this->PutBack(c);
        break;
      case ';':
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':
      case '=':
      case '+':
      case '*':
      case '/':
        // low-priority punctuation token
        token = static_cast<char>(c);
        return true;
      case '"':
        {
        // string token
        bool wasEscape = false;
        while ((c = this->Getc()) != EOF && charI < MAXLEN)
          {
          if (c == '\\' && !wasEscape)
            {
            wasEscape = true;
            continue;
            }
          else if (c == '"' && !wasEscape)
            {
            break;
            }
          else if (c == '\n')
            {
            ++this->Superclass::LineNumber;
            if (!wasEscape)
              {
              throw this->StackString()
              << "Unescaped newline in string constant";
              }
            }
          buf[charI++] = c;
          wasEscape = false;
          }
        buf[charI] = '\0';
        token = buf;
        }
        break;
      case EOF:
        // end of file
        token.SetBad();
        return false;
      case '$':
        {
        vtkFoamToken identifierToken;
        if (!this->Read(identifierToken))
          {
          throw this->StackString() << "Unexpected EOF reading identifier";
          }
        if (identifierToken.GetType() != vtkFoamToken::STRING)
          {
          throw this->StackString() << "Expected a word, found "
          << identifierToken;
          }
        token.SetIdentifier(identifierToken.ToString());
        return true;
        }
      case '#':
        {
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        // the OpenFOAM #-directives can indeed be placed in the
        // middle of a line
        if(!this->Superclass::WasNewline)
          {
          throw this->StackString()
          << "Encountered #-directive in the middle of a line";
          }
        this->Superclass::WasNewline = false;
#endif
        // read directive
        vtkFoamToken directiveToken;
        if (!this->Read(directiveToken))
          {
          throw this->StackString() << "Unexpected EOF reading directive";
          }
        if (directiveToken == "include")
          {
          vtkFoamToken fileNameToken;
          if (!this->Read(fileNameToken))
            {
            throw this->StackString() << "Unexpected EOF reading filename";
            }
          this->IncludeFile(fileNameToken.ToString(),
              this->ExtractPath(this->FileName));
          }
        else if (directiveToken == "includeIfPresent")
          {
          vtkFoamToken fileNameToken;
          if (!this->Read(fileNameToken))
            {
            throw this->StackString() << "Unexpected EOF reading filename";
            }

          // special treatment since the file is allowed to be missing
          const vtkStdString fullName =
            this->ExpandPath(fileNameToken.ToString(),
                this->ExtractPath(this->FileName));

          FILE *fh = fopen(fullName.c_str(), "rb");
          if (fh)
            {
            fclose(fh);

            this->IncludeFile(fileNameToken.ToString(),
                 this->ExtractPath(this->FileName));
            }
          }
        else if (directiveToken == "inputMode")
          {
          vtkFoamToken modeToken;
          if (!this->Read(modeToken))
            {
            throw this->StackString()
            << "Unexpected EOF reading inputMode specifier";
            }
          if (modeToken == "merge" || modeToken == "default")
            {
            this->InputMode = INPUT_MODE_MERGE;
            }
          else if (modeToken == "overwrite")
            {
            this->InputMode = INPUT_MODE_OVERWRITE;
            }
          else if (modeToken == "protect")
            {
            // not properly supported - treat like "merge" for now
            // this->InputMode = INPUT_MODE_PROTECT;
            this->InputMode = INPUT_MODE_MERGE;
            }
          else if (modeToken == "warn")
            {
            // not properly supported - treat like "error" for now
            // this->InputMode = INPUT_MODE_WARN;
            this->InputMode = INPUT_MODE_ERROR;
            }
          else if (modeToken == "error")
            {
            this->InputMode = INPUT_MODE_ERROR;
            }
          else
            {
            throw this->StackString() << "Expected one of inputMode specifiers "
            "(merge, overwrite, protect, warn, error, default), found " << modeToken;
            }
          }
        else
          {
          throw this->StackString() << "Unsupported directive "
          << directiveToken;
          }
        return this->Read(token);
        }
      default:
        // parses as a word token, but gives the STRING type for simplicity
        int inBrace = 0;
        do
          {
          if (c == '(')
            {
            inBrace++;
            }
          else if (c == ')' && --inBrace == -1)
            {
            break;
            }
          buf[charI++] = c;
          // valid characters that constitutes a word
          // cf. src/OpenFOAM/primitives/strings/word/wordI.H
          } while ((c = this->Getc()) != EOF && !isspace(c) && c != '"' && c
            != '/' && c != ';' && c != '{' && c != '}' && charI < MAXLEN);
        buf[charI] = '\0';
        token = buf;
        this->PutBack(c);
      }

    if (c == EOF)
      {
      this->ThrowUnexpectedEOFException();
      }
    if (charI == MAXLEN)
      {
      throw this->StackString() << "Exceeded maximum allowed length of "
      << MAXLEN << " chars";
      }
    return true;
  }

  void Open(const vtkStdString& fileName)
  {
    // reset line number to indicate the beginning of the file when an
    // exception is thrown
    this->Superclass::LineNumber = 0;
    this->Superclass::FileName = fileName;

    if (this->Superclass::File)
      {
      throw this->StackString() << "File already opened within this object";
      }

    if ((this->Superclass::File = fopen(this->Superclass::FileName.c_str(),
        "rb")) == NULL)
      {
      throw this->StackString() << "Can't open";
      }

    unsigned char zMagic[2];
    if (fread(zMagic, 1, 2, this->Superclass::File) == 2 && zMagic[0] == 0x1f
        && zMagic[1] == 0x8b)
      {
      // gzip-compressed format
      this->Superclass::Z.avail_in = 0;
      this->Superclass::Z.next_in = Z_NULL;
      // + 32 to automatically recognize gzip format
      if (inflateInit2(&this->Superclass::Z, 15 + 32) == Z_OK)
        {
        this->Superclass::IsCompressed = true;
        this->Superclass::Inbuf = new unsigned char[VTK_FOAMFILE_INBUFSIZE];
        }
      else
        {
        fclose(this->Superclass::File);
        this->Superclass::File = NULL;
        throw this->StackString() << "Can't init zstream "
        << (this->Superclass::Z.msg ? this->Superclass::Z.msg : "");
        }
      }
    else
      {
      // uncompressed format
      this->Superclass::IsCompressed = false;
      }
    rewind(this->Superclass::File);

    this->Superclass::ZStatus = Z_OK;
    this->Superclass::Outbuf = new unsigned char[VTK_FOAMFILE_OUTBUFSIZE + 1];
    this->Superclass::BufPtr = this->Superclass::Outbuf + 1;
    this->Superclass::BufEndPtr = this->Superclass::BufPtr;
    this->Superclass::LineNumber = 1;
  }

  void Close()
  {
    while (this->CloseIncludedFile())
      ;
    this->Clear();
  }

  // gzread with buffering handling
  int Read(unsigned char *buf, const int len)
  {
    int readlen;
    const int buflen = this->Superclass::BufEndPtr - this->Superclass::BufPtr;
    if (len > buflen)
      {
      memcpy(buf, this->Superclass::BufPtr, buflen);
      readlen = this->InflateNext(buf + buflen, len - buflen);
      if (readlen >= 0)
        {
        readlen += buflen;
        }
      else
        {
        if (buflen == 0) // return EOF
          {
          readlen = -1;
          }
        else
          {
          readlen = buflen;
          }
        }
      this->Superclass::BufPtr = this->Superclass::BufEndPtr;
      }
    else
      {
      memcpy(buf, this->Superclass::BufPtr, len);
      this->Superclass::BufPtr += len;
      readlen = len;
      }
    for (int i = 0; i < readlen; i++)
      {
      if (buf[i] == '\n')
        {
        this->Superclass::LineNumber++;
        }
      }
    return readlen;
  }

  void ReadExpecting(const char expected)
  {
    // skip prepending invalid chars
    // expanded the outermost loop in nextTokenHead() for performance
    int c;
    while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
      {
      if (c == '\n')
        {
        ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
        }
      }
    if (c == 47) // '/' == 47
      {
      this->PutBack(c);
      c = this->NextTokenHead();
      }
    if (c != expected)
      {
      this->ThrowUnexpectedTokenException(expected, c);
      }
  }

  void ReadExpecting(const char* str)
  {
    vtkFoamToken t;
    if (!this->Read(t) || t != str)
      {
      throw this->StackString() << "Expected string \"" << str << "\", found "
      << t;
      }
  }

  int ReadIntValue();
  float ReadFloatValue();
};

int vtkFoamFile::ReadNext()
{
  if (!this->InflateNext(this->Superclass::Outbuf + 1, VTK_FOAMFILE_OUTBUFSIZE))
    {
    return this->CloseIncludedFile() ? this->Getc() : EOF;
    }
  return *this->Superclass::BufPtr++;
}

// specialized for reading an integer value.
// not using the standard strtol() for speed reason.
int vtkFoamFile::ReadIntValue()
{
  // skip prepending invalid chars
  // expanded the outermost loop in nextTokenHead() for performance
  int c;
  while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
    {
    if (c == '\n')
      {
      ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
      }
    }
  if (c == 47) // '/' == 47
    {
    this->PutBack(c);
    c = this->NextTokenHead();
    }

  int nonNegative = c - 45; // '-' == 45
  if (nonNegative == 0 || c == 43) // '+' == 43
    {
    c = this->Getc();
    if (c == '\n')
      {
      ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
      }
    }

  if (!isdigit(c)) // isdigit() accepts -1 as EOF
    {
    if (c == EOF)
      {
      this->ThrowUnexpectedEOFException();
      }
    else
      {
      this->ThrowUnexpectedNondigitCharExecption(c);
      }
    }

  int num = c - 48; // '0' == 48
  while (isdigit(c = this->Getc()))
    {
    num = 10 * num + c - 48;
    }

  if (c == EOF)
    {
    this->ThrowUnexpectedEOFException();
    }
  this->PutBack(c);

  return nonNegative ? num : -num;
}

// extreamely simplified high-performing string to floating point
// conversion code based on
// ParaView3/VTK/Utilities/vtksqlite/vtk_sqlite3.c
float vtkFoamFile::ReadFloatValue()
{
  // skip prepending invalid chars
  // expanded the outermost loop in nextTokenHead() for performance
  int c;
  while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
    {
    if (c == '\n')
      {
      ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
      }
    }
  if (c == 47) // '/' == 47
    {
    this->PutBack(c);
    c = this->NextTokenHead();
    }

  // determine sign
  int nonNegative = c - 45; // '-' == 45
  if (nonNegative == 0 || c == 43) // '+' == 43
    {
    c = this->Getc();
    if (c == '\n')
      {
      ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
      this->Superclass::WasNewline = true;
#endif
      }
    }

  if (!isdigit(c) && c != 46) // '.' == 46, isdigit() accepts EOF
    {
    this->ThrowUnexpectedNondigitCharExecption(c);
    }

  // read integer part
  double num = c - 48; // '0' == 48
  while (isdigit(c = this->Getc()))
    {
    num = num * 10.0 + (c - 48);
    }

  // read decimal part
  if (c == 46) // '.'
    {
    double divisor = 1.0;

    while (isdigit(c = this->Getc()))
      {
      num = num * 10.0 + (c - 48);
      divisor *= 10.0;
      }
    num /= divisor;
    }

  // read exponent part
  if (c == 69 || c == 101) // 'E' == 69, 'e' == 101
    {
    int esign = 1;
    int eval = 0;
    double scale = 1.0;

    c = this->Getc();
    if (c == 45) // '-'
      {
      esign = -1;
      c = this->Getc();
      }
    else if (c == 43) // '+'
      {
      c = this->Getc();
      }

    while (isdigit(c))
      {
      eval = eval * 10 + (c - 48);
      c = this->Getc();
      }

    // fast exponent multiplication!
    while (eval >= 64)
      {
      scale *= 1.0e+64;
      eval -= 64;
      }
    while (eval >= 16)
      {
      scale *= 1.0e+16;
      eval -= 16;
      }
    while (eval >= 4)
      {
      scale *= 1.0e+4;
      eval -= 4;
      }
    while (eval >= 1)
      {
      scale *= 1.0e+1;
      eval -= 1;
      }

    if (esign < 0)
      {
      num /= scale;
      }
    else
      {
      num *= scale;
      }
    }

  if (c == EOF)
    {
    this->ThrowUnexpectedEOFException();
    }
  this->PutBack(c);

  return static_cast<float>(nonNegative ? num : -num);
}

// hacks to keep exception throwing code out-of-line to make
// putBack() and readExpecting() inline expandable
void vtkFoamFile::ThrowUnexpectedEOFException()
{
  throw this->StackString() << "Unexpected EOF";
}

void vtkFoamFile::ThrowUnexpectedNondigitCharExecption(const int c)
{
  throw this->StackString() << "Expected a number, found a non-digit character "
  << static_cast<char>(c);
}

void vtkFoamFile::ThrowUnexpectedTokenException(const char expected, const int c)
{
  vtkFoamError sstr;
  sstr << this->StackString() << "Expected punctuation token '" << expected
      << "', found ";
  if (c == EOF)
    {
    sstr << "EOF";
    }
  else
    {
    sstr << static_cast<char>(c);
    }
  throw sstr;
}

void vtkFoamFile::ThrowDuplicatedPutBackException()
{
  throw this->StackString() << "Attempted duplicated putBack()";
}

bool vtkFoamFile::InflateNext(unsigned char *buf,
    int requestSize)
{
  size_t size;
  if (this->Superclass::IsCompressed)
    {
    if (this->Superclass::ZStatus != Z_OK)
      {
      return false;
      }
    this->Superclass::Z.next_out = buf;
    this->Superclass::Z.avail_out = requestSize;

    do
      {
      if (this->Superclass::Z.avail_in == 0)
        {
        this->Superclass::Z.next_in = this->Superclass::Inbuf;
        this->Superclass::Z.avail_in = static_cast<uInt>(fread(this->Superclass::Inbuf, 1,
          VTK_FOAMFILE_INBUFSIZE, this->Superclass::File));
        if (ferror(this->Superclass::File))
          {
          throw this->StackString() << "Fread failed";
          }
        }
      this->Superclass::ZStatus = inflate(&this->Superclass::Z, Z_NO_FLUSH);
      if (this->Superclass::ZStatus == Z_STREAM_END
#if VTK_FOAMFILE_OMIT_CRCCHECK
      // the dummy CRC function causes data error when finalizing
      // so we have to proceed even when a data error is detected
      || this->Superclass::ZStatus == Z_DATA_ERROR
#endif
      )
        {
        break;
        }
      if (this->Superclass::ZStatus != Z_OK)
        {
        throw this->StackString() << "Inflation failed: "
        << (this->Superclass::Z.msg ? this->Superclass::Z.msg : "");
        }
      } while (this->Superclass::Z.avail_out > 0);

    size = requestSize - this->Superclass::Z.avail_out;
    }
  else
    {
    // not compressed
    size = fread(buf, 1, requestSize, this->Superclass::File);
    }

  if (size <= 0)
    {
    // retain the current location bufPtr_ to the end of the buffer so that
    // getc() returns EOF again when called next time
    return false;
    }
  // size > 0
  // reserve the first byte for getback char
  this->Superclass::BufPtr = this->Superclass::Outbuf + 1;
  this->Superclass::BufEndPtr = this->Superclass::BufPtr + size;
  return true;
}

// get next semantically valid character
int vtkFoamFile::NextTokenHead()
{
  for (;;)
    {
    int c;
    while (isspace(c = this->Getc())) // isspace() accepts -1 as EOF
      {
      if (c == '\n')
        {
        ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
        }
      }
    if (c == '/')
      {
      if ((c = this->Getc()) == '/')
        {
        while ((c = this->Getc()) != EOF && c != '\n')
          ;
        if (c == EOF)
          {
          return c;
          }
        ++this->Superclass::LineNumber;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        this->Superclass::WasNewline = true;
#endif
        }
      else if (c == '*')
        {
        for (;;)
          {
          while ((c = this->Getc()) != EOF && c != '*')
            {
            if (c == '\n')
              {
              ++this->Superclass::LineNumber;
              }
            }
          if (c == EOF)
            {
            return c;
            }
          else if ((c = this->Getc()) == '/')
            {
            break;
            }
          this->PutBack(c);
          }
        }
      else
        {
        this->PutBack(c); // may be an EOF
        return '/';
        }
      }
    else // may be an EOF
      {
      return c;
      }
    }
#if defined(__hpux)
  return EOF; // this line should not be executed; workaround for HP-UXia64-aCC
#endif
}

//-----------------------------------------------------------------------------
// class vtkFoamIOobject
// holds file handle, file format, name of the object the file holds and
// type of the object.
struct vtkFoamIOobject : public vtkFoamFile
{
private:
  typedef vtkFoamFile Superclass;

public:
  enum fileFormat
    {UNDEFINED, ASCII, BINARY};

private:
  fileFormat Format;
  vtkStdString ObjectName;
  vtkStdString HeaderClassName;
  vtkFoamError E;

  vtkFoamIOobject();
  void ReadHeader(); // defined later
public:
  vtkFoamIOobject(const vtkStdString& casePath) :
    vtkFoamFile(casePath), Format(UNDEFINED), E()
  {
  }
  ~vtkFoamIOobject()
  {
    this->Close();
  }

  bool Open(const vtkStdString& file)
  {
    try
      {
      this->Superclass::Open(file);
      }
    catch(vtkFoamError& e)
      {
      this->E = e;
      return false;
      }

    try
      {
      this->ReadHeader();
      }
    catch(vtkFoamError& e)
      {
      this->Superclass::Close();
      this->E = e;
      return false;
      }
    return true;
  }

  void Close()
  {
    this->Superclass::Close();
    this->Format = UNDEFINED;
    this->ObjectName.erase();
    this->HeaderClassName.erase();
    this->E.erase();
  }
  fileFormat GetFormat() const
  {
    return this->Format;
  }
  const vtkStdString& GetClassName() const
  {
    return this->HeaderClassName;
  }
  const vtkStdString& GetObjectName() const
  {
    return this->ObjectName;
  }
  const vtkFoamError& GetError() const
  {
    return this->E;
  }
  void SetError(const vtkFoamError& e)
  {
    this->E = e;
  }
};

//-----------------------------------------------------------------------------
// workarounding class for older compilers (gcc-3.3.x and possibly older)
template <typename T> struct vtkFoamReadValue
{
public:
  static T ReadValue(vtkFoamIOobject &io);
};

VTK_TEMPLATE_SPECIALIZE inline int vtkFoamReadValue<int>::ReadValue(vtkFoamIOobject& io)
{
  return io.ReadIntValue();
}

VTK_TEMPLATE_SPECIALIZE inline float vtkFoamReadValue<float>::ReadValue(vtkFoamIOobject& io)
{
  return io.ReadFloatValue();
}

//-----------------------------------------------------------------------------
// class vtkFoamEntryValue
// a class that represents a value of a dictionary entry that corresponds to
// its keyword. note that an entry can have more than one value.
struct vtkFoamEntryValue : public vtkFoamToken
{
private:
  typedef vtkFoamToken Superclass;

  bool IsUniform;
  bool Managed;
  const vtkFoamEntry *UpperEntryPtr;

  vtkFoamEntryValue();
  vtkObjectBase *ToVTKObject()
  {
    return this->Superclass::VtkObjectPtr;
  }
  void Clear();
  void ReadList(vtkFoamIOobject& io);

public:
  // reads primitive int/float lists
  template <typename listT, typename primitiveT> class listTraits
  {
    listT *Ptr;

public:
    listTraits() :
      Ptr(listT::New())
    {
    }
    listT *GetPtr()
    {
      return this->Ptr;
    }
    void ReadUniformValues(vtkFoamIOobject& io, const int size)
    {
      primitiveT value = vtkFoamReadValue<primitiveT>::ReadValue(io);
      for (int i = 0; i < size; i++)
        {
        this->Ptr->SetValue(i, value);
        }
    }
    void ReadAsciiList(vtkFoamIOobject& io, const int size)
    {
      for (int i = 0; i < size; i++)
        {
        this->Ptr->SetValue(i, vtkFoamReadValue<primitiveT>::ReadValue(io));
        }
    }
    void ReadBinaryList(vtkFoamIOobject& io, const int size)
    {
      io.Read(reinterpret_cast<unsigned char *>(this->Ptr->GetPointer(0)), size
          * sizeof(primitiveT));
    }
    void ReadValue(vtkFoamIOobject&, vtkFoamToken& currToken)
    {
      if (!currToken.Is<primitiveT>())
        {
        throw vtkFoamError() << "Expected an integer or a (, found "
        << currToken;
        }
      this->Ptr->InsertNextValue(currToken.To<primitiveT>());
    }
  };

  // reads rank 1 lists of types vector, sphericalTensor, symmTensor
  // and tensor. if isPositions is true it reads Cloud type of data as
  // particle positions. cf. (the positions format)
  // src/lagrangian/basic/particle/particleIO.C
  template <typename listT, typename primitiveT, int nComponents,
      bool isPositions> class vectorListTraits
  {
    listT *Ptr;

public:
    vectorListTraits() :
      Ptr(listT::New())
    {
      this->Ptr->SetNumberOfComponents(nComponents);
    }
    listT *GetPtr()
    {
      return this->Ptr;
    }
    void ReadUniformValues(vtkFoamIOobject& io, const int size)
    {
      io.ReadExpecting('(');
      primitiveT vectorValue[nComponents];
      for (int j = 0; j < nComponents; j++)
        {
        vectorValue[j] = vtkFoamReadValue<primitiveT>::ReadValue(io);
        }
      for (int i = 0; i < size; i++)
        {
        this->Ptr->SetTuple(i, vectorValue);
        }
      io.ReadExpecting(')');
      if (isPositions)
        {
        // skip label celli
        vtkFoamReadValue<int>::ReadValue(io);
        }
    }
    void ReadAsciiList(vtkFoamIOobject& io, const int size)
    {
      for (int i = 0; i < size; i++)
        {
        io.ReadExpecting('(');
        primitiveT *vectorTupleI = this->Ptr->GetPointer(nComponents * i);
        for (int j = 0; j < nComponents; j++)
          {
          vectorTupleI[j] = vtkFoamReadValue<primitiveT>::ReadValue(io);
          }
        io.ReadExpecting(')');
        if (isPositions)
          {
          // skip label celli
          vtkFoamReadValue<int>::ReadValue(io);
          }
        }
    }
    void ReadBinaryList(vtkFoamIOobject& io, const int size)
    {
      if (isPositions) // lagrangian/positions (class Cloud)
        {
        // allocate space along with the larger 1.4 format since the
        // size must be determined at compile-time. we allocate on the
        // stack to avoid leak when an exception is thrown.
        unsigned char buffer[sizeof(double) * (nComponents + 1) + 2
            * sizeof(int)];
        const int nBytes = (io.GetIs13Positions()
        // skip label celli
            ? sizeof(double) * nComponents + sizeof(int)
            // skip label celli, label facei and scalar stepFraction
                : sizeof(double) * (nComponents + 1) + 2 * sizeof(int));
        for (int i = 0; i < size; i++)
          {
          io.ReadExpecting('(');
          io.Read(buffer, nBytes);
          this->Ptr->SetTuple(i, reinterpret_cast<double *>(buffer));
          io.ReadExpecting(')');
          }
        }
      else
        {
        for (int i = 0; i < size; i++)
          {
          double buffer[nComponents];
          io.Read(reinterpret_cast<unsigned char *>(buffer), sizeof(double)
              * nComponents);
          this->Ptr->SetTuple(i, buffer);
          }
        }
    }
    void ReadValue(vtkFoamIOobject& io, vtkFoamToken& currToken)
    {
      if (currToken != '(')
        {
        throw vtkFoamError() << "Expected '(', found " << currToken;
        }
      primitiveT v[nComponents];
      for (int j = 0; j < nComponents; j++)
        {
        v[j] = vtkFoamReadValue<primitiveT>::ReadValue(io);
        }
      this->Ptr->InsertNextTuple(v);
      io.ReadExpecting(')');
    }
  };

  vtkFoamEntryValue(const vtkFoamEntry *upperEntryPtr) :
    vtkFoamToken(), IsUniform(false), Managed(true),
        UpperEntryPtr(upperEntryPtr)
  {
  }
  vtkFoamEntryValue(vtkFoamEntryValue&, const vtkFoamEntry *);
  ~vtkFoamEntryValue()
  {
    this->Clear();
  }

  void SetEmptyList()
  {
    this->Clear();
    this->IsUniform = false;
    this->Superclass::Type = EMPTYLIST;
  }
  bool GetIsUniform() const
  {
    return this->IsUniform;
  }
  void Read(vtkFoamIOobject& io);
  void ReadDictionary(vtkFoamIOobject& io, const vtkFoamToken& firstKeyword);
  const vtkIntArray& LabelList() const
  {
    return *this->Superclass::LabelListPtr;
  }
  vtkIntArray& LabelList()
  {
    return *this->Superclass::LabelListPtr;
  }
  const vtkFoamIntVectorVector& LabelListList() const
  {
    return *this->Superclass::LabelListListPtr;
  }
  const vtkFloatArray& ScalarList() const
  {
    return *this->Superclass::ScalarListPtr;
  }
  vtkFloatArray& ScalarList()
  {
    return *this->Superclass::ScalarListPtr;
  }
  const vtkFloatArray& VectorList() const
  {
    return *this->Superclass::VectorListPtr;
  }
  const vtkFoamDict& Dictionary() const
  {
    return *this->Superclass::DictPtr;
  }
  vtkFoamDict& Dictionary()
  {
    return *this->Superclass::DictPtr;
  }

  void *Ptr()
  {
    this->Managed = false; // returned pointer will not be deleted by the d'tor
    // all list pointers are in a single union
    return (void *)this->Superclass::LabelListPtr;
  }

  vtkStdString ToString() const
  {
    return this->Superclass::Type == STRING ? this->Superclass::ToString()
        : vtkStdString();
  }
  float ToFloat() const
  {
    return this->Superclass::Type == SCALAR || this->Superclass::Type == LABEL ? this->Superclass::To<float>()
        : 0.0F;
  }
  double ToDouble() const
  {
    return this->Superclass::Type == SCALAR || this->Superclass::Type == LABEL ? this->Superclass::To<double>()
        : 0.0;
  }
  int ToInt() const
  {
    return this->Superclass::Type == LABEL ? this->Superclass::To<int>() : 0;
  }

  // the following two are for an exceptional expression of
  // `LABEL{LABELorSCALAR}' without type prefix (e. g. `2{-0}' in
  // mixedRhoE B.C. in rhopSonicFoam/shockTube)
  void MakeLabelList(const int labelValue, const int size)
  {
    this->Superclass::LabelListPtr = vtkIntArray::New();
    this->Superclass::Type = LABELLIST;
    this->Superclass::LabelListPtr->SetNumberOfValues(size);
    for (int i = 0; i < size; i++)
      {
      this->Superclass::LabelListPtr->SetValue(i, labelValue);
      }
  }
  void MakeScalarList(const float scalarValue, const int size)
  {
    this->Superclass::ScalarListPtr = vtkFloatArray::New();
    this->Superclass::Type = SCALARLIST;
    this->Superclass::ScalarListPtr->SetNumberOfValues(size);
    for (int i = 0; i < size; i++)
      {
      this->Superclass::ScalarListPtr->SetValue(i, scalarValue);
      }
  }

  // reads dimensionSet
  void ReadDimensionSet(vtkFoamIOobject& io)
  {
    const int nDims = 7;
    this->Superclass::LabelListPtr = vtkIntArray::New();
    this->Superclass::Type = LABELLIST;
    this->Superclass::LabelListPtr->SetNumberOfValues(nDims);
    for (int dimI = 0; dimI < nDims; dimI++)
      {
      this->Superclass::LabelListPtr->SetValue(dimI, vtkFoamReadValue<int>::ReadValue(io));
      }
    io.ReadExpecting(']');
  }

  template <vtkFoamToken::tokenType listType, typename traitsT> void ReadNonuniformList(
      vtkFoamIOobject& io);

  // reads a list of labelLists. requires size prefix of the listList
  // to be present. size of each sublist must also be present in the
  // stream if the format is binary.
  void ReadLabelListList(vtkFoamIOobject& io)
  {
    vtkFoamToken currToken;
    if (!io.Read(currToken))
      {
      throw vtkFoamError() << "Unexpected EOF";
      }
    if (currToken.GetType() == vtkFoamToken::LABEL)
      {
      const int sizeI = currToken.To<int>();
      if (sizeI < 0)
        {
        throw vtkFoamError() << "List size must not be negative: size = "
        << sizeI;
        }
      // gives initial guess for list size
      this->Superclass::LabelListListPtr = new vtkFoamIntVectorVector(sizeI, 4 * sizeI);
      this->Superclass::Type = LABELLISTLIST;
      io.ReadExpecting('(');
      int bodyI = 0;
      for (int i = 0; i < sizeI; i++)
        {
        if (!io.Read(currToken))
          {
          throw vtkFoamError() << "Unexpected EOF";
          }
        if (currToken.GetType() == vtkFoamToken::LABEL)
          {
          const int sizeJ = currToken.To<int>();
          if (sizeJ < 0)
            {
            throw vtkFoamError() << "List size must not be negative: size = "
            << sizeJ;
            }
          int *listI = this->Superclass::LabelListListPtr->WritePointer(i,
              bodyI, sizeJ);

          if (io.GetFormat() == vtkFoamIOobject::ASCII)
            {
            io.ReadExpecting('(');
            for (int j = 0; j < sizeJ; j++)
              {
              listI[j] = vtkFoamReadValue<int>::ReadValue(io);
              }
            io.ReadExpecting(')');
            }
          else
            {
            if (sizeJ > 0) // avoid invalid reference to labelListI.at(0)
              {
              io.ReadExpecting('(');
              io.Read(reinterpret_cast<unsigned char*>(listI), sizeJ
                  * sizeof(int));
              io.ReadExpecting(')');
              }
            }
          bodyI += sizeJ;
          }
        else if (currToken == '(')
          {
          this->Superclass::LabelListListPtr->SetIndex(i, bodyI);
          while (io.Read(currToken) && currToken != ')')
            {
            if (currToken.GetType() != vtkFoamToken::LABEL)
              {
              throw vtkFoamError() << "Expected an integer, found "
              << currToken;
              }
            this->Superclass::LabelListListPtr
            ->InsertValue(bodyI++, currToken.To<int>());
            }
          }
        else
          {
          throw vtkFoamError() << "Expected integer or '(', found "
          << currToken;
          }
        }
      // set the next index of the last element to calculate the last
      // subarray size
      this->Superclass::LabelListListPtr->SetIndex(sizeI, bodyI);
      // shrink to the actually used size
      this->Superclass::LabelListListPtr->ResizeBody(bodyI);
      io.ReadExpecting(')');
      }
    else
      {
      throw vtkFoamError() << "Expected integer, found " << currToken;
      }
  }

  // reads compact list of labels.
  void ReadCompactIOLabelList(vtkFoamIOobject& io)
  {
    if (io.GetFormat() != vtkFoamIOobject::BINARY)
      {
      this->ReadLabelListList(io);
      return;
      }

    this->Superclass::LabelListListPtr = new vtkFoamIntVectorVector;
    this->Superclass::Type = LABELLISTLIST;
    for(int arrayI = 0; arrayI < 2; arrayI++)
      {
      vtkFoamToken currToken;
      if (!io.Read(currToken))
        {
        throw vtkFoamError() << "Unexpected EOF";
        }
      if (currToken.GetType() == vtkFoamToken::LABEL)
        {
        const int sizeI = currToken.To<int>();
        if (sizeI < 0)
          {
          throw vtkFoamError() << "List size must not be negative: size = "
              << sizeI;
          }
        if (sizeI > 0) // avoid invalid reference
          {
          vtkIntArray *array = (arrayI == 0
              ? this->Superclass::LabelListListPtr->GetIndices()
              : this->Superclass::LabelListListPtr->GetBody());
          array->SetNumberOfValues(sizeI);
          io.ReadExpecting('(');
          io.Read(reinterpret_cast<unsigned char*>(array->GetPointer(0)),
              sizeI * sizeof(int));
          io.ReadExpecting(')');
          }
        }
      else
        {
        throw vtkFoamError() << "Expected integer, found " << currToken;
        }
      }
  }

  bool ReadField(vtkFoamIOobject& io)
  {
    try
      {
      // lagrangian labels (cf. gnemdFoam/nanoNozzle)
      if(io.GetClassName() == "labelField")
        {
        this->ReadNonuniformList<LABELLIST, listTraits<vtkIntArray, int> >(io);
        }
      // lagrangian scalars

      else if(io.GetClassName() == "scalarField")
        {
        this->ReadNonuniformList<SCALARLIST, listTraits<vtkFloatArray, float> >(
            io);
        }
      else if(io.GetClassName() == "sphericalTensorField")
        {
        this->ReadNonuniformList<VECTORLIST,
        vectorListTraits<vtkFloatArray, float, 1, false> >(io);
        }
      // polyMesh/points, lagrangian vectors

      else if(io.GetClassName() == "vectorField")
        {
        this->ReadNonuniformList<VECTORLIST,
        vectorListTraits<vtkFloatArray, float, 3, false> >(io);
        }
      else if(io.GetClassName() == "symmTensorField")
        {
        this->ReadNonuniformList<VECTORLIST,
        vectorListTraits<vtkFloatArray, float, 6, false> >(io);
        }
      else if(io.GetClassName() == "tensorField")
        {
        this->ReadNonuniformList<VECTORLIST,
        vectorListTraits<vtkFloatArray, float, 9, false> >(io);
        }
      else
        {
        throw vtkFoamError() << "Non-supported field type "
        << io.GetClassName();
        }
      }
    catch(vtkFoamError& e)
      {
      io.SetError(e);
      return false;
      }
    return true;
  }
};

// specialization for reading double precision binary into vtkFloatArray.
// Must precede ReadNonuniformList() below (HP-UXia64-aCC).
VTK_TEMPLATE_SPECIALIZE
void vtkFoamEntryValue::listTraits<vtkFloatArray, float>::ReadBinaryList(
    vtkFoamIOobject& io, const int size)
{
  for (int i = 0; i < size; i++)
    {
    double buffer;
    io.Read(reinterpret_cast<unsigned char *>(&buffer), sizeof(double));
    this->Ptr->SetValue(i, static_cast<float>(buffer));
    }
}

// generic reader for nonuniform lists. requires size prefix of the
// list to be present in the stream if the format is binary.
template <vtkFoamToken::tokenType listType, typename traitsT>
void vtkFoamEntryValue::ReadNonuniformList(vtkFoamIOobject& io)
{
  vtkFoamToken currToken;
  if (!io.Read(currToken))
    {
    throw vtkFoamError() << "Unexpected EOF";
    }
  traitsT list;
  this->Superclass::Type = listType;
  this->Superclass::VtkObjectPtr = list.GetPtr();
  if (currToken.Is<int>())
    {
    const int size = currToken.To<int>();
    if (size < 0)
      {
      throw vtkFoamError() << "List size must not be negative: size = " << size;
      }
    list.GetPtr()->SetNumberOfTuples(size);
    if (io.GetFormat() == vtkFoamIOobject::ASCII)
      {
      if (!io.Read(currToken))
        {
        throw vtkFoamError() << "Unexpected EOF";
        }
      // some objects have lists with only one element enclosed by {}
      // e. g. simpleFoam/pitzDaily3Blocks/constant/polyMesh/faceZones
      if (currToken == '{')
        {
        list.ReadUniformValues(io, size);
        io.ReadExpecting('}');
        return;
        }
      else if (currToken != '(')
        {
        throw vtkFoamError() << "Expected '(', found " << currToken;
        }
      list.ReadAsciiList(io, size);
      io.ReadExpecting(')');
      }
    else
      {
      if (size > 0)
        {
        // read parentheses only when size > 0
        io.ReadExpecting('(');
        list.ReadBinaryList(io, size);
        io.ReadExpecting(')');
        }
      }
    }
  else if (currToken == '(')
    {
    while (io.Read(currToken) && currToken != ')')
      {
      list.ReadValue(io, currToken);
      }
    list.GetPtr()->Squeeze();
    }
  else
    {
    throw vtkFoamError() << "Expected integer or '(', found " << currToken;
    }
}

//-----------------------------------------------------------------------------
// class vtkFoamEntry
// a class that represents an entry of a dictionary. note that an
// entry can have more than one value.
struct vtkFoamEntry : public vtkstd::vector<vtkFoamEntryValue*>
{
private:
  typedef vtkstd::vector<vtkFoamEntryValue*> Superclass;
  vtkStdString Keyword;
  vtkFoamDict *UpperDictPtr;

  vtkFoamEntry();

public:
  vtkFoamEntry(vtkFoamDict *upperDictPtr) :
    UpperDictPtr(upperDictPtr)
  {
  }
  vtkFoamEntry(const vtkFoamEntry& entry, vtkFoamDict *upperDictPtr) :
    Superclass(entry.size()), Keyword(entry.GetKeyword()),
        UpperDictPtr(upperDictPtr)
  {
    for (size_t valueI = 0; valueI < entry.size(); valueI++)
      {
      this->Superclass::operator[](valueI) = new vtkFoamEntryValue(*entry[valueI], this);
      }
  }

  ~vtkFoamEntry()
  {
    this->Clear();
  }

  void Clear()
  {
    for (size_t i = 0; i < this->Superclass::size(); i++)
      {
      delete this->Superclass::operator[](i);
      }
    this->Superclass::clear();
  }
  const vtkStdString& GetKeyword() const
  {
    return this->Keyword;
  }
  void SetKeyword(const vtkStdString& keyword)
  {
    this->Keyword = keyword;
  }
  const vtkFoamEntryValue& FirstValue() const
  {
    return *this->Superclass::operator[](0);
  }
  vtkFoamEntryValue& FirstValue()
  {
    return *this->Superclass::operator[](0);
  }
  const vtkIntArray& LabelList() const
  {
    return this->FirstValue().LabelList();
  }
  vtkIntArray& LabelList()
  {
    return this->FirstValue().LabelList();
  }
  const vtkFoamIntVectorVector& LabelListList() const
  {
    return this->FirstValue().LabelListList();
  }
  const vtkFloatArray& ScalarList() const
  {
    return this->FirstValue().ScalarList();
  }
  vtkFloatArray& ScalarList()
  {
    return this->FirstValue().ScalarList();
  }
  const vtkFloatArray& VectorList() const
  {
    return this->FirstValue().VectorList();
  }
  const vtkFoamDict& Dictionary() const
  {
    return this->FirstValue().Dictionary();
  }
  vtkFoamDict& Dictionary()
  {
    return this->FirstValue().Dictionary();
  }
  void *Ptr()
  {
    return this->FirstValue().Ptr();
  }
  const vtkFoamDict *GetUpperDictPtr() const
  {
    return this->UpperDictPtr;
  }

  vtkStdString ToString() const
  {
    return this->Superclass::size() > 0 ? this->FirstValue().ToString() : vtkStdString();
  }
  float ToFloat() const
  {
    return this->Superclass::size() > 0 ? this->FirstValue().ToFloat() : 0.0F;
  }
  double ToDouble() const
  {
    return this->Superclass::size() > 0 ? this->FirstValue().ToDouble() : 0.0;
  }
  int ToInt() const
  {
    return this->Superclass::size() > 0 ? this->FirstValue().ToInt() : 0;
  }

  void ReadDictionary(vtkFoamIOobject& io)
  {
    this->Superclass::push_back(new vtkFoamEntryValue(this));
    this->Superclass::back()->ReadDictionary(io, vtkFoamToken());
  }

  // read values of an entry
  void Read(vtkFoamIOobject& io);
};

//-----------------------------------------------------------------------------
// class vtkFoamDict
// a class that holds a FoamFile data structure
struct vtkFoamDict : public vtkstd::vector<vtkFoamEntry*>
{
private:
  typedef vtkstd::vector<vtkFoamEntry*> Superclass;

  vtkFoamToken Token;
  const vtkFoamDict *UpperDictPtr;

  vtkFoamDict(const vtkFoamDict &);

public:
  vtkFoamDict(const vtkFoamDict *upperDictPtr = NULL) :
    Superclass(), Token(), UpperDictPtr(upperDictPtr)
  {
  }
  vtkFoamDict(const vtkFoamDict& dict, const vtkFoamDict *upperDictPtr) :
    Superclass(dict.size()), Token(), UpperDictPtr(upperDictPtr)
  {
    if (dict.GetType() == vtkFoamToken::DICTIONARY)
      {
      for (size_t entryI = 0; entryI < dict.size(); entryI++)
        {
        this->operator[](entryI) = new vtkFoamEntry(*dict[entryI], this);
        }
      }
  }

  ~vtkFoamDict()
  {
    if (this->Token.GetType() == vtkFoamToken::UNDEFINED)
      {
      for (size_t i = 0; i < this->Superclass::size(); i++)
        {
        delete this->operator[](i);
        }
      }
  }

  vtkFoamToken::tokenType GetType() const
  {
    return this->Token.GetType() == vtkFoamToken::UNDEFINED ? vtkFoamToken::DICTIONARY
        : this->Token.GetType();
  }
  const vtkFoamToken &GetToken() const
  {
    return this->Token;
  }
  const vtkFoamDict *GetUpperDictPtr() const
  {
    return this->UpperDictPtr;
  }
  vtkFoamEntry *Lookup(const vtkStdString& keyword) const
  {
    if (this->Token.GetType() == vtkFoamToken::UNDEFINED)
      {
      for (size_t i = 0; i < this->Superclass::size(); i++)
        {
        if (this->operator[](i)->GetKeyword() == keyword) // found
          {
          return this->operator[](i);
          }
        }
      }

    // not found
    return NULL;
  }

  // reads a FoamFile or a subdictionary. if the stream to be read is
  // a subdictionary the preceding '{' is assumed to have already been
  // thrown away.
  bool Read(vtkFoamIOobject& io, const bool isSubDictionary = false,
      const vtkFoamToken& firstToken = vtkFoamToken())
  {
    try
      {
      vtkFoamToken currToken;
      if(firstToken.GetType() == vtkFoamToken::UNDEFINED)
        {
        // read the first token
        if(!io.Read(currToken))
          {
          throw vtkFoamError() << "Unexpected EOF";
          }

        if(isSubDictionary)
          {
          // the following if clause is for an exceptional expression
          // of `LABEL{LABELorSCALAR}' without type prefix
          // (e. g. `2{-0}' in mixedRhoE B.C. in
          // rhopSonicFoam/shockTube)
          if(currToken.GetType() == vtkFoamToken::LABEL
              || currToken.GetType() == vtkFoamToken::SCALAR)
            {
            this->Token = currToken;
            io.ReadExpecting('}');
            return true;
            }
          // return as empty dictionary

          else if(currToken == '}')
            {
            return true;
            }
          }
        else
          {
          // list of dictionaries is read as a usual dictionary
          // polyMesh/boundary, point/face/cell-Zones
          if(currToken.GetType() == vtkFoamToken::LABEL)
            {
            io.ReadExpecting('(');
            if(currToken.To<int>()> 0)
              {
              if(!io.Read(currToken))
                {
                throw vtkFoamError() << "Unexpected EOF";
                }
              // continue to read as a usual dictionary
              }
            else // return as empty dictionary

              {
              io.ReadExpecting(')');
              return true;
              }
            }
          // some boundary files does not have the number of boundary
          // patches (e.g. settlingFoam/tank3D). in this case we need to
          // explicitly read the file as a dictionary.

          else if(currToken == '('
              && io.GetClassName() == "polyBoundaryMesh") // polyMesh/boundary

            {
            if(!io.Read(currToken)) // read the first keyword

              {
              throw vtkFoamError() << "Unexpected EOF";
              }
            if(currToken == ')') // return as empty dictionary

              {
              return true;
              }
            }
          }
        }
      // if firstToken is given as string read the following stream as
      // subdictionary

      else if(firstToken.GetType() == vtkFoamToken::STRING)
        {
        this->Superclass::push_back(new vtkFoamEntry(this));
        this->Superclass::back()->SetKeyword(firstToken.ToString());
        this->Superclass::back()->ReadDictionary(io);
        if(!io.Read(currToken) || currToken == '}' || currToken == ')')
          {
          return true;
          }
        }
      else // quite likely an identifier

        {
        currToken = firstToken;
        }

      if(currToken == ';' || currToken.GetType() == vtkFoamToken::STRING
          || currToken.GetType() == vtkFoamToken::IDENTIFIER)
        {
        // general dictionary
        do
          {
          if(currToken.GetType() == vtkFoamToken::STRING)
            {
            vtkFoamEntry *previousEntry = this->Lookup(currToken.ToString());
            if(previousEntry != NULL)
              {
              if(io.GetInputMode() == vtkFoamFile::INPUT_MODE_MERGE)
                {
                if(previousEntry->FirstValue().GetType()
                    == vtkFoamToken::DICTIONARY)
                  {
                  io.ReadExpecting('{');
                  previousEntry->FirstValue().Dictionary().Read(io, true);
                  }
                else
                  {
                  previousEntry->Clear();
                  previousEntry->Read(io);
                  }
                }
              else if(io.GetInputMode() == vtkFoamFile::INPUT_MODE_OVERWRITE)
                {
                previousEntry->Clear();
                previousEntry->Read(io);
                }
              else // INPUT_MODE_ERROR
                {
                throw vtkFoamError() << "Found duplicated entries with keyword "
                << currToken.ToString();
                }
              }
            else
              {
              this->Superclass::push_back(new vtkFoamEntry(this));
              this->Superclass::back()->SetKeyword(currToken.ToString());
              this->Superclass::back()->Read(io);
              }

            if(currToken == "FoamFile")
              {
              // delete the FoamFile header subdictionary entry
              delete this->Superclass::back();
              this->Superclass::pop_back();
              }
            else if(currToken == "include")
              {
              // include the named file. Exiting the included file at
              // EOF will be handled automatically by
              // vtkFoamFile::closeIncludedFile()
              if(this->Superclass::back()->FirstValue().GetType()
                  != vtkFoamToken::STRING)
                {
                throw vtkFoamError()
                << "Expected string as the file name to be included, found "
                << this->Superclass::back()->FirstValue();
                }
              const vtkStdString includeFileName(
                  this->Superclass::back()->ToString());
              delete this->Superclass::back();
              this->Superclass::pop_back();
              io.IncludeFile(includeFileName, io.GetFilePath());
              }
            }
          else if(currToken.GetType() == vtkFoamToken::IDENTIFIER)
            {
            // substitute identifier
            const vtkStdString identifier(currToken.ToIdentifier());

            for(const vtkFoamDict *uDictPtr = this;;)
              {
              const vtkFoamEntry *identifiedEntry
              = uDictPtr->Lookup(identifier);

              if(identifiedEntry != NULL)
                {
                if(identifiedEntry->FirstValue().GetType()
                    != vtkFoamToken::DICTIONARY)
                  {
                  throw vtkFoamError()
                  << "Expected dictionary for substituting entry "
                  << identifier;
                  }
                const vtkFoamDict& identifiedDict
                = identifiedEntry->FirstValue().Dictionary();
                for(size_t entryI = 0; entryI < identifiedDict.size(); entryI++)
                  {
                  // I think #inputMode handling should be done here
                  // as well, but the genuine FoamFile parser for OF
                  // 1.5 does not seem to be doing it.
                  this->Superclass::push_back(
                      new vtkFoamEntry(*identifiedDict[entryI], this));
                  }
                break;
                }
              else
                {
                uDictPtr = uDictPtr->GetUpperDictPtr();
                if(uDictPtr == NULL)
                  {
                  throw vtkFoamError() << "Substituting entry " << identifier
                  << " not found";
                  }
                }
              }
            }
          // skip empty entry only with ';'
          }while(io.Read(currToken)
            && (currToken.GetType() == vtkFoamToken::STRING
                || currToken.GetType() == vtkFoamToken::IDENTIFIER
                || currToken == ';'));

        if(currToken.GetType() == vtkFoamToken::TOKEN_ERROR || currToken == '}'
            || currToken == ')')
          {
          return true;
          }
        throw vtkFoamError()
        << "Expected keyword, closing brace, ';' or EOF, found " << currToken;
        }
      throw vtkFoamError() << "Expected keyword or identifier, found "
      << currToken;
      }
    catch(vtkFoamError& e)
      {
      if(isSubDictionary)
        {
        throw;
        }
      else
        {
        io.SetError(e);
        return false;
        }
      }
  }
};

void vtkFoamIOobject::ReadHeader()
{
  vtkFoamToken firstToken;

  this->Superclass::ReadExpecting("FoamFile");
  this->Superclass::ReadExpecting('{');

  vtkFoamDict headerDict;
  // throw exception in case of error
  headerDict.Read(*this, true, vtkFoamToken());

  const vtkFoamEntry *formatEntry = headerDict.Lookup("format");
  if (formatEntry == NULL)
    {
    throw vtkFoamError()
    << "format entry (binary/ascii) not found in FoamFile header";
    }
  // case does matter (e. g. "BINARY" is treated as ascii)
  // cf. src/OpenFOAM/db/IOstreams/IOstreams/IOstream.C
  this->Format = (formatEntry->ToString() == "binary" ? BINARY : ASCII);

  const vtkFoamEntry *classEntry = headerDict.Lookup("class");
  if (classEntry == NULL)
    {
    throw vtkFoamError() << "class name not found in FoamFile header";
    }
  this->HeaderClassName = classEntry->ToString();

  const vtkFoamEntry *objectEntry = headerDict.Lookup("object");
  if (objectEntry == NULL)
    {
    throw vtkFoamError() << "object name not found in FoamFile header";
    }
  this->ObjectName = objectEntry->ToString();
}

vtkFoamEntryValue::vtkFoamEntryValue(
    vtkFoamEntryValue& value, const vtkFoamEntry *upperEntryPtr) :
  vtkFoamToken(value), IsUniform(value.GetIsUniform()), Managed(true),
      UpperEntryPtr(upperEntryPtr)
{
  switch (this->Superclass::Type)
    {
    case VECTORLIST:
#if vtksys_DATE_STAMP_FULL >= 20080620
        {
        vtkFloatArray *fa = vtkFloatArray::SafeDownCast(value.ToVTKObject());
        if(fa->GetNumberOfComponents() == 6)
          {
          // create deepcopies for vtkObjects to avoid duplicated mainpulation
          vtkFloatArray *newfa = vtkFloatArray::New();
          newfa->DeepCopy(fa);
          this->Superclass::VtkObjectPtr = newfa;
          break;
          }
        }
#endif
    case LABELLIST:
    case SCALARLIST:
    case STRINGLIST:
      this->Superclass::VtkObjectPtr = value.ToVTKObject();
      this->Superclass::VtkObjectPtr->Register(0);
      break;
    case LABELLISTLIST:
      this->LabelListListPtr = new vtkFoamIntVectorVector(*value.LabelListListPtr);
      break;
    case ENTRYVALUELIST:
      {
      const size_t nValues = value.EntryValuePtrs->size();
      this->EntryValuePtrs = new vtkstd::vector<vtkFoamEntryValue*>(nValues);
      for (size_t valueI = 0; valueI < nValues; valueI++)
        {
        this->EntryValuePtrs->operator[](valueI) = new vtkFoamEntryValue(
            *value.EntryValuePtrs->operator[](valueI), upperEntryPtr);
        }
      }
      break;
    case DICTIONARY:
      // UpperEntryPtr is null when called from vtkFoamDict constructor
      if (this->UpperEntryPtr != NULL)
        {
        this->DictPtr = new vtkFoamDict(*value.DictPtr,
            this->UpperEntryPtr->GetUpperDictPtr());
        }
      else
        {
        this->DictPtr = NULL;
        }
      break;
    case EMPTYLIST:
      break;
      // required to suppress the 'enumeration value not handled' warning by
      // g++ when compiled with -Wall
    default:
      break;
    }
}

void vtkFoamEntryValue::Clear()
{
  if (this->Managed)
    {
    switch (this->Superclass::Type)
      {
      case LABELLIST:
      case SCALARLIST:
      case VECTORLIST:
      case STRINGLIST:
        this->VtkObjectPtr->Delete();
        break;
      case LABELLISTLIST:
        delete this->LabelListListPtr;
        break;
      case ENTRYVALUELIST:
        for (size_t valueI = 0; valueI < this->EntryValuePtrs->size() ; valueI++)
          {
          delete this->EntryValuePtrs->operator[](valueI);
          }
        delete this->EntryValuePtrs;
        break;
      case DICTIONARY:
        delete this->DictPtr;
        break;
        // required to suppress the 'enumeration value not handled' warning by
        // g++ when compiled with -Wall
      default:
        break;
      }
    }
}

// general-purpose list reader - guess the type of the list and read
// it. only supports ascii format and assumes the preceding '(' has
// already been thrown away.  the reader supports nested list with
// variable lengths (e. g. `((token token) (token token token)).'
// also supports compound of tokens and lists (e. g. `((token token)
// token)') only if a list comes as the first value.
void vtkFoamEntryValue::ReadList(vtkFoamIOobject& io)
{
  vtkFoamToken currToken;
  io.Read(currToken);

  // initial guess of the list type
  if (currToken.GetType() == this->Superclass::LABEL)
    {
    // if the first token is of type LABEL it might be either an element of
    // a labelList or the size of a sublist so proceed to the next token
    vtkFoamToken nextToken;
    if (!io.Read(nextToken))
      {
      throw vtkFoamError() << "Unexpected EOF";
      }
    if (nextToken.GetType() == this->Superclass::LABEL)
      {
      this->Superclass::LabelListPtr = vtkIntArray::New();
      this->Superclass::LabelListPtr->InsertNextValue(currToken.To<int>());
      this->Superclass::LabelListPtr->InsertNextValue(nextToken.To<int>());
      this->Superclass::Type = LABELLIST;
      }
    else if (nextToken.GetType() == this->Superclass::SCALAR)
      {
      this->Superclass::ScalarListPtr = vtkFloatArray::New();
      this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
      this->Superclass::ScalarListPtr->InsertNextValue(nextToken.To<float>());
      this->Superclass::Type = SCALARLIST;
      }
    else if (nextToken == '(') // list of list: read recursively
      {
      this->Superclass::EntryValuePtrs = new vtkstd::vector<vtkFoamEntryValue*>;
      this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(
          this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
      this->Superclass::Type = ENTRYVALUELIST;
      }
    else if (nextToken == ')') // list with only one label element
      {
      this->Superclass::LabelListPtr = vtkIntArray::New();
      this->Superclass::LabelListPtr->SetNumberOfValues(1);
      this->Superclass::LabelListPtr->SetValue(0, currToken.To<int>());
      this->Superclass::Type = LABELLIST;
      return;
      }
    else
      {
      throw vtkFoamError() << "Expected number, '(' or ')', found "
      << nextToken;
      }
    }
  else if (currToken.GetType() == this->Superclass::SCALAR)
    {
    this->Superclass::ScalarListPtr = vtkFloatArray::New();
    this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
    this->Superclass::Type = SCALARLIST;
    }
  // if the first word is a string we have to read another token to determine
  // if the first word is a keyword for the following dictionary
  else if (currToken.GetType() == this->Superclass::STRING)
    {
    vtkFoamToken nextToken;
    if (!io.Read(nextToken))
      {
      throw vtkFoamError() << "Unexpected EOF";
      }
    if (nextToken.GetType() == this->Superclass::STRING) // list of strings
      {
      this->Superclass::StringListPtr = vtkStringArray::New();
      this->Superclass::StringListPtr->InsertNextValue(currToken.ToString());
      this->Superclass::StringListPtr->InsertNextValue(nextToken.ToString());
      this->Superclass::Type = STRINGLIST;
      }
    // dictionary with the already read stringToken as the first keyword
    else if (nextToken == '{')
      {
      if (currToken.ToString() == "")
        {
        throw "Empty string is invalid as a keyword for dictionary entry";
        }
      this->ReadDictionary(io, currToken);
      // the dictionary read as list has the entry terminator ';' so
      // we have to skip it
      return;
      }
    else if (nextToken == ')') // list with only one string element
      {
      this->Superclass::StringListPtr = vtkStringArray::New();
      this->Superclass::StringListPtr->SetNumberOfValues(1);
      this->Superclass::StringListPtr->SetValue(0, currToken.ToString());
      this->Superclass::Type = STRINGLIST;
      return;
      }
    else
      {
      throw vtkFoamError() << "Expected string, '{' or ')', found "
      << nextToken;
      }
    }
  // list of lists or dictionaries: read recursively
  else if (currToken == '(' || currToken == '{')
    {
    this->Superclass::EntryValuePtrs = new vtkstd::vector<vtkFoamEntryValue*>;
    this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(
        this->UpperEntryPtr));
    if(currToken == '(')
      {
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
      }
    else // currToken == '{'
      {
      this->Superclass::EntryValuePtrs->back()->ReadDictionary(io, vtkFoamToken());
      }
    // read all the following values as arbitrary entryValues
    // the alphaContactAngle b.c. in multiphaseInterFoam/damBreak4phase
    // reaquires this treatment (reading by readList() is not enough)
    do
      {
      this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(
          this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->Read(io);
      } while (*this->Superclass::EntryValuePtrs->back() != ')'
        && *this->Superclass::EntryValuePtrs->back() != '}'
        && *this->Superclass::EntryValuePtrs->back() != ';');

    if (*this->Superclass::EntryValuePtrs->back() != ')')
      {
      throw vtkFoamError() << "Expected ')' before "
          << *this->Superclass::EntryValuePtrs->back();
      }

    // delete ')'
    delete this->Superclass::EntryValuePtrs->back();
    this->EntryValuePtrs->pop_back();
    this->Superclass::Type = ENTRYVALUELIST;
    return;
    }
  else if (currToken == ')') // empty list
    {
    this->Superclass::Type = EMPTYLIST;
    return;
    }
  // FIXME: may (or may not) need identifier handling

  while (io.Read(currToken) && currToken != ')')
    {
    if (this->Superclass::Type == LABELLIST)
      {
      if (currToken.GetType() == this->Superclass::SCALAR)
        {
        // switch to scalarList
        // LabelListPtr and ScalarListPtr are packed into a single union so
        // we need a temprary pointer
        vtkFloatArray* slPtr = vtkFloatArray::New();
        const int size = this->Superclass::LabelListPtr->GetNumberOfTuples();
        slPtr->SetNumberOfValues(size + 1);
        for (int i = 0; i < size; i++)
          {
          slPtr->SetValue(i,
              static_cast<float>(this->Superclass::LabelListPtr->GetValue(i)));
          }
        this->LabelListPtr->Delete();
        slPtr->SetValue(size, currToken.To<float>());
        // copy after LabelListPtr is deleted
        this->Superclass::ScalarListPtr = slPtr;
        this->Superclass::Type = SCALARLIST;
        }
      else if (currToken.GetType() == this->Superclass::LABEL)
        {
        this->Superclass::LabelListPtr->InsertNextValue(currToken.To<int>());
        }
      else
        {
        throw vtkFoamError() << "Expected a number, found " << currToken;
        }
      }
    else if (this->Superclass::Type == this->Superclass::SCALARLIST)
      {
      if (currToken.Is<float>())
        {
        this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
        }
      else
        {
        throw vtkFoamError() << "Expected a number, found " << currToken;
        }
      }
    else if (this->Superclass::Type == this->Superclass::STRINGLIST)
      {
      if (currToken.GetType() == this->Superclass::STRING)
        {
        this->Superclass::StringListPtr->InsertNextValue(currToken.ToString());
        }
      else
        {
        throw vtkFoamError() << "Expected a string, found " << currToken;
        }
      }
    else if (this->Superclass::Type == this->Superclass::ENTRYVALUELIST)
      {
      if (currToken.GetType() == this->Superclass::LABEL)
        {
        // skip the number of elements to make things simple
        if (!io.Read(currToken))
          {
          throw vtkFoamError() << "Unexpected EOF";
          }
        }
      if (currToken != '(')
        {
        throw vtkFoamError() << "Expected '(', found " << currToken;
        }
      this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
      }
    else
      {
      throw vtkFoamError() << "Unexpected token " << currToken;
      }
    }

  if (this->Superclass::Type == this->Superclass::LABELLIST)
    {
    this->Superclass::LabelListPtr->Squeeze();
    }
  else if (this->Superclass::Type == this->Superclass::SCALARLIST)
    {
    this->Superclass::ScalarListPtr->Squeeze();
    }
  else if (this->Superclass::Type == this->Superclass::STRINGLIST)
    {
    this->Superclass::StringListPtr->Squeeze();
    }
}

// a list of dictionaries is actually read as a dictionary
void vtkFoamEntryValue::ReadDictionary(vtkFoamIOobject& io,
    const vtkFoamToken& firstKeyword)
{
  this->Superclass::DictPtr = new vtkFoamDict(this->UpperEntryPtr->GetUpperDictPtr());
  this->Superclass::Type = this->Superclass::DICTIONARY;
  this->Superclass::DictPtr->Read(io, true, firstKeyword);
}

// guess the type of the given entry value and read it
void vtkFoamEntryValue::Read(vtkFoamIOobject& io)
{
  vtkFoamToken currToken;
  if (!io.Read(currToken))
    {
    throw vtkFoamError() << "Unexpected EOF";
    }

  if (currToken == '{')
    {
    this->ReadDictionary(io, vtkFoamToken());
    return;
    }
  // for reading sublist from vtkFoamEntryValue::readList() or there
  // are cases where lists without the (non)uniform keyword appear
  // (e. g. coodles/pitsDaily/0/U, uniformFixedValue b.c.)
  else if (currToken == '(')
    {
    this->ReadList(io);
    return;
    }
  else if (currToken == '[')
    {
    this->ReadDimensionSet(io);
    return;
    }
  else if (currToken == "uniform")
    {
    if (!io.Read(currToken))
      {
      throw vtkFoamError()
      << "Expected a uniform value or a list, found unexpected EOF";
      }
    if (currToken == '(')
      {
      this->ReadList(io);
      }
    else if (currToken.GetType() == this->Superclass::LABEL
        || currToken.GetType() == this->Superclass::SCALAR
        || currToken.GetType() == this->Superclass::STRING)
      {
      this->Superclass::operator=(currToken);
      }
    else // unexpected punctuation token
      {
      throw vtkFoamError() << "Expected number, string or (, found "
      << currToken;
      }
    this->IsUniform = true;
    }
  else if (currToken == "nonuniform")
    {
    if (!io.Read(currToken))
      {
      throw vtkFoamError() << "Expected list type specifier, found EOF";
      }
    this->IsUniform = false;
    if (currToken == "List<scalar>")
      {
      this->ReadNonuniformList<SCALARLIST, listTraits<vtkFloatArray, float> >(io);
      }
    else if (currToken == "List<sphericalTensor>")
      {
      this->ReadNonuniformList<VECTORLIST,
      vectorListTraits<vtkFloatArray, float, 1, false> >(io);
      }
    else if (currToken == "List<vector>")
      {
      this->ReadNonuniformList<VECTORLIST,
      vectorListTraits<vtkFloatArray, float, 3, false> >(io);
      }
    else if (currToken == "List<symmTensor>")
      {
      this->ReadNonuniformList<VECTORLIST,
      vectorListTraits<vtkFloatArray, float, 6, false> >(io);
      }
    else if (currToken == "List<tensor>")
      {
      this->ReadNonuniformList<VECTORLIST,
      vectorListTraits<vtkFloatArray, float, 9, false> >(io);
      }
    // List<bool> is read as List<label>
    else if (currToken =="List<label>" || currToken == "List<bool>")
      {
      this->ReadNonuniformList<LABELLIST, listTraits<vtkIntArray, int> >(io);
      }
    // an empty list doesn't have a list type specifier
    else if (currToken.GetType() == this->Superclass::LABEL
        && currToken.To<int>() == 0)
      {
      this->Superclass::Type = this->Superclass::EMPTYLIST;
      if(io.GetFormat() == vtkFoamIOobject::ASCII)
        {
        io.ReadExpecting('(');
        io.ReadExpecting(')');
        }
      }
    else
      {
      throw vtkFoamError() << "Unsupported nonuniform list type " << currToken;
      }
    }
  // zones have list without a uniform/nonuniform keyword
  // List<bool> is read as List<label>
  // (e. g. flipMap entry in faceZones)
  else if (currToken == "List<label>" || currToken == "List<bool>")
    {
    this->IsUniform = false;
    this->ReadNonuniformList<LABELLIST, listTraits<vtkIntArray, int> >(io);
    }
  else if (currToken.GetType() == this->Superclass::PUNCTUATION
      || currToken.GetType() == this->Superclass::LABEL || currToken.GetType()
      == this->Superclass::SCALAR || currToken.GetType()
      == this->Superclass::STRING || currToken.GetType()
      == this->Superclass::IDENTIFIER)
    {
    this->Superclass::operator=(currToken);
    }
}

// read values of an entry
void vtkFoamEntry::Read(vtkFoamIOobject& io)
{
  for (;;)
    {
    this->Superclass::push_back(new vtkFoamEntryValue(this));
    this->Superclass::back()->Read(io);

    if (this->Superclass::size() >= 2)
      {
      vtkFoamEntryValue& secondLastValue =
          *this->Superclass::operator[](this->Superclass::size() - 2);
      if (secondLastValue.GetType() == vtkFoamToken::LABEL)
        {
        vtkFoamEntryValue& lastValue = *this->Superclass::back();

        // a zero-sized nonuniform list without prefixing "nonuniform"
        // keyword nor list type specifier (i. e. `0()';
        // e. g. simpleEngine/0/polyMesh/pointZones) requires special
        // care (one with nonuniform prefix is treated within
        // vtkFoamEntryValue::read()). still this causes errornous
        // behavior for `0 nonuniform 0()' but this should be extremely
        // rare
        if (lastValue.GetType() == vtkFoamToken::EMPTYLIST && secondLastValue
            == 0)
          {
          delete this->Superclass::back();
          this->Superclass::pop_back(); // delete the last value
          // mark new last value as empty
          this->Superclass::back()->SetEmptyList();
          }
        // for an exceptional expression of `LABEL{LABELorSCALAR}' without
        // type prefix (e. g. `2{-0}' in mixedRhoE B.C. in
        // rhopSonicFoam/shockTube)
        else if (lastValue.GetType() == vtkFoamToken::DICTIONARY)
          {
          if (lastValue.Dictionary().GetType() == vtkFoamToken::LABEL)
            {
            const int asize = secondLastValue.To<int>();
            const int value = lastValue.Dictionary().GetToken().ToInt();
            // delete last two values
            delete this->Superclass::back();
            this->Superclass::pop_back();
            delete this->Superclass::back();
            this->Superclass::pop_back();
            // make new labelList
            this->Superclass::push_back(new vtkFoamEntryValue(this));
            this->Superclass::back()->MakeLabelList(value, asize);
            }
          else if (lastValue.Dictionary().GetType() == vtkFoamToken::SCALAR)
            {
            const int asize = secondLastValue.To<int>();
            const float value = lastValue.Dictionary().GetToken().ToFloat();
            // delete last two values
            delete this->Superclass::back();
            this->Superclass::pop_back();
            delete this->Superclass::back();
            this->Superclass::pop_back();
            // make new labelList
            this->Superclass::push_back(new vtkFoamEntryValue(this));
            this->Superclass::back()->MakeScalarList(value, asize);
            }
          }
        }
      }

    if (this->Superclass::back()->GetType() == vtkFoamToken::IDENTIFIER)
      {
      // substitute identifier
      const vtkStdString identifier(this->Superclass::back()->ToIdentifier());
      delete this->Superclass::back();
      this->Superclass::pop_back();

      for (const vtkFoamDict *uDictPtr = this->UpperDictPtr;;)
        {
        const vtkFoamEntry *identifiedEntry = uDictPtr->Lookup(identifier);

        if (identifiedEntry != NULL)
          {
          for (size_t valueI = 0; valueI < identifiedEntry->size(); valueI++)
            {
            this->Superclass::push_back(new vtkFoamEntryValue(
                *identifiedEntry->operator[](valueI), this));
            }
          break;
          }
        else
          {
          uDictPtr = uDictPtr->GetUpperDictPtr();
          if (uDictPtr == NULL)
            {
            throw vtkFoamError() << "substituting entry " << identifier
            << " not found";
            }
          }
        }
      }
    else if (*this->Superclass::back() == ';')
      {
      delete this->Superclass::back();
      this->Superclass::pop_back();
      break;
      }
    else if (this->Superclass::back()->GetType() == vtkFoamToken::DICTIONARY)
      {
      // subdictionary is not suffixed by an entry terminator ';'
      break;
      }
    else if (*this->Superclass::back() == '}' || *this->Superclass::back()
        == ')')
      {
      throw vtkFoamError() << "Unmatched " << *this->Superclass::back();
      }
    }
}

//-----------------------------------------------------------------------------
// vtkOpenFOAMReaderPrivate constructor and destructor
vtkOpenFOAMReaderPrivate::vtkOpenFOAMReaderPrivate()
{
  // DATA TIMES
  this->TimeStep = 0;
  this->TimeStepOld = -1;
  this->TimeValues = vtkDoubleArray::New();
  this->TimeNames = vtkStringArray::New();

  // selection
  this->InternalMeshSelectionStatus = 0;
  this->InternalMeshSelectionStatusOld = 0;

  // DATA COUNTS
  this->NumCells = 0;
  this->NumPoints = 0;

  this->VolFieldFiles = vtkStringArray::New();
  this->PointFieldFiles = vtkStringArray::New();
  this->LagrangianFieldFiles = vtkStringArray::New();
  this->PolyMeshPointsDir = vtkStringArray::New();
  this->PolyMeshFacesDir = vtkStringArray::New();

  // for creating cell-to-point translated data
  this->BoundaryPointMap = NULL;
  this->AllBoundaries = NULL;
  this->AllBoundariesPointMap = NULL;
  this->InternalPoints = NULL;

  // for caching mesh
  this->InternalMesh = NULL;
  this->BoundaryMesh = NULL;
  this->BoundaryPointMap = NULL;
  this->FaceOwner = NULL;
  this->PointZoneMesh = NULL;
  this->FaceZoneMesh = NULL;
  this->CellZoneMesh = NULL;

  // for decomposing polyhedra
  this->NumAdditionalCells = 0;
  this->AdditionalCellIds = NULL;
  this->NumAdditionalCells = NULL;
  this->AdditionalCellPoints = NULL;
}

vtkOpenFOAMReaderPrivate::~vtkOpenFOAMReaderPrivate()
{
  this->TimeValues->Delete();
  this->TimeNames->Delete();

  this->PolyMeshPointsDir->Delete();
  this->PolyMeshFacesDir->Delete();
  this->VolFieldFiles->Delete();
  this->PointFieldFiles->Delete();
  this->LagrangianFieldFiles->Delete();

  this->ClearMeshes();
}

void vtkOpenFOAMReaderPrivate::ClearInternalMeshes()
{
  if (this->FaceOwner != NULL)
    {
    this->FaceOwner->Delete();
    this->FaceOwner = NULL;
    }
  if (this->InternalMesh != NULL)
    {
    this->InternalMesh->Delete();
    this->InternalMesh = NULL;
    }
  if (this->AdditionalCellIds != NULL)
    {
    this->AdditionalCellIds->Delete();
    this->AdditionalCellIds = NULL;
    }
  if (this->NumAdditionalCells != NULL)
    {
    this->NumAdditionalCells->Delete();
    this->NumAdditionalCells = NULL;
    }
  delete this->AdditionalCellPoints;
  this->AdditionalCellPoints = NULL;

  if (this->PointZoneMesh != NULL)
    {
    this->PointZoneMesh->Delete();
    this->PointZoneMesh = NULL;
    }
  if (this->FaceZoneMesh != NULL)
    {
    this->FaceZoneMesh->Delete();
    this->FaceZoneMesh = NULL;
    }
  if (this->CellZoneMesh != NULL)
    {
    this->CellZoneMesh->Delete();
    this->CellZoneMesh = NULL;
    }
}

void vtkOpenFOAMReaderPrivate::ClearBoundaryMeshes()
{
  if (this->BoundaryMesh != NULL)
    {
    this->BoundaryMesh->Delete();
    this->BoundaryMesh = NULL;
    }

  delete this->BoundaryPointMap;
  this->BoundaryPointMap = NULL;

  if (this->InternalPoints != NULL)
    {
    this->InternalPoints->Delete();
    this->InternalPoints = NULL;
    }
  if (this->AllBoundaries != NULL)
    {
    this->AllBoundaries->Delete();
    this->AllBoundaries = NULL;
    }
  if (this->AllBoundariesPointMap != NULL)
    {
    this->AllBoundariesPointMap->Delete();
    this->AllBoundariesPointMap = NULL;
    }
}

void vtkOpenFOAMReaderPrivate::ClearMeshes()
{
  this->ClearInternalMeshes();
  this->ClearBoundaryMeshes();
}

void vtkOpenFOAMReaderPrivate::SetTimeValue(const double requestedTime)
{
  const int nTimeValues = this->TimeValues->GetNumberOfTuples();
  if (nTimeValues > 0)
    {
    int minTimeI = 0;
    double minTimeDiff = fabs(this->TimeValues->GetValue(0) - requestedTime);
    for (int timeI = 1; timeI < nTimeValues; timeI++)
      {
      const double timeDiff(fabs(this->TimeValues->GetValue(timeI)
          - requestedTime));
      if (timeDiff < minTimeDiff)
        {
        minTimeI = timeI;
        minTimeDiff = timeDiff;
        }
      }
    this->SetTimeStep(minTimeI); // set Modified() if TimeStep changed
    }
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::SetupInformation(const vtkStdString &casePath,
    const vtkStdString &regionName, const vtkStdString &procName,
    vtkOpenFOAMReaderPrivate *master)
{
  // copy parent, path and timestep information from master
  this->CasePath = casePath;
  this->RegionName = regionName;
  this->ProcessorName = procName;
  this->Parent = master->Parent;
  this->TimeValues->Delete();
  this->TimeValues = master->TimeValues;
  this->TimeValues->Register(0);
  this->TimeNames->Delete();
  this->TimeNames = master->TimeNames;
  this->TimeNames->Register(0);

  this->PopulatePolyMeshDirArrays();
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::GetFieldNames(const vtkStdString &tempPath,
    const bool isLagrangian, vtkStringArray *cellObjectNames,
    vtkStringArray *pointObjectNames)
{
  // open the directory and get num of files
  vtkDirectory *directory = vtkDirectory::New();
  if (!directory->Open(tempPath.c_str()))
    {
    // no data
    directory->Delete();
    return;
    }

  // loop over all files and locate valid fields
  int nFieldFiles = directory->GetNumberOfFiles();
  for (int j = 0; j < nFieldFiles; j++)
    {
    const vtkStdString fieldFile(directory->GetFile(j));
    const size_t len = fieldFile.length();

    // excluded extensions cf. src/OpenFOAM/OSspecific/Unix/Unix.C
    if (!directory->FileIsDirectory(fieldFile.c_str()) && fieldFile.substr(len
        - 1) != "~" && (len < 4 || (fieldFile.substr(len - 4) != ".bak"
        && fieldFile.substr(len - 4) != ".BAK" && fieldFile.substr(len - 4)
        != ".old")) && (len < 5 || fieldFile.substr(len - 5) != ".save"))
      {
      vtkFoamIOobject io(this->CasePath);
      if (io.Open(tempPath + "/" + fieldFile)) // file exists and readable
        {
        const vtkStdString& cn = io.GetClassName();
        if (isLagrangian)
          {
          if (cn == "labelField" || cn == "scalarField" || cn == "vectorField"
              || cn == "sphericalTensorField" || cn == "symmTensorField" || cn
              == "tensorField")
            {
            // real file name
            this->LagrangianFieldFiles->InsertNextValue(fieldFile);
            // object name
            pointObjectNames->InsertNextValue(io.GetObjectName());
            }
          }
        else
          {
          if (cn == "volScalarField" || cn == "pointScalarField" || cn
              == "volVectorField" || cn == "pointVectorField" || cn
              == "volSphericalTensorField" || cn == "pointSphericalTensorField"
              || cn == "volSymmTensorField" || cn == "pointSymmTensorField"
              || cn == "volTensorField" || cn == "pointTensorField")
            {
            if (cn.substr(0, 3) == "vol")
              {
              // real file name
              this->VolFieldFiles->InsertNextValue(fieldFile);
              // object name
              cellObjectNames->InsertNextValue(io.GetObjectName());
              }
            else
              {
              this->PointFieldFiles->InsertNextValue(fieldFile);
              pointObjectNames->InsertNextValue(io.GetObjectName());
              }
            }
          }
        io.Close();
        }
      }
    }
  // inserted objects are squeezed later in SortFieldFiles()
  directory->Delete();
}

//-----------------------------------------------------------------------------
// locate laglangian clouds
void vtkOpenFOAMReaderPrivate::LocateLagrangianClouds(
    vtkStringArray *lagrangianObjectNames, const vtkStdString &timePath)
{
  vtkDirectory *directory = vtkDirectory::New();
  if (directory->Open((timePath + this->RegionPath() + "/lagrangian").c_str()))
    {
    // search for sub-clouds (OF 1.5 format)
    const int nFiles = directory->GetNumberOfFiles();
    bool isSubCloud = false;
    for (int fileI = 0; fileI < nFiles; fileI++)
      {
      const vtkStdString fileNameI(directory->GetFile(fileI));
      if (fileNameI != "." && fileNameI != ".."
          && directory->FileIsDirectory(fileNameI.c_str()))
        {
        vtkFoamIOobject io(this->CasePath);
        const vtkStdString subCloudName(this->RegionPrefix() + "lagrangian/"
            + fileNameI);
        const vtkStdString subCloudFullPath(timePath + "/" + subCloudName);
        // lagrangian positions. there are many concrete class names
        // e. g. Cloud<parcel>, basicKinematicCloud etc.
        if ((io.Open(subCloudFullPath + "/positions")
            || io.Open(subCloudFullPath + "/positions.gz")) && io.GetClassName().find("Cloud") != vtkStdString::npos && io.GetObjectName()
            == "positions")
          {
          isSubCloud = true;
          // a lagrangianPath has to be in a bit different format from
          // subCloudName to make the "lagrangian" reserved path
          // component and a mesh region with the same name
          // distinguishable later
          const vtkStdString subCloudPath(this->RegionName + "/lagrangian/"
              + fileNameI);
          if (this->Parent->LagrangianPaths->LookupValue(subCloudPath) == -1)
            {
            this->Parent->LagrangianPaths->InsertNextValue(subCloudPath);
            }
          this->GetFieldNames(subCloudFullPath, true, NULL,
              lagrangianObjectNames);
          this->Parent->PatchDataArraySelection->AddArray(subCloudName.c_str());
          }
        }
      }
    // if there's no sub-cloud then OF < 1.5 format
    if (!isSubCloud)
      {
      vtkFoamIOobject io(this->CasePath);
      const vtkStdString cloudName(this->RegionPrefix() + "lagrangian");
      const vtkStdString cloudFullPath(timePath + "/" + cloudName);
      if ((io.Open(cloudFullPath + "/positions") || io.Open(cloudFullPath
          + "/positions.gz")) && io.GetClassName().find("Cloud") != vtkStdString::npos && io.GetObjectName()
          == "positions")
        {
        const vtkStdString cloudPath(this->RegionName + "/lagrangian");
        if (this->Parent->LagrangianPaths->LookupValue(cloudPath) == -1)
          {
          this->Parent->LagrangianPaths->InsertNextValue(cloudPath);
          }
        this->GetFieldNames(cloudFullPath, true, NULL, lagrangianObjectNames);
        this->Parent->PatchDataArraySelection->AddArray(cloudName.c_str());
        }
      }
    this->Parent->LagrangianPaths->Squeeze();
    }
  directory->Delete();
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::SortFieldFiles(vtkStringArray *selections,
    vtkStringArray *files, vtkStringArray *objects)
{
  objects->Squeeze();
  files->Squeeze();
  vtkSortDataArray::Sort(objects, files);
  for (int nameI = 0; nameI < objects->GetNumberOfValues(); nameI++)
    {
    selections->InsertNextValue(objects->GetValue(nameI));
    }
  objects->Delete();
}

//-----------------------------------------------------------------------------
// create field data lists and cell/point array selection lists
int vtkOpenFOAMReaderPrivate::MakeMetaDataAtTimeStep(
    vtkStringArray *cellSelectionNames, vtkStringArray *pointSelectionNames,
    vtkStringArray *lagrangianSelectionNames, const bool listNextTimeStep)
{
  // Read the patches from the boundary file into selection array
  if (this->PolyMeshFacesDir->GetValue(this->TimeStep)
      != this->BoundaryDict.TimeDir
      || this->Parent->PatchDataArraySelection->GetMTime()
          != this->Parent->PatchSelectionMTimeOld)
    {
    this->BoundaryDict.clear();
    this->BoundaryDict.TimeDir
        = this->PolyMeshFacesDir->GetValue(this->TimeStep);

    const bool isSubRegion = this->RegionName != "";
    vtkFoamDict *boundaryDict = this->GatherBlocks("boundary", isSubRegion);
    if (boundaryDict == NULL)
      {
      if (isSubRegion)
        {
        return 0;
        }
      }
    else
      {
      // Add the internal mesh by default always
      const vtkStdString
          internalMeshName(this->RegionPrefix() + "internalMesh");
      this->Parent->PatchDataArraySelection->AddArray(internalMeshName.c_str());
      this->InternalMeshSelectionStatus
          = this->Parent->GetPatchArrayStatus(internalMeshName.c_str());

      // iterate through each entry in the boundary file
      int allBoundariesNextStartFace = 0;
      this->BoundaryDict.resize(boundaryDict->size());
      for (size_t i = 0; i < boundaryDict->size(); i++)
        {
        vtkFoamEntry *boundaryEntryI = boundaryDict->operator[](i);
        const vtkFoamEntry *nFacesEntry = boundaryEntryI->Dictionary().Lookup("nFaces");
        if (nFacesEntry == NULL)
          {
          vtkErrorMacro(<< "nFaces entry not found in boundary entry "
              << boundaryEntryI->GetKeyword().c_str());
          delete boundaryDict;
          return 0;
          }
        const int nFaces = nFacesEntry->ToInt();

        // extract name of the current patch for insertion
        const vtkStdString &boundaryNameI = boundaryEntryI->GetKeyword();

        // create BoundaryDict entry
        vtkFoamBoundaryEntry &BoundaryEntryI = this->BoundaryDict[i];
        BoundaryEntryI.NFaces = nFaces;
        BoundaryEntryI.BoundaryName = boundaryNameI;
        const vtkFoamEntry *startFaceEntry = boundaryEntryI->Dictionary().Lookup("startFace");
        if (startFaceEntry == NULL)
          {
          vtkErrorMacro(<< "startFace entry not found in boundary entry "
              << boundaryEntryI->GetKeyword().c_str());
          delete boundaryDict;
          return 0;
          }
        BoundaryEntryI.StartFace = startFaceEntry->ToInt();
        const vtkFoamEntry *typeEntry = boundaryEntryI->Dictionary().Lookup("type");
        if (typeEntry == NULL)
          {
          vtkErrorMacro(<< "type entry not found in boundary entry "
              << boundaryEntryI->GetKeyword().c_str());
          delete boundaryDict;
          return 0;
          }
        BoundaryEntryI.AllBoundariesStartFace = allBoundariesNextStartFace;
        const vtkStdString typeNameI(typeEntry->ToString());
        // if the basic type of the patch is one of the followings the
        // point-filtered values at patches are overridden by patch values
        if (typeNameI == "patch" || typeNameI == "wall")
          {
          BoundaryEntryI.BoundaryType = vtkFoamBoundaryEntry::PHYSICAL;
          allBoundariesNextStartFace += nFaces;
          }
        else if (typeNameI == "processor")
          {
          BoundaryEntryI.BoundaryType = vtkFoamBoundaryEntry::PROCESSOR;
          allBoundariesNextStartFace += nFaces;
          }
        else
          {
          BoundaryEntryI.BoundaryType = vtkFoamBoundaryEntry::GEOMETRICAL;
          }
        BoundaryEntryI.IsActive = false;

        // always hide processor patches for decomposed cases to keep
        // vtkAppendCompositeDataLeaves happy
        if (this->ProcessorName != "" && BoundaryEntryI.BoundaryType
            == vtkFoamBoundaryEntry::PROCESSOR)
          {
          continue;
          }
        const vtkStdString selectionName(this->RegionPrefix() + boundaryNameI);
        if (this->Parent->PatchDataArraySelection->
        ArrayExists(selectionName.c_str()))
          {
          // Mark boundary if selected for display
          if (this->Parent->GetPatchArrayStatus(selectionName.c_str()))
            {
            BoundaryEntryI.IsActive = true;
            }
          }
        else
          {
          // add patch to list with selection status turned off:
          // the patch is added to list even if its size is zero
          this->Parent->PatchDataArraySelection->DisableArray(selectionName.c_str());
          }
        }

      delete boundaryDict;
      }
    }

  // Add scalars and vectors to metadata
  vtkStdString timePath(this->CurrentTimePath());
  // do not do "RemoveAllArrays()" to accumulate array selections
  // this->CellDataArraySelection->RemoveAllArrays();
  this->VolFieldFiles->Initialize();
  this->PointFieldFiles->Initialize();
  vtkStringArray *cellObjectNames = vtkStringArray::New();
  vtkStringArray *pointObjectNames = vtkStringArray::New();
  this->GetFieldNames(timePath + this->RegionPath(), false, cellObjectNames,
      pointObjectNames);

  this->LagrangianFieldFiles->Initialize();
  if (listNextTimeStep)
    {
    this->Parent->LagrangianPaths->Initialize();
    }
  vtkStringArray *lagrangianObjectNames = vtkStringArray::New();
  this->LocateLagrangianClouds(lagrangianObjectNames, timePath);

  // if the requested timestep is 0 then we also look at the next
  // timestep to add extra objects that don't exist at timestep 0 into
  // selection lists. Note the ObjectNames array will be recreated in
  // RequestData() so we don't have to worry about duplicated fields.
  if (listNextTimeStep && this->TimeValues->GetNumberOfTuples() >= 2
      && this->TimeStep == 0)
    {
    const vtkStdString timePath2(this->TimePath(1));
    this->GetFieldNames(timePath2 + this->RegionPath(), false, cellObjectNames,
        pointObjectNames);
    // if lagrangian clouds were not found at timestep 0
    if (this->Parent->LagrangianPaths->GetNumberOfTuples() == 0)
      {
      this->LocateLagrangianClouds(lagrangianObjectNames, timePath2);
      }
    }

  // sort array names
  this->SortFieldFiles(cellSelectionNames, this->VolFieldFiles, cellObjectNames);
  this->SortFieldFiles(pointSelectionNames, this->PointFieldFiles,
      pointObjectNames);
  this->SortFieldFiles(lagrangianSelectionNames, this->LagrangianFieldFiles,
      lagrangianObjectNames);

  return 1;
}

//-----------------------------------------------------------------------------
// list time directories according to controlDict
bool vtkOpenFOAMReaderPrivate::ListTimeDirectoriesByControlDict(
    vtkFoamDict* dictPtr)
{
  vtkFoamDict& dict = *dictPtr;

  const vtkFoamEntry *startTimeEntry = dict.Lookup("startTime");
  if (startTimeEntry == NULL)
    {
    vtkErrorMacro(<< "startTime entry not found in controlDict");
    return false;
    }
  // using double to precisely handle time values
  const double startTime = startTimeEntry->ToDouble();

  const vtkFoamEntry *endTimeEntry = dict.Lookup("endTime");
  if (endTimeEntry == NULL)
    {
    vtkErrorMacro(<< "endTime entry not found in controlDict");
    return false;
    }
  const double endTime = endTimeEntry->ToDouble();

  const vtkFoamEntry *deltaTEntry = dict.Lookup("deltaT");
  if (deltaTEntry == NULL)
    {
    vtkErrorMacro(<< "deltaT entry not found in controlDict");
    return false;
    }
  const double deltaT = deltaTEntry->ToDouble();

  const vtkFoamEntry *writeIntervalEntry = dict.Lookup("writeInterval");
  if (writeIntervalEntry == NULL)
    {
    vtkErrorMacro(<< "writeInterval entry not found in controlDict");
    return false;
    }
  const double writeInterval = writeIntervalEntry->ToDouble();

  const vtkFoamEntry *timeFormatEntry = dict.Lookup("timeFormat");
  if (timeFormatEntry == NULL)
    {
    vtkErrorMacro(<< "timeFormat entry not found in controlDict");
    return false;
    }
  const vtkStdString timeFormat(timeFormatEntry->ToString());

  const vtkFoamEntry *timePrecisionEntry = dict.Lookup("timePrecision");
  const int timePrecision // default is 6
      = (timePrecisionEntry != NULL ? timePrecisionEntry->ToInt() : 6);

  // calculate the time step increment based on type of run
  const vtkFoamEntry *writeControlEntry = dict.Lookup("writeControl");
  if (writeControlEntry == NULL)
    {
    vtkErrorMacro(<< "writeControl entry not found in controlDict");
    return false;
    }
  const vtkStdString writeControl(writeControlEntry->ToString());
  double timeStepIncrement;
  if (writeControl == "timeStep")
    {
    timeStepIncrement = writeInterval * deltaT;
    }
  else if (writeControl == "runTime" || writeControl == "adjustableRunTime")
    {
    timeStepIncrement = writeInterval;
    }
  else
    {
    vtkErrorMacro(<<"Time step can't be determined because writeControl is"
        " set to " << writeControl.c_str());
    return false;
    }

  // calculate how many timesteps there should be
  const double tempResult = (endTime - startTime) / timeStepIncrement;
  // +0.5 to round up
  const int tempNumTimeSteps = static_cast<int>(tempResult + 0.5) + 1;

  // make sure time step dir exists
  vtkstd::vector<double> tempSteps;
  vtkDirectory *test = vtkDirectory::New();
  this->TimeValues->Initialize();
  this->TimeNames->Initialize();

  // determine time name based on Foam::Time::timeName()
  // cf. src/OpenFOAM/db/Time/Time.C
  vtksys_ios::ostringstream parser;
#ifdef _MSC_VER
  bool correctExponent = true;
#endif
  if (timeFormat == "general")
    {
    parser.setf(vtksys_ios::ios_base::fmtflags(0), vtksys_ios::ios_base::floatfield);
    }
  else if (timeFormat == "fixed")
    {
    parser.setf(vtksys_ios::ios_base::fmtflags(vtksys_ios::ios_base::fixed),
        vtksys_ios::ios_base::floatfield);
#ifdef _MSC_VER
    correctExponent = false;
#endif
    }
  else if (timeFormat == "scientific")
    {
    parser.setf(vtksys_ios::ios_base::fmtflags(vtksys_ios::ios_base::scientific),
        vtksys_ios::ios_base::floatfield);
    }
  else
    {
    vtkWarningMacro("Warning: unsupported time format. Assuming general.");
    parser.setf(vtksys_ios::ios_base::fmtflags(0), vtksys_ios::ios_base::floatfield);
    }
  parser.precision(timePrecision);

  for (int i = 0; i < tempNumTimeSteps; i++)
    {
    parser.str("");
    const double tempStep = i * timeStepIncrement + startTime;
    parser << tempStep; // stringstream doesn't require ends
#ifdef _MSC_VER
    // workaround for format difference in MSVC++:
    // remove an extra 0 from exponent
    if(correctExponent)
      {
      vtkStdString tempStr(parser.str());
      vtkStdString::size_type pos = tempStr.find('e');
      if(pos != vtkStdString::npos && tempStr.length() >= pos + 3
          && tempStr[pos + 2] == '0')
        {
        tempStr.erase(pos + 2, 1);
        parser.str(tempStr);
        }
      }
#endif
    // Add the time steps that actually exist to steps
    // allows the run to be stopped short of controlDict spec
    // allows for removal of timesteps
    if (test->Open((this->CasePath + parser.str()).c_str()))
      {
      this->TimeValues->InsertNextValue(tempStep);
      this->TimeNames->InsertNextValue(parser.str());
      }
    // necessary for reading the case/0 directory whatever the timeFormat is
    // based on Foam::Time::operator++() cf. src/OpenFOAM/db/Time/Time.C
    else if ((fabs(tempStep) < 1.0e-14L) // 10*SMALL
        && test->Open((this->CasePath + vtkStdString("0")).c_str()))
      {
      this->TimeValues->InsertNextValue(tempStep);
      this->TimeNames->InsertNextValue(vtkStdString("0"));
      }
    }
  test->Delete();
  this->TimeValues->Squeeze();
  this->TimeNames->Squeeze();

  if (this->TimeValues->GetNumberOfTuples() == 0)
    {
    // set the number of timesteps to 1 if the constant subdirectory exists
    test = vtkDirectory::New();
    if (test->Open((this->CasePath + "constant").c_str()))
      {
      parser.str("");
      parser << startTime;
      this->TimeValues->InsertNextValue(startTime);
      this->TimeValues->Squeeze();
      this->TimeNames->InsertNextValue(parser.str());
      this->TimeNames->Squeeze();
      }
    test->Delete();
    }
  return true;
}

//-----------------------------------------------------------------------------
// list time directories by searching all valid time instances in a
// case directory
bool vtkOpenFOAMReaderPrivate::ListTimeDirectoriesByInstances()
{
  // open the case directory
  vtkDirectory* test = vtkDirectory::New();
  if (!test->Open(this->CasePath.c_str()))
    {
    test->Delete();
    vtkErrorMacro(<< "Can't open directory " << this->CasePath.c_str());
    return false;
    }

  // search all the directories in the case directory and detect
  // directories with names convertible to numbers
  this->TimeValues->Initialize();
  this->TimeNames->Initialize();
  const int nFiles = test->GetNumberOfFiles();
  for (int i = 0; i < nFiles; i++)
    {
    const vtkStdString dir = test->GetFile(i);
    if (test->FileIsDirectory(dir.c_str()))
      {
      // check if the name is convertible to a number
      bool isTimeDir = true;
      for (size_t j = 0; j < dir.length(); j++)
        {
        const char c = dir[j];
        if (!isdigit(c) && c != '+' && c != '-' && c != '.' && c != 'e' && c
            != 'E')
          {
          isTimeDir = false;
          break;
          }
        }
      if (!isTimeDir)
        {
        continue;
        }

      // convert to a number
      char *endptr;
      double timeValue = strtod(dir.c_str(), &endptr);
      // check if the value really was converted to a number
      if (timeValue == 0.0 && endptr == dir.c_str())
        {
        continue;
        }

      // add to the instance list
      this->TimeValues->InsertNextValue(timeValue);
      this->TimeNames->InsertNextValue(dir);
      }
    }
  test->Delete();

  this->TimeValues->Squeeze();
  this->TimeNames->Squeeze();

  if (this->TimeValues->GetNumberOfTuples() > 1)
    {
    // sort the detected time directories
    vtkSortDataArray::Sort(this->TimeValues, this->TimeNames);

    // if there are duplicated timeValues found, remove duplicates
    // (e.g. "0" and "0.000")
    for (int timeI = 1; timeI < this->TimeValues->GetNumberOfTuples(); timeI++)
      {
      // compare by exact match
      if (this->TimeValues->GetValue(timeI - 1)
          == this->TimeValues->GetValue(timeI))
        {
        vtkWarningMacro(<<"Different time directories with the same time value "
            << this->TimeNames->GetValue(timeI - 1).c_str() << " and "
            << this->TimeNames->GetValue(timeI).c_str() << " found. "
            << this->TimeNames->GetValue(timeI).c_str() << " will be ignored.");
        this->TimeValues->RemoveTuple(timeI);
        // vtkStringArray does not have RemoveTuple()
        for (int timeJ = timeI + 1; timeJ
            < this->TimeNames->GetNumberOfTuples(); timeJ++)
          {
          this->TimeNames->SetValue(timeJ - 1, this->TimeNames->GetValue(timeJ));
          }
        this->TimeNames->Resize(this->TimeNames->GetNumberOfTuples() - 1);
        }
      }
    }
  else if (this->TimeValues->GetNumberOfTuples() == 0)
    {
    // set the number of timesteps to 1 if the constant subdirectory exists
    test = vtkDirectory::New();
    if (test->Open((this->CasePath + "constant").c_str()))
      {
      this->TimeValues->InsertNextValue(0.0);
      this->TimeValues->Squeeze();
      this->TimeNames->InsertNextValue("0");
      this->TimeNames->Squeeze();
      }
    test->Delete();
    }

  return true;
}

//-----------------------------------------------------------------------------
// gather the necessary information to create a path to the data
bool vtkOpenFOAMReaderPrivate::MakeInformationVector(
    const vtkStdString &casePath, const vtkStdString &controlDictPath,
    const vtkStdString &procName, vtkOpenFOAMReader *parent)
{
  this->CasePath = casePath;
  this->ProcessorName = procName;
  this->Parent = parent;

  // list timesteps (skip parsing controlDict entirely if
  // ListTimeStepsByControlDict is set to 0)
  bool ret = false; // tentatively set to false to suppress warning by older compilers
  if (this->Parent->GetListTimeStepsByControlDict())
    {
    vtkFoamIOobject io(this->CasePath);

    // open and check if controlDict is readable
    if (!io.Open(controlDictPath))
      {
      vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
          << io.GetError().c_str());
      return false;
      }
    vtkFoamDict dict;
    if (!dict.Read(io))
      {
      vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
          << " of " << io.GetFileName().c_str() << ": " << io.GetError().c_str());
      return false;
      }
    if (dict.GetType() != vtkFoamToken::DICTIONARY)
      {
      vtkErrorMacro(<<"The file type of " << io.GetFileName().c_str()
          << " is not a dictionary");
      return false;
      }

    const vtkFoamEntry *writeControlEntry = dict.Lookup("writeControl");
    if (writeControlEntry == NULL)
      {
      vtkErrorMacro(<< "writeControl entry not found in "
          << io.GetFileName().c_str());
      return false;
      }
    const vtkStdString writeControl(writeControlEntry->ToString());

    // empty if not found
    const vtkFoamEntry *adjustTimeStepEntry = dict.Lookup("adjustTimeStep");
    const vtkStdString
        adjustTimeStep = adjustTimeStepEntry == NULL ? vtkStdString()
            : adjustTimeStepEntry->ToString();

    // list time directories according to controlDict if (adjustTimeStep
    // writeControl) == (off, timeStep) or (on, adjustableRunTime); list
    // by time instances in the case directory otherwise (different behavior
    // from paraFoam)
    // valid switching words cf. src/OpenFOAM/db/Switch/Switch.C
    if ((((adjustTimeStep == "off" || adjustTimeStep == "no" || adjustTimeStep
        == "n" || adjustTimeStep == "false" || adjustTimeStep == "")
        && writeControl == "timeStep") || ((adjustTimeStep == "on"
        || adjustTimeStep == "yes" || adjustTimeStep == "y" || adjustTimeStep
        == "true") && writeControl == "adjustableRunTime")))
      {
      ret = this->ListTimeDirectoriesByControlDict(&dict);
      }
    else
      {
      ret = this->ListTimeDirectoriesByInstances();
      }
    }
  else
    {
    ret = this->ListTimeDirectoriesByInstances();
    }

  if (!ret)
    {
    return ret;
    }

  // does not seem to be required even if number of timesteps reduced
  // upon refresh since ParaView rewinds TimeStep to 0, but for precaution
  if (this->TimeValues->GetNumberOfTuples() > 0)
    {
    if (this->TimeStep >= this->TimeValues->GetNumberOfTuples())
      {
      this->SetTimeStep(this->TimeValues->GetNumberOfTuples() - 1);
      }
    }
  else
    {
    this->SetTimeStep(0);
    }

  this->PopulatePolyMeshDirArrays();
  return ret;
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::AppendMeshDirToArray(
    vtkStringArray* polyMeshDir, const vtkStdString &path, const int timeI)
{
  vtkFoamIOobject io(this->CasePath);

  if (io.Open(path) || io.Open(path + ".gz"))
    {
    io.Close();
    // set points/faces location to current timesteps value
    polyMeshDir->SetValue(timeI, this->TimeNames->GetValue(timeI));
    }
  else
    {
    if (timeI != 0)
      {
      // set points/faces location to previous timesteps value
      polyMeshDir->SetValue(timeI, polyMeshDir->GetValue(timeI - 1));
      }
    else
      {
      // set points/faces to constant
      polyMeshDir->SetValue(timeI, "constant");
      }
    }
}

//-----------------------------------------------------------------------------
// create a Lookup Table containing the location of the points
// and faces files for each time steps mesh
void vtkOpenFOAMReaderPrivate::PopulatePolyMeshDirArrays()
{
  // intialize size to number of timesteps
  const int nSteps = this->TimeValues->GetNumberOfTuples();
  this->PolyMeshPointsDir->SetNumberOfValues(nSteps);
  this->PolyMeshFacesDir->SetNumberOfValues(nSteps);

  // loop through each timestep
  for (int i = 0; i < nSteps; i++)
    {
    // create the path to the timestep
    vtkStdString polyMeshPath = this->TimeRegionPath(i) + "/polyMesh/";
    AppendMeshDirToArray(this->PolyMeshPointsDir, polyMeshPath + "points", i);
    AppendMeshDirToArray(this->PolyMeshFacesDir, polyMeshPath + "faces", i);
    }
  return;
}

//-----------------------------------------------------------------------------
// read the points file into a vtkFloatArray
vtkFloatArray* vtkOpenFOAMReaderPrivate::ReadPointsFile()
{
  // path to points file
  const vtkStdString pointPath =
      this->CurrentTimeRegionMeshPath(this->PolyMeshPointsDir) + "points";

  vtkFoamIOobject io(this->CasePath);
  if (!(io.Open(pointPath) || io.Open(pointPath + ".gz")))
    {
    vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
        << io.GetError().c_str());
    return NULL;
    }

  vtkFoamEntryValue dict(NULL);
  try
    {
    dict.ReadNonuniformList<vtkFoamToken::VECTORLIST,
    vtkFoamEntryValue::vectorListTraits<vtkFloatArray, float, 3, false> >(io);
    }
  catch(vtkFoamError& e)
    {
    vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
        << " of " << io.GetFileName().c_str() << ": " << e.c_str());
    return NULL;
    }

  vtkFloatArray *pointArray = static_cast<vtkFloatArray *>(dict.Ptr());

  // set the number of points
  this->NumPoints = pointArray->GetNumberOfTuples();

  return pointArray;
}

//-----------------------------------------------------------------------------
// read the faces into a vtkFoamIntVectorVector
vtkFoamIntVectorVector * vtkOpenFOAMReaderPrivate::ReadFacesFile(
    const vtkStdString &facePathIn)
{
  const vtkStdString facePath(facePathIn + "faces");

  vtkFoamIOobject io(this->CasePath);
  if (!(io.Open(facePath) || io.Open(facePath + ".gz")))
    {
    vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
        << io.GetError().c_str() << ". If you are trying to read a parallel "
        "decomposed case, set Case Type to Decomposed Case.");
    return NULL;
    }

  vtkFoamEntryValue dict(NULL);
  try
    {
    if (io.GetClassName() == "faceCompactList")
      {
      dict.ReadCompactIOLabelList(io);
      }
    else
      {
      dict.ReadLabelListList(io);
      }
    }
  catch(vtkFoamError& e)
    {
    vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
        << " of " << io.GetFileName().c_str() << ": " << e.c_str());
    return NULL;
    }
  return static_cast<vtkFoamIntVectorVector *>(dict.Ptr());
}

//-----------------------------------------------------------------------------
// read the owner and neighbor file and create cellFaces
vtkFoamIntVectorVector * vtkOpenFOAMReaderPrivate::ReadOwnerNeighborFiles(
    const vtkStdString &ownerNeighborPath, vtkFoamIntVectorVector *facePoints)
{
  vtkFoamIOobject io(this->CasePath);
  vtkStdString ownerPath(ownerNeighborPath + "owner");
  if (io.Open(ownerPath) || io.Open(ownerPath + ".gz"))
    {
    vtkFoamEntryValue ownerDict(NULL);
    try
      {
      ownerDict.ReadNonuniformList<vtkFoamToken::LABELLIST,
      vtkFoamEntryValue::listTraits<vtkIntArray, int> >(io);
      }
    catch(vtkFoamError& e)
      {
      vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
          << " of " << io.GetFileName().c_str() << ": " << e.c_str());
      return NULL;
      }

    io.Close();

    const vtkStdString neighborPath(ownerNeighborPath + "neighbour");
    if (!(io.Open(neighborPath) || io.Open(neighborPath + ".gz")))
      {
      vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
          << io.GetError().c_str());
      return NULL;
      }

    vtkFoamEntryValue neighborDict(NULL);
    try
      {
      neighborDict.ReadNonuniformList<vtkFoamToken::LABELLIST,
      vtkFoamEntryValue::listTraits<vtkIntArray, int> >(io);
      }
    catch(vtkFoamError& e)
      {
      vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
          << " of " << io.GetFileName().c_str() << ": " << e.c_str());
      return NULL;
      }

    this->FaceOwner = static_cast<vtkIntArray *>(ownerDict.Ptr());
    vtkIntArray &faceOwner = *this->FaceOwner;
    vtkIntArray &faceNeighbor = neighborDict.LabelList();

    const int nFaces = faceOwner.GetNumberOfTuples();
    const int nNeiFaces = faceNeighbor.GetNumberOfTuples();

    if (nFaces < nNeiFaces)
      {
      vtkErrorMacro(<<"Numbers of owner faces " << nFaces
          << " must be equal or larger than number of neighbor faces "
          << nNeiFaces);
      return NULL;
      }

    if (nFaces != facePoints->GetNumberOfElements())
      {
      vtkWarningMacro(<<"Numbers of faces in faces "
          << facePoints->GetNumberOfElements() << " and owner " << nFaces
          << " does not match");
      return NULL;
      }

    // add the face numbers to the correct cell cf. Terry's code and
    // src/OpenFOAM/meshes/primitiveMesh/primitiveMeshCells.C
    // find the number of cells
    int nCells = -1;
    for (int faceI = 0; faceI < nNeiFaces; faceI++)
      {
      const int ownerCell = faceOwner.GetValue(faceI);
      if (nCells < ownerCell) // max(nCells, faceOwner[i])
        {
        nCells = ownerCell;
        }
      // we do need to take neighbor faces into account since all the
      // surrounding faces of a cell can be neighbors for a valid mesh
      const int neighborCell = faceNeighbor.GetValue(faceI);
      if (nCells < neighborCell) // max(nCells, faceNeighbor[i])
        {
        nCells = neighborCell;
        }
      }
    for (int faceI = nNeiFaces; faceI < nFaces; faceI++)
      {
      const int ownerCell = faceOwner.GetValue(faceI);
      if (nCells < ownerCell) // max(nCells, faceOwner[i])
        {
        nCells = ownerCell;
        }
      }
    nCells++;

    if (nCells == 0)
      {
      vtkWarningMacro(<<"The mesh contains no cells");
      }

    // set the number of cells
    this->NumCells = nCells;

    // create cellFaces with the length of the body undetermined
    vtkFoamIntVectorVector *cells = new vtkFoamIntVectorVector(nCells, 1);

    // count number of faces for each cell
    int *cfiPtr = cells->GetIndices()->GetPointer(0);
    for (int cellI = 0; cellI <= nCells; cellI++)
      {
      cfiPtr[cellI] = 0;
      }
    int nTotalCellFaces = 0;
    cfiPtr++; // offset +1
    for (int faceI = 0; faceI < nNeiFaces; faceI++)
      {
      const int ownerCell = faceOwner.GetValue(faceI);
      // simpleFoam/pitzDaily3Blocks has faces with owner cell number -1
      if (ownerCell >= 0)
        {
        cfiPtr[ownerCell]++;
        nTotalCellFaces++;
        }
      const int neighborCell=faceNeighbor.GetValue(faceI);
      if (neighborCell >= 0)
        {
        cfiPtr[neighborCell]++;
        nTotalCellFaces++;
        }
      }
    for (int faceI = nNeiFaces; faceI < nFaces; faceI++)
      {
      const int ownerCell = faceOwner.GetValue(faceI);
      if (ownerCell >= 0)
        {
        cfiPtr[ownerCell]++;
        nTotalCellFaces++;
        }
      }
    cfiPtr--; // revert offset +1

    // allocate cellFaces. To reduce the numbers of new/delete operations we
    // allocate memory space for all faces linearly
    cells->ResizeBody(nTotalCellFaces);

    // accumulate the number of cellFaces to create cellFaces indices
    // and copy them to a temporary array
    vtkIntArray *tmpFaceIndices = vtkIntArray::New();
    tmpFaceIndices->SetNumberOfValues(nCells + 1);
    int *tfiPtr = tmpFaceIndices->GetPointer(0);
    tfiPtr[0] = 0;
    for (int cellI = 1; cellI <= nCells; cellI++)
      {
      tfiPtr[cellI] = (cfiPtr[cellI] += cfiPtr[cellI - 1]);
      }

    // add face numbers to cell-faces list
    vtkIntArray *cellFacesList = cells->GetBody();
    for (int faceI = 0; faceI < nNeiFaces; faceI++)
      {
      const int ownerCell = faceOwner.GetValue(faceI); // must be a signed int
      // simpleFoam/pitzDaily3Blocks has faces with owner cell number -1
      if (ownerCell >= 0)
        {
        cellFacesList->SetValue(tfiPtr[ownerCell]++, faceI);
        }
      const int neighborCell = faceNeighbor.GetValue(faceI);
      if (neighborCell >= 0)
        {
        cellFacesList->SetValue(tfiPtr[neighborCell]++, faceI);
        }
      }
    for (int faceI = nNeiFaces; faceI < nFaces; faceI++)
      {
      const int ownerCell = faceOwner.GetValue(faceI); // must be a signed int
      // simpleFoam/pitzDaily3Blocks has faces with owner cell number -1
      if (ownerCell >= 0)
        {
        cellFacesList->SetValue(tfiPtr[ownerCell]++, faceI);
        }
      }
    tmpFaceIndices->Delete();

    return cells;
    }
  else // if owner does not exist look for cells
    {
    vtkStdString cellsPath(ownerNeighborPath + "cells");
    if (!(io.Open(cellsPath) || io.Open(cellsPath + ".gz")))
      {
      vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
          << io.GetError().c_str());
      return NULL;
      }
    vtkFoamEntryValue cellsDict(NULL);
    try
      {
      cellsDict.ReadLabelListList(io);
      }
    catch(vtkFoamError& e)
      {
      vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
          << " of " << io.GetFileName().c_str() << ": " << e.c_str());
      return NULL;
      }

    vtkFoamIntVectorVector *cells =
        static_cast<vtkFoamIntVectorVector *>(cellsDict.Ptr());
    this->NumCells = cells->GetNumberOfElements();
    const int nFaces = facePoints->GetNumberOfElements();

    // create face owner list
    this->FaceOwner = vtkIntArray::New();
    this->FaceOwner->SetNumberOfTuples(nFaces);
    for (int faceI = 0; faceI < nFaces; faceI++)
      {
      this->FaceOwner->SetValue(faceI, -1);
      }
    for (int cellI = 0; cellI < this->NumCells; cellI++)
      {
      const int nCellFaces = cells->GetSize(cellI);
      const int *cellFaces = cells->operator[](cellI);
      for (int faceI = 0; faceI < nCellFaces; faceI++)
        {
        const int f = cellFaces[faceI];
        if (f < 0 || f >= nFaces) // make sure the face number is valid
          {
          vtkErrorMacro("Face number " << f << " in cell " << cellI
              << " exceeds the number of faces " << nFaces);
          this->FaceOwner->Delete();
          this->FaceOwner = NULL;
          delete cells;
          return NULL;
          }
        const int owner = this->FaceOwner->GetValue(f);
        if (owner == -1 || owner > cellI)
          {
          this->FaceOwner->SetValue(f, cellI);
          }
        }
      }
    // check for unused faces
    for (int faceI = 0; faceI < nFaces; faceI++)
      {
      if (this->FaceOwner->GetValue(faceI) == -1)
        {
        vtkErrorMacro(<<"Face " << faceI << " is not used");
        this->FaceOwner->Delete();
        this->FaceOwner = NULL;
        delete cells;
        return NULL;
        }
      }
    return cells;
    }
}

//-----------------------------------------------------------------------------
bool vtkOpenFOAMReaderPrivate::CheckFacePoints(
    vtkFoamIntVectorVector *facePoints)
{
  const int nFaces = facePoints->GetNumberOfElements();

  for (int faceI = 0; faceI < nFaces; faceI++)
    {
    const int nPoints = facePoints->GetSize(faceI);
    const int *pointList = facePoints->operator[](faceI);
    if (nPoints < 3)
      {
      vtkErrorMacro(<< "Face " << faceI << " has only " << nPoints
          << " points which is not enough to constitute a face"
          " (a face must have at least 3 points)");
      return false;
      }
    for (int pointI = 0; pointI < nPoints; pointI++)
      {
      const int p = pointList[pointI];
      if (p < 0 || p >= this->NumPoints)
        {
        vtkErrorMacro(<< "The point number " << p << " at face number " << faceI
            << " is out of range for " << this->NumPoints << " points");
        return false;
        }
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
// determine cell shape and insert the cell into the mesh
// hexahedron, prism, pyramid, tetrahedron and decompose polyhedron
void vtkOpenFOAMReaderPrivate::InsertCellsToGrid(
    vtkUnstructuredGrid* internalMesh,
    const vtkFoamIntVectorVector *cellsFaces,
    const vtkFoamIntVectorVector *facesPoints, vtkFloatArray *pointArray,
    vtkIdTypeArray *additionalCells, vtkIntArray *cellList)
{
  const int maxNPoints = 256; // assume max number of points per cell
  vtkIdList* cellPoints = vtkIdList::New();
  cellPoints->SetNumberOfIds(maxNPoints);
  // assume max number of nPoints per face + points per cell
  const int maxNPolyPoints = 1024;
  vtkIdList* polyPoints = vtkIdList::New();
  polyPoints->SetNumberOfIds(maxNPolyPoints);

  const int nCells = (cellList == NULL ? this->NumCells
      : cellList->GetNumberOfTuples());
  int nAdditionalPoints = 0;
  this->NumTotalAdditionalCells = 0;

  // alias
  const vtkFoamIntVectorVector& facePoints = *facesPoints;

  for (int cellI = 0; cellI < nCells; cellI++)
    {
    int cellId;
    if (cellList == NULL)
      {
      cellId = cellI;
      }
    else
      {
      cellId = cellList->GetValue(cellI);
      if (cellId >= this->NumCells)
        {
        vtkWarningMacro(<<"cellLabels id " << cellId
            << " exceeds the number of cells " << nCells
            << ". Inserting an empty cell.");
        internalMesh->InsertNextCell(VTK_EMPTY_CELL, 0,
            cellPoints->GetPointer(0));
        continue;
        }
      }
    const int *cellFaces = cellsFaces->operator[](cellId);
    const int nCellFaces = cellsFaces->GetSize(cellId);

    // determine type of the cell
    // cf. src/OpenFOAM/meshes/meshShapes/cellMatcher/{hex|prism|pyr|tet}-
    // Matcher.C
    int cellType = VTK_CONVEX_POINT_SET;
    if (nCellFaces == 6)
      {
      int j = 0;
      for (; j < nCellFaces; j++)
        {
        if (facePoints.GetSize(cellFaces[j]) != 4)
          {
          break;
          }
        }
      if (j == nCellFaces)
        {
        cellType = VTK_HEXAHEDRON;
        }
      }
    else if (nCellFaces == 5)
      {
      int nTris = 0, nQuads = 0;
      for (int j = 0; j < nCellFaces; j++)
        {
        const int nPoints = facePoints.GetSize(cellFaces[j]);
        if (nPoints == 3)
          {
          nTris++;
          }
        else if (nPoints == 4)
          {
          nQuads++;
          }
        else
          {
          break;
          }
        }
      if (nTris == 2 && nQuads == 3)
        {
        cellType = VTK_WEDGE;
        }
      else if (nTris == 4 && nQuads == 1)
        {
        cellType = VTK_PYRAMID;
        }
      }
    else if (nCellFaces == 4)
      {
      int j = 0;
      for (; j < nCellFaces; j++)
        {
        if (facePoints.GetSize(cellFaces[j]) != 3)
          {
          break;
          }
        }
      if (j == nCellFaces)
        {
        cellType = VTK_TETRA;
        }
      }

    // not a Hex/Wedge/Pyramid/Tetra
    if (cellType == VTK_CONVEX_POINT_SET)
      {
      int nPoints = 0;
      for (int j = 0; j < nCellFaces; j++)
        {
        nPoints += facePoints.GetSize(cellFaces[j]);
        }
      if (nPoints == 0)
        {
        cellType = VTK_EMPTY_CELL;
        }
      }

    // Cell shape constructor based on the one implementd by Terry
    // Jordan, with lots of improvements. Not as elegant as the one in
    // OpenFOAM but it's simple and works reasonably fast.

    // OFhex | vtkHexahedron
    if (cellType == VTK_HEXAHEDRON)
      {
      // get first face in correct order
      const int cellBaseFaceId = cellFaces[0];
      const int *face0Points = facePoints[cellBaseFaceId];

      if (this->FaceOwner->GetValue(cellBaseFaceId) == cellId)
        {
        // if it is an owner face flip the points
        for (int j = 0; j < 4; j++)
          {
          cellPoints->SetId(j, face0Points[3 - j]);
          }
        }
      else
        {
        // add base face to cell points
        for (int j = 0; j < 4; j++)
          {
          cellPoints->SetId(j, face0Points[j]);
          }
        }
      const int baseFacePoint0 = cellPoints->GetId(0);
      const int baseFacePoint2 = cellPoints->GetId(2);
      int cellOppositeFaceI = -1, pivotPoint = -1;
      int dupPoint = -1;
      for (int faceI = 1; faceI < 5; faceI++) // skip face 0 and 5
        {
        const int cellFaceI = cellFaces[faceI];
        const int *faceIPoints = facePoints[cellFaceI];
        int foundDup = -1, pointI = 0;
        for (; pointI < 4; pointI++) // each point
          {
          const int faceIPointI = faceIPoints[pointI];
          // matching two points in base face is enough to find a
          // duplicated point since neighboring faces share two
          // neighboring points (i. e. an edge)
          if (baseFacePoint0 == faceIPointI)
            {
            foundDup = 0;
            break;
            }
          else if (baseFacePoint2 == faceIPointI)
            {
            foundDup = 2;
            break;
            }
          }
        if (foundDup >= 0)
          {
          // find the pivot point if still haven't
          if (pivotPoint == -1)
            {
            dupPoint = foundDup;

            const int faceINextPoint = faceIPoints[(pointI + 1) % 4];

            // if the next point of the faceI-th face matches the
            // previous point of the base face use the previous point
            // of the faceI-th face as the pivot point; or use the
            // next point otherwise
            if (faceINextPoint == (this->FaceOwner->GetValue(cellFaceI)
                == cellId ? cellPoints->GetId(1 + foundDup)
                : cellPoints->GetId(3 - foundDup)))
              {
              pivotPoint = faceIPoints[(3 + pointI) % 4];
              }
            else
              {
              pivotPoint = faceINextPoint;
              }

            if (cellOppositeFaceI >= 0)
              {
              break;
              }
            }
          }
        else
          {
          // if no duplicated point found, faceI is the opposite face
          cellOppositeFaceI = cellFaceI;

          if (pivotPoint >= 0)
            {
            break;
            }
          }
        }

      // if the opposite face is not found until face 4, face 5 is
      // always the opposite face
      if (cellOppositeFaceI == -1)
        {
        cellOppositeFaceI = cellFaces[5];
        }

      // find the pivot point in opposite face
      const int *oppositeFacePoints = facePoints[cellOppositeFaceI];
      int pivotPointI = 0;
      for (; pivotPointI < 4; pivotPointI++)
        {
        if (oppositeFacePoints[pivotPointI] == pivotPoint)
          {
          break;
          }
        }

      // shift the pivot point if the point corresponds to point 2
      // of the base face
      if (dupPoint == 2)
        {
        pivotPointI = (pivotPointI + 2) % 4;
        }
      // copy the face-point list of the opposite face to cell-point list
      int basePointI = 4;
      if (this->FaceOwner->GetValue(cellOppositeFaceI) == cellId)
        {
        for (int pointI = pivotPointI; pointI < 4; pointI++)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        for (int pointI = 0; pointI < pivotPointI; pointI++)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        }
      else
        {
        for (int pointI = pivotPointI; pointI >= 0; pointI--)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        for (int pointI = 3; pointI > pivotPointI; pointI--)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        }

      // create the hex cell and insert it into the mesh
      internalMesh->InsertNextCell(cellType, 8, cellPoints->GetPointer(0));
      }

    // the cell construction is about the same as that of a hex, but
    // the point ordering have to be reversed!!
    else if (cellType == VTK_WEDGE)
      {
      // find the base face number
      int baseFaceId = 0;
      for (int j = 0; j < 5; j++)
        {
        if (facePoints.GetSize(cellFaces[j]) == 3)
          {
          baseFaceId = j;
          break;
          }
        }

      // get first face in correct order
      const int cellBaseFaceId = cellFaces[baseFaceId];
      const int *face0Points = facePoints[cellBaseFaceId];

      if (this->FaceOwner->GetValue(cellBaseFaceId) == cellId)
        {
        for (int j = 0; j < 3; j++)
          {
          cellPoints->SetId(j, face0Points[j]);
          }
        }
      else
        {
        // if it is a neighbor face flip the points
        for (int j = 0; j < 3; j++)
          {
          // add base face to cell points
          cellPoints->SetId(j, face0Points[2 - j]);
          }
        }
      const int baseFacePoint0 = cellPoints->GetId(0);
      const int baseFacePoint2 = cellPoints->GetId(2);
      int cellOppositeFaceI = -1, pivotPoint = -1;
      bool dupPoint2 = false;
      for (int faceI = 0; faceI < 5; faceI++)
        {
        if (faceI == baseFaceId)
          {
          continue;
          }
        const int cellFaceI = cellFaces[faceI];
        if (facePoints.GetSize(cellFaceI) == 3)
          {
          cellOppositeFaceI = cellFaceI;
          }
        // find the pivot point if still haven't
        else if (pivotPoint == -1)
          {
          const int *faceIPoints = facePoints[cellFaceI];
          bool found0Dup = false, found2Dup = false;
          int pointI = 0;
          for (; pointI < 4; pointI++) // each point
            {
            const int faceIPointI = faceIPoints[pointI];
            // matching two points in base face is enough to find a
            // duplicated point since neighboring faces share two
            // neighboring points (i. e. an edge)
            if (baseFacePoint0 == faceIPointI)
              {
              found0Dup = true;
              break;
              }
            else if (baseFacePoint2 == faceIPointI)
              {
              found2Dup = true;
              break;
              }
            }
          // the matching point must always be found so omit the check
          int baseFacePrevPoint, baseFaceNextPoint;
          if (found0Dup)
            {
            baseFacePrevPoint = cellPoints->GetId(2);
            baseFaceNextPoint = cellPoints->GetId(1);
            }
          else
            {
            baseFacePrevPoint = cellPoints->GetId(1);
            baseFaceNextPoint = cellPoints->GetId(0);
            dupPoint2 = true;
            }

          const int faceINextPoint = faceIPoints[(pointI + 1) % 4];
          const int faceIPrevPoint = faceIPoints[(3 + pointI) % 4];

          // if the next point of the faceI-th face matches the
          // previous point of the base face use the previous point of
          // the faceI-th face as the pivot point; or use the next
          // point otherwise
          if (faceINextPoint
              == (this->FaceOwner->GetValue(cellFaceI) == cellId ? baseFacePrevPoint
                  : baseFaceNextPoint))
            {
            pivotPoint = faceIPrevPoint;
            }
          else
            {
            pivotPoint = faceINextPoint;
            }
          }

        // break when both of opposite face and pivot point are found
        if (cellOppositeFaceI >= 0 && pivotPoint >= 0)
          {
          break;
          }
        }

      // find the pivot point in opposite face
      const int *oppositeFacePoints = facePoints[cellOppositeFaceI];
      int pivotPointI = 0;
      for (; pivotPointI < 3; pivotPointI++)
        {
        if (oppositeFacePoints[pivotPointI] == pivotPoint)
          {
          break;
          }
        }

      if (this->FaceOwner->GetValue(cellOppositeFaceI) == cellId)
        {
        if (dupPoint2)
          {
          pivotPointI = (pivotPointI + 2) % 3;
          }
        int basePointI = 3;
        for (int pointI = pivotPointI; pointI >= 0; pointI--)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        for (int pointI = 2; pointI > pivotPointI; pointI--)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        }
      else
        {
        // shift the pivot point if the point corresponds to point 2
        // of the base face
        if (dupPoint2)
          {
          pivotPointI = (1 + pivotPointI) % 3;
          }
        // copy the face-point list of the opposite face to cell-point list
        int basePointI = 3;
        for (int pointI = pivotPointI; pointI < 3; pointI++)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        for (int pointI = 0; pointI < pivotPointI; pointI++)
          {
          cellPoints->SetId(basePointI++, oppositeFacePoints[pointI]);
          }
        }

      // create the wedge cell and insert it into the mesh
      internalMesh->InsertNextCell(cellType, 6, cellPoints->GetPointer(0));
      }

    // OFpyramid | vtkPyramid || OFtet | vtkTetrahedron
    else if (cellType == VTK_PYRAMID || cellType == VTK_TETRA)
      {
      int baseFaceId = -1, nPoints;
      if (cellType == VTK_PYRAMID)
        {
        for (int j = 0; j < nCellFaces; j++)
          {
          if (facePoints.GetSize(cellFaces[j]) == 4)
            {
            baseFaceId = j;
            break;
            }
          }
        nPoints = 5;
        }
      else // VTK_TETRA
        {
        baseFaceId = 0;
        nPoints = 4;
        }

      // add first face to cell points
      const int cellBaseFaceId = cellFaces[baseFaceId];
      const int *baseFacePoints = facePoints[cellBaseFaceId];
      const vtkIdType nBaseFacePoints = facePoints.GetSize(cellBaseFaceId);
      if (this->FaceOwner->GetValue(cellBaseFaceId) == cellId)
        {
        // if it is an owner face flip the points
        for (vtkIdType j = 0; j < nBaseFacePoints; j++)
          {
          cellPoints->SetId(j, baseFacePoints[nBaseFacePoints - 1 - j]);
          }
        }
      else
        {
        for (vtkIdType j = 0; j < nBaseFacePoints; j++)
          {
          cellPoints->SetId(j, baseFacePoints[j]);
          }
        }

      // compare an adjacent face (any non base face is ok) point 1 to
      // base face points
      const int adjacentFaceId = (baseFaceId == 0) ? 1 : baseFaceId - 1;
      const int cellAdjacentFaceId = cellFaces[adjacentFaceId];
      const int *adjacentFacePoints = facePoints[cellAdjacentFaceId];
      const int adjacentFacePoint1 = adjacentFacePoints[1];
      bool foundDup = false;
      for (vtkIdType j = 0; j < nBaseFacePoints; j++)
        {
        // if point 1 of the adjacent face matches point j of the base face...
        if (cellPoints->GetId(j) == adjacentFacePoint1)
          {
          // if point 2 of the adjacent face matches the previous point
          // of the base face use point 0 of the adjacent face as the
          // pivot point; use point 2 otherwise
          cellPoints->SetId(
              nBaseFacePoints,
              (adjacentFacePoints[2]
                  == cellPoints->GetId((this->FaceOwner->GetValue(cellAdjacentFaceId)
                      == cellId ? (j + 1) : (nBaseFacePoints + j - 1))
                      % nBaseFacePoints)) ? adjacentFacePoints[0]
                  : adjacentFacePoints[2]);
          foundDup = true;
          break;
          }
        }
      // if point 1 of the adjacent face does not match any points of
      // the base face, it's the pivot point
      if (!foundDup)
        {
        cellPoints->SetId(nBaseFacePoints, adjacentFacePoint1);
        }

      // create the tetra cell and insert it into the mesh
      internalMesh->InsertNextCell(cellType, nPoints, cellPoints->GetPointer(0));
      }

    // erronous cells
    else if (cellType == VTK_EMPTY_CELL)
      {
      vtkWarningMacro("Warning: No points in cellId " << cellId);
      internalMesh->InsertNextCell(VTK_EMPTY_CELL, 0, cellPoints->GetPointer(0));
      }

    // OFpolyhedron || vtkConvexPointSet
    else
      {
      if (additionalCells != NULL) // decompose into tets and pyramids
        {
        // calculate cell centroid and insert it to point list
        this->AdditionalCellPoints->push_back(vtkIntArray::New());
        vtkIntArray *polyCellPoints = this->AdditionalCellPoints->back();
        float centroid[3];
        centroid[0] = centroid[1] = centroid[2] = 0.0F;
        for (int j = 0; j < nCellFaces; j++)
          {
          // remove duplicate points from faces
          const int cellFacesJ = cellFaces[j];
          const int *faceJPoints = facePoints[cellFacesJ];
          const size_t nFaceJPoints = facePoints.GetSize(cellFacesJ);
          for (size_t k = 0; k < nFaceJPoints; k++)
            {
            const int faceJPointK = faceJPoints[k];
            bool foundDup = false;
            for (vtkIdType l = 0; l < static_cast<vtkIdType>(polyCellPoints->GetDataSize()); l++)
              {
              if (polyCellPoints->GetValue(l) == faceJPointK)
                {
                foundDup = true;
                break; // look no more
                }
              }
            if (!foundDup)
              {
              polyCellPoints->InsertNextValue(faceJPointK);
              float *pointK = pointArray->GetPointer(3 * faceJPointK);
              centroid[0] += pointK[0];
              centroid[1] += pointK[1];
              centroid[2] += pointK[2];
              }
            }
          }
        polyCellPoints->Squeeze();
        const float weight = 1.0F
            / static_cast<float>(polyCellPoints->GetDataSize());
        centroid[0] *= weight;
        centroid[1] *= weight;
        centroid[2] *= weight;
        pointArray->InsertNextTuple(centroid);

        // polyhedron decomposition.
        // a tweaked algorithm based on applications/utilities/postProcessing/
        // graphics/PVFoamReader/vtkFoam/vtkFoamAddInternalMesh.C
        bool insertDecomposedCell = true;
        int nAdditionalCells = 0;
        for (int j = 0; j < nCellFaces; j++)
          {
          const int cellFacesJ = cellFaces[j];
          const int *faceJPoints = facePoints[cellFacesJ];
          const int nFaceJPoints = facePoints.GetSize(cellFacesJ);
          const int flipNeighbor = (this->FaceOwner->GetValue(cellFacesJ)
              == cellId ? -1 : 1);
          const int nTris = nFaceJPoints % 2;

          int vertI = 2;

          // shift the start and end of the vertex loop if the
          // triangle of a decomposed face is going to be flat. Far
          // from perfect but better than nothing to avoid flat cells
          // which stops time integration of Stream Tracer especially
          // for split-hex unstructured meshes created by
          // e. g. autoRefineMesh
          if (nFaceJPoints >= 5 && nTris)
            {
            float *point0, *point1, *point2;
            point0 = pointArray->GetPointer(3 * faceJPoints[nFaceJPoints - 1]);
            point1 = pointArray->GetPointer(3 * faceJPoints[0]);
            point2 = pointArray->GetPointer(3 * faceJPoints[nFaceJPoints - 2]);
            float vsizeSqr1 = 0.0F, vsizeSqr2 = 0.0F, dotProduct = 0.0F;
            for (int i = 0; i < 3; i++)
              {
              const float v1 = point1[i] - point0[i], v2 = point2[i]
                  - point0[i];
              vsizeSqr1 += v1 * v1;
              vsizeSqr2 += v2 * v2;
              dotProduct += v1 * v2;
              }
            // compare in squared representation to avoid using sqrt()
            if (dotProduct * (float) fabs(dotProduct) / (vsizeSqr1 * vsizeSqr2)
                < -1.0F + 1.0e-3F)
              {
              vertI = 1;
              }
            }

          cellPoints->SetId(0, faceJPoints[vertI == 2 ? 0 : nFaceJPoints - 1]);
          cellPoints->SetId(4, this->NumPoints + nAdditionalPoints);

          // decompose a face into quads in order (flipping the
          // decomposed face if owner)
          const int nQuadVerts = nFaceJPoints - 1 - nTris;
          for (; vertI < nQuadVerts; vertI += 2)
            {
            cellPoints->SetId(1, faceJPoints[vertI - flipNeighbor]);
            cellPoints->SetId(2, faceJPoints[vertI]);
            cellPoints->SetId(3, faceJPoints[vertI + flipNeighbor]);

            // if the decomposed cell is the first one insert it to
            // the original position; or append to the decomposed cell
            // list otherwise
            if (insertDecomposedCell)
              {
              internalMesh->InsertNextCell(VTK_PYRAMID, 5,
                  cellPoints->GetPointer(0));
              insertDecomposedCell = false;
              }
            else
              {
              nAdditionalCells++;
              additionalCells->InsertNextTupleValue(cellPoints->GetPointer(0));
              }
            }

          // if the number of vertices is odd there's a triangle
          if (nTris)
            {
            if (flipNeighbor == -1)
              {
              cellPoints->SetId(1, faceJPoints[vertI]);
              cellPoints->SetId(2, faceJPoints[vertI - 1]);
              }
            else
              {
              cellPoints->SetId(1, faceJPoints[vertI - 1]);
              cellPoints->SetId(2, faceJPoints[vertI]);
              }
            cellPoints->SetId(3, this->NumPoints + nAdditionalPoints);

            if (insertDecomposedCell)
              {
              internalMesh->InsertNextCell(VTK_TETRA, 4,
                  cellPoints->GetPointer(0));
              insertDecomposedCell = false;
              }
            else
              {
              // set the 5th vertex number to -1 to distinguish a tetra cell
              cellPoints->SetId(4, -1);
              nAdditionalCells++;
              additionalCells->InsertNextTupleValue(cellPoints->GetPointer(0));
              }
            }
          }
        nAdditionalPoints++;
        this->AdditionalCellIds->InsertNextValue(cellId);
        this->NumAdditionalCells->InsertNextValue(nAdditionalCells);
        this->NumTotalAdditionalCells += nAdditionalCells;
        }
      else // don't decompose; use VTK_POLYHEDRON
        {
        // get first face
        const int cellFaces0 = cellFaces[0];
        const int *baseFacePoints = facePoints[cellFaces0];
        const int nBaseFacePoints = facePoints.GetSize(cellFaces0);
        int nPoints = nBaseFacePoints, nPolyPoints = nBaseFacePoints + 1;
        if (nPoints > maxNPoints || nPolyPoints > maxNPolyPoints)
          {
          vtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
          cellPoints->Delete();
          polyPoints->Delete();
          return;
          }
        polyPoints->SetId(0, nBaseFacePoints);
        if (this->FaceOwner->GetValue(cellFaces0) == cellId)
          {
          // add first face to cell points
          for (int j = 0; j < nBaseFacePoints; j++)
            {
            const int pointJ = baseFacePoints[j];
            cellPoints->SetId(j, pointJ);
            polyPoints->SetId(j + 1, pointJ);
            }
          }
        else
          {
          // if it is a _neighbor_ face flip the points
          for (int j = 0; j < nBaseFacePoints; j++)
            {
            const int pointJ = baseFacePoints[nBaseFacePoints - 1 - j];
            cellPoints->SetId(j, pointJ);
            polyPoints->SetId(j + 1, pointJ);
            }
          }

        // loop through faces and create a list of all points
        // j = 1 skip baseFace
        for (int j = 1; j < nCellFaces; j++)
          {
          // remove duplicate points from faces
          const int cellFacesJ = cellFaces[j];
          const int *faceJPoints = facePoints[cellFacesJ];
          const size_t nFaceJPoints = facePoints.GetSize(cellFacesJ);
          if (nPolyPoints >= maxNPolyPoints)
            {
            vtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
            cellPoints->Delete();
            polyPoints->Delete();
            return;
            }
          polyPoints->SetId(nPolyPoints++, nFaceJPoints);
          int pointI, delta; // must be signed
          if (this->FaceOwner->GetValue(cellFacesJ) == cellId)
            {
            pointI = 0;
            delta = 1;
            }
          else
            {
            // if it is a _neighbor_ face flip the points
            pointI = static_cast<int>(nFaceJPoints) - 1;
            delta = -1;
            }
          for (size_t k = 0; k < nFaceJPoints; k++, pointI += delta)
            {
            const int faceJPointK = faceJPoints[pointI];
            bool foundDup = false;
            for (int l = 0; l < nPoints; l++)
              {
              if (cellPoints->GetId(l) == faceJPointK)
                {
                foundDup = true;
                break; // look no more
                }
              }
            if (!foundDup)
              {
              if (nPoints >= maxNPoints)
                {
                vtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
                cellPoints->Delete();
                polyPoints->Delete();
                return;
                }
              cellPoints->SetId(nPoints++, faceJPointK);
              }
            if (nPolyPoints >= maxNPolyPoints)
                {
                vtkErrorMacro(<< "Too large polyhedron at cellId = " << cellId);
                cellPoints->Delete();
                polyPoints->Delete();
                return;
                }
            polyPoints->SetId(nPolyPoints++, faceJPointK);
            }
          }

        // create the poly cell and insert it into the mesh
        internalMesh->InsertNextCell(VTK_POLYHEDRON, nPoints,
            cellPoints->GetPointer(0), nCellFaces, polyPoints->GetPointer(0));
        }
      }
    }
  cellPoints->Delete();
  polyPoints->Delete();
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::SetBlockName(vtkMultiBlockDataSet *blocks,
    unsigned int blockI, const char *name)
{
  blocks->GetMetaData(blockI)->Set(vtkCompositeDataSet::NAME(), name);
}

//-----------------------------------------------------------------------------
// derive cell types and create the internal mesh
vtkUnstructuredGrid *vtkOpenFOAMReaderPrivate::MakeInternalMesh(
    const vtkFoamIntVectorVector *cellsFaces,
    const vtkFoamIntVectorVector *facesPoints, vtkFloatArray *pointArray)
{
  // Create Mesh
  vtkUnstructuredGrid* internalMesh = vtkUnstructuredGrid::New();
  internalMesh->Allocate(this->NumCells);

  if (this->Parent->GetDecomposePolyhedra())
    {
    // for polyhedral decomposition
    this->AdditionalCellIds = vtkIntArray::New();
    this->NumAdditionalCells = vtkIntArray::New();
    this->AdditionalCellPoints = new vtkFoamIntArrayVector;

    vtkIdTypeArray *additionalCells = vtkIdTypeArray::New();
    additionalCells->SetNumberOfComponents(5); // accommodates tetra or pyramid

    this->InsertCellsToGrid(internalMesh, cellsFaces, facesPoints, pointArray,
        additionalCells, NULL);

    // for polyhedral decomposition
    pointArray->Squeeze();
    this->AdditionalCellIds->Squeeze();
    this->NumAdditionalCells->Squeeze();
    additionalCells->Squeeze();

    // insert decomposed cells into mesh
    const int nComponents = additionalCells->GetNumberOfComponents();
    const int nAdditionalCells = additionalCells->GetNumberOfTuples();
    for (int i = 0; i < nAdditionalCells; i++)
      {
      if (additionalCells->GetComponent(i, 4) == -1)
        {
        internalMesh->InsertNextCell(VTK_TETRA, 4,
            additionalCells->GetPointer(i * nComponents));
        }
      else
        {
        internalMesh->InsertNextCell(VTK_PYRAMID, 5,
            additionalCells->GetPointer(i * nComponents));
        }
      }
    internalMesh->Squeeze();
    additionalCells->Delete();
    }
  else
    {
    this->InsertCellsToGrid(internalMesh, cellsFaces, facesPoints, pointArray,
        NULL, NULL);
    }

  // set the internal mesh points
  vtkPoints *points = vtkPoints::New();
  points->SetData(pointArray);
  internalMesh->SetPoints(points);
  points->Delete();

  return internalMesh;
}

//-----------------------------------------------------------------------------
// insert faces to grid
void vtkOpenFOAMReaderPrivate::InsertFacesToGrid(vtkPolyData *boundaryMesh,
    const vtkFoamIntVectorVector *facesPoints, int startFace, int endFace,
    vtkIntArray *boundaryPointMap, vtkIdList *facePointsVtkId,
    vtkIntArray *labels, bool isLookupValue)
{
  vtkPolyData &bm = *boundaryMesh;

  for (int j = startFace; j < endFace; j++)
    {
    int faceId;
    if (labels == NULL)
      {
      faceId = j;
      }
    else
      {
      faceId = labels->GetValue(j);
      if (faceId >= this->FaceOwner->GetNumberOfTuples())
        {
        vtkWarningMacro(<<"faceLabels id " << faceId
            << " exceeds the number of faces "
            << this->FaceOwner->GetNumberOfTuples());
        bm.InsertNextCell(VTK_EMPTY_CELL, 0, facePointsVtkId->GetPointer(0));
        continue;
        }
      }
    const int *facePoints = facesPoints->operator[](faceId);
    vtkIdType nFacePoints = facesPoints->GetSize(faceId);

    if (isLookupValue)
      {
      for (vtkIdType k = 0; k < nFacePoints; k++)
        {
        facePointsVtkId->SetId(k, boundaryPointMap->LookupValue(facePoints[k]));
        }
      }
    else
      {
      if (boundaryPointMap)
        {
        for (vtkIdType k = 0; k < nFacePoints; k++)
          {
          facePointsVtkId->SetId(k, boundaryPointMap->GetValue(facePoints[k]));
          }
        }
      else
        {
        for (vtkIdType k = 0; k < nFacePoints; k++)
          {
          facePointsVtkId->SetId(k, facePoints[k]);
          }
        }
      }

    // triangle
    if (nFacePoints == 3)
      {
      bm.InsertNextCell(VTK_TRIANGLE, 3, facePointsVtkId->GetPointer(0));
      }
    // quad
    else if (nFacePoints == 4)
      {
      bm.InsertNextCell(VTK_QUAD, 4, facePointsVtkId->GetPointer(0));
      }
    // polygon
    else
      {
      bm.InsertNextCell(VTK_POLYGON, nFacePoints,
          facePointsVtkId->GetPointer(0));
      }
    }
}

//-----------------------------------------------------------------------------
// returns requested boundary meshes
vtkMultiBlockDataSet *vtkOpenFOAMReaderPrivate::MakeBoundaryMesh(
    const vtkFoamIntVectorVector *facesPoints, vtkFloatArray* pointArray)
{
  const vtkIdType nBoundaries = static_cast<vtkIdType>(this->BoundaryDict.size());

  // do a consistency check of BoundaryDict
  int previousEndFace = -1;
  for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
    {
    const vtkFoamBoundaryEntry &beI = this->BoundaryDict[boundaryI];
    const int startFace = beI.StartFace;
    const int nFaces = beI.NFaces;
    if (startFace < 0 || nFaces < 0)
      {
      vtkErrorMacro(<<"Neither of startFace " << startFace << " nor nFaces "
          << nFaces << " can be negative for patch " << beI.BoundaryName.c_str());
      return NULL;
      }
    if (previousEndFace >= 0 && previousEndFace != startFace)
      {
      vtkErrorMacro(<<"The end face number " << previousEndFace - 1
          << " of patch "
          << this->BoundaryDict[boundaryI - 1].BoundaryName.c_str()
          << " is not consistent with the start face number " << startFace
          << " of patch " << beI.BoundaryName.c_str());
      return NULL;
      }
    previousEndFace = startFace + nFaces;
    }
  if (previousEndFace > facesPoints->GetNumberOfElements())
    {
    vtkErrorMacro(<<"The end face number " << previousEndFace - 1
        << " of the last patch "
        << this->BoundaryDict[nBoundaries - 1].BoundaryName.c_str()
        << " exceeds the number of faces " << facesPoints->GetNumberOfElements());
    return NULL;
    }

  vtkMultiBlockDataSet *boundaryMesh = vtkMultiBlockDataSet::New();

  if (this->Parent->GetCreateCellToPoint())
    {
    const int boundaryStartFace =
        (this->BoundaryDict.size() > 0 ? this->BoundaryDict[0].StartFace : 0);
    this->AllBoundaries = vtkPolyData::New();
    this->AllBoundaries->Allocate(facesPoints->GetNumberOfElements()
        - boundaryStartFace);
    }
  this->BoundaryPointMap = new vtkFoamIntArrayVector;

  vtkIntArray *nBoundaryPointsList = vtkIntArray::New();
  nBoundaryPointsList->SetNumberOfValues(nBoundaries);

  // count the max number of points per face and the number of points
  // (with duplicates) in mesh
  int maxNFacePoints = 0;
  for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
    {
    const int startFace = this->BoundaryDict[boundaryI].StartFace;
    const int endFace = startFace + this->BoundaryDict[boundaryI].NFaces;
    int nPoints = 0;
    for (int j = startFace; j < endFace; j++)
      {
      const int nFacePoints = facesPoints->GetSize(j);
      nPoints += nFacePoints;
      if (nFacePoints > maxNFacePoints)
        {
        maxNFacePoints = nFacePoints;
        }
      }
    nBoundaryPointsList->SetValue(boundaryI, nPoints);
    }

  // aloocate array for converting int vector to vtkIdType List:
  // workaround for 64bit machines
  vtkIdList *facePointsVtkId = vtkIdList::New();
  facePointsVtkId->SetNumberOfIds(maxNFacePoints);

  // create initial internal point list: set all points to -1
  if (this->Parent->GetCreateCellToPoint())
    {
    this->InternalPoints = vtkIntArray::New();
    this->InternalPoints->SetNumberOfValues(this->NumPoints);
    for (int pointI = 0; pointI < this->NumPoints; pointI++)
      {
      this->InternalPoints->SetValue(pointI, -1);
      }

    // mark boundary points as 0
    for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
      {
      const vtkFoamBoundaryEntry &beI = this->BoundaryDict[boundaryI];
      if (beI.BoundaryType == vtkFoamBoundaryEntry::PHYSICAL
          || beI.BoundaryType == vtkFoamBoundaryEntry::PROCESSOR)
        {
        const int startFace = beI.StartFace;
        const int endFace = startFace + beI.NFaces;

        for (int j = startFace; j < endFace; j++)
          {
          const int *facePoints = facesPoints->operator[](j);
          const int nFacePoints = facesPoints->GetSize(j);
          for (int k = 0; k < nFacePoints; k++)
            {
            this->InternalPoints->SetValue(facePoints[k], 0);
            }
          }
        }
      }
    }

  int nAllBoundaryPoints = 0;
  vtkstd::vector<vtkstd::vector<int> > procCellList;
  vtkIntArray *pointTypes = NULL;

  if (this->Parent->GetCreateCellToPoint())
    {
    // create global to AllBounaries point map
    for (int pointI = 0; pointI < this->NumPoints; pointI++)
      {
      if (this->InternalPoints->GetValue(pointI) == 0)
        {
        this->InternalPoints->SetValue(pointI, nAllBoundaryPoints);
        nAllBoundaryPoints++;
        }
      }

    if (this->ProcessorName != "")
      {
      // initialize physical-processor boundary shared point list
      procCellList.resize(nAllBoundaryPoints);
      pointTypes = vtkIntArray::New();
      pointTypes->SetNumberOfTuples(nAllBoundaryPoints);
      for (int pointI = 0; pointI < nAllBoundaryPoints; pointI++)
        {
        pointTypes->SetValue(pointI, 0);
        }
      }
    }

  for (int boundaryI = 0; boundaryI < nBoundaries; boundaryI++)
    {
    const vtkFoamBoundaryEntry &beI = this->BoundaryDict[boundaryI];
    const int nFaces = beI.NFaces;
    const int startFace = beI.StartFace;
    const int endFace = startFace + nFaces;

    if (this->Parent->GetCreateCellToPoint() && (beI.BoundaryType
        == vtkFoamBoundaryEntry::PHYSICAL || beI.BoundaryType
        == vtkFoamBoundaryEntry::PROCESSOR ))
      {
      // add faces to AllBoundaries
      this->InsertFacesToGrid(this->AllBoundaries, facesPoints, startFace,
          endFace, this->InternalPoints, facePointsVtkId, NULL, false);

      if (this->ProcessorName != "")
        {
        // mark belonging boundary types and, if PROCESSOR, cell numbers
        const int abStartFace = beI.AllBoundariesStartFace;
        const int abEndFace = abStartFace + beI.NFaces;
        for (int faceI = abStartFace; faceI < abEndFace; faceI++)
          {
          vtkIdType nPoints;
          vtkIdType *points;
          this->AllBoundaries->GetCellPoints(faceI, nPoints, points);
          if (beI.BoundaryType == vtkFoamBoundaryEntry::PHYSICAL)
            {
            for (int pointI = 0; pointI < nPoints; pointI++)
              {
              *pointTypes->GetPointer(points[pointI])
                  |= vtkFoamBoundaryEntry::PHYSICAL;
              }
            }
          else // PROCESSOR
            {
            for (int pointI = 0; pointI < nPoints; pointI++)
              {
              const int pointJ = points[pointI];
              *pointTypes->GetPointer(pointJ)
                  |= vtkFoamBoundaryEntry::PROCESSOR;
              procCellList[pointJ].push_back(faceI);
              }
            }
          }
        }
      }

    // skip below if inactive
    if (!beI.IsActive)
      {
      continue;
      }

    // create the mesh
    const unsigned int activeBoundaryI = boundaryMesh->GetNumberOfBlocks();
    vtkPolyData *bm = vtkPolyData::New();
    boundaryMesh->SetBlock(activeBoundaryI, bm);

    // set the name of boundary
    this->SetBlockName(boundaryMesh, activeBoundaryI, beI.BoundaryName.c_str());

    bm->Allocate(nFaces);
    const int nBoundaryPoints = nBoundaryPointsList->GetValue(boundaryI);

    // create global to boundary-local point map and boundary points
    vtkIntArray *boundaryPointList = vtkIntArray::New();
    boundaryPointList->SetNumberOfValues(nBoundaryPoints);
    int pointI = 0;
    for (int j = startFace; j < endFace; j++)
      {
      const int *facePoints = facesPoints->operator[](j);
      int nFacePoints = facesPoints->GetSize(j);
      for (int k = 0; k < nFacePoints; k++)
        {
        boundaryPointList->SetValue(pointI, facePoints[k]);
        pointI++;
        }
      }
    vtkSortDataArray::Sort(boundaryPointList);
    this->BoundaryPointMap->push_back(vtkIntArray::New());
    vtkIntArray& bpMap = *this->BoundaryPointMap->back();
    vtkFloatArray *boundaryPointArray = vtkFloatArray::New();
    boundaryPointArray->SetNumberOfComponents(3);
    int oldPointJ = -1;
    for (int j = 0; j < nBoundaryPoints; j++)
      {
      const int pointJ = boundaryPointList->GetValue(j);
      if (pointJ != oldPointJ)
        {
        oldPointJ = pointJ;
        boundaryPointArray->InsertNextTuple(pointArray->GetPointer(3 * pointJ));
        bpMap.InsertNextValue(pointJ);
        }
      }
    boundaryPointArray->Squeeze();
    bpMap.Squeeze();
    boundaryPointList->Delete();
    vtkPoints *boundaryPoints = vtkPoints::New();
    boundaryPoints->SetData(boundaryPointArray);
    boundaryPointArray->Delete();

    // set points for boundary
    bm->SetPoints(boundaryPoints);
    boundaryPoints->Delete();

    // insert faces to boundary mesh
    this->InsertFacesToGrid(bm, facesPoints, startFace, endFace, &bpMap,
        facePointsVtkId, NULL, true);
    bm->Delete();
    bpMap.ClearLookup();
    }

  nBoundaryPointsList->Delete();
  facePointsVtkId->Delete();

  if (this->Parent->GetCreateCellToPoint())
    {
    this->AllBoundaries->Squeeze();
    this->AllBoundariesPointMap = vtkIntArray::New();
    vtkIntArray &abpMap = *this->AllBoundariesPointMap;
    abpMap.SetNumberOfValues(nAllBoundaryPoints);

    // create lists of internal points and AllBoundaries points
    int nInternalPoints = 0;
    for (int pointI = 0, allBoundaryPointI = 0; pointI < this->NumPoints; pointI++)
      {
      const int globalPointId = this->InternalPoints->GetValue(pointI);
      if (globalPointId == -1)
        {
        this->InternalPoints->SetValue(nInternalPoints, pointI);
        nInternalPoints++;
        }
      else
        {
        abpMap.SetValue(allBoundaryPointI, pointI);
        allBoundaryPointI++;
        }
      }
    // shrink to the number of internal points
    if (nInternalPoints > 0)
      {
      this->InternalPoints->Resize(nInternalPoints);
      }
    else
      {
      this->InternalPoints->Delete();
      this->InternalPoints = NULL;
      }

    // set dummy vtkPoints to tell the grid the number of points
    // (otherwise GetPointCells will crash)
    vtkPoints *allBoundaryPoints = vtkPoints::New();
    allBoundaryPoints->SetNumberOfPoints(abpMap.GetNumberOfTuples());
    this->AllBoundaries->SetPoints(allBoundaryPoints);
    allBoundaryPoints->Delete();

    if (this->ProcessorName != "")
      {
      // remove links to processor boundary faces from point-to-cell
      // links of physical-processor shared points to avoid cracky seams
      // on fixedValue-type boundaries which are noticeable when all the
      // decomposed meshes are appended
      this->AllBoundaries->BuildLinks();
      for (int pointI = 0; pointI < nAllBoundaryPoints; pointI++)
        {
        if (pointTypes->GetValue(pointI) == (vtkFoamBoundaryEntry::PHYSICAL
            | vtkFoamBoundaryEntry::PROCESSOR))
          {
          const vtkstd::vector<int> &procCells = procCellList[pointI];
          for (size_t cellI = 0; cellI < procCellList[pointI].size(); cellI++)
            {
            this->AllBoundaries->RemoveReferenceToCell(pointI, procCells[cellI]);
            }
          // omit reclaiming memory as the possibly recovered size should
          // not typically be so large
          }
        }
      pointTypes->Delete();
      }
    }

  return boundaryMesh;
}

//-----------------------------------------------------------------------------
// truncate face owner to have only boundary face info
void vtkOpenFOAMReaderPrivate::TruncateFaceOwner()
{
  const int boundaryStartFace =
      this->BoundaryDict.size() > 0 ? this->BoundaryDict[0].StartFace
          : this->FaceOwner->GetNumberOfTuples();
  // all the boundary faces
  const int nBoundaryFaces = this->FaceOwner->GetNumberOfTuples()
      - boundaryStartFace;
  memmove(this->FaceOwner->GetPointer(0),
      this->FaceOwner->GetPointer(boundaryStartFace), sizeof(int)
          * nBoundaryFaces);
  this->FaceOwner->Resize(nBoundaryFaces);
}

//-----------------------------------------------------------------------------
// this is necessary due to the strange vtkDataArrayTemplate::Resize()
// implementation when the array size is to be extended
template <typename T1, typename T2> bool vtkOpenFOAMReaderPrivate::ExtendArray(
    T1 *array, const int nTuples)
{
  const int newSize = nTuples * array->GetNumberOfComponents();
  void *ptr = malloc(newSize * array->GetDataTypeSize());
  if (ptr == NULL)
    {
    return false;
    }
  memmove(ptr, array->GetVoidPointer(0), array->GetDataSize()
      * array->GetDataTypeSize());
  array->SetArray(static_cast<T2 *>(ptr), newSize, 0);
  return true;
}

//-----------------------------------------------------------------------------
// move polyhedral cell centroids
vtkPoints *vtkOpenFOAMReaderPrivate::MoveInternalMesh(
    vtkUnstructuredGrid *internalMesh, vtkFloatArray *pointArray)
{
  if (this->Parent->GetDecomposePolyhedra())
    {
    const vtkIdType nAdditionalCells = static_cast<vtkIdType>(this->AdditionalCellPoints->size());
    this->ExtendArray<vtkFloatArray, float>(pointArray, this->NumPoints
        + nAdditionalCells);
    for (int i = 0; i < nAdditionalCells; i++)
      {
      vtkIntArray *polyCellPoints = this->AdditionalCellPoints->operator[](i);
      float centroid[3];
      centroid[0] = centroid[1] = centroid[2] = 0.0F;
      const int nCellPoints = polyCellPoints->GetDataSize();
      for (int j = 0; j < nCellPoints; j++)
        {
        float *pointK = pointArray->GetPointer(3 * polyCellPoints->GetValue(j));
        centroid[0] += pointK[0];
        centroid[1] += pointK[1];
        centroid[2] += pointK[2];
        }
      const float weight = (nCellPoints ? 1.0F
          / static_cast<float>(nCellPoints) : 0.0F);
      centroid[0] *= weight;
      centroid[1] *= weight;
      centroid[2] *= weight;
      pointArray->InsertTuple(this->NumPoints + i, centroid);
      }
    }
  if (internalMesh->GetPoints()->GetNumberOfPoints() != pointArray->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "The numbers of points for old points "
        << internalMesh->GetPoints()->GetNumberOfPoints() << " and new points"
        << pointArray->GetNumberOfTuples() << " don't match");
    return NULL;
    }

  // instantiate the points class
  vtkPoints *points = vtkPoints::New();
  points->SetData(pointArray);
  internalMesh->SetPoints(points);
  return points;
}

//-----------------------------------------------------------------------------
// move boundary points
void vtkOpenFOAMReaderPrivate::MoveBoundaryMesh(
    vtkMultiBlockDataSet *boundaryMesh, vtkFloatArray *pointArray)
{
  for (vtkIdType boundaryI = 0, activeBoundaryI = 0; boundaryI
    < static_cast<vtkIdType>(this->BoundaryDict.size()); boundaryI++)
    {
    if (this->BoundaryDict[boundaryI].IsActive)
      {
      vtkIntArray *bpMap = this->BoundaryPointMap->operator[](activeBoundaryI);
      const int nBoundaryPoints = bpMap->GetNumberOfTuples();
      vtkFloatArray *boundaryPointArray = vtkFloatArray::New();
      boundaryPointArray->SetNumberOfComponents(3);
      boundaryPointArray->SetNumberOfTuples(nBoundaryPoints);
      for (int pointI = 0; pointI < nBoundaryPoints; pointI++)
        {
        boundaryPointArray->SetTuple(pointI, bpMap->GetValue(pointI),
            pointArray);
        }
      vtkPoints *boundaryPoints = vtkPoints::New();
      boundaryPoints->SetData(boundaryPointArray);
      boundaryPointArray->Delete();
      vtkPolyData::SafeDownCast(boundaryMesh->GetBlock(activeBoundaryI))
      ->SetPoints(boundaryPoints);
      boundaryPoints->Delete();
      activeBoundaryI++;
      }
    }
}

//-----------------------------------------------------------------------------
// as of now the function does not do interpolation, but do just averaging.
void vtkOpenFOAMReaderPrivate::InterpolateCellToPoint(vtkFloatArray *pData,
    vtkFloatArray *iData, vtkPointSet *mesh, vtkIntArray *pointList,
    const int nPoints)
{
  if (nPoints == 0)
    {
    return;
    }

  // a dummy call to let GetPointCells() build the cell links if still not built
  // (not using BuildLinks() since it always rebuild links)
  vtkIdList *pointCells = vtkIdList::New();
  mesh->GetPointCells(0, pointCells);
  pointCells->Delete();

  // since vtkPolyData and vtkUnstructuredGrid do not share common
  // overloaded GetCellLink() or GetPointCells() functions we have to
  // do a tedious task
  vtkUnstructuredGrid *ug = vtkUnstructuredGrid::SafeDownCast(mesh);
  vtkPolyData *pd = vtkPolyData::SafeDownCast(mesh);
  vtkCellLinks *cl = NULL;
  if (ug)
    {
    cl = ug->GetCellLinks();
    }

  const int nComponents = iData->GetNumberOfComponents();

  if (nComponents == 1)
    {
    // a special case with the innermost componentI loop unrolled
    float *tuples = iData->GetPointer(0);
    for (int pointI = 0; pointI < nPoints; pointI++)
      {
      const int pI = (pointList ? pointList->GetValue(pointI) : pointI);
      unsigned short nCells;
      vtkIdType *cells;
      if (cl)
        {
        const vtkCellLinks::Link &l = cl->GetLink(pI);
        nCells = l.ncells;
        cells = l.cells;
        }
      else
        {
        pd->GetPointCells(pI, nCells, cells);
        }
      // use double intermediate variable for precision
      double interpolatedValue = 0.0;
      for (int cellI = 0; cellI < nCells; cellI++)
        {
        interpolatedValue += tuples[cells[cellI]];
        }
      interpolatedValue = (nCells ? interpolatedValue
          / static_cast<double>(nCells) : 0.0);
      pData->SetValue(pI, interpolatedValue);
      }
    }
  else if (nComponents == 3)
    {
    // a special case with the innermost componentI loop unrolled
    float *pDataPtr = pData->GetPointer(0);
    for (int pointI = 0; pointI < nPoints; pointI++)
      {
      const int pI = (pointList ? pointList->GetValue(pointI) : pointI);
      unsigned short nCells;
      vtkIdType *cells;
      if (cl)
        {
        const vtkCellLinks::Link &l = cl->GetLink(pI);
        nCells = l.ncells;
        cells = l.cells;
        }
      else
        {
        pd->GetPointCells(pI, nCells, cells);
        }
      // use double intermediate variables for precision
      const double weight = (nCells ? 1.0 / static_cast<double>(nCells) : 0.0);
      double summedValue0 = 0.0, summedValue1 = 0.0, summedValue2 = 0.0;

      // hand unrolling
      for (int cellI = 0; cellI < nCells; cellI++)
        {
        const float *tuple = iData->GetPointer(3 * cells[cellI]);
        summedValue0 += tuple[0];
        summedValue1 += tuple[1];
        summedValue2 += tuple[2];
        }

      float *interpolatedValue = &pDataPtr[3 * pI];
      interpolatedValue[0] = weight * summedValue0;
      interpolatedValue[1] = weight * summedValue1;
      interpolatedValue[2] = weight * summedValue2;
      }
    }
  else
    {
    float *pDataPtr = pData->GetPointer(0);
    for (int pointI = 0; pointI < nPoints; pointI++)
      {
      const int pI = (pointList ? pointList->GetValue(pointI) : pointI);
      unsigned short nCells;
      vtkIdType *cells;
      if (cl)
        {
        const vtkCellLinks::Link &l = cl->GetLink(pI);
        nCells = l.ncells;
        cells = l.cells;
        }
      else
        {
        pd->GetPointCells(pI, nCells, cells);
        }
      // use double intermediate variables for precision
      const double weight = (nCells ? 1.0 / static_cast<double>(nCells) : 0.0);
      float *interpolatedValue = &pDataPtr[nComponents * pI];
      // a bit strange loop order but this works fastest
      for (int componentI = 0; componentI < nComponents; componentI++)
        {
        const float *tuple = iData->GetPointer(componentI);
        double summedValue = 0.0;
        for (int cellI = 0; cellI < nCells; cellI++)
          {
          summedValue += tuple[nComponents * cells[cellI]];
          }
        interpolatedValue[componentI] = weight * summedValue;
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkOpenFOAMReaderPrivate::ReadFieldFile(vtkFoamIOobject *ioPtr,
    vtkFoamDict *dictPtr, const vtkStdString &varName,
    vtkDataArraySelection *selection)
{
  const vtkStdString varPath(this->CurrentTimeRegionPath() + "/" + varName);

  // open the file
  vtkFoamIOobject &io = *ioPtr;
  if (!io.Open(varPath))
    {
    vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
        << io.GetError().c_str());
    return false;
    }

  // if the variable is disabled on selection panel then skip it
  if (selection->ArrayExists(io.GetObjectName().c_str()) && !selection->ArrayIsEnabled(io.GetObjectName().c_str()))
    {
    return false;
    }

  // read the field file into dictionary
  vtkFoamDict &dict = *dictPtr;
  if (!dict.Read(io))
    {
    vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
        << " of " << io.GetFileName().c_str() << ": " << io.GetError().c_str());
    return false;
    }

  if (dict.GetType() != vtkFoamToken::DICTIONARY)
    {
    vtkErrorMacro(<<"File " << io.GetFileName().c_str()
        << "is not valid as a field file");
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
vtkFloatArray *vtkOpenFOAMReaderPrivate::FillField(vtkFoamEntry *entryPtr,
    int nElements, vtkFoamIOobject *ioPtr, const vtkStdString &fieldType)
{
  vtkFloatArray *data;
  vtkFoamEntry &entry = *entryPtr;
  const vtkStdString &className = ioPtr->GetClassName();

  // "uniformValue" keyword is for uniformFixedValue B.C.
  if (entry.FirstValue().GetIsUniform() || entry.GetKeyword() == "uniformValue")
    {
    if (entry.FirstValue().GetType() == vtkFoamToken::SCALAR || entry.FirstValue().GetType() == vtkFoamToken::LABEL)
      {
      const float num = entry.ToFloat();
      data = vtkFloatArray::New();
      data->SetNumberOfValues(nElements);
      for (int i = 0; i < nElements; i++)
        {
        data->SetValue(i, num);
        }
      }
    else
      {
      float tupleBuffer[9], *tuple;
      int nComponents;
      // have to determine the type of vector
      if (entry.FirstValue().GetType() == vtkFoamToken::LABELLIST)
        {
        vtkIntArray &ll = entry.LabelList();
        nComponents = ll.GetNumberOfTuples();
        for (int componentI = 0; componentI < nComponents; componentI++)
          {
          tupleBuffer[componentI] = static_cast<float>(ll.GetValue(componentI));
          }
        tuple = tupleBuffer;
        }
      else if (entry.FirstValue().GetType() == vtkFoamToken::SCALARLIST)
        {
        vtkFloatArray& sl = entry.ScalarList();
        nComponents = sl.GetSize();
        tuple = sl.GetPointer(0);
        }
      else
        {
        vtkErrorMacro(<<"Wrong list type for uniform field");
        return NULL;
        }

      if ((fieldType == "SphericalTensorField" && nComponents == 1)
          || (fieldType == "VectorField" && nComponents == 3) || (fieldType
          == "SymmTensorField" && nComponents == 6) || (fieldType
          == "TensorField" && nComponents == 9))
        {
        data = vtkFloatArray::New();
        data->SetNumberOfComponents(nComponents);
        data->SetNumberOfTuples(nElements);
#if vtksys_DATE_STAMP_FULL >= 20080620
        // swap the components of symmTensor to match the component
        // names in paraview
        if(nComponents == 6)
          {
          const float symxy = tuple[1], symxz = tuple[2], symyy = tuple[3];
          const float symyz = tuple[4], symzz = tuple[5];
          tuple[1] = symyy;
          tuple[2] = symzz;
          tuple[3] = symxy;
          tuple[4] = symyz;
          tuple[5] = symxz;
          }
#endif
        for (int i = 0; i < nElements; i++)
          {
          data->SetTuple(i, tuple);
          }
        }
      else
        {
        vtkErrorMacro(<< "Number of components and field class doesn't match "
                      << "for " << ioPtr->GetFileName().c_str() << ". class = " << className.c_str()
                      << ", nComponents = " << nComponents);
        return NULL;
        }
      }
    }
  else // nonuniform
    {
    if ((fieldType == "ScalarField" && entry.FirstValue().GetType() == vtkFoamToken::SCALARLIST) || ((fieldType
        == "VectorField" || fieldType == "SphericalTensorField" || fieldType
        == "SymmTensorField" || fieldType == "TensorField")
        && entry.FirstValue().GetType() == vtkFoamToken::VECTORLIST))
      {
      const int nTuples = entry.ScalarList().GetNumberOfTuples();
      if (nTuples != nElements)
        {
        vtkErrorMacro(<<"Number of cells/points in mesh and field don't match: "
            << "mesh = " << nElements << ", field = " << nTuples);
        return NULL;
        }
      data = static_cast<vtkFloatArray *>(entry.Ptr());
#if vtksys_DATE_STAMP_FULL >= 20080620
      // swap the components of symmTensor to match the component
      // names in paraview
      const int nComponents = data->GetNumberOfComponents();
      if(nComponents == 6)
        {
        for(int tupleI = 0; tupleI < nTuples; tupleI++)
          {
          float *tuple = data->GetPointer(nComponents * tupleI);
          const float symxy = tuple[1], symxz = tuple[2], symyy = tuple[3];
          const float symyz = tuple[4], symzz = tuple[5];
          tuple[1] = symyy;
          tuple[2] = symzz;
          tuple[3] = symxy;
          tuple[4] = symyz;
          tuple[5] = symxz;
          }
        }
#endif
      }
    else if (entry.FirstValue().GetType() == vtkFoamToken::EMPTYLIST && nElements <= 0)
      {
      data = vtkFloatArray::New();
      // set the number of components as appropriate if the list is empty
      if (fieldType == "ScalarField" || fieldType == "SphericalTensorField")
        {
        data->SetNumberOfComponents(1);
        }
      else if (fieldType == "VectorField")
        {
        data->SetNumberOfComponents(3);
        }
      else if (fieldType == "SymmTensorField")
        {
        data->SetNumberOfComponents(6);
        }
      else if (fieldType == "TensorField")
        {
        data->SetNumberOfComponents(9);
        }
      }
    else
      {
      vtkErrorMacro(<< ioPtr->GetFileName().c_str() << " is not a valid "
          << ioPtr->GetClassName().c_str());
      return NULL;
      }
    }
  return data;
}

//-----------------------------------------------------------------------------
// convert OpenFOAM's dimension array representation to string
void vtkOpenFOAMReaderPrivate::ConstructDimensions(vtkStdString *dimString,
    vtkFoamDict *dictPtr)
{
  if (!this->Parent->GetAddDimensionsToArrayNames())
    {
    return;
    }
  vtkFoamEntry *dimEntry = dictPtr->Lookup("dimensions");
  if (dimEntry != NULL && dimEntry->FirstValue().GetType() == vtkFoamToken::LABELLIST)
    {
    vtkIntArray &dims = dimEntry->LabelList();
    if (dims.GetNumberOfTuples() == 7)
      {
      int dimSet[7];
      for (int dimI = 0; dimI < 7; dimI++)
        {
        dimSet[dimI] = dims.GetValue(dimI);
        }
      static const char *units[7] =
      { "kg", "m", "s", "K", "mol", "A", "cd" };
      vtksys_ios::ostringstream posDim, negDim;
      int posSpc = 0, negSpc = 0;
      if (dimSet[0] == 1 && dimSet[1] == -1 && dimSet[2] == -2)
        {
        posDim << "Pa";
        dimSet[0] = dimSet[1] = dimSet[2] = 0;
        posSpc = 1;
        }
      for (int dimI = 0; dimI < 7; dimI++)
        {
        const int dimDim = dimSet[dimI];
        if (dimDim > 0)
          {
          if (posSpc)
            {
            posDim << " ";
            }
          posDim << units[dimI];
          if (dimDim > 1)
            {
            posDim << dimDim;
            }
          posSpc++;
          }
        else if (dimDim < 0)
          {
          if (negSpc)
            {
            negDim << " ";
            }
          negDim << units[dimI];
          if (dimDim < -1)
            {
            negDim << -dimDim;
            }
          negSpc++;
          }
        }
      *dimString += " [" + posDim.str();
      if (negSpc > 0)
        {
        if (posSpc == 0)
          {
          *dimString += "1";
          }
        if (negSpc > 1)
          {
          *dimString += "/(" + negDim.str() + ")";
          }
        else
          {
          *dimString += "/" + negDim.str();
          }
        }
      else if (posSpc == 0)
        {
        *dimString += "-";
        }
      *dimString += "]";
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::GetVolFieldAtTimeStep(
    vtkUnstructuredGrid *internalMesh, vtkMultiBlockDataSet *boundaryMesh,
    const vtkStdString &varName)
{
  vtkFoamIOobject io(this->CasePath);
  vtkFoamDict dict;
  if (!this->ReadFieldFile(&io, &dict, varName,
      this->Parent->CellDataArraySelection))
    {
    return;
    }

  if (io.GetClassName().substr(0, 3) != "vol")
    {
    vtkErrorMacro(<< io.GetFileName().c_str() << " is not a volField");
    return;
    }

  vtkFoamEntry *iEntry = dict.Lookup("internalField");
  if (iEntry == NULL)
    {
    vtkErrorMacro(<<"internalField not found in " << io.GetFileName().c_str());
    return;
    }

  if (iEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
    {
    // if there's no cell there shouldn't be any boundary faces either
    if (this->NumCells > 0)
      {
      vtkErrorMacro(<<"internalField of " << io.GetFileName().c_str()
          << " is empty");
      }
    return;
    }

  vtkStdString fieldType = io.GetClassName().substr(3, vtkStdString::npos);
  vtkFloatArray *iData =
      this->FillField(iEntry, this->NumCells, &io, fieldType);
  if (iData == NULL)
    {
    return;
    }

  vtkStdString dimString;
  this->ConstructDimensions(&dimString, &dict);

  vtkFloatArray *acData = NULL, *ctpData = NULL;

  if (this->Parent->GetCreateCellToPoint())
    {
    acData = vtkFloatArray::New();
    acData->SetNumberOfComponents(iData->GetNumberOfComponents());
    acData->SetNumberOfTuples(this->AllBoundaries->GetNumberOfCells());
    }

  if (iData->GetSize() > 0)
    {
    // Add field only if internal Mesh exists (skip if not selected).
    // Note we still need to read internalField even if internal mesh is
    // not selected, since boundaries without value entries may refer to
    // the internalField.
    if (internalMesh != NULL)
      {
      if (this->Parent->GetDecomposePolyhedra())
        {
        // add values for decomposed cells
        this->ExtendArray<vtkFloatArray, float>(iData, this->NumCells
            + this->NumTotalAdditionalCells);
        const int nTuples = this->AdditionalCellIds->GetNumberOfTuples();
        int additionalCellI = this->NumCells;
        for (int tupleI = 0; tupleI < nTuples; tupleI++)
          {
          const int nCells = this->NumAdditionalCells->GetValue(tupleI);
          const vtkIdType cellId = this->AdditionalCellIds->GetValue(tupleI);
          for (int cellI = 0; cellI < nCells; cellI++)
            {
            iData->InsertTuple(additionalCellI++, cellId, iData);
            }
          }
        }

      // set data to internal mesh
      this->AddArrayToFieldData(internalMesh->GetCellData(), iData,
          io.GetObjectName() + dimString);

      if (this->Parent->GetCreateCellToPoint())
        {
        // Create cell-to-point interpolated data
        ctpData = vtkFloatArray::New();
        ctpData->SetNumberOfComponents(iData->GetNumberOfComponents());
        ctpData->SetNumberOfTuples(internalMesh->GetPoints()->GetNumberOfPoints());
        if (this->InternalPoints != NULL)
          {
          this->InterpolateCellToPoint(ctpData, iData, internalMesh,
              this->InternalPoints, this->InternalPoints->GetNumberOfTuples());
          }

        if (this->Parent->GetDecomposePolyhedra())
          {
          // assign cell values to additional points
          const int nPoints = this->AdditionalCellIds->GetNumberOfTuples();
          for (int pointI = 0; pointI < nPoints; pointI++)
            {
            ctpData->SetTuple(this->NumPoints + pointI,
                this->AdditionalCellIds->GetValue(pointI), iData);
            }
          }
        }
      }
    }
  else
    {
    // determine as there's no cells
    iData->Delete();
    if (acData != NULL)
      {
      acData->Delete();
      }
    return;
    }

  // set boundary values
  const vtkFoamEntry *bEntry = dict.Lookup("boundaryField");
  if (bEntry == NULL)
    {
    vtkErrorMacro(<< "boundaryField not found in object " << varName.c_str()
        << " at time = " << this->TimeNames->GetValue(this->TimeStep).c_str());
    iData->Delete();
    if (acData != NULL)
      {
      acData->Delete();
      }
    if (ctpData != NULL)
      {
      ctpData->Delete();
      }
    return;
    }

  vtkstd::vector<vtkFloatArray *> vDataVector;
  for (int boundaryI = 0, activeBoundaryI = 0; boundaryI
    < static_cast<int>(this->BoundaryDict.size()); boundaryI++)
    {
    const vtkFoamBoundaryEntry &beI = this->BoundaryDict[boundaryI];
    const vtkStdString &boundaryNameI = beI.BoundaryName;

    const vtkFoamEntry *bEntryI = bEntry->Dictionary().Lookup(boundaryNameI);
    if (bEntryI == NULL)
      {
      vtkErrorMacro(<< "boundaryField " << boundaryNameI.c_str()
          << " not found in object " << varName.c_str() << " at time = "
          << this->TimeNames->GetValue(this->TimeStep).c_str());
      iData->Delete();
      if (acData != NULL)
        {
        acData->Delete();
        }
      if (ctpData != NULL)
        {
        ctpData->Delete();
        }
      return;
      }

    if (bEntryI->FirstValue().GetType() != vtkFoamToken::DICTIONARY)
      {
      vtkErrorMacro(<< "Type of boundaryField " << boundaryNameI.c_str()
          << " is not a subdictionary in object " << varName.c_str()
          << " at time = " << this->TimeNames->GetValue(this->TimeStep).c_str());
      iData->Delete();
      if (acData != NULL)
        {
        acData->Delete();
        }
      if (ctpData != NULL)
        {
        ctpData->Delete();
        }
      return;
      }

    const int nFaces = beI.NFaces;

    vtkFloatArray* vData = NULL;
    bool valueFound = false;
    vtkFoamEntry *vEntry = bEntryI->Dictionary().Lookup("value");
    if (vEntry != NULL) // the boundary has a value entry
      {
      vData = this->FillField(vEntry, nFaces, &io, fieldType);
      if (vData == NULL)
        {
        iData->Delete();
        if (acData != NULL)
          {
          acData->Delete();
          }
        if (ctpData != NULL)
          {
          ctpData->Delete();
          }
        return;
        }
      valueFound = true;
      }
    else
      {
      // uniformFixedValue B.C.
      const vtkFoamEntry *ufvEntry = bEntryI->Dictionary().Lookup("type");
      if (ufvEntry != NULL)
        {
        if (ufvEntry->ToString() == "uniformFixedValue")
          {
          // the boundary is of uniformFixedValue type
          vtkFoamEntry *uvEntry = bEntryI->Dictionary().Lookup("uniformValue");
          if (uvEntry != NULL) // and has a uniformValue entry
            {
            vData = this->FillField(uvEntry, nFaces, &io, fieldType);
            if (vData == NULL)
              {
              iData->Delete();
              if (acData != NULL)
                {
                acData->Delete();
                }
              if (ctpData != NULL)
                {
                ctpData->Delete();
                }
              return;
              }
            valueFound = true;
            }
          }
        }
      }

    const int boundaryStartFace = beI.StartFace
        - this->BoundaryDict[0].StartFace;

    if (!valueFound) // doesn't have a value nor uniformValue entry
      {
      // use patch-internal values as boundary values
      vData = vtkFloatArray::New();
      vData->SetNumberOfComponents(iData->GetNumberOfComponents());
      vData->SetNumberOfTuples(nFaces);
      for (int j = 0; j < nFaces; j++)
        {
        const int cellId = this->FaceOwner->GetValue(boundaryStartFace + j);
        vData->SetTuple(j, cellId, iData);
        }
      }

    if (this->Parent->GetCreateCellToPoint())
      {
      const int startFace = beI.AllBoundariesStartFace;
      // if reading a processor sub-case of a decomposed case as is,
      // use the patch values of the processor patch as is
      if (beI.BoundaryType == vtkFoamBoundaryEntry::PHYSICAL
          || (this->ProcessorName == "" && beI.BoundaryType
              == vtkFoamBoundaryEntry::PROCESSOR))
        {
        // set the same value to AllBoundaries
        for (int faceI = 0; faceI < nFaces; faceI++)
          {
          acData->SetTuple(faceI + startFace, faceI, vData);
          }
        }
      // implies && this->ProcessorName != ""
      else if (beI.BoundaryType == vtkFoamBoundaryEntry::PROCESSOR)
        {
        // average patch internal value and patch value assuming the
        // patch value to be the patchInternalField of the neighbor
        // decomposed mesh. Using double precision to avoid degrade in
        // accuracy.
        const int nComponents = vData->GetNumberOfComponents();
        for (int faceI = 0; faceI < nFaces; faceI++)
          {
          const float *vTuple = vData->GetPointer(nComponents * faceI);
          const float *iTuple = iData->GetPointer(nComponents
              * this->FaceOwner->GetValue(boundaryStartFace + faceI));
          float *acTuple =
              acData->GetPointer(nComponents * (startFace + faceI));
          for (int componentI = 0; componentI < nComponents; componentI++)
            {
            acTuple[componentI] = (static_cast<double>(vTuple[componentI])
                + static_cast<double>(iTuple[componentI])) * 0.5;
            }
          }
        }
      }

    if (beI.IsActive)
      {
      vtkPolyData *bm =
          vtkPolyData::SafeDownCast(boundaryMesh->GetBlock(activeBoundaryI));
      this->AddArrayToFieldData(bm->GetCellData(), vData, io.GetObjectName()
          + dimString);

      if (this->Parent->GetCreateCellToPoint())
        {
        // construct cell-to-point interpolated boundary values. This
        // is done independently from allBoundary interpolation so
        // that the interpolated values are not affected by
        // neighboring patches especially at patch edges and for
        // baffle patches
        vtkFloatArray *pData = vtkFloatArray::New();
        pData->SetNumberOfComponents(vData->GetNumberOfComponents());
        const int nPoints = bm->GetPoints()->GetNumberOfPoints();
        pData->SetNumberOfTuples(nPoints);
        this->InterpolateCellToPoint(pData, vData, bm, NULL, nPoints);
        this->AddArrayToFieldData(bm->GetPointData(), pData, io.GetObjectName()
            + dimString);
        pData->Delete();
        }

      activeBoundaryI++;
      }
    vData->Delete();
    }
  iData->Delete();

  if (this->Parent->GetCreateCellToPoint())
    {
    // Create cell-to-point interpolated data for all boundaries and
    // override internal values
    vtkFloatArray *bpData = vtkFloatArray::New();
    bpData->SetNumberOfComponents(acData->GetNumberOfComponents());
    const int nPoints = this->AllBoundariesPointMap->GetNumberOfTuples();
    bpData->SetNumberOfTuples(nPoints);
    this->InterpolateCellToPoint(bpData, acData, this->AllBoundaries, NULL,
        nPoints);
    acData->Delete();

    if (ctpData != NULL)
      {
      // set cell-to-pint data for internal mesh
      for (int pointI = 0; pointI < nPoints; pointI++)
        {
        ctpData->SetTuple(this->AllBoundariesPointMap->GetValue(pointI),
            pointI, bpData);
        }
      this->AddArrayToFieldData(internalMesh->GetPointData(), ctpData,
          io.GetObjectName() + dimString);
      ctpData->Delete();
      }

    bpData->Delete();
    }
}

//-----------------------------------------------------------------------------
// read point field at a timestep
void vtkOpenFOAMReaderPrivate::GetPointFieldAtTimeStep(
    vtkUnstructuredGrid *internalMesh, vtkMultiBlockDataSet *boundaryMesh,
    const vtkStdString &varName)
{
  vtkFoamIOobject io(this->CasePath);
  vtkFoamDict dict;
  if (!this->ReadFieldFile(&io, &dict, varName,
      this->Parent->PointDataArraySelection))
    {
    return;
    }

  if (io.GetClassName().substr(0, 5) != "point")
    {
    vtkErrorMacro(<< io.GetFileName().c_str() << " is not a pointField");
    return;
    }

  vtkFoamEntry *iEntry = dict.Lookup("internalField");
  if (iEntry == NULL)
    {
    vtkErrorMacro(<<"internalField not found in " << io.GetFileName().c_str());
    return;
    }

  if (iEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
    {
    // if there's no cell there shouldn't be any boundary faces either
    if (this->NumPoints > 0)
      {
      vtkErrorMacro(<<"internalField of " << io.GetFileName().c_str()
          << " is empty");
      }
    return;
    }

  vtkStdString fieldType = io.GetClassName().substr(5, vtkStdString::npos);
  vtkFloatArray *iData = this->FillField(iEntry, this->NumPoints, &io,
      fieldType);
  if (iData == NULL)
    {
    return;
    }

  vtkStdString dimString;
  this->ConstructDimensions(&dimString, &dict);

  // AdditionalCellPoints is NULL if creation of InternalMesh had been skipped
  if (this->AdditionalCellPoints != NULL)
    {
    // point-to-cell interpolation to additional cell centroidal points
    // for decomposed cells
    const int nAdditionalPoints = static_cast<int>(this->AdditionalCellPoints->size());
    const int nComponents = iData->GetNumberOfComponents();
    this->ExtendArray<vtkFloatArray, float>(iData, this->NumPoints
        + nAdditionalPoints);
    for (int i = 0; i < nAdditionalPoints; i++)
      {
      vtkIntArray *acp = this->AdditionalCellPoints->operator[](i);
      int nPoints = acp->GetDataSize();
      double interpolatedValue[9];
      for (int k = 0; k < nComponents; k++)
        {
        interpolatedValue[k] = 0.0;
        }
      for (int j = 0; j < nPoints; j++)
        {
        const float *tuple = iData->GetPointer(nComponents * acp->GetValue(j));
        for (int k = 0; k < nComponents; k++)
          {
          interpolatedValue[k] += tuple[k];
          }
        }
      const double weight = 1.0 / static_cast<double>(nPoints);
      for (int k = 0; k < nComponents; k++)
        {
        interpolatedValue[k] *= weight;
        }
      // will automatically be converted to float
      iData->InsertTuple(this->NumPoints + i, interpolatedValue);
      }
    }

  if (iData->GetSize() > 0)
    {
    // Add field only if internal Mesh exists (skip if not selected).
    // Note we still need to read internalField even if internal mesh is
    // not selected, since boundaries without value entries may refer to
    // the internalField.
    if (internalMesh != NULL)
      {
      // set data to internal mesh
      this->AddArrayToFieldData(internalMesh->GetPointData(), iData,
          io.GetObjectName() + dimString);
      }
    }
  else
    {
    // determine as there's no points
    iData->Delete();
    return;
    }

  // use patch-internal values as boundary values
  for (vtkIdType boundaryI = 0, activeBoundaryI = 0; boundaryI
    < static_cast<vtkIdType>(this->BoundaryDict.size()); boundaryI++)
    {
    if (this->BoundaryDict[boundaryI].IsActive)
      {
      vtkFloatArray *vData = vtkFloatArray::New();
      vtkIntArray& bpMap = *this->BoundaryPointMap->operator[](activeBoundaryI);
      const int nPoints = bpMap.GetNumberOfTuples();
      vData->SetNumberOfComponents(iData->GetNumberOfComponents());
      vData->SetNumberOfTuples(nPoints);
      for (int j = 0; j < nPoints; j++)
        {
        vData->SetTuple(j, bpMap.GetValue(j), iData);
        }
      this->AddArrayToFieldData(vtkPolyData::SafeDownCast(
          boundaryMesh->GetBlock(activeBoundaryI))->GetPointData(), vData, io.GetObjectName()
          + dimString);
      vData->Delete();
      activeBoundaryI++;
      }
    }
  iData->Delete();
}

//-----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkOpenFOAMReaderPrivate::MakeLagrangianMesh()
{
  vtkMultiBlockDataSet *lagrangianMesh = vtkMultiBlockDataSet::New();

  for (int cloudI = 0; cloudI
      < this->Parent->LagrangianPaths->GetNumberOfTuples(); cloudI++)
    {
    const vtkStdString& pathI = this->Parent->LagrangianPaths->GetValue(cloudI);

    // still can't distinguish on patch selection panel, but can
    // distinguish the "lagrangian" reserved path component and a mesh
    // region with the same name
    vtkStdString subCloudName;
    if (pathI[0] == '/')
      {
      subCloudName = pathI.substr(1, vtkStdString::npos);
      }
    else
      {
      subCloudName = pathI;
      }
    if (this->RegionName != pathI.substr(0, pathI.find('/'))
        || !this->Parent->GetPatchArrayStatus(subCloudName.c_str()))
      {
      continue;
      }

    const vtkStdString cloudPath(this->CurrentTimePath() + "/" + subCloudName
        + "/");
    const vtkStdString positionsPath(cloudPath + "positions");

    // create an empty mesh to keep node/leaf structure of the
    // multi-block consistent even if mesh doesn't exist
    vtkPolyData *meshI = vtkPolyData::New();
    const int blockI = lagrangianMesh->GetNumberOfBlocks();
    lagrangianMesh->SetBlock(blockI, meshI);
    // extract the cloud name
    this->SetBlockName(lagrangianMesh, blockI, pathI.substr(pathI.rfind('/') + 1).c_str());

    vtkFoamIOobject io(this->CasePath);
    if (!(io.Open(positionsPath) || io.Open(positionsPath + ".gz")))
      {
      meshI->Delete();
      continue;
      }

    // tell the IO object if the file is in OF 1.3 binary
    // lagrangian/positions format
    io.SetIs13Positions(this->Parent->GetPositionsIsIn13Format() != 0);

    vtkFoamEntryValue dict(NULL);
    try
      {
      dict.ReadNonuniformList<vtkFoamToken::VECTORLIST,
      vtkFoamEntryValue::vectorListTraits<vtkFloatArray, float, 3, true> >(
          io);
      }
    catch(vtkFoamError& e)
      {
      vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
          << " of " << io.GetFileName().c_str() << ": " << e.c_str());
      meshI->Delete();
      continue;
      }
    io.Close();

    vtkFloatArray *pointArray = static_cast<vtkFloatArray *>(dict.Ptr());
    const int nParticles = pointArray->GetNumberOfTuples();

    // instantiate the points class
    vtkPoints *points = vtkPoints::New();
    points->SetData(pointArray);
    pointArray->Delete();

    // create lagrangian mesh
    meshI->Allocate(nParticles);
    for (vtkIdType i = 0; i < nParticles; i++)
      {
      meshI->InsertNextCell(VTK_VERTEX, 1, &i);
      }
    meshI->SetPoints(points);
    points->Delete();

    // read lagrangian fields
    for (int fieldI = 0; fieldI
        < this->LagrangianFieldFiles->GetNumberOfValues(); fieldI++)
      {
      const vtkStdString varPath(cloudPath
          + this->LagrangianFieldFiles->GetValue(fieldI));

      vtkFoamIOobject io2(this->CasePath);
      if (!io2.Open(varPath))
        {
        // if the field file doesn't exist we simply return without
        // issuing an error as a simple way of supporting multi-region
        // lagrangians
        continue;
        }

      // if the variable is disabled on selection panel then skip it
      const vtkStdString selectionName(io2.GetObjectName());
      if (this->Parent->LagrangianDataArraySelection->ArrayExists(selectionName.c_str())
          && !this->Parent->GetLagrangianArrayStatus(selectionName.c_str()))
        {
        continue;
        }

      // read the field file into dictionary
      vtkFoamEntryValue dict2(NULL);
      if (!dict2.ReadField(io2))
        {
        vtkErrorMacro(<<"Error reading line " << io2.GetLineNumber()
            << " of " << io2.GetFileName().c_str() << ": "
            << io2.GetError().c_str());
        continue;
        }

      // set lagrangian values
      if (dict2.GetType() != vtkFoamToken::SCALARLIST && dict2.GetType()
          != vtkFoamToken::VECTORLIST && dict2.GetType()
          != vtkFoamToken::LABELLIST)
        {
        vtkErrorMacro(<< io2.GetFileName().c_str()
            << ": Unsupported lagrangian field type "
            << io2.GetClassName().c_str());
        continue;
        }

      vtkDataArray* lData = static_cast<vtkDataArray *>(dict2.Ptr());

      // GetNumberOfTuples() works for both scalar and vector
      const int nParticles2 = lData->GetNumberOfTuples();
      if (nParticles2 != meshI->GetNumberOfCells())
        {
        vtkErrorMacro(<< io2.GetFileName().c_str()
            <<": Sizes of lagrangian mesh and field don't match: mesh = "
            << meshI->GetNumberOfCells() << ", field = " << nParticles2);
        lData->Delete();
        continue;
        }

      this->AddArrayToFieldData(meshI->GetCellData(), lData, selectionName);
      if (this->Parent->GetCreateCellToPoint())
        {
        this->AddArrayToFieldData(meshI->GetPointData(), lData, selectionName);
        }
      lData->Delete();
      }
    meshI->Delete();
    }
  return lagrangianMesh;
}

//-----------------------------------------------------------------------------
// returns a dictionary of block names for a specified domain
vtkFoamDict* vtkOpenFOAMReaderPrivate::GatherBlocks(const char* typeIn, bool mustRead)
{
  vtkStdString type(typeIn);
  vtkStdString blockPath =
      this->CurrentTimeRegionMeshPath(this->PolyMeshFacesDir) + type;

  vtkFoamIOobject io(this->CasePath);
  if (!(io.Open(blockPath) || io.Open(blockPath + ".gz")))
    {
    if (mustRead)
      {
      vtkErrorMacro(<<"Error opening " << io.GetFileName().c_str() << ": "
          << io.GetError().c_str());
      }
    return NULL;
    }

  vtkFoamDict* dictPtr = new vtkFoamDict;
  vtkFoamDict& dict = *dictPtr;
  if (!dict.Read(io))
    {
    vtkErrorMacro(<<"Error reading line " << io.GetLineNumber()
        << " of " << io.GetFileName().c_str() << ": " << io.GetError().c_str());
    delete dictPtr;
    return NULL;
    }
  if (dict.GetType() != vtkFoamToken::DICTIONARY)
    {
    vtkErrorMacro(<<"The file type of " << io.GetFileName().c_str()
        << " is not a dictionary");
    delete dictPtr;
    return NULL;
    }
  return dictPtr;
}

//-----------------------------------------------------------------------------
// returns a requested point zone mesh
bool vtkOpenFOAMReaderPrivate::GetPointZoneMesh(
    vtkMultiBlockDataSet *pointZoneMesh, vtkPoints *points)
{
  vtkFoamDict *pointZoneDictPtr = this->GatherBlocks("pointZones", false);

  if (pointZoneDictPtr == NULL)
    {
    // not an error
    return true;
    }

  vtkFoamDict &pointZoneDict = *pointZoneDictPtr;
  int nPointZones = static_cast<int>(pointZoneDict.size());

  for (int i = 0; i < nPointZones; i++)
    {
    // look up point labels
    vtkFoamDict &dict = pointZoneDict[i]->Dictionary();
    vtkFoamEntry *pointLabelsEntry = dict.Lookup("pointLabels");
    if (pointLabelsEntry == NULL)
      {
      delete pointZoneDictPtr;
      vtkErrorMacro(<<"pointLabels not found in pointZones");
      return false;
      }

    // allocate an empty mesh if the list is empty
    if (pointLabelsEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
      {
      vtkPolyData *pzm = vtkPolyData::New();
      pointZoneMesh->SetBlock(i, pzm);
      pzm->Delete();
      // set name
      this->SetBlockName(pointZoneMesh, i, pointZoneDict[i]->GetKeyword().c_str());
      continue;
      }

    if (pointLabelsEntry->FirstValue().GetType() != vtkFoamToken::LABELLIST)
      {
      delete pointZoneDictPtr;
      vtkErrorMacro(<<"pointLabels not of type labelList: type = "
          << pointLabelsEntry->FirstValue().GetType());
      return false;
      }

    vtkIntArray &labels = pointLabelsEntry->LabelList();

    int nPoints = labels.GetNumberOfTuples();
    if (nPoints > this->NumPoints)
      {
      vtkErrorMacro(<<"The length of pointLabels " << nPoints
          << " for pointZone " << pointZoneDict[i]->GetKeyword().c_str()
          << " exceeds the number of points " << this->NumPoints);
      delete pointZoneDictPtr;
      return false;
      }

    // allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    vtkPolyData *pzm = vtkPolyData::New();

    // set pointZone size
    pzm->Allocate(nPoints);

    // insert points
    for (int j = 0; j < nPoints; j++)
      {
      vtkIdType pointLabel = labels.GetValue(j); // must be vtkIdType
      if (pointLabel >= this->NumPoints)
        {
        vtkWarningMacro(<<"pointLabels id " << pointLabel
            << " exceeds the number of points " << this->NumPoints);
        pzm->InsertNextCell(VTK_EMPTY_CELL, 0, &pointLabel);
        continue;
        }
      pzm->InsertNextCell(VTK_VERTEX, 1, &pointLabel);
      }
    pzm->SetPoints(points);

    pointZoneMesh->SetBlock(i, pzm);
    pzm->Delete();
    // set name
    this->SetBlockName(pointZoneMesh, i, pointZoneDict[i]->GetKeyword().c_str());
    }

  delete pointZoneDictPtr;

  return true;
}

//-----------------------------------------------------------------------------
// returns a requested face zone mesh
bool vtkOpenFOAMReaderPrivate::GetFaceZoneMesh(
    vtkMultiBlockDataSet *faceZoneMesh,
    const vtkFoamIntVectorVector *facesPoints, vtkPoints *points)
{
  vtkFoamDict *faceZoneDictPtr = this->GatherBlocks("faceZones", false);

  if (faceZoneDictPtr == NULL)
    {
    // not an error
    return true;
    }

  vtkFoamDict &faceZoneDict = *faceZoneDictPtr;
  int nFaceZones = static_cast<int>(faceZoneDict.size());

  for (int i = 0; i < nFaceZones; i++)
    {
    // look up face labels
    vtkFoamDict &dict = faceZoneDict[i]->Dictionary();
    vtkFoamEntry *faceLabelsEntry = dict.Lookup("faceLabels");
    if (faceLabelsEntry == NULL)
      {
      delete faceZoneDictPtr;
      vtkErrorMacro(<<"faceLabels not found in faceZones");
      return false;
      }

    // allocate an empty mesh if the list is empty
    if (faceLabelsEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
      {
      vtkPolyData *fzm = vtkPolyData::New();
      faceZoneMesh->SetBlock(i, fzm);
      fzm->Delete();
      // set name
      this->SetBlockName(faceZoneMesh, i, faceZoneDict[i]->GetKeyword().c_str());
      continue;
      }

    if (faceLabelsEntry->FirstValue().GetType() != vtkFoamToken::LABELLIST)
      {
      delete faceZoneDictPtr;
      vtkErrorMacro(<<"faceLabels not of type labelList");
      return false;
      }

    vtkIntArray &labels = faceLabelsEntry->LabelList();

    int nFaces = labels.GetNumberOfTuples();
    if (nFaces > this->FaceOwner->GetNumberOfTuples())
      {
      vtkErrorMacro(<<"The length of faceLabels " << nFaces
          << " for faceZone " << faceZoneDict[i]->GetKeyword().c_str()
          << " exceeds the number of faces "
          << this->FaceOwner->GetNumberOfTuples());
      delete faceZoneDictPtr;
      return false;
      }

    // allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    vtkPolyData *fzm = vtkPolyData::New();

    // set faceZone size
    fzm->Allocate(nFaces);

    // aloocate array for converting int vector to vtkIdType vector:
    // workaround for 64bit machines
    int maxNFacePoints = 0;
    for (int j = 0; j < nFaces; j++)
      {
      const int nFacePoints = facesPoints->GetSize(labels.GetValue(j));
      if (nFacePoints > maxNFacePoints)
        {
        maxNFacePoints = nFacePoints;
        }
      }
    vtkIdList *facePointsVtkId = vtkIdList::New();
    facePointsVtkId->SetNumberOfIds(maxNFacePoints);

    // insert faces
    this->InsertFacesToGrid(fzm, facesPoints, 0, nFaces, NULL, facePointsVtkId,
        &labels, false);

    facePointsVtkId->Delete();
    fzm->SetPoints(points);
    faceZoneMesh->SetBlock(i, fzm);
    fzm->Delete();
    // set name
    this->SetBlockName(faceZoneMesh, i, faceZoneDict[i]->GetKeyword().c_str());
    }

  delete faceZoneDictPtr;

  return true;
}

//-----------------------------------------------------------------------------
// returns a requested cell zone mesh
bool vtkOpenFOAMReaderPrivate::GetCellZoneMesh(
    vtkMultiBlockDataSet *cellZoneMesh,
    const vtkFoamIntVectorVector *cellsFaces,
    const vtkFoamIntVectorVector *facesPoints, vtkPoints *points)
{
  vtkFoamDict *cellZoneDictPtr = this->GatherBlocks("cellZones", false);

  if (cellZoneDictPtr == NULL)
    {
    // not an error
    return true;
    }

  vtkFoamDict &cellZoneDict = *cellZoneDictPtr;
  int nCellZones = static_cast<int>(cellZoneDict.size());

  for (int i = 0; i < nCellZones; i++)
    {
    // look up cell labels
    vtkFoamDict &dict = cellZoneDict[i]->Dictionary();
    vtkFoamEntry *cellLabelsEntry = dict.Lookup("cellLabels");
    if (cellLabelsEntry == NULL)
      {
      delete cellZoneDictPtr;
      vtkErrorMacro(<<"cellLabels not found in cellZones");
      return false;
      }

    // allocate an empty mesh if the list is empty
    if (cellLabelsEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
      {
      vtkUnstructuredGrid *czm = vtkUnstructuredGrid::New();
      cellZoneMesh->SetBlock(i, czm);
      // set name
      this->SetBlockName(cellZoneMesh, i, cellZoneDict[i]->GetKeyword().c_str());
      continue;
      }

    if (cellLabelsEntry->FirstValue().GetType() != vtkFoamToken::LABELLIST)
      {
      delete cellZoneDictPtr;
      vtkErrorMacro(<<"cellLabels not of type labelList");
      return false;
      }

    vtkIntArray &labels = cellLabelsEntry->LabelList();

    int nCells = labels.GetNumberOfTuples();
    if (nCells > this->NumCells)
      {
      vtkErrorMacro(<<"The length of cellLabels " << nCells
          << " for cellZone " << cellZoneDict[i]->GetKeyword().c_str()
          << " exceeds the number of cells " << this->NumCells);
      delete cellZoneDictPtr;
      return false;
      }

    // allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointers if we return by error
    vtkUnstructuredGrid *czm = vtkUnstructuredGrid::New();

    // set cellZone size
    czm->Allocate(nCells);

    // insert cells
    this->InsertCellsToGrid(czm, cellsFaces, facesPoints, NULL, NULL, &labels);

    // set cell zone points
    czm->SetPoints(points);

    cellZoneMesh->SetBlock(i, czm);
    czm->Delete();

    // set name
    this->SetBlockName(cellZoneMesh, i, cellZoneDict[i]->GetKeyword().c_str());
    }

  delete cellZoneDictPtr;
  return true;
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::AddArrayToFieldData(
    vtkDataSetAttributes *fieldData, vtkDataArray *array,
    const vtkStdString &arrayName)
{
  // exclude dimensional unit string if any
  const vtkStdString arrayNameString(arrayName.substr(0, arrayName.find(' ')));
  array->SetName(arrayName.c_str());

  if (array->GetNumberOfComponents() == 1 && arrayNameString == "p")
    {
    fieldData->SetScalars(array);
    }
  else if (array->GetNumberOfComponents() == 3 && arrayNameString == "U")
    {
    fieldData->SetVectors(array);
    }
  else
    {
    fieldData->AddArray(array);
    }
}

//-----------------------------------------------------------------------------
// return 0 if there's any error, 1 if success
int vtkOpenFOAMReaderPrivate::RequestData(vtkMultiBlockDataSet *output,
    bool recreateInternalMesh, bool recreateBoundaryMesh, bool updateVariables)
{
  recreateInternalMesh |= this->TimeStepOld == -1
      || this->InternalMeshSelectionStatus
          != this->InternalMeshSelectionStatusOld
      || this->PolyMeshFacesDir->GetValue(this->TimeStep)
          != this->PolyMeshFacesDir->GetValue(this->TimeStepOld)
      || this->FaceOwner == NULL;
  recreateBoundaryMesh |= recreateInternalMesh;
  updateVariables |= recreateBoundaryMesh || this->TimeStep
      != this->TimeStepOld;
  const bool pointsMoved = this->TimeStepOld == -1
      || this->PolyMeshPointsDir->GetValue(this->TimeStep)
          != this->PolyMeshPointsDir->GetValue(this->TimeStepOld);
  const bool moveInternalPoints = !recreateInternalMesh && pointsMoved;
  const bool moveBoundaryPoints = !recreateBoundaryMesh && pointsMoved;

  // RegionName check is added since subregions have region name prefixes
  const bool createEulerians = this->Parent->PatchDataArraySelection
  ->ArrayExists("internalMesh") || this->RegionName != "";

  // determine if we need to reconstruct meshes
  if (recreateInternalMesh)
    {
    this->ClearInternalMeshes();
    }
  if (recreateBoundaryMesh)
    {
    this->ClearBoundaryMeshes();
    }

  vtkFoamIntVectorVector *facePoints = NULL;
  vtkStdString meshDir;
  if (createEulerians && (recreateInternalMesh || recreateBoundaryMesh))
    {
    // create paths to polyMesh files
    meshDir = this->CurrentTimeRegionMeshPath(this->PolyMeshFacesDir);

    // create the faces vector
    facePoints = this->ReadFacesFile(meshDir);
    if (facePoints == NULL)
      {
      return 0;
      }
    this->Parent->UpdateProgress(0.2);
    }

  vtkFoamIntVectorVector *cellFaces = NULL;
  if (createEulerians && recreateInternalMesh)
    {
    // read owner/neighbor and create the FaceOwner and cellFaces vectors
    cellFaces = this->ReadOwnerNeighborFiles(meshDir, facePoints);
    if (cellFaces == NULL)
      {
      delete facePoints;
      return 0;
      }
    this->Parent->UpdateProgress(0.3);
    }

  vtkFloatArray *pointArray = NULL;
  if (createEulerians && (recreateInternalMesh || (recreateBoundaryMesh
      && !recreateInternalMesh && this->InternalMesh == NULL)
      || moveInternalPoints || moveBoundaryPoints))
    {
    // get the points
    pointArray = this->ReadPointsFile();
    if ((pointArray == NULL && recreateInternalMesh) || (facePoints != NULL
        && !this->CheckFacePoints(facePoints)))
      {
      delete cellFaces;
      delete facePoints;
      return 0;
      }
    this->Parent->UpdateProgress(0.4);
    }

  // make internal mesh
  // Create Internal Mesh only if required for display
  if (createEulerians && recreateInternalMesh)
    {
    if (this->Parent->GetPatchArrayStatus((this->RegionPrefix() + "internalMesh").c_str()))
      {
      this->InternalMesh = this->MakeInternalMesh(cellFaces, facePoints,
          pointArray);
      }
    // read and construct zones
    if (this->Parent->GetReadZones())
      {
      vtkPoints *points;
      if (this->InternalMesh != NULL)
        {
        points = this->InternalMesh->GetPoints();
        }
      else
        {
        points = vtkPoints::New();
        points->SetData(pointArray);
        }

      this->PointZoneMesh = vtkMultiBlockDataSet::New();
      if (!this->GetPointZoneMesh(this->PointZoneMesh, points))
        {
        this->PointZoneMesh->Delete();
        this->PointZoneMesh = NULL;
        delete cellFaces;
        delete facePoints;
        if (this->InternalMesh == NULL)
          {
          points->Delete();
          }
        pointArray->Delete();
        return 0;
        }
      if (this->PointZoneMesh->GetNumberOfBlocks() == 0)
        {
        this->PointZoneMesh->Delete();
        this->PointZoneMesh = NULL;
        }

      this->FaceZoneMesh = vtkMultiBlockDataSet::New();
      if (!this->GetFaceZoneMesh(this->FaceZoneMesh, facePoints, points))
        {
        this->FaceZoneMesh->Delete();
        this->FaceZoneMesh = NULL;
        if (this->PointZoneMesh != NULL)
          {
          this->PointZoneMesh->Delete();
          this->PointZoneMesh = NULL;
          }
        delete cellFaces;
        delete facePoints;
        if (this->InternalMesh == NULL)
          {
          points->Delete();
          }
        pointArray->Delete();
        return 0;
        }
      if (this->FaceZoneMesh->GetNumberOfBlocks() == 0)
        {
        this->FaceZoneMesh->Delete();
        this->FaceZoneMesh = NULL;
        }

      this->CellZoneMesh = vtkMultiBlockDataSet::New();
      if (!this->GetCellZoneMesh(this->CellZoneMesh, cellFaces, facePoints,
          points))
        {
        this->CellZoneMesh->Delete();
        this->CellZoneMesh = NULL;
        if (this->FaceZoneMesh != NULL)
          {
          this->FaceZoneMesh->Delete();
          this->FaceZoneMesh = NULL;
          }
        if (this->PointZoneMesh != NULL)
          {
          this->PointZoneMesh->Delete();
          this->PointZoneMesh = NULL;
          }
        delete cellFaces;
        delete facePoints;
        if (this->InternalMesh == NULL)
          {
          points->Delete();
          }
        pointArray->Delete();
        return 0;
        }
      if (this->CellZoneMesh->GetNumberOfBlocks() == 0)
        {
        this->CellZoneMesh->Delete();
        this->CellZoneMesh = NULL;
        }
      if (this->InternalMesh == NULL)
        {
        points->Delete();
        }
      }
    delete cellFaces;
    this->TruncateFaceOwner();
    }

  if (createEulerians && recreateBoundaryMesh)
    {
    vtkFloatArray *boundaryPointArray;
    if (pointArray != NULL)
      {
      boundaryPointArray = pointArray;
      }
    else
      {
      boundaryPointArray
          = static_cast<vtkFloatArray *>(this->InternalMesh->GetPoints()->GetData());
      }
    // create boundary mesh
    this->BoundaryMesh = this->MakeBoundaryMesh(facePoints, boundaryPointArray);
    if (this->BoundaryMesh == NULL)
      {
      delete facePoints;
      if (pointArray != NULL)
        {
        pointArray->Delete();
        }
      return 0;
      }
    }

  delete facePoints;

  // if only point coordinates change refresh point vector
  if (createEulerians && moveInternalPoints)
    {
    // refresh the points in each mesh
    vtkPoints *points;
    // Check if Internal Mesh exists first....
    if (this->InternalMesh != NULL)
      {
      points = this->MoveInternalMesh(this->InternalMesh, pointArray);
      if (points == NULL)
        {
        pointArray->Delete();
        return 0;
        }
      }
    else
      {
      points = vtkPoints::New();
      points->SetData(pointArray);
      }

    if (this->PointZoneMesh != NULL)
      {
      for (unsigned int i = 0; i < this->PointZoneMesh->GetNumberOfBlocks(); i++)
        {
        vtkPolyData::SafeDownCast(this->PointZoneMesh->GetBlock(i))
        ->SetPoints(points);
        }
      }
    if (this->FaceZoneMesh != NULL)
      {
      for (unsigned int i = 0; i < this->FaceZoneMesh->GetNumberOfBlocks(); i++)
        {
        vtkPolyData::SafeDownCast(this->FaceZoneMesh->GetBlock(i))
        ->SetPoints(points);
        }
      }
    if (this->CellZoneMesh != NULL)
      {
      for (unsigned int i = 0; i < this->CellZoneMesh->GetNumberOfBlocks(); i++)
        {
        vtkUnstructuredGrid::SafeDownCast(this->CellZoneMesh->GetBlock(i))
        ->SetPoints(points);
        }
      }
    points->Delete();
    }

  if (createEulerians && moveBoundaryPoints)
    {
    // Check if Boundary Mesh exists first....
    if (this->BoundaryMesh != NULL)
      {
      this->MoveBoundaryMesh(this->BoundaryMesh, pointArray);
      }
    }

  if (pointArray != NULL)
    {
    pointArray->Delete();
    }
  this->Parent->UpdateProgress(0.5);

  vtkMultiBlockDataSet *lagrangianMesh = NULL;
  if (updateVariables)
    {
    if (createEulerians)
      {
      if (!recreateInternalMesh && this->InternalMesh != NULL)
        {
        // clean up arrays of the previous timestep
        // Check if Internal Mesh Exists first...
        this->InternalMesh->GetCellData()->Initialize();
        this->InternalMesh->GetPointData()->Initialize();
        }
      // Check if Boundary Mesh Exists first...
      if (!recreateBoundaryMesh && this->BoundaryMesh != NULL)
        {
        for (unsigned int i = 0; i < this->BoundaryMesh->GetNumberOfBlocks(); i++)
          {
          vtkPolyData *bm =
              vtkPolyData::SafeDownCast(this->BoundaryMesh->GetBlock(i));
          bm->GetCellData()->Initialize();
          bm->GetPointData()->Initialize();
          }
        }
      // read field data variables into Internal/Boundary meshes
      for (int i = 0; i < (int)this->VolFieldFiles->GetNumberOfValues(); i++)
        {
        this->GetVolFieldAtTimeStep(this->InternalMesh, this->BoundaryMesh,
            this->VolFieldFiles->GetValue(i));
        this->Parent->UpdateProgress(0.5 + 0.25 * ((float)(i + 1)
            / ((float)this->VolFieldFiles->GetNumberOfValues() + 0.0001)));
        }
      for (int i = 0; i < (int)this->PointFieldFiles->GetNumberOfValues(); i++)
        {
        this->GetPointFieldAtTimeStep(this->InternalMesh, this->BoundaryMesh,
            this->PointFieldFiles->GetValue(i));
        this->Parent->UpdateProgress(0.75 + 0.125 * ((float)(i + 1)
            / ((float)this->PointFieldFiles->GetNumberOfValues() + 0.0001)));
        }
      }
    // read lagrangian mesh and fields
    lagrangianMesh = this->MakeLagrangianMesh();
    }

  // Add Internal Mesh to final output only if selected for display
  if (this->InternalMesh != NULL)
    {
    output->SetBlock(0, this->InternalMesh);
    this->SetBlockName(output, 0, "internalMesh");
    }

  // set boundary meshes/data as output
  if (this->BoundaryMesh != NULL && this->BoundaryMesh->GetNumberOfBlocks() > 0)
    {
    const unsigned int groupTypeI = output->GetNumberOfBlocks();
    output->SetBlock(groupTypeI, this->BoundaryMesh);
    this->SetBlockName(output, groupTypeI, "Patches");
    }

  // set lagrangian mesh as output
  if (lagrangianMesh != NULL)
    {
    if (lagrangianMesh->GetNumberOfBlocks() > 0)
      {
      const unsigned int groupTypeI = output->GetNumberOfBlocks();
      output->SetBlock(groupTypeI, lagrangianMesh);
      this->SetBlockName(output, groupTypeI, "Lagrangian Particles");
      }
    lagrangianMesh->Delete();
    }

  if (this->Parent->GetReadZones())
    {
    vtkMultiBlockDataSet *zones = NULL;
    // set Zone Meshes as output
    if (this->PointZoneMesh != NULL)
      {
      zones = vtkMultiBlockDataSet::New();
      const unsigned int zoneTypeI = zones->GetNumberOfBlocks();
      zones->SetBlock(zoneTypeI, this->PointZoneMesh);
      this->SetBlockName(zones, zoneTypeI, "pointZones");
      }

    if (this->FaceZoneMesh != NULL)
      {
      if (zones == NULL)
        {
        zones = vtkMultiBlockDataSet::New();
        }
      const unsigned int zoneTypeI = zones->GetNumberOfBlocks();
      zones->SetBlock(zoneTypeI, this->FaceZoneMesh);
      this->SetBlockName(zones, zoneTypeI, "faceZones");
      }

    if (this->CellZoneMesh != NULL)
      {
      if (zones == NULL)
        {
        zones = vtkMultiBlockDataSet::New();
        }
      const unsigned int zoneTypeI = zones->GetNumberOfBlocks();
      zones->SetBlock(zoneTypeI, this->CellZoneMesh);
      this->SetBlockName(zones, zoneTypeI, "cellZones");
      }
    if (zones != NULL)
      {
      const unsigned int groupTypeI = output->GetNumberOfBlocks();
      output->SetBlock(groupTypeI, zones);
      this->SetBlockName(output, groupTypeI, "Zones");
      }
    }

  if (this->Parent->GetCacheMesh())
    {
    this->TimeStepOld = this->TimeStep;
    }
  else
    {
    this->ClearMeshes();
    this->TimeStepOld = -1;
    }
  this->InternalMeshSelectionStatusOld = this->InternalMeshSelectionStatus;

  this->Parent->UpdateProgress(1.0);
  return 1;
}

//-----------------------------------------------------------------------------
// constructor
vtkOpenFOAMReader::vtkOpenFOAMReader()
{
  this->SetNumberOfInputPorts(0);

  this->Parent = this;
  // must be false to avoid reloading by vtkAppendCompositeDataLeaves::Update()
  this->Refresh = false;

  // INTIALIZE FILE NAME
  this->FileName = NULL;
  this->FileNameOld = new vtkStdString;

  // Case path
  this->CasePath = vtkCharArray::New();

  // Child readers
  this->Readers = vtkCollection::New();

  // VTK CLASSES
  this->PatchDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->LagrangianDataArraySelection = vtkDataArraySelection::New();

  this->PatchSelectionMTimeOld = 0;
  this->CellSelectionMTimeOld = 0;
  this->PointSelectionMTimeOld = 0;
  this->LagrangianSelectionMTimeOld = 0;

  // for creating cell-to-point translated data
  this->CreateCellToPoint = 1;
  this->CreateCellToPointOld = 1;

  // for caching mesh
  this->CacheMesh = 1;

  // for decomposing polyhedra
  this->DecomposePolyhedra = 0;
  this->DecomposePolyhedraOld = 0;

  // for reading old binary lagrangian/positions format
  this->PositionsIsIn13Format = 0; // turned off by default
  this->PositionsIsIn13FormatOld = 0;

  // for reading zones
  this->ReadZones = 0; // turned off by default
  this->ReadZonesOld = 0;

  // determine if time directories are to be listed according to controlDict
  this->ListTimeStepsByControlDict = 0;
  this->ListTimeStepsByControlDictOld = 0;

  // add dimensions to array names
  this->AddDimensionsToArrayNames = 0;
  this->AddDimensionsToArrayNamesOld = 0;

  // Lagrangian paths
  this->LagrangianPaths = vtkStringArray::New();

  this->CurrentReaderIndex = 0;
  this->NumberOfReaders = 0;
}

//-----------------------------------------------------------------------------
// destructor
vtkOpenFOAMReader::~vtkOpenFOAMReader()
{
  this->LagrangianPaths->Delete();

  this->PatchDataArraySelection->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  this->LagrangianDataArraySelection->Delete();

  this->Readers->Delete();
  this->CasePath->Delete();

  this->SetFileName(0);
  delete this->FileNameOld;
}

//-----------------------------------------------------------------------------
// CanReadFile
int vtkOpenFOAMReader::CanReadFile(const char *vtkNotUsed(fileName))
{
  return 1; // so far CanReadFile does nothing.
}

//-----------------------------------------------------------------------------
// PrintSelf
void vtkOpenFOAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)")
      << endl;
  os << indent << "Refresh: " << this->Refresh << endl;
  os << indent << "CreateCellToPoint: " << this->CreateCellToPoint << endl;
  os << indent << "CacheMesh: " << this->CacheMesh << endl;
  os << indent << "DecomposePolyhedra: " << this->DecomposePolyhedra << endl;
  os << indent << "PositionsIsIn13Format: " << this->PositionsIsIn13Format
      << endl;
  os << indent << "ReadZones: " << this->ReadZones << endl;
  os << indent << "ListTimeStepsByControlDict: "
      << this->ListTimeStepsByControlDict << endl;
  os << indent << "AddDimensionsToArrayNames: "
      << this->AddDimensionsToArrayNames << endl;

  this->Readers->InitTraversal();
  vtkObject *reader;
  while ((reader = this->Readers->GetNextItemAsObject()) != NULL)
    {
    os << indent << "Reader instance " << static_cast<void *>(reader) << ": \n";
    reader->PrintSelf(os, indent.GetNextIndent());
    }

  return;
}

//-----------------------------------------------------------------------------
// selection list handlers

int vtkOpenFOAMReader::GetNumberOfSelectionArrays(vtkDataArraySelection *s)
{
  return s->GetNumberOfArrays();
}

int vtkOpenFOAMReader::GetSelectionArrayStatus(vtkDataArraySelection *s,
    const char *name)
{
  return s->ArrayIsEnabled(name);
}

void vtkOpenFOAMReader::SetSelectionArrayStatus(vtkDataArraySelection *s,
    const char* name, int status)
{
  unsigned long int mTime = s->GetMTime();
  if (status)
    {
    s->EnableArray(name);
    }
  else
    {
    s->DisableArray(name);
    }
  if (mTime != s->GetMTime()) // indicate that the pipeline needs to be updated
    {
    this->Modified();
    }
}

const char *vtkOpenFOAMReader::GetSelectionArrayName(vtkDataArraySelection *s,
    int index)
{
  return s->GetArrayName(index);
}

void vtkOpenFOAMReader::DisableAllSelectionArrays(vtkDataArraySelection *s)
{
  unsigned long int mTime = s->GetMTime();
  s->DisableAllArrays();
  if (mTime != s->GetMTime())
    {
    this->Modified();
    }
}

void vtkOpenFOAMReader::EnableAllSelectionArrays(vtkDataArraySelection *s)
{
  unsigned long int mTime = s->GetMTime();
  s->EnableAllArrays();
  if (mTime != s->GetMTime())
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
// RequestInformation
int vtkOpenFOAMReader::RequestInformation(vtkInformation *vtkNotUsed(request), vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  if (!this->FileName || strlen(this->FileName) == 0)
    {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
    }

  if (this->Parent == this && (*this->FileNameOld != this->FileName
      || this->ListTimeStepsByControlDict
          != this->ListTimeStepsByControlDictOld || this->Refresh))
    {
    // retain selection status when just refreshing a case
    if (*this->FileNameOld != "" && *this->FileNameOld != this->FileName)
      {
      // clear selections
      this->CellDataArraySelection->RemoveAllArrays();
      this->PointDataArraySelection->RemoveAllArrays();
      this->LagrangianDataArraySelection->RemoveAllArrays();
      this->PatchDataArraySelection->RemoveAllArrays();
      }

    // Reset NumberOfReaders here so that the variable will not be
    // reset unwantedly when MakeInformationVector() is called from
    // vtkPOpenFOAMReader
    this->NumberOfReaders = 0;

    if (!this->MakeInformationVector(outputVector, vtkStdString(""))
        || !this->MakeMetaDataAtTimeStep(true))
      {
      return 0;
      }
    this->Refresh = false;
    }
  return 1;
}

//-----------------------------------------------------------------------------
// RequestData
int vtkOpenFOAMReader::RequestData(vtkInformation *vtkNotUsed(request), vtkInformationVector **vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet
      *output =
          vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int nSteps = 0;
  double *requestedTimeValues = NULL;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    requestedTimeValues
        = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }

  if (nSteps > 0)
    {
    outInfo->Set(vtkDataObject::DATA_TIME_STEPS(), requestedTimeValues, 1);
    this->SetTimeValue(requestedTimeValues[0]);
    }

  if (this->Parent == this)
    {
    output->GetFieldData()->AddArray(this->CasePath);
    if (!this->MakeMetaDataAtTimeStep(false))
      {
      return 0;
      }
    this->CurrentReaderIndex = 0;
    }

  // compute flags
  // internal mesh selection change is detected within each reader
  const bool recreateInternalMesh = (!this->Parent->CacheMesh)
      || this->Parent->DecomposePolyhedra
          != this->Parent->DecomposePolyhedraOld || this->Parent->ReadZones
      != this->Parent->ReadZonesOld || this->Parent->ListTimeStepsByControlDict
      != this->Parent->ListTimeStepsByControlDictOld;
  const bool recreateBoundaryMesh =
      this->Parent->PatchDataArraySelection->GetMTime()
          != this->Parent->PatchSelectionMTimeOld
          || this->Parent->CreateCellToPoint
              != this->Parent->CreateCellToPointOld;
  const bool updateVariables = this->Parent->CellDataArraySelection->GetMTime()
      != this->Parent->CellSelectionMTimeOld
      || this->Parent->PointDataArraySelection->GetMTime()
          != this->Parent->PointSelectionMTimeOld
      || this->Parent->LagrangianDataArraySelection->GetMTime()
          != this->Parent->LagrangianSelectionMTimeOld
      || this->Parent->PositionsIsIn13Format
          != this->Parent->PositionsIsIn13FormatOld
      || this->Parent->AddDimensionsToArrayNames
          != this->Parent->AddDimensionsToArrayNamesOld;

  // create dataset
  int ret = 1;
  vtkOpenFOAMReaderPrivate *reader;
  // if the only region is not a subregion, omit being wrapped by a
  // multiblock dataset
  if (this->Readers->GetNumberOfItems() == 1 && (reader = vtkOpenFOAMReaderPrivate::SafeDownCast(
          this->Readers->GetItemAsObject(0)))->GetRegionName() == "")
    {
    ret = reader->RequestData(output, recreateInternalMesh,
        recreateBoundaryMesh, updateVariables);
    this->Parent->CurrentReaderIndex++;
    }
  else
    {
    this->Readers->InitTraversal();
    while ((reader
        = vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetNextItemAsObject()))
        != NULL)
      {
      vtkMultiBlockDataSet *subOutput = vtkMultiBlockDataSet::New();
      if (reader->RequestData(subOutput, recreateInternalMesh,
          recreateBoundaryMesh, updateVariables))
        {
        vtkStdString regionName(reader->GetRegionName());
        if (regionName == "")
          {
          regionName = "defaultRegion";
          }
        const int blockI = output->GetNumberOfBlocks();
        output->SetBlock(blockI, subOutput);
        output->GetMetaData(blockI)->Set(vtkCompositeDataSet::NAME(), regionName.c_str());
        }
      else
        {
        ret = 0;
        }
      subOutput->Delete();
      this->Parent->CurrentReaderIndex++;
      }
    }

  if (this->Parent == this) // update only if this is the top-level reader
    {
    this->UpdateStatus();
    }

  return ret;
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReader::SetTimeInformation(vtkInformationVector *outputVector,
    vtkDoubleArray *timeValues)
{
  if (timeValues->GetNumberOfTuples() > 0)
    {
    outputVector->GetInformationObject(0)->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
        timeValues->GetPointer(0), timeValues->GetNumberOfTuples());

    double timeRange[2];
    timeRange[0] = timeValues->GetValue(0);
    timeRange[1] = timeValues->GetValue(timeValues->GetNumberOfTuples() - 1);
    outputVector->GetInformationObject(0)->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
  else
    {
    double timeRange[2];
    timeRange[0] = timeRange[1] = 0.0;
    outputVector->GetInformationObject(0)->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeRange, 0);
    outputVector->GetInformationObject(0)->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenFOAMReader::MakeInformationVector(
    vtkInformationVector *outputVector, const vtkStdString& procName)
{
  *this->FileNameOld = vtkStdString(this->FileName);

  // clear prior case information
  this->Readers->RemoveAllItems();

  // recreate case information
  vtkStdString casePath, controlDictPath;
  this->CreateCasePath(casePath, controlDictPath);
  casePath += procName + (procName == "" ? "" : "/");
  vtkOpenFOAMReaderPrivate *masterReader = vtkOpenFOAMReaderPrivate::New();
  if (!masterReader->MakeInformationVector(casePath, controlDictPath, procName,
      this->Parent))
    {
    masterReader->Delete();
    return 0;
    }

  if (masterReader->GetTimeValues()->GetNumberOfTuples() == 0)
    {
    vtkErrorMacro(<< this->FileName << " contains no timestep data.");
    masterReader->Delete();
    return 0;
    }

  this->Readers->AddItem(masterReader);

  if (outputVector != NULL)
    {
    this->SetTimeInformation(outputVector, masterReader->GetTimeValues());
    }

  // search subregions under constant subdirectory
  vtkStdString constantPath(casePath + "constant/");
  vtkDirectory *dir = vtkDirectory::New();
  if (!dir->Open(constantPath.c_str()))
    {
    vtkErrorMacro(<< "Can't open " << constantPath.c_str());
    return 0;
    }
  for (int fileI = 0; fileI < dir->GetNumberOfFiles(); fileI++)
    {
    vtkStdString subDir(dir->GetFile(fileI));
    if (subDir != "." && subDir != ".." && dir->FileIsDirectory(subDir.c_str()))
      {
      vtkStdString boundaryPath(constantPath + subDir + "/polyMesh/boundary");
      if (vtksys::SystemTools::FileExists(boundaryPath.c_str(), true)
          || vtksys::SystemTools::FileExists((boundaryPath + ".gz").c_str(), true))
        {
        vtkOpenFOAMReaderPrivate *subReader = vtkOpenFOAMReaderPrivate::New();
        subReader->SetupInformation(casePath, subDir, procName, masterReader);
        this->Readers->AddItem(subReader);
        subReader->Delete();
        }
      }
    }
  dir->Delete();
  masterReader->Delete();
  this->Parent->NumberOfReaders += this->Readers->GetNumberOfItems();

  if (this->Parent == this)
    {
    this->CreateCharArrayFromString(this->CasePath, "CasePath", casePath);
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReader::CreateCasePath(vtkStdString &casePath,
    vtkStdString &controlDictPath)
{
#if defined(_WIN32)
  const vtkStdString pathFindSeparator = "/\\", pathSeparator = "\\";
#else
  const vtkStdString pathFindSeparator = "/", pathSeparator = "/";
#endif
  controlDictPath = this->FileName;

  // determine the case directory and path to controlDict
  vtkStdString::size_type pos = controlDictPath.find_last_of(pathFindSeparator);
  if (pos == vtkStdString::npos)
    {
    // if there's no prepending path, prefix with the current directory
    controlDictPath = "." + pathSeparator + controlDictPath;
    pos = 1;
    }
  if (controlDictPath.substr(pos + 1, 11) == "controlDict")
    {
    // remove trailing "/controlDict*"
    casePath = controlDictPath.substr(0, pos - 1);
    if (casePath == ".")
      {
      casePath = ".." + pathSeparator;
      }
    else
      {
      pos = casePath.find_last_of(pathFindSeparator);
      if (pos == vtkStdString::npos)
        {
        casePath = "." + pathSeparator;
        }
      else
        {
        // remove trailing "system" (or any other directory name)
        casePath.erase(pos + 1); // preserve the last "/"
        }
      }
    }
  else
    {
    // if the file is named other than controlDict*, use the directory
    // containing the file as case directory
    casePath = controlDictPath.substr(0, pos + 1);
    controlDictPath = casePath + "system" + pathSeparator + "controlDict";
    }
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReader::AddSelectionNames(vtkDataArraySelection *selections,
    vtkStringArray *objects)
{
  objects->Squeeze();
  vtkSortDataArray::Sort(objects);
  for (int nameI = 0; nameI < objects->GetNumberOfValues(); nameI++)
    {
    selections->AddArray(objects->GetValue(nameI).c_str());
    }
  objects->Delete();
}

//-----------------------------------------------------------------------------
bool vtkOpenFOAMReader::SetTimeValue(const double timeValue)
{
  bool modified = false;
  vtkOpenFOAMReaderPrivate *reader;
  this->Readers->InitTraversal();
  while ((reader
      = vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetNextItemAsObject()))
      != NULL)
    {
    const unsigned long mTime = reader->GetMTime();
    reader->SetTimeValue(timeValue);
    if (reader->GetMTime() != mTime)
      {
      modified = true;
      }
    }
  return modified;
}

//-----------------------------------------------------------------------------
vtkDoubleArray *vtkOpenFOAMReader::GetTimeValues()
{
  if (this->Readers->GetNumberOfItems() <= 0)
    {
    return NULL;
    }
  vtkOpenFOAMReaderPrivate *reader =
      vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetItemAsObject(0));
  return reader != NULL ? reader->GetTimeValues() : NULL;
}

//-----------------------------------------------------------------------------
int vtkOpenFOAMReader::MakeMetaDataAtTimeStep(const bool listNextTimeStep)
{
  vtkStringArray *cellSelectionNames = vtkStringArray::New();
  vtkStringArray *pointSelectionNames = vtkStringArray::New();
  vtkStringArray *lagrangianSelectionNames = vtkStringArray::New();
  int ret = 1;
  vtkOpenFOAMReaderPrivate *reader;
  this->Readers->InitTraversal();
  while ((reader
      = vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetNextItemAsObject()))
      != NULL)
    {
    ret *= reader->MakeMetaDataAtTimeStep(cellSelectionNames,
        pointSelectionNames, lagrangianSelectionNames, listNextTimeStep);
    }
  this->AddSelectionNames(this->Parent->CellDataArraySelection,
      cellSelectionNames);
  this->AddSelectionNames(this->Parent->PointDataArraySelection,
      pointSelectionNames);
  this->AddSelectionNames(this->Parent->LagrangianDataArraySelection,
      lagrangianSelectionNames);

  return ret;
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReader::CreateCharArrayFromString(vtkCharArray *array,
    const char *name, vtkStdString &string)
{
  array->Initialize();
  array->SetName(name);
  const size_t len = string.length();
  char *ptr = array->WritePointer(0, static_cast<vtkIdType>(len + 1));
  memcpy(ptr, string.c_str(), len);
  ptr[len] = '\0';
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReader::UpdateStatus()
{
  // update selection MTimes
  this->PatchSelectionMTimeOld = this->PatchDataArraySelection->GetMTime();
  this->CellSelectionMTimeOld = this->CellDataArraySelection->GetMTime();
  this->PointSelectionMTimeOld = this->PointDataArraySelection->GetMTime();
  this->LagrangianSelectionMTimeOld
      = this->LagrangianDataArraySelection->GetMTime();
  this->CreateCellToPointOld = this->CreateCellToPoint;
  this->DecomposePolyhedraOld = this->DecomposePolyhedra;
  this->PositionsIsIn13FormatOld = this->PositionsIsIn13Format;
  this->ReadZonesOld = this->ReadZones;
  this->ListTimeStepsByControlDictOld = this->ListTimeStepsByControlDict;
  this->AddDimensionsToArrayNamesOld = this->AddDimensionsToArrayNames;
}

//-----------------------------------------------------------------------------
void vtkOpenFOAMReader::UpdateProgress(double amount)
{
  this->vtkAlgorithm::UpdateProgress((static_cast<double>(this->Parent->CurrentReaderIndex)
      + amount) / static_cast<double>(this->Parent->NumberOfReaders));
}
