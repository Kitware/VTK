#include "PIOAdaptor.h"
#include "BHTree.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"

#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

#include <float.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vtksys/FStream.hxx>

#ifdef _WIN32
const static char* Slash = "\\/";
#else
const static char* Slash = "/";
#endif

namespace
{
bool sort_desc(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
  return (a.first > b.first);
}

void BroadcastString(vtkMultiProcessController* controller, std::string& str, int rank)
{
  unsigned long len = static_cast<unsigned long>(str.size()) + 1;
  controller->Broadcast(&len, 1, 0);
  if (len)
  {
    if (rank)
    {
      std::vector<char> tmp;
      tmp.resize(len);
      controller->Broadcast(&(tmp[0]), len, 0);
      str = &tmp[0];
    }
    else
    {
      const char* start = str.c_str();
      std::vector<char> tmp(start, start + len);
      controller->Broadcast(&tmp[0], len, 0);
    }
  }
}

void BroadcastStringVector(
  vtkMultiProcessController* controller, std::vector<std::string>& svec, int rank)
{
  unsigned long len = static_cast<unsigned long>(svec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
    svec.resize(len);
  std::vector<std::string>::iterator it;
  for (it = svec.begin(); it != svec.end(); ++it)
  {
    BroadcastString(controller, *it, rank);
  }
}

void BroadcastStringList(
  vtkMultiProcessController* controller, std::list<std::string>& slist, int rank)
{
  unsigned long len = static_cast<unsigned long>(slist.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
    slist.resize(len);
  std::list<std::string>::iterator it;
  for (it = slist.begin(); it != slist.end(); ++it)
  {
    BroadcastString(controller, *it, rank);
  }
}

void BroadcastDoubleVector(
  vtkMultiProcessController* controller, std::vector<double>& dvec, int rank)
{
  unsigned long len = static_cast<unsigned long>(dvec.size());
  controller->Broadcast(&len, 1, 0);
  if (rank)
  {
    dvec.resize(len);
  }
  if (len)
  {
    controller->Broadcast(&dvec[0], len, 0);
  }
}
};

struct PIOAdaptor::AdaptorImpl
{
  // Global size information
  int dimension = 0;
  int numberOfDaughters = 0;
  unsigned int gridSize[3];
  double gridOrigin[3];
  double gridScale[3];
  double minLoc[3];
  double maxLoc[3];

  // Global geometry information from dump file
  // Used on geometry and variable data for selection
  std::valarray<int64_t> daughter;

  // Used in load balancing of unstructured grid
  int* startCell;
  int* endCell;
  int* countCell;

  // mpi tag
  const int mpiTag = 2564961;
};

///////////////////////////////////////////////////////////////////////////////
//
// Constructor and destructor
//
///////////////////////////////////////////////////////////////////////////////

PIOAdaptor::PIOAdaptor(vtkMultiProcessController* ctrl)
  : useHTG(false)
  , useTracer(false)
  , useFloat64(false)
  , hasTracers(false)
  , Impl(new AdaptorImpl)
{
  this->Controller = ctrl;
  if (this->Controller)
  {
    this->Rank = this->Controller->GetLocalProcessId();
    this->TotalRank = this->Controller->GetNumberOfProcesses();
  }
  else
  {
    this->Rank = 0;
    this->TotalRank = 1;
  }
  this->pioData = nullptr;

  // For load balancing in unstructured grid
  this->Impl->startCell = new int[this->TotalRank];
  this->Impl->endCell = new int[this->TotalRank];
  this->Impl->countCell = new int[this->TotalRank];
}

PIOAdaptor::~PIOAdaptor()
{
  delete this->pioData;
  this->pioData = nullptr;
  this->Controller = nullptr;
  delete[] this->Impl->startCell;
  delete[] this->Impl->endCell;
  delete[] this->Impl->countCell;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read descriptor file, collect meta data on proc 0, share with other procs
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::initializeGlobal(const char* PIOFileName)
{
  int success;
  if (this->Rank == 0)
  {
    success = collectMetaData(PIOFileName);
    this->Controller->Broadcast(&success, 1, 0);
  }
  else
  {
    this->Controller->Broadcast(&success, 1, 0);
  }

  if (!success)
  {
    return 0;
  }

  // Share with all processors
  BroadcastStringVector(this->Controller, this->dumpFileName, this->Rank);
  BroadcastStringVector(this->Controller, this->variableName, this->Rank);
  BroadcastStringVector(this->Controller, this->variableDefault, this->Rank);
  BroadcastStringList(this->Controller, this->fieldsToRead, this->Rank);
  BroadcastDoubleVector(this->Controller, this->CycleIndex, this->Rank);
  BroadcastDoubleVector(this->Controller, this->SimulationTime, this->Rank);
  BroadcastDoubleVector(this->Controller, this->PIOFileIndex, this->Rank);

  int tmp = static_cast<int>(this->useHTG);
  this->Controller->Broadcast(&tmp, 1, 0);
  this->useHTG = static_cast<bool>(tmp);
  tmp = static_cast<int>(this->useTracer);
  this->Controller->Broadcast(&tmp, 1, 0);
  this->useTracer = static_cast<bool>(tmp);
  tmp = static_cast<int>(this->useFloat64);
  this->Controller->Broadcast(&tmp, 1, 0);
  this->useFloat64 = static_cast<bool>(tmp);
  tmp = static_cast<int>(this->hasTracers);
  this->Controller->Broadcast(&tmp, 1, 0);
  this->hasTracers = static_cast<bool>(tmp);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read the global descriptor file (name.pio) collecting dump directory info
// Read dump file to collect variable names, cycle indices, simulation times
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::collectMetaData(const char* PIOFileName)
{
  // Parse descriptor file collecting dump directory, base name, and
  // booleans indicating type structure to build, data precision and tracers
  if (parsePIOFile(PIOFileName) == 0)
  {
    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // Using the dump directories and base name, scan for all dump files
  //
  vtkNew<vtkDirectory> directory;
  uint64_t numFiles = 0;
  std::map<long, std::string> fileMap;
  std::map<long, std::string>::iterator miter;

  std::vector<int> cycleIndex;
  std::vector<double> simulationTime;
  std::vector<std::string> fileNames;

  for (size_t dir = 0; dir < this->dumpDirectory.size(); dir++)
  {
    if (directory->Open(this->dumpDirectory[dir].c_str()) == false)
    {
      vtkGenericWarningMacro("Dump directory does not exist: " << this->dumpDirectory[dir]);
    }
    else
    {
      numFiles = directory->GetNumberOfFiles();
      uint64_t numDumps = 0;
      for (unsigned int i = 0; i < numFiles; i++)
      {
        // check if fileName starts with base name
        std::string fileName = directory->GetFile(i);
        std::size_t matchPos = fileName.find(this->dumpBaseName);
        if (matchPos == 0)
        {
          // try to open it and see if it is a valid pio file
          std::ostringstream tmpStr;
          tmpStr << this->dumpDirectory[dir] << Slash << fileName;
          PIO_DATA* pioData = new PIO_DATA(tmpStr.str().c_str());
          if (pioData->good_read())
          {
            // Collect metadata of dump file
            // cycle number is the first integer in the controller_i array
            // simulation time is the first double in the controller_r8 array
            // Note: cannot use hist_cycle and hist_time because even/odd dumps
            // will not have the correct values
            std::valarray<int> controller_i;
            std::valarray<double> controller_r8;
            pioData->set_scalar_field(controller_i, "controller_i");
            pioData->set_scalar_field(controller_r8, "controller_r8");
            cycleIndex.emplace_back(controller_i[0]);
            simulationTime.emplace_back(controller_r8[0]);
            fileNames.emplace_back(tmpStr.str());
            numDumps++;
          }
          delete pioData;
        }
      }
      if (numDumps == 0)
      {
        // get the original base name for the warning message
        std::string basename = this->dumpBaseName;
        std::string::size_type pos = basename.find("-dmp");
        if (pos != std::string::npos)
        {
          basename = basename.substr(0, pos);
        }
        vtkGenericWarningMacro("No files exist with the base name: '"
          << basename << "' in the dump directory: " << this->dumpDirectory[dir]);
      }
    }
  }

  if (cycleIndex.empty())
  {
    // no dump files were found
    return 0;
  }
  else
  {
    // at least one dump file was found.
    // sort information by cycle number, and add information to permanent arrays.
    // create an array of indices, and sort the indices array. then we can use
    // the indices array to sort other metadata in the same order.
    class sort_indices
    {
    private:
      std::vector<int> mparr;

    public:
      sort_indices(std::vector<int> parr)
        : mparr(parr)
      {
      }
      bool operator()(int i, int j) const { return mparr[i] < mparr[j]; }
    };

    int numDumps = (int)cycleIndex.size();
    std::vector<int> indices(numDumps);
    for (int i = 0; i < numDumps; i++)
    {
      indices[i] = i;
    }

    std::sort(indices.begin(), indices.end(), sort_indices(cycleIndex));

    for (int i = 0; i < numDumps; i++)
    {
      this->CycleIndex.emplace_back(static_cast<double>(cycleIndex[indices[i]]));
      this->SimulationTime.emplace_back(simulationTime[indices[i]]);
      this->dumpFileName.emplace_back(fileNames[indices[i]]);
      this->PIOFileIndex.emplace_back(static_cast<double>(i));
    }

    // this needs to be set for later functions to use
    this->pioData = new PIO_DATA(this->dumpFileName.back().c_str());

    // collect rest of metadata
    collectVariableMetaData();
  }

  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Remove whitespace from the beginning and end of a string
//
///////////////////////////////////////////////////////////////////////////////

std::string PIOAdaptor::trimString(const std::string& str)
{
  std::string whitespace = " \n\r\t\f\v";
  size_t start = str.find_first_not_of(whitespace);
  size_t end = str.find_last_not_of(whitespace);
  if (start == std::string::npos || end == std::string::npos)
  {
    // the whole line is whitespace
    return std::string("");
  }
  else
  {
    return str.substr(start, end - start + 1);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Read the global descriptor file (name.pio)
//
// DUMP_BASE_NAME base        (Required)
// DUMP_DIRECTORY dumps0      (Defaults to "." if missing)
// DUMP_DIRECTORY dumps1
// DUMP_DIRECTORY dumpsN
//
// MAKE_HTG YES    (Default NO) means create unstructured grid
// MAKE_TRACER NO  (Default NO) means don't create unstructured grid of particles
// FLOAT64 YES     (Default NO) means use 32 bit float for data
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::parsePIOFile(const char* PIOFileName)
{
  this->descFileName = PIOFileName;
  vtksys::ifstream pioStr(this->descFileName.c_str());
  if (!pioStr)
  {
    vtkGenericWarningMacro("Could not open the global description .pio file: " << PIOFileName);
    return 0;
  }

  // Get the directory name from the full path of the .pio file in GUI
  // or simple name from python script
  std::string::size_type dirPos = this->descFileName.find_last_of(Slash);
  std::string dirName;
  if (dirPos == std::string::npos)
  {
    // No directory name included
    std::ostringstream tempStr;
    tempStr << "." << Slash;
    dirName = tempStr.str();
  }
  else
  {
    // Directory name before final slash
    dirName = this->descFileName.substr(0, dirPos);
  }

  // Either .pio file or an actual basename-dmp000000 to guide open file
  // Opening actual dump file requires asking for All files and picking PIOReader
  // Opening a pio suffix file defaults to the correct action
  std::string::size_type pos = this->descFileName.rfind('.');
  std::string suffix = this->descFileName.substr(pos + 1);
  if (suffix == "pio")
  {
    // Parse the pio input file
    char inBuf[256];
    std::string rest;
    std::string keyword;
    this->useHTG = false;
    this->useTracer = false;
    this->useFloat64 = false;
    this->hasTracers = false;

    while (pioStr.getline(inBuf, 256))
    {
      std::string localline(inBuf);
      localline = trimString(localline);
      if (localline.length() > 0)
      {
        if (localline[0] != '#' && localline[0] != '!')
        {
          // Remove quotes from input
          localline.erase(std::remove(localline.begin(), localline.end(), '\"'), localline.end());
          localline.erase(std::remove(localline.begin(), localline.end(), '\''), localline.end());

          // check for comments in the middle of the line
          std::string::size_type poundPos = localline.find('#');
          if (poundPos != std::string::npos)
          {
            localline = localline.substr(0, poundPos);
          }

          std::string::size_type bangPos = localline.find('!');
          if (bangPos != std::string::npos)
          {
            localline = localline.substr(0, bangPos);
          }

          std::string::size_type keyPos = localline.find(' ');
          keyword = trimString(localline.substr(0, keyPos));
          rest = trimString(localline.substr(keyPos + 1));

          if (keyword == "DUMP_DIRECTORY")
          {
            if (rest[0] == '/')
            {
              // If a full path is given use it
              this->dumpDirectory.push_back(rest);
            }
            else
            {
              // If partial path append to the dir of the .pio file
              std::ostringstream tempStr;
              tempStr << dirName << Slash << rest;
              this->dumpDirectory.push_back(tempStr.str());
            }
          }
          if (keyword == "DUMP_BASE_NAME")
          {
            std::ostringstream tempStr;
            tempStr << rest << "-dmp";
            this->dumpBaseName = tempStr.str();
          }
          if (keyword == "MAKE_HTG")
          {
            if (rest == "YES")
              this->useHTG = true;
          }
          if (keyword == "MAKE_TRACER")
          {
            if (rest == "YES")
              this->useTracer = true;
          }
          if (keyword == "FLOAT64")
          {
            if (rest == "YES")
              this->useFloat64 = true;
          }
        }
      }
    }
    if (this->dumpDirectory.empty())
    {
      this->dumpDirectory.push_back(dirName);
    }
  }
  else
  {
    //
    // Use the basename-dmp000000 file to discern the info that is in the pio file
    //
    std::string::size_type pos1 = this->descFileName.rfind(Slash);
    std::string::size_type pos2 = this->descFileName.find("-dmp");
    this->dumpBaseName = this->descFileName.substr(pos1 + 1, pos2 - pos1 + 3);
    this->dumpDirectory.push_back(this->descFileName.substr(0, pos1));
    this->useHTG = false;
    this->useTracer = false;
    this->useFloat64 = false;
    this->hasTracers = false;
  }
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read the variable meta data from first pio dump file
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::collectVariableMetaData()
{
  std::valarray<int> histsize;
  this->pioData->set_scalar_field(histsize, "hist_size");
  int numberOfCells = histsize[histsize.size() - 1];
  int numberOfFields = this->pioData->get_pio_num();
  PIO_FIELD* pioField = this->pioData->get_pio_field();

  for (int i = 0; i < numberOfFields; i++)
  {
    // Are tracers available in file
    char* pioName = pioField[i].pio_name;
    if (strcmp(pioName, "tracer_num_pnts") == 0)
    {
      this->hasTracers = true;
    }

    // Default variable names that are initially enabled for loading if pres
    if ((strcmp(pioName, "tev") == 0) || (strcmp(pioName, "pres") == 0) ||
      (strcmp(pioName, "rho") == 0) || (strcmp(pioName, "rade") == 0) ||
      (strcmp(pioName, "cell_energy") == 0) || (strcmp(pioName, "kemax") == 0) ||
      (strcmp(pioName, "vel") == 0) || (strcmp(pioName, "eng") == 0))
    {
      this->variableDefault.emplace_back(pioName);
    }

    if (pioField[i].length == numberOfCells && pioField[i].cdata_len == 0)
    {
      // index = 0 is scalar, index = 1 is vector, index = -1 is request from input deck
      int index = pioField[i].index;
      if (index == 0 || index == 1 || index == -1)
      {
        // Discard names used in geometry and variables with too many components
        // which are present for use in tracers
        size_t numberOfComponents = this->pioData->VarMMap.count(pioName);

        if ((numberOfComponents <= 9) && (strcmp(pioName, "cell_has_tracers") != 0) &&
          (strcmp(pioName, "cell_level") != 0) && (strcmp(pioName, "cell_mother") != 0) &&
          (strcmp(pioName, "cell_daughter") != 0) && (strcmp(pioName, "cell_center") != 0) &&
          (strcmp(pioName, "cell_active") != 0) && (strcmp(pioName, "amr_tag") != 0))
        {
          this->variableName.emplace_back(pioName);
        }
      }
    }
  }
  sort(this->variableName.begin(), this->variableName.end());

  //
  // List of all data fields to read from dump files
  //
  this->fieldsToRead.emplace_back("amhc_i");
  this->fieldsToRead.emplace_back("amhc_r8");
  this->fieldsToRead.emplace_back("amhc_l");
  this->fieldsToRead.emplace_back("cell_center");
  this->fieldsToRead.emplace_back("cell_daughter");
  this->fieldsToRead.emplace_back("cell_level");
  this->fieldsToRead.emplace_back("global_numcell");
  this->fieldsToRead.emplace_back("hist_cycle");
  this->fieldsToRead.emplace_back("hist_time");
  this->fieldsToRead.emplace_back("hist_size");
  this->fieldsToRead.emplace_back("l_eap_version");
  this->fieldsToRead.emplace_back("hist_usernm");
  this->fieldsToRead.emplace_back("hist_prbnm");
  this->fieldsToRead.emplace_back("controller_i");
  this->fieldsToRead.emplace_back("controller_r8");

  // If tracers are contained in the file
  if (this->hasTracers == true)
  {
    this->fieldsToRead.emplace_back("tracer_num_pnts");
    this->fieldsToRead.emplace_back("tracer_num_vars");
    this->fieldsToRead.emplace_back("tracer_record_count");
    this->fieldsToRead.emplace_back("tracer_type");
    this->fieldsToRead.emplace_back("tracer_position");
    this->fieldsToRead.emplace_back("tracer_data");
  }

  // Requested variable fields from pio meta file
  for (size_t i = 0; i < this->variableName.size(); i++)
  {
    this->fieldsToRead.push_back(this->variableName[i]);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialize with the *.pio file giving the name of the dump file, whether to
// create hypertree grid or unstructured grid, and variables to read
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::initializeDump(int timeStep)
{
  // Open file and read metadata on proc 0, broadcast
  if (this->Rank == 0)
  {
    // Start with a fresh pioData initialized for this time step
    if (this->pioData != nullptr)
    {
      delete this->pioData;
      this->pioData = nullptr;
    }

    // Create one PIOData which accesses the PIO file to fetch data
    if (this->pioData == nullptr)
    {
      this->pioData = new PIO_DATA(this->dumpFileName[timeStep].c_str(), &this->fieldsToRead);
      if (this->pioData->good_read())
      {
        // First collect the sizes of the domains
        const double* amhc_i = this->pioData->GetPIOData("amhc_i");
        const double* amhc_r8 = this->pioData->GetPIOData("amhc_r8");
        const double* amhc_l = this->pioData->GetPIOData("amhc_l");

        if (amhc_i != nullptr && amhc_r8 != nullptr && amhc_l != nullptr)
        {
          this->Impl->dimension = uint32_t(amhc_i[Nnumdim]);
          this->Impl->numberOfDaughters = (int)pow(2.0, this->Impl->dimension);

          // Save sizes for use in creating structures
          for (int i = 0; i < 3; i++)
          {
            this->Impl->gridOrigin[i] = 0.0;
            this->Impl->gridScale[i] = 0.0;
            this->Impl->gridSize[i] = 0;
          }
          this->Impl->gridOrigin[0] = amhc_r8[NZero0];
          this->Impl->gridScale[0] = amhc_r8[Nd0];
          this->Impl->gridSize[0] = static_cast<int>(amhc_i[Nmesh0]);

          if (this->Impl->dimension > 1)
          {
            this->Impl->gridOrigin[1] = amhc_r8[NZero1];
            this->Impl->gridScale[1] = amhc_r8[Nd1];
            this->Impl->gridSize[1] = static_cast<int>(amhc_i[Nmesh1]);
          }
          if (this->Impl->dimension > 2)
          {
            this->Impl->gridOrigin[2] = amhc_r8[NZero2];
            this->Impl->gridScale[2] = amhc_r8[Nd2];
            this->Impl->gridSize[2] = static_cast<int>(amhc_i[Nmesh2]);
          }
        }
      }
      else
      {
        vtkGenericWarningMacro("PIOFile " << this->dumpFileName[timeStep] << " can't be read ");
        return 0;
      }
    }
    // Needed for the BHTree and locating level 1 cells for hypertree
    for (int i = 0; i < 3; i++)
    {
      this->Impl->minLoc[i] = this->Impl->gridOrigin[i];
      this->Impl->maxLoc[i] =
        this->Impl->minLoc[i] + (this->Impl->gridSize[i] * this->Impl->gridScale[i]);
    }
  }

  // Broadcast the metadata
  this->Controller->Broadcast(&this->Impl->dimension, 1, 0);
  this->Controller->Broadcast(&this->Impl->numberOfDaughters, 1, 0);
  this->Controller->Broadcast(this->Impl->gridSize, 3, 0);
  this->Controller->Broadcast(this->Impl->gridOrigin, 3, 0);
  this->Controller->Broadcast(this->Impl->gridScale, 3, 0);
  this->Controller->Broadcast(this->Impl->minLoc, 3, 0);
  this->Controller->Broadcast(this->Impl->maxLoc, 3, 0);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Create the geometry for either unstructured or hypertree grid using sizes
// already collected and the dump file geometry and load balancing information
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_geometry(vtkMultiBlockDataSet* grid)
{
  // Create Blocks in the grid as requested (unstructured, hypertree, tracer)
  grid->SetNumberOfBlocks(1);
  if (this->useHTG == false)
  {
    // Create a multipiece dataset and an unstructured grid to hold the dump file data
    vtkNew<vtkMultiPieceDataSet> multipiece;
    multipiece->SetNumberOfPieces(this->TotalRank);

    vtkNew<vtkUnstructuredGrid> ugrid;
    ugrid->Initialize();

    multipiece->SetPiece(this->Rank, ugrid);
    grid->SetBlock(0, multipiece);
    grid->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(), "AMR Grid");
  }
  else
  {
    // Create a multipiece dataset and hypertree grid to hold the dump file data
    vtkNew<vtkMultiPieceDataSet> multipiece;
    multipiece->SetNumberOfPieces(this->TotalRank);

    vtkNew<vtkHyperTreeGrid> htgrid;
    htgrid->Initialize();

    multipiece->SetPiece(this->Rank, htgrid);
    grid->SetBlock(0, multipiece);
    grid->GetMetaData((unsigned int)0)->Set(vtkCompositeDataSet::NAME(), "AMR Grid");
  }

  // If tracers are used add a second block of unstructured grid particles
  if (this->hasTracers == true && this->useTracer == true)
  {
    vtkNew<vtkMultiPieceDataSet> multipiece;
    multipiece->SetNumberOfPieces(this->TotalRank);

    vtkNew<vtkUnstructuredGrid> tgrid;
    tgrid->Initialize();

    multipiece->SetPiece(this->Rank, tgrid);
    grid->SetNumberOfBlocks(2);
    grid->SetBlock(1, multipiece);
    grid->GetMetaData((unsigned int)1)->Set(vtkCompositeDataSet::NAME(), "Tracers");
  }

  // Create the VTK structures within multiblock
  if (this->useHTG == true)
  {
    create_amr_HTG(grid);
  }
  else
  {
    create_amr_UG(grid);
  }

  // Create Tracer Unstructured if tracers exist
  if (this->useTracer == true)
  {
    if (this->hasTracers == true)
    {
      if (this->Rank == 0)
      {
        create_tracer_UG(grid);
      }
    }
    else
    {
      vtkGenericWarningMacro("Tracers don't exist in .pio file: " << this->descFileName);
    }
  }

  // Collect other information from PIOData
  double currentCycle;
  double currentTime;
  double currentIndex;
  vtkStdString eap_version;
  vtkStdString user_name;
  vtkStdString problem_name;

  if (this->Rank == 0)
  {
    const char* cdata;
    this->pioData->GetPIOData("l_eap_version", cdata);
    eap_version = cdata;

    this->pioData->GetPIOData("hist_usernm", cdata);
    user_name = cdata;

    this->pioData->GetPIOData("hist_prbnm", cdata);
    problem_name = cdata;

    std::valarray<int> controller_i;
    std::valarray<double> controller_r8;
    this->pioData->set_scalar_field(controller_i, "controller_i");
    this->pioData->set_scalar_field(controller_r8, "controller_r8");

    currentCycle = static_cast<double>(controller_i[0]);
    currentTime = controller_r8[0];

    // find the current index by searching for currentCycle in CycleIndex
    std::vector<double>::iterator it =
      std::find(this->CycleIndex.begin(), this->CycleIndex.end(), currentCycle);
    currentIndex = static_cast<double>(std::distance(this->CycleIndex.begin(), it));
  }

  // Share information
  BroadcastString(this->Controller, eap_version, 0);
  BroadcastString(this->Controller, user_name, 0);
  BroadcastString(this->Controller, problem_name, 0);
  this->Controller->Broadcast(&currentCycle, 1, 0);
  this->Controller->Broadcast(&currentTime, 1, 0);
  this->Controller->Broadcast(&currentIndex, 1, 0);

  vtkNew<vtkStringArray> dumpFileNameArray;
  dumpFileNameArray->SetName("dump_filename");
  dumpFileNameArray->InsertNextValue(
    vtksys::SystemTools::GetFilenameName(this->dumpFileName[currentIndex]));
  grid->GetFieldData()->AddArray(dumpFileNameArray);

  // Add FieldData array for version number
  vtkNew<vtkStringArray> versionArray;
  versionArray->SetName("eap_version");
  versionArray->InsertNextValue(eap_version);
  grid->GetFieldData()->AddArray(versionArray);

  // Add FieldData array for user name
  vtkNew<vtkStringArray> userNameArray;
  userNameArray->SetName("user_name");
  userNameArray->InsertNextValue(user_name);
  grid->GetFieldData()->AddArray(userNameArray);

  // Add FieldData array for problem name
  vtkNew<vtkStringArray> probNameArray;
  probNameArray->SetName("problem_name");
  probNameArray->InsertNextValue(problem_name);
  grid->GetFieldData()->AddArray(probNameArray);

  // Add FieldData array for cycle number
  vtkNew<vtkDoubleArray> cycleArray;
  cycleArray->SetName("CycleIndex");
  cycleArray->SetNumberOfComponents(1);
  cycleArray->SetNumberOfTuples(1);
  cycleArray->SetTuple1(0, currentCycle);
  grid->GetFieldData()->AddArray(cycleArray);

  // Add FieldData array for simulation time
  vtkNew<vtkDoubleArray> simTimeArray;
  simTimeArray->SetName("SimulationTime");
  simTimeArray->SetNumberOfComponents(1);
  simTimeArray->SetNumberOfTuples(1);
  simTimeArray->SetTuple1(0, currentTime);
  grid->GetFieldData()->AddArray(simTimeArray);

  // Add FieldData array for pio file index
  vtkNew<vtkDoubleArray> pioFileIndexArray;
  pioFileIndexArray->SetName("PIOFileIndex");
  pioFileIndexArray->SetNumberOfComponents(1);
  pioFileIndexArray->SetNumberOfTuples(1);
  pioFileIndexArray->SetTuple1(0, currentIndex);
  grid->GetFieldData()->AddArray(pioFileIndexArray);
}

///////////////////////////////////////////////////////////////////////////////
//
// Build unstructured grid for tracers
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_tracer_UG(vtkMultiBlockDataSet* grid)
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(1));
  vtkUnstructuredGrid* tgrid = vtkUnstructuredGrid::SafeDownCast(multipiece->GetPiece(this->Rank));
  tgrid->Initialize();

  // Get tracer information from PIOData
  std::valarray<int> tracer_num_pnts;
  std::valarray<int> tracer_num_vars;
  std::valarray<int> tracer_record_count;
  std::valarray<std::valarray<double>> tracer_position;
  std::valarray<std::valarray<double>> tracer_data;

  this->pioData->set_scalar_field(tracer_num_pnts, "tracer_num_pnts");
  this->pioData->set_scalar_field(tracer_num_vars, "tracer_num_vars");
  this->pioData->set_scalar_field(tracer_record_count, "tracer_record_count");
  this->pioData->set_vector_field(tracer_position, "tracer_position");
  this->pioData->set_vector_field(tracer_data, "tracer_data");

  int numberOfTracers = tracer_num_pnts[0];
  int numberOfTracerVars = tracer_num_vars[0];
  int numberOfTracerRecords = tracer_record_count[0];
  int lastTracerCycle = numberOfTracerRecords - 1;

  // Names of the tracer variables
  std::vector<std::string> tracer_type(numberOfTracerVars);
  int tracer_name_len = 4;
  const char* cdata;
  PIO_FIELD* pioField = this->pioData->VarMMap.equal_range("tracer_type").first->second;
  this->pioData->GetPIOData(*pioField, cdata);
  size_t cdata_len = pioField->cdata_len * tracer_name_len;

  for (int var = 0; var < numberOfTracerVars; var++)
  {
    tracer_type[var] = cdata + var * cdata_len;
  }

  // For each tracer insert point location and create an unstructured vertex
  vtkNew<vtkPoints> points;
  tgrid->SetPoints(points);
  tgrid->Allocate(numberOfTracers, numberOfTracers);
  vtkIdType cell[1];
  double pointPos[3] = { 0.0, 0.0, 0.0 };

  for (int i = 0; i < numberOfTracers; i++)
  {
    for (int dim = 0; dim < this->Impl->dimension; dim++)
    {
      pointPos[dim] = tracer_position[dim][i];
    }
    points->InsertNextPoint(pointPos[0], pointPos[1], pointPos[2]);
    cell[0] = i;
    tgrid->InsertNextCell(VTK_VERTEX, 1, cell);
  }

  // Add other tracer data which appears by time step, then by tracer, then by variable
  // Variable data starts with cycle time and coordinate[numdim]
  int tracerDataOffset = 1 + this->Impl->dimension;
  if (this->useFloat64)
  {
    std::vector<double*> varData(numberOfTracerVars);
    for (int var = 0; var < numberOfTracerVars; var++)
    {
      vtkNew<vtkDoubleArray> arr;
      arr->SetName(tracer_type[var].c_str());
      arr->SetNumberOfComponents(1);
      arr->SetNumberOfTuples(numberOfTracers);
      varData[var] = arr->GetPointer(0);
      tgrid->GetCellData()->AddArray(arr);
    }
    int index = 0;
    for (int i = 0; i < numberOfTracers; i++)
    {
      index += tracerDataOffset;
      for (int var = 0; var < numberOfTracerVars; var++)
      {
        varData[var][i] = tracer_data[lastTracerCycle][index++];
      }
    }
  }
  else
  {
    std::vector<float*> varData(numberOfTracerVars);
    for (int var = 0; var < numberOfTracerVars; var++)
    {
      vtkNew<vtkFloatArray> arr;
      arr->SetName(tracer_type[var].c_str());
      arr->SetNumberOfComponents(1);
      arr->SetNumberOfTuples(numberOfTracers);
      varData[var] = arr->GetPointer(0);
      tgrid->GetCellData()->AddArray(arr);
    }
    int index = 0;
    for (int i = 0; i < numberOfTracers; i++)
    {
      index += tracerDataOffset;
      for (int var = 0; var < numberOfTracerVars; var++)
      {
        varData[var][i] = (float)tracer_data[lastTracerCycle][index++];
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Build unstructured grid geometry
// Consider dimension and load balancing
// Proc 0 has geometry information for all and calculates distribution
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_UG(vtkMultiBlockDataSet* grid)
{
  // On proc 0 the valarray holds all data
  // On other processors the correct size for the piece is allocated
  std::valarray<int> level;
  std::valarray<std::valarray<double>> center;
  int numberOfCells;
  int64_t* cell_daughter;
  int* cell_level;
  double* cell_center[3];

  // Proc 0 calculates what cells are distributed storing the
  // first index, last index and count going to each other rank
  if (this->Rank == 0)
  {
    // Collect geometry information for distribution schedule
    std::valarray<int> histsize;
    std::valarray<int> numcell;
    this->pioData->set_scalar_field(histsize, "hist_size");
    this->pioData->set_scalar_field(numcell, "global_numcell");

    int* global_numcell = &numcell[0];
    int procsInDump = static_cast<int>(numcell.size());
    std::vector<int> procsPerRank(this->TotalRank);

    // More PIO processors than paraview processors
    if (procsInDump > this->TotalRank)
    {
      for (int rank = 0; rank < this->TotalRank; rank++)
      {
        procsPerRank[rank] = procsInDump / this->TotalRank;
      }
      procsPerRank[0] += (procsInDump % this->TotalRank);
    }
    else
    // Fewer PIO processors than paraview processors so one or none per
    {
      for (int rank = 0; rank < procsInDump; rank++)
      {
        procsPerRank[rank] = 1;
      }
      for (int rank = procsInDump; rank < this->TotalRank; rank++)
      {
        procsPerRank[rank] = 0;
      }
    }

    // Calculate the first and last cell index per rank for redistribution
    int currentCell = 0;
    int globalIndx = 0;
    for (int rank = 0; rank < this->TotalRank; rank++)
    {
      this->Impl->startCell[rank] = currentCell;
      this->Impl->endCell[rank] = currentCell;
      for (int i = 0; i < procsPerRank[rank]; i++)
      {
        this->Impl->endCell[rank] += global_numcell[globalIndx++];
      }
      this->Impl->countCell[rank] = this->Impl->endCell[rank] - this->Impl->startCell[rank];
      currentCell = this->Impl->endCell[rank];
    }

    // Collect the rest of the data for sharing via Send and Receive
    this->pioData->set_scalar_field(this->Impl->daughter, "cell_daughter");
    this->pioData->set_scalar_field(level, "cell_level");
    this->pioData->set_vector_field(center, "cell_center");

    cell_daughter = &this->Impl->daughter[0];
    cell_level = &level[0];
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      cell_center[d] = &center[d][0];
    };

    numberOfCells = this->Impl->countCell[0];
    for (int rank = 1; rank < this->TotalRank; rank++)
    {
      this->Controller->Send(&this->Impl->countCell[rank], 1, rank, this->Impl->mpiTag);
      this->Controller->Send(&cell_level[this->Impl->startCell[rank]], this->Impl->countCell[rank],
        rank, this->Impl->mpiTag);
      this->Controller->Send(&cell_daughter[this->Impl->startCell[rank]],
        this->Impl->countCell[rank], rank, this->Impl->mpiTag);
      for (int d = 0; d < this->Impl->dimension; d++)
      {
        this->Controller->Send(&cell_center[d][this->Impl->startCell[rank]],
          this->Impl->countCell[rank], rank, this->Impl->mpiTag);
      }
    }
  }
  else
  {
    this->Controller->Receive(&numberOfCells, 1, 0, this->Impl->mpiTag);

    // Allocate space for holding geometry information
    cell_level = new int[numberOfCells];
    cell_daughter = new int64_t[numberOfCells];
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      cell_center[d] = new double[numberOfCells];
    }

    this->Controller->Receive(cell_level, numberOfCells, 0, this->Impl->mpiTag);
    this->Controller->Receive(cell_daughter, numberOfCells, 0, this->Impl->mpiTag);
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      this->Controller->Receive(cell_center[d], numberOfCells, 0, this->Impl->mpiTag);
    }

    // Copy the daughter into the namespace valarray so it looks like proc 0
    // It is the only one that must be saved because load_variables use it
    this->Impl->daughter.resize(numberOfCells);
    for (int i = 0; i < numberOfCells; i++)
    {
      this->Impl->daughter[i] = cell_daughter[i];
    }
  }
  // Based on dimension and cell range build the unstructured grid pieces
  // Called for all processors
  if (this->Impl->dimension == 1)
  {
    create_amr_UG_1D(grid, numberOfCells, cell_level, cell_daughter, cell_center);
  }
  else if (this->Impl->dimension == 2)
  {
    create_amr_UG_2D(grid, numberOfCells, cell_level, cell_daughter, cell_center);
  }
  else
  {
    create_amr_UG_3D(grid, numberOfCells, cell_level, cell_daughter, cell_center);
  }
  // Only delete space allocated by receiving processors
  if (this->Rank > 0)
  {
    delete[] cell_level;
    delete[] cell_daughter;
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      delete[] cell_center[d];
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// Build 1D geometry of line cells
// Geometry is created for each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_UG_1D(vtkMultiBlockDataSet* grid,
  int numberOfCells,      // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double* cell_center[1]) // Cell center
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(0));
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(multipiece->GetPiece(this->Rank));
  ugrid->Initialize();

  // Get count of cells which will be created for allocation
  int numberOfActiveCells = 0;
  for (int cell = 0; cell < numberOfCells; cell++)
    if (cell_daughter[cell] == 0)
      numberOfActiveCells++;

  // Geometry
  vtkIdType* cell = new vtkIdType[this->Impl->numberOfDaughters];
  vtkNew<vtkPoints> points;
  ugrid->SetPoints(points);
  ugrid->Allocate(numberOfActiveCells, numberOfActiveCells);

  double xLine[2];
  int numberOfPoints = 0;

  // Insert regular cells
  for (int i = 0; i < numberOfCells; i++)
  {
    if (cell_daughter[i] == 0)
    {

      double cell_half = this->Impl->gridScale[0] / pow(2.0f, cell_level[i]);
      xLine[0] = cell_center[0][i] - cell_half;
      xLine[1] = cell_center[0][i] + cell_half;

      for (int j = 0; j < this->Impl->numberOfDaughters; j++)
      {
        points->InsertNextPoint(xLine[j], 0.0, 0.0);
        numberOfPoints++;
        cell[j] = numberOfPoints - 1;
      }
      ugrid->InsertNextCell(VTK_LINE, this->Impl->numberOfDaughters, cell);
    }
  }
  delete[] cell;
}

///////////////////////////////////////////////////////////////////////////////
//
// Build 2D geometry of quad cells
// Geometry is created for each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_UG_2D(vtkMultiBlockDataSet* grid,
  int numberOfCells,      // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double* cell_center[2]) // Cell center
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(0));
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(multipiece->GetPiece(this->Rank));
  ugrid->Initialize();

  // Get count of cells which will be created for allocation
  int numberOfActiveCells = 0;
  for (int cell = 0; cell < numberOfCells; cell++)
    if (cell_daughter[cell] == 0)
      numberOfActiveCells++;

  // Geometry
  vtkIdType* cell = new vtkIdType[this->Impl->numberOfDaughters];
  vtkNew<vtkPoints> points;
  ugrid->SetPoints(points);
  ugrid->Allocate(numberOfActiveCells, numberOfActiveCells);
  int numberOfPoints = 0;

  // Create the BHTree to ensure unique points
  BHTree* bhTree = new BHTree(
    this->Impl->dimension, this->Impl->numberOfDaughters, this->Impl->minLoc, this->Impl->maxLoc);

  float xBox[4], yBox[4];
  double cell_half[2];
  double point[2];

  // Insert regular cells
  for (int i = 0; i < numberOfCells; i++)
  {
    if (cell_daughter[i] == 0)
    {
      for (int d = 0; d < 2; d++)
      {
        cell_half[d] = this->Impl->gridScale[d] / pow(2.0f, cell_level[i]);
      }

      xBox[0] = cell_center[0][i] - cell_half[0];
      xBox[1] = cell_center[0][i] + cell_half[0];
      xBox[2] = xBox[1];
      xBox[3] = xBox[0];

      yBox[0] = cell_center[1][i] - cell_half[1];
      yBox[1] = yBox[0];
      yBox[2] = cell_center[1][i] + cell_half[1];
      yBox[3] = yBox[2];

      for (int j = 0; j < this->Impl->numberOfDaughters; j++)
      {
        point[0] = xBox[j];
        point[1] = yBox[j];

        // Returned index is one greater than the ParaView index
        int pIndx = bhTree->insertLeaf(point);
        if (pIndx > numberOfPoints)
        {
          points->InsertNextPoint(xBox[j], yBox[j], 0.0);
          numberOfPoints++;
        }
        cell[j] = pIndx - 1;
      }
      ugrid->InsertNextCell(VTK_QUAD, this->Impl->numberOfDaughters, cell);
    }
  }
  delete bhTree;
  delete[] cell;
}

///////////////////////////////////////////////////////////////////////////////
//
// Build 3D geometry of hexahedron cells
// Geometry is created for each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_UG_3D(vtkMultiBlockDataSet* grid,
  int numberOfCells,      // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double* cell_center[3]) // Cell center
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(0));
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(multipiece->GetPiece(this->Rank));
  ugrid->Initialize();

  // Get count of cells which will be created for allocation
  int numberOfActiveCells = 0;
  for (int cell = 0; cell < numberOfCells; cell++)
    if (cell_daughter[cell] == 0)
      numberOfActiveCells++;

  // Geometry
  vtkIdType* cell = new vtkIdType[this->Impl->numberOfDaughters];
  vtkNew<vtkPoints> points;
  ugrid->SetPoints(points);
  ugrid->Allocate(numberOfActiveCells, numberOfActiveCells);

  // Create the BHTree to ensure unique points IDs
  BHTree* bhTree = new BHTree(
    this->Impl->dimension, this->Impl->numberOfDaughters, this->Impl->minLoc, this->Impl->maxLoc);

  /////////////////////////////////////////////////////////////////////////
  //
  // Insert regular cells
  //
  float xBox[8], yBox[8], zBox[8];
  double cell_half[3];
  double point[3];
  int numberOfPoints = 0;

  for (int i = 0; i < numberOfCells; i++)
  {
    if (cell_daughter[i] == 0)
    {
      for (int d = 0; d < 3; d++)
      {
        cell_half[d] = this->Impl->gridScale[d] / pow(2.0f, cell_level[i]);
      }
      xBox[0] = cell_center[0][i] - cell_half[0];
      xBox[1] = cell_center[0][i] + cell_half[0];
      xBox[2] = xBox[1];
      xBox[3] = xBox[0];
      xBox[4] = xBox[0];
      xBox[5] = xBox[1];
      xBox[6] = xBox[1];
      xBox[7] = xBox[0];

      yBox[0] = cell_center[1][i] - cell_half[1];
      yBox[1] = yBox[0];
      yBox[2] = yBox[0];
      yBox[3] = yBox[0];
      yBox[4] = cell_center[1][i] + cell_half[1];
      yBox[5] = yBox[4];
      yBox[6] = yBox[4];
      yBox[7] = yBox[4];

      zBox[0] = cell_center[2][i] - cell_half[2];
      zBox[1] = zBox[0];
      zBox[2] = cell_center[2][i] + cell_half[2];
      zBox[3] = zBox[2];
      zBox[4] = zBox[0];
      zBox[5] = zBox[0];
      zBox[6] = zBox[2];
      zBox[7] = zBox[2];

      for (int j = 0; j < this->Impl->numberOfDaughters; j++)
      {
        point[0] = xBox[j];
        point[1] = yBox[j];
        point[2] = zBox[j];

        // Returned index is one greater than the ParaView index
        int pIndx = bhTree->insertLeaf(point);
        if (pIndx > numberOfPoints)
        {
          points->InsertNextPoint(xBox[j], yBox[j], zBox[j]);
          numberOfPoints++;
        }
        cell[j] = pIndx - 1;
      }
      ugrid->InsertNextCell(VTK_HEXAHEDRON, this->Impl->numberOfDaughters, cell);
    }
  }
  delete bhTree;
  delete[] cell;
}

///////////////////////////////////////////////////////////////////////////////
//
// Recursive part of the level 1 cell count used in load balancing
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::count_hypertree(int64_t curIndex, int64_t* _daughter)
{
  int64_t curDaughter = _daughter[curIndex];
  if (curDaughter == 0)
    return 1;
  curDaughter--;
  int totalVertices = 1;
  for (int d = 0; d < this->Impl->numberOfDaughters; d++)
  {
    totalVertices += count_hypertree(curDaughter + d, _daughter);
  }
  return totalVertices;
}

///////////////////////////////////////////////////////////////////////////////
//
// Recursive part of the hypertree grid build
// Saves the order that cells are made into nodes and leaves for data ordering
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::build_hypertree(
  vtkHyperTreeGridNonOrientedCursor* treeCursor, int64_t curIndex, int64_t* _daughter)
{
  int64_t curDaughter = _daughter[curIndex];

  if (curDaughter == 0)
  {
    return;
  }

  // Indices stored in the daughter are Fortran one based so fix for C access
  curDaughter--;

  // If daughter has children continue to subdivide and recurse
  treeCursor->SubdivideLeaf();

  // All variable data must be stored to line up with all nodes and leaves
  for (int d = 0; d < this->Impl->numberOfDaughters; d++)
  {
    this->indexNodeLeaf.push_back(curDaughter + d);
  }

  // Process each child in the subdivided daughter by descending to that
  // daughter, calculating the index that matches the global value of the
  // daughter, recursing, and finally returning to the parent
  for (int d = 0; d < this->Impl->numberOfDaughters; d++)
  {
    treeCursor->ToChild(d);
    build_hypertree(treeCursor, curDaughter + d, _daughter);
    treeCursor->ToParent();
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Build 3D hypertree grid geometry method:
// XRAGE numbering of level 1 grids does not match the HTG numbering
// HTG varies X grid fastest, then Y, then Z
// XRAGE groups the level 1 into blocks of 8 in a cube and numbers as it does AMR
//
//  2  3  10  11               4   5   6   7
//  0  1   8   9       vs      0   1   2   3
//
//  6  7  14  15              12  13  14  15
//  4  5  12  13               8   9  10  11
//
//  So using the cell_center of a level 1 cell we have to calculate the index
//  in x,y,z and then the tree index from that
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_HTG(vtkMultiBlockDataSet* grid)
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(0));
  vtkHyperTreeGrid* htgrid =
    vtkHyperTreeGrid::SafeDownCast(multipiece->GetPieceAsDataObject(this->Rank));

  htgrid->Initialize();
  htgrid->SetDimensions(
    this->Impl->gridSize[0] + 1, this->Impl->gridSize[1] + 1, this->Impl->gridSize[2] + 1);
  htgrid->SetBranchFactor(2);
  int numberOfTrees = htgrid->GetMaxNumberOfTrees();
  int numberOfCells;

  std::valarray<int> histsize;
  std::valarray<int> level;
  std::valarray<std::valarray<double>> center;
  int64_t* cell_daughter = nullptr;
  int* cell_level = nullptr;
  double* cell_center[3];

  if (this->Rank == 0)
  {
    this->pioData->set_scalar_field(histsize, "hist_size");
    this->pioData->set_scalar_field(this->Impl->daughter, "cell_daughter");
    this->pioData->set_scalar_field(level, "cell_level");
    this->pioData->set_vector_field(center, "cell_center");

    numberOfCells = histsize[histsize.size() - 1];
    cell_daughter = &this->Impl->daughter[0];
    cell_level = &level[0];
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      cell_center[d] = &center[d][0];
    };
  }

  // Allocate space on other processors for all cells
  this->Controller->Broadcast(&numberOfCells, 1, 0);
  if (this->Rank > 0)
  {
    cell_level = new int[numberOfCells];
    cell_daughter = new int64_t[numberOfCells];
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      cell_center[d] = new double[numberOfCells];
    }
  }

  // Share the necessary data
  this->Controller->Broadcast(cell_daughter, numberOfCells, 0);
  this->Controller->Broadcast(cell_level, numberOfCells, 0);
  for (int d = 0; d < this->Impl->dimension; d++)
  {
    this->Controller->Broadcast(cell_center[d], numberOfCells, 0);
  }

  // Copy the daughter into the namespace valarray so it looks like proc 0
  // It is the only one that must be saved because load_variables use it
  if (this->Rank > 0)
  {
    this->Impl->daughter.resize(numberOfCells);
    for (int i = 0; i < numberOfCells; i++)
    {
      this->Impl->daughter[i] = cell_daughter[i];
    }
  }

  for (unsigned int i = 0; i < 3; ++i)
  {
    vtkNew<vtkDoubleArray> coords;
    unsigned int n = this->Impl->gridSize[i] + 1;
    coords->SetNumberOfValues(n);
    for (unsigned int j = 0; j < n; j++)
    {
      double coord = this->Impl->gridOrigin[i] + this->Impl->gridScale[i] * static_cast<double>(j);
      coords->SetValue(j, coord);
    }
    switch (i)
    {
      case 0:
        htgrid->SetXCoordinates(coords.GetPointer());
        break;
      case 1:
        htgrid->SetYCoordinates(coords.GetPointer());
        break;
      case 2:
        htgrid->SetZCoordinates(coords.GetPointer());
        break;
      default:
        break;
    }
  }

  // Locate the level 1 cells which are the top level AMR for a grid position
  // Count the number of nodes and leaves in each level 1 cell for load balance
  int64_t* level1_index = new int64_t[numberOfTrees];
  std::vector<std::pair<int, int>> treeCount;
  std::vector<int> _myHyperTree;

  int planeSize = this->Impl->gridSize[1] * this->Impl->gridSize[0];
  int rowSize = this->Impl->gridSize[0];
  int gridIndx[3] = { 0, 0, 0 };

  for (int i = 0; i < numberOfCells; i++)
  {
    if (cell_level[i] == 1)
    {
      // Calculate which tree because the XRAGE arrangement does not match the HTG
      for (int dim = 0; dim < this->Impl->dimension; dim++)
      {
        gridIndx[dim] = this->Impl->gridSize[dim] *
          ((cell_center[dim][i] - this->Impl->minLoc[dim]) /
            (this->Impl->maxLoc[dim] - this->Impl->minLoc[dim]));
      }

      // Collect the count per tree for load balancing
      int whichTree = (gridIndx[2] * planeSize) + (gridIndx[1] * rowSize) + gridIndx[0];
      int gridCount = count_hypertree(i, cell_daughter);
      treeCount.emplace_back(gridCount, whichTree);

      // Save the xrage cell which corresponds to a level 1 cell
      level1_index[whichTree] = i;
    }
  }

  // Sort the counts and associated hypertrees
  sort(treeCount.begin(), treeCount.end(), sort_desc);

  // Process in descending count order and distribute round robin
  for (int i = 0; i < numberOfTrees; i++)
  {
    int tree = treeCount[i].second;
    int distIndx = i % this->TotalRank;
    if (distIndx == this->Rank)
    {
      _myHyperTree.push_back(tree);
    }
  }

  // Process assigned hypertrees in order
  sort(_myHyperTree.begin(), _myHyperTree.end());

  // Keep a running map of nodes and vertices to xrage indices for displaying data
  vtkNew<vtkHyperTreeGridNonOrientedCursor> treeCursor;
  int globalIndx = 0;
  this->indexNodeLeaf.clear();

  for (size_t i = 0; i < _myHyperTree.size(); i++)
  {
    int tree = _myHyperTree[i];
    int xrageIndx = level1_index[tree];

    htgrid->InitializeNonOrientedCursor(treeCursor, tree, true);
    treeCursor->SetGlobalIndexStart(globalIndx);

    // First node in the hypertree must get a slot
    this->indexNodeLeaf.push_back(xrageIndx);

    // Recursion
    build_hypertree(treeCursor, xrageIndx, cell_daughter);

    vtkHyperTree* htree = htgrid->GetTree(tree);
    int numberOfVertices = htree->GetNumberOfVertices();
    globalIndx += numberOfVertices;
  }
  delete[] level1_index;
  if (this->Rank > 0)
  {
    delete[] cell_level;
    delete[] cell_daughter;
    for (int d = 0; d < this->Impl->dimension; d++)
    {
      delete[] cell_center[d];
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Load all requested variable data into the requested Block() structure
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::load_variable_data(
  vtkMultiBlockDataSet* grid, vtkDataArraySelection* cellDataArraySelection)
{
  if (this->useHTG == false)
  {
    load_variable_data_UG(grid, cellDataArraySelection);
  }
  else
  {
    load_variable_data_HTG(grid, cellDataArraySelection);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Load all requested variable data into unstructured grid which reads on proc 0
// and distributes pieces to other processors
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::load_variable_data_UG(
  vtkMultiBlockDataSet* grid, vtkDataArraySelection* cellDataArraySelection)
{
  int64_t* cell_daughter = &this->Impl->daughter[0];
  for (size_t var = 0; var < this->variableName.size(); var++)
  {
    int numberOfComponents;
    int numberOfCells;
    double** dataVector;
    std::valarray<double> scalarArray;
    std::valarray<std::valarray<double>> vectorArray;

    if (cellDataArraySelection->ArrayIsEnabled(this->variableName[var].c_str()))
    {
      // Using PIOData fetch the variable data from the file only on proc 0
      if (this->Rank == 0)
      {
        numberOfCells = this->Impl->countCell[0];
        numberOfComponents =
          static_cast<int>(this->pioData->VarMMap.count(this->variableName[var].c_str()));
        dataVector = new double*[numberOfComponents];

        bool status = true;
        if (numberOfComponents == 1)
        {
          status = this->pioData->set_scalar_field(scalarArray, this->variableName[var].c_str());
          dataVector[0] = &scalarArray[0];
        }
        else
        {
          status = this->pioData->set_vector_field(vectorArray, this->variableName[var].c_str());
          for (int d = 0; d < numberOfComponents; d++)
          {
            dataVector[d] = &vectorArray[d][0];
          };
        }

        if (status == false)
        {
          // send a -1 as the number of cells to signal to other ranks to skip this variable
          int negative_one = -1;
          for (int rank = 1; rank < this->TotalRank; rank++)
          {
            this->Controller->Send(&negative_one, 1, rank, this->Impl->mpiTag);
          }
          vtkGenericWarningMacro("Error, PIO data was not retrieved: " << this->variableName[var]);
        }
        else
        {
          // Send number of cells, number of components and data
          for (int rank = 1; rank < this->TotalRank; rank++)
          {
            this->Controller->Send(&this->Impl->countCell[rank], 1, rank, this->Impl->mpiTag);
            this->Controller->Send(&numberOfComponents, 1, rank, this->Impl->mpiTag);
            for (int d = 0; d < numberOfComponents; d++)
            {
              this->Controller->Send(&dataVector[d][this->Impl->startCell[rank]],
                this->Impl->countCell[rank], rank, this->Impl->mpiTag);
            }
          }
          // Add the data to the structure
          add_amr_UG_scalar(grid, this->variableName[var], cell_daughter, dataVector, numberOfCells,
            numberOfComponents);
          delete[] dataVector;
        }
      }
      else
      {
        this->Controller->Receive(&numberOfCells, 1, 0, this->Impl->mpiTag);
        if (numberOfCells == -1)
        {
          // there was a problem reading this variable, skip
          continue;
        }
        this->Controller->Receive(&numberOfComponents, 1, 0, this->Impl->mpiTag);

        // Allocate space to receive data
        dataVector = new double*[numberOfComponents];
        for (int d = 0; d < numberOfComponents; d++)
        {
          dataVector[d] = new double[numberOfCells];
        }

        for (int d = 0; d < numberOfComponents; d++)
        {
          this->Controller->Receive(&dataVector[d][0], numberOfCells, 0, this->Impl->mpiTag);
        }
        // Add the data to the structure
        add_amr_UG_scalar(grid, this->variableName[var], cell_daughter, dataVector, numberOfCells,
          numberOfComponents);

        // Delete allocated data after processed
        for (int d = 0; d < numberOfComponents; d++)
        {
          delete[] dataVector[d];
        }
        delete[] dataVector;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Load all requested variable data into hypertree grid which reads on proc 0
// and distributes everything to other processors because it needs recursion
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::load_variable_data_HTG(
  vtkMultiBlockDataSet* grid, vtkDataArraySelection* cellDataArraySelection)
{
  for (size_t var = 0; var < this->variableName.size(); var++)
  {
    double** dataVector = nullptr;
    std::valarray<double> scalarArray;
    std::valarray<std::valarray<double>> vectorArray;
    int numberOfComponents;
    int numberOfCells;
    if (cellDataArraySelection->ArrayIsEnabled(this->variableName[var].c_str()))
    {
      if (this->Rank == 0)
      {
        // Using PIOData fetch the variable data from the file
        numberOfComponents =
          static_cast<int>(this->pioData->VarMMap.count(this->variableName[var].c_str()));
        dataVector = new double*[numberOfComponents];
        if (numberOfComponents == 1)
        {
          this->pioData->set_scalar_field(scalarArray, this->variableName[var].c_str());
          numberOfCells = static_cast<int>(scalarArray.size());
          dataVector[0] = &scalarArray[0];
        }
        else
        {
          this->pioData->set_vector_field(vectorArray, this->variableName[var].c_str());
          numberOfCells = static_cast<int>(vectorArray[0].size());
          for (int d = 0; d < numberOfComponents; d++)
          {
            dataVector[d] = &vectorArray[d][0];
          }
        }
      }

      // Broadcast number of components and number of cells
      this->Controller->Broadcast(&numberOfCells, 1, 0);
      this->Controller->Broadcast(&numberOfComponents, 1, 0);

      // Other processors allocate dataVector
      if (this->Rank > 0)
      {
        // Allocate space to receive data
        dataVector = new double*[numberOfComponents];
        for (int d = 0; d < numberOfComponents; d++)
        {
          dataVector[d] = new double[numberOfCells];
        }
      }

      // Broadcast the data
      for (int d = 0; d < numberOfComponents; d++)
      {
        this->Controller->Broadcast(dataVector[d], numberOfCells, 0);
      }

      // Adding data to hypertree grid uses indirect array built when geometry was built
      add_amr_HTG_scalar(grid, this->variableName[var], dataVector, numberOfComponents);

      // Clear out allocated data for other processors
      if (this->Rank > 0)
      {
        for (int d = 0; d < numberOfComponents; d++)
        {
          delete[] dataVector[d];
        }
      }
      delete[] dataVector;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Add scalar data to hypertree grid points
// Problem with HTG is that both nodes (not visible) and leaves (visible)
// have values, but node values should not be used because they will skew the
// color range of the render.  For each component get a legal value from a leaf
// to set as the value of the nodes.  Not viewed so it will render OK.
// Called each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::add_amr_HTG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
  double* data[],         // Data for all cells
  int numberOfComponents) // Number of components
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(0));
  vtkHyperTreeGrid* htgrid =
    vtkHyperTreeGrid::SafeDownCast(multipiece->GetPieceAsDataObject(this->Rank));

  int numberOfNodesLeaves = static_cast<int>(this->indexNodeLeaf.size());

  // Find the first leaf value to use on all nodes so color range is good
  std::vector<double> nodeValue(numberOfComponents);
  bool done = false;
  int n = 0;
  while (!done && n < numberOfNodesLeaves)
  {
    if (this->Impl->daughter[this->indexNodeLeaf[n]] == 0)
    {
      for (int j = 0; j < numberOfComponents; j++)
      {
        nodeValue[j] = data[j][this->indexNodeLeaf[n]];
      }
      done = true;
    }
    n++;
  }

  // Data array in same order as the geometry cells
  if (this->useFloat64 == true)
  {
    vtkNew<vtkDoubleArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfNodesLeaves);
    htgrid->GetCellData()->AddArray(arr);
    double* varData = arr->GetPointer(0);

    // Copy the data in the order needed for recursive create of HTG
    int varIndex = 0;
    for (int i = 0; i < numberOfNodesLeaves; i++)
    {
      for (int j = 0; j < numberOfComponents; j++)
      {
        if (this->Impl->daughter[this->indexNodeLeaf[i]] == 0)
        {
          varData[varIndex++] = data[j][this->indexNodeLeaf[i]];
        }
        else
        {
          varData[varIndex++] = nodeValue[j];
        }
      }
    }
  }
  else
  {
    vtkNew<vtkFloatArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfNodesLeaves);
    htgrid->GetCellData()->AddArray(arr);
    float* varData = arr->GetPointer(0);

    // Copy the data in the order needed for recursive create of HTG
    int varIndex = 0;
    for (int i = 0; i < numberOfNodesLeaves; i++)
    {
      for (int j = 0; j < numberOfComponents; j++)
      {
        if (this->Impl->daughter[this->indexNodeLeaf[i]] == 0)
        {
          varData[varIndex++] = (float)data[j][this->indexNodeLeaf[i]];
        }
        else
        {
          varData[varIndex++] = (float)nodeValue[j];
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Add scalar data to unstructured grid cells
// daughter array indicates whether data should be used because it is top level
// Called each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::add_amr_UG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
  int64_t* _daughter, // Indicates top level cell or not
  double* data[],     // Data for all cells
  int numberOfCells,
  int numberOfComponents) // Number of components
{
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(grid->GetBlock(0));
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(multipiece->GetPiece(this->Rank));

  int numberOfActiveCells = ugrid->GetNumberOfCells();

  // Data array in same order as the geometry cells
  if (this->useFloat64 == true)
  {
    vtkNew<vtkDoubleArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfActiveCells);
    ugrid->GetCellData()->AddArray(arr);
    double* varData = arr->GetPointer(0);

    // Set the data in the matching cells skipping lower level cells
    int index = 0;
    for (int cell = 0; cell < numberOfCells; cell++)
    {
      if (_daughter[cell] == 0)
      {
        for (int j = 0; j < numberOfComponents; j++)
        {
          varData[index++] = data[j][cell];
        }
      }
    }
  }
  else
  {
    vtkNew<vtkFloatArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfActiveCells);
    ugrid->GetCellData()->AddArray(arr);
    float* varData = arr->GetPointer(0);

    // Set the data in the matching cells skipping lower level cells
    int index = 0;
    for (int cell = 0; cell < numberOfCells; cell++)
    {
      if (_daughter[cell] == 0)
      {
        for (int j = 0; j < numberOfComponents; j++)
        {
          varData[index++] = (float)data[j][cell];
        }
      }
    }
  }
}
