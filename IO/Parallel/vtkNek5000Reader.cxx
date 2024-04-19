// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNek5000Reader.h"
#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArraySelection.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkTrivialProducer.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include <map>
#include <new>
#include <string>
#include <vtksys/SystemTools.hxx>

#ifdef _WIN32
#include <algorithm>
#include <direct.h>
#include <string.h>
#define strcasecmp _stricmp
#define getcwd _getcwd
#endif

using std::string;

vtkStandardNewMacro(vtkNek5000Reader);

namespace
{

void ByteSwap32(void* aVals, int nVals)
{
  char* v = (char*)aVals;
  char tmp;
  for (long ii = 0; ii < nVals; ii++, v += 4)
  {
    tmp = v[0];
    v[0] = v[3];
    v[3] = tmp;
    tmp = v[1];
    v[1] = v[2];
    v[2] = tmp;
  }
}

void ByteSwap64(void* aVals, int nVals)
{
  char* v = (char*)aVals;
  char tmp;
  for (long ii = 0; ii < nVals; ii++, v += 8)
  {
    tmp = v[0];
    v[0] = v[7];
    v[7] = tmp;
    tmp = v[1];
    v[1] = v[6];
    v[6] = tmp;
    tmp = v[2];
    v[2] = v[5];
    v[5] = tmp;
    tmp = v[3];
    v[3] = v[4];
    v[4] = tmp;
  }
}

int compare_ids(const void* id1, const void* id2)
{
  const int* a = (const int*)id1;
  const int* b = (const int*)id2;

  if (*a < *b)
    return (-1);
  if (*a > *b)
    return (1);
  return (0);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
class vtkNek5000Reader::nek5KObject
{
public:
  static constexpr int MAX_VARS = 100;
  vtkUnstructuredGrid* ugrid;
  bool vorticity;
  bool lambda_2;
  bool wss;
  bool stress_tensor;
  bool vars[MAX_VARS];
  bool der_vars[MAX_VARS];
  int index;

  nek5KObject* prev;
  nek5KObject* next;
  char* dataFilename;

  void setDataFilename(char* filename);
  void reset();

  // protected:
  nek5KObject();
  ~nek5KObject();
};

//----------------------------------------------------------------------------
class vtkNek5000Reader::nek5KList
{
public:
  nek5KObject* head;
  nek5KObject* tail;
  int max_count;
  int cur_count;
  nek5KObject* getObject(int);

  // protected:
  nek5KList();
  ~nek5KList();
};

//----------------------------------------------------------------------------
vtkNek5000Reader::vtkNek5000Reader()
{
  this->DebugOff();
  // vtkDebugMacro(<<"vtkNek5000Reader::vtkNek5000Reader(): ENTER");

  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = nullptr;
  this->DataFileName = nullptr;

  this->UGrid = nullptr;

  this->READ_GEOM_FLAG = true;
  this->CALC_GEOM_FLAG = true;
  this->IAM_INITIALLIZED = false;
  this->I_HAVE_DATA = false;
  this->MeshIs3D = true;
  this->swapEndian = false;
  this->ActualTimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 0;
  this->displayed_step = -1;
  this->memory_step = -1;
  this->requested_step = -1;

  this->num_vars = 0;
  this->var_names = nullptr;
  this->var_length = nullptr;
  this->dataArray = nullptr;
  this->meshCoords = nullptr;
  this->myBlockIDs = nullptr;
  this->myBlockPositions = nullptr;
  this->timestep_has_mesh = nullptr;
  this->proc_numBlocks = nullptr;

  this->SpectralElementIds = 0;
  this->CleanGrid = 0;

  this->PointDataArraySelection = vtkDataArraySelection::New();

  this->myList = new nek5KList();
}

//----------------------------------------------------------------------------
vtkNek5000Reader::~vtkNek5000Reader()
{
  delete[] this->timestep_has_mesh;
  delete[] this->FileName;
  delete[] this->DataFileName;
  delete this->myList;

  if (this->dataArray)
  {
    delete[] this->dataArray;
    this->dataArray = nullptr;
  }
  if (this->num_vars > 0)
  {
    for (auto i = 0; i < this->num_vars; i++)
      if (this->var_names[i])
      {
        free(this->var_names[i]);
      }
  }
  delete[] this->proc_numBlocks;

  if (this->UGrid)
  {
    this->UGrid->Delete();
  }

  delete[] this->var_length;
  if (this->var_names)
    free(this->var_names);
  this->PointDataArraySelection->Delete();

  delete[] this->myBlockPositions;
}

//----------------------------------------------------------------------------

bool vtkNek5000Reader::GetAllTimesAndVariableNames(vtkInformationVector* outputVector)
{
  std::ifstream dfPtr;
  char dummy[64];
  double t;
  int c;
  string v;

  char dfName[265];
  char firstTags[32];
  int file_index;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // vtkInformation* outInfo1 = outputVector->GetInformationObject(1);

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = this->NumberOfTimeSteps - 1;

  this->TimeSteps.resize(this->NumberOfTimeSteps);
  this->timestep_has_mesh = new bool[this->NumberOfTimeSteps];

  for (int i = 0; i < (this->NumberOfTimeSteps); i++)
  {
    this->timestep_has_mesh[i] = false;
    file_index = this->datafile_start + i;

    snprintf(dfName, sizeof(dfName), this->datafile_format.c_str(), 0, file_index);
    vtkDebugMacro(<< "vtkNek5000Reader::GetAllTimesAndVariableNames:  this->datafile_start = "
                  << this->datafile_start << "  i: " << i << " file_index: " << file_index
                  << " dfName: " << dfName);

    dfPtr.open(dfName, std::ifstream::binary);

    if ((dfPtr.rdstate() & std::ifstream::failbit) != 0)
    {
      vtkErrorMacro(<< "Error opening : " << dfName);
      return false;
    }

    dfPtr >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy;
    dfPtr >> t >> c >> dummy;
    vtkDebugMacro(<< "vtkNek5000Reader::GetAllTimesAndVariableNames:  time = " << t
                  << " cycle =  " << c);
    // I do this to skip the num directories token, because it may abut
    // the field tags without a whitespace separator.
    while (dfPtr.peek() == ' ')
      dfPtr.get();
    while (dfPtr.peek() >= '0' && dfPtr.peek() <= '9')
      dfPtr.get();

    char tmpTags[32];
    dfPtr.read(tmpTags, 32);
    tmpTags[31] = '\0';

    v = tmpTags;

    // for the first time step on the master
    if (0 == i)
    {
      // store the tags for the first step, and share with other procs to parse for variables
      strcpy(firstTags, tmpTags);
    }

    this->TimeSteps[i] = t;

    // If this file contains a mesh, the first variable codes after the
    // cycle number will be X Y
    if (v.find('X') != std::string::npos)
      this->timestep_has_mesh[i] = true;

    dfPtr.close();

    vtkDebugMacro(<< "vtkNek5000Reader::GetAllTimesAndVariableNames: this->TimeSteps[" << i
                  << "]= " << this->TimeSteps[i] << "  this->timestep_has_mesh[" << i
                  << "] = " << this->timestep_has_mesh[i]);

  } // for (int i=0; i<(this->NumberOfTimeSteps); i++)

  this->GetVariableNamesFromData(firstTags);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &(*this->TimeSteps.begin()),
    static_cast<int>(this->TimeSteps.size()));

  double timeRange[2];
  timeRange[0] = *this->TimeSteps.begin();

  vtkDebugMacro(<< "vtkNek5000Reader::GetAllTimes: timeRange[0] = " << timeRange[0]
                << ", timeRange[1] = " << timeRange[1]);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return true;
} // vtkNek5000Reader::GetAllTimes()

//----------------------------------------------------------------------------
vtkMTimeType vtkNek5000Reader::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  time = this->PointDataArraySelection->GetMTime();
  mTime = (time > mTime ? time : mTime);

  return mTime;
}

//----------------------------------------------------------------------------
void vtkNek5000Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkNek5000Reader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkNek5000Reader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
bool vtkNek5000Reader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
bool vtkNek5000Reader::GetPointArrayStatus(int index)
{
  return this->PointDataArraySelection->GetArraySetting(index);
}

//----------------------------------------------------------------------------
void vtkNek5000Reader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
void vtkNek5000Reader::EnableAllPointArrays()
{
  this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkNek5000Reader::DisableAllPointArrays()
{
  this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------

void vtkNek5000Reader::updateVariableStatus()
{
#ifndef NDEBUG
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }
#endif
  // vtkDebugMacro(<<"vtkNek5000Reader::updateVariableStatus: Rank: "<<my_rank<< " ENTER ");
  this->num_used_vectors = 0;
  this->num_used_scalars = 0;

  // if a variable is used, set it to true
  for (auto i = 0; i < this->num_vars; i++)
  {
    if (this->GetPointArrayStatus(i))
    {
      // increment the number of vectors or scalars used, accordingly
      if (this->var_length[i] > 1)
      {
        this->num_used_vectors++;
      }
      else
      {
        this->num_used_scalars++;
      }
    }
  }

  vtkDebugMacro(<< "vtkNek5000Reader::updateVariableStatus: Rank: " << my_rank
                << ": this->num_used_scalars= " << this->num_used_scalars
                << " : this->num_used_vectors= " << this->num_used_vectors);
}

//----------------------------------------------------------------------------
size_t vtkNek5000Reader::GetVariableNamesFromData(char* varTags)
{
  size_t ind = 0;
  int numSFields = 0;

  char* sPtr = nullptr;
  sPtr = strchr(varTags, 'S');
  if (sPtr)
  {
    sPtr++;
    while (*sPtr == ' ')
      sPtr++;
    char digit1 = *sPtr;
    sPtr++;
    while (*sPtr == ' ')
      sPtr++;
    char digit2 = *sPtr;

    if (digit1 >= '0' && digit1 <= '9' && digit2 >= '0' && digit2 <= '9')
      numSFields = (digit1 - '0') * 10 + (digit2 - '0');
    else
      numSFields = 1;
  }

  this->num_vars = 0;

  size_t len = strlen(varTags);

  // allocate space for variable names and lengths,
  // will be at most 4 + numSFields  (4 for velocity, velocity_magnitude, pressure and temperature)
  this->var_names = (char**)malloc((4 + numSFields) * sizeof(char*));
  for (int i = 0; i < 4 + numSFields; i++)
    this->var_names[i] = nullptr;

  this->var_length = new int[4 + numSFields];

  while (ind < len)
  {
    switch (varTags[ind])
    {
      case 'X':
      case 'Y':
      case 'Z':
        // if it is a coordinate, we have already accounted for that
        ind++;
        break;

      case 'U':
        this->PointDataArraySelection->AddArray("Velocity");
        this->var_names[this->num_vars] = strdup("Velocity");
        vtkDebugMacro(<< "GetVariableNamesFromData:  this->var_names[" << this->num_vars
                      << "] = " << this->var_names[this->num_vars]);
        this->var_length[this->num_vars] = 3; // this is a vector
        ind++;
        this->num_vars++;
        // Also add a magnitude scalar
        this->PointDataArraySelection->AddArray("Velocity Magnitude");
        this->var_names[this->num_vars] = strdup("Velocity Magnitude");
        vtkDebugMacro(<< "GetVariableNamesFromData:  this->var_names[" << this->num_vars
                      << "] = " << this->var_names[this->num_vars]);
        this->var_length[this->num_vars] = 1; // this is a scalar
        this->num_vars++;
        break;

      case 'P':
        this->PointDataArraySelection->AddArray("Pressure");
        this->var_names[this->num_vars] = strdup("Pressure");
        vtkDebugMacro(<< "GetVariableNamesFromData:  this->var_names[" << this->num_vars
                      << "] = " << this->var_names[this->num_vars]);
        this->var_length[this->num_vars] = 1; // this is a scalar
        ind++;
        this->num_vars++;
        break;

      case 'T':
        this->PointDataArraySelection->AddArray("Temperature");
        this->var_names[this->num_vars] = strdup("Temperature");
        vtkDebugMacro(<< "GetVariableNamesFromData:  this->var_names[" << this->num_vars
                      << "] = " << this->var_names[this->num_vars]);
        this->var_length[this->num_vars] = 1; // this is a scalar
        ind++;
        this->num_vars++;
        break;

      case 'S':
        for (int sloop = 0; sloop < numSFields; sloop++)
        {
          char sname[4];
          snprintf(sname, sizeof(sname), "S%02d", sloop + 1);
          this->PointDataArraySelection->AddArray(sname);
          this->var_names[this->num_vars] = strdup(sname);
          vtkDebugMacro(<< "GetVariableNamesFromData:  this->var_names[" << this->num_vars
                        << "] = " << this->var_names[this->num_vars]);
          this->var_length[this->num_vars] = 1; // this is a scalar
          ind += 3;
          this->num_vars++;
        }
        break;
      default:
        ind++;
        break;
    }

  } // while(ind<len)

  // this->DisableAllDerivedVariableArrays();

  return len;
}

//----------------------------------------------------------------------------

void vtkNek5000Reader::readData(char* dfName)
{
  long total_header_size = 136 + (this->numBlocks * 4);
  long read_location;
  long read_size;
  std::ifstream dfPtr;
  float* dataPtr;
  double* tmpDblPtr = nullptr;

  dfPtr.open(dfName, std::ifstream::binary);
  if (dfPtr.is_open())
  {
    // if this data file includes the mesh, add it to header size
    if (this->timestep_has_mesh[this->ActualTimeStep])
    {
      long offset1;
      offset1 = this->numBlocks;
      offset1 *= this->totalBlockSize;
      if (this->MeshIs3D)
        offset1 *= 3; // account for X, Y and Z
      else
        offset1 *= 2; // account only for X, Y
      offset1 *= this->precision;
      total_header_size += offset1;
    }
    // currently reading a block at a time, if we need doubles, allocate an array for a block
    if (this->precision == 8)
    {
      tmpDblPtr = new double[this->totalBlockSize * 3];
    }

    // for each variable
    long var_offset;
    long l_blocksize, scalar_offset;
    scalar_offset = this->numBlocks;
    scalar_offset *= this->totalBlockSize;
    scalar_offset *= this->precision;

    for (auto i = 0; i < this->num_vars; i++)
    {
      if (i < 2)
      { // if Velocity or Velocity Magnitude
        var_offset = 0;
      }
      else
      {
        if (this->MeshIs3D)
          var_offset = (3 + (i - 2)) * scalar_offset; // counts VxVyVz
        else
          var_offset = (2 + (i - 2)) * scalar_offset; // counts VxVy
      }
      dataPtr = this->dataArray[i];

      if (dataPtr)
      {
        if (strcmp(this->var_names[i], "Velocity") == 0 && !this->MeshIs3D)
        {
          read_size = this->totalBlockSize * 2;
        }
        else
        {
          read_size = this->totalBlockSize * this->var_length[i];
        }
        l_blocksize = read_size * this->precision;

        if (this->precision == 4)
        {
          for (auto j = 0; j < this->myNumBlocks; j++)
          {
            read_location =
              total_header_size + var_offset + long(this->myBlockPositions[j] * l_blocksize);
            dfPtr.seekg(read_location, std::ios_base::beg);
            if (!dfPtr)
              std::cerr << __LINE__ << "block=" << j
                        << ": seekg error for block position = " << this->myBlockPositions[j]
                        << std::endl;
            dfPtr.read((char*)dataPtr, read_size * sizeof(float));
            if (!dfPtr)
              std::cerr << __LINE__ << ": read error for paylood of " << read_size
                        << " floats = " << read_size * sizeof(float) << std::endl;
            /*
            when reading vectors, such as Velocity, first come all Vx components, then all Vy, then
            all Vz. if reading 2D, it is safer to set the Z component to 0.
            */
            if (strcmp(this->var_names[i], "Velocity") == 0 && !this->MeshIs3D)
            {
              std::fill_n(&dataPtr[read_size], this->totalBlockSize, 0);
            }
            dataPtr += this->totalBlockSize * this->var_length[i];
          }
          if (this->swapEndian)
          {
            std::cout << "ByteSwap32()\n";
            ByteSwap32(
              this->dataArray[i], this->myNumBlocks * this->totalBlockSize * this->var_length[i]);
          }
        }
        else // precision == 8
        {
          for (auto j = 0; j < this->myNumBlocks; j++)
          {
            read_location =
              total_header_size + var_offset + long(this->myBlockPositions[j] * l_blocksize);
            dfPtr.seekg(read_location, std::ios_base::beg);
            if (!dfPtr)
              std::cerr << __LINE__ << ": seekg error at read_location = " << read_location
                        << std::endl;
            dfPtr.read((char*)tmpDblPtr, read_size * sizeof(double));
            if (!dfPtr)
              std::cerr << __LINE__ << ": read error\n";
            for (auto ind = 0; ind < read_size; ind++)
            {
              *dataPtr = (float)tmpDblPtr[ind];
              dataPtr++;
            }
          }
          if (this->swapEndian)
            ByteSwap64(
              this->dataArray[i], this->myNumBlocks * this->totalBlockSize * this->var_length[i]);
        }

        // if this is velocity, also add the velocity magnitude if and only if it has also been
        // requested
        if (strcmp(this->var_names[i], "Velocity") == 0 &&
          this->GetPointArrayStatus("Velocity Magnitude"))
        {
          float vx, vy, vz;
          int coord_offset =
            this->totalBlockSize; // number of values for one coordinate (X or Y or Z)
          for (auto j = 0; j < this->myNumBlocks; j++)
          {
            int mag_block_offset = j * this->totalBlockSize;
            int comp_block_offset = mag_block_offset * 3;
            for (auto k = 0; k < this->totalBlockSize; k++)
            {
              vx = this->dataArray[i][comp_block_offset + k];
              vy = this->dataArray[i][coord_offset + comp_block_offset + k];
              vz = this->dataArray[i][coord_offset + coord_offset + comp_block_offset + k];
              this->dataArray[i + 1][mag_block_offset + k] =
                std::sqrt((vx * vx) + (vy * vy) + (vz * vz));
            }
          }
          i++; // skip over the velocity magnitude variable, since we just took care of it
        }      // if "Velocity"
      }        // only read if valid pointer
    }          // for(i=0; i<this->num_vars; i++)

    if (this->precision == 8)
    {
      delete[] tmpDblPtr;
    }
    dfPtr.close();
  }
  else
  {
    std::cerr << "Error opening datafile : " << dfName << endl;
    exit(1);
  }
} // vtkNek5000Reader::readData(char* dfName)

//----------------------------------------------------------------------------

void vtkNek5000Reader::partitionAndReadMesh()
{
  char dfName[265];
  std::ifstream dfPtr;
  int i;
  string buf2, tag;
  std::map<int, int> blockMap;

  int my_rank;
  int num_ranks;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
    num_ranks = ctrl->GetNumberOfProcesses();
  }
  else
  {
    my_rank = 0;
    num_ranks = 1;
  }

  snprintf(dfName, sizeof(dfName), this->datafile_format.c_str(), 0, this->datafile_start);
  dfPtr.open(dfName, std::ifstream::binary);

  if ((dfPtr.rdstate() & std::ifstream::failbit) != 0)
  {
    std::cerr << "Error opening : " << dfName << endl;
    exit(1);
  }

  dfPtr >> tag;
  if (tag != "#std")
  {
    cerr << "Error reading the header.  Expected it to start with #std " << dfName << endl;
    exit(1);
  }
  dfPtr >> this->precision;
  dfPtr >> this->blockDims[0];
  dfPtr >> this->blockDims[1];
  dfPtr >> this->blockDims[2];
  dfPtr >> buf2; // blocks per file
  dfPtr >> this->numBlocks;

  this->totalBlockSize = this->blockDims[0] * this->blockDims[1] * this->blockDims[2];
  if (this->blockDims[2] > 1)
  {
    this->MeshIs3D = true;
    std::cout << "3D-Mesh found";
  }
  else
  {
    this->MeshIs3D = false;
    std::cout << "2D-Mesh found";
  }
  std::cout << ", spectral element of size = " << this->blockDims[0] << "*" << this->blockDims[1]
            << "*" << this->blockDims[2] << "=" << this->totalBlockSize << std::endl;

  float test;
  dfPtr.seekg(132, std::ios_base::beg);
  dfPtr.read((char*)(&test), 4);

  // see if we need to swap endian
  if (test > 6.5 && test < 6.6)
    this->swapEndian = false;
  else
  {
    ByteSwap32(&test, 1);
    if (test > 6.5 && test < 6.6)
      this->swapEndian = true;
    else
    {
      std::cerr << "Error reading file, while trying to determine endianness : " << dfName << endl;
      exit(1);
    }
  }

  int* tmpBlocks = new int[numBlocks];
  this->proc_numBlocks = new int[num_ranks];

  // figure out how many blocks (elements) each proc will handle
  int elements_per_proc = this->numBlocks / num_ranks;
  int one_extra_until = this->numBlocks % num_ranks;

  for (i = 0; i < num_ranks; i++)
  {
    this->proc_numBlocks[i] = elements_per_proc + (i < one_extra_until ? 1 : 0);
  }
  this->myNumBlocks = this->proc_numBlocks[my_rank];
  this->myBlockIDs = new int[this->myNumBlocks];

  // read the ids of all of the blocks in the file
  dfPtr.seekg(136, std::ios_base::beg);
  dfPtr.read((char*)tmpBlocks, this->numBlocks * sizeof(int));
  if (this->swapEndian)
    ByteSwap32(tmpBlocks, this->numBlocks);

  // add the block locations to a map, so that we can easily find their position based on their id
  for (i = 0; i < this->numBlocks; i++)
  {
    blockMap[tmpBlocks[i]] = i;
  }

  // if there is a .map file, we will use that to partition the blocks
  char* map_filename = strdup(this->GetFileName());
  char* ext = strrchr(map_filename, '.');
  int* all_element_list;
  ext++;
  size_t extLength = strlen(ext);
  snprintf(ext, extLength + 1, "map");
  std::ifstream mptr(map_filename);
  int* map_elements = nullptr;
  if (mptr.is_open())
  {
    vtkDebugMacro(<< "vtkNek5000Reader::partitionAndReadMesh: found mapfile: " << map_filename);
    int num_map_elements;
    mptr >> num_map_elements >> buf2 >> buf2 >> buf2 >> buf2 >> buf2 >> buf2;
    map_elements = new int[num_map_elements];
    for (i = 0; i < num_map_elements; i++)
    {
      mptr >> map_elements[i] >> buf2 >> buf2 >> buf2 >> buf2 >> buf2 >> buf2 >> buf2 >> buf2;
      map_elements[i] += 1;
    }
    mptr.close();

    all_element_list = map_elements;
  }
  // otherwise just use the order in the data file
  else
  {
    vtkDebugMacro(<< "vtkNek5000Reader::partitionAndReadMesh: did not find mapfile: "
                  << map_filename);
    all_element_list = tmpBlocks;
  }
  free(map_filename);

  int start_index = 0;
  for (i = 0; i < my_rank; i++)
  {
    start_index += this->proc_numBlocks[i];
  }
  // copy my list of elements
  for (i = 0; i < this->myNumBlocks; i++)
  {
    this->myBlockIDs[i] = all_element_list[start_index + i];
  }
  // if they came from the map file, sort them
  if (map_elements != nullptr)
  {
    qsort(this->myBlockIDs, this->myNumBlocks, sizeof(int), compare_ids);
  }

  // now that we have our list of blocks, get their positions in the file (their index)
  this->myBlockPositions = new int[this->myNumBlocks];

  for (i = 0; i < this->myNumBlocks; i++)
  {
    this->myBlockPositions[i] = blockMap.find(this->myBlockIDs[i])->second;
  }

  // TEMP: checking for duplicates within myBlockPositions
  if (map_elements != nullptr)
  {
    for (i = 0; i < this->myNumBlocks - 1; i++)
    {
      for (auto j = i + 1; j < this->myNumBlocks; j++)
      {
        if (this->myBlockPositions[i] == this->myBlockPositions[j])
        {
          cerr << "********my_rank: " << my_rank << " : Hey (this->myBlockPositions[" << i
               << "] and [" << j << "] both == " << this->myBlockPositions[j] << endl;
        }
      }
    }
  }

  delete[] tmpBlocks;
  delete[] map_elements;

  // now read the coordinates for all of my blocks
  if (nullptr == this->meshCoords)
  {
    vtkDebugMacro(<< ": partitionAndReadMesh:  ALLOCATE meshCoords[" << this->myNumBlocks << "*"
                  << this->totalBlockSize << "*" << 3 << "]");
    this->meshCoords = new float[this->myNumBlocks * this->totalBlockSize * 3];
  }

  long total_header_size = 136 + (this->numBlocks * 4);
  long read_location, offset1;

  if (this->precision == 4)
  {
    float* coordPtr = this->meshCoords;
    int read_size;
    if (this->MeshIs3D)
    {
      read_size = this->totalBlockSize * 3;
      for (i = 0; i < this->myNumBlocks; i++)
      {
        // header + (index_of_this_block * size_of_a_block * variable_in_block (x,y,z) * precision)
        offset1 = this->myBlockPositions[i];
        offset1 *= this->totalBlockSize;
        offset1 *= 3;
        offset1 *= this->precision;
        read_location = total_header_size + offset1;
        dfPtr.seekg(read_location, std::ios_base::beg);
        if (!dfPtr)
          std::cerr << __LINE__ << ": seekg error at read_location = " << read_location
                    << std::endl;
        dfPtr.read((char*)coordPtr, read_size * sizeof(float));
        if (!dfPtr)
          std::cerr << __LINE__ << ": read error\n";
        coordPtr += read_size;
      }
    }
    else
    { // 2D case
      read_size = this->totalBlockSize * 2;
      for (i = 0; i < this->myNumBlocks; i++)
      {
        // header + (index_of_this_block * size_of_a_block * variable_in_block (x,y,z) * precision)
        offset1 = this->myBlockPositions[i];
        offset1 *= this->totalBlockSize;
        offset1 *= 2;
        offset1 *= this->precision;
        read_location = total_header_size + offset1;
        dfPtr.seekg(read_location, std::ios_base::beg);
        if (!dfPtr)
          std::cerr << __LINE__ << ": seekg error at read_location = " << read_location
                    << std::endl;
        dfPtr.read((char*)coordPtr, read_size * sizeof(float));

        if (!dfPtr)
          std::cerr << __LINE__ << ": read error\n";
        // now set the Z component to 0.0
        std::fill_n(&coordPtr[read_size], this->totalBlockSize, 0);
        coordPtr += (this->totalBlockSize * 3);
      }
    }

    if (this->swapEndian)
      ByteSwap32(this->meshCoords, this->myNumBlocks * this->totalBlockSize * 3);
  }
  else // precision == 8
  {
    float* coordPtr = this->meshCoords;
    double* tmpDblPts = new double[this->totalBlockSize * 3];
    int read_size = this->totalBlockSize * 3;
    for (i = 0; i < this->myNumBlocks; i++)
    {
      // header + (index_of_this_block * size_of_a_block * variable_in_block (x,y,z) * precision)
      read_location = total_header_size +
        int64_t(this->myBlockPositions[i] * this->totalBlockSize * 3 * this->precision);
      // fseek(dfPtr, read_location, SEEK_SET);
      // fread(tmpDblPts, sizeof(double), read_size, dfPtr);
      dfPtr.seekg(read_location, std::ios_base::beg);
      if (!dfPtr)
        std::cerr << __LINE__ << ": seekg error at read_location = " << read_location << std::endl;
      dfPtr.read((char*)tmpDblPts, read_size * sizeof(double));
      for (auto ind = 0; ind < read_size; ind++)
      {
        *coordPtr = (float)tmpDblPts[ind];
        coordPtr++;
      }
    }
    if (this->swapEndian)
      ByteSwap64(this->meshCoords, this->myNumBlocks * this->totalBlockSize * 3);
    delete[] tmpDblPts;
  }
  delete[] this->myBlockIDs;
  dfPtr.close();
} // void vtkNek5000Reader::partitionAndReadMesh()

//----------------------------------------------------------------------------
int vtkNek5000Reader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  string tag;
  char buf[2048];
#ifndef NDEBUG
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }
#endif

  if (!this->IAM_INITIALLIZED)
  {
    // Might consider having just the master node read the .nek5000 file, and broadcast each line to
    // the other processes ??

    char* filename = this->GetFileName();
    std::ifstream inPtr(this->GetFileName());

    // print the name of the file we're supposed to open
    vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation: FileName: " << this->GetFileName());

    // Process a tag at a time until all lines have been read
    while (inPtr.good())
    {
      inPtr >> tag;
      if (inPtr.eof())
      {
        inPtr.clear();
        break;
      }

      if (tag[0] == '#')
      {
        inPtr.getline(buf, 2048);
        continue;
      }

      if (strcasecmp("nek5000", tag.c_str()) == 0)
      {
        vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation: format: " << tag.c_str());
      }
      else if (strcasecmp("endian:", tag.c_str()) == 0)
      {
        // This tag is deprecated.  There's a float written into each binary file
        // from which endianness can be determined.
        string dummy_endianness;
        inPtr >> dummy_endianness;
      }
      else if (strcasecmp("version:", tag.c_str()) == 0)
      {
        // This tag is deprecated.  There's a float written into each binary file
        // from which endianness can be determined.
        string dummy_version;
        inPtr >> dummy_version;
        vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation:  version: " << dummy_version);
      }
      else if (strcasecmp("filetemplate:", tag.c_str()) == 0)
      {
        inPtr >> this->datafile_format;
        vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation:  this->datafile_format: "
                      << this->datafile_format);
      }
      else if (strcasecmp("firsttimestep:", tag.c_str()) == 0)
      {
        inPtr >> this->datafile_start;
        vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation:  this->datafile_start: "
                      << this->datafile_start);
      }
      else if (strcasecmp("numtimesteps:", tag.c_str()) == 0)
      {
        inPtr >> this->datafile_num_steps;
        vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation:  this->datafile_num_steps: "
                      << this->datafile_num_steps);
      }
      else
      {
        snprintf(buf, 2048, "Error parsing file.  Unknown tag %s", tag.c_str());
        cerr << buf << endl;
        exit(1);
      }
    } // while (inPtr.good())

    inPtr.close();

    int ii = 0;
    if (this->datafile_format[0] != '/')
    {
      for (ii = static_cast<int>(strlen(filename)) - 1; ii >= 0; ii--)
      {
        if (filename[ii] == '/' || filename[ii] == '\\')
        {
          this->datafile_format.insert(0, filename, ii + 1);
          break;
        }
      }
    }
    if (ii == -1)
    {
      if (!getcwd(buf, 512))
      {
        vtkWarningMacro(<< "CWD longer than 512: " << buf);
      }
      strcat(buf, "/");
      this->datafile_format.insert(0, buf, strlen(buf));
    }

    vtkDebugMacro(<< "vtkNek5000Reader::RequestInformation:  this->datafile_format: "
                  << this->datafile_format);

    this->NumberOfTimeSteps = this->datafile_num_steps;

    // GetAllTimes() now also calls GetVariableNamesFromData()
    vtkNew<vtkTimerLog> timer;

    if (!this->GetAllTimesAndVariableNames(outputVector))
    {
      return 0;
    }

    char dfName[265];

    vtkDebugMacro(<< "Rank: " << my_rank << " :: this->datafile_start= " << this->datafile_start);

    snprintf(dfName, sizeof(dfName), this->datafile_format.c_str(), 0, this->datafile_start);
    this->SetDataFileName(dfName);

    vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
    outInfo0->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

    this->IAM_INITIALLIZED = true;
  } // if(!this->IAM_INITIALLIZED)

  return 1;
} // int vtkNek5000Reader::RequestInformation()

int vtkNek5000Reader::RequestData(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
#ifndef NDEBUG
  double total_timer_diff;
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkTimerLog> total_timer;
  total_timer->StartTimer();
#endif
  int i;
  char dfName[256];
  if (!this->IAM_INITIALLIZED)
  {
    vtkErrorMacro("Reader not initialized properly");
    return 0;
  }

  // which output port did the request come from
  int outputPort = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  vtkDebugMacro(<< "RequestData: ENTER: outputPort = " << outputPort);

  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
  {
    outputPort = 0;
  }

  // get the data object
  vtkInformation* outInfo = outputVector->GetInformationObject(0); //(outputPort);

  vtkInformation* outInfoArray[2];
  outInfoArray[0] = outInfo;

  vtkInformation* requesterInfo = outputVector->GetInformationObject(outputPort);

  int tsLength = requesterInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double* steps = requesterInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkDebugMacro(<< "RequestData: tsLength= " << tsLength);

  this->updateVariableStatus();

  // Check if a particular time was requested.
  bool hasTimeValue = false;
  vtkDebugMacro(<< __LINE__ << " RequestData:");
  // Collect the time step requested
  vtkInformationDoubleKey* timeKey = vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP();

  if (outInfoArray[outputPort]->Has(timeKey))
  {
    this->TimeValue = outInfoArray[outputPort]->Get(timeKey);
    hasTimeValue = true;
  }

  if (hasTimeValue)
  {
    vtkDebugMacro(<< "RequestData: this->TimeValue= " << this->TimeValue);

    // find the timestep with the closest value to the requested time value
    int closestStep = 0;
    double minDist = -1;
    for (int cnt = 0; cnt < tsLength; cnt++)
    {
      // fprintf(stderr, "RequestData: steps[%d]=%f\n", cnt, steps[cnt]);
      double tdist = (steps[cnt] - this->TimeValue > this->TimeValue - steps[cnt])
        ? steps[cnt] - this->TimeValue
        : this->TimeValue - steps[cnt];
      if (minDist < 0 || tdist < minDist)
      {
        minDist = tdist;
        closestStep = cnt;
      }
    }
    this->ActualTimeStep = closestStep;
  }

  vtkDebugMacro(<< "RequestData: this->ActualTimeStep= " << this->ActualTimeStep);

#ifndef NDEBUG
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }
#endif
  vtkDebugMacro(<< "RequestData: ENTER: rank: " << my_rank << "  outputPort: " << outputPort
                << "  this->ActualTimeStep = " << this->ActualTimeStep);

  vtkUnstructuredGrid* ugrid =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Save the time value in the output (ugrid) data information.
  if (steps)
  {
    ugrid->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), steps[this->ActualTimeStep]);
  }

  //  int new_rst_val = this->p_rst_start + (this->p_rst_inc* this->ActualTimeStep);
  this->requested_step = this->datafile_start + this->ActualTimeStep;

  //  if the step being displayed is different than the one requested
  if (this->displayed_step != this->requested_step)
  {
    // get the requested object from the list, if the ugrid in the object is NULL
    // then we have not loaded it yet
    this->curObj = this->myList->getObject(this->requested_step);

    if (this->isObjectMissingData())
    {
      // if the step in memory is different than the step requested
      if (this->requested_step != this->memory_step)
      {
        this->I_HAVE_DATA = false;
      }
    }
  }

  // if I have not yet read the geometry, this should only happen once
  if (this->READ_GEOM_FLAG)
  {
    this->partitionAndReadMesh();
    this->READ_GEOM_FLAG = false;
  }

  if (!this->dataArray)
  {
    this->dataArray = new float*[this->num_vars];
  }
  // only allocate data array if the varname has been selected
  for (i = 0; i < this->num_vars; i++)
  {
    if (this->GetPointArrayStatus(i))
    {
      this->dataArray[i] =
        new float[this->myNumBlocks * this->totalBlockSize * this->var_length[i]];
    }
    else
    {
      this->dataArray[i] = nullptr;
    }
  }

  // Get the file name for requested time step

  snprintf(dfName, sizeof(dfName), this->datafile_format.c_str(), 0, this->requested_step);
  vtkDebugMacro(<< "vtkNek5000Reader::RequestData: Rank: " << my_rank
                << " Now reading data from file: " << dfName
                << " this->requested_step: " << this->requested_step);

  this->readData(dfName);
  this->curObj->setDataFilename(dfName);

  this->I_HAVE_DATA = true;
  this->memory_step = this->requested_step;

  this->updateVtuData(ugrid);

  this->SetDataFileName(this->curObj->dataFilename);

#ifndef NDEBUG
  total_timer->StopTimer();
  total_timer_diff = total_timer->GetElapsedTime();
#endif
  vtkDebugMacro(<< "vtkNek5000Reader::RequestData: Rank: " << my_rank
                << "  outputPort: " << outputPort << " EXIT :: Total time: " << total_timer_diff);

  return 1;
} // vtkNek5000Reader::RequestData()

void vtkNek5000Reader::updateVtuData(vtkUnstructuredGrid* pv_ugrid)
{
#ifndef NDEBUG
  double timer_diff;
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }
#endif
  // if the grid in the curObj is not NULL, we may have everything we need
  if (this->curObj->ugrid)
  {
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank
                  << ": this->curObj->ugrid != nullptr, see if it matches");
    if (this->objectMatchesRequest() && (this->displayed_step == this->requested_step))
    {
      // copy the ugrid
      pv_ugrid->ShallowCopy(this->curObj->ugrid);

      this->displayed_step = this->requested_step;
      vtkDebugMacro(<< "vtkNek5000Reader::updateVtuData: ugrid same, copy : Rank: " << my_rank);
      this->SetDataFileName(curObj->dataFilename);

      return;
    }
    else if (this->objectHasExtraData())
    {
      for (int vid = 0; vid < this->num_vars; vid++)
      {
        if (!this->GetPointArrayStatus(vid) && this->curObj->vars[vid])
        {
          // Does PV already have this array?  If so, remove it.
          if (pv_ugrid->GetPointData()->GetArray(this->var_names[vid]) != nullptr)
          {
            pv_ugrid->GetPointData()->RemoveArray(this->var_names[vid]);
          }
          // Do I already have this array?  If so, remove it.
          if (this->curObj->ugrid->GetPointData()->GetArray(this->var_names[vid]) != nullptr)
          {
            this->curObj->ugrid->GetPointData()->RemoveArray(this->var_names[vid]);
          }
          this->curObj->vars[vid] = false;
        }
      }

      pv_ugrid->ShallowCopy(this->curObj->ugrid);
      this->displayed_step = this->requested_step;
      // if(!this->USE_MESH_ONLY)
      {
        this->SetDataFileName(curObj->dataFilename);
      }
      // return;
    } // else if(this->objectHasExtraData())
  }   // if(this->curObj->ugrid)

  // otherwise the grid in the curObj is NULL, and/or the resolution has changed,
  // and/or we need more data than is in curObj, we need to do everything

  int Nvert_total = 0;
  int Nelements_total;

  vtkSmartPointer<vtkPoints> points;

  Nvert_total = this->myNumBlocks * this->totalBlockSize;
  if (this->MeshIs3D)
    Nelements_total = this->myNumBlocks * (this->blockDims[0] - 1) * (this->blockDims[1] - 1) *
      (this->blockDims[2] - 1);
  else
    Nelements_total = this->myNumBlocks * (this->blockDims[0] - 1) * (this->blockDims[1] - 1);

  vtkDebugMacro(<< "updateVtuData: rank = " << my_rank << " :Nvert_total= " << Nvert_total
                << ", Nelements_total= " << Nelements_total);

  // if we need to calculate the geometry (first time, or it has changed)
  if (this->CALC_GEOM_FLAG)
  {
#ifndef NDEBUG
    vtkNew<vtkTimerLog> timer;
    timer->StartTimer();
#endif
    if (this->UGrid)
    {
      this->UGrid->Delete();
    }
    this->UGrid = vtkUnstructuredGrid::New();
    // this->UGrid->Allocate(Nelements_total);
    // remove the Allocation here, in order to do a direct SelCells()
    // call in addCellsToContinuumMesh
    points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(Nvert_total);

    vtkDebugMacro(<< " : updateVtuData : rank = " << my_rank
                  << ": Nelements_total = " << Nelements_total << " Nvert_total = " << Nvert_total);

    copyContinuumPoints(points);
#ifndef NDEBUG
    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();
#endif
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank
                  << ": time to copy/convert xyz and uvw: " << timer_diff);
  } // if (this->CALC_GEOM_FLAG)

  vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank << ": call copyContinuumData()");

  this->copyContinuumData(pv_ugrid);

#ifndef NDEBUG
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
#endif
  if (this->CALC_GEOM_FLAG)
  {
    addCellsToContinuumMesh();
    if (this->SpectralElementIds) // optional. If one wants to extract cells belonging to specific
                                  // spectral element(s)
      addSpectralElementId(Nelements_total);
    this->UGrid->SetPoints(points);
  }
#ifndef NDEBUG
  timer->StopTimer();
  timer_diff = timer->GetElapsedTime();
#endif
  vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank
                << ": time of CALC_GEOM (the mesh): " << timer_diff);
  if (this->CleanGrid)
  {
#ifndef NDEBUG
    timer->StartTimer();
#endif
    vtkNew<vtkStaticCleanUnstructuredGrid> clean;

    vtkNew<vtkUnstructuredGrid> tmpGrid;
    tmpGrid->ShallowCopy(this->UGrid);
    clean->SetInputData(tmpGrid.GetPointer());

    clean->Update();
#ifndef NDEBUG
    timer->StopTimer();
    timer_diff = timer->GetElapsedTime();
#endif
    vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank
                  << ": time to clean the grid: " << timer_diff);

    pv_ugrid->ShallowCopy(clean->GetOutput());
  }
  else
  {
    pv_ugrid->ShallowCopy(this->UGrid);
  }
  vtkDebugMacro(<< "updateVtuData: my_rank= " << my_rank
                << ":  completed ShallowCopy to pv_ugrid\n");
  if (this->curObj->ugrid)
  {
    this->curObj->ugrid->Delete();
  }
  this->curObj->ugrid = vtkUnstructuredGrid::New();

  this->curObj->ugrid->ShallowCopy(this->UGrid);

  this->displayed_step = this->requested_step;

  for (int kk = 0; kk < this->num_vars; kk++)
  {
    this->curObj->vars[kk] = this->GetPointArrayStatus(kk);
  }

  this->CALC_GEOM_FLAG = false;
} // vtkNek5000Reader::updateVtuData()

void vtkNek5000Reader::addCellsToContinuumMesh()
{
  // Note that point ids are starting at 0, and are local to each processor
  // same with cellids. Local and starting at 0 on each MPI task
  int numVTKCells = this->myNumBlocks * (this->blockDims[0] - 1) * (this->blockDims[1] - 1);
  if (this->MeshIs3D)
    numVTKCells *= (this->blockDims[2] - 1);

  vtkUnsignedCharArray* cellTypes = vtkUnsignedCharArray::New(); // type array (HEX or QUAD)
  cellTypes->SetNumberOfTuples(numVTKCells);

  vtkCellArray* outCells = vtkCellArray::New(); // the connectivity array

  vtkIdTypeArray* locations = vtkIdTypeArray::New(); // the offset array
  locations->SetNumberOfTuples(numVTKCells);

  vtkIdType p, pts[8];
  int n = 0, c = 0;

  if (this->MeshIs3D)
  {
    cellTypes->Fill(VTK_HEXAHEDRON);
    outCells->Allocate(9L * numVTKCells);
    for (auto e = 0; e < this->myNumBlocks; ++e)
    {
      for (auto ii = 0; ii < this->blockDims[0] - 1; ++ii)
      {
        for (auto jj = 0; jj < this->blockDims[1] - 1; ++jj)
        {
          for (auto kk = 0; kk < this->blockDims[2] - 1; ++kk)
          {
            p =
              kk * (this->blockDims[1]) * (this->blockDims[0]) + jj * (this->blockDims[0]) + ii + n;
            pts[0] = p;
            pts[1] = p + 1;
            p += this->blockDims[0];
            pts[2] = p + 1;
            pts[3] = p;
            p = (kk + 1) * (this->blockDims[1]) * (this->blockDims[0]) + jj * (this->blockDims[0]) +
              ii + n;
            pts[4] = p;
            pts[5] = p + 1;
            p += this->blockDims[0];
            pts[6] = p + 1;
            pts[7] = p;

            outCells->InsertNextCell(8, pts);
            locations->SetTuple1(c, c * 9L);
            c++;
          }
        }
      }
      n += this->totalBlockSize;
    }
  }
  else // 2D
  {
    cellTypes->Fill(VTK_QUAD);
    outCells->Allocate(5L * numVTKCells);
    for (auto e = 0; e < this->myNumBlocks; ++e)
    {
      for (auto ii = 0; ii < this->blockDims[0] - 1; ++ii)
      {
        for (auto jj = 0; jj < this->blockDims[1] - 1; ++jj)
        {
          p = n + jj * (this->blockDims[0]) + ii;
          pts[0] = p;
          pts[1] = p + 1;
          p += this->blockDims[0];
          pts[2] = p + 1;
          pts[3] = p;
          outCells->InsertNextCell(4, pts);
          locations->SetTuple1(c, c * 5L);
          c++;
        }
      }
      n += this->totalBlockSize;
    }
  }

  this->UGrid->SetCells(cellTypes, locations, outCells);
  locations->Delete();
  outCells->Delete();
  cellTypes->Delete();
} // addPointsToContinuumMesh()

void vtkNek5000Reader::addSpectralElementId(int nelements)
{
  vtkTypeUInt32Array* spectral_id = vtkTypeUInt32Array::New();
  spectral_id->SetNumberOfTuples(nelements);
  spectral_id->SetName("spectral element id");
  int n = 0;
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }

  int start_index = 0;
  for (auto i = 0; i < my_rank; i++)
  {
    start_index += this->proc_numBlocks[i];
  }

  if (this->MeshIs3D)
  {
    for (auto e = start_index; e < start_index + this->myNumBlocks; ++e)
    {
      for (auto ii = 0; ii < this->blockDims[0] - 1; ++ii)
      {
        for (auto jj = 0; jj < this->blockDims[1] - 1; ++jj)
        {
          for (auto kk = 0; kk < this->blockDims[2] - 1; ++kk)
          {
            spectral_id->SetTuple1(n++, e);
          }
        }
      }
    }
  }
  else // 2D
  {
    for (auto e = start_index; e < start_index + this->myNumBlocks; ++e)
    {
      for (auto ii = 0; ii < this->blockDims[0] - 1; ++ii)
      {
        for (auto jj = 0; jj < this->blockDims[1] - 1; ++jj)
        {
          spectral_id->SetTuple1(n++, e);
        }
      }
    }
  }
  this->UGrid->GetCellData()->AddArray(spectral_id);
  spectral_id->Delete();
} // addSpectralElementId()

void vtkNek5000Reader::copyContinuumPoints(vtkPoints* points)
{
  int index = 0;
  // for each element/block in the continuum mesh
  for (auto k = 0; k < this->myNumBlocks; ++k)
  {
    int block_offset = k * this->totalBlockSize * 3; // 3 is for X,Y,Z coordinate components
    // for every point in this element/block
    for (auto i = 0; i < this->totalBlockSize; ++i)
    { /*
       std::cerr<< index << ": " <<
                   this->meshCoords[block_offset+i] << ", " <<
                   this->meshCoords[block_offset+this->totalBlockSize+i] << ", " <<
                   this->meshCoords[block_offset+this->totalBlockSize+this->totalBlockSize+i] <<
       std::endl;
                   */
      points->InsertPoint(index,
        this->meshCoords[block_offset + i],                                                // X val
        this->meshCoords[block_offset + this->totalBlockSize + i],                         // Y val
        this->meshCoords[block_offset + this->totalBlockSize + this->totalBlockSize + i]); // Z val
      index++;
    }
  }
  delete[] this->meshCoords;
}

void vtkNek5000Reader::copyContinuumData(vtkUnstructuredGrid* pv_ugrid)
{
#ifndef NDEBUG
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }
#endif
  int index = 0;
  int num_verts = this->myNumBlocks * this->totalBlockSize;

  // for each variable
  for (auto v_index = 0; v_index < this->num_vars; v_index++)
  {
    if (this->GetPointArrayStatus(v_index))
    {
      // if this is a scalar
      if (this->var_length[v_index] == 1)
      {
        index = 0;
        vtkFloatArray* scalars = vtkFloatArray::New();
        // scalars->SetNumberOfComponents(1);
        // scalars->SetNumberOfTuples(num_verts);
        scalars->SetName(this->var_names[v_index]);
        scalars->SetArray(
          this->dataArray[v_index], num_verts, 0, vtkDataArray::VTK_DATA_ARRAY_DELETE);
        /*
        for (int b_index = 0; b_index < this->myNumBlocks; ++b_index)
        {
          for (int p_index = 0; p_index < this->totalBlockSize; ++p_index)
          {
            scalars->SetValue(index, this->dataArray[v_index][index]);
            index++;
          }
        }*/
        this->UGrid->GetPointData()->AddArray(scalars);
        scalars->Delete();
      }
      // if this is a vector
      else if (this->var_length[v_index] > 1)
      {
        index = 0;
        vtkFloatArray* vectors = vtkFloatArray::New();
        vectors->SetNumberOfComponents(3);
        vectors->SetNumberOfTuples(num_verts);
        vectors->SetName(this->var_names[v_index]);

        // for each  element/block in the continuum mesh
        for (int b_index = 0; b_index < this->myNumBlocks; ++b_index)
        {
          // for every point in this element/block
#ifndef NDEBUG
          cerr << "rank= " << my_rank << " : b_index= " << b_index << endl;
#endif
          int mag_block_offset = b_index * this->totalBlockSize;
          int comp_block_offset = mag_block_offset * 3;

          for (int p_index = 0; p_index < this->totalBlockSize; ++p_index)
          {
            float vxyz[3];
            vxyz[0] = this->dataArray[v_index][comp_block_offset + p_index];
            vxyz[1] = this->dataArray[v_index][comp_block_offset + p_index + this->totalBlockSize];
            vxyz[2] = this->dataArray[v_index][comp_block_offset + p_index + this->totalBlockSize +
              this->totalBlockSize];
            vectors->SetTypedTuple(index, vxyz);
            index++;
          }
        }
        this->UGrid->GetPointData()->AddArray(vectors);
        vectors->Delete();
        delete[] this->dataArray[v_index];
      }
      //
      // std::cerr << __LINE__ << " deleting this->dataArray[" << v_index << "]\n";

    } // if(this->use_variable[v_index])
    else
    {
      // remove array if present, it is not needed
      if (pv_ugrid->GetPointData()->GetArray(this->var_names[v_index]) != nullptr)
      {
        pv_ugrid->GetPointData()->RemoveArray(this->var_names[v_index]);
      }
      // Do I already have this array?  If so, remove it.
      if (this->UGrid->GetPointData()->GetArray(this->var_names[v_index]) != nullptr)
      {
        this->UGrid->GetPointData()->RemoveArray(this->var_names[v_index]);
      }
    }
  }
} // vtkNek5000Reader::copyContinuumData()

// see if the current object is missing data that was requested
// return true if it is, otherwise false
bool vtkNek5000Reader::isObjectMissingData()
{
  // check the stored variables
  for (int i = 0; i < this->num_vars; i++)
  {
    if (this->GetPointArrayStatus(i) == 1 && !this->curObj->vars[i])
    {
      return (true);
    }
  }

  return (false);
} // vtkNek5000Reader::isObjectMissingData()

bool vtkNek5000Reader::objectMatchesRequest()
{
  // see if the current object matches the requested data
  // return false if it does not match, otherwise true
  for (int i = 0; i < this->num_vars; i++)
  {
    if (this->GetPointArrayStatus(i) != this->curObj->vars[i])
    {
      return (false);
    }
  }
  return (true);
} // vtkNek5000Reader::objectMatchesRequest()

bool vtkNek5000Reader::objectHasExtraData()
{
  // see if the current object has extra data than was requested
  // return false if object has less than request, otherwise true
#ifndef NDEBUG
  int my_rank;
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    my_rank = ctrl->GetLocalProcessId();
  }
  else
  {
    my_rank = 0;
  }
#endif
  // check the stored variables, if it was requested, but it is not in the current object, return
  // false
  for (int i = 0; i < this->num_vars; i++)
  {
    if (this->GetPointArrayStatus(i) && !this->curObj->vars[i])
    {
      return (false);
    }
  }

  vtkDebugMacro(<< "objectHasExtraData(): my_rank= " << my_rank << " : returning true");
  return (true);
} // vtkNek5000Reader::objectHasExtraData()

int vtkNek5000Reader::CanReadFile(const char* fname)
{
  FILE* fp;
  if ((fp = vtksys::SystemTools::Fopen(fname, "r")) == nullptr)
  {
    return 0;
  }
  else
    return 1;
} // vtkNek5000Reader::CanReadFile()

vtkNek5000Reader::nek5KObject::nek5KObject()
{
  this->ugrid = nullptr;
  this->vorticity = false;
  this->lambda_2 = false;

  for (int ii = 0; ii < MAX_VARS; ii++)
  {
    this->vars[ii] = false;
  }

  this->index = 0;
  this->prev = nullptr;
  this->next = nullptr;
  this->dataFilename = nullptr;
}

vtkNek5000Reader::nek5KObject::~nek5KObject()
{
  if (this->ugrid)
    this->ugrid->Delete();
  if (this->dataFilename)
  {
    free(this->dataFilename);
    this->dataFilename = nullptr;
  }
}

void vtkNek5000Reader::nek5KObject::reset()
{
  this->vorticity = false;
  this->lambda_2 = false;

  for (int ii = 0; ii < MAX_VARS; ii++)
  {
    this->vars[ii] = false;
  }
  this->index = 0;

  if (this->ugrid)
  {
    this->ugrid->Delete();
    this->ugrid = nullptr;
  }

  if (this->dataFilename)
  {
    free(this->dataFilename);
    this->dataFilename = nullptr;
  }
}

void vtkNek5000Reader::nek5KObject::setDataFilename(char* filename)
{
  if (this->dataFilename)
  {
    free(this->dataFilename);
  }
  this->dataFilename = strdup(filename);
}

vtkNek5000Reader::nek5KList::nek5KList()
{
  this->head = nullptr;
  this->tail = nullptr;
  this->max_count = 10;
  this->cur_count = 0;
}

vtkNek5000Reader::nek5KList::~nek5KList()
{
  int new_cnt = 0;
  nek5KObject* obj = this->head;
  while (obj && new_cnt < this->cur_count)
  {
    this->head = this->head->next;
    delete obj;
    obj = this->head;
    new_cnt++;
  }
}

vtkNek5000Reader::nek5KObject* vtkNek5000Reader::nek5KList::getObject(int id)
{
  nek5KObject* obj = this->head;
  while (obj)
  {
    if (obj->index == id) // if we found it
    {
      // move found obj to tail of the list
      // if already tail, do nothing
      if (obj == this->tail)
        break;

      // if it's the head, update head to next
      if (obj == this->head)
      {
        this->head = this->head->next;
      }
      // now move obj to tail
      obj->next->prev = obj->prev;
      if (obj->prev) // i.e. if current was not the head
      {
        obj->prev->next = obj->next;
      }
      this->tail->next = obj;
      obj->prev = this->tail;
      obj->next = nullptr;
      this->tail = obj;
      break;
    }
    else // otherwise, lok at the next one
    {
      obj = obj->next;
    }
  }

  // if we didn't find it
  if (obj == nullptr)
  {
    // if we are not over allocated,
    // create a new object, and put it at the tail
    if (this->cur_count < this->max_count)
    {
      this->cur_count++;
      // obj = nek5KObject::New();
      obj = new nek5KObject();
      if (this->head == nullptr) // if list is empty
      {
        this->head = obj;
        this->tail = obj;
      }
      else
      {
        this->tail->next = obj;
        obj->prev = this->tail;
        obj->next = nullptr;
        this->tail = obj;
      }
      // set the index to the one requested
      obj->index = id;
    }
    else // otherwise reuse oldest obj (head), reset and move to tail
    {
      obj = this->head;
      this->head = this->head->next;
      this->head->prev = nullptr;

      this->tail->next = obj;
      obj->prev = this->tail;
      obj->next = nullptr;
      this->tail = obj;
      obj->reset();
      obj->index = id;
    }
  }
  return (obj);
}
