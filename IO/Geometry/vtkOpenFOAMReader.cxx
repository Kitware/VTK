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
// Thanks to Terry Jordan (terry.jordan@sa.netl.doe.gov) of SAIC
// at the National Energy Technology Laboratory who originally developed this class.
//
// --------
// Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp)
// provided the major bulk of improvements (rewrite) that made the reader
// truly functional.
//
// Token-based FoamFile format lexer/parser,
// performance/stability/compatibility enhancements, gzipped file
// support, lagrangian field support, variable timestep support,
// builtin cell-to-point filter, pointField support, polyhedron
// decomposition support, multiregion support,
// parallelization support for
// decomposed cases in conjunction with vtkPOpenFOAMReader etc.
//
// --------
// Philippose Rajan (sarith@rocketmail.com)
// provided various adjustments
//
// * GUI Based selection of mesh regions and fields available in the case
// * Minor bug fixes / Strict memory allocation checks
// * Minor performance enhancements
//
// --------
// Mark Olesen (OpenCFD Ltd.) www.openfoam.com
// provided various bugfixes, improvements, cleanup
//
// ---------------------------------------------------------------------------
//
// Bugs or support questions should be addressed to the discourse forum
// https://discourse.paraview.org/ and/or KitWare
//
// ---------------------------------------------------------------------------
// OpenFOAM mesh files (serial), located under constant/polyMesh/
//
// https://www.openfoam.com/documentation/user-guide/mesh-description.php
//
// - points (type: vectorField)
//   * x,y,z values
//
// - faces (type: faceList or faceCompactList)
//   * a list of list of nodes.
//     Either stored as such, or as offsets and content
//
// - owner (type: labelList)
//   * the 'owner' cell for each face.
//
// - neighbour (type: labelList)
//   * for 'neighbour' cell for each internal face.
//
// - boundary (type: polyBoundaryMesh)
//   * list of patches with startFace/nFaces for external boundary regions
//
// The owner cell always has a lower number than neighbour.
// The face points outwards from owner to neighbour.
//
// To construct the internal (volume) mesh
// - require points, faces, owner/neighbour.
//   Construct cells from owner/neighbour + faces.
//
// To construct the boundary mesh
// - require points, faces.
//   The owners from the boundary faces are size (owner_list - neighbour_list).
//
// To construct cell zones, cell sets
// - similar requirements as internal mesh
//
// To construct face zones, face sets
// - require points, faces, owners
//
// To construct point zones, point sets
// - require points only
//
// ---------------------------------------------------------------------------
// Patch/mesh selection naming
// single region:
// - internalMesh
// - group/...
// - patch/...
// - lagrangian/...
//
// multi-region:
// - /regionName/internalMesh
// - /regionName/group/...
// - /regionName/patch/...
// - /regionName/lagrangian/...
//
// Prefixed with "/regionName/" to provide unambiguous names. For example,
// - "lagrangian/..."  (lagrangian on default region)
// - "/lagrangian/..." (mesh region called 'lagrangian' - silly, but accept)
//
// ---------------------------------------------------------------------------

// Hijack the CRC routine of zlib to omit CRC check for gzipped files
// (on OSes other than Windows where the mechanism doesn't work due
// to pre-bound DLL symbols) if set to 1, or not (set to 0). Affects
// performance by about 3% - 4%.
#define VTK_FOAMFILE_OMIT_CRCCHECK 0

// The input/output buffer sizes for zlib in bytes.
#define VTK_FOAMFILE_INBUFSIZE (16384)
#define VTK_FOAMFILE_OUTBUFSIZE (131072)
#define VTK_FOAMFILE_INCLUDE_STACK_SIZE (10)

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS 1
// No strtoll on msvc:
#define strtoll _strtoi64
#endif

#if VTK_FOAMFILE_OMIT_CRCCHECK
#define ZLIB_INTERNAL
#endif

// For possible future extension of linehead-aware directives
#define VTK_FOAMFILE_RECOGNIZE_LINEHEAD 0

// List time directories according to system/controlDict
#define VTK_FOAMFILE_LIST_TIMEDIRS_BY_CONTROLDICT 1

// Ignore things like 'U_0' restart files.
// This could also be made part of the GUI properties
#define VTK_FOAMFILE_IGNORE_FIELD_RESTART 1

// Support for finiteArea
#define VTK_FOAMFILE_FINITE_AREA 0

// Support extra decomposition of polyhedral cells
#define VTK_FOAMFILE_DECOMPOSE_POLYHEDRA 1

//------------------------------------------------------------------------------
// Developer option to debug the reader states
#define VTK_FOAMFILE_DEBUG 0

// Similar to vtkErrorMacro etc.
#if VTK_FOAMFILE_DEBUG
#define vtkFoamDebug(x)                                                                            \
  do                                                                                               \
  {                                                                                                \
    std::cerr << "" x;                                                                             \
  } while (false)
#else
#define vtkFoamDebug(x)                                                                            \
  do                                                                                               \
  {                                                                                                \
  } while (false)
#endif // VTK_FOAMFILE_DEBUG

//------------------------------------------------------------------------------

#include "vtkOpenFOAMReader.h"

#include "vtk_zlib.h"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtkAssume.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCollection.h"
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
#include "vtkPolyhedron.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkSmartPointer.h"
#include "vtkSortDataArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeTraits.h"
#include "vtkTypeUInt8Array.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"
#include "vtkWedge.h"

#if !(defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__))
#include <pwd.h> // For getpwnam(), getpwuid()
#include <sys/types.h>
#include <unistd.h> // For getuid()
#endif

#include <algorithm>
#include <cctype> // For isalnum(), isdigit(), isspace()
#include <cmath>  // For abs()
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if VTK_FOAMFILE_OMIT_CRCCHECK
uLong ZEXPORT crc32(uLong, const Bytef*, uInt)
{
  return 0;
}
#endif

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

vtkStandardNewMacro(vtkOpenFOAMReader);

#if VTK_FOAMFILE_FINITE_AREA
// The name for finiteArea mesh
static constexpr const char* const NAME_AREAMESH = "areaMesh";
#endif

// The name for finiteVolume internal mesh (unzoned)
static constexpr const char* const NAME_INTERNALMESH = "internalMesh";

// Index is "constant" time
static constexpr int TIMEINDEX_CONSTANT = -1;

// Index has not been visited
static constexpr int TIMEINDEX_UNVISITED = -2;

//------------------------------------------------------------------------------
// Local Functions

namespace
{

// True if data array uses 64-bit representation for its storage
bool Is64BitArray(const vtkDataArray* array)
{
  return (array && array->GetElementComponentSize() == 8);
}

// Given a data array and a flag indicating whether 64 bit labels are used,
// lookup and return a single element in the array. The data array must
// be either a vtkTypeInt32Array or vtkTypeInt64Array.
vtkTypeInt64 GetLabelValue(const vtkDataArray* array, vtkIdType idx, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    vtkTypeInt64 result =
      static_cast<vtkTypeInt64>(static_cast<const vtkTypeInt32Array*>(array)->GetValue(idx));
    assert(result >= -1); // some arrays store -1 == 'uninitialized'.
    return result;
  }
  else
  {
    vtkTypeInt64 result = static_cast<const vtkTypeInt64Array*>(array)->GetValue(idx);
    assert(result >= -1); // some arrays store -1 == 'uninitialized'.
    return result;
  }
}

// Setter analogous to the above getter.
void SetLabelValue(vtkDataArray* array, vtkIdType idx, vtkTypeInt64 value, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    assert(static_cast<vtkTypeInt32>(value) >= 0);
    static_cast<vtkTypeInt32Array*>(array)->SetValue(idx, static_cast<vtkTypeInt32>(value));
  }
  else
  {
    assert(value >= 0);
    static_cast<vtkTypeInt64Array*>(array)->SetValue(idx, value);
  }
}

// Another helper for appending an id to a list
void AppendLabelValue(vtkDataArray* array, vtkTypeInt64 val, bool use64BitLabels)
{
  if (!use64BitLabels)
  {
    assert(static_cast<vtkTypeInt32>(val) >= 0);
    static_cast<vtkTypeInt32Array*>(array)->InsertNextValue(static_cast<vtkTypeInt32>(val));
  }
  else
  {
    assert(val >= 0);
    static_cast<vtkTypeInt64Array*>(array)->InsertNextValue(val);
  }
}

// Append unique string to list
void appendUniq(vtkStringArray* list, vtkStringArray* items)
{
  for (int i = 0; i < items->GetNumberOfTuples(); ++i)
  {
    std::string& str = items->GetValue(i);
    if (list->LookupValue(str) == -1)
    {
      list->InsertNextValue(str);
    }
  }
}

// Tuple remapping for symmTensor ordering
// OpenFOAM [XX XY XZ YY YZ ZZ]
// VTK uses [XX YY ZZ XY YZ XZ]
template <typename T>
void remapFoamSymmTensor(T data[])
{
  std::swap(data[1], data[3]); // swap XY <-> YY
  std::swap(data[2], data[5]); // swap XZ <-> ZZ
}

// Generic tuple remapping is a no-op
template <bool symmTensor, typename T>
void remapFoamTuple(T[])
{
}

// Remapping for symmTensor (float)
template <>
void remapFoamTuple<true>(float data[])
{
  ::remapFoamSymmTensor(data);
}

// Remapping for symmTensor (double)
template <>
void remapFoamTuple<true>(double data[])
{
  ::remapFoamSymmTensor(data);
}

} // End anonymous namespace

//------------------------------------------------------------------------------
// Forward Declarations

struct vtkFoamDict;
struct vtkFoamEntry;
struct vtkFoamEntryValue;
struct vtkFoamFile;
struct vtkFoamIOobject;
struct vtkFoamToken;

//------------------------------------------------------------------------------
// class vtkFoamError
// for exception-carrying object or general place to collect errors
struct vtkFoamError : public std::string
{
  vtkFoamError& operator<<(const std::string& str)
  {
    this->std::string::operator+=(str);
    return *this;
  }
  vtkFoamError& operator<<(const char* str)
  {
    this->std::string::operator+=(str);
    return *this;
  }
  template <class T>
  vtkFoamError& operator<<(const T& val)
  {
    std::ostringstream os;
    os << val;
    this->std::string::operator+=(os.str());
    return *this;
  }
};

//------------------------------------------------------------------------------
// Some storage containers

// Manage a list of pointers
template <typename T>
struct vtkFoamPtrList : public std::vector<T*>
{
private:
  typedef std::vector<T*> Superclass;

  // Plain 'delete' each entry
  void DeleteAll()
  {
    for (T* ptr : *this)
    {
      delete ptr;
    }
  }

public:
  // Inherit all constructors
  using std::vector<T*>::vector;

  // Default construct
  vtkFoamPtrList() = default;

  // No copy construct/assignment
  vtkFoamPtrList(const vtkFoamPtrList&) = delete;
  void operator=(const vtkFoamPtrList&) = delete;

  // Destructor - delete each entry
  ~vtkFoamPtrList() { DeleteAll(); }

  // Remove top element, deleting its pointer
  void remove_back()
  {
    if (!Superclass::empty())
    {
      delete Superclass::back();
      Superclass::pop_back();
    }
  }

  // Clear list, delete all elements
  void clear()
  {
    DeleteAll();
    Superclass::clear();
  }
};

// Manage a list of vtkDataObject pointers
template <typename ObjectT>
struct vtkFoamDataArrayVector : public std::vector<ObjectT*>
{
private:
  typedef std::vector<ObjectT*> Superclass;

  // Invoke vtkDataObject Delete() on each (non-null) entry
  void DeleteAll()
  {
    for (ObjectT* ptr : *this)
    {
      if (ptr)
      {
        ptr->Delete();
      }
    }
  }

public:
  // Destructor - invoke vtkDataObject Delete() on each entry
  ~vtkFoamDataArrayVector() { DeleteAll(); }

  // Remove top element, invoking vtkDataObject Delete() on it
  void remove_back()
  {
    if (!Superclass::empty())
    {
      ObjectT* ptr = Superclass::back();
      if (ptr)
      {
        ptr->Delete();
      }
      Superclass::pop_back();
    }
  }

  // Clear list, invoking vtkDataObject Delete() on each element
  void clear()
  {
    DeleteAll();
    Superclass::clear();
  }
};

// Forward Declarations
typedef vtkFoamDataArrayVector<vtkDataArray> vtkFoamLabelArrayVector;

//------------------------------------------------------------------------------
// A std::vector-like data structure where the data
// lies on the stack. If the requested size in the
// resize method is larger than N, the class allocates
// the array on the heap.
//
// Unlike std::vector, the array is not default initialized
// and behaves more like std::array in that manner.
//
// Since this simple structure is largely used for scratch space,
// it allocates on growth, but not on shrinking.
// It has both copying and non-copying reserve/resize methods.
template <typename T, size_t N = 2 * 64 / sizeof(T)>
struct vtkFoamStackVector
{
  typedef T value_type;

  /**
   * Default construct, zero length and default capacity
   */
  vtkFoamStackVector() = default;

  /**
   * Construct with specified length
   */
  explicit vtkFoamStackVector(std::size_t len) { this->fast_resize(len); }

  ~vtkFoamStackVector()
  {
    if (ptr != stck)
    {
      delete[] ptr;
    }
  }

  bool empty() const noexcept { return !size_; }
  std::size_t size() const noexcept { return size_; }
  std::size_t capacity() const noexcept { return capacity_; }

  T* data() noexcept { return ptr; }
  const T* data() const noexcept { return ptr; }

  T* begin() noexcept { return ptr; }
  T* end() noexcept { return (ptr + size_); }
  const T* begin() const noexcept { return ptr; }
  const T* end() const noexcept { return (ptr + size_); }

  T& operator[](std::size_t pos) { return ptr[pos]; }
  const T& operator[](std::size_t pos) const { return ptr[pos]; }

  // Reserve space, retaining old values on growth. Uses doubling strategy.
  void copy_reserve(std::size_t len) { _reserve(len, false); }

  // Resize, retaining old values on growth. Uses doubling strategy.
  void copy_resize(std::size_t len)
  {
    _reserve(len, false);
    size_ = len;
  }

  // Faster reserve space, may discard old values on growth. Uses doubling strategy.
  void fast_reserve(std::size_t len) { _reserve(len, true); }

  // Faster resize, may discard old values on growth. Uses doubling strategy.
  void fast_resize(std::size_t len)
  {
    _reserve(len, true);
    size_ = len;
  }

private:
  T stck[N];
  T* ptr = stck;
  std::size_t capacity_ = N;
  std::size_t size_ = 0;

  // Reserve space, using doubling strategy.
  // Fast (non-copying) or copy/move old values on growth.
  void _reserve(std::size_t len, bool fast)
  {
    if (capacity_ < len)
    {
      while (capacity_ < len)
      {
        capacity_ *= 2;
      }
      if (fast)
      {
        if (ptr != stck)
        {
          delete[] ptr;
        }
        ptr = new T[capacity_];
      }
      else
      {
        T* old = ptr;
        ptr = new T[capacity_];
        for (size_t i = 0; i < size_; ++i)
        {
          ptr[i] = std::move(old[i]);
        }
        if (old != stck)
        {
          delete[] old;
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// struct vtkFoamLabelListList - details in the implementation class
struct vtkFoamLabelListList
{
  using CellType = vtkFoamStackVector<vtkTypeInt64>;

  virtual ~vtkFoamLabelListList() = default;

  virtual size_t GetLabelSize() const = 0; // in bytes
  bool IsLabel64() const { return this->GetLabelSize() == 8; }
  virtual vtkIdType GetNumberOfElements() const = 0;
  virtual vtkDataArray* GetOffsetsArray() = 0;
  virtual vtkDataArray* GetDataArray() = 0;

  virtual void ResizeExact(vtkIdType numElem, vtkIdType numValues) = 0;
  virtual void ResizeData(vtkIdType numValues) = 0;

  // Fill offsets with zero
  virtual void ResetOffsets() = 0;

  virtual vtkTypeInt64 GetBeginOffset(vtkIdType i) const = 0;
  virtual vtkTypeInt64 GetEndOffset(vtkIdType i) const = 0;
  virtual vtkIdType GetSize(vtkIdType i) const = 0;
  virtual void SetOffset(vtkIdType i, vtkIdType val) = 0;
  virtual void IncrementOffset(vtkIdType i) = 0;

  // Combine assignment of the new offset and accessing the data
  virtual void* WritePointer(vtkIdType cellId, vtkIdType dataOffset, vtkIdType elemLength) = 0;

  virtual vtkTypeInt64 GetValue(vtkIdType bodyIndex) const = 0;
  virtual void SetValue(vtkIdType bodyIndex, vtkTypeInt64 val) = 0;

  virtual vtkTypeInt64 GetValue(vtkIdType cellId, vtkIdType subIndex) const = 0;
  virtual void SetValue(vtkIdType cellId, vtkIdType subIndex, vtkTypeInt64 val) = 0;

  virtual void InsertValue(vtkIdType bodyIndex, vtkTypeInt64 val) = 0;
  virtual void GetCell(vtkIdType i, CellType& cell) const = 0;
};

//------------------------------------------------------------------------------
// struct vtkFoamLabelListListImpl (implementation for vtkFoamLabelListList)
// This is roughly comparable to an OpenFOAM CompactListList and largely
// mirrors what the new vtkCellArray (2020: VTK_CELL_ARRAY_V2) now does.
// It contains packed data and a table of offsets
//
template <typename ArrayT>
struct vtkFoamLabelListListImpl : public vtkFoamLabelListList
{
private:
  ArrayT* Offsets;
  ArrayT* Data;

public:
  using LabelArrayType = ArrayT;
  using LabelType = typename ArrayT::ValueType;

  // Default construct
  vtkFoamLabelListListImpl()
    : Offsets(LabelArrayType::New())
    , Data(LabelArrayType::New())
  {
  }

  // Construct a shallow copy from base class
  explicit vtkFoamLabelListListImpl(const vtkFoamLabelListList& rhs)
    : Offsets(nullptr)
    , Data(nullptr)
  {
    assert("Require same element representation." && this->IsLabel64() == rhs.IsLabel64());
    const auto& rhsCast = static_cast<const vtkFoamLabelListListImpl<LabelArrayType>&>(rhs);
    this->Offsets = rhsCast.Offsets;
    this->Data = rhsCast.Data;
    this->Offsets->Register(nullptr); // ref count the copy
    this->Data->Register(nullptr);
  }

  vtkFoamLabelListListImpl(const vtkFoamLabelListListImpl<ArrayT>& rhs)
    : Offsets(rhs.Offsets)
    , Data(rhs.Data)
  {
    this->Offsets->Register(nullptr); // ref count the copy
    this->Data->Register(nullptr);
  }

  void operator=(const vtkFoamLabelListListImpl<ArrayT>&) = delete;

  // Destructor
  ~vtkFoamLabelListListImpl() override
  {
    this->Offsets->Delete();
    this->Data->Delete();
  }

  size_t GetLabelSize() const override { return sizeof(LabelType); }
  vtkIdType GetNumberOfElements() const override { return this->Offsets->GetNumberOfTuples() - 1; }
  vtkDataArray* GetOffsetsArray() override { return this->Offsets; }
  vtkDataArray* GetDataArray() override { return this->Data; }

  void ResizeExact(vtkIdType numElem, vtkIdType numValues) override
  {
    this->Offsets->SetNumberOfValues(numElem + 1);
    this->Data->SetNumberOfValues(numValues);
    this->Offsets->SetValue(0, 0);
  }
  void ResizeData(vtkIdType numValues) override { this->Data->Resize(numValues); }
  void ResetOffsets() override { this->Offsets->FillValue(0); }

  vtkTypeInt64 GetBeginOffset(vtkIdType i) const override { return this->Offsets->GetValue(i); }
  vtkTypeInt64 GetEndOffset(vtkIdType i) const override { return this->Offsets->GetValue(i + 1); }
  vtkIdType GetSize(vtkIdType i) const override
  {
    return this->Offsets->GetValue(i + 1) - this->Offsets->GetValue(i);
  }
  void SetOffset(vtkIdType i, vtkIdType val) override
  {
    this->Offsets->SetValue(i, static_cast<LabelType>(val));
  }
  void IncrementOffset(vtkIdType i) override
  {
    this->Offsets->SetValue(i, this->Offsets->GetValue(i) + 1);
  }

  void* WritePointer(vtkIdType cellId, vtkIdType dataOffset, vtkIdType subLength) override
  {
    return this->Data->WritePointer(*(this->Offsets->GetPointer(cellId)) = dataOffset, subLength);
  }

  vtkTypeInt64 GetValue(vtkIdType bodyIndex) const override
  {
    return this->Data->GetValue(bodyIndex);
  }
  void SetValue(vtkIdType bodyIndex, vtkTypeInt64 value) override
  {
    this->Data->SetValue(bodyIndex, static_cast<LabelType>(value));
  }

  vtkTypeInt64 GetValue(vtkIdType cellId, vtkIdType subIndex) const override
  {
    return this->Data->GetValue(this->Offsets->GetValue(cellId) + subIndex);
  }
  void SetValue(vtkIdType cellId, vtkIdType subIndex, vtkTypeInt64 value) override
  {
    this->Data->SetValue(this->Offsets->GetValue(cellId) + subIndex, static_cast<LabelType>(value));
  }

  void InsertValue(vtkIdType bodyIndex, vtkTypeInt64 value) override
  {
    this->Data->InsertValue(bodyIndex, value);
  }

  void GetCell(vtkIdType i, CellType& cell) const override
  {
    auto idx = this->Offsets->GetValue(i);
    const auto last = this->Offsets->GetValue(i + 1);
    cell.fast_resize(last - idx);

    auto outIter = cell.begin();
    while (idx != last)
    {
      *outIter = this->Data->GetValue(idx);
      ++outIter;
      ++idx;
    }
  }
};

// Forward Declarations
typedef vtkFoamLabelListListImpl<vtkTypeInt32Array> vtkFoamLabelListList32;
typedef vtkFoamLabelListListImpl<vtkTypeInt64Array> vtkFoamLabelListList64;

//------------------------------------------------------------------------------
// struct vtkFoamPatch
// A simple struct to hold OpenFOAM boundary patch information extracted
// from polyMesh/boundary. Similar to Foam::polyPatch
struct vtkFoamPatch
{
  // General patch types (fits as vtkTypeInt8)
  enum patchType
  {
    GEOMETRICAL = 0, // symmetryPlane, wedge, cyclic, empty, etc.
    PHYSICAL = 1,    // patch, wall
    PROCESSOR = 2    // processor
  };

  std::string name_;
  vtkIdType index_ = 0;
  vtkIdType start_ = 0;
  vtkIdType size_ = 0;
  vtkIdType offset_ = 0; // The start-face offset into all boundaries
  patchType type_ = patchType::GEOMETRICAL;
  bool owner_ = true; // Patch owner (processor patch)

  // The first patch face
  vtkIdType startFace() const noexcept { return (this->start_); }

  // One beyond the last patch face
  vtkIdType endFace() const noexcept { return (this->start_ + this->size_); }

  // The patch local face (as per OpenFOAM polyPatch)
  vtkIdType whichFace(vtkIdType meshFacei) const { return (meshFacei - this->start_); }
};

//------------------------------------------------------------------------------
// struct vtkFoamBoundaries
// A collection of boundary patches with additional grouping and selection information
struct vtkFoamBoundaries : public std::vector<vtkFoamPatch>
{
  // Collect and forwarding of errors (cannot use vtkErrorMacro here)
  vtkFoamError error_;

  // Patch groups, according to the inGroups keyword
  std::map<std::string, std::vector<vtkIdType>> groups;

  // Active patch groups
  std::unordered_set<std::string> groupActive;

  // Active patch indices, selected directly
  std::unordered_set<vtkIdType> patchActive;

  // Active patch indices, selected by group
  std::unordered_set<vtkIdType> patchActiveByGroup;

  // Reset group and patch selections
  void clearSelections()
  {
    groupActive.clear();
    patchActive.clear();
    patchActiveByGroup.clear();
  }

  // Reset storage and errors, leaves timeName intact
  void clearAll()
  {
    this->clear();
    error_.clear();
    groups.clear();
    this->clearSelections();
  }

  const vtkFoamError& error() const noexcept { return error_; }
  vtkFoamError& error() noexcept { return error_; }

  // The start label of boundary faces in the polyMesh face list.
  // Same as mesh nInternalFaces() if boundaries exist
  vtkIdType startFace() const { return this->empty() ? 0 : this->front().startFace(); }

  // One beyond the last boundary face
  vtkIdType endFace() const { return this->empty() ? 0 : this->back().endFace(); }

  void enablePatch(vtkIdType patchIndex) { patchActive.emplace(patchIndex); }

  void enableGroup(const std::string& groupName)
  {
    auto citer = groups.find(groupName);
    if (citer != groups.end())
    {
      const std::vector<vtkIdType>& patchIndices = citer->second;
      for (const vtkIdType patchIndex : patchIndices)
      {
        patchActiveByGroup.emplace(patchIndex);
      }
    }
  }

  // True if given patch index is active
  bool isActive(vtkIdType patchIndex) const
  {
    return (patchActive.find(patchIndex) != patchActive.end()) ||
      (patchActiveByGroup.find(patchIndex) != patchActiveByGroup.end());
  }

  // Set contents from dictionary
  // Return false on errors
  bool update(const vtkFoamDict& dict);

  // The patch index for a given face label, -1 for internal face or out-of-bounds
  vtkIdType whichPatch(vtkIdType faceIndex) const;
};

//------------------------------------------------------------------------------
// struct vtkFoamZones
// A collection of names id-lists, used for OpenFOAM zones or sets.
// Stored as an unordered map instead of being ordered or a vector of items,
// since any ordering (like zones) will seen during input and managed with the
// VTK block structure.
//
// The idea is to maintain a list of ids (cell,face,point) in a cache that is
// separate from the mesh to allow flexible usage later.
// Also, it does not make any sense to have an entry like "CellId" in the CellData
// since that not only complicates handling, but is also quite misleading when local
// cell ids have been assembled from different processors.
struct vtkFoamZones
{
  // Representation for the zone or set type
  enum zoneType
  {
    UNKNOWN = 0, // placeholder
    POINT = 1,   // pointZone
    FACE = 2,    // faceZone
    CELL = 3     // cellZone
  };

  // Collect and forwarding of errors (cannot use vtkErrorMacro here)
  vtkFoamError error_;

  // The {cell,face,point}Labels per zone
  std::unordered_map<std::string, vtkSmartPointer<vtkIdList>> zones_;

  // The zone type
  zoneType type_ = zoneType::UNKNOWN;

  // If zone map ids have content
  bool empty() const { return zones_.empty(); }

  // Reset storage and errors
  void clearAll()
  {
    error_.clear();
    zones_.clear();
  }

  // Clear and reset the zone type
  void reset(enum zoneType ztype)
  {
    this->clearAll();
    type_ = ztype;
  }

  const vtkFoamError& error() const noexcept { return error_; }
  vtkFoamError& error() noexcept { return error_; }

  // Find zone by name and return list of ids or nullptr on failure
  vtkIdList* findZone(const std::string& zoneName)
  {
    auto iter = zones_.find(zoneName);
    if (iter != zones_.end())
    {
      return iter->second;
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Simple handling of common OpenFOAM data types
struct vtkFoamTypes
{
  // Primitive types, with nComponents encoded in lower 4 bits
  enum dataType
  {
    NO_TYPE = 0,
    SCALAR_TYPE = 1,
    VECTOR_TYPE = 3,
    SYMM_TENSOR_TYPE = 6,
    TENSOR_TYPE = 9,
    // Single-component types, but disambiguate from SCALAR_TYPE
    BOOL_TYPE = (0x10 | SCALAR_TYPE),
    LABEL_TYPE = (0x20 | SCALAR_TYPE),
    SPH_TENSOR_TYPE = (0x30 | SCALAR_TYPE)
  };

  // The number of data components
  static int GetNumberOfComponents(const dataType dtype) noexcept { return (dtype & 0xF); }

  static bool IsGood(dataType dtype) noexcept { return dtype != NO_TYPE; }
  static bool IsBool(dataType dtype) noexcept { return dtype == BOOL_TYPE; }
  static bool IsLabel(dataType dtype) noexcept { return dtype == LABEL_TYPE; }
  static bool IsScalar(dataType dtype) noexcept { return dtype == SCALAR_TYPE; }
  static bool IsNumeric(dataType dtype) noexcept { return IsLabel(dtype) || IsScalar(dtype); }

  // Is a VectorSpace type?
  static bool IsVectorSpace(dataType dtype) noexcept
  {
    return GetNumberOfComponents(dtype) > 1 || dtype == SPH_TENSOR_TYPE;
  }

  // Parse things like "scalarField" or "ScalarField" -> SCALAR_TYPE etc.
  // Ignore case on first letter (at pos), which makes it convenient for "volScalarField" too.
  static dataType FieldToEnum(const std::string& fieldTypeName, size_t pos = 0);

  // Handle "List<scalar>" -> SCALAR_TYPE etc.
  static dataType ListToEnum(const std::string& listTypeName);

private:
  // Implementation for FieldToEnum, ListToEnum
  static dataType ToEnumImpl(const std::string& str, size_t pos, size_t len, bool ignoreCase);
};

//------------------------------------------------------------------------------
// class vtkOpenFOAMReaderPrivate
// the reader core of vtkOpenFOAMReader
class vtkOpenFOAMReaderPrivate : public vtkObject
{
public:
  // Use sparingly
  friend class vtkOpenFOAMReader;

  static vtkOpenFOAMReaderPrivate* New();
  vtkTypeMacro(vtkOpenFOAMReaderPrivate, vtkObject);

  vtkGetMacro(TimeStep, int);
  vtkSetMacro(TimeStep, int);

  double GetTimeValue() const;
  void SetTimeValue(double requestedTime);

  vtkStringArray* GetTimeNames() { return this->TimeNames; }
  vtkDoubleArray* GetTimeValues() { return this->TimeValues; }

  // Print some time information (names, current time-step)
  void PrintTimes(std::ostream& os, vtkIndent indent, bool full = false) const;

  bool HasPolyMesh() const noexcept { return !this->PolyMeshTimeIndexFaces.empty(); }

  const std::string& GetRegionName() const noexcept { return this->RegionName; }

  vtkStringArray* GetLagrangianPaths() { return this->LagrangianPaths; }

  // Read mesh/fields and create dataset
  int RequestData(vtkMultiBlockDataSet* output);
  int MakeMetaDataAtTimeStep(vtkStringArray*, vtkStringArray*, vtkStringArray*, bool);

  // Gather time instances information and create cache for mesh times
  bool MakeInformationVector(const std::string& casePath, const std::string& controlDictPath,
    const std::string& procName, vtkOpenFOAMReader* parent, bool requirePolyMesh = true);

  // Use given time instances information and create cache for mesh times
  bool MakeInformationVector(const std::string& casePath, const std::string& procName,
    vtkOpenFOAMReader* parent, vtkStringArray* timeNames, vtkDoubleArray* timeValues,
    bool requirePolyMesh = true);

  // Copy time instances information and create cache for mesh times
  void SetupInformation(const std::string& casePath, const std::string& regionName,
    const std::string& procName, vtkOpenFOAMReaderPrivate* master, bool requirePolyMesh = true);

private:
  vtkOpenFOAMReader* Parent;

  std::string CasePath;      // The full path to the case - includes trailing '/'
  std::string RegionName;    // Region name. Empty for default region
  std::string ProcessorName; // Processor subdirectory. Empty for serial case

  // Time information
  vtkDoubleArray* TimeValues; // Time values
  vtkStringArray* TimeNames;  // Directory names

  // Topology indices into TimeValues, TimeName
  std::vector<vtkIdType> PolyMeshTimeIndexPoints;
  std::vector<vtkIdType> PolyMeshTimeIndexFaces;

  // Indices into TimeValues, TimeName
  int TimeStep;
  int TimeStepOld;

  // Topology time index, driven by PolyMeshTimeIndexFaces
  int TopologyTimeIndex;

  int InternalMeshSelectionStatus;
  int InternalMeshSelectionStatusOld;

  // filenames / directories
  vtkStringArray* VolFieldFiles;
  vtkStringArray* DimFieldFiles;
  vtkStringArray* AreaFieldFiles;
  vtkStringArray* PointFieldFiles;
  vtkStringArray* LagrangianFieldFiles;

  // The cloud paths (region-local)
  vtkNew<vtkStringArray> LagrangianPaths;

  // Mesh dimensions and construction information
  vtkIdType NumPoints;
  vtkIdType NumInternalFaces;
  vtkIdType NumFaces;
  vtkIdType NumCells;

  // The face owner, neighbour (labelList)
  vtkDataArray* FaceOwner;
  vtkDataArray* FaceNeigh;

  // For cell-to-point interpolation
  vtkPolyData* AllBoundaries;
  vtkDataArray* AllBoundariesPointMap;
  vtkDataArray* InternalPoints;

  // For caching mesh
  vtkUnstructuredGrid* InternalMesh;
  vtkMultiBlockDataSet* BoundaryMesh;
  vtkFoamLabelArrayVector* BoundaryPointMap;
  vtkFoamBoundaries BoundaryDict;

  // Zones
  vtkFoamZones cellZoneMap;
  vtkFoamZones faceZoneMap;
  vtkFoamZones pointZoneMap;

  vtkMultiBlockDataSet* CellZoneMesh;
  vtkMultiBlockDataSet* FaceZoneMesh;
  vtkMultiBlockDataSet* PointZoneMesh;

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  // For polyhedral decomposition
  vtkIdType NumTotalAdditionalCells;
  vtkIdTypeArray* AdditionalCellIds;
  vtkIntArray* NumAdditionalCells;
  vtkFoamLabelArrayVector* AdditionalCellPoints;
#endif

#if VTK_FOAMFILE_FINITE_AREA
  vtkFoamZones areaMeshMap;
  vtkPolyData* AreaMesh;
#endif

  // Constructor and destructor are kept private
  vtkOpenFOAMReaderPrivate();
  ~vtkOpenFOAMReaderPrivate() override;

  vtkOpenFOAMReaderPrivate(const vtkOpenFOAMReaderPrivate&) = delete;
  void operator=(const vtkOpenFOAMReaderPrivate&) = delete;

  // Clear mesh construction
  void ClearInternalMeshes();
  void ClearBoundaryMeshes();
  void ClearZoneMeshes();
  void ClearAreaMeshes();
  void ClearMeshes();

  // The subdirectory for a region. Eg, "/solid". Empty for default region.
  std::string RegionPath() const
  {
    if (this->RegionName.empty())
    {
      return "";
    }
    return ("/" + this->RegionName);
  }

  // Prefix display qualifier for a region. Eg, "/solid/". Empty for default region.
  std::string RegionPrefix() const
  {
    if (this->RegionName.empty())
    {
      return "";
    }
    return ("/" + this->RegionName + "/");
  }

  // Test if display (selection) name matches the current region.
  // See RegionPrefix() comments
  bool IsDisplayRegion(const std::string& displayName) const
  {
    if (this->RegionName.empty())
    {
      return (displayName[0] != '/');
    }
    else if (displayName[0] != '/')
    {
      return false;
    }
    // Match "/regionName/..."
    const auto slash1 = displayName.find('/', 1);
    return (slash1 != std::string::npos) &&
      (displayName.compare(1, slash1 - 1, this->RegionName) == 0);
  }

  // The timeName for the given index, with special handling for "constant" time directory
  std::string TimePath(int timeIndex) const
  {
    if (timeIndex < 0)
    {
      return this->CasePath + "constant";
    }
    return this->CasePath + this->TimeNames->GetValue(timeIndex);
  }
  std::string CurrentTimePath() const { return this->TimePath(this->TimeStep); }

  // TimePath + region
  std::string TimeRegionPath(int timeIndex) const
  {
    return this->TimePath(timeIndex) + this->RegionPath();
  }

  std::string CurrentTimeRegionPath() const { return this->TimeRegionPath(this->TimeStep); }

  std::string CurrentTimeRegionPath(const std::vector<vtkIdType>& indexer) const
  {
    return this->TimeRegionPath(indexer[this->TimeStep]);
  }

#if VTK_FOAMFILE_DEBUG
  void PrintMeshTimes(const char* name, const std::vector<vtkIdType>&) const; // For debugging
#endif

  // Search time directories for mesh
  void PopulateMeshTimeIndices();

  void AddFieldName(
    const std::string& fieldName, const std::string& fieldType, bool isLagrangian = false);
  // Search a time directory for field objects
  void GetFieldNames(const std::string&, bool isLagrangian = false);
  void SortFieldFiles(vtkStringArray* selections, vtkStringArray* files);
  void LocateLagrangianClouds(const std::string& timePath);

#if VTK_FOAMFILE_LIST_TIMEDIRS_BY_CONTROLDICT
  // List time directories according to system/controlDict
  vtkFoamError ListTimeDirectoriesByControlDict(const std::string& controlDictPath);
#endif

  // List time directories by searching in a case directory
  bool ListTimeDirectoriesByInstances();

  // Read polyMesh/points (vectorField)
  vtkSmartPointer<vtkFloatArray> ReadPointsFile(const std::string& timeRegionDir);

  // Read polyMesh/faces (faceCompactList or faceList)
  std::unique_ptr<vtkFoamLabelListList> ReadFacesFile(const std::string& timeRegionDir);

  // Read polyMesh/{owner,neighbour}, check overall number of faces.
  bool ReadOwnerNeighbourFiles(const std::string& timeRegionDir);

  // Create meshCells from owner/neighbour information
  std::unique_ptr<vtkFoamLabelListList> CreateCellFaces();

  bool CheckFaceList(const vtkFoamLabelListList& faces);

  // Create volume mesh
  void InsertCellsToGrid(vtkUnstructuredGrid*, std::unique_ptr<vtkFoamLabelListList>& meshCellsPtr,
    const vtkFoamLabelListList& meshFaces, vtkIdList* cellLabels = nullptr
#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
    ,
    vtkIdTypeArray* additionalCellIds = nullptr, vtkFloatArray* pointArray = nullptr
#endif
  );

  vtkUnstructuredGrid* MakeInternalMesh(std::unique_ptr<vtkFoamLabelListList>& meshCellsPtr,
    const vtkFoamLabelListList& meshFaces, vtkFloatArray* pointArray);

  void InsertFacesToGrid(vtkPolyData*, const vtkFoamLabelListList& meshFaces, vtkIdType startFace,
    vtkIdType endFace, vtkIdList* faceLabels = nullptr, vtkDataArray* pointMap = nullptr,
    bool isLookupValue = false);

  vtkMultiBlockDataSet* MakeBoundaryMesh(
    const vtkFoamLabelListList& meshFaces, vtkFloatArray* pointArray);

  // Move additional points for decomposed cells
  bool MoveInternalMesh(vtkUnstructuredGrid*, vtkFloatArray*);
  bool MoveBoundaryMesh(vtkMultiBlockDataSet*, vtkFloatArray*);

  // cell-to-point interpolator
  void InterpolateCellToPoint(
    vtkFloatArray*, vtkFloatArray*, vtkPointSet*, vtkDataArray*, vtkTypeInt64);

  // Convert OpenFOAM dimension array to string
  std::string ConstructDimensions(const vtkFoamDict& dict) const;

  // read and create cell/point fields
  bool ReadFieldFile(vtkFoamIOobject& io, vtkFoamDict& dict, const std::string& varName,
    const vtkDataArraySelection* selection);
  vtkSmartPointer<vtkFloatArray> FillField(vtkFoamEntry& entry, vtkIdType nElements,
    const vtkFoamIOobject& io, vtkFoamTypes::dataType fieldDataType);
  void GetVolFieldAtTimeStep(const std::string& varName, bool isInternalField = false);
  void GetPointFieldAtTimeStep(const std::string& varName);

#if VTK_FOAMFILE_FINITE_AREA
  void GetAreaFieldAtTimeStep(const std::string& varName);
#endif

  // Create lagrangian mesh/fields
  vtkMultiBlockDataSet* MakeLagrangianMesh();

  // Read specified file (typeName) from polyMesh directory, using faces instance
  std::unique_ptr<vtkFoamDict> GetPolyMeshFile(const std::string& typeName, bool mandatory);

  // Create (cell|face|point) zones
  bool GetCellZoneMesh(vtkMultiBlockDataSet* zoneMesh,
    std::unique_ptr<vtkFoamLabelListList>& meshCellsPtr, const vtkFoamLabelListList& meshFaces,
    vtkPoints*);
  bool GetFaceZoneMesh(
    vtkMultiBlockDataSet* zoneMesh, const vtkFoamLabelListList& meshFaces, vtkPoints*);
  bool GetPointZoneMesh(vtkMultiBlockDataSet* zoneMesh, vtkPoints*);

#if VTK_FOAMFILE_FINITE_AREA
  // Mechanism for finiteArea mesh is similar to faceZone
  bool GetAreaMesh(vtkPolyData* areaMesh, const vtkFoamLabelListList& meshFaces, vtkPoints*);
#endif
};

vtkStandardNewMacro(vtkOpenFOAMReaderPrivate);

//------------------------------------------------------------------------------
// Local Functions

namespace
{

// Set named block
void SetBlock(vtkMultiBlockDataSet* parent, unsigned int blockIndex, vtkDataObject* block,
  const std::string& name)
{
  parent->SetBlock(blockIndex, block);
  parent->GetMetaData(blockIndex)->Set(vtkCompositeDataSet::NAME(), name.c_str());
}

// Append named block
void AppendBlock(vtkMultiBlockDataSet* parent, vtkDataObject* block, const std::string& name)
{
  ::SetBlock(parent, parent->GetNumberOfBlocks(), block, name);
}

// Set array name and fieldData attributes
// The optional suffix is for dimensions etc
void AddArrayToFieldData(vtkDataSetAttributes* fieldData, vtkDataArray* array,
  const std::string& name, const std::string& suffix = "")
{
  if (suffix.empty())
  {
    array->SetName(name.c_str());
  }
  else
  {
    array->SetName((name + suffix).c_str());
  }

  if (array->GetNumberOfComponents() == 1 && name == "p")
  {
    fieldData->SetScalars(array);
  }
  else if (array->GetNumberOfComponents() == 3 && name == "U")
  {
    fieldData->SetVectors(array);
  }
  else
  {
    fieldData->AddArray(array);
  }
}

} // End anonymous namespace

//------------------------------------------------------------------------------
// Simple handling of common OpenFOAM data types

// Low-level implementation
vtkFoamTypes::dataType vtkFoamTypes::ToEnumImpl(
  const std::string& str, size_t pos, size_t last, bool ignoreCase)
{
  vtkFoamTypes::dataType dtype(vtkFoamTypes::NO_TYPE);

  char firstChar = str[pos];
  if (ignoreCase)
  {
    firstChar = std::tolower(firstChar);
  }
  ++pos; // First character handled separately (for ignoring case)

  size_t len = std::string::npos;
  if (last != std::string::npos)
  {
    if (last > pos)
    {
      len = last - pos;
    }
    else
    {
      // Caught bad input
      firstChar = '\0';
    }
  }

  switch (firstChar)
  {
    case '\0':
    {
      break;
    }

    case 'b':
    {
      if (str.compare(pos, len, "ool") == 0)
      {
        // (Bool | bool)
        dtype = vtkFoamTypes::BOOL_TYPE;
      }
      break;
    }

    case 'l':
    {
      if (str.compare(pos, len, "abel") == 0)
      {
        // (Label | label)
        dtype = vtkFoamTypes::LABEL_TYPE;
      }
      break;
    }

    case 's':
    {
      if (str.compare(pos, len, "calar") == 0)
      {
        // (Scalar | scalar)
        dtype = vtkFoamTypes::SCALAR_TYPE;
      }
      else if (str.compare(pos, len, "phericalTensor") == 0)
      {
        // (SphericalTensor | sphericalTensor)
        dtype = vtkFoamTypes::SPH_TENSOR_TYPE;
      }
      else if (str.compare(pos, len, "ymmTensor") == 0)
      {
        // (SymmTensor | symmTensor)
        dtype = vtkFoamTypes::SYMM_TENSOR_TYPE;
      }
      break;
    }

    case 't':
    {
      if (str.compare(pos, len, "ensor") == 0)
      {
        // (Tensor | tensor)
        dtype = vtkFoamTypes::TENSOR_TYPE;
      }
      break;
    }

    case 'v':
    {
      if (str.compare(pos, len, "ector") == 0)
      {
        // (Vector | vector)
        dtype = vtkFoamTypes::VECTOR_TYPE;
      }
      break;
    }
  }

  return dtype;
}

// Fields: expects scalarField, volScalarField etc.
vtkFoamTypes::dataType vtkFoamTypes::FieldToEnum(const std::string& fieldTypeName, size_t pos)
{
  // With ignoreCase
  return vtkFoamTypes::ToEnumImpl(fieldTypeName, pos, fieldTypeName.find("Field", pos), true);
}

// Lists: expects "List<scalar>", "List<vector>" etc.
vtkFoamTypes::dataType vtkFoamTypes::ListToEnum(const std::string& listTypeName)
{
  const auto endp = listTypeName.find('>');

  if ((endp != std::string::npos) && (endp + 1 == listTypeName.length()) &&
    listTypeName.compare(0, 5, "List<") == 0)
  {
    // Without ignoreCase
    return vtkFoamTypes::ToEnumImpl(listTypeName, 5, endp, false);
  }

  return vtkFoamTypes::NO_TYPE;
}

//------------------------------------------------------------------------------
// class vtkFoamStreamOption
// Some elements from Foam::IOstreamOption and from Foam::IOstream
// - format (ASCII | BINARY)
// - label, scalar sizes
//
// Note: all enums pack into 32-bits, so we can use them in vtkFoamToken, vtkFoamFile etc.
// without adversely affecting the size of the structures
struct vtkFoamStreamOption
{
public:
  // The OpenFOAM input stream format is ASCII or BINARY
  enum fileFormat : unsigned char
  {
    ASCII = 0, // ASCII unless otherwise specified
    BINARY
  };

  // Bitwidth of an OpenFOAM label (integer type).
  // Corresponds to WM_LABEL_SIZE (32|64)
  enum labelType : unsigned char
  {
    INT32,
    INT64
  };

  // Bitwidth of an OpenFOAM scalar (floating-point type)
  // Corresponds to WM_PRECISION_OPTION (SP|DP|SPDP)
  enum scalarType : unsigned char
  {
    FLOAT32,
    FLOAT64
  };

private:
  fileFormat Format = fileFormat::ASCII;
  labelType LabelType = labelType::INT32;
  scalarType ScalarType = scalarType::FLOAT64;

public:
  // Default construct. ASCII, Int32, double precision
  vtkFoamStreamOption() = default;

  // Construct with specified handling for labels/floats
  vtkFoamStreamOption(const bool use64BitLabels, const bool use64BitFloats)
  {
    this->SetLabel64(use64BitLabels);
    this->SetFloat64(use64BitFloats);
  }

  bool IsAsciiFormat() const noexcept { return this->Format == fileFormat::ASCII; }
  bool IsLabel64() const noexcept { return this->LabelType == labelType::INT64; }
  bool IsFloat64() const noexcept { return this->ScalarType == scalarType::FLOAT64; }

  void SetBinaryFormat(const bool on)
  {
    this->Format = (on ? fileFormat::BINARY : fileFormat::ASCII);
  }
  void SetLabel64(const bool on) noexcept
  {
    this->LabelType = (on ? labelType::INT64 : labelType::INT32);
  }
  void SetFloat64(const bool on) noexcept
  {
    this->ScalarType = (on ? scalarType::FLOAT64 : scalarType::FLOAT32);
  }
  const vtkFoamStreamOption& GetStreamOption() const noexcept
  {
    return static_cast<const vtkFoamStreamOption&>(*this);
  }
  void SetStreamOption(const vtkFoamStreamOption& opt) noexcept
  {
    static_cast<vtkFoamStreamOption&>(*this) = opt;
  }
};

//------------------------------------------------------------------------------
// class vtkFoamToken
// token class which also works as container for list types
// - a word token is treated as a string token for simplicity
// - handles only atomic types. Handling of list types are left to the
//   derived classes.
struct vtkFoamToken : public vtkFoamStreamOption
{
public:
  enum tokenType
  {
    // Undefined type
    UNDEFINED = 0,
    // atomic types
    PUNCTUATION,
    LABEL,
    SCALAR,
    STRING,
    IDENTIFIER,
    // List types (vtkObject-derived)
    BOOLLIST,
    LABELLIST,
    SCALARLIST,
    VECTORLIST,
    STRINGLIST,
    // List types (non-vtkObject)
    LABELLISTLIST,
    ENTRYVALUELIST,
    EMPTYLIST,
    DICTIONARY,
    // error state
    TOKEN_ERROR
  };

protected:
  tokenType Type = tokenType::UNDEFINED;
  union {
    char Char;
    vtkTypeInt64 Int;
    double Double;
    // Any/all pointer types
    void* AnyPointer;
    std::string* StringPtr;
    // List types (vtkObject-derived)
    vtkObjectBase* VtkObjectPtr;
    vtkTypeInt8Array* BoolListPtr;
    vtkDataArray* LabelListPtr;
    vtkFloatArray* ScalarListPtr;
    vtkFloatArray* VectorListPtr;
    vtkStringArray* StringListPtr;
    // List types (non-vtkObject)
    vtkFoamLabelListList* LabelListListPtr;
    vtkFoamPtrList<vtkFoamEntryValue>* EntryValuePtrs;
    vtkFoamDict* DictPtr;
  };

  void Clear()
  {
    if (this->Type == STRING || this->Type == IDENTIFIER) // IsStringType
    {
      delete this->StringPtr;
    }
  }

  void AssignData(const vtkFoamToken& tok)
  {
    switch (tok.Type)
    {
      case PUNCTUATION:
        this->Char = tok.Char;
        break;
      case LABEL:
        this->Int = tok.Int;
        break;
      case SCALAR:
        this->Double = tok.Double;
        break;
      case STRING:
      case IDENTIFIER:
        this->StringPtr = new std::string(*tok.StringPtr);
        break;
      default:
        break;
    }
  }

public:
  // Default construct
  vtkFoamToken() = default;

  vtkFoamToken(const vtkFoamToken& tok)
    : vtkFoamStreamOption(tok)
    , Type(tok.Type)
  {
    this->AssignData(tok);
  }
  ~vtkFoamToken() { this->Clear(); }

  tokenType GetType() const { return this->Type; }

  template <typename T>
  bool Is() const;
  template <typename T>
  T To() const;
#if defined(_MSC_VER)
  // workaround for Win32-64ids-nmake70
  template <>
  bool Is<vtkTypeInt32>() const;
  template <>
  bool Is<vtkTypeInt64>() const;
  template <>
  bool Is<float>() const;
  template <>
  bool Is<double>() const;
  template <>
  vtkTypeInt32 To<vtkTypeInt32>() const;
  template <>
  vtkTypeInt64 To<vtkTypeInt64>() const;
  template <>
  float To<float>() const;
  template <>
  double To<double>() const;
#endif

  // Token represents PUNCTUATION
  bool IsPunctuation() const noexcept { return this->Type == PUNCTUATION; }

  // Token is PUNCTUATION and equal to parameter
  bool IsPunctuation(const char c) const noexcept
  {
    return this->Type == PUNCTUATION && c == this->Char;
  }

  // Token represents an LABEL (integer) value
  bool IsLabel() const noexcept { return this->Type == LABEL; }

  // Token is LABEL (integer) value and equal to parameter
  bool IsLabel(const vtkTypeInt64 val) const noexcept
  {
    return this->Type == LABEL && val == this->Int;
  }

  // Token represents a SCALAR (floating-point) value
  bool IsScalar() const noexcept { return this->Type == SCALAR; }

  // Token represents a numerical value
  bool IsNumeric() const noexcept { return this->Type == LABEL || this->Type == SCALAR; }

  // Token is STRING
  bool IsString() const noexcept { return this->Type == STRING; }

  // Token is STRING and equal to parameter
  bool IsString(const std::string& str) const
  {
    return this->Type == STRING && str == *this->StringPtr;
  }

  // Token represents string content
  bool IsStringType() const noexcept { return this->Type == STRING || this->Type == IDENTIFIER; }

  // Integer value from LABEL token without checks
  vtkTypeInt64 ToInt() const noexcept { return this->Int; }

  // Mostly the same as To<float>, with additional check
  float ToFloat() const noexcept
  {
    return this->Type == LABEL ? static_cast<float>(this->Int)
                               : this->Type == SCALAR ? static_cast<float>(this->Double) : 0.0F;
  }

  // Mostly the same as To<double>, with additional check
  double ToDouble() const noexcept
  {
    return this->Type == LABEL ? static_cast<double>(this->Int)
                               : this->Type == SCALAR ? this->Double : 0.0;
  }

  std::string ToString() const { return *this->StringPtr; }
  std::string ToIdentifier() const { return *this->StringPtr; }

  // Clear token and set to be ERROR.
  void SetBad()
  {
    this->Clear();
    this->Type = TOKEN_ERROR;
  }
  void SetIdentifier(const std::string& idString)
  {
    this->operator=(idString);
    this->Type = IDENTIFIER;
  }

  void operator=(const char c)
  {
    this->Clear();
    this->Type = PUNCTUATION;
    this->Char = c;
  }
  void operator=(const vtkTypeInt32 val)
  {
    this->Clear();
    this->Type = LABEL;
    this->Int = static_cast<vtkTypeInt32>(val);
    if (this->IsLabel64())
    {
      vtkGenericWarningMacro("Assigned int32 to int64 label");
    }
  }
  void operator=(const vtkTypeInt64 val)
  {
    this->Clear();
    this->Type = LABEL;
    this->Int = val;
    if (!this->IsLabel64())
    {
      vtkGenericWarningMacro("Assigned int64 to int32 label - may lose precision");
    }
  }
  void operator=(const double val)
  {
    this->Clear();
    this->Type = SCALAR;
    this->Double = val;
  }
  void operator=(const char* str)
  {
    this->Clear();
    this->Type = STRING;
    this->StringPtr = new std::string(str);
  }
  void operator=(const std::string& str)
  {
    this->Clear();
    this->Type = STRING;
    this->StringPtr = new std::string(str);
  }
  vtkFoamToken& operator=(const vtkFoamToken& tok)
  {
    this->Clear();
    this->SetStreamOption(tok);
    this->Type = tok.Type;
    this->AssignData(tok);
    return *this;
  }
  bool operator==(const char c) const noexcept { return this->IsPunctuation(c); }
  bool operator!=(const char c) const noexcept { return !this->IsPunctuation(c); }
  bool operator==(const vtkTypeInt32 val) const { return this->IsLabel(val); }
  bool operator==(const vtkTypeInt64 val) const { return this->IsLabel(val); }
  bool operator==(const std::string& str) const { return this->IsString(str); }
  bool operator!=(const std::string& str) const { return !this->IsString(str); }

  friend std::ostringstream& operator<<(std::ostringstream& os, const vtkFoamToken& tok)
  {
    switch (tok.GetType())
    {
      case TOKEN_ERROR:
        os << "badToken (an unexpected EOF?)";
        break;
      case PUNCTUATION:
        os << tok.Char;
        break;
      case LABEL:
        if (tok.IsLabel64())
        {
          os << tok.Int;
        }
        else
        {
          os << static_cast<vtkTypeInt32>(tok.Int);
        }
        break;
      case SCALAR:
        os << tok.Double;
        break;
      case STRING:
      case IDENTIFIER:
        os << *(tok.StringPtr);
        break;
      default:
        break;
    }
    return os;
  }
};

//------------------------------------------------------------------------------
// Specializations for vtkFoamToken

template <>
inline bool vtkFoamToken::Is<vtkTypeInt8>() const
{
  // masquerade for bool
  return this->Type == LABEL;
}

template <>
inline bool vtkFoamToken::Is<vtkTypeInt32>() const
{
  return this->Type == LABEL && !(this->IsLabel64());
}

template <>
inline bool vtkFoamToken::Is<vtkTypeInt64>() const
{
  return this->Type == LABEL;
}

template <>
inline bool vtkFoamToken::Is<float>() const
{
  return this->Type == LABEL || this->Type == SCALAR;
}

template <>
inline bool vtkFoamToken::Is<double>() const
{
  return this->Type == SCALAR;
}

// ie, a bool value
template <>
inline vtkTypeInt8 vtkFoamToken::To<vtkTypeInt8>() const
{
  return static_cast<vtkTypeInt8>(this->Int);
}

template <>
inline vtkTypeInt32 vtkFoamToken::To<vtkTypeInt32>() const
{
  if (this->IsLabel64())
  {
    vtkGenericWarningMacro("Casting int64 label to int32 - may lose precision");
  }
  return static_cast<vtkTypeInt32>(this->Int);
}

template <>
inline vtkTypeInt64 vtkFoamToken::To<vtkTypeInt64>() const
{
  return this->Int;
}

template <>
inline float vtkFoamToken::To<float>() const
{
  return this->Type == LABEL ? static_cast<float>(this->Int) : static_cast<float>(this->Double);
}

template <>
inline double vtkFoamToken::To<double>() const
{
  return this->Type == LABEL ? static_cast<double>(this->Int) : this->Double;
}

//------------------------------------------------------------------------------
// class vtkFoamFileStack
// list of variables that have to be saved when a file is included.
struct vtkFoamFileStack
{
protected:
  vtkOpenFOAMReader* Reader; // GUI preference
  std::string FileName;
  FILE* File;
  z_stream Z;
  int ZStatus;
  int LineNumber;
  bool IsCompressed;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
  bool WasNewline;
#endif
  // buffer pointers. using raw pointers for performance reason.
  unsigned char* Inbuf;
  unsigned char* Outbuf;
  unsigned char* BufPtr;
  unsigned char* BufEndPtr;

  vtkFoamFileStack(vtkOpenFOAMReader* reader)
    : Reader(reader)
    , File(nullptr)
    , ZStatus(Z_OK)
    , LineNumber(0)
    , IsCompressed(false)
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
    , WasNewline(true)
#endif
    , Inbuf(nullptr)
    , Outbuf(nullptr)
    , BufPtr(nullptr)
    , BufEndPtr(nullptr)
  {
    this->Z.zalloc = Z_NULL;
    this->Z.zfree = Z_NULL;
    this->Z.opaque = Z_NULL;
  }

  void Reset()
  {
    // this->FileName = "";
    this->File = nullptr;
    // this->ZStatus = Z_OK;
    this->Z.zalloc = Z_NULL;
    this->Z.zfree = Z_NULL;
    this->Z.opaque = Z_NULL;
    // this->LineNumber = 0;
    this->IsCompressed = false;
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
    this->WasNewline = true;
#endif

    this->Inbuf = nullptr;
    this->Outbuf = nullptr;
    // this->BufPtr = nullptr;
    // this->BufEndPtr = nullptr;
  }

public:
  const std::string& GetFileName() const noexcept { return this->FileName; }
  int GetLineNumber() const noexcept { return this->LineNumber; }

  // Try to open the file. Return non-empty error string on failure
  vtkFoamError TryOpen(const std::string& fileName)
  {
    vtkFoamError errors;
    do
    {
      // Line number 0 to indicate beginning of file when an exception is thrown
      this->LineNumber = 0;
      this->FileName = fileName;

      if (this->File)
      {
        errors << "File already opened within this object";
        break;
      }
      this->File = vtksys::SystemTools::Fopen(this->FileName, "rb");
      if (this->File == nullptr)
      {
        errors << "Cannot open file for reading";
        break;
      }

      unsigned char zMagic[2];
      if (fread(zMagic, 1, 2, this->File) == 2 && zMagic[0] == 0x1f && zMagic[1] == 0x8b)
      {
        // gzip-compressed format
        this->Z.avail_in = 0;
        this->Z.next_in = Z_NULL;
        // + 32 to automatically recognize gzip format
        if (inflateInit2(&this->Z, 15 + 32) == Z_OK)
        {
          this->IsCompressed = true;
          this->Inbuf = new unsigned char[VTK_FOAMFILE_INBUFSIZE];
        }
        else
        {
          fclose(this->File);
          this->File = nullptr;

          errors << "Cannot init zstream";
          if (this->Z.msg)
          {
            errors << " " << this->Z.msg;
          }
          break;
        }
      }
      else
      {
        this->IsCompressed = false;
      }
      rewind(this->File);

      this->ZStatus = Z_OK;
      this->Outbuf = new unsigned char[VTK_FOAMFILE_OUTBUFSIZE + 1];
      this->BufPtr = this->Outbuf + 1;
      this->BufEndPtr = this->BufPtr;
      this->LineNumber = 1;
    } while (false);

    return errors;
  }

  void CloseCurrentFile()
  {
    if (this->IsCompressed)
    {
      inflateEnd(&this->Z);
    }

    delete[] this->Inbuf;
    delete[] this->Outbuf;
    this->Inbuf = this->Outbuf = nullptr;

    if (this->File)
    {
      fclose(this->File);
      this->File = nullptr;
    }
    // don't reset the line number so that the last line number is
    // retained after close
    // lineNumber_ = 0;
  }
};

//------------------------------------------------------------------------------
// class vtkFoamFile
// Read and tokenize the input. Retains format and label/scalar size information
struct vtkFoamFile
  : public vtkFoamStreamOption
  , public vtkFoamFileStack
{
private:
  typedef vtkFoamFileStack Superclass;

  // Find last slash (os-specific)
  static size_t rfind_slash(const std::string& str, size_t pos = std::string::npos) noexcept
  {
#if defined(_WIN32)
    return str.find_last_of("/\\", pos);
#else
    return str.find_last_of('/', pos);
#endif
  }

  // String equivalent cwd (os-specific)
  static std::string cwd_string() noexcept
  {
#if defined(_WIN32)
    return std::string(".\\");
#else
    return std::string("./");
#endif
  }

public:
  // The dictionary #inputMode values
  enum inputMode
  {
    INPUT_MODE_MERGE,
    INPUT_MODE_OVERWRITE,
    INPUT_MODE_PROTECT,
    INPUT_MODE_WARN,
    INPUT_MODE_ERROR
  };

  // Generic exception throwing with stack trace
  void ThrowStackTrace(const std::string& msg);

private:
  std::string CasePath; // The full path to the case - includes trailing '/'

  // The current input mode
  inputMode InputMode;

  // Handling include files
  vtkFoamFileStack* Stack[VTK_FOAMFILE_INCLUDE_STACK_SIZE];
  int StackI;

  bool InflateNext(unsigned char* buf, size_t requestSize, vtkTypeInt64* readSize = nullptr);
  int NextTokenHead();

  // Keep exception throwing / recursive codes out-of-line to make
  // putBack(), getc() and readExpecting() inline expandable
  void ThrowDuplicatedPutBackException();
  void ThrowUnexpectedEOFException();
  void ThrowUnexpectedNondigitException(int c);
  void ThrowUnexpectedTokenException(char, int c);
  int ReadNext();

  void PutBack(const int c)
  {
    if (--this->Superclass::BufPtr < this->Superclass::Outbuf)
    {
      this->ThrowDuplicatedPutBackException();
    }
    *this->Superclass::BufPtr = static_cast<unsigned char>(c);
  }

  // get a character
  int Getc()
  {
    return this->Superclass::BufPtr == this->Superclass::BufEndPtr ? this->ReadNext()
                                                                   : *this->Superclass::BufPtr++;
  }

  vtkFoamError StackString()
  {
    vtkFoamError err;
    if (this->StackI > 0)
    {
      err << "\n included";

      for (int stackI = this->StackI - 1; stackI >= 0; stackI--)
      {
        err << " from line " << this->Stack[stackI]->GetLineNumber() << " of "
            << this->Stack[stackI]->GetFileName() << "\n";
      }
      err << ": ";
    }
    return err;
  }

  bool CloseIncludedFile()
  {
    if (this->StackI == 0)
    {
      return false;
    }
    this->StackI--;
    this->Superclass::CloseCurrentFile();
    // use the default bitwise assignment operator
    this->Superclass::operator=(*this->Stack[this->StackI]);
    delete this->Stack[this->StackI];
    return true;
  }

public:
  // No default construct, copy or assignment
  vtkFoamFile() = delete;
  vtkFoamFile(const vtkFoamFile&) = delete;
  void operator=(const vtkFoamFile&) = delete;

  vtkFoamFile(const std::string& casePath, vtkOpenFOAMReader* reader)
    : vtkFoamStreamOption(reader->GetUse64BitLabels(), reader->GetUse64BitFloats())
    , vtkFoamFileStack(reader)
    , CasePath(casePath)
    , InputMode(INPUT_MODE_MERGE)
    , StackI(0)
  {
  }
  ~vtkFoamFile() { this->Close(); }

  std::string GetCasePath() const noexcept { return this->CasePath; }
  std::string GetFilePath() const { return vtkFoamFile::ExtractPath(this->FileName); }
  inputMode GetInputMode() const noexcept { return this->InputMode; }

  void Open(const std::string& fileName)
  {
    vtkFoamError err = this->Superclass::TryOpen(fileName);
    if (!err.empty())
    {
      this->ThrowStackTrace(err);
    }
  }

  void Close()
  {
    while (this->CloseIncludedFile())
      ;
    this->CloseCurrentFile();

    // Reinstate values from reader (eg, GUI)
    auto& streamOpt = static_cast<vtkFoamStreamOption&>(*this);
    streamOpt.SetLabel64(this->Reader->GetUse64BitLabels());
    streamOpt.SetFloat64(this->Reader->GetUse64BitFloats());
  }

  // Static File Functions

  // Check for existence of specified file
  static bool IsFile(const std::string& file, bool checkGzip = true)
  {
    return (vtksys::SystemTools::FileExists(file, true) ||
      (checkGzip && vtksys::SystemTools::FileExists(file + ".gz", true)));
  }

  //! Return file name (part beyond last /)
  static std::string ExtractName(const std::string& path)
  {
    auto pos = vtkFoamFile::rfind_slash(path);
    if (pos == std::string::npos)
    {
      // no slash
      return path;
    }
    else if (pos + 1 == path.length())
    {
      // trailing slash
      const auto endPos = pos;
      pos = vtkFoamFile::rfind_slash(path, pos - 1);
      if (pos == std::string::npos)
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
      return path.substr(pos + 1);
    }
  }

  //! Return directory path name (part before last /). Return includes trailing slash!
  static std::string ExtractPath(const std::string& path)
  {
    const auto pos = vtkFoamFile::rfind_slash(path);
    return pos == std::string::npos ? vtkFoamFile::cwd_string() : path.substr(0, pos + 1);
  }

  // Member Functions

  std::string ExpandPath(const std::string& pathIn, const std::string& defaultPath)
  {
    std::string expandedPath;
    bool isExpanded = false, wasPathSeparator = true;
    size_t charI = 0;
    const size_t nChars = pathIn.length();

    std::string::size_type delim = 0;

    if ('<' == pathIn[0] && (delim = pathIn.find(">/")) != std::string::npos)
    {
      // Expand a leading <tag>/
      // Convenient for frequently used directories - see OpenFOAM stringOps.C
      //
      // Handle
      //   <case>/       => FOAM_CASE directory
      //   <constant>/   => FOAM_CASE/constant directory
      //   <system>/     => FOAM_CASE/system directory
      //   <etc>/        => not handled

      const std::string tag(pathIn, 1, delim - 2);

      if (tag == "case")
      {
        expandedPath = this->CasePath + '/';
        isExpanded = true;
        wasPathSeparator = false;
      }
      else if (tag == "constant" || tag == "system")
      {
        expandedPath = this->CasePath + '/' + tag + '/';
        isExpanded = true;
        wasPathSeparator = false;
      }
      // <etc> in not handled

      if (isExpanded)
      {
        charI = delim + 2;
      }
    }

    while (charI < nChars)
    {
      const char c = pathIn[charI];
      switch (c)
      {
        case '$': // $-variable expansion
        {
          std::string variable;
          while (++charI < nChars && (isalnum(pathIn[charI]) || pathIn[charI] == '_'))
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
            expandedPath += vtkFoamFile::ExtractName(this->CasePath);
            wasPathSeparator = false;
            isExpanded = true;
          }
          else
          {
            std::string value;
            if (vtksys::SystemTools::GetEnv(variable, value))
            {
              expandedPath += value;
            }
            const auto len = expandedPath.length();
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
            std::string userName;
            while (++charI < nChars && (pathIn[charI] != '/' && pathIn[charI] != '\\') &&
              pathIn[charI] != '$')
            {
              userName += pathIn[charI];
            }

            std::string homeDir;
            if (userName.empty())
            {
              if (!vtksys::SystemTools::GetEnv("HOME", homeDir) || homeDir.empty())
              {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
                // No fallback
                homeDir.clear();
#else
                const struct passwd* pwentry = getpwuid(getuid());
                if (pwentry == nullptr)
                {
                  this->ThrowStackTrace("Home directory path not found");
                }
                homeDir = pwentry->pw_dir;
#endif
              }
              expandedPath = homeDir;
            }
            else if (userName == "OpenFOAM")
            {
              // So far only "~/.OpenFOAM" expansion is supported

              if (!vtksys::SystemTools::GetEnv("HOME", homeDir) || homeDir.empty())
              {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
                // No fallback
                homeDir.clear();
#else
                const struct passwd* pwentry = getpwuid(getuid());
                if (pwentry == nullptr)
                {
                  this->ThrowStackTrace("Home directory path not found");
                }
                homeDir = pwentry->pw_dir;
#endif
              }

              if (homeDir.empty())
              {
                expandedPath = homeDir;
              }
              else
              {
                expandedPath = homeDir + "/.OpenFOAM";
              }
            }
            else
            {
#if defined(_WIN32) && !defined(__CYGWIN__) || defined(__LIBCATAMOUNT__)
              if (!vtksys::SystemTools::GetEnv("HOME", homeDir))
              {
                // No fallback
                homeDir.clear();
              }
              expandedPath = vtkFoamFile::ExtractPath(homeDir) + userName;
#else
              const struct passwd* pwentry = getpwnam(userName.c_str());
              if (pwentry == nullptr)
              {
                this->ThrowStackTrace("No home directory for user " + userName);
              }
              expandedPath = pwentry->pw_dir;
#endif
            }
            wasPathSeparator = false;
            isExpanded = true;
            break;
          }
          VTK_FALLTHROUGH;
        default:
          wasPathSeparator = (c == '/' || c == '\\');
          expandedPath += c;
          charI++;
      }
    }
    if (isExpanded || expandedPath[0] == '/' || expandedPath[0] == '\\')
    {
      return expandedPath;
    }
    else
    {
      return defaultPath + expandedPath;
    }
  }

  void IncludeFile(const std::string& includedFileName, const std::string& defaultPath)
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
    token.SetStreamOption(this->GetStreamOption());
    const bool use64BitLabels = this->IsLabel64();

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
    if (c == '/')
    {
      this->PutBack(c);
      c = this->NextTokenHead();
    }
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
    if (c != '#')
    {
      this->Superclass::WasNewline = false;
    }
#endif

    constexpr int MAXLEN = 1024;
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
          buf[charI++] = static_cast<unsigned char>(c);
        } while (isdigit(c = this->Getc()) && charI < MAXLEN);
        if (c != '.' && c != 'e' && c != 'E' && charI < MAXLEN && c != EOF)
        {
          // label token
          buf[charI] = '\0';
          if (use64BitLabels)
          {
            token = static_cast<vtkTypeInt64>(strtoll(buf, nullptr, 10));
          }
          else
          {
            token = static_cast<vtkTypeInt32>(strtol(buf, nullptr, 10));
          }
          this->PutBack(c);
          return true;
        }
        VTK_FALLTHROUGH;
      case '.':
        // scalar token
        if (c == '.' && charI < MAXLEN)
        {
          // read decimal fraction part
          buf[charI++] = static_cast<unsigned char>(c);
          while (isdigit(c = this->Getc()) && charI < MAXLEN)
          {
            buf[charI++] = static_cast<unsigned char>(c);
          }
        }
        if ((c == 'e' || c == 'E') && charI < MAXLEN)
        {
          // read exponent part
          buf[charI++] = static_cast<unsigned char>(c);
          if (((c = this->Getc()) == '+' || c == '-') && charI < MAXLEN)
          {
            buf[charI++] = static_cast<unsigned char>(c);
            c = this->Getc();
          }
          while (isdigit(c) && charI < MAXLEN)
          {
            buf[charI++] = static_cast<unsigned char>(c);
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
        token = strtod(buf, nullptr);
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
              this->ThrowStackTrace("Unescaped newline in string constant");
            }
          }
          buf[charI++] = static_cast<unsigned char>(c);
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
          this->ThrowStackTrace("Unexpected EOF reading identifier");
        }
        if (identifierToken.GetType() != vtkFoamToken::STRING)
        {
          throw this->StackString() << "Expected a word, found " << identifierToken;
        }
        token.SetIdentifier(identifierToken.ToString());
        return true;
      }
      case '#':
      {
#if VTK_FOAMFILE_RECOGNIZE_LINEHEAD
        // the OpenFOAM #-directives can indeed be placed in the
        // middle of a line
        if (!this->Superclass::WasNewline)
        {
          this->ThrowStackTrace("Encountered #-directive in the middle of a line");
        }
        this->Superclass::WasNewline = false;
#endif
        // read directive
        vtkFoamToken directiveToken;
        if (!this->Read(directiveToken))
        {
          this->ThrowStackTrace("Unexpected EOF reading directive");
        }
        if (directiveToken == "include")
        {
          vtkFoamToken fileNameToken;
          if (!this->Read(fileNameToken))
          {
            this->ThrowStackTrace("Unexpected EOF reading filename");
          }
          this->IncludeFile(fileNameToken.ToString(), vtkFoamFile::ExtractPath(this->FileName));
        }
        else if (directiveToken == "sinclude" || directiveToken == "includeIfPresent")
        {
          vtkFoamToken fileNameToken;
          if (!this->Read(fileNameToken))
          {
            this->ThrowStackTrace("Unexpected EOF reading filename");
          }

          // special treatment since the file is allowed to be missing
          const std::string fullName =
            this->ExpandPath(fileNameToken.ToString(), vtkFoamFile::ExtractPath(this->FileName));

          FILE* fh = vtksys::SystemTools::Fopen(fullName, "rb");
          if (fh)
          {
            fclose(fh);

            this->IncludeFile(fileNameToken.ToString(), vtkFoamFile::ExtractPath(this->FileName));
          }
        }
        else if (directiveToken == "inputMode")
        {
          vtkFoamToken modeToken;
          if (!this->Read(modeToken))
          {
            this->ThrowStackTrace("Unexpected EOF reading inputMode specifier");
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
                                         "(merge, overwrite, protect, warn, error, default), found "
                                      << modeToken;
          }
        }
        else if (directiveToken == '{')
        {
          // '#{' verbatim/code block. swallow everything until a closing '#}'
          // This hopefully matches the first one...
          while (true)
          {
            c = this->NextTokenHead();
            if (c == EOF)
            {
              this->ThrowStackTrace("Unexpected EOF while skipping over #{ directive");
            }
            else if (c == '#')
            {
              c = this->Getc();
              if (c == '/')
              {
                this->PutBack(c);
              }
              else if (c == '}')
              {
                break;
              }
            }
          }
        }
        else
        {
          throw this->StackString() << "Unsupported directive " << directiveToken;
        }
        return this->Read(token);
      }
      default:
      {
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
          buf[charI++] = static_cast<unsigned char>(c);
          // valid characters that constitutes a word
          // cf. src/OpenFOAM/primitives/strings/word/wordI.H
        } while ((c = this->Getc()) != EOF && !isspace(c) && c != '"' && c != '/' && c != ';' &&
          c != '{' && c != '}' && charI < MAXLEN);
        buf[charI] = '\0';
        token = buf;
        this->PutBack(c);
      }
    }

    if (c == EOF)
    {
      this->ThrowUnexpectedEOFException();
    }
    if (charI == MAXLEN)
    {
      throw this->StackString() << "Exceeded maximum allowed length of " << MAXLEN;
    }
    return true;
  }

  // fread or gzread with buffering handling
  vtkTypeInt64 Read(unsigned char* buf, const vtkTypeInt64 len)
  {
    const size_t buflen = (this->Superclass::BufEndPtr - this->Superclass::BufPtr);

    vtkTypeInt64 readlen;
    if (static_cast<size_t>(len) > buflen)
    {
      memcpy(buf, this->Superclass::BufPtr, buflen);
      this->InflateNext(buf + buflen, len - buflen, &readlen);
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
    for (vtkTypeInt64 i = 0; i < readlen; ++i)
    {
      if (buf[i] == '\n')
      {
        ++this->Superclass::LineNumber;
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
    if (c == '/')
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
    vtkFoamToken tok;
    if (!this->Read(tok) || tok != str)
    {
      throw this->StackString() << "Expected string \"" << str << "\", found " << tok;
    }
  }

  // ASCII read of longest integer
  vtkTypeInt64 ReadIntegerValue();

  // ASCII read of longest floating-point
  double ReadDoubleValue();
};

//------------------------------------------------------------------------------
// Code: vtkFoamFile

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
vtkTypeInt64 vtkFoamFile::ReadIntegerValue()
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
  if (c == '/')
  {
    this->PutBack(c);
    c = this->NextTokenHead();
  }

  // leading sign?
  const bool negNum = (c == '-');
  if (negNum || c == '+')
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
      this->ThrowUnexpectedNondigitException(c);
    }
  }

  vtkTypeInt64 num = c - '0';
  while (isdigit(c = this->Getc()))
  {
    num = 10 * num + c - '0';
  }

  if (c == EOF)
  {
    this->ThrowUnexpectedEOFException();
  }
  this->PutBack(c);

  return negNum ? -num : num;
}

// extremely simplified high-performing string to floating point
// conversion code based on
// ParaView3/VTK/Utilities/vtksqlite/vtk_sqlite3.c
double vtkFoamFile::ReadDoubleValue()
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
  if (c == '/')
  {
    this->PutBack(c);
    c = this->NextTokenHead();
  }

  // leading sign?
  const bool negNum = (c == '-');
  if (negNum || c == '+')
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

  if (!isdigit(c) && c != '.') // Attention: isdigit() accepts EOF
  {
    this->ThrowUnexpectedNondigitException(c);
  }

  double num = 0;

  // read integer part (before '.')
  if (c != '.')
  {
    num = c - '0';
    while (isdigit(c = this->Getc()))
    {
      num = num * 10.0 + (c - '0');
    }
  }

  // read decimal part (after '.')
  if (c == '.')
  {
    double divisor = 1.0;

    while (isdigit(c = this->Getc()))
    {
      num = num * 10.0 + (c - '0');
      divisor *= 10.0;
    }
    num /= divisor;
  }

  // read exponent part
  if (c == 'E' || c == 'e')
  {
    int esign = 1;
    int eval = 0;
    double scale = 1.0;

    c = this->Getc();
    if (c == '-')
    {
      esign = -1;
      c = this->Getc();
    }
    else if (c == '+')
    {
      c = this->Getc();
    }

    while (isdigit(c))
    {
      eval = eval * 10 + (c - '0');
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

  return negNum ? -num : num;
}

void vtkFoamFile::ThrowStackTrace(const std::string& msg)
{
  throw this->StackString() << msg;
}

// hacks to keep exception throwing code out-of-line to make
// putBack() and readExpecting() inline expandable
void vtkFoamFile::ThrowUnexpectedEOFException()
{
  this->ThrowStackTrace("Unexpected EOF");
}

void vtkFoamFile::ThrowUnexpectedNondigitException(int c)
{
  throw this->StackString() << "Expected a number, found a non-digit character "
                            << static_cast<char>(c);
}

void vtkFoamFile::ThrowUnexpectedTokenException(char expected, int c)
{
  vtkFoamError err;
  err << this->StackString() << "Expected punctuation token '" << expected << "', found ";
  if (c == EOF)
  {
    err << "EOF";
  }
  else
  {
    err << static_cast<char>(c);
  }
  throw err;
}

void vtkFoamFile::ThrowDuplicatedPutBackException()
{
  this->ThrowStackTrace("Attempted duplicated putBack()");
}

bool vtkFoamFile::InflateNext(unsigned char* buf, size_t requestSize, vtkTypeInt64* readSize)
{
  if (readSize)
  {
    *readSize = -1; // Set to an error state for early returns
  }
  size_t size;
  if (this->Superclass::IsCompressed)
  {
    if (this->Superclass::ZStatus != Z_OK)
    {
      return false;
    }
    this->Superclass::Z.next_out = buf;
    this->Superclass::Z.avail_out = static_cast<uInt>(requestSize);

    do
    {
      if (this->Superclass::Z.avail_in == 0)
      {
        this->Superclass::Z.next_in = this->Superclass::Inbuf;
        this->Superclass::Z.avail_in = static_cast<uInt>(
          fread(this->Superclass::Inbuf, 1, VTK_FOAMFILE_INBUFSIZE, this->Superclass::File));
        if (ferror(this->Superclass::File))
        {
          this->ThrowStackTrace("failed in fread()");
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
  if (readSize)
  {
    // Cast size_t to int64. Should be OK since requestSize came from OpenFOAM (signed integer)
    *readSize = static_cast<vtkTypeInt64>(size);
  }
  return true;
}

// get next semantically valid character
int vtkFoamFile::NextTokenHead()
{
  while (true)
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
        while (true)
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

//------------------------------------------------------------------------------
// class vtkFoamDict
// A class to holds a FoamFile data structure
struct vtkFoamDict : public std::vector<vtkFoamEntry*>
{
private:
  typedef std::vector<vtkFoamEntry*> Superclass;

  vtkFoamToken Token;
  const vtkFoamDict* UpperDictPtr;

  vtkFoamDict(const vtkFoamDict&) = delete;

public:
  // Default construct with given parent directory
  explicit vtkFoamDict(const vtkFoamDict* upperDictPtr = nullptr)
    : UpperDictPtr(upperDictPtr)
  {
  }
  // Copy construct with given parent directory
  vtkFoamDict(const vtkFoamDict& dict, const vtkFoamDict* upperDictPtr);

  // Destructor - delete list elements if held by the dictionary
  ~vtkFoamDict();

  // Remove top element, deleting its pointer
  void remove_back();

  void SetStreamOption(const vtkFoamStreamOption& opt) { this->Token.SetStreamOption(opt); }
  bool IsLabel64() const { return this->Token.IsLabel64(); } // convenience

  const vtkFoamToken& GetToken() const { return this->Token; }
  const vtkFoamDict* GetUpperDictPtr() const { return this->UpperDictPtr; }

  vtkFoamToken::tokenType GetType() const
  {
    return (this->Token.GetType() == vtkFoamToken::UNDEFINED ? vtkFoamToken::DICTIONARY
                                                             : this->Token.GetType());
  }

  // Return list of keywords - table of contents
  std::vector<std::string> Toc() const;

  // Search dictionary for specified keyword. Return nullptr on failure.
  vtkFoamEntry* Lookup(const std::string& keyword, bool isPattern = false) const;

  // Reads a FoamFile or a subdictionary.
  // If the stream to be read is a subdictionary,
  // the preceding '{' is assumed to have already been discarded.
  bool Read(
    vtkFoamIOobject& io, bool isSubDict = false, const vtkFoamToken& firstKeyword = vtkFoamToken());
};

//------------------------------------------------------------------------------
// class vtkFoamIOobject
// Extends vtkFoamFile with OpenFOAM class/object information
struct vtkFoamIOobject : public vtkFoamFile
{
private:
  typedef vtkFoamFile Superclass;

  std::string objectName_;
  std::string headerClassName_;
  vtkFoamError error_;

  // Inform IO object that lagrangian/positions has extra data (OpenFOAM v1.4 - v2.4)
  const bool LagrangianPositionsExtraData_;

  // Reads OpenFOAM format/class/object information and handles "arch" information
  void ReadHeader();

  // Attempt to open file (or file.gz) and read header
  bool OpenFile(const std::string& file, bool checkGzip = false)
  {
    this->ClearError(); // Discard any previous errors
    try
    {
      this->Superclass::Open(file);
      checkGzip = false;
    }
    catch (const vtkFoamError& err)
    {
      if (checkGzip)
      {
        // Avoid checking again if already ends_with(".gz")
        const auto len = file.length();
        if (len > 3 && file.compare(len - 3, std::string::npos, ".gz") == 0)
        {
          checkGzip = false;
        }
      }
      if (!checkGzip)
      {
        this->SetError(err);
        return false;
      }
    }

    if (checkGzip)
    {
      try
      {
        this->Superclass::Open(file + ".gz");
      }
      catch (const vtkFoamError& err)
      {
        this->SetError(err);
        return false;
      }
    }

    try
    {
      this->ReadHeader();
    }
    catch (const vtkFoamError& err)
    {
      this->Superclass::Close();
      this->SetError(err);
      return false;
    }
    return true;
  }

  void CloseFile()
  {
    this->Superclass::Close();
    this->objectName_.clear();
    this->headerClassName_.clear();
    this->error_.clear();
  }

public:
  // No generated methods
  vtkFoamIOobject() = delete;
  vtkFoamIOobject(const vtkFoamIOobject&) = delete;
  void operator=(const vtkFoamIOobject&) = delete;

  // Construct for specified case -path
  vtkFoamIOobject(const std::string& casePath, vtkOpenFOAMReader* reader)
    : vtkFoamFile(casePath, reader)
    , LagrangianPositionsExtraData_(static_cast<bool>(!reader->GetPositionsIsIn13Format()))
  {
  }

  ~vtkFoamIOobject() { this->Close(); }

  // Attempt to open file (without gzip fallback) and read FoamFile header
  bool Open(const std::string& file) { return this->OpenFile(file, false); }

  // Attempt to open file (with gzip fallback) and read FoamFile header
  bool OpenOrGzip(const std::string& file) { return this->OpenFile(file, true); }

  // Attempt to open file relative to the case, and read FoamFile header
  bool OpenCaseFile(const std::string& file) { return this->OpenFile(this->GetCasePath() + file); }

  // Attempt to open file (or file.gz) relative to the case, and read FoamFile header
  bool OpenCaseFileOrGzip(const std::string& file)
  {
    return this->OpenFile(this->GetCasePath() + file);
  }

  void Close() { this->CloseFile(); }

  const std::string& GetClassName() const noexcept { return this->headerClassName_; }
  const std::string& GetObjectName() const noexcept { return this->objectName_; }
  const vtkFoamError& GetError() const noexcept { return this->error_; }
  void ClearError() noexcept { this->error_.clear(); }
  bool HasError() const noexcept { return !this->error_.empty(); }
  void SetError(const vtkFoamError& err) { this->error_ = err; }
  bool GetLagrangianPositionsExtraData() const { return this->LagrangianPositionsExtraData_; }
};

//------------------------------------------------------------------------------
// ASCII read of primitive, with type casting
template <typename T>
struct vtkFoamReadValue
{
  static T ReadValue(vtkFoamIOobject& io);
};

template <>
inline vtkTypeInt8 vtkFoamReadValue<vtkTypeInt8>::ReadValue(vtkFoamIOobject& io)
{
  return static_cast<vtkTypeInt8>(io.ReadIntegerValue());
}

template <>
inline vtkTypeInt32 vtkFoamReadValue<vtkTypeInt32>::ReadValue(vtkFoamIOobject& io)
{
  return static_cast<vtkTypeInt32>(io.ReadIntegerValue());
}

template <>
inline vtkTypeInt64 vtkFoamReadValue<vtkTypeInt64>::ReadValue(vtkFoamIOobject& io)
{
  return io.ReadIntegerValue();
}

template <>
inline float vtkFoamReadValue<float>::ReadValue(vtkFoamIOobject& io)
{
  return static_cast<float>(io.ReadDoubleValue());
}

template <>
inline double vtkFoamReadValue<double>::ReadValue(vtkFoamIOobject& io)
{
  return io.ReadDoubleValue();
}

//------------------------------------------------------------------------------
// struct vtkFoamRead for reading primitives, lists etc.

struct vtkFoamRead
{
  // --------------------------------------------------------------------------
  // Reading lists of primitives (int/float/...)

  template <typename listT, typename primitiveT>
  class listTraits
  {
    listT* Ptr;

  public:
    using ValueType = typename listT::ValueType;

    listTraits()
      : Ptr(listT::New())
    {
    }

    // Get the contained pointer
    listT* GetPointer() const noexcept { return this->Ptr; }

    // De-reference pointer for operation
    listT* operator->() const noexcept { return this->Ptr; }

    void ReadValue(vtkFoamIOobject&, const vtkFoamToken& currToken)
    {
      if (!currToken.Is<primitiveT>())
      {
        throw vtkFoamError() << "Expected an integer or a (, found " << currToken;
      }
      this->Ptr->InsertNextValue(currToken.To<primitiveT>());
    }

    void ReadUniformValues(vtkFoamIOobject& io)
    {
      primitiveT value = vtkFoamReadValue<primitiveT>::ReadValue(io);
      this->Ptr->FillValue(value);
    }

    void ReadAsciiList(vtkFoamIOobject& io)
    {
      const vtkIdType nTuples = this->Ptr->GetNumberOfTuples();
      for (vtkIdType i = 0; i < nTuples; ++i)
      {
        this->Ptr->SetValue(i, vtkFoamReadValue<primitiveT>::ReadValue(io));
      }
    }

    void ReadBinaryList(vtkFoamIOobject& io)
    {
      // nComponents == 1
      const vtkIdType nTuples = this->Ptr->GetNumberOfTuples();
      const size_t nbytes = (nTuples * sizeof(primitiveT));

      if (typeid(ValueType) == typeid(primitiveT))
      {
        io.Read(reinterpret_cast<unsigned char*>(this->Ptr->GetPointer(0)), nbytes);
      }
      else
      {
        auto* fileData = vtkDataArray::CreateDataArray(vtkTypeTraits<primitiveT>::VTKTypeID());
        // nComponents == 1
        fileData->SetNumberOfTuples(nTuples);
        io.Read(reinterpret_cast<unsigned char*>(fileData->GetVoidPointer(0)), nbytes);
        this->Ptr->DeepCopy(fileData);
        fileData->Delete();
      }
    }
  };

  // --------------------------------------------------------------------------
  // Reads rank 1 lists of types vector, sphericalTensor, symmTensor and tensor.
  // If isPositions is true it reads Cloud type of data as
  // particle positions. cf. (the positions format)
  // src/lagrangian/basic/particle/particleIO.C - writePosition()
  template <typename listT, typename primitiveT, int nComponents, bool isPositions = false>
  class vectorListTraits
  {
    listT* Ptr;

    // Items to skip for lagrangian/positions (class Cloud) after the x/y/z values.
    // xyz (3*scalar) + celli (label)
    // in OpenFOAM 1.4 -> 2.4 also had facei (label) and stepFraction (scalar)
    // ASCII only
    static void LagrangianPositionsSkip(vtkFoamIOobject& io)
    {
      (void)io.ReadIntegerValue(); // Skip celli (label)
      if (io.GetLagrangianPositionsExtraData())
      {
        (void)io.ReadIntegerValue(); // Skip facei (label)
        (void)io.ReadDoubleValue();  // Skip stepFraction (scalar)
      }
    }

  public:
    using ValueType = typename listT::ValueType;

    vectorListTraits()
      : Ptr(listT::New())
    {
      this->Ptr->SetNumberOfComponents(nComponents);
    }

    // Get the contained pointer
    listT* GetPointer() const noexcept { return this->Ptr; }

    // De-reference pointer for operation
    listT* operator->() const noexcept { return this->Ptr; }

    void ReadValue(vtkFoamIOobject& io, const vtkFoamToken& currToken)
    {
      if (currToken != '(')
      {
        throw vtkFoamError() << "Expected '(', found " << currToken;
      }
      primitiveT tuple[nComponents];
      for (int cmpt = 0; cmpt < nComponents; ++cmpt)
      {
        tuple[cmpt] = vtkFoamReadValue<primitiveT>::ReadValue(io);
      }
      ::remapFoamTuple<nComponents == 6>(tuple); // For symmTensor
      io.ReadExpecting(')');
      this->Ptr->InsertNextTuple(tuple);
    }

    void ReadUniformValues(vtkFoamIOobject& io)
    {
      const vtkIdType nTuples = this->Ptr->GetNumberOfTuples();

      io.ReadExpecting('(');
      primitiveT tuple[nComponents];
      for (int cmpt = 0; cmpt < nComponents; ++cmpt)
      {
        tuple[cmpt] = vtkFoamReadValue<primitiveT>::ReadValue(io);
      }
      ::remapFoamTuple<nComponents == 6>(tuple); // For symmTensor
      io.ReadExpecting(')');
      if (isPositions)
      {
        this->LagrangianPositionsSkip(io);
      }
      for (vtkIdType i = 0; i < nTuples; ++i)
      {
        this->Ptr->SetTuple(i, tuple);
      }
    }

    void ReadAsciiList(vtkFoamIOobject& io)
    {
      const vtkIdType nTuples = this->Ptr->GetNumberOfTuples();

      for (vtkIdType i = 0; i < nTuples; ++i)
      {
        io.ReadExpecting('(');
        ValueType* tuple = this->Ptr->GetPointer(nComponents * i);
        for (int cmpt = 0; cmpt < nComponents; ++cmpt)
        {
          tuple[cmpt] = static_cast<ValueType>(vtkFoamReadValue<primitiveT>::ReadValue(io));
        }
        ::remapFoamTuple<nComponents == 6>(tuple); // For symmTensor
        io.ReadExpecting(')');
        if (isPositions)
        {
          this->LagrangianPositionsSkip(io);
        }
      }
    }

    void ReadBinaryList(vtkFoamIOobject& io)
    {
      const vtkTypeInt64 nTuples = this->Ptr->GetNumberOfTuples();

      if (isPositions) // lagrangian/positions (class Cloud)
      {
        // xyz (3*scalar) + celli (label)
        // in OpenFOAM 1.4 -> 2.4 also had facei (label) and stepFraction (scalar)

        const unsigned labelWidth = (io.IsLabel64() ? 8 : 4);
        const unsigned tupleLength = (sizeof(primitiveT) * nComponents + labelWidth +
          (io.GetLagrangianPositionsExtraData() ? (labelWidth + sizeof(primitiveT)) : 0));

        // Variable-sized stack arrays are non-standard, so use our own version
        // Have a max of 6 double/int64 elements = 48 bytes. Allocate 64 bytes for good measure
        vtkFoamStackVector<unsigned char, 64> buffer(tupleLength);
        primitiveT* tuple = reinterpret_cast<primitiveT*>(buffer.data());

        for (vtkTypeInt64 i = 0; i < nTuples; ++i)
        {
          io.ReadExpecting('(');
          io.Read(reinterpret_cast<unsigned char*>(tuple), tupleLength);
          io.ReadExpecting(')');
          this->Ptr->SetTuple(i, tuple);
        }
      }
      else
      {
        // Compiler hint for better unrolling:
        VTK_ASSUME(this->Ptr->GetNumberOfComponents() == nComponents);

        const unsigned tupleLength = (sizeof(primitiveT) * nComponents);
        primitiveT tuple[nComponents];

        for (vtkTypeInt64 i = 0; i < nTuples; ++i)
        {
          const int readLength = io.Read(reinterpret_cast<unsigned char*>(tuple), tupleLength);
          if (readLength != tupleLength)
          {
            throw vtkFoamError() << "Failed to read tuple " << i << '/' << nTuples << ": Expected "
                                 << tupleLength << " bytes, got " << readLength << " bytes.";
          }
          ::remapFoamTuple<nComponents == 6>(tuple); // For symmTensor
          for (int cmpt = 0; cmpt < nComponents; ++cmpt)
          {
            this->Ptr->SetTypedComponent(i, cmpt, static_cast<ValueType>(tuple[cmpt]));
          }
        }
      }
    }
  };
};

//------------------------------------------------------------------------------
// class vtkFoamEntryValue
// a class that represents a value of a dictionary entry that corresponds to
// its keyword. note that an entry can have more than one value.
struct vtkFoamEntryValue : public vtkFoamToken
{
private:
  typedef vtkFoamToken Superclass;

  bool IsUniformEntry;
  bool Managed;
  const vtkFoamEntry* UpperEntryPtr;

  vtkFoamEntryValue() = delete;
  vtkObjectBase* ToVTKObject() { return this->Superclass::VtkObjectPtr; }

  // Delete if managed
  void Clear();
  void ReadList(vtkFoamIOobject& io);

public:
  // Construct empty value with given parent
  explicit vtkFoamEntryValue(const vtkFoamEntry* parent)
    : IsUniformEntry(false)
    , Managed(true)
    , UpperEntryPtr(parent)
  {
  }
  // Copy construct
  vtkFoamEntryValue(vtkFoamEntryValue& val, const vtkFoamEntry* parent);
  ~vtkFoamEntryValue() { this->Clear(); }

  // Member Functions

  void SetEmptyList()
  {
    this->Clear();
    this->IsUniformEntry = false;
    this->Superclass::Type = vtkFoamToken::EMPTYLIST;
  }
  bool IsUniform() const noexcept { return this->IsUniformEntry; }
  bool Read(vtkFoamIOobject& io);
  void ReadDictionary(vtkFoamIOobject& io, const vtkFoamToken& firstKeyword);

  const vtkDataArray& LabelList() const { return *this->Superclass::LabelListPtr; }
  vtkDataArray& LabelList() { return *this->Superclass::LabelListPtr; }

  const vtkFoamLabelListList& LabelListList() const { return *this->Superclass::LabelListListPtr; }

  const vtkFloatArray& ScalarList() const { return *this->Superclass::ScalarListPtr; }
  vtkFloatArray& ScalarList() { return *this->Superclass::ScalarListPtr; }

  const vtkFloatArray& VectorList() const { return *this->Superclass::VectorListPtr; }
  vtkFloatArray& VectorList() { return *this->Superclass::VectorListPtr; }

  const vtkStringArray& StringList() const { return *this->Superclass::StringListPtr; }
  vtkStringArray& StringList() { return *this->Superclass::StringListPtr; }

  const vtkFoamDict& Dictionary() const { return *this->Superclass::DictPtr; }
  vtkFoamDict& Dictionary() { return *this->Superclass::DictPtr; }

  // Release the managed pointer, cast to specified type
  template <class DataType>
  DataType* ReleasePtr()
  {
    // Not managed = do not delete pointer in destructor
    this->Managed = false;
    return static_cast<DataType*>(this->Superclass::AnyPointer);
  }

  std::string ToString() const
  {
    return this->Superclass::Type == STRING ? this->Superclass::ToString() : std::string();
  }
  float ToFloat() const
  {
    return this->Superclass::IsNumeric() ? this->Superclass::To<float>() : 0.0F;
  }
  double ToDouble() const
  {
    return this->Superclass::IsNumeric() ? this->Superclass::To<double>() : 0.0;
  }
  // TODO is it ok to always use a 64bit int here?
  vtkTypeInt64 ToInt() const
  {
    return this->Superclass::Type == LABEL ? this->Superclass::To<vtkTypeInt64>() : 0;
  }

  void MakeLabelList(const vtkIdType len, const vtkTypeInt64 val = 0)
  {
    this->Superclass::Type = vtkFoamToken::LABELLIST;
    if (this->IsLabel64())
    {
      auto* array = vtkTypeInt64Array::New();
      array->SetNumberOfValues(len);
      array->FillValue(val);
      this->Superclass::LabelListPtr = array;
    }
    else
    {
      auto* array = vtkTypeInt32Array::New();
      array->SetNumberOfValues(len);
      array->FillValue(static_cast<vtkTypeInt32>(val));
      this->Superclass::LabelListPtr = array;
    }
  }

  void MakeScalarList(const vtkIdType len, const float val = 0.0f)
  {
    this->Superclass::Type = vtkFoamToken::SCALARLIST;
    this->Superclass::ScalarListPtr = vtkFloatArray::New();
    this->Superclass::ScalarListPtr->SetNumberOfValues(len);
    this->Superclass::ScalarListPtr->FillValue(val);
  }

  template <vtkFoamToken::tokenType listType, typename traitsT>
  void ReadNonUniformList(vtkFoamIOobject& io);

  // Dispatch reading of uniform list based on the field data type (scalar, vector etc).
  // Return false if could not be dispatched
  bool ReadNonUniformList(vtkFoamIOobject& io, vtkFoamTypes::dataType fieldDataType);

  bool ReadField(vtkFoamIOobject& io);

  // Read a list of labelLists. requires size prefix of the listList
  // to be present. size of each sublist must also be present in the
  // stream if the format is binary.
  void ReadLabelListList(vtkFoamIOobject& io);

  // Read compact labelListList which has offsets and data
  void ReadCompactLabelListList(vtkFoamIOobject& io);

  // Read dimensions set (always ASCII). The leading '[' has already been removed before calling.
  // - can be integer or floating point
  // - user-generated files may have only the first five dimensions.
  // Note
  // - may even have "human-readable" values such as [kg m^-1 s^-2] but they are very rare
  //   and we silently skip these
  void ReadDimensionSet(vtkFoamIOobject& io);
};

//------------------------------------------------------------------------------
// Code: vtkFoamEntryValue

// generic reader for nonuniform lists. requires size prefix of the
// list to be present in the stream if the format is binary.
template <vtkFoamToken::tokenType listType, typename traitsT>
void vtkFoamEntryValue::ReadNonUniformList(vtkFoamIOobject& io)
{
  this->SetStreamOption(io);

  vtkFoamToken currToken;
  currToken.SetStreamOption(io);
  if (!io.Read(currToken))
  {
    throw vtkFoamError() << "Unexpected EOF";
  }
  traitsT list;
  this->Superclass::Type = listType;
  this->Superclass::VtkObjectPtr = list.GetPointer();
  if (currToken.Is<vtkTypeInt64>())
  {
    const vtkTypeInt64 size = currToken.To<vtkTypeInt64>();
    if (size < 0)
    {
      throw vtkFoamError() << "List size must not be negative: size = " << size;
    }
    list->SetNumberOfTuples(size);
    if (io.IsAsciiFormat())
    {
      if (!io.Read(currToken))
      {
        throw vtkFoamError() << "Unexpected EOF";
      }
      // some objects have lists with only one element enclosed by {}
      // e. g. simpleFoam/pitzDaily3Blocks/constant/polyMesh/faceZones
      if (currToken == '{')
      {
        list.ReadUniformValues(io);
        io.ReadExpecting('}');
        return;
      }
      else if (currToken != '(')
      {
        throw vtkFoamError() << "Expected '(', found " << currToken;
      }
      list.ReadAsciiList(io);
      io.ReadExpecting(')');
    }
    else
    {
      if (size > 0)
      {
        // Non-empty (binary) list - only read parentheses only when size > 0
        io.ReadExpecting('(');
        list.ReadBinaryList(io);
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
    list->Squeeze();
  }
  else
  {
    throw vtkFoamError() << "Expected integer or '(', found " << currToken;
  }
}

// Dispatch known field/list types for non-uniform reading
bool vtkFoamEntryValue::ReadNonUniformList(vtkFoamIOobject& io, vtkFoamTypes::dataType listDataType)
{
  bool handled = true;
  switch (listDataType)
  {
    case vtkFoamTypes::BOOL_TYPE:
    {
      // List<bool> is read as a list of bytes (binary) or ints (ascii)
      // - primary location is the flipMap entry in faceZones

      this->ReadNonUniformList<BOOLLIST, //
        vtkFoamRead::listTraits<vtkTypeInt8Array, vtkTypeInt8>>(io);
      break;
    }

    case vtkFoamTypes::LABEL_TYPE:
    {
      if (io.IsLabel64())
      {
        this->ReadNonUniformList<LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt64Array, vtkTypeInt64>>(io);
      }
      else
      {
        this->ReadNonUniformList<LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt32Array, vtkTypeInt32>>(io);
      }
      break;
    }

    case vtkFoamTypes::SCALAR_TYPE:
    {
      if (io.IsFloat64())
      {
        this->ReadNonUniformList<SCALARLIST, //
          vtkFoamRead::listTraits<vtkFloatArray, double>>(io);
      }
      else
      {
        this->ReadNonUniformList<SCALARLIST, //
          vtkFoamRead::listTraits<vtkFloatArray, float>>(io);
      }
      break;
    }

    case vtkFoamTypes::SPH_TENSOR_TYPE:
    {
      if (io.IsFloat64())
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, double, 1>>(io);
      }
      else
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, float, 1>>(io);
      }
      break;
    }

    case vtkFoamTypes::VECTOR_TYPE:
    {
      if (io.IsFloat64())
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, double, 3>>(io);
      }
      else
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, float, 3>>(io);
      }
      break;
    }

    case vtkFoamTypes::SYMM_TENSOR_TYPE:
    {
      if (io.IsFloat64())
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, double, 6>>(io);
      }
      else
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, float, 6>>(io);
      }
      break;
    }

    case vtkFoamTypes::TENSOR_TYPE:
    {
      if (io.IsFloat64())
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, double, 9>>(io);
      }
      else
      {
        this->ReadNonUniformList<VECTORLIST, //
          vtkFoamRead::vectorListTraits<vtkFloatArray, float, 9>>(io);
      }
      break;
    }

    default:
    {
      handled = false;
      break;
    }
  }
  return handled;
}

bool vtkFoamEntryValue::ReadField(vtkFoamIOobject& io)
{
  this->SetStreamOption(io);

  // Basic field types: "boolField", "labelField", "scalarField" ...
  vtkFoamTypes::dataType listDataType(vtkFoamTypes::FieldToEnum(io.GetClassName()));

  try
  {
    if (vtkFoamTypes::IsGood(listDataType))
    {
      this->ReadNonUniformList(io, listDataType);
    }
    else
    {
      throw vtkFoamError() << "Unsupported field type " << io.GetClassName();
    }
  }
  catch (const vtkFoamError& err)
  {
    io.SetError(err);
    return false;
  }
  return true;
}

// Read a list of labelLists. requires size prefix of the listList
// to be present. size of each sublist must also be present in the
// stream if the format is binary.
void vtkFoamEntryValue::ReadLabelListList(vtkFoamIOobject& io)
{
  // NOTE:
  // when OpenFOAM writes a "faceCompactList" it automatically switches to ASCII
  // if it detects that the offsets will overflow 32bits.
  //
  // We risk the same overflow potential when constructing a compact labelListList.
  // Thus assume the worst and use 64bit sizing when reading ASCII.

  const bool use64BitLabels = (io.IsLabel64() || io.IsAsciiFormat());

  vtkFoamToken currToken;
  currToken.SetStreamOption(io);
  if (!io.Read(currToken))
  {
    throw vtkFoamError() << "Unexpected EOF";
  }
  if (currToken.IsLabel())
  {
    const vtkTypeInt64 listLen = currToken.To<vtkTypeInt64>();
    if (listLen < 0)
    {
      throw vtkFoamError() << "Illegal negative list length: " << listLen;
    }
    if (use64BitLabels)
    {
      this->LabelListListPtr = new vtkFoamLabelListList64;
    }
    else
    {
      this->LabelListListPtr = new vtkFoamLabelListList32;
    }
    // Initial guess for list length
    this->LabelListListPtr->ResizeExact(listLen, 4 * listLen);

    this->Superclass::Type = vtkFoamToken::LABELLISTLIST;
    io.ReadExpecting('(');
    vtkIdType nTotalElems = 0;
    for (vtkTypeInt64 idx = 0; idx < listLen; ++idx)
    {
      if (!io.Read(currToken))
      {
        throw vtkFoamError() << "Unexpected EOF";
      }
      if (currToken.IsLabel())
      {
        const vtkTypeInt64 sublistLen = currToken.To<vtkTypeInt64>();
        if (sublistLen < 0)
        {
          throw vtkFoamError() << "Illegal negative list length: " << sublistLen;
        }

        // LabelListListPtr->SetOffset(idx, nTotalElems);
        void* sublist = this->LabelListListPtr->WritePointer(idx, nTotalElems, sublistLen);

        if (io.IsAsciiFormat())
        {
          io.ReadExpecting('(');
          for (vtkTypeInt64 subIdx = 0; subIdx < sublistLen; ++subIdx)
          {
            vtkTypeInt64 value(vtkFoamReadValue<vtkTypeInt64>::ReadValue(io));
            this->LabelListListPtr->SetValue(idx, subIdx, value);
          }
          io.ReadExpecting(')');
        }
        else if (sublistLen > 0)
        {
          // Non-empty (binary) list - only read parentheses only when size > 0
          const size_t nbytes =
            static_cast<size_t>(sublistLen * this->LabelListListPtr->GetLabelSize());

          io.ReadExpecting('(');
          io.Read(reinterpret_cast<unsigned char*>(sublist), nbytes);
          io.ReadExpecting(')');
        }
        nTotalElems += sublistLen;
      }
      else if (currToken == '(')
      {
        this->Superclass::LabelListListPtr->SetOffset(idx, nTotalElems);
        while (io.Read(currToken) && currToken != ')')
        {
          if (!currToken.IsLabel())
          {
            throw vtkFoamError() << "Expected an integer, found " << currToken;
          }
          this->Superclass::LabelListListPtr->InsertValue(nTotalElems++, currToken.To<int>());
          ++nTotalElems;
        }
      }
      else
      {
        throw vtkFoamError() << "Expected integer or '(', found " << currToken;
      }
    }

    // Set the final offset
    this->Superclass::LabelListListPtr->SetOffset(listLen, nTotalElems);

    // Shrink to the actually used size
    this->Superclass::LabelListListPtr->ResizeData(nTotalElems);
    io.ReadExpecting(')');
  }
  else
  {
    throw vtkFoamError() << "Expected integer, found " << currToken;
  }
}

// Read compact labelListList which has offsets and data
void vtkFoamEntryValue::ReadCompactLabelListList(vtkFoamIOobject& io)
{
  if (io.IsAsciiFormat())
  {
    this->ReadLabelListList(io);
    return;
  }

  this->SetStreamOption(io);
  const bool use64BitLabels = io.IsLabel64();

  if (use64BitLabels)
  {
    this->LabelListListPtr = new vtkFoamLabelListList64;
  }
  else
  {
    this->LabelListListPtr = new vtkFoamLabelListList32;
  }
  this->Superclass::Type = vtkFoamToken::LABELLISTLIST;
  for (int arrayI = 0; arrayI < 2; arrayI++)
  {
    vtkFoamToken currToken;
    currToken.SetStreamOption(io);
    if (!io.Read(currToken))
    {
      throw vtkFoamError() << "Unexpected EOF";
    }
    if (currToken.IsLabel())
    {
      vtkTypeInt64 listLen = currToken.To<vtkTypeInt64>();
      if (listLen < 0)
      {
        throw vtkFoamError() << "Illegal negative list length: " << listLen;
      }

      vtkDataArray* array = (arrayI == 0 ? this->Superclass::LabelListListPtr->GetOffsetsArray()
                                         : this->Superclass::LabelListListPtr->GetDataArray());
      array->SetNumberOfValues(static_cast<vtkIdType>(listLen));

      if (listLen > 0)
      {
        // Non-empty (binary) list - only read parentheses only when size > 0

        io.ReadExpecting('('); // Begin list
        io.Read(reinterpret_cast<unsigned char*>(array->GetVoidPointer(0)),
          static_cast<vtkTypeInt64>(listLen * array->GetDataTypeSize()));
        io.ReadExpecting(')'); // End list
      }
    }
    else
    {
      throw vtkFoamError() << "Expected integer, found " << currToken;
    }
  }
}

// Read dimensions set (always ASCII). The leading '[' has already been removed before calling.
// - can be integer or floating point
// - user-generated files may have only the first five dimensions.
// Note
// - may even have "human-readable" values such as [kg m^-1 s^-2] but they are very rare
//   and we silently skip these
void vtkFoamEntryValue::ReadDimensionSet(vtkFoamIOobject& io)
{
  const int nDimensions = 7; // There are 7 base dimensions
  this->MakeScalarList(nDimensions, 0.0);
  vtkFloatArray& dims = *(this->Superclass::ScalarListPtr);

  // Read using tokenizer to handle scalar/label, variable lengths, and ignore human-readable
  vtkFoamToken tok;
  char expectEnding = ']';
  bool goodInput = true;

  for (int ndims = 0; ndims < nDimensions && goodInput && expectEnding; ++ndims)
  {
    if (!io.Read(tok))
    {
      goodInput = false;
    }
    else if (tok.IsNumeric())
    {
      dims.SetValue(ndims, tok.ToFloat());
    }
    else if (tok.IsPunctuation())
    {
      if (tok == expectEnding)
      {
        expectEnding = '\0'; // Already got the closing ']'
      }
      else
      {
        goodInput = false;
      }
    }
    else
    {
      // Some unknown token type (eg, encountered human-readable units)
      // - skip until ']'
      while ((goodInput = io.Read(tok)))
      {
        if (tok.IsPunctuation() && (tok == expectEnding))
        {
          expectEnding = '\0'; // Already got the closing ']'
          break;
        }
      }
      break;
    }
  }

  if (!goodInput)
  {
    io.ThrowStackTrace("Unexpected input while parsing dimensions array");
  }
  else if (expectEnding)
  {
    io.ReadExpecting(expectEnding);
  }
}

//------------------------------------------------------------------------------
// class vtkFoamEntry
// a class that represents an entry of a dictionary. note that an
// entry can have more than one value.
struct vtkFoamEntry : public vtkFoamPtrList<vtkFoamEntryValue>
{
private:
  typedef vtkFoamPtrList<vtkFoamEntryValue> Superclass;
  std::string Keyword;
  vtkFoamDict* UpperDictPtr;

  vtkFoamEntry() = delete;

public:
  vtkFoamEntry(vtkFoamDict* upperDictPtr)
    : UpperDictPtr(upperDictPtr)
  {
  }
  vtkFoamEntry(const vtkFoamEntry& entry, vtkFoamDict* upperDictPtr)
    : Superclass(entry.size())
    , Keyword(entry.GetKeyword())
    , UpperDictPtr(upperDictPtr)
  {
    for (size_t i = 0; i < entry.size(); ++i)
    {
      (*this)[i] = new vtkFoamEntryValue(*entry[i], this);
    }
  }

  ~vtkFoamEntry() = default;

  void Clear() { this->Superclass::clear(); }

  const std::string& GetKeyword() const { return this->Keyword; }
  void SetKeyword(const std::string& keyword) { this->Keyword = keyword; }

  const vtkFoamEntryValue& FirstValue() const { return *this->Superclass::operator[](0); }
  vtkFoamEntryValue& FirstValue() { return *this->Superclass::operator[](0); }

  const vtkDataArray& LabelList() const { return this->FirstValue().LabelList(); }
  vtkDataArray& LabelList() { return this->FirstValue().LabelList(); }

  const vtkFoamLabelListList& LabelListList() const { return this->FirstValue().LabelListList(); }

  const vtkFloatArray& ScalarList() const { return this->FirstValue().ScalarList(); }
  vtkFloatArray& ScalarList() { return this->FirstValue().ScalarList(); }

  const vtkFloatArray& VectorList() const { return this->FirstValue().VectorList(); }
  vtkFloatArray& VectorList() { return this->FirstValue().VectorList(); }

  const vtkFoamDict& Dictionary() const { return this->FirstValue().Dictionary(); }
  vtkFoamDict& Dictionary() { return this->FirstValue().Dictionary(); }
  const vtkFoamDict* GetUpperDictPtr() const { return this->UpperDictPtr; }

  template <class DataType>
  DataType* ReleasePtr()
  {
    return this->FirstValue().ReleasePtr<DataType>();
  }

  std::string ToString() const
  {
    return this->empty() ? std::string() : this->FirstValue().ToString();
  }
  float ToFloat() const { return this->empty() ? 0.0F : this->FirstValue().ToFloat(); }
  double ToDouble() const { return this->empty() ? 0.0 : this->FirstValue().ToDouble(); }
  vtkTypeInt64 ToInt() const { return this->empty() ? 0 : this->FirstValue().ToInt(); }

  void ReadDictionary(vtkFoamIOobject& io)
  {
    this->Superclass::push_back(new vtkFoamEntryValue(this));
    this->Superclass::back()->ReadDictionary(io, vtkFoamToken());
  }

  // read values of an entry
  void Read(vtkFoamIOobject& io);
};

//------------------------------------------------------------------------------
// Code: vtkFoamDict

// Copy construct
vtkFoamDict::vtkFoamDict(const vtkFoamDict& dict, const vtkFoamDict* upperDictPtr)
  : Superclass(dict.size())
  , UpperDictPtr(upperDictPtr)
{
  if (dict.GetType() == vtkFoamToken::DICTIONARY)
  {
    for (size_t i = 0; i < dict.size(); ++i)
    {
      (*this)[i] = new vtkFoamEntry(*dict[i], this);
    }
  }
  else
  {
    Superclass::assign(dict.size(), nullptr);
  }
}

// Destructor
vtkFoamDict::~vtkFoamDict()
{
  if (this->Token.GetType() == vtkFoamToken::UNDEFINED)
  {
    for (auto* ptr : *this)
    {
      delete ptr;
    }
  }
}

// Remove top element, deleting its pointer
void vtkFoamDict::remove_back()
{
  if (!Superclass::empty())
  {
    delete Superclass::back();
    Superclass::pop_back();
  }
}

// Return list of keywords - table of contents
std::vector<std::string> vtkFoamDict::Toc() const
{
  std::vector<std::string> list;
  list.reserve(this->size());
  for (const vtkFoamEntry* eptr : *this)
  {
    const std::string& key = eptr->GetKeyword();
    if (!key.empty()) // should not really happen anyhow
    {
      list.push_back(key);
    }
  }
  return list;
}

// Search dictionary for specified keyword. Return nullptr on failure.
vtkFoamEntry* vtkFoamDict::Lookup(const std::string& keyword, const bool isPattern) const
{
  if (this->Token.GetType() == vtkFoamToken::UNDEFINED)
  {
    int lastMatch = -1;
    for (size_t i = 0; i < this->Superclass::size(); i++)
    {
      const std::string& key = this->operator[](i)->GetKeyword();
      vtksys::RegularExpression rex;
      if (key == keyword) // found
      {
        return this->operator[](i);
      }
      else if (isPattern && rex.compile(key) && rex.find(keyword) && rex.start(0) == 0 &&
        rex.end(0) == keyword.size())
      {
        // regular expression matches full keyword
        lastMatch = static_cast<int>(i);
      }
    }
    if (lastMatch >= 0)
    {
      return this->operator[](lastMatch);
    }
  }

  // Not found
  return nullptr;
}

// Reads a FoamFile or a subdictionary.
// If the stream to be read is a subdictionary,
// the preceding '{' is assumed to have already been discarded.
bool vtkFoamDict::Read(vtkFoamIOobject& io, const bool isSubDict, const vtkFoamToken& firstKeyword)
{
  try
  {
    vtkFoamToken currToken;
    currToken.SetStreamOption(io);

    if (firstKeyword.GetType() == vtkFoamToken::UNDEFINED)
    {
      // read the first token
      if (!io.Read(currToken))
      {
        throw vtkFoamError() << "Unexpected EOF";
      }

      if (isSubDict)
      {
        // the following if clause is for an exceptional expression
        // of `LABEL{numeric}' without type prefix
        // (e. g. `2{-0}' in mixedRhoE B.C. in
        // rhopSonicFoam/shockTube)
        if (currToken.IsNumeric())
        {
          this->Token = currToken;
          io.ReadExpecting('}');
          return true;
        }

        // return as empty dictionary
        else if (currToken == '}')
        {
          return true;
        }
      }
      else
      {
        // list of dictionaries is read as a usual dictionary
        // polyMesh/boundary, point/face/cell-Zones
        if (currToken.IsLabel())
        {
          io.ReadExpecting('(');
          if (currToken.To<vtkTypeInt64>() > 0)
          {
            if (!io.Read(currToken))
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

        else if (currToken == '(' && io.GetClassName() == "polyBoundaryMesh") // polyMesh/boundary
        {
          if (!io.Read(currToken)) // read the first keyword
          {
            throw vtkFoamError() << "Unexpected EOF";
          }

          if (currToken == ')') // return as empty dictionary
          {
            return true;
          }
        }
      }
    }
    // if firstKeyword is given as string read the following stream as
    // subdictionary

    else if (firstKeyword.GetType() == vtkFoamToken::STRING)
    {
      this->Superclass::push_back(new vtkFoamEntry(this));
      this->Superclass::back()->SetKeyword(firstKeyword.ToString());
      this->Superclass::back()->ReadDictionary(io);
      if (!io.Read(currToken) || currToken == '}' || currToken == ')')
      {
        return true;
      }
    }
    else // quite likely an identifier
    {
      currToken = firstKeyword;
    }

    if (currToken == ';' || currToken.IsStringType())
    {
      // general dictionary
      do
      {
        if (currToken.GetType() == vtkFoamToken::STRING)
        {
          vtkFoamEntry* previousEntry = this->Lookup(currToken.ToString());
          if (previousEntry != nullptr)
          {
            if (io.GetInputMode() == vtkFoamFile::INPUT_MODE_MERGE)
            {
              if (previousEntry->FirstValue().GetType() == vtkFoamToken::DICTIONARY)
              {
                io.ReadExpecting('{');
                previousEntry->FirstValue().Dictionary().Read(io, true); // Read as sub-dict
              }
              else
              {
                previousEntry->Clear();
                previousEntry->Read(io);
              }
            }
            else if (io.GetInputMode() == vtkFoamFile::INPUT_MODE_OVERWRITE)
            {
              previousEntry->Clear();
              previousEntry->Read(io);
            }
            else // INPUT_MODE_ERROR
            {
              throw vtkFoamError()
                << "Found duplicated entries with keyword " << currToken.ToString();
            }
          }
          else
          {
            this->Superclass::push_back(new vtkFoamEntry(this));
            this->Superclass::back()->SetKeyword(currToken.ToString());
            this->Superclass::back()->Read(io);
          }

          if (currToken == "FoamFile")
          {
            // Drop the FoamFile header subdictionary entry
            this->remove_back();
          }
        }
        else if (currToken.GetType() == vtkFoamToken::IDENTIFIER)
        {
          // substitute identifier
          const std::string identifier(currToken.ToIdentifier());

          for (const vtkFoamDict* uDictPtr = this;;)
          {
            const vtkFoamEntry* identifiedEntry = uDictPtr->Lookup(identifier);

            if (identifiedEntry != nullptr)
            {
              if (identifiedEntry->FirstValue().GetType() != vtkFoamToken::DICTIONARY)
              {
                throw vtkFoamError() << "Expected dictionary for substituting entry " << identifier;
              }
              const vtkFoamDict& identifiedDict = identifiedEntry->FirstValue().Dictionary();
              for (size_t entryI = 0; entryI < identifiedDict.size(); entryI++)
              {
                // I think #inputMode handling should be done here
                // as well, but the genuine FoamFile parser for OF
                // 1.5 does not seem to be doing it.
                this->Superclass::push_back(new vtkFoamEntry(*identifiedDict[entryI], this));
              }
              break;
            }
            else
            {
              uDictPtr = uDictPtr->GetUpperDictPtr();
              if (uDictPtr == nullptr)
              {
                throw vtkFoamError() << "Substituting entry " << identifier << " not found";
              }
            }
          }
        }
        // skip empty entry only with ';'
      } while (io.Read(currToken) && (currToken.IsStringType() || currToken == ';'));

      if (currToken.GetType() == vtkFoamToken::TOKEN_ERROR || currToken == '}' || currToken == ')')
      {
        return true;
      }
      throw vtkFoamError() << "Expected keyword, closing brace, ';' or EOF, found " << currToken;
    }
    throw vtkFoamError() << "Expected keyword or identifier, found " << currToken;
  }
  catch (const vtkFoamError& err)
  {
    if (isSubDict)
    {
      throw;
    }
    else
    {
      io.SetError(err);
      return false;
    }
  }
}

//------------------------------------------------------------------------------
// Code: vtkFoamIOobject

void vtkFoamIOobject::ReadHeader()
{
  this->Superclass::ReadExpecting("FoamFile");
  this->Superclass::ReadExpecting('{');

  vtkFoamDict headerDict;
  headerDict.SetStreamOption(this->GetStreamOption());
  headerDict.Read(*this, true); // Read as sub-dict. Throw exception in case of error

  const vtkFoamEntry* eptr;

  // Essentials
  if ((eptr = headerDict.Lookup("class")) == nullptr)
  {
    throw vtkFoamError() << "No 'class' in FoamFile header";
  }
  this->headerClassName_ = eptr->ToString();

  if ((eptr = headerDict.Lookup("object")) == nullptr)
  {
    throw vtkFoamError() << "No 'object' in FoamFile header";
  }
  this->objectName_ = eptr->ToString();

  if ((eptr = headerDict.Lookup("format")) == nullptr)
  {
    // Note (2021-03-19): may make this optional in the future, defaulting to ascii
    throw vtkFoamError() << "No 'format' (ascii|binary) in FoamFile header";
  }
  this->SetBinaryFormat("binary" == eptr->ToString()); // case sensitive

  // The arch entry has "label=(32|64) scalar=(32|64)"
  // If missing/incomplete, use fallback values from reader (defined in constructor and Close)
  if ((eptr = headerDict.Lookup("arch")) != nullptr)
  {
    const std::string archValue(eptr->ToString());

    // Match "label=(32|64)"
    {
      auto pos = archValue.find("label=");
      if (pos != std::string::npos)
      {
        pos += 6; // Skip past "label="
        if (archValue.compare(pos, 2, "32") == 0)
        {
          this->SetLabel64(false);
        }
        else if (archValue.compare(pos, 2, "64") == 0)
        {
          this->SetLabel64(true);
        }
      }
    }

    // Match "scalar=(32|64)"
    {
      auto pos = archValue.find("scalar=");
      if (pos != std::string::npos)
      {
        pos += 7; // Skip past "scalar="
        if (archValue.compare(pos, 2, "32") == 0)
        {
          this->SetFloat64(false);
        }
        else if (archValue.compare(pos, 2, "64") == 0)
        {
          this->SetFloat64(true);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Code: vtkFoamEntryValue

vtkFoamEntryValue::vtkFoamEntryValue(vtkFoamEntryValue& value, const vtkFoamEntry* parent)
  : vtkFoamToken(value)
  , IsUniformEntry(value.IsUniform())
  , Managed(true)
  , UpperEntryPtr(parent)
{
  switch (this->Superclass::Type)
  {
    case BOOLLIST:
    case LABELLIST:
    case SCALARLIST:
    case VECTORLIST:
    case STRINGLIST:
    {
      this->Superclass::VtkObjectPtr = value.ToVTKObject();
      this->Superclass::VtkObjectPtr->Register(nullptr);
      break;
    }
    case LABELLISTLIST:
    {
      if (value.LabelListListPtr->IsLabel64())
      {
        this->LabelListListPtr = new vtkFoamLabelListList64(*value.LabelListListPtr);
      }
      else
      {
        this->LabelListListPtr = new vtkFoamLabelListList32(*value.LabelListListPtr);
      }
      break;
    }
    case ENTRYVALUELIST:
    {
      const size_t nValues = value.EntryValuePtrs->size();
      this->EntryValuePtrs = new vtkFoamPtrList<vtkFoamEntryValue>(nValues);
      for (size_t valueI = 0; valueI < nValues; valueI++)
      {
        this->EntryValuePtrs->operator[](valueI) =
          new vtkFoamEntryValue(*value.EntryValuePtrs->operator[](valueI), this->UpperEntryPtr);
      }
      break;
    }
    case DICTIONARY:
    {
      // UpperEntryPtr is null when called from vtkFoamDict constructor
      if (this->UpperEntryPtr == nullptr)
      {
        this->DictPtr = nullptr;
      }
      else
      {
        this->DictPtr = new vtkFoamDict(*value.DictPtr, this->UpperEntryPtr->GetUpperDictPtr());
        this->DictPtr->SetStreamOption(value.GetStreamOption());
      }
      break;
    }
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
      case BOOLLIST:
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
        delete this->EntryValuePtrs;
        break;
      case DICTIONARY:
        delete this->DictPtr;
        break;
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
  this->SetStreamOption(io);

  vtkFoamToken currToken;
  currToken.SetStreamOption(io);
  io.Read(currToken);

  if (currToken.IsLabel())
  {
    // Use lookahead to guess the list type.
    // if the first token is of type LABEL it might be either an element of
    // a labelList or the size of a sublist so proceed to the next token
    vtkFoamToken nextToken;
    nextToken.SetStreamOption(io);
    if (!io.Read(nextToken))
    {
      throw vtkFoamError() << "Unexpected EOF";
    }

    if (nextToken.IsPunctuation('('))
    {
      // A List of List: read recursively
      this->Superclass::EntryValuePtrs = new vtkFoamPtrList<vtkFoamEntryValue>;
      this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->SetStreamOption(*this);
      this->Superclass::EntryValuePtrs->back()->ReadList(io);
      this->Superclass::Type = vtkFoamToken::ENTRYVALUELIST;
    }
    else if (nextToken.IsPunctuation(')'))
    {
      // List with only one label element. Eg "(5)"
      this->MakeLabelList(1, currToken.To<vtkTypeInt64>());
      return; // DONE
    }
    else if (nextToken.IsLabel())
    {
      // Start of a list of labels
      if (this->IsLabel64())
      {
        auto* array = vtkTypeInt64Array::New();
        array->InsertNextValue(currToken.To<vtkTypeInt64>());
        array->InsertNextValue(nextToken.To<vtkTypeInt64>());
        this->Superclass::LabelListPtr = array;
      }
      else
      {
        auto* array = vtkTypeInt32Array::New();
        array->InsertNextValue(currToken.To<vtkTypeInt32>());
        array->InsertNextValue(nextToken.To<vtkTypeInt32>());
        this->Superclass::LabelListPtr = array;
      }
      this->Superclass::Type = vtkFoamToken::LABELLIST;
    }
    else if (nextToken.IsScalar())
    {
      // Start of a list of scalars
      this->Superclass::ScalarListPtr = vtkFloatArray::New();
      this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
      this->Superclass::ScalarListPtr->InsertNextValue(nextToken.To<float>());
      this->Superclass::Type = vtkFoamToken::SCALARLIST;
    }
    else
    {
      throw vtkFoamError() << "Expected number, '(' or ')', found " << nextToken;
    }
  }
  else if (currToken.IsScalar())
  {
    // The first element of a scalar list
    this->Superclass::ScalarListPtr = vtkFloatArray::New();
    this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
    this->Superclass::Type = vtkFoamToken::SCALARLIST;
  }
  else if (currToken.GetType() == vtkFoamToken::STRING)
  {
    // if the first word is a string we have to read another token to determine
    // if it is a keyword for the following dictionary

    vtkFoamToken nextToken;
    nextToken.SetStreamOption(io);
    if (!io.Read(nextToken))
    {
      throw vtkFoamError() << "Unexpected EOF";
    }

    if (nextToken.IsPunctuation('{'))
    {
      // Dictionary. Use previously read stringToken as the first keyword
      if (currToken.ToString().empty())
      {
        throw "Empty string is invalid as a keyword for dictionary entry";
      }
      this->ReadDictionary(io, currToken);
      // the dictionary read as list has the entry terminator ';' so
      // we have to skip it
      return; // DONE
    }
    else if (nextToken.IsPunctuation(')'))
    {
      // List with only one string element. Eg "(wall)"
      this->Superclass::StringListPtr = vtkStringArray::New();
      this->Superclass::StringListPtr->SetNumberOfValues(1);
      this->Superclass::StringListPtr->SetValue(0, currToken.ToString());
      this->Superclass::Type = vtkFoamToken::STRINGLIST;
      return; // DONE
    }
    else if (nextToken.GetType() == vtkFoamToken::STRING) // list of strings
    {
      this->Superclass::StringListPtr = vtkStringArray::New();
      this->Superclass::StringListPtr->InsertNextValue(currToken.ToString());
      this->Superclass::StringListPtr->InsertNextValue(nextToken.ToString());
      this->Superclass::Type = vtkFoamToken::STRINGLIST;
    }
    else
    {
      throw vtkFoamError() << "Expected string, '{' or ')', found " << nextToken;
    }
  }
  else if (currToken == '(' || currToken == '{')
  {
    // List of lists or dictionaries: read recursively
    this->Superclass::EntryValuePtrs = new vtkFoamPtrList<vtkFoamEntryValue>;
    this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(this->UpperEntryPtr));
    this->Superclass::EntryValuePtrs->back()->SetStreamOption(io);
    if (currToken == '(')
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
      this->Superclass::EntryValuePtrs->push_back(new vtkFoamEntryValue(this->UpperEntryPtr));
      this->Superclass::EntryValuePtrs->back()->SetStreamOption(io);
      this->Superclass::EntryValuePtrs->back()->Read(io);
    } while (*this->Superclass::EntryValuePtrs->back() != ')' &&
      *this->Superclass::EntryValuePtrs->back() != '}' &&
      *this->Superclass::EntryValuePtrs->back() != ';');

    if (*this->Superclass::EntryValuePtrs->back() != ')')
    {
      throw vtkFoamError() << "Expected ')' before " << *this->Superclass::EntryValuePtrs->back();
    }

    // Drop ')'
    this->Superclass::EntryValuePtrs->remove_back();
    this->Superclass::Type = vtkFoamToken::ENTRYVALUELIST;
    return; // DONE
  }
  else if (currToken.IsPunctuation(')'))
  {
    // Empty list
    this->Superclass::Type = vtkFoamToken::EMPTYLIST;
    return; // DONE
  }
  // FIXME: may (or may not) need identifier handling

  while (io.Read(currToken) && !currToken.IsPunctuation(')'))
  {
    if (this->Superclass::Type == vtkFoamToken::LABELLIST)
    {
      if (currToken.GetType() == vtkFoamToken::SCALAR)
      {
        // Encountered a scalar while reading a labelList - switch representation
        // Need intermediate pointer since LabelListPtr and ScalarListPtr are in a union

        vtkDataArray* labels = this->LabelListPtr;
        const vtkIdType currLen = labels->GetNumberOfTuples();
        const bool use64BitLabels = ::Is64BitArray(labels); // <- Same as io.IsLabel64()

        // Copy, with append
        auto* scalars = vtkFloatArray::New();
        scalars->SetNumberOfValues(currLen + 1);
        for (vtkIdType i = 0; i < currLen; ++i)
        {
          scalars->SetValue(i, static_cast<float>(GetLabelValue(labels, i, use64BitLabels)));
        }
        scalars->SetValue(currLen, currToken.To<float>()); // Append value

        // Replace
        labels->Delete();
        this->Superclass::ScalarListPtr = scalars;
        this->Superclass::Type = vtkFoamToken::SCALARLIST;
      }
      else if (currToken.IsLabel())
      {
        if (currToken.IsLabel64())
        {
          assert(vtkTypeInt64Array::FastDownCast(this->LabelListPtr) != nullptr);
          static_cast<vtkTypeInt64Array*>(this->LabelListPtr)
            ->InsertNextValue(currToken.To<vtkTypeInt64>());
        }
        else
        {
          assert(vtkTypeInt32Array::FastDownCast(this->LabelListPtr) != nullptr);
          static_cast<vtkTypeInt32Array*>(this->LabelListPtr)
            ->InsertNextValue(currToken.To<vtkTypeInt32>());
        }
      }
      else
      {
        throw vtkFoamError() << "Expected a number, found " << currToken;
      }
    }
    else if (this->Superclass::Type == vtkFoamToken::SCALARLIST)
    {
      if (currToken.IsNumeric())
      {
        this->Superclass::ScalarListPtr->InsertNextValue(currToken.To<float>());
      }
      else if (currToken == '(')
      {
        vtkDebugWithObjectMacro(nullptr,
          "Found a list containing scalar data followed "
          "by a nested list, but this reader only "
          "supports nested lists that precede all "
          "scalars. Discarding nested list data.");
        vtkFoamEntryValue tmp(this->UpperEntryPtr);
        tmp.SetStreamOption(io);
        tmp.ReadList(io);
      }
      else
      {
        throw vtkFoamError() << "Expected a number, found " << currToken;
      }
    }
    else if (this->Superclass::Type == vtkFoamToken::STRINGLIST)
    {
      if (currToken.GetType() == vtkFoamToken::STRING)
      {
        this->Superclass::StringListPtr->InsertNextValue(currToken.ToString());
      }
      else
      {
        throw vtkFoamError() << "Expected a string, found " << currToken;
      }
    }
    else if (this->Superclass::Type == vtkFoamToken::ENTRYVALUELIST)
    {
      if (currToken.IsLabel())
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

  if (this->Superclass::Type == vtkFoamToken::BOOLLIST)
  {
    this->Superclass::BoolListPtr->Squeeze();
  }
  else if (this->Superclass::Type == vtkFoamToken::LABELLIST)
  {
    this->Superclass::LabelListPtr->Squeeze();
  }
  else if (this->Superclass::Type == vtkFoamToken::SCALARLIST)
  {
    this->Superclass::ScalarListPtr->Squeeze();
  }
  else if (this->Superclass::Type == vtkFoamToken::STRINGLIST)
  {
    this->Superclass::StringListPtr->Squeeze();
  }
}

// a list of dictionaries is actually read as a dictionary
void vtkFoamEntryValue::ReadDictionary(vtkFoamIOobject& io, const vtkFoamToken& firstKeyword)
{
  this->Superclass::DictPtr = new vtkFoamDict(this->UpperEntryPtr->GetUpperDictPtr());
  this->DictPtr->SetStreamOption(io);
  this->Superclass::Type = vtkFoamToken::DICTIONARY;
  this->Superclass::DictPtr->Read(io, true, firstKeyword);
}

// Guess the type of the given entry value and read it
// Return:
// - true on success
// - false if end of entry (';') encountered during parsing composite entry value
bool vtkFoamEntryValue::Read(vtkFoamIOobject& io)
{
  this->SetStreamOption(io);

  vtkFoamToken currToken;
  currToken.SetStreamOption(io);
  if (!io.Read(currToken))
  {
    throw vtkFoamError() << "Unexpected EOF";
  }

  // List types
  if (currToken == '{')
  {
    this->ReadDictionary(io, vtkFoamToken());
    return true;
  }
  else if (currToken == '(')
  {
    this->ReadList(io);
    return true;
  }
  else if (currToken == '[')
  {
    this->ReadDimensionSet(io);
    return true;
  }

  vtkFoamTypes::dataType listDataType(vtkFoamTypes::NO_TYPE);

  if (currToken == "uniform")
  {
    if (!io.Read(currToken))
    {
      throw vtkFoamError() << "Expected a uniform value or a list, found unexpected EOF";
    }
    if (currToken == '(')
    {
      this->ReadList(io);
    }
    else if (currToken == ';')
    {
      this->Superclass::operator=("uniform");
      return false;
    }
    else if (currToken.IsNumeric() || currToken.GetType() == vtkFoamToken::STRING)
    {
      this->Superclass::operator=(currToken);
    }
    else // unexpected punctuation token
    {
      throw vtkFoamError() << "Expected number, string or ( for uniform entry, found " << currToken;
    }
    this->IsUniformEntry = true;
  }
  else if (currToken == "nonuniform")
  {
    if (!io.Read(currToken))
    {
      throw vtkFoamError() << "Expected list type specifier for nonuniform entry, found EOF";
    }
    this->IsUniformEntry = false;

    if (currToken.GetType() == vtkFoamToken::STRING)
    {
      // List types: "List<label>", "List<scalar>" ...
      listDataType = vtkFoamTypes::ListToEnum(currToken.ToString());
    }
    if (vtkFoamTypes::IsGood(listDataType))
    {
      this->ReadNonUniformList(io, listDataType);
    }
    else if (currToken.IsLabel(0))
    {
      // An empty list doesn't have a list type specifier
      this->Superclass::Type = vtkFoamToken::EMPTYLIST;
      if (io.IsAsciiFormat())
      {
        io.ReadExpecting('(');
        io.ReadExpecting(')');
      }
    }
    else if (currToken == ';')
    {
      this->Superclass::operator=("nonuniform");
      return false;
    }
    else
    {
      throw vtkFoamError() << "Unsupported nonuniform list type " << currToken;
    }
  }
  else if (currToken.GetType() == vtkFoamToken::STRING &&
    (listDataType = vtkFoamTypes::ListToEnum(currToken.ToString())) != vtkFoamTypes::NO_TYPE)
  {
    // Lists without a uniform/nonuniform keyword - eg, zones
    this->IsUniformEntry = false;
    this->ReadNonUniformList(io, listDataType);
  }
  else if (currToken.IsPunctuation() || currToken.IsNumeric() || currToken.IsStringType())
  {
    this->Superclass::operator=(currToken);
  }

  return true;
}

// read values of an entry
void vtkFoamEntry::Read(vtkFoamIOobject& io)
{
  while (true)
  {
    this->Superclass::push_back(new vtkFoamEntryValue(this));
    this->Superclass::back()->SetStreamOption(io);
    if (!this->Superclass::back()->Read(io))
    {
      break;
    }

    if (this->Superclass::size() >= 2)
    {
      vtkFoamEntryValue& secondLastValue =
        *this->Superclass::operator[](this->Superclass::size() - 2);
      if (secondLastValue.IsLabel())
      {
        const vtkTypeInt64 listLen = secondLastValue.To<vtkTypeInt64>();
        vtkFoamEntryValue& lastValue = *this->Superclass::back();

        // a zero-sized nonuniform list without prefixing "nonuniform"
        // keyword nor list type specifier (i. e. `0()';
        // e. g. simpleEngine/0/polyMesh/pointZones) requires special
        // care (one with nonuniform prefix is treated within
        // vtkFoamEntryValue::read()). still this causes erroneous
        // behavior for `0 nonuniform 0()' but this should be extremely
        // rare
        if (lastValue.GetType() == vtkFoamToken::EMPTYLIST && listLen == 0)
        {
          // Remove last value, and mark new last value as EMPTYLIST
          this->remove_back();
          this->Superclass::back()->SetEmptyList();
        }

        // for an exceptional expression of `LABEL{LABELorSCALAR}' without
        // type prefix (e. g. `2{-0}' in mixedRhoE B.C. in
        // rhopSonicFoam/shockTube)
        else if (lastValue.GetType() == vtkFoamToken::DICTIONARY)
        {
          if (lastValue.Dictionary().GetType() == vtkFoamToken::LABEL)
          {
            const vtkTypeInt64 val = lastValue.Dictionary().GetToken().ToInt();
            // Remove the last two values
            this->remove_back();
            this->remove_back();
            // Make new labelList
            this->Superclass::push_back(new vtkFoamEntryValue(this));
            this->Superclass::back()->SetStreamOption(io);
            this->Superclass::back()->MakeLabelList(listLen, val);
          }
          else if (lastValue.Dictionary().GetType() == vtkFoamToken::SCALAR)
          {
            const float val = lastValue.Dictionary().GetToken().ToFloat();
            // Remove the last two values
            this->remove_back();
            this->remove_back();
            // Make new scalarList
            this->Superclass::push_back(new vtkFoamEntryValue(this));
            this->Superclass::back()->SetStreamOption(io);
            this->Superclass::back()->MakeScalarList(listLen, val);
          }
        }

        // Cleanup for tokenized string lists, the parser will handle something like
        //
        //     keyword  2(abc def);
        //
        // as  LABEL + STRINGLIST : so drop the label when the sizes are identical
        // limit the fixup to entries that contains no other tokens (2021-03-19)
        else if (lastValue.GetType() == vtkFoamToken::STRINGLIST)
        {
          if (listLen == lastValue.StringList().GetNumberOfValues() &&
            this->Superclass::size() == 2)
          {
            // The label (list size parsing remnant) can be removed.
            std::swap(this->Superclass::operator[](this->Superclass::size() - 1),
              this->Superclass::operator[](this->Superclass::size() - 2));
            this->remove_back();
          }
        }
      }
    }

    if (this->Superclass::back()->GetType() == vtkFoamToken::IDENTIFIER)
    {
      // Substitute identifier
      const std::string identifier(this->Superclass::back()->ToIdentifier());
      this->remove_back();

      for (const vtkFoamDict* uDictPtr = this->UpperDictPtr;;)
      {
        const vtkFoamEntry* identifiedEntry = uDictPtr->Lookup(identifier);

        if (identifiedEntry != nullptr)
        {
          for (size_t valueI = 0; valueI < identifiedEntry->size(); valueI++)
          {
            this->Superclass::push_back(
              new vtkFoamEntryValue(*identifiedEntry->operator[](valueI), this));
            this->back()->SetStreamOption(io);
          }
          break;
        }
        else
        {
          uDictPtr = uDictPtr->GetUpperDictPtr();
          if (uDictPtr == nullptr)
          {
            throw vtkFoamError() << "substituting entry " << identifier << " not found";
          }
        }
      }
    }
    else if (*this->Superclass::back() == ';')
    {
      // Drop entry terminator
      this->remove_back();
      break;
    }
    else if (this->Superclass::back()->GetType() == vtkFoamToken::DICTIONARY)
    {
      // subdictionary is not suffixed by an entry terminator ';'
      break;
    }
    else if (*this->Superclass::back() == '}' || *this->Superclass::back() == ')')
    {
      throw vtkFoamError() << "Unmatched " << *this->Superclass::back();
    }
  }
}

//------------------------------------------------------------------------------
// vtkOpenFOAMReaderPrivate constructor and destructor
vtkOpenFOAMReaderPrivate::vtkOpenFOAMReaderPrivate()
{
  // Time information
  this->TimeValues = vtkDoubleArray::New();
  this->TimeNames = vtkStringArray::New();

  this->TimeStep = 0;
  this->TimeStepOld = TIMEINDEX_UNVISITED;
  this->TopologyTimeIndex = TIMEINDEX_UNVISITED;

  // Selection
  this->InternalMeshSelectionStatus = 0;
  this->InternalMeshSelectionStatusOld = 0;

  // Mesh dimensions
  this->NumPoints = 0;
  this->NumInternalFaces = 0;
  this->NumFaces = 0;
  this->NumCells = 0;

  this->VolFieldFiles = vtkStringArray::New();
  this->DimFieldFiles = vtkStringArray::New();
  this->AreaFieldFiles = vtkStringArray::New();
  this->PointFieldFiles = vtkStringArray::New();
  this->LagrangianFieldFiles = vtkStringArray::New();

  // For creating cell-to-point translated data
  this->BoundaryPointMap = nullptr;
  this->AllBoundaries = nullptr;
  this->AllBoundariesPointMap = nullptr;
  this->InternalPoints = nullptr;

  // For caching mesh
  this->InternalMesh = nullptr;
  this->BoundaryMesh = nullptr;
  this->BoundaryPointMap = nullptr;
  this->FaceOwner = nullptr;
  this->FaceNeigh = nullptr;

  this->cellZoneMap.reset(vtkFoamZones::CELL);
  this->faceZoneMap.reset(vtkFoamZones::FACE);
  this->pointZoneMap.reset(vtkFoamZones::POINT);

  this->PointZoneMesh = nullptr;
  this->FaceZoneMesh = nullptr;
  this->CellZoneMesh = nullptr;

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  // For polyhedral decomposition
  this->NumTotalAdditionalCells = 0;
  this->AdditionalCellIds = nullptr;
  this->NumAdditionalCells = nullptr;
  this->AdditionalCellPoints = nullptr;
#endif

#if VTK_FOAMFILE_FINITE_AREA
  this->areaMeshMap.reset(vtkFoamZones::FACE);
  this->AreaMesh = nullptr;
#endif

  this->Parent = nullptr;
}

vtkOpenFOAMReaderPrivate::~vtkOpenFOAMReaderPrivate()
{
  this->TimeValues->Delete();
  this->TimeNames->Delete();

  this->VolFieldFiles->Delete();
  this->DimFieldFiles->Delete();
  this->AreaFieldFiles->Delete();
  this->PointFieldFiles->Delete();
  this->LagrangianFieldFiles->Delete();

  this->ClearMeshes();
}

void vtkOpenFOAMReaderPrivate::ClearInternalMeshes()
{
  if (this->FaceOwner != nullptr)
  {
    this->FaceOwner->Delete();
    this->FaceOwner = nullptr;
  }
  if (this->FaceNeigh != nullptr)
  {
    this->FaceNeigh->Delete();
    this->FaceNeigh = nullptr;
  }
  if (this->InternalMesh != nullptr)
  {
    this->InternalMesh->Delete();
    this->InternalMesh = nullptr;
  }

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  // For polyhedral decomposition
  this->NumTotalAdditionalCells = 0;
  if (this->AdditionalCellIds != nullptr)
  {
    this->AdditionalCellIds->Delete();
    this->AdditionalCellIds = nullptr;
  }
  if (this->NumAdditionalCells != nullptr)
  {
    this->NumAdditionalCells->Delete();
    this->NumAdditionalCells = nullptr;
  }

  delete this->AdditionalCellPoints;
  this->AdditionalCellPoints = nullptr;
#endif
}

void vtkOpenFOAMReaderPrivate::ClearZoneMeshes()
{
  this->cellZoneMap.clearAll();
  this->faceZoneMap.clearAll();
  this->pointZoneMap.clearAll();

  if (this->CellZoneMesh != nullptr)
  {
    this->CellZoneMesh->Delete();
    this->CellZoneMesh = nullptr;
  }
  if (this->FaceZoneMesh != nullptr)
  {
    this->FaceZoneMesh->Delete();
    this->FaceZoneMesh = nullptr;
  }
  if (this->PointZoneMesh != nullptr)
  {
    this->PointZoneMesh->Delete();
    this->PointZoneMesh = nullptr;
  }
}

void vtkOpenFOAMReaderPrivate::ClearAreaMeshes()
{
#if VTK_FOAMFILE_FINITE_AREA
  this->areaMeshMap.clearAll();
  if (this->AreaMesh != nullptr)
  {
    this->AreaMesh->Delete();
    this->AreaMesh = nullptr;
  }
#endif
}

void vtkOpenFOAMReaderPrivate::ClearBoundaryMeshes()
{
  if (this->BoundaryMesh != nullptr)
  {
    this->BoundaryMesh->Delete();
    this->BoundaryMesh = nullptr;
  }

  delete this->BoundaryPointMap;
  this->BoundaryPointMap = nullptr;

  if (this->InternalPoints != nullptr)
  {
    this->InternalPoints->Delete();
    this->InternalPoints = nullptr;
  }
  if (this->AllBoundaries != nullptr)
  {
    this->AllBoundaries->Delete();
    this->AllBoundaries = nullptr;
  }
  if (this->AllBoundariesPointMap != nullptr)
  {
    this->AllBoundariesPointMap->Delete();
    this->AllBoundariesPointMap = nullptr;
  }
}

void vtkOpenFOAMReaderPrivate::ClearMeshes()
{
  this->ClearInternalMeshes();
  this->ClearBoundaryMeshes();
  this->ClearZoneMeshes();
  this->ClearAreaMeshes();
}

//------------------------------------------------------------------------------
// Time handling

double vtkOpenFOAMReaderPrivate::GetTimeValue() const
{
  const vtkIdType nTimes = this->TimeValues->GetNumberOfTuples();

  if (this->TimeStep < 0 || this->TimeStep >= nTimes)
  {
    return 0;
  }

  return TimeValues->GetValue(this->TimeStep);
}

void vtkOpenFOAMReaderPrivate::SetTimeValue(double requestedTime)
{
  const vtkIdType nTimes = this->TimeValues->GetNumberOfTuples();

  if (nTimes)
  {
    int nearestIndex = 0;
    double deltaT = fabs(this->TimeValues->GetValue(0) - requestedTime);

    for (vtkIdType timei = 1; timei < nTimes; ++timei)
    {
      const double diff = fabs(this->TimeValues->GetValue(timei) - requestedTime);
      if (diff < deltaT)
      {
        deltaT = diff;
        nearestIndex = timei;
      }
    }
    this->SetTimeStep(nearestIndex); // set Modified() if TimeStep changed
  }
}

void vtkOpenFOAMReaderPrivate::PrintTimes(std::ostream& os, vtkIndent indent, bool full) const
{
  const vtkIdType nTimes = this->TimeNames->GetNumberOfTuples();

  os << indent << "Times: " << nTimes << " (";
  if ((nTimes > 5) && !full)
  {
    os << this->TimeNames->GetValue(0) << ' ' << this->TimeNames->GetValue(1) << " .. "
       << this->TimeNames->GetValue(nTimes - 1);
  }
  else
  {
    for (vtkIdType timei = 0; timei < nTimes; ++timei)
    {
      if (timei)
      {
        os << ' ';
      }
      os << this->TimeNames->GetValue(timei);
    }
  }
  os << ')' << endl;
  os << indent << "Step: " << this->TimeStep << " (";

  if (this->TimeStep < 0 || this->TimeStep >= nTimes)
  {
    os << "n/a";
  }
  else
  {
    os << this->TimeNames->GetValue(this->TimeStep);
  }
  os << ')' << endl;
}

//------------------------------------------------------------------------------
// Gather the necessary information to create a path to the data
bool vtkOpenFOAMReaderPrivate::MakeInformationVector(const std::string& casePath,
  const std::string& controlDictPath, const std::string& procName, vtkOpenFOAMReader* parent,
  bool requirePolyMesh)
{
  vtkFoamDebug(<< "MakeInformationVector (" << this->RegionName << "/" << procName
               << ") polyMesh:" << requirePolyMesh << " - list times\n");

  this->CasePath = casePath;
  this->ProcessorName = procName;
  this->Parent = parent;

  bool scanTimeDirs = true;
  bool listOk = true; // Tentative return value

#if VTK_FOAMFILE_LIST_TIMEDIRS_BY_CONTROLDICT
  if (!controlDictPath.empty() && this->Parent->GetListTimeStepsByControlDict())
  {
    // Predict timesteps from controlDict values
    vtkFoamError errors = this->ListTimeDirectoriesByControlDict(controlDictPath);

    listOk = errors.empty();
    if (listOk)
    {
      scanTimeDirs = false;
    }
    else
    {
      // Fall through to list by directory
      vtkWarningMacro(<< errors << " - listing by instance instead");
    }
  }
#endif // VTK_FOAMFILE_LIST_TIMEDIRS_BY_CONTROLDICT

  if (scanTimeDirs)
  {
    // List timesteps by directory
    listOk = this->ListTimeDirectoriesByInstances();
  }
  if (!listOk)
  {
    return false;
  }

  // does not seem to be required even if number of timesteps reduced
  // upon refresh since ParaView rewinds TimeStep to 0, but for precaution
  const vtkIdType nTimes = this->TimeValues->GetNumberOfTuples();
  if (nTimes)
  {
    if (this->TimeStep >= nTimes)
    {
      this->SetTimeStep(static_cast<int>(nTimes - 1));
    }
  }
  else
  {
    this->SetTimeStep(0);
  }

  // Clear any cached knowledge
  this->PolyMeshTimeIndexPoints.clear();
  this->PolyMeshTimeIndexFaces.clear();

  // Normally expect a (default region) polyMesh/, but not for multi-region cases
  if (requirePolyMesh)
  {
    this->PopulateMeshTimeIndices();
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenFOAMReaderPrivate::MakeInformationVector(const std::string& casePath,
  const std::string& procName, vtkOpenFOAMReader* parent, vtkStringArray* timeNames,
  vtkDoubleArray* timeValues, bool requirePolyMesh)
{
  vtkFoamDebug(<< "MakeInformationVector (" << this->RegionName << "/" << procName
               << ") polyMesh:" << requirePolyMesh << " - inherit times\n");

  this->CasePath = casePath;
  this->ProcessorName = procName;
  this->Parent = parent;

  this->TimeNames->Delete();
  this->TimeNames = timeNames;
  this->TimeNames->Register(nullptr);
  this->TimeValues->Delete();
  this->TimeValues = timeValues;
  this->TimeValues->Register(nullptr);

  // does not seem to be required even if number of timesteps reduced
  // upon refresh since ParaView rewinds TimeStep to 0, but for precaution
  const vtkIdType nTimes = this->TimeValues->GetNumberOfTuples();
  if (nTimes)
  {
    if (this->TimeStep >= nTimes)
    {
      this->SetTimeStep(static_cast<int>(nTimes - 1));
    }
  }
  else
  {
    this->SetTimeStep(0);
  }

  // Clear any cached knowledge
  this->PolyMeshTimeIndexPoints.clear();
  this->PolyMeshTimeIndexFaces.clear();

  // Normally expect a (default region) polyMesh/, but not for multi-region cases
  if (requirePolyMesh)
  {
    this->PopulateMeshTimeIndices();
  }

  return true;
}

//------------------------------------------------------------------------------
// Copy time instances information and create mesh times
void vtkOpenFOAMReaderPrivate::SetupInformation(const std::string& casePath,
  const std::string& regionName, const std::string& procName, vtkOpenFOAMReaderPrivate* master,
  bool requirePolyMesh)
{
  vtkFoamDebug(<< "SetupInformation (" << this->RegionName << "/" << procName
               << ") polyMesh:" << requirePolyMesh << "\n");

  // Copy parent, path and timestep information from master
  this->CasePath = casePath;
  this->RegionName = regionName;
  this->ProcessorName = procName;
  this->Parent = master->Parent;
  this->TimeValues->Delete();
  this->TimeValues = master->TimeValues;
  this->TimeValues->Register(nullptr);
  this->TimeNames->Delete();
  this->TimeNames = master->TimeNames;
  this->TimeNames->Register(nullptr);

  // Clear any cached knowledge
  this->PolyMeshTimeIndexPoints.clear();
  this->PolyMeshTimeIndexFaces.clear();

  // Normally expect a (default region) polyMesh/, but not for multi-region cases
  if (requirePolyMesh)
  {
    this->PopulateMeshTimeIndices();
  }
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::AddFieldName(
  const std::string& fieldName, const std::string& fieldType, bool isLagrangian)
{
  if (fieldName.empty() || fieldType.empty())
  {
    return;
  }

  size_t len = fieldType.find("Field");
  bool isInternalField = false;

  if (len == std::string::npos)
  {
    return;
  }
  else if ((len + 5) == fieldType.size())
  {
    // OK: ends_with("Field")
  }
  else if (fieldType.compare(len, std::string::npos, "Field::Internal") == 0)
  {
    // OK: ends_with("Field::Internal")
    // but only valid for volScalarField::Internal, etc

    isInternalField = true;
    if (isLagrangian || (fieldType.compare(0, 3, "vol") != 0))
    {
      return;
    }
  }
  else
  {
    // Some other (unknown) type - ignore
    return;
  }

  if (isLagrangian)
  {
    // Lagrangian (point) fields: labelField, scalarField, vectorField, ...

    const auto fieldDataType(vtkFoamTypes::FieldToEnum(fieldType));
    if (fieldDataType != vtkFoamTypes::NO_TYPE)
    {
      // NB: Cloud has labelField too
      this->LagrangianFieldFiles->InsertNextValue(fieldName);
    }
    return;
  }

  size_t prefix = 0;
  vtkStringArray* target = nullptr;
  if (fieldType.compare(0, 3, "vol") == 0) // starts_with("vol")
  {
    // Volume fields: volScalarField, volVectorField, ...
    // or Dimensioned (internal) fields: volScalarField::Internal, ...
    prefix = 3;
    len -= prefix;

    target = this->VolFieldFiles;
    if (isInternalField)
    {
      target = this->DimFieldFiles;
    }
  }
  else if (fieldType.compare(0, 4, "area") == 0) // starts_with("area")
  {
    // Mesh area fields: areaScalarField, areaVectorField, ...
    prefix = 4;
    len -= prefix;

    target = this->AreaFieldFiles;
  }
  else if (fieldType.compare(0, 5, "point") == 0) // starts_with("point")
  {
    // Mesh point fields: pointScalarField, pointVectorField, ...
    prefix = 5;
    len -= prefix;

    target = this->PointFieldFiles;
  }

  if (target != nullptr)
  {
    const auto fieldDataType(vtkFoamTypes::FieldToEnum(fieldType.substr(prefix, len)));
    if (vtkFoamTypes::IsScalar(fieldDataType) || vtkFoamTypes::IsVectorSpace(fieldDataType))
    {
      target->InsertNextValue(fieldName);
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::GetFieldNames(const std::string& tempPath, bool isLagrangian)
{
  // Open the directory and get num of files
  vtkNew<vtkDirectory> directory;
  if (!directory->Open(tempPath.c_str()))
  {
    // No data
    return;
  }

  // loop over all files and locate valid fields
  const vtkIdType nFieldFiles = directory->GetNumberOfFiles();
  for (vtkIdType fileI = 0; fileI < nFieldFiles; ++fileI)
  {
    const std::string fieldFile(directory->GetFile(fileI));
    const auto len = fieldFile.length();

    if (!len || (fieldFile[len - 1] == '~') || directory->FileIsDirectory(fieldFile.c_str()))
    {
      continue;
    }
#if VTK_FOAMFILE_IGNORE_FIELD_RESTART
    else if (len > 2 && (fieldFile[len - 2] == '_') && (fieldFile[len - 1] == '0'))
    {
      // Exclude "*_0" restart files
      continue;
    }
#endif
    else
    {
      // Exclude various backup extensions - cf. Foam::fileName::isBackup()

      auto sep = fieldFile.rfind('.');
      if (sep != std::string::npos)
      {
        ++sep;

        if (!fieldFile.compare(sep, std::string::npos, "bak") ||
          !fieldFile.compare(sep, std::string::npos, "BAK") ||
          !fieldFile.compare(sep, std::string::npos, "old") ||
          !fieldFile.compare(sep, std::string::npos, "save"))
        {
          continue;
        }
      }
    }

    // Note: for isLagrangian, could reject "positions" and "coordinates" instead of opening files

    vtkFoamIOobject io(this->CasePath, this->Parent);
    if (io.Open(tempPath + "/" + fieldFile)) // file exists and readable
    {
      this->AddFieldName(fieldFile, io.GetClassName(), isLagrangian);
    }
  }
  // delay Squeeze of inserted objects until SortFieldFiles()
}

//------------------------------------------------------------------------------
// Locate lagrangian clouds
void vtkOpenFOAMReaderPrivate::LocateLagrangianClouds(const std::string& timePath)
{
  const std::string lagrangianDir(timePath + this->RegionPath() + "/lagrangian");

  vtkNew<vtkDirectory> directory;
  if (directory->Open(lagrangianDir.c_str()))
  {
    // Search for clouds (OF 1.5 and later format)
    const vtkIdType nFiles = directory->GetNumberOfFiles();

    for (vtkIdType filei = 0; filei < nFiles; ++filei)
    {
      const std::string cloudName(directory->GetFile(filei));
      if (cloudName == "." || cloudName == ".." || !directory->FileIsDirectory(cloudName.c_str()))
      {
        continue;
      }

      const std::string cloudPath(lagrangianDir + "/" + cloudName);
      const std::string displayName(this->RegionPrefix() + "lagrangian/" + cloudName);

      // lagrangian positions. there are many concrete class names
      // e. g. Cloud<parcel>, basicKinematicCloud etc.

      vtkFoamIOobject io(this->CasePath, this->Parent);
      if (io.OpenOrGzip(cloudPath + "/positions") && io.GetObjectName() == "positions" &&
        io.GetClassName().find("Cloud") != std::string::npos)
      {
        // Append unique
        if (this->LagrangianPaths->LookupValue(displayName) == -1)
        {
          this->LagrangianPaths->InsertNextValue(displayName);
        }
        this->GetFieldNames(cloudPath, true);
        this->Parent->PatchDataArraySelection->AddArray(displayName.c_str());
      }
    }
    this->LagrangianPaths->Squeeze();
  }
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReaderPrivate::SortFieldFiles(vtkStringArray* selections, vtkStringArray* files)
{
  // The object (field) name in the FoamFile header should always correspond
  // to the filename (without any trailing .gz etc)

  const vtkIdType nFiles = files->GetNumberOfValues();

  vtkNew<vtkStringArray> names;
  names->SetNumberOfValues(nFiles);

  for (vtkIdType i = 0; i < nFiles; ++i)
  {
    std::string name(files->GetValue(i));
    const auto ending = name.rfind(".gz");
    if (ending != std::string::npos)
    {
      name.erase(ending);
    }
    names->SetValue(i, name);
  }

  names->Squeeze();
  files->Squeeze();
  vtkSortDataArray::Sort(names, files);
  for (vtkIdType i = 0; i < nFiles; ++i)
  {
    selections->InsertNextValue(names->GetValue(i));
  }
}

//------------------------------------------------------------------------------
// Set contents from dictionary information read from the polyMesh/boundary file
bool vtkFoamBoundaries::update(const vtkFoamDict& dict)
{
  auto& patches = *this;
  patches.clearAll();
  patches.resize(dict.size());
  auto& inGroups = patches.groups;

  const auto nBoundaries = static_cast<vtkIdType>(patches.size());

  vtkIdType endFace = -1; // for sanity check
  vtkTypeInt64 nBoundaryFaces = 0;

  for (vtkIdType patchi = 0; patchi < nBoundaries; ++patchi)
  {
    // The name/dictionary, from "polyMesh/boundary" entry
    const vtkFoamEntry* eptr = dict[patchi];
    const std::string& patchName = eptr->GetKeyword();
    const vtkFoamDict& patchDict = eptr->Dictionary();

    // The patch entry to populate
    vtkFoamPatch& patch = patches[patchi];
    patch.index_ = patchi;
    patch.offset_ = nBoundaryFaces;
    patch.type_ = vtkFoamPatch::GEOMETRICAL;
    patch.owner_ = true; // Patch owner (processor patch)

    patch.name_ = patchName;
    if ((eptr = patchDict.Lookup("type")) == nullptr)
    {
      this->error() // Report errors
        << "No 'type' entry found for patch: " << patch.name_;
      return false;
    }
    const std::string patchTypeName(eptr->ToString());

    if ((eptr = patchDict.Lookup("startFace")) == nullptr)
    {
      this->error() // Report errors
        << "No 'startFace' entry found for patch: " << patch.name_;
      return false;
    }
    patch.start_ = eptr->ToInt();

    if ((eptr = patchDict.Lookup("nFaces")) == nullptr)
    {
      this->error() // Report errors
        << "No 'nFaces' entry found for patch: " << patch.name_;
      return false;
    }
    patch.size_ = eptr->ToInt();

    // Size, consistency sanity checks
    if (patch.start_ < 0 || patch.size_ < 0)
    {
      this->error() // Report errors
        << "The startFace:" << patch.start_ << " or nFaces:" << patch.size_
        << " are negative for patch " << patch.name_;
      return false;
    }
    if (endFace >= 0 && endFace != patch.start_)
    {
      this->error() // Report errors
        << "The end face number " << (endFace - 1) << " of patch " << patches[patchi - 1].name_
        << " is inconsistent with start face number " << patch.start_ << " of patch "
        << patch.name_;
      return false;
    }
    endFace = patch.endFace(); // <- The startFace for the next patch index

    // If the basic type of the patch is one of the following the
    // point-filtered values at patches are overridden by patch values
    if (patchTypeName == "patch" || patchTypeName == "wall" || patchTypeName == "mappedWall")
    {
      patch.type_ = vtkFoamPatch::PHYSICAL;
      nBoundaryFaces += patch.size_;
    }
    else if (patchTypeName == "processor" || patchTypeName == "processorCyclic")
    {
      patch.type_ = vtkFoamPatch::PROCESSOR;
      nBoundaryFaces += patch.size_;

      // Note owner/neighbour relationship for processor patch
      const auto* ownptr = patchDict.Lookup("myProcNo");
      const auto* neiptr = patchDict.Lookup("neighbProcNo");

      if (ownptr != nullptr && neiptr != nullptr && // Safety
        ownptr->FirstValue().IsLabel() && neiptr->FirstValue().IsLabel())
      {
        const vtkTypeInt64 own = ownptr->FirstValue().ToInt();
        const vtkTypeInt64 nei = neiptr->FirstValue().ToInt();
        patch.owner_ = (own < nei);
      }
    }

    // Handle inGroups which could have this type of content:
    //   - inGroups (name1 .. nameN);
    //   - inGroups 2(name1 name2);
    // but never for processor boundaries (potential clutter or false positives)
    if ((eptr = patchDict.Lookup("inGroups")) != nullptr && patch.type_ != vtkFoamPatch::PROCESSOR)
    {
      for (const vtkFoamEntryValue* subentry : *eptr)
      {
        if (subentry && subentry->GetType() == vtkFoamToken::STRINGLIST)
        {
          // Yes this is really needed, VTK constness is a bit odd
          vtkStringArray& groupNames = const_cast<vtkStringArray&>(subentry->StringList());
          const vtkIdType nGroups = groupNames.GetNumberOfValues();

          for (vtkIdType groupi = 0; groupi < nGroups; ++groupi)
          {
            const std::string& groupName = groupNames.GetValue(groupi);
            inGroups[groupName].push_back(patchi);
          }
        }
      }
    }
  }

  // Could also use HasError() for an additional sanity check
  return true;
}

//------------------------------------------------------------------------------
// Binary search for patch index for a given face label
// Return -1 for internal face or out-of-bounds
vtkIdType vtkFoamBoundaries::whichPatch(vtkIdType faceIndex) const
{
  if (this->empty() ||                         // Safety/short-circuit
    (faceIndex < this->front().startFace()) || // Internal mesh face
    (faceIndex >= (this->back().endFace())))   // Out-of-bounds
  {
    return -1;
  }

  // Binary search like std::lower_bound, but slightly modified
  auto first = this->begin();
  const auto last = this->end();
  auto count = this->size();

  while (count > 0)
  {
    auto iter = first;
    auto step = count / 2;
    iter += step;

    if (iter->start_ <= faceIndex) // NB: must include start in the comparison
    {
      first = ++iter;
      count -= step + 1;
    }
    else
    {
      count = step;
    }
  }
  return (first != last) ? first->index_ : -1;
}

//------------------------------------------------------------------------------
// Create field data lists and cell/point array selection lists
int vtkOpenFOAMReaderPrivate::MakeMetaDataAtTimeStep(vtkStringArray* cellSelectionNames,
  vtkStringArray* pointSelectionNames, vtkStringArray* lagrangianSelectionNames,
  bool listNextTimeStep)
{
  vtkFoamDebug(<< "MakeMetaDataAtTimeStep (" << this->RegionName << "/" << this->ProcessorName
               << ")\n");

  if (!this->HasPolyMesh())
  {
    // Ignore a region without a mesh, but will normally be precluded earlier
    vtkWarningMacro("Called MakeMetaDataAtTimeStep without a mesh.");
    return 1;
  }

  // Track topology change
  const bool topoChanged = (this->TopologyTimeIndex < -1 ||
    (this->TopologyTimeIndex != this->PolyMeshTimeIndexFaces[this->TimeStep]));
  this->TopologyTimeIndex = this->PolyMeshTimeIndexFaces[this->TimeStep];

  // Change in topology or selection, may need to update boundaries
  {
    auto& patches = this->BoundaryDict;

    // User selection changed
    const bool selectChanged = topoChanged ||
      (this->Parent->PatchDataArraySelection->GetMTime() != this->Parent->PatchSelectionMTimeOld);

    bool addInternalSelection = false;

    // Read contents of polyMesh/boundary to update patch definitions
    if (topoChanged)
    {
      patches.clearAll();
      const bool isSubRegion = !this->RegionName.empty();
      auto boundaryEntriesPtr(this->GetPolyMeshFile("boundary", isSubRegion));

      if (boundaryEntriesPtr)
      {
        if (!patches.update(*boundaryEntriesPtr))
        {
          vtkErrorMacro(<< patches.error());
          return 0;
        }

        // On topology change, add the internal mesh by default
        addInternalSelection = true;
      }
      else if (isSubRegion)
      {
        // Could be missing polyMesh/boundary for sub-region
        return 0;
      }
    }

    // The internal mesh - set/check status
    if (selectChanged)
    {
      const std::string displayName(this->RegionPrefix() + NAME_INTERNALMESH);

      if (addInternalSelection)
      {
        this->Parent->PatchDataArraySelection->AddArray(displayName.c_str());
      }
      this->InternalMeshSelectionStatus =
        (this->Parent->PatchDataArraySelection->ArrayExists(displayName.c_str()) &&
          this->Parent->GetPatchArrayStatus(displayName.c_str()));
    }

    // The boundary mesh - change in user selection or topology
    if (selectChanged)
    {
      // Can perhaps do more with preserving old selections
      // and check which boundaries actually changed
      //
      // decltype(patches.groupActive) groupActiveOld;
      // decltype(patches.patchActive) patchActiveOld;
      // decltype(patches.patchActiveByGroup) patchActiveByGroupOld;
      //
      // std::swap(groupActiveOld, patches.groupActive);
      // std::swap(patchActiveOld, patches.patchActive);
      // std::swap(patchActiveByGroupOld, patches.patchActiveByGroup);

      // For now, simply start afresh
      patches.clearSelections();

      // Patch groups (sorted)
      const auto& inGroups = patches.groups;
      for (auto citer = inGroups.begin(), endIter = inGroups.end(); citer != endIter; ++citer)
      {
        const std::string& groupName = citer->first;
        const std::string displayName(this->RegionPrefix() + "group/" + groupName);
        if (this->Parent->PatchDataArraySelection->ArrayExists(displayName.c_str()))
        {
          if (this->Parent->GetPatchArrayStatus(displayName.c_str()))
          {
            // Selected by group
            patches.enableGroup(groupName);
          }
        }
        else
        {
          // Add to list with selection status == off.
          this->Parent->PatchDataArraySelection->DisableArray(displayName.c_str());
        }
      }

      // Individual patches
      for (vtkFoamPatch& patch : patches)
      {
        const std::string& patchName = patch.name_;

        // always hide processor patches for decomposed cases to keep
        // vtkAppendCompositeDataLeaves happy
        if (patch.type_ == vtkFoamPatch::PROCESSOR && !this->ProcessorName.empty())
        {
          continue;
        }

        const std::string displayName(this->RegionPrefix() + "patch/" + patchName);
        if (this->Parent->PatchDataArraySelection->ArrayExists(displayName.c_str()))
        {
          if (this->Parent->GetPatchArrayStatus(displayName.c_str()))
          {
            // Selected by patch
            patches.enablePatch(patch.index_);
          }
        }
        else
        {
          // Add to list with selection status == off.
          // The patch is added to list even if its size is zero
          this->Parent->PatchDataArraySelection->DisableArray(displayName.c_str());
        }
      }
    }
  }

  // Add scalars and vectors to metadata
  std::string timePath(this->CurrentTimePath());

  // do not do "RemoveAllArrays()" to accumulate array selections
  // this->CellDataArraySelection->RemoveAllArrays();
  this->VolFieldFiles->Initialize();
  this->DimFieldFiles->Initialize();
  this->AreaFieldFiles->Initialize();
  this->PointFieldFiles->Initialize();
  this->GetFieldNames(timePath + this->RegionPath());

  this->LagrangianFieldFiles->Initialize();
  if (listNextTimeStep)
  {
    this->LagrangianPaths->Initialize();
  }
  this->LocateLagrangianClouds(timePath);

  // if the requested timestep is 0 then we also look at the next
  // timestep to add extra objects that don't exist at timestep 0 into
  // selection lists. Note the ObjectNames array will be recreated in
  // RequestData() so we don't have to worry about duplicated fields.

  if (listNextTimeStep && this->TimeStep == 0)
  {
    int nextTimeStep = this->TimeStep + 1;
    if (nextTimeStep < this->TimeValues->GetNumberOfTuples())
    {
      timePath = this->TimePath(nextTimeStep);
      this->GetFieldNames(timePath + this->RegionPath());

      // Lagrangian clouds are likely missing at time 0
      // - could also lookahead multiple time steps (if desired)
      if (!this->LagrangianPaths->GetNumberOfTuples())
      {
        this->LocateLagrangianClouds(timePath);
      }
    }
  }

  // sort array names. volFields first, followed by internal fields
  this->SortFieldFiles(cellSelectionNames, this->VolFieldFiles);
  this->SortFieldFiles(cellSelectionNames, this->DimFieldFiles);
#if VTK_FOAMFILE_FINITE_AREA
  this->SortFieldFiles(cellSelectionNames, this->AreaFieldFiles);
#endif
  this->SortFieldFiles(pointSelectionNames, this->PointFieldFiles);
  this->SortFieldFiles(lagrangianSelectionNames, this->LagrangianFieldFiles);

  return 1;
}

//------------------------------------------------------------------------------
// List time directories according to system/controlDict

#if VTK_FOAMFILE_LIST_TIMEDIRS_BY_CONTROLDICT
vtkFoamError vtkOpenFOAMReaderPrivate::ListTimeDirectoriesByControlDict(
  const std::string& controlDictPath)
{
  // Note: use double (not float) to handle time values precisely

  // Open and check if controlDict is readable
  vtkFoamIOobject io(this->CasePath, this->Parent);

  if (!io.Open(controlDictPath))
  {
    return vtkFoamError() << "Error opening " << io.GetFileName() << ": " << io.GetError();
  }

  vtkFoamDict dict;
  if (!dict.Read(io))
  {
    return vtkFoamError() << "Error reading line " << io.GetLineNumber() << " of "
                          << io.GetFileName() << ": " << io.GetError();
  }

  if (dict.GetType() != vtkFoamToken::DICTIONARY)
  {
    return vtkFoamError() << "The file " << io.GetFileName() << " is not a dictionary";
  }

  const vtkFoamEntry* eptr;

  // Calculate time step increment based on type of run
  if ((eptr = dict.Lookup("writeControl")) == nullptr)
  {
    return vtkFoamError() << "No 'writeControl' in " << io.GetFileName();
  }
  const std::string writeControl(eptr->ToString());

  // When (adjustTimeStep, writeControl) == (on, adjustableRunTime) or (off, timeStep)
  // list by time instances in the case directory otherwise
  // (different behaviour from paraFoam)

  bool adjustTimeStep = false;
  if ((eptr = dict.Lookup("adjustTimeStep")) != nullptr)
  {
    const std::string sw(eptr->ToString());

    // Switch values for 'true' (cf. src/OpenFOAM/db/Switch/Switch.C)
    adjustTimeStep = (sw == "on" || sw == "yes" || sw == "y" || sw == "true" || sw == "t");
  }
  if (adjustTimeStep)
  {
    if (writeControl.compare(0, 10, "adjustable") != 0)
    {
      // Require "adjustable" or "adjustableRunTime"
      return vtkFoamError() << "Used adjustTimeStep, but writeControl was not adjustable: "
                            << writeControl;
    }
  }
  else if (writeControl != "timeStep")
  {
    return vtkFoamError() << "Use fixed time step, but writeControl was not 'timeStep': "
                          << writeControl;
  }

  if ((eptr = dict.Lookup("startTime")) == nullptr)
  {
    return vtkFoamError() << "No 'startTime' in controlDict";
  }
  const double startTime = eptr->ToDouble();

  if ((eptr = dict.Lookup("endTime")) == nullptr)
  {
    return vtkFoamError() << "No 'endTime' in controlDict";
  }
  const double endTime = eptr->ToDouble();

  if ((eptr = dict.Lookup("deltaT")) == nullptr)
  {
    return vtkFoamError() << "No 'deltaT' in controlDict";
  }
  const double deltaT = eptr->ToDouble();

  if ((eptr = dict.Lookup("writeInterval")) == nullptr)
  {
    return vtkFoamError() << "No 'writeInterval' in controlDict";
  }
  const double writeInterval = eptr->ToDouble();

  // "timeFormat" is optional - default is "general"
  std::string timeFormat("general");
  if ((eptr = dict.Lookup("timeFormat")) != nullptr)
  {
    timeFormat = eptr->ToString();
  }

  // Default timePrecision is 6
  const vtkTypeInt64 timePrecision =
    ((eptr = dict.Lookup("timePrecision")) != nullptr ? eptr->ToInt() : 6);

  double timeStepIncrement = 1;
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
    return vtkFoamError() << "Cannot determine time-step for writeControl: " << writeControl;
  }

  // How many timesteps there should be, rounded up
  const int numTimeSteps = 1 + static_cast<int>((endTime - startTime) / timeStepIncrement + 0.5);

  // Determine time name based on Foam::Time::timeName()
  // cf. src/OpenFOAM/db/Time/Time.C
  std::ostringstream parser;
#ifdef _MSC_VER
  bool correctExponent = true;
#endif
  if (timeFormat == "general" || timeFormat.empty())
  {
    parser.setf(std::ios_base::fmtflags(0), std::ios_base::floatfield);
  }
  else if (timeFormat == "fixed")
  {
    parser.setf(std::ios_base::fmtflags(std::ios_base::fixed), std::ios_base::floatfield);
#ifdef _MSC_VER
    correctExponent = false;
#endif
  }
  else if (timeFormat == "scientific")
  {
    parser.setf(std::ios_base::fmtflags(std::ios_base::scientific), std::ios_base::floatfield);
  }
  else
  {
    parser.setf(std::ios_base::fmtflags(0), std::ios_base::floatfield);
  }
  parser.precision(timePrecision);

  this->TimeValues->Initialize();
  this->TimeNames->Initialize();

  for (int timeStepi = 0; timeStepi < numTimeSteps; ++timeStepi)
  {
    parser.str("");
    const double timeValue = startTime + timeStepIncrement * timeStepi;
    parser << timeValue;

    std::string timeName(parser.str());

#ifdef _MSC_VER
    // workaround for format difference in MSVC++:
    // remove an extra 0 from exponent
    if (correctExponent)
    {
      const auto pos = timeName.find('e');
      if (pos != std::string::npos && timeName.length() >= pos + 3 && timeName[pos + 2] == '0')
      {
        timeName.erase(pos + 2, 1);
      }
    }
#endif

    // Add the time steps that actually exist to steps
    // allows the run to be stopped short of controlDict spec
    // allows for removal of timesteps
    if (vtksys::SystemTools::FileIsDirectory(this->CasePath + timeName))
    {
      this->TimeNames->InsertNextValue(timeName);
      this->TimeValues->InsertNextValue(timeValue);
    }
    // necessary for reading the case/0 directory whatever the timeFormat is
    // based on Foam::Time::operator++() cf. src/OpenFOAM/db/Time/Time.C
    else if ((fabs(timeValue) < 1.0e-14L) // 10*SMALL
      && vtksys::SystemTools::FileIsDirectory(this->CasePath + "0"))
    {
      this->TimeNames->InsertNextValue("0");
      this->TimeValues->InsertNextValue(0);
    }
  }

  // If there are no other times and "constant/" directory exists - treat as startTime
  if (this->TimeValues->GetNumberOfTuples() == 0 &&
    vtksys::SystemTools::FileIsDirectory(this->CasePath + "constant"))
  {
    parser.str("");
    parser << startTime;
    this->TimeNames->InsertNextValue(parser.str());
    this->TimeValues->InsertNextValue(startTime);
  }

  this->TimeValues->Squeeze();
  this->TimeNames->Squeeze();
  return vtkFoamError();
}
#endif // VTK_FOAMFILE_LIST_TIMEDIRS_BY_CONTROLDICT

//------------------------------------------------------------------------------
// List time directories by searching all valid time instances in a
// case directory
bool vtkOpenFOAMReaderPrivate::ListTimeDirectoriesByInstances()
{
  // Open the case directory
  vtkNew<vtkDirectory> dir;
  if (!dir->Open(this->CasePath.c_str()))
  {
    vtkErrorMacro(<< "Can't open directory " << this->CasePath);
    return false;
  }

  const bool ignore0Dir = this->Parent->GetSkipZeroTime();

  // Detect all directories in the case directory with names convertible to numbers
  this->TimeNames->Initialize();
  this->TimeValues->Initialize();

  const vtkIdType nFiles = dir->GetNumberOfFiles();
  for (vtkIdType filei = 0; filei < nFiles; ++filei)
  {
    const char* timeName = dir->GetFile(filei);

    // Perform string checks first (quick) before any filestat
    // - expect numbers starting with a digit or [-+]
    // - no numbers starting with '.', since they would be hidden files!

    // Optionally ignore "0/" directory
    if (ignore0Dir && timeName[0] == '0' && timeName[1] == '\0')
    {
      continue;
    }

    bool isNumber = (std::isdigit(timeName[0]) || timeName[0] == '+' || timeName[0] == '-');
    for (const char* p = (timeName + 1); *p && isNumber; ++p)
    {
      const char c = *p;
      isNumber = (std::isdigit(c) || c == '+' || c == '-' || c == '.' || c == 'E' || c == 'e');
    }

    if (isNumber)
    {
      // Convert to a number
      char* endptr = nullptr;
      const double timeValue = std::strtod(timeName, &endptr);

      // Check for good parse of entire string, and filestat that it is a directory
      if (timeName != endptr && *endptr == '\0' && dir->FileIsDirectory(timeName))
      {
        this->TimeNames->InsertNextValue(timeName);
        this->TimeValues->InsertNextValue(timeValue);
      }
    }
  }

  // If there are no other times, use "constant/" directory
  if (this->TimeValues->GetNumberOfTuples() == 0 && dir->FileIsDirectory("constant"))
  {
    this->TimeNames->InsertNextValue("constant");
    this->TimeValues->InsertNextValue(0);
  }

  this->TimeNames->Squeeze();
  this->TimeValues->Squeeze();

  vtkIdType nTimes = this->TimeValues->GetNumberOfTuples();

  if (nTimes > 1)
  {
    // Sort the detected time directories
    vtkSortDataArray::Sort(this->TimeValues, this->TimeNames);

    // Remove duplicate time values (e.g. "0" and "0.000")
    for (vtkIdType timeI = 1; timeI < nTimes; ++timeI)
    {
      // compare by exact match
      if (this->TimeValues->GetValue(timeI - 1) == this->TimeValues->GetValue(timeI))
      {
        vtkWarningMacro(<< "Different time directories with the same time value "
                        << this->TimeNames->GetValue(timeI - 1) << " and "
                        << this->TimeNames->GetValue(timeI) << " found. "
                        << this->TimeNames->GetValue(timeI) << " will be ignored.");
        this->TimeValues->RemoveTuple(timeI);
        // vtkStringArray does not have RemoveTuple()
        for (vtkIdType oldTimei = timeI + 1; oldTimei < nTimes; ++oldTimei)
        {
          this->TimeNames->SetValue(oldTimei - 1, this->TimeNames->GetValue(oldTimei));
        }
        --nTimes;
        this->TimeNames->Resize(nTimes);
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
// Print changes in mesh times (debugging only)
#if VTK_FOAMFILE_DEBUG
void vtkOpenFOAMReaderPrivate::PrintMeshTimes(
  const char* name, const std::vector<vtkIdType>& indexer) const
{
  // Yes this is really needed, VTK constness is a bit odd
  auto& timeNames = const_cast<vtkStringArray&>(*this->TimeNames);

  std::cerr << name << " times (";

  vtkIdType prev = TIMEINDEX_UNVISITED;
  for (const auto idx : indexer)
  {
    if (idx == TIMEINDEX_UNVISITED)
    {
      std::cerr << " .";
    }
    else if (prev != idx)
    {
      std::cerr << ' ';
      if (idx < 0)
      {
        std::cerr << "constant";
      }
      else
      {
        std::cerr << timeNames.GetValue(idx);
      }
    }
    prev = idx;
  }
  std::cerr << " )\n";
}
#endif

//------------------------------------------------------------------------------
// Local Function

namespace
{

// Update mesh instance for change
// - Changed: set to current time index
// - No change: set to previous time instance
// - No change and first instance: it is "constant" time instance
inline void UpdateTimeInstance(std::vector<vtkIdType>& list, vtkIdType i, bool changed)
{
  list[i] = changed ? i : (i == 0) ? TIMEINDEX_CONSTANT : list[i - 1];
}

} // End anonymous namespace

//------------------------------------------------------------------------------
// create a Lookup Table containing the location of the points
// and faces files for each time steps mesh
void vtkOpenFOAMReaderPrivate::PopulateMeshTimeIndices()
{
  auto& faces = this->PolyMeshTimeIndexFaces;
  auto& points = this->PolyMeshTimeIndexPoints;

  // Ensure consistent sizing
  const vtkIdType nTimes = this->TimeValues->GetNumberOfTuples();

  faces.resize(nTimes, TIMEINDEX_UNVISITED);
  points.resize(nTimes, TIMEINDEX_UNVISITED);

  for (vtkIdType timeIter = 0; timeIter < nTimes; ++timeIter)
  {
    // The mesh directory for this timestep
    const std::string meshDir(this->TimeRegionPath(timeIter) + "/polyMesh/");

    const bool hasMeshDir = vtksys::SystemTools::FileIsDirectory(meshDir);
    const bool topoChanged = hasMeshDir && vtkFoamFile::IsFile(meshDir + "faces", true);
    const bool pointsMoved = hasMeshDir && vtkFoamFile::IsFile(meshDir + "points", true);

    UpdateTimeInstance(faces, timeIter, topoChanged);
    UpdateTimeInstance(points, timeIter, pointsMoved);
  }

#if VTK_FOAMFILE_DEBUG
  PrintMeshTimes("faces", faces);
  PrintMeshTimes("points", points);
#endif
}

//------------------------------------------------------------------------------
// Read the points file into a vtkFloatArray
//
// - sets NumPoints

vtkSmartPointer<vtkFloatArray> vtkOpenFOAMReaderPrivate::ReadPointsFile(
  const std::string& timeRegionDir)
{
  // Assume failure
  this->NumPoints = 0;

  vtkFoamIOobject io(this->CasePath, this->Parent);

  // Read polyMesh/points
  if (!io.OpenOrGzip(timeRegionDir + "/polyMesh/points"))
  {
    vtkErrorMacro(<< "Error opening " << io.GetFileName() << ": " << io.GetError());
    return nullptr;
  }

  vtkSmartPointer<vtkFloatArray> pointArray;

  try
  {
    vtkFoamEntryValue dict(nullptr);

    if (io.IsFloat64())
    {
      dict.ReadNonUniformList<vtkFoamToken::VECTORLIST, //
        vtkFoamRead::vectorListTraits<vtkFloatArray, double, 3>>(io);
    }
    else
    {
      dict.ReadNonUniformList<vtkFoamToken::VECTORLIST, //
        vtkFoamRead::vectorListTraits<vtkFloatArray, float, 3>>(io);
    }

    // Capture content as smart pointer
    pointArray.TakeReference(dict.ReleasePtr<vtkFloatArray>());
  }
  catch (const vtkFoamError& err)
  {
    vtkErrorMacro("Mesh points data are neither 32 nor 64 bit, or some other "
                  "parse error occurred while reading points. Failed at line "
      << io.GetLineNumber() << " of " << io.GetFileName() << ": " << err);
    return nullptr;
  }

  assert(pointArray.Get() != nullptr);

  // The number of points
  this->NumPoints = pointArray->GetNumberOfTuples();

  return pointArray;
}

//------------------------------------------------------------------------------
// Read the faces into a vtkFoamLabelListList
//
// - sets NumFaces, clears NumInternalFaces
//
// Return meshFaces

std::unique_ptr<vtkFoamLabelListList> vtkOpenFOAMReaderPrivate::ReadFacesFile(
  const std::string& timeRegionDir)
{
  // Assume failure
  this->NumFaces = 0;
  this->NumInternalFaces = 0;

  vtkFoamIOobject io(this->CasePath, this->Parent);

  // Read polyMesh/faces
  if (!io.OpenOrGzip(timeRegionDir + "/polyMesh/faces"))
  {
    vtkErrorMacro(<< "Error opening " << io.GetFileName() << ": " << io.GetError()
                  << ". If you are trying to read a parallel decomposed case, "
                     "set Case Type to Decomposed Case.");
    return nullptr;
  }

  std::unique_ptr<vtkFoamLabelListList> meshFaces;

  try
  {
    vtkFoamEntryValue dict(nullptr);
    dict.SetStreamOption(io);

    if (io.GetClassName() == "faceCompactList")
    {
      dict.ReadCompactLabelListList(io);
    }
    else
    {
      dict.ReadLabelListList(io);
    }

    // Capture content
    meshFaces.reset(dict.ReleasePtr<vtkFoamLabelListList>());
  }
  catch (const vtkFoamError& err)
  {
    vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                  << ": " << err);
    return nullptr;
  }

  if (meshFaces)
  {
    this->NumFaces = meshFaces->GetNumberOfElements();
  }

  return meshFaces;
}

//------------------------------------------------------------------------------
// Read owner, neighbour files
// - sets NumFaces and NumInternalFaces, and NumCells

bool vtkOpenFOAMReaderPrivate::ReadOwnerNeighbourFiles(const std::string& timeRegionDir)
{
  // Assume failure
  this->NumCells = 0;

  vtkFoamIOobject io(this->CasePath, this->Parent);

  // Read polyMesh/owner
  if (!io.OpenOrGzip(timeRegionDir + "/polyMesh/owner"))
  {
    vtkErrorMacro(<< "Error opening " << io.GetFileName() << ": " << io.GetError());
    return false;
  }
  const bool use64BitLabels = io.IsLabel64();

  // Count cells by tracking the max cell id seen
  vtkTypeInt64 nCells = -1;

  {
    vtkFoamEntryValue ownerDict(nullptr);
    ownerDict.SetStreamOption(io);
    try
    {
      if (use64BitLabels)
      {
        ownerDict.ReadNonUniformList<vtkFoamToken::LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt64Array, vtkTypeInt64>>(io);
      }
      else
      {
        ownerDict.ReadNonUniformList<vtkFoamToken::LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt32Array, vtkTypeInt32>>(io);
      }
    }
    catch (const vtkFoamError& err)
    {
      vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                    << ": " << err);
      return false;
    }
    io.Close();

    // Store owner faces
    this->FaceOwner = ownerDict.ReleasePtr<vtkDataArray>();
    const vtkIdType nFaces = this->FaceOwner->GetNumberOfTuples();

    // Check for max cell, check validity
    for (vtkIdType facei = 0; facei < nFaces; ++facei)
    {
      const vtkTypeInt64 celli = GetLabelValue(this->FaceOwner, facei, use64BitLabels);
      if (celli < 0)
      {
        vtkErrorMacro(<< "Illegal cell label in owner addressing. Face " << facei);
        return false;
      }
      nCells = std::max(nCells, celli);
    }
  }

  // Read polyMesh/neighbour
  if (!io.OpenOrGzip(timeRegionDir + "/polyMesh/neighbour"))
  {
    vtkErrorMacro(<< "Error opening " << io.GetFileName() << ": " << io.GetError());
    return false;
  }

  if (use64BitLabels != io.IsLabel64())
  {
    vtkErrorMacro(<< "owner/neighbour with different label-size: should not happen"
                  << io.GetCasePath());
    return false;
  }

  {
    vtkFoamEntryValue neighDict(nullptr);
    neighDict.SetStreamOption(io);
    try
    {
      if (use64BitLabels)
      {
        neighDict.ReadNonUniformList<vtkFoamToken::LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt64Array, vtkTypeInt64>>(io);
      }
      else
      {
        neighDict.ReadNonUniformList<vtkFoamToken::LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt32Array, vtkTypeInt32>>(io);
      }
    }
    catch (const vtkFoamError& err)
    {
      vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                    << ": " << err);
      return false;
    }
    io.Close();

    // Store neighbour faces
    this->FaceNeigh = neighDict.ReleasePtr<vtkDataArray>();
    const vtkIdType nFaces = this->FaceOwner->GetNumberOfTuples();

    if (nFaces == this->FaceNeigh->GetNumberOfTuples())
    {
      // Extremely old meshes had identical size for owner/neighbour and -1 padding
      vtkIdType nInternalFaces = 0;
      for (vtkIdType facei = 0; facei < nFaces; ++facei)
      {
        if (GetLabelValue(this->FaceNeigh, facei, use64BitLabels) < 0)
        {
          break;
        }
        else
        {
          ++nInternalFaces;
        }
      }
      this->FaceNeigh->Resize(nInternalFaces);
    }

    const vtkIdType nInternalFaces = this->FaceNeigh->GetNumberOfTuples();

    // Check for max cell, check validity
    for (vtkIdType facei = 0; facei < nInternalFaces; ++facei)
    {
      const vtkTypeInt64 celli = GetLabelValue(this->FaceNeigh, facei, use64BitLabels);
      if (celli < 0)
      {
        vtkErrorMacro(<< "Illegal cell label in neighbour addressing. Face " << facei);
        return false;
      }
      nCells = std::max(nCells, celli);
    }
  }

  this->NumCells = static_cast<vtkIdType>(++nCells);

  // Size checks
  if (this->NumCells == 0)
  {
    vtkWarningMacro(<< "The mesh contains no cells");
  }

  const vtkIdType nFaces = this->FaceOwner->GetNumberOfTuples();
  const vtkIdType nInternalFaces = this->FaceNeigh->GetNumberOfTuples();

  if (nFaces < nInternalFaces)
  {
    vtkErrorMacro(<< "Number of owner faces " << nFaces
                  << " not equal or greater than number of neighbour faces " << nInternalFaces);
    return false;
  }

  // Set or check number of mesh faces.
  // NB: with foam-extend it is possible that "owner" is shorter than "faces"
  // with additional faces being in a globalFaceZone.
  // so update NumFaces to avoid inconsistencies
  if ((this->NumFaces == 0) || (nFaces < this->NumFaces))
  {
    this->NumFaces = nFaces;
  }
  else if (this->NumFaces != nFaces)
  {
    vtkErrorMacro(<< "Expected " << this->NumFaces << " faces, but owner had " << nFaces
                  << " faces");
    return false;
  }
  this->NumInternalFaces = nInternalFaces;

  return true;
}

//------------------------------------------------------------------------------
// Create meshCells from owner/neighbour information
//
// - sets NumFaces and NumInternalFaces (again), optionally NumCells

std::unique_ptr<vtkFoamLabelListList> vtkOpenFOAMReaderPrivate::CreateCellFaces()
{
  if (!this->FaceOwner)
  {
    vtkErrorMacro(<< "Cannot create cell faces without face owner information");
    return nullptr;
  }
  if (!this->FaceNeigh)
  {
    vtkErrorMacro(<< "Cannot create cell faces without face neighbour information");
    return nullptr;
  }
  const bool use64BitLabels = ::Is64BitArray(this->FaceOwner);

  const vtkDataArray& faceOwner = *this->FaceOwner;
  const vtkDataArray& faceNeigh = *this->FaceNeigh;

  const vtkIdType nFaces = faceOwner.GetNumberOfTuples();
  const vtkIdType nInternalFaces = faceNeigh.GetNumberOfTuples();

  // Extra safety (consistency)
  this->NumFaces = nFaces;
  this->NumInternalFaces = nInternalFaces;

  // Recalculate number of cells if needed
  if (this->NumCells == 0)
  {
    vtkTypeInt64 nCells = -1;
    for (vtkIdType facei = 0; facei < nFaces; ++facei)
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceOwner, facei, use64BitLabels);
      nCells = std::max(nCells, celli);
    }
    for (vtkIdType facei = 0; facei < nInternalFaces; ++facei)
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceNeigh, facei, use64BitLabels);
      nCells = std::max(nCells, celli);
    }

    // Set the number of cells
    this->NumCells = static_cast<vtkIdType>(++nCells);
  }

  // The number of cells
  const vtkTypeInt64 nCells = this->NumCells;

  // Total number of cell faces
  const vtkTypeInt64 nTotalCellFaces =
    static_cast<vtkTypeInt64>(nFaces) + static_cast<vtkTypeInt64>(nInternalFaces);

  // Create meshCells. Avoid 32bit overflow for nTotalCellFaces
  std::unique_ptr<vtkFoamLabelListList> meshCells;
  if (use64BitLabels || (VTK_TYPE_INT32_MAX < nTotalCellFaces))
  {
    meshCells.reset(new vtkFoamLabelListList64);
  }
  else
  {
    meshCells.reset(new vtkFoamLabelListList32);
  }
  auto& cells = *meshCells;

  cells.ResizeExact(nCells, nTotalCellFaces);
  cells.ResetOffsets(); // Fill offsets with zero

  // Count number of faces for each cell
  // Establish the per-cell face count
  {
    // Accumulate offsets into slot *above*
    constexpr vtkIdType cellIndexOffset = 1;

    for (vtkIdType facei = 0; facei < nFaces; ++facei)
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceOwner, facei, use64BitLabels);
      cells.IncrementOffset(cellIndexOffset + celli);
    }
    for (vtkIdType facei = 0; facei < nInternalFaces; ++facei)
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceNeigh, facei, use64BitLabels);
      cells.IncrementOffset(cellIndexOffset + celli);
    }

    // Reduce per-cell face count -> start offsets
    vtkTypeInt64 currOffset = 0;
    for (vtkIdType celli = 1; celli <= nCells; ++celli)
    {
      currOffset += cells.GetBeginOffset(celli);
      cells.SetOffset(celli, currOffset);
    }
  }

  // Deep copy of offsets into a temporary array
  std::unique_ptr<vtkFoamLabelListList> tmpAddr;
  if (cells.IsLabel64())
  {
    tmpAddr.reset(new vtkFoamLabelListList64);
  }
  else
  {
    tmpAddr.reset(new vtkFoamLabelListList32);
  }
  tmpAddr->ResizeExact(nCells, 1);
  tmpAddr->GetOffsetsArray()->DeepCopy(cells.GetOffsetsArray());

  // Add face numbers to cell-faces list, using tmpAddr offsets to manage the locations
  for (vtkIdType facei = 0; facei < nInternalFaces; ++facei)
  {
    // owner
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceOwner, facei, use64BitLabels);
      const vtkTypeInt64 next = tmpAddr->GetBeginOffset(celli);
      tmpAddr->IncrementOffset(celli);
      cells.SetValue(next, facei);
    }
    // neighbour
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceNeigh, facei, use64BitLabels);
      const vtkTypeInt64 next = tmpAddr->GetBeginOffset(celli);
      tmpAddr->IncrementOffset(celli);
      cells.SetValue(next, facei);
    }
  }

  for (vtkIdType facei = nInternalFaces; facei < nFaces; ++facei)
  {
    // owner
    {
      const vtkTypeInt64 celli = GetLabelValue(&faceOwner, facei, use64BitLabels);
      const vtkTypeInt64 next = tmpAddr->GetBeginOffset(celli);
      tmpAddr->IncrementOffset(celli);
      cells.SetValue(next, facei);
    }
  }

  return meshCells;
}

//------------------------------------------------------------------------------
bool vtkOpenFOAMReaderPrivate::CheckFaceList(const vtkFoamLabelListList& faces)
{
  const vtkIdType nFaces = faces.GetNumberOfElements();
  const vtkIdType nPoints = this->NumPoints;

  vtkFoamLabelListList::CellType face;
  for (vtkIdType facei = 0; facei < nFaces; ++facei)
  {
    faces.GetCell(facei, face);

    if (face.size() < 3)
    {
      vtkErrorMacro(<< "Face " << facei << " is bad. Has " << face.size()
                    << " points but requires 3 or more");
      return false;
    }

    for (const vtkTypeInt64 pointi : face)
    {
      if (pointi < 0 || pointi >= nPoints)
      {
        vtkErrorMacro(<< "Face " << facei << " is bad. Point " << pointi
                      << " out of range: " << nPoints << " points");
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// determine cell shape and insert the cell into the mesh
// hexahedron, prism, pyramid, tetrahedron and decompose polyhedron
void vtkOpenFOAMReaderPrivate::InsertCellsToGrid(
  vtkUnstructuredGrid* internalMesh, std::unique_ptr<vtkFoamLabelListList>& meshCellsPtr,
  const vtkFoamLabelListList& meshFaces, vtkIdList* cellLabels
#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  ,
  vtkIdTypeArray* additionalCells, vtkFloatArray* pointArray
#endif
)
{
  // Scratch arrays
  vtkFoamStackVector<vtkIdType, 256> cellPoints;  // For inserting primitive cell points
  vtkFoamStackVector<vtkIdType, 1024> polyPoints; // For inserting polyhedral faces and sizes
  vtkFoamLabelListList::CellType cellFaces;       // For analyzing cell types (shapes)
  vtkFoamLabelListList::CellType facePoints;      // For processing individual cell faces

  const bool faceOwner64Bit = ::Is64BitArray(this->FaceOwner);
  const bool cellLabels64Bit = faceOwner64Bit; // reasonable assumption

  const vtkIdType nCells = (cellLabels == nullptr ? this->NumCells : cellLabels->GetNumberOfIds());

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  // Local variable for polyhedral decomposition
  vtkIdType nAdditionalPoints = 0;

  if (additionalCells && cellLabels) // sanity check
  {
    vtkErrorMacro(<< "Decompose polyhedral is not supported on mesh subset");
    return;
  }
#endif
  if (!nCells)
  {
    return;
  }
  if (!meshCellsPtr)
  {
    meshCellsPtr = this->CreateCellFaces();
  }
  const auto& meshCells = *meshCellsPtr;

  for (vtkIdType celli = 0; celli < nCells; ++celli)
  {
    vtkIdType cellId = celli;
    if (cellLabels != nullptr)
    {
      cellId = cellLabels->GetId(celli);
      if (cellId < 0 || cellId >= this->NumCells)
      {
        // sanity check. bad values should have been removed before this
        vtkWarningMacro(<< "cellLabels id " << cellId << " exceeds the number of cells " << nCells);
        continue;
      }
    }

    meshCells.GetCell(cellId, cellFaces);

    // determine type of the cell
    // cf. src/OpenFOAM/meshes/meshShapes/cellMatcher/{hex|prism|pyr|tet}-
    // Matcher.C

    int cellType = VTK_POLYHEDRON; // Fallback value
    if (cellFaces.size() == 6)
    {
      // Check for HEXAHEDRON
      bool allQuads = false;
      for (size_t facei = 0; facei < cellFaces.size(); ++facei)
      {
        allQuads = (meshFaces.GetSize(cellFaces[facei]) == 4);
        if (!allQuads)
        {
          break;
        }
      }
      if (allQuads)
      {
        cellType = VTK_HEXAHEDRON;
      }
    }
    else if (cellFaces.size() == 5)
    {
      // Check for WEDGE or PYRAMID
      int nTris = 0, nQuads = 0;
      for (size_t facei = 0; facei < cellFaces.size(); ++facei)
      {
        const vtkIdType nPoints = meshFaces.GetSize(cellFaces[facei]);
        if (nPoints == 3)
        {
          ++nTris;
        }
        else if (nPoints == 4)
        {
          ++nQuads;
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
    else if (cellFaces.size() == 4)
    {
      // Check for TETRA
      bool allTris = false;
      for (size_t facei = 0; facei < cellFaces.size(); ++facei)
      {
        allTris = (meshFaces.GetSize(cellFaces[facei]) == 3);
        if (!allTris)
        {
          break;
        }
      }
      if (allTris)
      {
        cellType = VTK_TETRA;
      }
    }

    // Cell shape constructor based on the one implemented by Terry
    // Jordan, with lots of improvements. Not as elegant as the one in
    // OpenFOAM but it's simple and works reasonably fast.

    // Note: faces are flipped around their 0 point (as per OpenFOAM)
    // to keep predictable face point ordering

    // OpenFOAM "hex" | vtkHexahedron
    if (cellType == VTK_HEXAHEDRON)
    {
      int nCellPoints = 0;

      // Get first face in correct order
      {
        const vtkTypeInt64 cellFacei = cellFaces[0];
        const bool isOwner = (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
        meshFaces.GetCell(cellFacei, facePoints);

        // Add face0 to cell points - flip owner to point inwards
        cellPoints[nCellPoints++] = facePoints[0];
        if (isOwner)
        {
          for (int fp = 3; fp > 0; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
        else
        {
          for (int fp = 1; fp < 4; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
      }
      const vtkIdType baseFacePoint0 = cellPoints[0];
      const vtkIdType baseFacePoint2 = cellPoints[2];
      vtkTypeInt64 cellOppositeFaceI = -1;
      vtkTypeInt64 pivotMeshPoint = -1;
      int dupPoint = -1;
      for (int facei = 1; facei < 5; ++facei) // Skip face 0 (already done) and 5 (fallback)
      {
        const vtkTypeInt64 cellFacei = cellFaces[facei];
        const bool isOwner = (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
        meshFaces.GetCell(cellFacei, facePoints);

        int foundDup = -1;
        int pointI = 0;
        for (; pointI < 4; ++pointI) // each face point
        {
          // matching two points in base face is enough to find a
          // duplicated point since neighboring faces share two
          // neighboring points (i. e. an edge)
          if (baseFacePoint0 == facePoints[pointI])
          {
            foundDup = 0;
            break;
          }
          else if (baseFacePoint2 == facePoints[pointI])
          {
            foundDup = 2;
            break;
          }
        }
        if (foundDup == -1)
        {
          // No duplicate points found, this is the opposite face
          cellOppositeFaceI = cellFacei;
          if (pivotMeshPoint >= 0)
          {
            break;
          }
        }
        else if (pivotMeshPoint == -1)
        {
          // Has duplicate point(s) - find the pivot point if still unknown
          dupPoint = foundDup;

          const vtkTypeInt64 faceNextPoint = facePoints[(pointI + 1) % 4];
          const vtkTypeInt64 facePrevPoint = facePoints[(3 + pointI) % 4];

          // if the next point of the faceI-th face matches the
          // previous point of the base face use the previous point
          // of the faceI-th face as the pivot point; or use the
          // next point otherwise
          if (faceNextPoint == (isOwner ? cellPoints[1 + foundDup] : cellPoints[3 - foundDup]))
          {
            pivotMeshPoint = facePrevPoint;
          }
          else
          {
            pivotMeshPoint = faceNextPoint;
          }

          if (cellOppositeFaceI >= 0)
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

      // Find the pivot point in opposite face
      meshFaces.GetCell(cellOppositeFaceI, facePoints);
      int pivotPointI = 0;
      for (; pivotPointI < 4; ++pivotPointI)
      {
        if (pivotMeshPoint == facePoints[pivotPointI])
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

      // Copy last (opposite) face in correct order. Copy into cellPoints list
      {
        const bool isOwner =
          (cellId == GetLabelValue(this->FaceOwner, cellOppositeFaceI, faceOwner64Bit));

        if (isOwner)
        {
          for (int fp = pivotPointI; fp < 4; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
          for (int fp = 0; fp < pivotPointI; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
        else
        {
          for (int fp = pivotPointI; fp >= 0; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
          for (int fp = 3; fp > pivotPointI; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
      }

      // Add HEXAHEDRON (hex) cell to the mesh
      internalMesh->InsertNextCell(VTK_HEXAHEDRON, 8, cellPoints.data());
    }

    // OpenFOAM "prism" | vtkWedge
    // - cell construction similar to "hex",
    // but the OpenFOAM face0 points inwards (like hex) and VTK face0 points outwards
    // so point ordering is reversed
    else if (cellType == VTK_WEDGE)
    {
      int nCellPoints = 0;

      // Find the base face number and get it in correct order
      int baseFaceId = 0;
      {
        for (int facei = 0; facei < 5; ++facei)
        {
          if (meshFaces.GetSize(cellFaces[facei]) == 3)
          {
            baseFaceId = facei;
            break;
          }
        }

        const vtkTypeInt64 cellFacei = cellFaces[baseFaceId];
        const bool isOwner = (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
        meshFaces.GetCell(cellFacei, facePoints);

        // Add face0 to cell points - flip neighbour to point outwards
        // - OpenFOAM face0 points inwards
        // - VTK face0 points outwards
        cellPoints[nCellPoints++] = facePoints[0];
        if (isOwner)
        {
          for (int fp = 1; fp < 3; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
        else
        {
          for (int fp = 2; fp > 0; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
      }
      const vtkIdType baseFacePoint0 = cellPoints[0];
      const vtkIdType baseFacePoint2 = cellPoints[2];

      vtkTypeInt64 cellOppositeFaceI = -1;
      vtkTypeInt64 pivotMeshPoint = -1;
      bool dupPoint2 = false;
      // Search for opposite face and pivot point
      for (int facei = 0; facei < 5; ++facei)
      {
        if (facei == baseFaceId)
        {
          continue;
        }
        const vtkTypeInt64 cellFacei = cellFaces[facei];
        if (meshFaces.GetSize(cellFacei) == 3)
        {
          cellOppositeFaceI = cellFacei;
        }
        else if (pivotMeshPoint == -1)
        {
          // Find the pivot point if still unknown
          const bool isOwner =
            (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
          meshFaces.GetCell(cellFacei, facePoints);

          bool found0Dup = false;
          int pointI = 0;
          for (; pointI < 4; ++pointI) // each face point
          {
            // matching two points in base face is enough to find a
            // duplicated point since neighboring faces share two
            // neighboring points (i. e. an edge)
            if (baseFacePoint0 == facePoints[pointI])
            {
              found0Dup = true;
              break;
            }
            else if (baseFacePoint2 == facePoints[pointI])
            {
              break;
            }
          }
          // the matching point must always be found so omit the check
          vtkIdType baseFacePrevPoint;
          vtkIdType baseFaceNextPoint;
          if (found0Dup)
          {
            baseFacePrevPoint = cellPoints[2];
            baseFaceNextPoint = cellPoints[1];
          }
          else
          {
            baseFacePrevPoint = cellPoints[1];
            baseFaceNextPoint = cellPoints[0];
            dupPoint2 = true;
          }

          const vtkTypeInt64 faceNextPoint = facePoints[(pointI + 1) % 4];
          const vtkTypeInt64 facePrevPoint = facePoints[(3 + pointI) % 4];

          // if the next point of the faceI-th face matches the
          // previous point of the base face use the previous point of
          // the faceI-th face as the pivot point; or use the next
          // point otherwise

          if (faceNextPoint == (isOwner ? baseFacePrevPoint : baseFaceNextPoint))
          {
            pivotMeshPoint = facePrevPoint;
          }
          else
          {
            pivotMeshPoint = faceNextPoint;
          }
        }

        // break when both of opposite face and pivot point are found
        if (cellOppositeFaceI >= 0 && pivotMeshPoint >= 0)
        {
          break;
        }
      }

      // Find the pivot point in opposite face
      meshFaces.GetCell(cellOppositeFaceI, facePoints);
      int pivotPointI = -1;
      for (int fp = 0; fp < 3; ++fp)
      {
        if (pivotMeshPoint == facePoints[fp])
        {
          pivotPointI = fp;
          break;
        }
      }

      if (pivotPointI == -1)
      {
        // No pivot found - does not look like a wedge, process as polyhedron instead.
        cellType = VTK_POLYHEDRON;
      }
      else
      {
        // Found a pivot - can process cell as a wedge
        const bool isOwner =
          (cellId == GetLabelValue(this->FaceOwner, cellOppositeFaceI, faceOwner64Bit));

        if (isOwner)
        {
          if (dupPoint2)
          {
            pivotPointI = (pivotPointI + 2) % 3;
          }
          for (int fp = pivotPointI; fp >= 0; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
          for (int fp = 2; fp > pivotPointI; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
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
          // copy the face-point list of the opposite face to cellPoints list
          for (int fp = pivotPointI; fp < 3; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
          for (int fp = 0; fp < pivotPointI; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }

        // Add WEDGE (prism) cell to the mesh
        internalMesh->InsertNextCell(VTK_WEDGE, 6, cellPoints.data());
      }
    }

    // OpenFOAM "pyr" | vtkPyramid || OpenFOAM "tet" | vtkTetrahedron
    else if (cellType == VTK_PYRAMID || cellType == VTK_TETRA)
    {
      int nCellPoints = 0;
      int baseFaceId = 0;
      if (cellType == VTK_PYRAMID)
      {
        // Find the pyramid base
        for (size_t facei = 0; facei < cellFaces.size(); ++facei)
        {
          if (meshFaces.GetSize(cellFaces[facei]) == 4)
          {
            baseFaceId = static_cast<int>(facei);
            break;
          }
        }
      }

      // Add base-face points to cell points - flip for owner (to point inwards)
      {
        const vtkTypeInt64 cellFacei = cellFaces[baseFaceId];
        const bool isOwner = (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
        meshFaces.GetCell(cellFacei, facePoints);
        const size_t nFacePoints = facePoints.size();

        cellPoints[nCellPoints++] = facePoints[0];
        if (isOwner)
        {
          for (size_t fp = nFacePoints - 1; fp > 0; --fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
        else
        {
          for (size_t fp = 1; fp < nFacePoints; ++fp)
          {
            cellPoints[nCellPoints++] = facePoints[fp];
          }
        }
      }

      // Take any other face to find the apex point
      vtkFoamLabelListList::CellType otherFacePoints;
      meshFaces.GetCell(cellFaces[(baseFaceId ? 0 : 1)], otherFacePoints);

      // Find the apex point (non-common to the base)
      // initialize with anything
      // - if the search really fails, we have much bigger problems anyhow
      vtkIdType apexMeshPointi = 0;
      for (size_t otheri = 0; otheri < otherFacePoints.size(); ++otheri)
      {
        apexMeshPointi = otherFacePoints[otheri];
        bool isUnique = true;
        for (size_t fp = 0; isUnique && fp < facePoints.size(); ++fp)
        {
          isUnique = (apexMeshPointi != facePoints[fp]);
        }
        if (isUnique)
        {
          break;
        }
      }

      // ... and add the apex-point
      cellPoints[nCellPoints++] = apexMeshPointi;

      // Add tetra or pyramid to the mesh
      internalMesh->InsertNextCell(cellType, nCellPoints, cellPoints.data());
    }

    // Polyhedron cell (vtkPolyhedron)
    if (cellType == VTK_POLYHEDRON)
    {
      // Preliminary checks for sizes and sanity check

      size_t nPolyPoints = 0;
      {
        bool allEmpty = true;
        for (size_t facei = 0; facei < cellFaces.size(); ++facei)
        {
          const size_t nFacePoints = meshFaces.GetSize(cellFaces[facei]);
          nPolyPoints += nFacePoints;
          if (nFacePoints)
          {
            allEmpty = false;
          }
        }
        if (allEmpty)
        {
          vtkWarningMacro("Warning: No points in cellId " << cellId);
          internalMesh->InsertNextCell(VTK_EMPTY_CELL, 0, cellPoints.data());
          continue;
        }
      }

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
      if (additionalCells != nullptr)
      {
        // Decompose into tets and pyramids

        // Calculate cell centroid and insert it to point list
        vtkDataArray* polyCellPoints;
        if (cellLabels64Bit)
        {
          polyCellPoints = vtkTypeInt64Array::New();
        }
        else
        {
          polyCellPoints = vtkTypeInt32Array::New();
        }
        this->AdditionalCellPoints->push_back(polyCellPoints);

        double centroid[3];
        centroid[0] = centroid[1] = centroid[2] = 0; // zero the contents
        for (size_t facei = 0; facei < cellFaces.size(); ++facei)
        {
          // Eliminate duplicate points from faces
          const vtkTypeInt64 cellFacei = cellFaces[facei];
          meshFaces.GetCell(cellFacei, facePoints);
          for (size_t fp = 0; fp < facePoints.size(); ++fp)
          {
            const vtkTypeInt64 meshPointi = facePoints[fp];
            bool isUnique = true;
            for (vtkIdType cp = 0; isUnique && cp < polyCellPoints->GetDataSize(); ++cp)
            {
              isUnique = (meshPointi != GetLabelValue(polyCellPoints, cp, cellLabels64Bit));
            }
            if (isUnique)
            {
              AppendLabelValue(polyCellPoints, meshPointi, cellLabels64Bit);
              const float* tuple = pointArray->GetPointer(3 * meshPointi);
              centroid[0] += static_cast<double>(tuple[0]);
              centroid[1] += static_cast<double>(tuple[1]);
              centroid[2] += static_cast<double>(tuple[2]);
            }
          }
        }
        polyCellPoints->Squeeze();
        {
          const double weight = 1.0 / static_cast<double>(polyCellPoints->GetDataSize());
          centroid[0] *= weight;
          centroid[1] *= weight;
          centroid[2] *= weight;
        }
        pointArray->InsertNextTuple(centroid);

        // polyhedron decomposition.
        // a tweaked algorithm based on OpenFOAM
        // src/fileFormats/vtk/part/foamVtuSizingTemplates.C

        // TODO: improve consistency of face point ordering.
        // - currently just flips the faces without preserving the face point 0 order.
        bool firstCell = true;
        int nAdditionalCells = 0;
        for (size_t facei = 0; facei < cellFaces.size(); ++facei)
        {
          const vtkTypeInt64 cellFacei = cellFaces[facei];
          const bool isOwner =
            (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
          meshFaces.GetCell(cellFacei, facePoints);
          const size_t nFacePoints = facePoints.size();

          const int flipNeighbor = (isOwner ? -1 : 1);
          const size_t nTris = (nFacePoints % 2);

          size_t vertI = 2;

          // shift the start and end of the vertex loop if the
          // triangle of a decomposed face is going to be flat. Far
          // from perfect but better than nothing to avoid flat cells
          // which stops time integration of Stream Tracer especially
          // for split-hex unstructured meshes created by
          // e. g. autoRefineMesh
          if (nFacePoints >= 5 && nTris)
          {
            const float* point0 = pointArray->GetPointer(3 * facePoints[nFacePoints - 1]);
            const float* point1 = pointArray->GetPointer(3 * facePoints[0]);
            const float* point2 = pointArray->GetPointer(3 * facePoints[nFacePoints - 2]);
            float vsizeSqr1 = 0.0F, vsizeSqr2 = 0.0F, dotProduct = 0.0F;
            for (int i = 0; i < 3; i++)
            {
              const float v1 = point1[i] - point0[i];
              const float v2 = point2[i] - point0[i];
              vsizeSqr1 += v1 * v1;
              vsizeSqr2 += v2 * v2;
              dotProduct += v1 * v2;
            }
            // compare in squared representation to avoid using sqrt()
            if (dotProduct * (float)fabs(dotProduct) / (vsizeSqr1 * vsizeSqr2) < -1.0F + 1.0e-3F)
            {
              vertI = 1;
            }
          }

          cellPoints[0] = facePoints[(vertI == 2) ? static_cast<vtkIdType>(0)
                                                  : static_cast<vtkIdType>(nFacePoints - 1)];
          cellPoints[4] = static_cast<vtkIdType>(this->NumPoints + nAdditionalPoints); // apex

          // Decompose a face into quads in order (flipping decomposed face if owner)
          const size_t nQuadVerts = nFacePoints - 1 - nTris;
          for (; vertI < nQuadVerts; vertI += 2)
          {
            cellPoints[1] = facePoints[vertI - flipNeighbor];
            cellPoints[2] = facePoints[vertI];
            cellPoints[3] = facePoints[vertI + flipNeighbor];

            // Insert first decomposed cell into the original position,
            // subsequent ones are appended to the decomposed cell list
            if (firstCell)
            {
              firstCell = false;
              internalMesh->InsertNextCell(VTK_PYRAMID, 5, cellPoints.data());
            }
            else
            {
              ++nAdditionalCells;
              additionalCells->InsertNextTypedTuple(cellPoints.data());
            }
          }

          // if the number of vertices is odd there's a triangle
          if (nTris)
          {
            if (flipNeighbor == -1) // isOwner
            {
              cellPoints[1] = facePoints[vertI];
              cellPoints[2] = facePoints[vertI - 1];
            }
            else
            {
              cellPoints[1] = facePoints[vertI - 1];
              cellPoints[2] = facePoints[vertI];
            }
            cellPoints[3] = static_cast<vtkIdType>(this->NumPoints + nAdditionalPoints);

            // Insert first decomposed cell into the original position,
            // subsequent ones are appended to the decomposed cell list
            if (firstCell)
            {
              firstCell = false;
              internalMesh->InsertNextCell(VTK_TETRA, 4, cellPoints.data());
            }
            else
            {
              // set the 5th vertex number to -1 to distinguish a tetra cell
              cellPoints[4] = -1;
              ++nAdditionalCells;
              additionalCells->InsertNextTypedTuple(cellPoints.data());
            }
          }
        }

        ++nAdditionalPoints;
        this->AdditionalCellIds->InsertNextValue(cellId);
        this->NumAdditionalCells->InsertNextValue(nAdditionalCells);
        this->NumTotalAdditionalCells += nAdditionalCells;
      }
      else
#endif // VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
      {
        // Not decomposed - using VTK_POLYHEDRON

        // Precalculated 'nPolyPoints' has all face points, including duplicates
        // - need nPolyPoints + nPolyFaces for the face loops
        // - use nPolyPoints to estimate unique cell points
        //   (assume a point connects at least three faces)

        cellPoints.copy_resize(0);
        polyPoints.copy_resize(0);
        cellPoints.copy_reserve(nPolyPoints / 3);
        polyPoints.copy_reserve(nPolyPoints + cellFaces.size());

        size_t nCellPoints = 0;
        nPolyPoints = 0; // Reset

        for (size_t facei = 0; facei < cellFaces.size(); ++facei)
        {
          const vtkTypeInt64 cellFacei = cellFaces[facei];
          const bool isOwner =
            (cellId == GetLabelValue(this->FaceOwner, cellFacei, faceOwner64Bit));
          meshFaces.GetCell(cellFacei, facePoints);
          const size_t nFacePoints = facePoints.size();
          size_t nUnique = 0;

          // Pass 1: add face points, and mark up duplicates on the way

          polyPoints.copy_resize(nPolyPoints + nFacePoints + 1);
          polyPoints[nPolyPoints++] = static_cast<vtkIdType>(nFacePoints);

          if (!nFacePoints)
          {
            continue;
          }

          // Add face point 0
          {
            const auto meshPointi = static_cast<vtkIdType>(facePoints[0]);
            polyPoints[nPolyPoints++] = meshPointi;
            bool isUnique = true;
            for (size_t cp = 0; isUnique && cp < nCellPoints; ++cp)
            {
              isUnique = (meshPointi != cellPoints[cp]);
            }
            if (isUnique)
            {
              ++nUnique;
            }
            else
            {
              facePoints[0] = -1; // Duplicate
            }
          }

          // Add other face points
          {
            // Local face point indexing - must be signed
            const int faceDirn = (isOwner ? 1 : -1); // Flip direction for neighbour face
            int facePointi = (isOwner ? 1 : static_cast<int>(nFacePoints) - 1);

            for (size_t fp = 1; fp < nFacePoints; ++fp, facePointi += faceDirn)
            {
              const auto meshPointi = static_cast<vtkIdType>(facePoints[facePointi]);
              polyPoints[nPolyPoints++] = meshPointi;
              bool isUnique = true;
              for (size_t cp = 0; isUnique && cp < nCellPoints; ++cp)
              {
                isUnique = (meshPointi != cellPoints[cp]);
              }
              if (isUnique)
              {
                ++nUnique;
              }
              else
              {
                facePoints[facePointi] = -1; // Duplicate
              }
            }
          }

          cellPoints.copy_resize(nCellPoints + nUnique);

          // Pass 2: add unique cell points - order is arbitrary
          for (size_t fp = 0; fp < nFacePoints; ++fp)
          {
            const auto meshPointi = static_cast<vtkIdType>(facePoints[fp]);
            if (meshPointi != -1) // isUnique
            {
              cellPoints[nCellPoints++] = meshPointi;
            }
          }
        }

        // Create the poly cell and insert it into the mesh
        internalMesh->InsertNextCell(VTK_POLYHEDRON, static_cast<vtkIdType>(nCellPoints),
          cellPoints.data(), static_cast<vtkIdType>(cellFaces.size()), polyPoints.data());
      }
    }
  }
}

//------------------------------------------------------------------------------
// derive cell types and create the internal mesh
vtkUnstructuredGrid* vtkOpenFOAMReaderPrivate::MakeInternalMesh(
  std::unique_ptr<vtkFoamLabelListList>& meshCellsPtr, const vtkFoamLabelListList& meshFaces,
  vtkFloatArray* pointArray)
{
  // Create Mesh
  auto* internalMesh = vtkUnstructuredGrid::New();
  internalMesh->Allocate(this->NumCells);

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  if (this->Parent->GetDecomposePolyhedra())
  {
    // For polyhedral decomposition
    this->NumTotalAdditionalCells = 0;
    this->AdditionalCellIds = vtkIdTypeArray::New();
    this->NumAdditionalCells = vtkIntArray::New();
    this->AdditionalCellPoints = new vtkFoamLabelArrayVector;

    vtkNew<vtkIdTypeArray> additionalCells;
    additionalCells->SetNumberOfComponents(5); // Accommodate tetra or pyramid

    this->InsertCellsToGrid(
      internalMesh, meshCellsPtr, meshFaces, nullptr, additionalCells, pointArray);

    // For polyhedral decomposition
    pointArray->Squeeze();
    this->AdditionalCellIds->Squeeze();
    this->NumAdditionalCells->Squeeze();
    additionalCells->Squeeze();

    // Insert decomposed cells into mesh
    const int nComponents = additionalCells->GetNumberOfComponents(); // Should still be 5
    const vtkIdType nAdditionalCells = additionalCells->GetNumberOfTuples();
    for (vtkIdType i = 0; i < nAdditionalCells; i++)
    {
      if (additionalCells->GetComponent(i, 4) == -1)
      {
        internalMesh->InsertNextCell(VTK_TETRA, 4, additionalCells->GetPointer(i * nComponents));
      }
      else
      {
        internalMesh->InsertNextCell(VTK_PYRAMID, 5, additionalCells->GetPointer(i * nComponents));
      }
    }
    internalMesh->Squeeze();
  }
  else
#endif // VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  {
    this->InsertCellsToGrid(internalMesh, meshCellsPtr, meshFaces);
  }

  // Set points for internalMesh
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);
  internalMesh->SetPoints(points);

  return internalMesh;
}

//------------------------------------------------------------------------------
// Insert faces to grid
void vtkOpenFOAMReaderPrivate::InsertFacesToGrid(vtkPolyData* boundaryMesh,
  const vtkFoamLabelListList& meshFaces, vtkIdType startFace, vtkIdType endFace,
  vtkIdList* faceLabels, vtkDataArray* pointMap, bool isLookupValue)
{
  vtkPolyData& bm = *boundaryMesh;

  // Limits
  const vtkIdType maxLabels = this->FaceOwner->GetNumberOfTuples(); // NumFaces

  // A per-face scratch array for vtkIdType ids.
  vtkFoamStackVector<vtkIdType, 64> facePointIds;

  for (vtkIdType facei = startFace; facei < endFace; ++facei)
  {
    vtkIdType faceId = facei;
    if (faceLabels != nullptr)
    {
      faceId = faceLabels->GetId(facei);
      if (faceId < 0 || faceId >= maxLabels)
      {
        // sanity check. bad values should have been removed before this
        vtkWarningMacro(<< "faceLabels id " << faceId << " exceeds number of faces " << maxLabels);
        continue;
      }
    }

    const int nFacePoints = static_cast<int>(meshFaces.GetSize(faceId));
    facePointIds.fast_resize(nFacePoints);

    if (isLookupValue)
    {
      for (int fp = 0; fp < nFacePoints; ++fp)
      {
        const auto meshPointi = static_cast<vtkIdType>(meshFaces.GetValue(faceId, fp));
        facePointIds[fp] = pointMap->LookupValue(meshPointi);
      }
    }
    else if (pointMap)
    {
      const bool pointMap64Bit = ::Is64BitArray(pointMap); // null-safe
      for (int fp = 0; fp < nFacePoints; ++fp)
      {
        const auto meshPointi = static_cast<vtkIdType>(meshFaces.GetValue(faceId, fp));
        facePointIds[fp] = GetLabelValue(pointMap, meshPointi, pointMap64Bit);
      }
    }
    else
    {
      for (int fp = 0; fp < nFacePoints; ++fp)
      {
        const auto meshPointi = static_cast<vtkIdType>(meshFaces.GetValue(faceId, fp));
        facePointIds[fp] = meshPointi;
      }
    }

    const int vtkFaceType =
      (nFacePoints == 3 ? VTK_TRIANGLE : nFacePoints == 4 ? VTK_QUAD : VTK_POLYGON);
    bm.InsertNextCell(vtkFaceType, nFacePoints, facePointIds.data());
  }
}

//------------------------------------------------------------------------------
// Returns requested boundary meshes
vtkMultiBlockDataSet* vtkOpenFOAMReaderPrivate::MakeBoundaryMesh(
  const vtkFoamLabelListList& meshFaces, vtkFloatArray* pointArray)
{
  const auto& patches = this->BoundaryDict;
  const vtkIdType nBoundaries = static_cast<vtkIdType>(patches.size());

  // Final consistency check for boundaries
  if (patches.endFace() > meshFaces.GetNumberOfElements())
  {
    vtkErrorMacro(<< "The boundary describes " << patches.startFace() << " to "
                  << (patches.endFace() - 1) << " faces, but mesh only has "
                  << meshFaces.GetNumberOfElements() << " faces");
    return nullptr;
  }

  auto* boundaryMesh = vtkMultiBlockDataSet::New();

  if (this->Parent->GetCreateCellToPoint())
  {
    this->AllBoundaries = vtkPolyData::New();
    this->AllBoundaries->AllocateEstimate(
      // ==> nBoundaryFaces
      meshFaces.GetNumberOfElements() - patches.startFace(), 1);
  }
  this->BoundaryPointMap = new vtkFoamLabelArrayVector;

  // Use same integer width as per faces
  const bool meshPoints64Bit = meshFaces.IsLabel64();

  // create initial internal point list: set all points to -1
  if (this->Parent->GetCreateCellToPoint())
  {
    if (meshPoints64Bit)
    {
      this->InternalPoints = vtkTypeInt64Array::New();
    }
    else
    {
      this->InternalPoints = vtkTypeInt32Array::New();
    }
    this->InternalPoints->SetNumberOfValues(this->NumPoints);
    this->InternalPoints->FillComponent(0, -1);

    // Mark boundary points as 0
    for (const vtkFoamPatch& patch : patches)
    {
      if (patch.type_ == vtkFoamPatch::PHYSICAL || patch.type_ == vtkFoamPatch::PROCESSOR)
      {
        const vtkIdType startFace = patch.startFace();
        const vtkIdType endFace = patch.endFace();

        for (vtkIdType facei = startFace; facei < endFace; ++facei)
        {
          const vtkIdType nFacePoints = meshFaces.GetSize(facei);
          for (vtkIdType pointi = 0; pointi < nFacePoints; ++pointi)
          {
            SetLabelValue(
              this->InternalPoints, meshFaces.GetValue(facei, pointi), 0, meshPoints64Bit);
          }
        }
      }
    }
  }

  vtkTypeInt64 nAllBoundaryPoints = 0;
  std::vector<std::vector<vtkIdType>> procCellList;
  vtkSmartPointer<vtkTypeInt8Array> pointTypes;

  if (this->Parent->GetCreateCellToPoint())
  {
    // Create global to AllBoundaries point map
    for (vtkIdType pointi = 0; pointi < this->NumPoints; ++pointi)
    {
      if (GetLabelValue(this->InternalPoints, pointi, meshPoints64Bit) == 0)
      {
        SetLabelValue(this->InternalPoints, pointi, nAllBoundaryPoints, meshPoints64Bit);
        nAllBoundaryPoints++;
      }
    }

    if (!this->ProcessorName.empty())
    {
      // Initialize physical-processor boundary shared point list
      procCellList.resize(static_cast<size_t>(nAllBoundaryPoints));
      pointTypes = vtkSmartPointer<vtkTypeInt8Array>::New();
      pointTypes->SetNumberOfTuples(nAllBoundaryPoints);
      pointTypes->FillValue(0);
    }
  }

  for (vtkIdType patchi = 0; patchi < nBoundaries; ++patchi)
  {
    const vtkFoamPatch& patch = patches[patchi];
    const vtkIdType startFace = patch.startFace();
    const vtkIdType endFace = patch.endFace();
    const vtkIdType nFaces = patch.size_;

    if (this->Parent->GetCreateCellToPoint() &&
      (patch.type_ == vtkFoamPatch::PHYSICAL || patch.type_ == vtkFoamPatch::PROCESSOR))
    {
      // Add faces to AllBoundaries
      this->InsertFacesToGrid(
        this->AllBoundaries, meshFaces, startFace, endFace, nullptr, this->InternalPoints, false);

      if (!this->ProcessorName.empty())
      {
        // Mark belonging boundary types and, if PROCESSOR, cell numbers
        const vtkIdType absStartFace = patch.offset_;
        const vtkIdType absEndFace = absStartFace + nFaces;
        for (vtkIdType facei = absStartFace; facei < absEndFace; ++facei)
        {
          vtkIdType nPoints;
          const vtkIdType* points;
          this->AllBoundaries->GetCellPoints(facei, nPoints, points);
          if (patch.type_ == vtkFoamPatch::PHYSICAL)
          {
            for (vtkIdType pointi = 0; pointi < nPoints; ++pointi)
            {
              *pointTypes->GetPointer(points[pointi]) |= vtkFoamPatch::PHYSICAL;
            }
          }
          else
          {
            // PROCESSOR
            for (vtkIdType pointi = 0; pointi < nPoints; ++pointi)
            {
              const vtkIdType procPoint = points[pointi];
              *pointTypes->GetPointer(procPoint) |= vtkFoamPatch::PROCESSOR;
              procCellList[procPoint].push_back(facei);
            }
          }
        }
      }
    }

    // Skip below if not active
    if (!patches.isActive(patch.index_))
    {
      continue;
    }

    // Create the boundary patch mesh
    vtkNew<vtkPolyData> bm;
    ::AppendBlock(boundaryMesh, bm, patch.name_);
    bm->AllocateEstimate(nFaces, 1);

    // Local to global point index mapping
    vtkDataArray* bpMap;
    if (meshPoints64Bit)
    {
      bpMap = vtkTypeInt64Array::New();
    }
    else
    {
      bpMap = vtkTypeInt32Array::New();
    }
    this->BoundaryPointMap->push_back(bpMap);

    // The point locations for the boundary
    vtkNew<vtkFloatArray> boundaryPointArray;
    boundaryPointArray->SetNumberOfComponents(3);

    // In OpenFOAM-1.5 and earlier, meshPoints were in increasing order
    // but this gave problems in processor point synchronisation.
    // - now uses the order in which faces are visited

    // Visit order
    // - normally with unordered_map to create a global to local map.
    //   However, we later use LookupValue() method on a regular list,
    //   so unordered_set is sufficient
    {
      // A global to local map for marking points
      std::unordered_set<vtkTypeInt64> markedPoints;

      vtkFoamLabelListList::CellType face;
      for (vtkIdType facei = startFace; facei < endFace; ++facei)
      {
        meshFaces.GetCell(facei, face);

        for (const auto meshPointi : face)
        {
          auto insertion = markedPoints.emplace(meshPointi);
          if (insertion.second)
          {
            // A previously unvisited point
            boundaryPointArray->InsertNextTuple(pointArray->GetPointer(3 * meshPointi));
            AppendLabelValue(bpMap, meshPointi, meshPoints64Bit);
          }
        }
      }
    }
    bpMap->Squeeze();
    boundaryPointArray->Squeeze();

    vtkNew<vtkPoints> boundaryPoints;
    boundaryPoints->SetData(boundaryPointArray);

    // Set points for boundary
    bm->SetPoints(boundaryPoints);

    // Insert faces to boundary mesh
    this->InsertFacesToGrid(bm, meshFaces, startFace, endFace, nullptr, bpMap, true);
    bpMap->ClearLookup();
  }

  if (this->Parent->GetCreateCellToPoint())
  {
    this->AllBoundaries->Squeeze();
    if (meshPoints64Bit)
    {
      this->AllBoundariesPointMap = vtkTypeInt64Array::New();
    }
    else
    {
      this->AllBoundariesPointMap = vtkTypeInt32Array::New();
    }
    vtkDataArray& abpMap = *this->AllBoundariesPointMap;
    abpMap.SetNumberOfValues(nAllBoundaryPoints);

    // create lists of internal points and AllBoundaries points
    vtkIdType nInternalPoints = 0;
    for (vtkIdType pointI = 0, allBoundaryPointI = 0; pointI < this->NumPoints; pointI++)
    {
      vtkIdType globalPointId = GetLabelValue(this->InternalPoints, pointI, meshPoints64Bit);
      if (globalPointId == -1)
      {
        SetLabelValue(this->InternalPoints, nInternalPoints, pointI, meshPoints64Bit);
        nInternalPoints++;
      }
      else
      {
        SetLabelValue(&abpMap, allBoundaryPointI, pointI, meshPoints64Bit);
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
      this->InternalPoints = nullptr;
    }

    // set dummy vtkPoints to tell the grid the number of points
    // (otherwise GetPointCells will crash)
    vtkNew<vtkPoints> allBoundaryPoints;
    allBoundaryPoints->SetNumberOfPoints(abpMap.GetNumberOfTuples());
    this->AllBoundaries->SetPoints(allBoundaryPoints);

    if (!this->ProcessorName.empty())
    {
      // remove links to processor boundary faces from point-to-cell
      // links of physical-processor shared points to avoid cracky seams
      // on fixedValue-type boundaries which are noticeable when all the
      // decomposed meshes are appended
      this->AllBoundaries->BuildLinks();
      for (int pointI = 0; pointI < nAllBoundaryPoints; pointI++)
      {
        if (pointTypes->GetValue(pointI) == (vtkFoamPatch::PHYSICAL | vtkFoamPatch::PROCESSOR))
        {
          const std::vector<vtkIdType>& procCells = procCellList[pointI];
          for (size_t cellI = 0; cellI < procCellList[pointI].size(); cellI++)
          {
            this->AllBoundaries->RemoveReferenceToCell(pointI, procCells[cellI]);
          }
          // omit reclaiming memory as the possibly recovered size should
          // not typically be so large
        }
      }
    }
  }

  return boundaryMesh;
}

//------------------------------------------------------------------------------
// Move mesh points, including the cell centroids for any decomposed polyhedra
bool vtkOpenFOAMReaderPrivate::MoveInternalMesh(
  vtkUnstructuredGrid* internalMesh, vtkFloatArray* pointArray)
{
  const auto nOldPoints = internalMesh->GetPoints()->GetNumberOfPoints();

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  if (this->Parent->GetDecomposePolyhedra() && this->AdditionalCellPoints &&
    !this->AdditionalCellPoints->empty())
  {
    const auto& addCellPoints = *this->AdditionalCellPoints;
    const vtkIdType nAddPoints = static_cast<vtkIdType>(addCellPoints.size());
    pointArray->Resize(this->NumPoints + nAddPoints);

    const bool cellPoints64Bit = ::Is64BitArray(this->AdditionalCellPoints->front());

    double centroid[3];
    for (vtkIdType i = 0, newPointi = this->NumPoints; i < nAddPoints; ++i, ++newPointi)
    {
      vtkDataArray* polyCellPoints = addCellPoints[i];
      const vtkIdType nCellPoints = polyCellPoints->GetDataSize();

      centroid[0] = centroid[1] = centroid[2] = 0; // zero contents
      for (vtkIdType j = 0; j < nCellPoints; j++)
      {
        const vtkTypeInt64 polyCellPointi = GetLabelValue(polyCellPoints, j, cellPoints64Bit);
        const float* tuple = pointArray->GetPointer(3 * polyCellPointi);
        centroid[0] += static_cast<double>(tuple[0]);
        centroid[1] += static_cast<double>(tuple[1]);
        centroid[2] += static_cast<double>(tuple[2]);
      }
      if (nCellPoints)
      {
        const double weight = 1.0 / static_cast<double>(nCellPoints);
        centroid[0] *= weight;
        centroid[1] *= weight;
        centroid[2] *= weight;
      }
      pointArray->InsertTuple(newPointi, centroid);
    }
  }
#endif // VTK_FOAMFILE_DECOMPOSE_POLYHEDRA

  if (nOldPoints != pointArray->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Mismatch in number of old points (" << nOldPoints << ") and new points ("
                  << pointArray->GetNumberOfTuples() << ')');
    return false;
  }

  // Update the mesh points. No Delete on pointArray (still used outside)
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);
  internalMesh->SetPoints(points);

  return true;
}

//------------------------------------------------------------------------------
// Move boundary points
bool vtkOpenFOAMReaderPrivate::MoveBoundaryMesh(
  vtkMultiBlockDataSet* boundaryMesh, vtkFloatArray* pointArray)
{
  const auto& patches = this->BoundaryDict;

  unsigned int activeBoundaryIndex = 0;
  for (const vtkFoamPatch& patch : patches)
  {
    if (patches.isActive(patch.index_))
    {
      auto* bm = vtkPolyData::SafeDownCast(boundaryMesh->GetBlock(activeBoundaryIndex));
      vtkDataArray* bpMap = this->BoundaryPointMap->operator[](activeBoundaryIndex);
      ++activeBoundaryIndex;

      const vtkIdType nBoundaryPoints = bpMap->GetNumberOfTuples();
      const bool meshPoints64Bit = ::Is64BitArray(bpMap);

      vtkNew<vtkFloatArray> boundaryPointArray;
      boundaryPointArray->SetNumberOfComponents(3);
      boundaryPointArray->SetNumberOfTuples(nBoundaryPoints);
      for (vtkIdType pointi = 0; pointi < nBoundaryPoints; ++pointi)
      {
        const auto meshPointi = GetLabelValue(bpMap, pointi, meshPoints64Bit);
        boundaryPointArray->SetTuple(pointi, meshPointi, pointArray);
      }
      vtkNew<vtkPoints> boundaryPoints;
      boundaryPoints->SetData(boundaryPointArray);

      bm->SetPoints(boundaryPoints);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
// as of now the function does not do interpolation, but do just averaging.
void vtkOpenFOAMReaderPrivate::InterpolateCellToPoint(vtkFloatArray* pData, vtkFloatArray* iData,
  vtkPointSet* mesh, vtkDataArray* pointList, vtkTypeInt64 nPoints)
{
  if (nPoints == 0)
  {
    return;
  }

  const bool meshPoints64Bit = ::Is64BitArray(pointList);

  // a dummy call to let GetPointCells() build the cell links if still not built
  // (not using BuildLinks() since it always rebuild links)
  mesh->GetPointCells(0, vtkNew<vtkIdList>());

  // Set up to grab point cells
  auto* ug = vtkUnstructuredGrid::SafeDownCast(mesh);
  auto* pd = vtkPolyData::SafeDownCast(mesh);
  vtkIdType nCells;
  vtkIdType* cells;

  const int nComponents = iData->GetNumberOfComponents();

  if (nComponents == 1)
  {
    // a special case with the innermost componentI loop unrolled
    float* tuples = iData->GetPointer(0);
    for (vtkTypeInt64 pointI = 0; pointI < nPoints; pointI++)
    {
      vtkTypeInt64 pI = pointList ? GetLabelValue(pointList, pointI, meshPoints64Bit) : pointI;
      if (ug)
      {
        ug->GetPointCells(pI, nCells, cells);
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
      interpolatedValue = (nCells ? interpolatedValue / static_cast<double>(nCells) : 0.0);
      pData->SetValue(pI, static_cast<float>(interpolatedValue));
    }
  }
  else if (nComponents == 3)
  {
    // a special case with the innermost componentI loop unrolled
    float* pDataPtr = pData->GetPointer(0);
    for (vtkTypeInt64 pointI = 0; pointI < nPoints; pointI++)
    {
      vtkTypeInt64 pI = pointList ? GetLabelValue(pointList, pointI, meshPoints64Bit) : pointI;
      if (ug)
      {
        ug->GetPointCells(pI, nCells, cells);
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
        const float* tuple = iData->GetPointer(3 * cells[cellI]);
        summedValue0 += tuple[0];
        summedValue1 += tuple[1];
        summedValue2 += tuple[2];
      }

      float* interpolatedValue = &pDataPtr[3 * pI];
      interpolatedValue[0] = static_cast<float>(weight * summedValue0);
      interpolatedValue[1] = static_cast<float>(weight * summedValue1);
      interpolatedValue[2] = static_cast<float>(weight * summedValue2);
    }
  }
  else
  {
    float* pDataPtr = pData->GetPointer(0);
    for (vtkTypeInt64 pointI = 0; pointI < nPoints; pointI++)
    {
      vtkTypeInt64 pI = pointList ? GetLabelValue(pointList, pointI, meshPoints64Bit) : pointI;
      if (ug)
      {
        ug->GetPointCells(pI, nCells, cells);
      }
      else
      {
        pd->GetPointCells(pI, nCells, cells);
      }

      // use double intermediate variables for precision
      const double weight = (nCells ? 1.0 / static_cast<double>(nCells) : 0.0);
      float* interpolatedValue = &pDataPtr[nComponents * pI];
      // a bit strange loop order but this works fastest
      for (int componentI = 0; componentI < nComponents; componentI++)
      {
        const float* tuple = iData->GetPointer(componentI);
        double summedValue = 0.0;
        for (int cellI = 0; cellI < nCells; cellI++)
        {
          summedValue += tuple[nComponents * cells[cellI]];
        }
        interpolatedValue[componentI] = static_cast<float>(weight * summedValue);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenFOAMReaderPrivate::ReadFieldFile(vtkFoamIOobject& io, vtkFoamDict& dict,
  const std::string& varName, const vtkDataArraySelection* selection)
{
  const std::string varPath(this->CurrentTimeRegionPath() + "/" + varName);

  // Open the file
  if (!io.Open(varPath))
  {
    vtkErrorMacro(<< "Error opening " << io.GetFileName() << ": " << io.GetError());
    return false;
  }

  // if the variable is disabled on selection panel then skip it
  if (selection->ArrayExists(io.GetObjectName().c_str()) &&
    !selection->ArrayIsEnabled(io.GetObjectName().c_str()))
  {
    return false;
  }

  // Read the field file into dictionary
  if (!dict.Read(io))
  {
    vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                  << ": " << io.GetError());
    return false;
  }

  if (dict.GetType() != vtkFoamToken::DICTIONARY)
  {
    vtkErrorMacro(<< "File " << io.GetFileName() << "is not valid as a field file");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkFloatArray> vtkOpenFOAMReaderPrivate::FillField(vtkFoamEntry& entry,
  vtkIdType nElements, const vtkFoamIOobject& io, vtkFoamTypes::dataType fieldDataType)
{
  const std::string& className = io.GetClassName();

  vtkSmartPointer<vtkFloatArray> data;
  if (entry.FirstValue().IsUniform())
  {
    if (entry.FirstValue().IsNumeric())
    {
      const float num = entry.ToFloat();
      data = vtkSmartPointer<vtkFloatArray>::New();
      data->SetNumberOfValues(nElements);
      data->FillValue(num);
    }
    else
    {
      float tupleBuffer[9], *tuple;
      int nComponents = -1;
      // have to determine the type of vector
      if (entry.FirstValue().GetType() == vtkFoamToken::LABELLIST)
      {
        vtkDataArray& ll = entry.LabelList();
        nComponents = static_cast<int>(ll.GetNumberOfTuples());
        for (int componentI = 0; componentI < nComponents; componentI++)
        {
          tupleBuffer[componentI] = static_cast<float>(ll.GetTuple1(componentI));
        }
        tuple = tupleBuffer;
      }
      else if (entry.FirstValue().GetType() == vtkFoamToken::SCALARLIST)
      {
        vtkFloatArray& sl = entry.ScalarList();
        nComponents = static_cast<int>(sl.GetSize());
        tuple = sl.GetPointer(0);
      }
      else
      {
        vtkErrorMacro(<< "Wrong list type for uniform field: " << io.GetObjectName());
        return nullptr;
      }

      if ((vtkFoamTypes::GetNumberOfComponents(fieldDataType) == nComponents) &&
        vtkFoamTypes::IsVectorSpace(fieldDataType))
      {
        data = vtkSmartPointer<vtkFloatArray>::New();
        data->SetNumberOfComponents(nComponents);
        data->SetNumberOfTuples(nElements);
        for (vtkIdType i = 0; i < nElements; i++)
        {
          data->SetTuple(i, tuple);
        }
      }
      else
      {
        vtkErrorMacro(<< "Number of components and field class do not match "
                      << "for " << io.GetFileName() << ". class = " << className
                      << ", nComponents = " << nComponents);
        return nullptr;
      }
    }
  }
  else // nonuniform
  {
    if ((entry.FirstValue().GetType() == vtkFoamToken::SCALARLIST &&
          vtkFoamTypes::IsScalar(fieldDataType)) ||
      (entry.FirstValue().GetType() == vtkFoamToken::VECTORLIST &&
        vtkFoamTypes::IsVectorSpace(fieldDataType)))
    {
      const vtkIdType nTuples = entry.ScalarList().GetNumberOfTuples();
      if (nTuples != nElements)
      {
        vtkErrorMacro(<< "Number of cells/points in mesh and field do not match: "
                      << "mesh = " << nElements << ", field = " << nTuples << " in "
                      << io.GetObjectName());
        return nullptr;
      }

      // Capture content as smart pointer
      data.TakeReference(entry.ReleasePtr<vtkFloatArray>());
    }
    else if (entry.FirstValue().GetType() == vtkFoamToken::EMPTYLIST && nElements <= 0)
    {
      data = vtkSmartPointer<vtkFloatArray>::New();

      // Set appropriate number of components for empty list as well
      const int nComp = vtkFoamTypes::GetNumberOfComponents(fieldDataType);
      if (nComp > 0)
      {
        data->SetNumberOfComponents(nComp);
      }
    }
    else
    {
      vtkErrorMacro(<< io.GetFileName() << " is not a valid " << io.GetClassName());
      return nullptr;
    }
  }
  return data;
}

//------------------------------------------------------------------------------
// Convert OpenFOAM dimension array to string representation
std::string vtkOpenFOAMReaderPrivate::ConstructDimensions(const vtkFoamDict& dict) const
{
  const int nDimensions = 7; // There are 7 base dimensions
  static const char* units[7] = { "kg", "m", "s", "K", "mol", "A", "cd" };

  if (!this->Parent->GetAddDimensionsToArrayNames())
  {
    return std::string();
  }

  const vtkFoamEntry* dimEntry = dict.Lookup("dimensions");
  if ((dimEntry == nullptr) || (dimEntry->FirstValue().GetType() != vtkFoamToken::SCALARLIST))
  {
    return std::string();
  }

  const vtkFloatArray& values = dimEntry->ScalarList();
  const vtkIdType nValues = values.GetNumberOfTuples();

  // Expect seven dimensions, but may have only the first five.
  // OpenFOAM accepts both and so do we.
  if (nValues != 5 && nValues != nDimensions)
  {
    return std::string();
  }

  // Make a copy
  float dims[7] = { 0 };

  for (vtkIdType i = 0; i < nValues; ++i)
  {
    dims[i] = values.GetValue(i);
  }

  const auto equal = // Compare floats with rounding
    [](const float a, const float b) { return (std::abs(a - b) < 1e-3); };

  const auto integral = // Test if integral/non-integral
    [](const float val) { return (std::abs(val - std::round(val)) < 1e-4); };

  // Stringify. Use stringstream to build the string
  std::ostringstream dimensions, denominator;
  dimensions << " [";
  int nPositive = 0;
  int nNegative = 0;

  // Some standard units
  if (equal(dims[0], 1) && equal(dims[1], -1) && equal(dims[2], -2))
  {
    dimensions << "Pa";
    nPositive = 1;
    dims[0] = dims[1] = dims[2] = 0;
  }
  else if (equal(dims[0], 1) && equal(dims[1], 1) && equal(dims[2], -2))
  {
    dimensions << "N";
    nPositive = 1;
    dims[0] = dims[1] = dims[2] = 0;
  }
  else if (equal(dims[0], 1) && equal(dims[1], 2) && equal(dims[2], -3))
  {
    dimensions << "W";
    nPositive = 1;
    dims[0] = dims[1] = dims[2] = 0;
  }
  // Note: cannot know if 'J' or 'N m' is the better representation, so skip that one

  for (int dimi = 0; dimi < nDimensions; ++dimi)
  {
    float expon = dims[dimi];

    if (expon > 0)
    {
      if (nPositive++)
      {
        dimensions << ' ';
      }
      dimensions << units[dimi];
      if (equal(expon, 1))
      {
        continue;
      }
      if (!integral(expon))
      {
        dimensions << '^';
      }
      dimensions << expon;
    }
    else if (expon < 0)
    {
      expon = -expon;
      if (nNegative++)
      {
        denominator << ' ';
      }
      denominator << units[dimi];
      if (equal(expon, 1))
      {
        continue;
      }
      if (!integral(expon))
      {
        denominator << '^';
      }
      denominator << expon;
    }
  }

  // Finalize, adding denominator as required
  if (nNegative)
  {
    if (nPositive == 0)
    {
      // No numerator
      dimensions << '1';
    }
    dimensions << '/';

    if (nNegative > 1)
    {
      dimensions << '(' << denominator.str() << ')';
    }
    else
    {
      dimensions << denominator.str();
    }
  }
  else if (nPositive == 0)
  {
    // No dimensions
    dimensions << '-';
  }

  dimensions << ']';
  return dimensions.str();
}

//------------------------------------------------------------------------------
// Read volume or internal field at a timestep
void vtkOpenFOAMReaderPrivate::GetVolFieldAtTimeStep(
  const std::string& varName, bool isInternalField)
{
  // Where to map data
  vtkUnstructuredGrid* internalMesh = this->InternalMesh;
  vtkMultiBlockDataSet* boundaryMesh = this->BoundaryMesh;

  // Boundary information
  const auto& patches = this->BoundaryDict;
  const bool faceOwner64Bit = ::Is64BitArray(this->FaceOwner);

  vtkFoamIOobject io(this->CasePath, this->Parent);
  vtkFoamDict dict;
  if (!this->ReadFieldFile(io, dict, varName, this->Parent->CellDataArraySelection))
  {
    return;
  }

  // For internal field (eg, volScalarField::Internal)
  const bool hasColons = (io.GetClassName().find("::Internal") != std::string::npos);

  if ((io.GetClassName().compare(0, 3, "vol") != 0) ||
    (hasColons ? !isInternalField : isInternalField))
  {
    vtkErrorMacro(<< io.GetFileName() << " is not a volume/internal field");
    return;
  }

  // Eg, from "volScalarField" or "volScalarField::Internal" -> SCALAR_TYPE
  const auto fieldDataType(vtkFoamTypes::FieldToEnum(io.GetClassName(), 3));

  // -------------------------
  // Handle dictionary lookups first

  // The "dimensions" entry - stringify
  const std::string dimString(this->ConstructDimensions(dict));

  // The "internalField" entry, or "value" for Dimensioned field
  vtkFoamEntry* ifieldEntry = nullptr;
  {
    const std::string entryName = (isInternalField ? "value" : "internalField");

    ifieldEntry = dict.Lookup(entryName);
    if (ifieldEntry == nullptr)
    {
      vtkErrorMacro(<< entryName << " not found in " << io.GetFileName());
      return;
    }
    else if (ifieldEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
    {
      if (this->NumCells)
      {
        vtkErrorMacro(<< entryName << " of " << io.GetFileName() << " is empty");
      }
      return;
    }
  }

  // The "boundaryField" entry
  const vtkFoamEntry* bfieldEntries = nullptr;
  if (!isInternalField)
  {
    bfieldEntries = dict.Lookup("boundaryField");
    if (bfieldEntries == nullptr)
    {
      vtkWarningMacro(<< "boundaryField not found in " << io.GetFileName()
                      << " at time = " << this->TimeNames->GetValue(this->TimeStep));
      return;
    }
  }

  // -------------------------

  // Generate internal field data
  vtkSmartPointer<vtkFloatArray> iData =
    this->FillField(*ifieldEntry, this->NumCells, io, fieldDataType);
  if (iData == nullptr)
  {
    return;
  }
  else if (iData->GetSize() == 0)
  {
    // Determined that there are no cells. Ignore the field
    return;
  }

  // Invariant for this field
  const int nComponents = iData->GetNumberOfComponents();

  // The cell-to-point interpolated data for cells
  vtkSmartPointer<vtkFloatArray> ctpData;

  // Add field only if internal Mesh exists (skip if not selected).
  // Note we still need to read internalField even if internal mesh is
  // not selected, since boundaries without value entries may refer to
  // the internalField.
  if (internalMesh != nullptr)
  {
#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
    if (this->Parent->GetDecomposePolyhedra() && this->NumTotalAdditionalCells > 0)
    {
      // Add values for decomposed cells
      const vtkIdType nTuples = this->AdditionalCellIds->GetNumberOfTuples();
      vtkIdType newCelli = this->NumCells;
      iData->Resize(this->NumCells + this->NumTotalAdditionalCells);

      for (vtkIdType tupleI = 0; tupleI < nTuples; tupleI++)
      {
        const int nvals = this->NumAdditionalCells->GetValue(tupleI);
        const vtkIdType cellId = this->AdditionalCellIds->GetValue(tupleI);
        for (int vali = 0; vali < nvals; ++vali)
        {
          iData->InsertTuple(newCelli++, cellId, iData);
        }
      }
    }
#endif // VTK_FOAMFILE_DECOMPOSE_POLYHEDRA

    // Set data to internal mesh
    ::AddArrayToFieldData(internalMesh->GetCellData(), iData, io.GetObjectName(), dimString);

    if (this->Parent->GetCreateCellToPoint())
    {
      // Create cell-to-point interpolated data
      ctpData = vtkSmartPointer<vtkFloatArray>::New();
      ctpData->SetNumberOfComponents(nComponents);
      ctpData->SetNumberOfTuples(internalMesh->GetPoints()->GetNumberOfPoints());
      if (this->InternalPoints != nullptr)
      {
        this->InterpolateCellToPoint(ctpData, iData, internalMesh, this->InternalPoints,
          this->InternalPoints->GetNumberOfTuples());
      }

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
      if (this->Parent->GetDecomposePolyhedra())
      {
        // assign cell values to additional points
        const vtkIdType nAddPoints = this->AdditionalCellIds->GetNumberOfTuples();
        for (vtkIdType pointi = 0, newPointi = this->NumPoints; pointi < nAddPoints;
             ++pointi, ++newPointi)
        {
          ctpData->SetTuple(newPointi, this->AdditionalCellIds->GetValue(pointi), iData);
        }
      }
#endif // VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
    }
  }

  // Handle cell zones
  if (this->Parent->CopyDataToCellZones && this->CellZoneMesh)
  {
    auto* zoneMesh = this->CellZoneMesh;
    auto& zoneMap = this->cellZoneMap;
    unsigned int nZones = zoneMesh->GetNumberOfBlocks();

    if (nZones && zoneMap.empty()) // sanity check
    {
      vtkWarningMacro(<< "No cellLabels saved for zones");
      nZones = 0;
    }

    for (unsigned int zonei = 0; zonei < nZones; ++zonei)
    {
      auto* zm = vtkUnstructuredGrid::SafeDownCast(zoneMesh->GetBlock(zonei));

      const vtkIdType nZoneCells = zm->GetNumberOfCells();
      vtkIdList* cellLabels = nullptr;

      const std::string zoneName = zoneMesh->GetMetaData(zonei)->Get(vtkCompositeDataSet::NAME());

      {
        cellLabels = zoneMap.findZone(zoneName);
        if (cellLabels == nullptr)
        {
          if (nZoneCells) // sanity check
          {
            vtkWarningMacro(<< "No cellLabels saved for zone: " << zoneName);
          }
          continue;
        }

        const vtkIdType nLabels = cellLabels->GetNumberOfIds();
        if (nLabels != nZoneCells) // sanity check
        {
          vtkWarningMacro(<< "Mismatch in cellLabels (" << nLabels << ") and number of cells ("
                          << nZoneCells << ") for zone: " << zoneName);
          continue;
        }
      }

      vtkNew<vtkFloatArray> zoneCellData;
      zoneCellData->SetNumberOfComponents(nComponents);
      zoneCellData->SetNumberOfTuples(nZoneCells);

      // Copy tuples from internalField
      // Like SetTuple() without the extra checks
      for (vtkIdType celli = 0; celli < nZoneCells; ++celli)
      {
        float* dstData = zoneCellData->GetPointer(celli * nComponents);
        const vtkIdType cellId = cellLabels->GetId(celli);
        const float* srcData = iData->GetPointer(cellId * nComponents);

        for (int cmpt = 0; cmpt < nComponents; ++cmpt)
        {
          dstData[cmpt] = srcData[cmpt];
        }
      }

      // Set data to zone mesh
      ::AddArrayToFieldData(zm->GetCellData(), zoneCellData, io.GetObjectName(), dimString);

      // Point data temporarily unavailable (needs more reworking)
      // // Copy points data
      // zm->GetPointData()->ShallowCopy(this->InternalMesh->GetPointData());
    }
  }

  // The cell-to-point interpolated data for boundary cells
  vtkSmartPointer<vtkFloatArray> acData;

  if (boundaryMesh != nullptr)
  {
    if (this->Parent->GetCreateCellToPoint())
    {
      acData = vtkSmartPointer<vtkFloatArray>::New();
      acData->SetNumberOfComponents(nComponents);
      acData->SetNumberOfTuples(this->AllBoundaries->GetNumberOfCells());
    }

    unsigned int activeBoundaryIndex = 0;
    vtkFoamError warnings;

    for (const vtkFoamPatch& patch : patches)
    {
      const vtkIdType nFaces = patch.size_;

      vtkSmartPointer<vtkFloatArray> vData;
      std::string bcType;

      if (bfieldEntries != nullptr)
      {
        bool badEntry = false;

        const vtkFoamEntry* bfieldEntry = bfieldEntries->Dictionary().Lookup(patch.name_, true);
        if (bfieldEntry == nullptr)
        {
          badEntry = true;
          warnings << "not found";
        }
        else if (bfieldEntry->FirstValue().GetType() != vtkFoamToken::DICTIONARY)
        {
          badEntry = true;
          warnings << "not a subdictionary";
        }
        else
        {
          const vtkFoamDict& patchDict = bfieldEntry->Dictionary();

          // Look for "value" entry
          vtkFoamEntry* vEntry = patchDict.Lookup("value");
          if (vEntry == nullptr)
          {
            // For alternative fallback
            const vtkFoamEntry* eptr = patchDict.Lookup("type");
            if (eptr != nullptr)
            {
              bcType = eptr->ToString();
            }
          }
          else
          {
            vData = this->FillField(*vEntry, nFaces, io, fieldDataType);
            if (vData == nullptr)
            {
              badEntry = true;
            }
          }
        }

        // If anything unexpected happened, get out
        if (badEntry)
        {
          if (!warnings.empty())
          {
            vtkWarningMacro(<< "boundaryField " << patch.name_ << ' ' << warnings << " in object "
                            << varName
                            << " at time = " << this->TimeNames->GetValue(this->TimeStep));
          }
          return;
        }
      }

      // Relative start into the FaceOwner list, which may have been truncated (boundaries only)
      // or have its original length

      const vtkIdType boundaryStartFace = patch.start_ -
        (this->FaceOwner->GetNumberOfTuples() < this->NumFaces ? patches.startFace() : 0);

      if (vData == nullptr)
      {
        // No "value" entry
        // Default to patch-internal values as boundary values
        vData = vtkSmartPointer<vtkFloatArray>::New();
        vData->SetNumberOfComponents(nComponents);
        vData->SetNumberOfTuples(nFaces);

        // Ad hoc handling of some known bcs without a "value"
        if (bcType == "noSlip")
        {
          vData->FillValue(0);
        }
        else
        {
          for (vtkIdType bFacei = 0; bFacei < nFaces; ++bFacei)
          {
            const vtkTypeInt64 own =
              GetLabelValue(this->FaceOwner, bFacei + boundaryStartFace, faceOwner64Bit);

            vData->SetTuple(bFacei, own, iData);
          }
        }
      }

      if (this->Parent->GetCreateCellToPoint())
      {
        const vtkIdType startFace = patch.offset_;

        // if reading a processor sub-case of a decomposed case as is,
        // use the patch values of the processor patch as is
        if (patch.type_ == vtkFoamPatch::PHYSICAL ||
          (this->ProcessorName.empty() && patch.type_ == vtkFoamPatch::PROCESSOR))
        {
          // set the same value to AllBoundaries
          for (vtkIdType bFacei = 0; bFacei < nFaces; ++bFacei)
          {
            acData->SetTuple(bFacei + startFace, bFacei, vData);
          }
        }
        // implies && !this->ProcessorName.empty()
        else if (patch.type_ == vtkFoamPatch::PROCESSOR)
        {
          // average patch internal value and patch value assuming the
          // patch value to be the patchInternalField of the neighbor
          // decomposed mesh. Using double precision to avoid degrade in
          // accuracy.
          for (vtkIdType bFacei = 0; bFacei < nFaces; ++bFacei)
          {
            const vtkIdType own =
              GetLabelValue(this->FaceOwner, bFacei + boundaryStartFace, faceOwner64Bit);

            const float* vTuple = vData->GetPointer(bFacei * nComponents);
            const float* iTuple = iData->GetPointer(own * nComponents);
            float* acTuple = acData->GetPointer((bFacei + startFace) * nComponents);

            for (int cmpt = 0; cmpt < nComponents; ++cmpt)
            {
              acTuple[cmpt] = static_cast<float>(
                0.5 * (static_cast<double>(vTuple[cmpt]) + static_cast<double>(iTuple[cmpt])));
            }
          }
        }
      }

      if (patches.isActive(patch.index_))
      {
        auto* bm = vtkPolyData::SafeDownCast(boundaryMesh->GetBlock(activeBoundaryIndex));
        ++activeBoundaryIndex;

        ::AddArrayToFieldData(bm->GetCellData(), vData, io.GetObjectName(), dimString);

        if (this->Parent->GetCreateCellToPoint())
        {
          // construct cell-to-point interpolated boundary values. This
          // is done independently from allBoundary interpolation so
          // that the interpolated values are not affected by
          // neighboring patches especially at patch edges and for
          // baffle patches
          vtkNew<vtkFloatArray> pData;
          pData->SetNumberOfComponents(vData->GetNumberOfComponents());
          const vtkIdType nPoints = bm->GetPoints()->GetNumberOfPoints();
          pData->SetNumberOfTuples(nPoints);
          this->InterpolateCellToPoint(pData, vData, bm, nullptr, nPoints);
          ::AddArrayToFieldData(bm->GetPointData(), pData, io.GetObjectName(), dimString);
        }
      }
    }

    if (this->Parent->GetCreateCellToPoint())
    {
      // Create cell-to-point interpolated data for all boundaries and
      // override internal values
      vtkNew<vtkFloatArray> bpData;
      bpData->SetNumberOfComponents(nComponents);
      vtkIdType nPoints = this->AllBoundariesPointMap->GetNumberOfTuples();
      bpData->SetNumberOfTuples(nPoints);
      this->InterpolateCellToPoint(bpData, acData, this->AllBoundaries, nullptr, nPoints);

      if (ctpData)
      {
        const bool meshPoints64Bit = ::Is64BitArray(this->AllBoundariesPointMap);

        // set cell-to-pint data for internal mesh
        for (vtkIdType pointI = 0; pointI < nPoints; pointI++)
        {
          ctpData->SetTuple(
            GetLabelValue(this->AllBoundariesPointMap, pointI, meshPoints64Bit), pointI, bpData);
        }
        ::AddArrayToFieldData(internalMesh->GetPointData(), ctpData, io.GetObjectName(), dimString);
      }
    }
  }

  // Handle face zones, only possible if FaceOwner has not been truncated
  if (this->Parent->CopyDataToCellZones && this->FaceZoneMesh && this->FaceNeigh)
  {
    auto& zoneMap = this->faceZoneMap;
    auto* zoneMesh = this->FaceZoneMesh;
    unsigned int nZones = zoneMesh->GetNumberOfBlocks();

    if (nZones && zoneMap.empty()) // sanity check
    {
      vtkWarningMacro(<< "No faceLabels saved for zones");
      nZones = 0;
    }

    // Or NumInternalFaces...
    const vtkIdType nInternalFaces = this->FaceNeigh->GetNumberOfTuples();

    for (unsigned int zonei = 0; zonei < nZones; ++zonei)
    {
      auto* zm = vtkPolyData::SafeDownCast(zoneMesh->GetBlock(zonei));

      const vtkIdType nZoneFaces = zm->GetNumberOfCells();
      vtkIdList* faceLabels = nullptr;

      const std::string zoneName = zoneMesh->GetMetaData(zonei)->Get(vtkCompositeDataSet::NAME());

      {
        faceLabels = zoneMap.findZone(zoneName);
        if (faceLabels == nullptr)
        {
          if (nZoneFaces) // sanity check
          {
            vtkWarningMacro(<< "No faceLabels saved for zone: " << zoneName);
          }
          continue;
        }

        const vtkIdType nLabels = faceLabels->GetNumberOfIds();
        if (nLabels != nZoneFaces) // sanity check
        {
          vtkWarningMacro(<< "Mismatch in faceLabels (" << nLabels << ") and number of faces ("
                          << nZoneFaces << ") for zone: " << zoneName);
          continue;
        }
      }

      vtkNew<vtkFloatArray> zoneCellData;
      zoneCellData->SetNumberOfComponents(nComponents);
      zoneCellData->SetNumberOfTuples(nZoneFaces);

      // Copy tuples from internalField
      // TODO: revise this to use boundaryField values...
      for (vtkIdType facei = 0; facei < nZoneFaces; ++facei)
      {
        float* dstData = zoneCellData->GetPointer(facei * nComponents);
        const vtkIdType faceId = faceLabels->GetId(facei);
        const vtkIdType ownCell = GetLabelValue(this->FaceOwner, faceId, faceOwner64Bit);
        const float* ownData = iData->GetPointer(ownCell * nComponents);

        if (faceId < nInternalFaces)
        {
          // Internal face. Use average
          const vtkIdType neiCell = GetLabelValue(this->FaceNeigh, faceId, faceOwner64Bit);
          const float* neiData = iData->GetPointer(neiCell * nComponents);

          for (int cmpt = 0; cmpt < nComponents; ++cmpt)
          {
            dstData[cmpt] = static_cast<float>(
              0.5 * (static_cast<double>(ownData[cmpt]) + static_cast<double>(neiData[cmpt])));
          }
        }
        else
        {
          // Boundary face.
          // For now, just use owner side (cell) information.
          // Revise/improve in the future (2021-02-04)
          for (int cmpt = 0; cmpt < nComponents; ++cmpt)
          {
            dstData[cmpt] = ownData[cmpt];
          }
        }
      }

      // Set data to zone mesh
      ::AddArrayToFieldData(zm->GetCellData(), zoneCellData, io.GetObjectName(), dimString);

      // Point data temporarily unavailable (needs more reworking)
      // // Copy points data
      // zm->GetPointData()->ShallowCopy(this->InternalMesh->GetPointData());
    }
  }
}

//------------------------------------------------------------------------------
// Read point field at a timestep
void vtkOpenFOAMReaderPrivate::GetPointFieldAtTimeStep(const std::string& varName)
{
  // Where to map data
  vtkUnstructuredGrid* internalMesh = this->InternalMesh;
  vtkMultiBlockDataSet* boundaryMesh = this->BoundaryMesh;

  // Boundary information
  const auto& patches = this->BoundaryDict;

  vtkFoamIOobject io(this->CasePath, this->Parent);
  vtkFoamDict dict;
  if (!this->ReadFieldFile(io, dict, varName, this->Parent->PointDataArraySelection))
  {
    return;
  }

  if (io.GetClassName().compare(0, 5, "point") != 0)
  {
    vtkErrorMacro(<< io.GetFileName() << " is not a pointField");
    return;
  }

  // Eg, from "pointScalarField" -> SCALAR_TYPE
  const auto fieldDataType(vtkFoamTypes::FieldToEnum(io.GetClassName(), 5));

  // -------------------------
  // Handle dictionary lookups first

  // The "dimensions" entry - stringify
  const std::string dimString(this->ConstructDimensions(dict));

  // The "internalField" entry
  vtkFoamEntry* ifieldEntry = nullptr;
  {
    ifieldEntry = dict.Lookup("internalField");
    if (ifieldEntry == nullptr)
    {
      vtkErrorMacro(<< "internalField not found in " << io.GetFileName());
      return;
    }
    else if (ifieldEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
    {
      if (this->NumPoints)
      {
        vtkErrorMacro(<< "internalField of " << io.GetFileName() << " is empty");
      }
      return;
    }
  }

  // The "boundaryField" entry
  const vtkFoamEntry* bfieldEntries = nullptr;
  {
    bfieldEntries = dict.Lookup("boundaryField");
    if (bfieldEntries == nullptr)
    {
      vtkWarningMacro(<< "boundaryField not found in " << io.GetFileName()
                      << " at time = " << this->TimeNames->GetValue(this->TimeStep));
      return;
    }
  }

  // -------------------------

  // Generate internal field data
  vtkSmartPointer<vtkFloatArray> iData =
    this->FillField(*ifieldEntry, this->NumPoints, io, fieldDataType);
  if (iData == nullptr)
  {
    return;
  }
  else if (iData->GetSize() == 0)
  {
    // Determined that there are no points. Ignore the field
    return;
  }

  // Invariant for this field
  const int nComponents = iData->GetNumberOfComponents();

  // Add field only if internal Mesh exists (skip if not selected).
  // Note we still need to read internalField even if internal mesh is
  // not selected, since boundaries without value entries may refer to
  // the internalField.
  if (internalMesh != nullptr)
  {
#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
    if (this->Parent->GetDecomposePolyhedra() && this->AdditionalCellPoints &&
      !this->AdditionalCellPoints->empty())
    {
      // The point-to-cell interpolation to additional cell centroidal points for decomposed cells
      const auto& addCellPoints = *this->AdditionalCellPoints;
      const vtkIdType nAddPoints = static_cast<vtkIdType>(addCellPoints.size());
      iData->Resize(this->NumPoints + nAddPoints);

      const bool cellPoints64Bit =
        (nAddPoints > 0 && ::Is64BitArray(this->AdditionalCellPoints->front()));

      double interpolatedValue[9];
      for (vtkIdType i = 0, newPointi = this->NumPoints; i < nAddPoints; ++i, ++newPointi)
      {
        vtkDataArray* acp = addCellPoints[i];
        const vtkIdType nPoints = acp->GetDataSize();

        for (int cmpt = 0; cmpt < nComponents; ++cmpt)
        {
          interpolatedValue[cmpt] = 0; // zero contents
        }
        for (vtkIdType meshPointi = 0; meshPointi < nPoints; ++meshPointi)
        {
          const float* tuple =
            iData->GetPointer(nComponents * GetLabelValue(acp, meshPointi, cellPoints64Bit));

          for (int cmpt = 0; cmpt < nComponents; ++cmpt)
          {
            interpolatedValue[cmpt] += tuple[cmpt];
          }
        }
        const double weight = 1.0 / static_cast<double>(nPoints);
        for (int cmpt = 0; cmpt < nComponents; ++cmpt)
        {
          interpolatedValue[cmpt] *= weight;
        }

        // Will automatically be converted to float
        iData->InsertTuple(newPointi, interpolatedValue);
      }
    }
#endif

    // Set data to internal mesh
    ::AddArrayToFieldData(internalMesh->GetPointData(), iData, io.GetObjectName(), dimString);
  }

  // Boundary
  // Use patch-internal values as boundary values

  unsigned int activeBoundaryIndex = 0;

  for (const vtkFoamPatch& patch : patches)
  {
    if (patches.isActive(patch.index_))
    {
      auto* bm = vtkPolyData::SafeDownCast(boundaryMesh->GetBlock(activeBoundaryIndex));
      vtkDataArray* bpMap = this->BoundaryPointMap->operator[](activeBoundaryIndex);
      ++activeBoundaryIndex;

      const vtkIdType nPoints = bpMap->GetNumberOfTuples();
      const bool meshPoints64Bit = ::Is64BitArray(bpMap);

      vtkSmartPointer<vtkFloatArray> vData;
      std::string bcType;

      if (bfieldEntries != nullptr)
      {
        const vtkFoamEntry* bfieldEntry = bfieldEntries->Dictionary().Lookup(patch.name_, true);
        if (bfieldEntry == nullptr)
        {
          // badEntry - but be silent about it
        }
        else if (bfieldEntry->FirstValue().GetType() != vtkFoamToken::DICTIONARY)
        {
          // badEntry - but be silent about it
        }
        else
        {
          const vtkFoamDict& patchDict = bfieldEntry->Dictionary();

          // Look for "value" entry
          vtkFoamEntry* vEntry = patchDict.Lookup("value");
          if (vEntry == nullptr)
          {
            // For alternative fallback
            const vtkFoamEntry* eptr = patchDict.Lookup("type");
            if (eptr != nullptr)
            {
              bcType = eptr->ToString();
            }
          }
          else
          {
            vData = this->FillField(*vEntry, nPoints, io, fieldDataType);
            // silent about bad entry
          }
        }
      }

      if (vData == nullptr)
      {
        // No "value" entry
        // Default to patch-internal values as boundary values
        vData = vtkSmartPointer<vtkFloatArray>::New();
        vData->SetNumberOfComponents(nComponents);
        vData->SetNumberOfTuples(nPoints);

        // Ad hoc handling of some known bcs without a "value"
        if (bcType == "noSlip")
        {
          vData->FillValue(0);
        }
        else
        {
          for (vtkIdType pointi = 0; pointi < nPoints; ++pointi)
          {
            vData->SetTuple(pointi, GetLabelValue(bpMap, pointi, meshPoints64Bit), iData);
          }
        }
      }

      ::AddArrayToFieldData(bm->GetPointData(), vData, io.GetObjectName(), dimString);
    }
  }

  // Handle any zones
  // ...
}

//------------------------------------------------------------------------------
// Read area field at a timestep
#if VTK_FOAMFILE_FINITE_AREA
void vtkOpenFOAMReaderPrivate::GetAreaFieldAtTimeStep(const std::string& varName)
{
  // Where to map data
  vtkPolyData* areaMesh = this->AreaMesh;

  // Really simple, skip if not selected, or no areaMesh
  const vtkIdType nFaces = (areaMesh ? areaMesh->GetNumberOfCells() : 0);

  if (!nFaces)
  {
    return;
  }

  vtkFoamIOobject io(this->CasePath, this->Parent);
  vtkFoamDict dict;
  if (!this->ReadFieldFile(io, dict, varName, this->Parent->CellDataArraySelection))
  {
    return;
  }

  if (io.GetClassName().compare(0, 4, "area") != 0)
  {
    vtkErrorMacro(<< io.GetFileName() << " is not a areaField");
    return;
  }

  // Eg, from "areaScalarField" -> SCALAR_TYPE
  const auto fieldDataType(vtkFoamTypes::FieldToEnum(io.GetClassName(), 4));

  // -------------------------
  // Handle dictionary lookups first

  // The "dimensions" entry - stringify
  const std::string dimString(this->ConstructDimensions(dict));

  // The "internalField" entry
  vtkFoamEntry* ifieldEntry = nullptr;
  {
    ifieldEntry = dict.Lookup("internalField");
    if (ifieldEntry == nullptr)
    {
      vtkErrorMacro(<< "internalField not found in " << io.GetFileName());
      return;
    }
    else if (ifieldEntry->FirstValue().GetType() == vtkFoamToken::EMPTYLIST)
    {
      if (this->NumFaces)
      {
        vtkErrorMacro(<< "internalField of " << io.GetFileName() << " is empty");
      }
      return;
    }
  }

  // Forget "boundaryField" entry
  // - don't really handle edge constraints,

  // -------------------------

  // Generate internal field data
  vtkSmartPointer<vtkFloatArray> iData = this->FillField(*ifieldEntry, nFaces, io, fieldDataType);
  if (iData == nullptr)
  {
    return;
  }
  else if (iData->GetSize() == 0)
  {
    // Determined that there are no faces. Ignore the field
    return;
  }

  // Set data to area mesh
  ::AddArrayToFieldData(areaMesh->GetCellData(), iData, io.GetObjectName(), dimString);
}
#endif

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkOpenFOAMReaderPrivate::MakeLagrangianMesh()
{
  // Can have several clouds coming from different regions.
  // Displayed in selection panel like this:
  // - lagrangian/someCloud
  // - /regionName/lagrangian/otherCloud
  //
  // Pick out active ones with matching region

  auto* lagrangianMesh = vtkMultiBlockDataSet::New();

  const std::string regionCloudPrefix(this->RegionPrefix() + "lagrangian/");

  vtkDataArraySelection* selection = this->Parent->PatchDataArraySelection;

  const vtkIdType nItems = selection->GetNumberOfArrays();
  for (vtkIdType itemi = 0; itemi < nItems; ++itemi)
  {
    if (!selection->GetArraySetting(itemi))
    {
      continue;
    }
    const std::string displayName(selection->GetArrayName(itemi));

    auto slash = displayName.rfind('/');
    if (slash == std::string::npos || displayName.compare(0, ++slash, regionCloudPrefix) != 0)
    {
      continue;
    }
    // The cloud name is the final component of the displayName
    const std::string cloudName(displayName.substr(slash));

    // Equivalent to CurrentTimeRegionPath() + "lagrangian/{cloudName}"
    std::string cloudPath(this->CurrentTimePath());
    if (displayName[0] != '/')
    {
      cloudPath += '/';
    }
    cloudPath += displayName;

    // Keep node/leaf structure consistent even if mesh doesn't exist
    vtkNew<vtkPolyData> cloudMesh;
    ::AppendBlock(lagrangianMesh, cloudMesh, cloudName);

    // Get the parcel positions as vtkPoints
    vtkNew<vtkPoints> points;
    {
      bool missingCloud = true;
      const std::string positionsPath(cloudPath + "/positions");

      vtkFoamIOobject io(this->CasePath, this->Parent);
      if (io.OpenOrGzip(positionsPath))
      {
        vtkFoamEntryValue dict(nullptr);

        try
        {
          if (io.IsFloat64())
          {
            dict.ReadNonUniformList<vtkFoamToken::VECTORLIST, //
              vtkFoamRead::vectorListTraits<vtkFloatArray, double, 3, true>>(io);
          }
          else
          {
            dict.ReadNonUniformList<vtkFoamToken::VECTORLIST, //
              vtkFoamRead::vectorListTraits<vtkFloatArray, float, 3, true>>(io);
          }

          // Transfer float tuples to points
          auto* pointArray = dict.ReleasePtr<vtkFloatArray>();

          points->SetData(pointArray);
          pointArray->Delete();
          missingCloud = false;
        }
        catch (const vtkFoamError& err)
        {
          vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                        << ": " << err);
        }
      }

      if (missingCloud)
      {
        vtkNew<vtkFloatArray> pointArray;
        pointArray->SetNumberOfComponents(3);
        points->SetData(pointArray);
      }
      const vtkIdType nParticles = points->GetNumberOfPoints();

      cloudMesh->SetPoints(points);

      // Cells (verts) for lagrangian mesh
      vtkNew<vtkCellArray> verts;
      verts->AllocateExact(nParticles, nParticles);
      for (vtkIdType i = 0; i < nParticles; ++i)
      {
        verts->InsertNextCell(1, &i);
      }
      cloudMesh->SetVerts(verts);
    }

    const vtkIdType nParticles = cloudMesh->GetPoints()->GetNumberOfPoints();

    // Can be empty or missing for a particular region or processor
    if (!nParticles)
    {
      continue;
    }

    // Read lagrangian fields
    const vtkIdType nFields = this->LagrangianFieldFiles->GetNumberOfValues();
    for (vtkIdType fieldi = 0; fieldi < nFields; ++fieldi)
    {
      const std::string varPath(cloudPath + "/" + this->LagrangianFieldFiles->GetValue(fieldi));

      vtkFoamIOobject io(this->CasePath, this->Parent);
      if (!io.OpenOrGzip(varPath))
      {
        continue; // Could be empty or missing for a particular region or processor
      }

      // If the variable is disabled on selection panel then skip it
      const std::string varDisplayName(io.GetObjectName());
      if (this->Parent->LagrangianDataArraySelection->ArrayExists(varDisplayName.c_str()) &&
        !this->Parent->GetLagrangianArrayStatus(varDisplayName.c_str()))
      {
        continue;
      }

      // Read the field file into dictionary
      vtkFoamEntryValue dict(nullptr);
      if (!dict.ReadField(io))
      {
        vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                      << ": " << io.GetError());
        continue;
      }

      // Set lagrangian values
      if (dict.GetType() != vtkFoamToken::SCALARLIST &&
        dict.GetType() != vtkFoamToken::VECTORLIST && dict.GetType() != vtkFoamToken::LABELLIST)
      {
        vtkErrorMacro(<< io.GetFileName() << ": Unsupported lagrangian field type "
                      << io.GetClassName());
        continue;
      }

      // Capture content as smart pointer
      auto fldData = vtkSmartPointer<vtkDataArray>::Take(dict.ReleasePtr<vtkDataArray>());

      const vtkIdType nValues = fldData->GetNumberOfTuples();
      if (nValues != nParticles)
      {
        vtkErrorMacro(<< io.GetFileName() << ": Size mismatch for lagrangian mesh (" << nParticles
                      << ") and field (" << nValues << ')');
        continue;
      }

      // Provide identical data as cell and as point data
      ::AddArrayToFieldData(cloudMesh->GetCellData(), fldData, varDisplayName);
      ::AddArrayToFieldData(cloudMesh->GetPointData(), fldData, varDisplayName);
    }
  }
  return lagrangianMesh;
}

//------------------------------------------------------------------------------
// Read specified file (typeName) from polyMesh directory as dictionary

std::unique_ptr<vtkFoamDict> vtkOpenFOAMReaderPrivate::GetPolyMeshFile(
  const std::string& typeName, bool mandatory)
{
  const std::string timeRegionDir(this->CurrentTimeRegionPath(this->PolyMeshTimeIndexFaces));

  vtkFoamIOobject io(this->CasePath, this->Parent);
  if (!io.OpenOrGzip(timeRegionDir + "/polyMesh/" + typeName))
  {
    if (mandatory)
    {
      vtkErrorMacro(<< "Error opening " << io.GetFileName() << ": " << io.GetError());
    }
    return nullptr;
  }

  std::unique_ptr<vtkFoamDict> dictPtr(new vtkFoamDict);
  vtkFoamDict& dict = *dictPtr;
  if (!dict.Read(io))
  {
    vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                  << ": " << io.GetError());
    return nullptr;
  }
  if (dict.GetType() != vtkFoamToken::DICTIONARY)
  {
    vtkErrorMacro(<< "The file type of " << io.GetFileName() << " is not a dictionary");
    return nullptr;
  }
  return dictPtr;
}

//------------------------------------------------------------------------------
// Populate cell zone(s) mesh

bool vtkOpenFOAMReaderPrivate::GetCellZoneMesh(vtkMultiBlockDataSet* zoneMesh,
  std::unique_ptr<vtkFoamLabelListList>& meshCellsPtr, const vtkFoamLabelListList& meshFaces,
  vtkPoints* points)
{
  const bool supportFields = this->Parent->CopyDataToCellZones;

  typedef vtkUnstructuredGrid zoneVtkType;
  constexpr const char* const zonePrefix = "cellZone";
  constexpr const char* const zoneFileName = "cellZones";
  constexpr const char* const labelsName = "cellLabels";
  const vtkIdType maxLabels = this->NumCells;
  auto& zoneMap = cellZoneMap;

  zoneMap.clearAll(); // Remove all old ids and errors

  auto zonesDictPtr(this->GetPolyMeshFile(zoneFileName, false));
  if (zonesDictPtr == nullptr)
  {
    // Not an error if mesh zones are missing
    return true;
  }

  const vtkFoamDict& zones = *zonesDictPtr;
  const bool use64BitLabels = zones.IsLabel64();

  vtkFoamError warnings;
  const unsigned nZones = static_cast<unsigned>(zones.size());

  // Detect duplicates
  std::unordered_map<std::string, unsigned> zoneNames;

  for (unsigned zonei = 0; zonei < nZones; ++zonei)
  {
    const std::string& zoneName = zones[zonei]->GetKeyword();
    const vtkFoamDict& dict = zones[zonei]->Dictionary();

    // Look up cellLabels
    vtkFoamEntry* eptr = dict.Lookup(labelsName);
    if (eptr == nullptr)
    {
      vtkErrorMacro(<< labelsName << " not found in " << zonePrefix);
      return false;
    }
    vtkFoamEntryValue& labelsEntry = eptr->FirstValue();

    // Need mesh, even if the list is empty
    vtkNew<zoneVtkType> zm;

    // Some OpenFOAM versions write an empty list as zero label only (in binary)
    if (labelsEntry.GetType() == vtkFoamToken::EMPTYLIST || labelsEntry.IsLabel(0))
    {
      // For empty list - store empty mesh (for proper block ordering)
      ::SetBlock(zoneMesh, zonei, zm, zoneName);
      continue;
    }
    else if (labelsEntry.GetType() != vtkFoamToken::LABELLIST)
    {
      vtkErrorMacro(<< labelsName << " is not a labelList");
      return false;
    }

    // Detect duplicate zone names (being really paranoid)
    {
      auto insertion = zoneNames.emplace(zoneName, zonei);
      if (!insertion.second)
      {
        vtkErrorMacro(<< zonePrefix << '/' << zoneName << " (zone:" << zonei
                      << ") is duplicate of zone:" << insertion.first->second);
        return false;
      }
    }

    vtkDataArray& labels = labelsEntry.LabelList();
    const vtkIdType nLabels = labels.GetNumberOfTuples();

    // Transcribe into vtkIdList and remove questionable entries
    auto elemIds = vtkSmartPointer<vtkIdList>::New();
    elemIds->SetNumberOfIds(nLabels);

    vtkIdType nUsed = 0;
    for (vtkIdType idx = 0; idx < nLabels; ++idx)
    {
      const vtkIdType elemId = GetLabelValue(&labels, idx, use64BitLabels);
      if (elemId >= 0 && elemId < maxLabels)
      {
        elemIds->SetId(nUsed, elemId);
        ++nUsed;
      }
    }
    if (nLabels != nUsed)
    {
      elemIds->Resize(nUsed);
      warnings << zonePrefix << '/' << zoneName << " had " << (nLabels - nUsed)
               << " out-of-range elements\n";
    }

    // Retain ids (in cache) for supporting fields
    if (supportFields)
    {
      zoneMap.zones_[zoneName] = elemIds;
    }

    // Allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointers if we return by error
    zm->Allocate(nUsed);

    // Insert cells
    this->InsertCellsToGrid(zm, meshCellsPtr, meshFaces, elemIds);

    // Set points for zone
    zm->SetPoints(points);

    ::SetBlock(zoneMesh, zonei, zm, zoneName);
  }

  if (!warnings.empty())
  {
    vtkWarningMacro(<< warnings);
  }

  return true;
}

//------------------------------------------------------------------------------
// Populate face zone(s) mesh

bool vtkOpenFOAMReaderPrivate::GetFaceZoneMesh(
  vtkMultiBlockDataSet* zoneMesh, const vtkFoamLabelListList& meshFaces, vtkPoints* points)
{
  const bool supportFields = this->Parent->CopyDataToCellZones;

  typedef vtkPolyData zoneVtkType;
  constexpr const char* const zonePrefix = "faceZone";
  constexpr const char* const zoneFileName = "faceZones";
  constexpr const char* const labelsName = "faceLabels";
  const vtkIdType maxLabels = this->FaceOwner->GetNumberOfTuples(); // NumFaces
  auto& zoneMap = faceZoneMap;

  zoneMap.clearAll(); // Remove all old ids and errors

  auto zonesDictPtr(this->GetPolyMeshFile(zoneFileName, false));
  if (zonesDictPtr == nullptr)
  {
    // Not an error if mesh zones are missing
    return true;
  }

  const vtkFoamDict& zones = *zonesDictPtr;
  const bool use64BitLabels = zones.IsLabel64();

  vtkFoamError warnings;
  const unsigned nZones = static_cast<unsigned>(zones.size());

  // Detect duplicates
  std::unordered_map<std::string, unsigned> zoneNames;

  // Additional bookkeeping for faceZones
  const auto& patches = this->BoundaryDict;

  // Ignore neighbour side of processor patches
  const bool ignoreProcNeighbour = !this->ProcessorName.empty();
  const vtkIdType nInternalFaces = this->NumInternalFaces;

  for (unsigned zonei = 0; zonei < nZones; ++zonei)
  {
    const std::string& zoneName = zones[zonei]->GetKeyword();
    const vtkFoamDict& dict = zones[zonei]->Dictionary();

    // Look up faceLabels
    vtkFoamEntry* eptr = dict.Lookup(labelsName);
    if (eptr == nullptr)
    {
      vtkErrorMacro(<< labelsName << " not found in " << zonePrefix);
      return false;
    }
    vtkFoamEntryValue& labelsEntry = eptr->FirstValue();

    // Need mesh, even if the list is empty
    vtkNew<zoneVtkType> zm;

    // Some OpenFOAM versions write an empty list as zero label only (in binary)
    if (labelsEntry.GetType() == vtkFoamToken::EMPTYLIST || labelsEntry.IsLabel(0))
    {
      // For empty list - store empty mesh (for proper block ordering)
      ::SetBlock(zoneMesh, zonei, zm, zoneName);
      continue;
    }
    else if (labelsEntry.GetType() != vtkFoamToken::LABELLIST)
    {
      vtkErrorMacro(<< labelsName << " is not a labelList");
      return false;
    }

    // Detect duplicate zone names (being really paranoid)
    {
      auto insertion = zoneNames.emplace(zoneName, zonei);
      if (!insertion.second)
      {
        vtkErrorMacro(<< zonePrefix << '/' << zoneName << " (zone:" << zonei
                      << ") is duplicate of zone:" << insertion.first->second);
        return false;
      }
    }

    vtkDataArray& labels = labelsEntry.LabelList();
    const vtkIdType nLabels = labels.GetNumberOfTuples();

    // Transcribe into vtkIdList and remove questionable entries
    auto elemIds = vtkSmartPointer<vtkIdList>::New();
    elemIds->SetNumberOfIds(nLabels);

    vtkIdType nUsed = 0, nNonOwner = 0;
    for (vtkIdType idx = 0; idx < nLabels; ++idx)
    {
      const vtkIdType elemId = GetLabelValue(&labels, idx, use64BitLabels);
      if (elemId >= 0 && elemId < maxLabels)
      {
        const vtkIdType patchId =
          ((ignoreProcNeighbour && elemId >= nInternalFaces) ? patches.whichPatch(elemId) : -1);

        // NB: the test for patch owner is always true for non-processor patches
        if (patchId >= 0 && !patches[patchId].owner_)
        {
          ++nNonOwner;
        }
        else
        {
          elemIds->SetId(nUsed, elemId);
          ++nUsed;
        }
      }
    }
    if (nLabels != nUsed)
    {
      elemIds->Resize(nUsed);
      if (nLabels != (nUsed + nNonOwner))
      {
        warnings << zonePrefix << '/' << zoneName << " had " << (nLabels - (nUsed + nNonOwner))
                 << " out-of-range elements\n";
      }
    }

    // Retain ids (in cache) for supporting fields
    if (supportFields)
    {
      zoneMap.zones_[zoneName] = elemIds;
    }

    // Allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    zm->AllocateEstimate(nUsed, 1);

    // Insert faces
    this->InsertFacesToGrid(zm, meshFaces, 0, nUsed, elemIds);

    // Set points for zone
    zm->SetPoints(points);

    ::SetBlock(zoneMesh, zonei, zm, zoneName);
  }

  if (!warnings.empty())
  {
    vtkWarningMacro(<< warnings);
  }

  return true;
}

//------------------------------------------------------------------------------
// Populate point zone(s) mesh

bool vtkOpenFOAMReaderPrivate::GetPointZoneMesh(vtkMultiBlockDataSet* zoneMesh, vtkPoints* points)
{
  const bool supportFields = false; // this->Parent->CopyDataToCellZones;

  typedef vtkPolyData zoneVtkType;
  constexpr const char* const zonePrefix = "pointZone";
  constexpr const char* const zoneFileName = "pointZones";
  constexpr const char* const labelsName = "pointLabels";
  const vtkIdType maxLabels = this->NumPoints;
  auto& zoneMap = pointZoneMap;

  zoneMap.clearAll(); // Remove all old ids and errors

  auto zonesDictPtr(this->GetPolyMeshFile(zoneFileName, false));
  if (zonesDictPtr == nullptr)
  {
    // Not an error if mesh zones are missing
    return true;
  }

  const vtkFoamDict& zones = *zonesDictPtr;
  const bool use64BitLabels = zones.IsLabel64();

  vtkFoamError warnings;
  const unsigned nZones = static_cast<unsigned>(zones.size());

  // Detect duplicates
  std::unordered_map<std::string, unsigned> zoneNames;

  for (unsigned zonei = 0; zonei < nZones; ++zonei)
  {
    const std::string& zoneName = zones[zonei]->GetKeyword();
    const vtkFoamDict& dict = zones[zonei]->Dictionary();

    // Look up pointLabels
    vtkFoamEntry* eptr = dict.Lookup(labelsName);
    if (eptr == nullptr)
    {
      vtkErrorMacro(<< labelsName << " not found in " << zonePrefix);
      return false;
    }
    vtkFoamEntryValue& labelsEntry = eptr->FirstValue();

    // Need mesh, even if the list is empty
    vtkNew<zoneVtkType> zm;

    // Some OpenFOAM versions write an empty list as zero label only (in binary)
    if (labelsEntry.GetType() == vtkFoamToken::EMPTYLIST || labelsEntry.IsLabel(0))
    {
      // For empty list - store empty mesh (for proper block ordering)
      ::SetBlock(zoneMesh, zonei, zm, zoneName);
      continue;
    }
    else if (labelsEntry.GetType() != vtkFoamToken::LABELLIST)
    {
      vtkErrorMacro(<< labelsName << " is not a labelList");
      return false;
    }

    // Detect duplicate zone names (being really paranoid)
    {
      auto insertion = zoneNames.emplace(zoneName, zonei);
      if (!insertion.second)
      {
        vtkErrorMacro(<< zonePrefix << '/' << zoneName << " (zone:" << zonei
                      << ") is duplicate of zone:" << insertion.first->second);
        return false;
      }
    }

    vtkDataArray& labels = labelsEntry.LabelList();
    const vtkIdType nLabels = labels.GetNumberOfTuples();

    // Transcribe into vtkIdList and remove questionable entries
    // Not completely necessary since the VTK_VERTEX has the value anyhow, but do it
    // to filter out potentially bad entries and be more similar to cell/face zones.
    // - besides which, not many point sets being used anyhow.

    auto elemIds = vtkSmartPointer<vtkIdList>::New();
    elemIds->SetNumberOfIds(nLabels);

    vtkIdType nUsed = 0;
    for (vtkIdType idx = 0; idx < nLabels; ++idx)
    {
      const vtkIdType elemId = GetLabelValue(&labels, idx, use64BitLabels);
      if (elemId >= 0 && elemId < maxLabels)
      {
        elemIds->SetId(nUsed, elemId);
        ++nUsed;
      }
    }
    if (nLabels != nUsed)
    {
      elemIds->Resize(nUsed);
      warnings << zonePrefix << '/' << zoneName << " had " << (nLabels - nUsed)
               << " out-of-range elements\n";
    }

    // Retain ids (in cache) for supporting fields
    if (supportFields)
    {
      zoneMap.zones_[zoneName] = elemIds;
    }

    // Allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    zm->AllocateEstimate(nUsed, 1);

    // Insert vertex cells
    for (vtkIdType pointi = 0; pointi < nUsed; ++pointi)
    {
      const vtkIdType elemId = elemIds->GetId(pointi);
      zm->InsertNextCell(VTK_VERTEX, 1, &elemId);
    }

    // Set points for zone
    zm->SetPoints(points);

    ::SetBlock(zoneMesh, zonei, zm, zoneName);
  }

  if (!warnings.empty())
  {
    vtkWarningMacro(<< warnings);
  }

  return true;
}

//------------------------------------------------------------------------------
// Populate face zone(s) mesh

#if VTK_FOAMFILE_FINITE_AREA
bool vtkOpenFOAMReaderPrivate::GetAreaMesh(
  vtkPolyData* areaMesh, const vtkFoamLabelListList& meshFaces, vtkPoints* points)
{
  // Tie to faces instance (like zones etc)
  const std::string timeRegionDir(this->CurrentTimeRegionPath(*this->PolyMeshTimeIndexFaces));

  auto& zoneMap = this->areaMeshMap;
  zoneMap.clearAll(); // Remove all old ids and errors

  vtkFoamIOobject io(this->CasePath, this->Parent);

  // Read faMesh/faceLabels
  if (!io.OpenOrGzip(timeRegionDir + "/faMesh/faceLabels"))
  {
    // Not an error if missing
    return true;
  }
  const bool use64BitLabels = io.IsLabel64();

  vtkSmartPointer<vtkDataArray> labelArray;

  {
    vtkFoamEntryValue dict(nullptr);
    dict.SetStreamOption(io);
    try
    {
      if (use64BitLabels)
      {
        dict.ReadNonUniformList<vtkFoamToken::LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt64Array, vtkTypeInt64>>(io);
      }
      else
      {
        dict.ReadNonUniformList<vtkFoamToken::LABELLIST, //
          vtkFoamRead::listTraits<vtkTypeInt32Array, vtkTypeInt32>>(io);
      }

      // Capture content as smart pointer
      labelArray.TakeReference(dict.ReleasePtr<vtkDataArray>());
    }
    catch (const vtkFoamError& err)
    {
      vtkErrorMacro(<< "Error reading line " << io.GetLineNumber() << " of " << io.GetFileName()
                    << ": " << err);
      return false;
    }
    io.Close();
  }

  if (labelArray)
  {
    vtkDataArray& labels = *labelArray;
    const vtkIdType nLabels = labels.GetNumberOfTuples();

    // Transcribe into vtkIdList. Don't check for questionable entries just yet
    auto elemIds = vtkSmartPointer<vtkIdList>::New();
    elemIds->SetNumberOfIds(nLabels);

    for (vtkIdType idx = 0; idx < nLabels; ++idx)
    {
      const vtkTypeInt64 elemId = GetLabelValue(labelArray, idx, use64BitLabels);
      elemIds->SetId(idx, elemId);
    }

    zoneMap.zones_[NAME_AREAMESH] = elemIds;

    // Allocate new grid: we do not use resize() beforehand since it
    // could lead to undefined pointer if we return by error
    areaMesh->AllocateEstimate(nLabels, 1);

    // Insert faces
    this->InsertFacesToGrid(areaMesh, meshFaces, 0, nLabels, elemIds);

    // Set points for zone
    areaMesh->SetPoints(points);

    return true;
  }
  return false;
}
#endif

//------------------------------------------------------------------------------
// return 0 if there's any error, 1 if success
int vtkOpenFOAMReaderPrivate::RequestData(vtkMultiBlockDataSet* output)
{
  if (!this->HasPolyMesh())
  {
    // Ignore a region without a mesh, but will normally be precluded earlier
    vtkWarningMacro("Called RequestData without a mesh.");
    return 1;
  }

  //----------------------------------------
  // Determine changes in state

  // Basics
  const bool changedStorageType =
    (this->Parent->Use64BitLabels != this->Parent->Use64BitLabelsOld) ||
    (this->Parent->Use64BitFloats != this->Parent->Use64BitFloatsOld);

  // Mesh changes
  const bool topoChanged = (this->TimeStepOld < 0) || (this->FaceOwner == nullptr) ||
    (this->PolyMeshTimeIndexFaces[this->TimeStep] !=
      this->PolyMeshTimeIndexFaces[this->TimeStepOld]);

  const bool pointsMoved = (this->TimeStepOld < 0) ||
    (this->PolyMeshTimeIndexPoints[this->TimeStep] !=
      this->PolyMeshTimeIndexPoints[this->TimeStepOld]);

  // Internal mesh
  bool recreateInternalMesh = (changedStorageType) || (topoChanged) || (!this->Parent->CacheMesh) ||
    (this->Parent->SkipZeroTime != this->Parent->SkipZeroTimeOld) ||
    (this->Parent->ListTimeStepsByControlDict != this->Parent->ListTimeStepsByControlDictOld);

  // Internal mesh - selection changes
  recreateInternalMesh |=
    (this->InternalMeshSelectionStatus != this->InternalMeshSelectionStatusOld);

#if VTK_FOAMFILE_DECOMPOSE_POLYHEDRA
  if (this->InternalMeshSelectionStatus)
  {
    // Cell representation changed that affects the internalMesh
    recreateInternalMesh |=
      (this->Parent->DecomposePolyhedra != this->Parent->DecomposePolyhedraOld);
  }
#endif

  // NOTE: this is still not quite right for zones, but until we get better separation
  // - can remove zones without triggering reread
  recreateInternalMesh |=
    (this->Parent->ReadZones && (this->Parent->ReadZones != this->Parent->ReadZonesOld));

  // Boundary mesh
  bool recreateBoundaryMesh = (changedStorageType) ||
    (this->Parent->PatchDataArraySelection->GetMTime() != this->Parent->PatchSelectionMTimeOld) ||
    (this->Parent->CreateCellToPoint != this->Parent->CreateCellToPointOld);

  // Fields
  bool updateVariables = (changedStorageType) || (this->TimeStep != this->TimeStepOld) ||
    (this->Parent->CellDataArraySelection->GetMTime() != this->Parent->CellSelectionMTimeOld) ||
    (this->Parent->PointDataArraySelection->GetMTime() != this->Parent->PointSelectionMTimeOld) ||
    (this->Parent->LagrangianDataArraySelection->GetMTime() !=
      this->Parent->LagrangianSelectionMTimeOld) ||
    (this->Parent->PositionsIsIn13Format != this->Parent->PositionsIsIn13FormatOld) ||
    (this->Parent->AddDimensionsToArrayNames != this->Parent->AddDimensionsToArrayNamesOld);

  // Apply these changes too
  recreateBoundaryMesh |= recreateInternalMesh;
  updateVariables |= recreateBoundaryMesh;

  const bool moveInternalPoints = !recreateInternalMesh && pointsMoved;
  const bool moveBoundaryPoints = !recreateBoundaryMesh && pointsMoved;

  // Has eulerian fields if there is an internal mesh
  const bool createEulerians = this->Parent->PatchDataArraySelection->ArrayExists(
    (this->RegionPrefix() + NAME_INTERNALMESH).c_str());

  vtkFoamDebug(<< "RequestData (" << this->RegionName << "/" << this->ProcessorName << ")\n"
               << " internal=" << recreateInternalMesh      //
               << " boundary=" << recreateBoundaryMesh      //
               << " zones=" << this->Parent->GetReadZones() //
               << " topoChanged=" << topoChanged            //
               << " pointsMoved=" << pointsMoved            //
               << " variables=" << updateVariables          //
               << " eulerians=" << createEulerians          //
               << "\n");

  //----------------------------------------

  // Determine if we need to reconstruct meshes
  if (recreateInternalMesh)
  {
    this->ClearInternalMeshes();
    this->ClearZoneMeshes();
  }
  if (recreateBoundaryMesh)
  {
    this->ClearBoundaryMeshes();
  }
  // Discard unwanted remnant zones
  if (!this->Parent->GetReadZones())
  {
    this->ClearZoneMeshes();
  }

  // Mesh primitives
  vtkSmartPointer<vtkFloatArray> pointArray;
  std::unique_ptr<vtkFoamLabelListList> meshCells;
  std::unique_ptr<vtkFoamLabelListList> meshFaces;

  if (createEulerians && (recreateInternalMesh || recreateBoundaryMesh))
  {
    const std::string facesInstance = this->CurrentTimeRegionPath(this->PolyMeshTimeIndexFaces);

    vtkFoamDebug(<< "Read faces: " << facesInstance << "\n");
    // Read polyMesh/faces, create the list of faces, set the number of faces
    meshFaces = this->ReadFacesFile(facesInstance);
    if (!meshFaces)
    {
      return 0;
    }
    this->Parent->UpdateProgress(0.2);
  }

  if (createEulerians && recreateInternalMesh)
  {
    const std::string facesInstance = this->CurrentTimeRegionPath(this->PolyMeshTimeIndexFaces);

    vtkFoamDebug(<< "Read owner/neighbour: " << facesInstance << "\n");

    // Read polyMesh/{owner,neighbour}, create FaceOwner/FaceNeigh
    if (!this->ReadOwnerNeighbourFiles(facesInstance))
    {
      return 0;
    }
    this->Parent->UpdateProgress(0.3);
  }

  if (createEulerians &&
    (recreateInternalMesh ||
      (recreateBoundaryMesh && !recreateInternalMesh && this->InternalMesh == nullptr) ||
      moveInternalPoints || moveBoundaryPoints))
  {
    const std::string pointsInstance = this->CurrentTimeRegionPath(this->PolyMeshTimeIndexPoints);

    vtkFoamDebug(<< "Read points: " << pointsInstance << "\n");

    // Read polyMesh/points, set the number of faces
    pointArray = this->ReadPointsFile(pointsInstance);
    if ((recreateInternalMesh && pointArray.Get() == nullptr) ||
      (meshFaces && !this->CheckFaceList(*meshFaces)))
    {
      return 0;
    }
    this->Parent->UpdateProgress(0.4);
  }

  // Create internal mesh, only if required for display
  if (createEulerians && recreateInternalMesh)
  {
    const std::string displayName(this->RegionPrefix() + NAME_INTERNALMESH);
    if (this->Parent->PatchDataArraySelection->ArrayExists(displayName.c_str()) &&
      this->Parent->GetPatchArrayStatus(displayName.c_str()))
    {
      this->InternalMesh = this->MakeInternalMesh(meshCells, *meshFaces, pointArray);
    }
  }

  // Read and construct zones
  if (createEulerians && recreateInternalMesh && this->Parent->GetReadZones())
  {
    vtkSmartPointer<vtkPoints> tmpPoints; // Localized vtkPoints storage
    vtkPoints* points;

    if (this->InternalMesh != nullptr)
    {
      points = this->InternalMesh->GetPoints();
    }
    else
    {
      tmpPoints = vtkSmartPointer<vtkPoints>::New();
      tmpPoints->SetData(pointArray);
      points = tmpPoints;
    }

    this->CellZoneMesh = vtkMultiBlockDataSet::New();
    if (!this->GetCellZoneMesh(this->CellZoneMesh, meshCells, *meshFaces, points) ||
      this->CellZoneMesh->GetNumberOfBlocks() == 0)
    {
      this->cellZoneMap.clearAll();
      this->CellZoneMesh->Delete();
      this->CellZoneMesh = nullptr;
    }

    this->FaceZoneMesh = vtkMultiBlockDataSet::New();
    if (!this->GetFaceZoneMesh(this->FaceZoneMesh, *meshFaces, points) ||
      this->FaceZoneMesh->GetNumberOfBlocks() == 0)
    {
      this->faceZoneMap.clearAll();
      this->FaceZoneMesh->Delete();
      this->FaceZoneMesh = nullptr;
    }

    this->PointZoneMesh = vtkMultiBlockDataSet::New();
    if (!this->GetPointZoneMesh(this->PointZoneMesh, points) ||
      this->PointZoneMesh->GetNumberOfBlocks() == 0)
    {
      this->pointZoneMap.clearAll();
      this->PointZoneMesh->Delete();
      this->PointZoneMesh = nullptr;
    }

#if VTK_FOAMFILE_FINITE_AREA
    // Needs a proper selection mechanism
    this->AreaMesh = vtkPolyData::New();
    if (!this->GetAreaMesh(this->AreaMesh, *meshFaces, points))
    {
      this->areaMeshMap.clearAll();
      this->AreaMesh->Delete();
      this->AreaMesh = nullptr;
    }
#endif
  }

  // Don't need meshCells beyond here
  meshCells.reset(nullptr);

  // Note: preserve face owner/neighbour information for reconstruction, face zones etc.

  // Create boundary mesh
  if (createEulerians && recreateBoundaryMesh)
  {
    vtkFloatArray* boundaryPointArray = pointArray.Get();
    if (boundaryPointArray == nullptr)
    {
      boundaryPointArray = vtkFloatArray::SafeDownCast(this->InternalMesh->GetPoints()->GetData());
    }

    this->BoundaryMesh = this->MakeBoundaryMesh(*meshFaces, boundaryPointArray);
    if (this->BoundaryMesh == nullptr)
    {
      return 0;
    }
  }

  // Don't need meshFaces beyond here
  meshFaces.reset(nullptr);

  // Update the points in each mesh, if the point coordinates changed

  // Update internal mesh first - decomposed polyhedra will modify pointArray
  if (createEulerians && moveInternalPoints)
  {
    if (this->InternalMesh != nullptr)
    {
      vtkFoamDebug("Move internal points\n");
      if (!this->MoveInternalMesh(this->InternalMesh, pointArray))
      {
        return 0; // Failed!
      }
    }
  }

  // Update zones
  if (createEulerians && moveInternalPoints)
  {
    vtkSmartPointer<vtkPoints> tmpPoints; // Localized vtkPoints storage
    vtkPoints* points;

    if (this->InternalMesh != nullptr)
    {
      points = this->InternalMesh->GetPoints();
    }
    else
    {
      tmpPoints = vtkSmartPointer<vtkPoints>::New();
      tmpPoints->SetData(pointArray);
      points = tmpPoints;
    }

    if (this->PointZoneMesh != nullptr)
    {
      auto* zoneMesh = this->PointZoneMesh;
      for (unsigned int zonei = 0; zonei < zoneMesh->GetNumberOfBlocks(); ++zonei)
      {
        vtkPolyData::SafeDownCast(zoneMesh->GetBlock(zonei))->SetPoints(points);
      }
    }
    if (this->FaceZoneMesh != nullptr)
    {
      auto* zoneMesh = this->FaceZoneMesh;
      for (unsigned int zonei = 0; zonei < zoneMesh->GetNumberOfBlocks(); ++zonei)
      {
        vtkPolyData::SafeDownCast(zoneMesh->GetBlock(zonei))->SetPoints(points);
      }
    }
    if (this->CellZoneMesh != nullptr)
    {
      auto* zoneMesh = this->CellZoneMesh;
      for (unsigned int zonei = 0; zonei < zoneMesh->GetNumberOfBlocks(); ++zonei)
      {
        vtkUnstructuredGrid::SafeDownCast(zoneMesh->GetBlock(zonei))->SetPoints(points);
      }
    }
  }

  // Update boundary mesh
  if (createEulerians && moveBoundaryPoints)
  {
    if (this->BoundaryMesh != nullptr)
    {
      vtkFoamDebug("Move boundary points\n");
      this->MoveBoundaryMesh(this->BoundaryMesh, pointArray);
    }
  }

  // Don't need mesh points beyond here...
  // Be explicit: pointArray = vtkSmartPointer<vtkFloatArray>{};

  this->Parent->UpdateProgress(0.5);

  // Eulerian variables
  if (updateVariables && createEulerians)
  {
    // Clean up arrays of the previous timestep

    // Internal
    if (!recreateInternalMesh && this->InternalMesh != nullptr)
    {
      this->InternalMesh->GetCellData()->Initialize();
      this->InternalMesh->GetPointData()->Initialize();
    }

    // Boundary
    if (!recreateBoundaryMesh && this->BoundaryMesh != nullptr)
    {
      for (unsigned int i = 0; i < this->BoundaryMesh->GetNumberOfBlocks(); i++)
      {
        auto* bm = vtkPolyData::SafeDownCast(this->BoundaryMesh->GetBlock(i));
        bm->GetCellData()->Initialize();
        bm->GetPointData()->Initialize();
      }
    }

    // Zones. This may need some reworking...
    if (!recreateInternalMesh)
    {
      if (this->CellZoneMesh != nullptr)
      {
        auto* zoneMesh = this->CellZoneMesh;
        for (unsigned int zonei = 0; zonei < zoneMesh->GetNumberOfBlocks(); ++zonei)
        {
          auto* zm = vtkUnstructuredGrid::SafeDownCast(zoneMesh->GetBlock(zonei));
          zm->GetCellData()->Initialize();
          zm->GetPointData()->Initialize();
        }
      }
      if (this->FaceZoneMesh != nullptr)
      {
        auto* zoneMesh = this->FaceZoneMesh;
        for (unsigned int zonei = 0; zonei < zoneMesh->GetNumberOfBlocks(); ++zonei)
        {
          auto* zm = vtkPolyData::SafeDownCast(zoneMesh->GetBlock(zonei));
          zm->GetCellData()->Initialize();
          zm->GetPointData()->Initialize();
        }
      }
      if (this->PointZoneMesh != nullptr)
      {
        auto* zoneMesh = this->PointZoneMesh;
        for (unsigned int zonei = 0; zonei < zoneMesh->GetNumberOfBlocks(); ++zonei)
        {
          auto* zm = vtkPolyData::SafeDownCast(zoneMesh->GetBlock(zonei));
          zm->GetCellData()->Initialize();
          zm->GetPointData()->Initialize();
        }
      }
    }

    // read field data variables into Internal/Boundary meshes
    vtkIdType nFieldsRead = 0;
    vtkIdType nFieldsToRead = (this->VolFieldFiles->GetNumberOfValues() +
      this->DimFieldFiles->GetNumberOfValues() + this->PointFieldFiles->GetNumberOfValues());

#if VTK_FOAMFILE_FINITE_AREA
    nFieldsToRead += this->AreaFieldFiles->GetNumberOfValues();
#endif

    for (vtkIdType i = 0; i < this->VolFieldFiles->GetNumberOfValues(); ++i)
    {
      this->GetVolFieldAtTimeStep(this->VolFieldFiles->GetValue(i));
      this->Parent->UpdateProgress(0.5 + (0.5 * ++nFieldsRead) / nFieldsToRead);
    }
    for (vtkIdType i = 0; i < this->DimFieldFiles->GetNumberOfValues(); ++i)
    {
      this->GetVolFieldAtTimeStep(this->DimFieldFiles->GetValue(i), true); // Internal field
      this->Parent->UpdateProgress(0.5 + (0.5 * ++nFieldsRead) / nFieldsToRead);
    }
    for (vtkIdType i = 0; i < this->PointFieldFiles->GetNumberOfValues(); ++i)
    {
      this->GetPointFieldAtTimeStep(this->PointFieldFiles->GetValue(i));
      this->Parent->UpdateProgress(0.5 + (0.5 * ++nFieldsRead) / nFieldsToRead);
    }
#if VTK_FOAMFILE_FINITE_AREA
    for (vtkIdType i = 0; i < this->AreaFieldFiles->GetNumberOfValues(); ++i)
    {
      this->GetAreaFieldAtTimeStep(this->AreaFieldFiles->GetValue(i));
      this->Parent->UpdateProgress(0.5 + (0.5 * ++nFieldsRead) / nFieldsToRead);
    }
#endif
  }

  // Read lagrangian mesh and fields
  vtkMultiBlockDataSet* lagrangianMesh = nullptr;
  if (updateVariables)
  {
    lagrangianMesh = this->MakeLagrangianMesh();
  }

  // Add Internal mesh to final output only if selected for display
  if (this->InternalMesh != nullptr)
  {
    ::SetBlock(output, 0, this->InternalMesh, NAME_INTERNALMESH);
  }

  // Set boundary meshes/data as output
  if (this->BoundaryMesh != nullptr && this->BoundaryMesh->GetNumberOfBlocks())
  {
    ::AppendBlock(output, this->BoundaryMesh, "boundary");
  }

#if VTK_FOAMFILE_FINITE_AREA
  // Set finiteArea mesh/data as output
  if (this->AreaMesh != nullptr)
  {
    ::AppendBlock(output, this->AreaMesh, NAME_AREAMESH);
  }
#endif

  // Set lagrangian mesh as output
  if (lagrangianMesh != nullptr)
  {
    if (lagrangianMesh->GetNumberOfBlocks())
    {
      ::AppendBlock(output, lagrangianMesh, "lagrangian");
    }
    lagrangianMesh->Delete();
  }

  if (this->Parent->GetReadZones())
  {
    // Add zone meshes (if any) to output
    vtkNew<vtkMultiBlockDataSet> zones;

    if (this->CellZoneMesh != nullptr)
    {
      ::AppendBlock(zones, this->CellZoneMesh, "cellZones");
    }
    if (this->FaceZoneMesh != nullptr)
    {
      ::AppendBlock(zones, this->FaceZoneMesh, "faceZones");
    }
    if (this->PointZoneMesh != nullptr)
    {
      ::AppendBlock(zones, this->PointZoneMesh, "pointZones");
    }
    if (zones->GetNumberOfBlocks())
    {
      ::AppendBlock(output, zones, "zones");
    }
  }

  if (this->Parent->GetCacheMesh())
  {
    this->TimeStepOld = this->TimeStep;
  }
  else
  {
    this->ClearMeshes();
    this->TimeStepOld = TIMEINDEX_UNVISITED;
  }
  this->InternalMeshSelectionStatusOld = this->InternalMeshSelectionStatus;

  this->Parent->UpdateProgress(1.0);
  return 1;
}

//------------------------------------------------------------------------------
// constructor
vtkOpenFOAMReader::vtkOpenFOAMReader()
{
  this->SetNumberOfInputPorts(0);

  this->Parent = this;
  // must be false to avoid reloading by vtkAppendCompositeDataLeaves::Update()
  this->Refresh = false;

  // initialize file name
  this->FileName = nullptr;
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

  // For creating cell-to-point translated data
  this->CreateCellToPoint = 1;
  this->CreateCellToPointOld = 1;

  // For caching mesh
  this->CacheMesh = 1;

  // For decomposing polyhedra
  this->DecomposePolyhedra = 0;
  this->DecomposePolyhedraOld = 0;

  // for lagrangian/positions format without the additional data that existed
  // in OpenFOAM 1.4-2.4
  this->PositionsIsIn13Format = 1;
  this->PositionsIsIn13FormatOld = 1;

  // Zone handling
  this->ReadZones = 0; // turned off by default
  this->ReadZonesOld = 0;
  this->CopyDataToCellZones = true;

  // Ignore 0/ time directory, which is normally missing Lagrangian fields
  this->SkipZeroTime = false;
  this->SkipZeroTimeOld = false;

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
  this->Use64BitLabels = false;
  this->Use64BitFloats = true;
  this->Use64BitLabelsOld = false;
  this->Use64BitFloatsOld = true;
}

//------------------------------------------------------------------------------
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

  this->SetFileName(nullptr);
  delete this->FileNameOld;
}

//------------------------------------------------------------------------------
// CanReadFile
int vtkOpenFOAMReader::CanReadFile(const char* vtkNotUsed(fileName))
{
  return 1; // so far CanReadFile does nothing.
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::SetUse64BitLabels(bool val)
{
  if (this->Use64BitLabels != val)
  {
    this->Use64BitLabels = val;
    this->Refresh = true; // Need to reread everything
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::SetUse64BitFloats(bool val)
{
  if (this->Use64BitFloats != val)
  {
    this->Use64BitFloats = val;
    this->Refresh = true; // Need to reread everything
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Printing

void vtkOpenFOAMReader::PrintTimes(ostream& os, vtkIndent indent, bool full) const
{
  os << indent << "TimeInformation (SkipZeroTime: " << this->SkipZeroTime << ")\n";
  this->Readers->InitTraversal();
  for (vtkObject* obj; (obj = this->Readers->GetNextItemAsObject()) != nullptr;)
  {
    // Is private implementation
    {
      auto* reader = vtkOpenFOAMReaderPrivate::SafeDownCast(obj);
      if (reader)
      {
        reader->PrintTimes(os, indent.GetNextIndent(), full);
        continue;
      }
    }
    // Is sub-reader for derived type
    {
      auto* reader = vtkOpenFOAMReader::SafeDownCast(obj);
      if (reader)
      {
        reader->PrintTimes(os, indent.GetNextIndent(), full);
        continue;
      }
    }
  }
}

void vtkOpenFOAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Refresh: " << this->Refresh << endl;
  os << indent << "CreateCellToPoint: " << this->CreateCellToPoint << endl;
  os << indent << "CacheMesh: " << this->CacheMesh << endl;
  os << indent << "DecomposePolyhedra: " << this->DecomposePolyhedra << endl;
  os << indent << "PositionsIsIn13Format: " << this->PositionsIsIn13Format << endl;
  os << indent << "ReadZones: " << this->ReadZones << endl;
  os << indent << "AddDimensionsToArrayNames: " << this->AddDimensionsToArrayNames << endl;

  this->PrintTimes(os, indent);

  // PrintSelf for any type of sub-readers
  this->Readers->InitTraversal();
  for (vtkObject* obj; (obj = this->Readers->GetNextItemAsObject()) != nullptr;)
  {
    os << indent << "Reader instance " << static_cast<void*>(obj) << ": \n";
    obj->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
// selection list handlers

int vtkOpenFOAMReader::GetNumberOfSelectionArrays(vtkDataArraySelection* s)
{
  return s->GetNumberOfArrays();
}

int vtkOpenFOAMReader::GetSelectionArrayStatus(vtkDataArraySelection* s, const char* name)
{
  return s->ArrayIsEnabled(name);
}

void vtkOpenFOAMReader::SetSelectionArrayStatus(
  vtkDataArraySelection* s, const char* name, int status)
{
  vtkMTimeType mTime = s->GetMTime();
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

const char* vtkOpenFOAMReader::GetSelectionArrayName(vtkDataArraySelection* s, int index)
{
  return s->GetArrayName(index);
}

void vtkOpenFOAMReader::DisableAllSelectionArrays(vtkDataArraySelection* s)
{
  vtkMTimeType mTime = s->GetMTime();
  s->DisableAllArrays();
  if (mTime != s->GetMTime())
  {
    this->Modified();
  }
}

void vtkOpenFOAMReader::EnableAllSelectionArrays(vtkDataArraySelection* s)
{
  vtkMTimeType mTime = s->GetMTime();
  s->EnableAllArrays();
  if (mTime != s->GetMTime())
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// RequestInformation
int vtkOpenFOAMReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->FileName || !*(this->FileName))
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (this->Parent == this &&
    ((*this->FileNameOld != this->FileName) || this->Refresh ||
      (this->ListTimeStepsByControlDict != this->ListTimeStepsByControlDictOld) ||
      (this->SkipZeroTime != this->SkipZeroTimeOld)))
  {
    // Retain selection status when just refreshing a case
    if (!this->FileNameOld->empty() && *this->FileNameOld != this->FileName)
    {
      // Clear selections
      this->CellDataArraySelection->RemoveAllArrays();
      this->PointDataArraySelection->RemoveAllArrays();
      this->LagrangianDataArraySelection->RemoveAllArrays();
      this->PatchDataArraySelection->RemoveAllArrays();
    }

    // Reset NumberOfReaders here so that the variable will not be
    // reset unwantedly when MakeInformationVector() is called from
    // vtkPOpenFOAMReader
    this->NumberOfReaders = 0;

    if (!this->MakeInformationVector(outputVector, {}) || !this->MakeMetaDataAtTimeStep(true))
    {
      return 0;
    }
    this->Refresh = false;
  }
  return 1;
}

//------------------------------------------------------------------------------
// RequestData
int vtkOpenFOAMReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto* output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Times
  {
    int nTimes(0); // Also used for logic
    double requestedTimeValue(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      nTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

      // UPDATE_TIME_STEP is unreliable if there is only one time-step
      requestedTimeValue =
        (1 == nTimes ? outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 0)
                     : outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));
    }

    if (nTimes) // nTimes also used for logic
    {
      outInfo->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
      this->SetTimeValue(requestedTimeValue);
    }
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

  // Create dataset
  int ret = 1;
  vtkOpenFOAMReaderPrivate* reader;

  // Avoid wrapping single region as a multiblock dataset
  if (this->Readers->GetNumberOfItems() == 1 &&
    (reader = vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetItemAsObject(0)))
      ->GetRegionName()
      .empty())
  {
    ret = reader->RequestData(output);
    this->Parent->CurrentReaderIndex++;
  }
  else
  {
    this->Readers->InitTraversal();
    while ((reader = vtkOpenFOAMReaderPrivate::SafeDownCast(
              this->Readers->GetNextItemAsObject())) != nullptr)
    {
      auto subOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
      if (reader->RequestData(subOutput))
      {
        std::string regionName(reader->GetRegionName());
        if (regionName.empty())
        {
          regionName = "defaultRegion"; // == OpenFOAM "region0"
        }
        if (reader->HasPolyMesh()) // sanity check
        {
          ::AppendBlock(output, subOutput, regionName);
        }
      }
      else
      {
        ret = 0;
      }
      this->Parent->CurrentReaderIndex++;
    }
  }

  if (this->Parent == this) // update only if this is the top-level reader
  {
    this->UpdateStatus();
  }

  return ret;
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::SetTimeInformation(
  vtkInformationVector* outputVector, vtkDoubleArray* timeValues)
{
  const vtkIdType nTimes = timeValues->GetNumberOfTuples();
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  double timeRange[2];
  if (nTimes)
  {
    timeRange[0] = timeValues->GetValue(0);
    timeRange[1] = timeValues->GetValue(nTimes - 1);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeValues->GetPointer(0),
      static_cast<int>(nTimes));
  }
  else
  {
    timeRange[0] = timeRange[1] = 0.0;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeRange, 0);
  }
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
}

//------------------------------------------------------------------------------
int vtkOpenFOAMReader::MakeInformationVector(vtkInformationVector* outputVector,
  const vtkStdString& procName, vtkStringArray* timeNames, vtkDoubleArray* timeValues)
{
  this->FileNameOld->assign(this->FileName);

  // Clear prior case information
  this->Readers->RemoveAllItems();

  // Recreate case information
  vtkStdString casePath, controlDictPath;
  this->CreateCasePath(casePath, controlDictPath);

  if (!procName.empty())
  {
    casePath += procName + "/";
  }

  // Check for mesh directory and subregions.
  // Should check contents of constant/regionProperties, but that file may be missing so instead
  // check the existence of files in constant/polyMesh and constant/{region}/polyMesh.
  // A multi-region case will often not have the default region

  bool hasDefaultRegion = false;
  std::vector<std::string> regionNames;

  {
    const std::string constantPath(casePath + "constant/");
    vtkNew<vtkDirectory> dir;
    if (!dir->Open(constantPath.c_str()))
    {
      vtkErrorMacro(<< "Cannot open directory: " << constantPath);
      return 0;
    }

    hasDefaultRegion = vtkFoamFile::IsFile(constantPath + "polyMesh/faces", true);

    for (vtkIdType entryi = 0; entryi < dir->GetNumberOfFiles(); ++entryi)
    {
      std::string subDir(dir->GetFile(entryi));
      if (subDir != "." && subDir != ".." && dir->FileIsDirectory(subDir.c_str()) &&
        vtkFoamFile::IsFile((constantPath + subDir + "/polyMesh/faces"), true))
      {
        regionNames.push_back(std::move(subDir));
      }
    }

    if (!hasDefaultRegion && regionNames.empty())
    {
      vtkErrorMacro(<< this->FileName << " contains no meshes.");
      return 0;
    }

    // Consistently ordered
    std::sort(regionNames.begin(), regionNames.end());
  }

  vtkIdType nTimeNames = 0;
  vtkIdType nTimeValues = 0;

  if (((timeNames != nullptr) && (nTimeNames = timeNames->GetNumberOfTuples()) != 0) &&
    ((timeValues != nullptr) && (nTimeValues = timeValues->GetNumberOfTuples()) != 0) &&
    (nTimeNames != nTimeValues))
  {
    vtkErrorMacro(<< "Number of time values " << nTimeValues << " != number of time names "
                  << nTimeNames);
    return 0;
  }

  auto masterReader = vtkSmartPointer<vtkOpenFOAMReaderPrivate>::New();

  if (nTimeNames && nTimeNames == nTimeValues)
  {
    if (!masterReader->MakeInformationVector(
          casePath, procName, this->Parent, timeNames, timeValues, hasDefaultRegion))
    {
      return 0;
    }
  }
  else
  {
    if (!masterReader->MakeInformationVector(
          casePath, controlDictPath, procName, this->Parent, hasDefaultRegion))
    {
      return 0;
    }
  }

  nTimeValues = masterReader->GetTimeValues()->GetNumberOfTuples();
  if (!nTimeValues)
  {
    vtkErrorMacro(<< this->FileName << " contains no timestep data.");
  }

  if (hasDefaultRegion)
  {
    this->Readers->AddItem(masterReader);
  }

  // Add subregions
  for (const auto& regionName : regionNames)
  {
    auto subReader = vtkSmartPointer<vtkOpenFOAMReaderPrivate>::New();
    subReader->SetupInformation(casePath, regionName, procName, masterReader);
    this->Readers->AddItem(subReader);
  }

  this->Parent->NumberOfReaders += this->Readers->GetNumberOfItems();

  if (outputVector != nullptr)
  {
    this->SetTimeInformation(outputVector, masterReader->GetTimeValues());
  }
  if (this->Parent == this)
  {
    this->CreateCharArrayFromString(this->CasePath, "CasePath", casePath);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::CreateCasePath(vtkStdString& casePath, vtkStdString& controlDictPath)
{
#if defined(_WIN32)
  const std::string pathFindSeparator = "/\\", pathSeparator = "\\";
#else
  const std::string pathFindSeparator = "/", pathSeparator = "/";
#endif
  controlDictPath = this->FileName;

  // determine the case directory and path to controlDict
  auto pos = controlDictPath.find_last_of(pathFindSeparator);
  if (pos == std::string::npos)
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
      if (pos == std::string::npos)
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

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::AddSelectionNames(
  vtkDataArraySelection* selections, vtkStringArray* objects)
{
  objects->Squeeze();
  vtkSortDataArray::Sort(objects);
  for (int nameI = 0; nameI < objects->GetNumberOfValues(); nameI++)
  {
    selections->AddArray(objects->GetValue(nameI).c_str());
  }
}

//------------------------------------------------------------------------------
bool vtkOpenFOAMReader::SetTimeValue(const double timeValue)
{
  bool modified = false;
  this->Readers->InitTraversal();
  for (vtkObject* obj; (obj = this->Readers->GetNextItemAsObject()) != nullptr;)
  {
    // Is private implementation
    {
      auto* reader = vtkOpenFOAMReaderPrivate::SafeDownCast(obj);
      if (reader)
      {
        const vtkMTimeType mTime = reader->GetMTime();
        reader->SetTimeValue(timeValue);
        if (reader->GetMTime() != mTime)
        {
          modified = true;
        }
        continue;
      }
    }
    // Is sub-reader for derived type
    {
      auto* reader = vtkOpenFOAMReader::SafeDownCast(obj);
      if (reader)
      {
        if (reader->SetTimeValue(timeValue))
        {
          modified = true;
        }
        continue;
      }
    }
  }
  return modified;
}

//------------------------------------------------------------------------------
double vtkOpenFOAMReader::GetTimeValue() const
{
  vtkObject* obj = this->Readers->GetNumberOfItems() ? this->Readers->GetItemAsObject(0) : nullptr;

  if (obj)
  {
    // Is private implementation
    {
      auto* reader = vtkOpenFOAMReaderPrivate::SafeDownCast(obj);
      if (reader)
      {
        return reader->GetTimeValue();
      }
    }
    // Is sub-reader for derived type
    {
      auto* reader = vtkOpenFOAMReader::SafeDownCast(obj);
      if (reader)
      {
        return reader->GetTimeValue();
      }
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkStringArray* vtkOpenFOAMReader::GetTimeNames()
{
  vtkObject* obj = this->Readers->GetNumberOfItems() ? this->Readers->GetItemAsObject(0) : nullptr;

  if (obj)
  {
    // Is private implementation
    {
      auto* reader = vtkOpenFOAMReaderPrivate::SafeDownCast(obj);
      if (reader)
      {
        return reader->GetTimeNames();
      }
    }
    // Is sub-reader for derived type
    {
      auto* reader = vtkOpenFOAMReader::SafeDownCast(obj);
      if (reader)
      {
        return reader->GetTimeNames();
      }
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkOpenFOAMReader::GetTimeValues()
{
  vtkObject* obj = this->Readers->GetNumberOfItems() ? this->Readers->GetItemAsObject(0) : nullptr;

  if (obj)
  {
    // Is private implementation
    {
      auto* reader = vtkOpenFOAMReaderPrivate::SafeDownCast(obj);
      if (reader)
      {
        return reader->GetTimeValues();
      }
    }
    // Is sub-reader for derived type
    {
      auto* reader = vtkOpenFOAMReader::SafeDownCast(obj);
      if (reader)
      {
        return reader->GetTimeValues();
      }
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------
int vtkOpenFOAMReader::MakeMetaDataAtTimeStep(const bool listNextTimeStep)
{
  vtkNew<vtkStringArray> cellDataNames;
  vtkNew<vtkStringArray> pointDataNames;
  vtkNew<vtkStringArray> lagrangianDataNames;
  vtkNew<vtkStringArray> lagrangianPaths;

  if (listNextTimeStep)
  {
    this->LagrangianPaths->Initialize();
  }
  else
  {
    lagrangianPaths->DeepCopy(this->LagrangianPaths);
  }

  int ret = 1;
  vtkOpenFOAMReaderPrivate* reader;
  this->Readers->InitTraversal();
  while ((reader = vtkOpenFOAMReaderPrivate::SafeDownCast(this->Readers->GetNextItemAsObject())) !=
    nullptr)
  {
    ret *= reader->MakeMetaDataAtTimeStep(
      cellDataNames, pointDataNames, lagrangianDataNames, listNextTimeStep);

    appendUniq(lagrangianPaths, reader->GetLagrangianPaths());
  }
  this->AddSelectionNames(this->Parent->CellDataArraySelection, cellDataNames);
  this->AddSelectionNames(this->Parent->PointDataArraySelection, pointDataNames);
  this->AddSelectionNames(this->Parent->LagrangianDataArraySelection, lagrangianDataNames);

  lagrangianPaths->Squeeze();
  vtkSortDataArray::Sort(lagrangianPaths);

  // Combine for all regions
  this->LagrangianPaths->DeepCopy(lagrangianPaths);

  return ret;
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::CreateCharArrayFromString(
  vtkCharArray* array, const char* name, vtkStdString& string)
{
  array->Initialize();
  array->SetName(name);
  const size_t len = string.length();
  char* ptr = array->WritePointer(0, static_cast<vtkIdType>(len + 1));
  memcpy(ptr, string.c_str(), len);
  ptr[len] = '\0';
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::UpdateStatus()
{
  // update selection MTimes
  this->PatchSelectionMTimeOld = this->PatchDataArraySelection->GetMTime();
  this->CellSelectionMTimeOld = this->CellDataArraySelection->GetMTime();
  this->PointSelectionMTimeOld = this->PointDataArraySelection->GetMTime();
  this->LagrangianSelectionMTimeOld = this->LagrangianDataArraySelection->GetMTime();
  this->CreateCellToPointOld = this->CreateCellToPoint;
  this->DecomposePolyhedraOld = this->DecomposePolyhedra;
  this->PositionsIsIn13FormatOld = this->PositionsIsIn13Format;
  this->ReadZonesOld = this->ReadZones;
  this->SkipZeroTimeOld = this->SkipZeroTime;
  this->ListTimeStepsByControlDictOld = this->ListTimeStepsByControlDict;
  this->AddDimensionsToArrayNamesOld = this->AddDimensionsToArrayNames;
  this->Use64BitLabelsOld = this->Use64BitLabels;
  this->Use64BitFloatsOld = this->Use64BitFloats;
}

//------------------------------------------------------------------------------
void vtkOpenFOAMReader::UpdateProgress(double amount)
{
  this->vtkAlgorithm::UpdateProgress(
    (static_cast<double>(this->Parent->CurrentReaderIndex) + amount) /
    static_cast<double>(this->Parent->NumberOfReaders));
}
