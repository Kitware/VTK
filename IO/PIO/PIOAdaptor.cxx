#include "PIOAdaptor.h"
#include "BHTree.h"

#include "vtkCellType.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"

#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUnstructuredGrid.h"

#include <float.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#ifdef _WIN32
const static char* Slash = "\\/";
#else
const static char* Slash = "/";
#endif

using namespace std;

namespace
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
std::valarray<int64_t> daughter;

// Used in load balancing of unstructure grid
int firstCell;
int lastCell;

// Used in load balancing of hypertree grid
// int numberOfTrees;
std::map<int, int> myHyperTree;
bool sort_desc(const pair<int, int>& a, const pair<int, int>& b)
{
  return (a.first > b.first);
}
};

///////////////////////////////////////////////////////////////////////////////
//
// Constructor and destructor
//
///////////////////////////////////////////////////////////////////////////////

PIOAdaptor::PIOAdaptor(int rank, int totalrank)
{
  this->Rank = rank;
  this->TotalRank = totalrank;
  this->pioData = 0;
}

PIOAdaptor::~PIOAdaptor()
{
  if (this->pioData != 0)
    delete this->pioData;
  delete[] this->timeSteps;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read the global descriptor file (name.pio)
//
// DUMP_DIRECTORY dumps       (Default to .)
// DUMP_BASE_NAME base        (Required)
//
// MAKE_HTG YES    (Default NO) means create unstructured grid
// MAKE_TRACER NO  (Default NO) means don't create unstructured grid of particles
// FLOAT64 YES     (Default NO) means use 32 bit float for data
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::initializeGlobal(const char* PIOFileName)
{
  this->descFileName = PIOFileName;
  ifstream inStr(this->descFileName);
  if (!inStr)
  {
    vtkGenericWarningMacro("Could not open the global description .pio file: " << PIOFileName);
    return 0;
  }

  // Get the directory name from the full path of the .pio file
  string::size_type dirPos = this->descFileName.find_last_of(Slash);
  string dirName;
  if (dirPos == string::npos)
  {
    vtkGenericWarningMacro("Bad input file name: " << PIOFileName);
    return 0;
  }
  else
  {
    dirName = this->descFileName.substr(0, dirPos);
  }

  // Either .pio file or an actual basename-dmp000000 to guide open file
  // Opening actual dump file requires asking for All files and picking PIOReader
  // Opening a pio suffix file defaults to the correct action
  string::size_type pos = this->descFileName.rfind('.');
  string suffix = this->descFileName.substr(pos + 1);
  if (suffix == "pio")
  {
    /////////////////////////////////////////////////////////////////////////////
    //
    // Parse the pio input file
    //
    char inBuf[256];
    string rest;
    string keyword;
    this->useHTG = false;
    this->useTracer = false;
    this->useFloat64 = false;
    this->hasTracers = false;
    this->dumpDirectory = dirName;

    while (inStr.getline(inBuf, 256))
    {
      if (inBuf[0] != '#' && inStr.gcount() > 1)
      {
        // Remove quotes from input
        string localline(inBuf);
        localline.erase(std::remove(localline.begin(), localline.end(), '\"'), localline.end());
        localline.erase(std::remove(localline.begin(), localline.end(), '\''), localline.end());

        string::size_type keyPos = localline.find(' ');
        keyword = localline.substr(0, keyPos);
        rest = localline.substr(keyPos + 1);
        istringstream line(rest.c_str());

        if (keyword == "DUMP_DIRECTORY")
        {
          line >> rest;
          if (rest[0] == '/')
          {
            // If a full path is given use it
            this->dumpDirectory = rest;
          }
          else
          {
            // If partial path append to the dir of the .pio file
            ostringstream tempStr;
            tempStr << dirName << Slash << rest;
            this->dumpDirectory = tempStr.str();
          }
        }
        if (keyword == "DUMP_BASE_NAME")
        {
          line >> rest;
          ostringstream tempStr;
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
  else
  {
    /////////////////////////////////////////////////////////////////////////////
    //
    // Use the basename-dmp000000 file to discern the info that is in the pio file
    //
    string::size_type pos1 = this->descFileName.rfind(Slash);
    string::size_type pos2 = this->descFileName.find("-dmp");
    this->dumpBaseName = this->descFileName.substr(pos1 + 1, pos2 - pos1 + 3);
    this->dumpDirectory = this->descFileName.substr(0, pos1);
    this->useHTG = false;
    this->useTracer = false;
    this->useFloat64 = false;
    this->hasTracers = false;
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // Using the dump directory and base name form the dump file names
  // Sort into order by cycle number which ends the dump file name
  //
  auto dir = vtkSmartPointer<vtkDirectory>::New();
  uint64_t numFiles = 0;
  std::vector<string> fileCandidate;
  if (dir->Open(this->dumpDirectory.c_str()) != false)
  {
    numFiles = dir->GetNumberOfFiles();
    this->timeSteps = new double[numFiles];
    this->numberOfTimeSteps = 0;
    for (unsigned int i = 0; i < numFiles; i++)
    {
      string fileName = dir->GetFile(i);
      std::size_t found = fileName.find(this->dumpBaseName);
      if (found != std::string::npos)
      {
        fileCandidate.push_back(fileName);
      }
    }
    sort(fileCandidate.begin(), fileCandidate.end());

    // Iterate on candidates and allow only those ending in cycle number
    for (size_t i = 0; i < fileCandidate.size(); i++)
    {
      std::size_t pos1 = this->dumpBaseName.size();
      std::size_t pos2 = fileCandidate[i].size();
      std::string timeStr = fileCandidate[i].substr(pos1, pos2);
      double time = 0.0;
      if (timeStr.size() > 0)
      {
        char* p;
        std::strtol(timeStr.c_str(), &p, 10);

        // File name starting with base, ending in cycle number
        if (*p == 0)
        {
          time = std::stod(timeStr);
          this->timeSteps[this->numberOfTimeSteps++] = time;
          ostringstream tempStr;
          tempStr << dumpDirectory << Slash << fileCandidate[i];
          this->dumpFileName.push_back(tempStr.str());
        }
      }
    }
  }
  else
  {
    vtkGenericWarningMacro("Dump directory does not exist: " << this->dumpDirectory);
    return 0;
  }
  if (this->dumpFileName.size() == 0)
  {
    vtkGenericWarningMacro("No files exist with the base name :" << this->dumpBaseName);
    return 0;
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // Collect variables having a value for every cell from dump file
  //
  this->pioData = new PIO_DATA(this->dumpFileName[0].c_str());
  if (this->pioData->good_read())
  {
    // Get the number of cells in dump file for this time step
    std::valarray<int> histsize;
    this->pioData->set_scalar_field(histsize, "hist_size");
    int numberOfCells = histsize[histsize.size() - 1];
    int numberOfFields = this->pioData->get_pio_num();
    PIO_FIELD* pioField = this->pioData->get_pio_field();

    for (int i = 0; i < numberOfFields; i++)
    {
      if (pioField[i].length == numberOfCells && pioField[i].cdata_len == 0)
      {
        // index = 0 is scalar, index = 1 is vector, index = -1 is request from input deck
        int index = pioField[i].index;
        if (index == 0 || index == 1 || index == -1)
        {
          // Discard names used in geometry and variables with too many components
          // which are present for use in tracers
          char* pioName = pioField[i].pio_name;
          size_t numberOfComponents = this->pioData->VarMMap.count(pioName);

          // Are tracers available in file
          if (strcmp(pioName, "tracer_num_pnts") == 0)
          {
            this->hasTracers = true;
          }

          if ((numberOfComponents <= 9) && (strcmp(pioName, "cell_has_tracers") != 0) &&
            (strcmp(pioName, "cell_level") != 0) && (strcmp(pioName, "cell_mother") != 0) &&
            (strcmp(pioName, "cell_daughter") != 0) && (strcmp(pioName, "cell_center") != 0) &&
            (strcmp(pioName, "cell_active") != 0) && (strcmp(pioName, "amr_tag") != 0))
          {
            this->variableName.push_back(pioName);
          }
        }
      }
    }
    sort(this->variableName.begin(), this->variableName.end());

    // Default variable names that are initially enabled for loading
    this->variableDefault.push_back("tev");
    this->variableDefault.push_back("prs");
    this->variableDefault.push_back("rho");
    this->variableDefault.push_back("rade");
    this->variableDefault.push_back("cell_energy");
    this->variableDefault.push_back("kemax");
    this->variableDefault.push_back("vel");
    this->variableDefault.push_back("eng");
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // List of all data fields to read from dump files
  //
  this->fieldsToRead.push_back("amhc_i");
  this->fieldsToRead.push_back("amhc_r8");
  this->fieldsToRead.push_back("amhc_l");
  this->fieldsToRead.push_back("cell_center");
  this->fieldsToRead.push_back("cell_daughter");
  this->fieldsToRead.push_back("cell_level");
  this->fieldsToRead.push_back("global_numcell");
  this->fieldsToRead.push_back("hist_cycle");
  this->fieldsToRead.push_back("hist_time");
  this->fieldsToRead.push_back("hist_size");
  this->fieldsToRead.push_back("l_eap_version");
  this->fieldsToRead.push_back("hist_usernm");
  this->fieldsToRead.push_back("hist_prbnm");

  // If tracers are contained in the file
  if (this->hasTracers == true)
  {
    this->fieldsToRead.push_back("tracer_num_pnts");
    this->fieldsToRead.push_back("tracer_num_vars");
    this->fieldsToRead.push_back("tracer_record_count");
    this->fieldsToRead.push_back("tracer_type");
    this->fieldsToRead.push_back("tracer_position");
    this->fieldsToRead.push_back("tracer_data");
  }

  // Requested variable fields from pio meta file
  for (unsigned int i = 0; i < this->variableName.size(); i++)
  {
    this->fieldsToRead.push_back(this->variableName[i]);
  }
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialize with the *.pio file giving the name of the dump file, whether to
// create hypertree grid or unstructured grid, and variables to read
//
///////////////////////////////////////////////////////////////////////////////

int PIOAdaptor::initializeDump(int timeStep)
{
  // Start with a fresh pioData initialized for this time step
  if (this->pioData != 0)
  {
    delete this->pioData;
    this->pioData = 0;
  }

  // Create one PIOData which accesses the PIO file to fetch data
  if (this->pioData == 0)
  {
    this->pioData = new PIO_DATA(this->dumpFileName[timeStep].c_str(), &this->fieldsToRead);
    if (this->pioData->good_read())
    {

      // First collect the sizes of the domains
      const double* amhc_i = this->pioData->GetPIOData("amhc_i");
      const double* amhc_r8 = this->pioData->GetPIOData("amhc_r8");
      const double* amhc_l = this->pioData->GetPIOData("amhc_l");

      if (amhc_i != 0 && amhc_r8 != 0 && amhc_l != 0)
      {
        dimension = uint32_t(amhc_i[Nnumdim]);
        numberOfDaughters = (int)pow(2.0, dimension);

        // Save sizes for use in creating structures
        for (int i = 0; i < 3; i++)
        {
          gridOrigin[i] = 0.0;
          gridScale[i] = 0.0;
          gridSize[i] = 0;
        }
        gridOrigin[0] = amhc_r8[NZero0];
        gridScale[0] = amhc_r8[Nd0];
        gridSize[0] = static_cast<int>(amhc_i[Nmesh0]);

        if (dimension > 1)
        {
          gridOrigin[1] = amhc_r8[NZero1];
          gridScale[1] = amhc_r8[Nd1];
          gridSize[1] = static_cast<int>(amhc_i[Nmesh1]);
        }

        if (dimension > 2)
        {
          gridOrigin[2] = amhc_r8[NZero2];
          gridScale[2] = amhc_r8[Nd2];
          gridSize[2] = static_cast<int>(amhc_i[Nmesh2]);
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
    minLoc[i] = gridOrigin[i];
    maxLoc[i] = minLoc[i] + (gridSize[i] * gridScale[i]);
  }
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
    // Create an unstructured grid to hold the dump file data
    vtkNew<vtkUnstructuredGrid> ugrid;
    ugrid->Initialize();
    grid->SetBlock(0, ugrid);
  }
  else
  {
    // Create a hypertree grid to hold the dump file data
    vtkNew<vtkHyperTreeGrid> htgrid;
    htgrid->Initialize();
    grid->SetBlock(0, htgrid);
  }

  // If tracers are used add a second block of unstructured grid particles
  if (this->hasTracers == true && this->useTracer == true)
  {
    grid->SetNumberOfBlocks(2);
    vtkNew<vtkUnstructuredGrid> tgrid;
    tgrid->Initialize();
    grid->SetBlock(1, tgrid);
  }

  // Collect geometry information from PIOData files
  std::valarray<int> histsize;
  std::valarray<int> level;
  std::valarray<int> numcell;
  std::valarray<double> simCycle;
  std::valarray<double> simTime;
  std::valarray<std::valarray<double> > center;

  this->pioData->set_scalar_field(histsize, "hist_size");
  this->pioData->set_scalar_field(daughter, "cell_daughter");
  this->pioData->set_scalar_field(level, "cell_level");
  this->pioData->set_scalar_field(numcell, "global_numcell");
  this->pioData->set_vector_field(center, "cell_center");

  int numberOfCells = histsize[histsize.size() - 1];
  int numProc = static_cast<int>(numcell.size());

  int64_t* cell_daughter = &daughter[0];
  int* cell_level = &level[0];
  double* cell_center[3];
  int* global_numcell = &numcell[0];
  for (int d = 0; d < dimension; d++)
  {
    cell_center[d] = &center[d][0];
  };

  // Create the VTK structures within multiblock
  if (this->useHTG == true)
  {
    // Create AMR HyperTreeGrid
    create_amr_HTG(grid, numberOfCells, cell_level, cell_daughter, cell_center);
  }
  else
  {
    // Create AMR UnstructuredGrid
    create_amr_UG(grid, numProc, global_numcell, cell_level, cell_daughter, cell_center);
  }

  // Create Tracer Unstructured if tracers exist
  if (this->useTracer == true)
  {
    if (this->hasTracers == true)
    {
      create_tracer_UG(grid);
    }
    else
    {
      vtkGenericWarningMacro("Tracers don't exist in .pio file: " << this->descFileName);
    }
  }

  // Collect other information from PIOData
  const char* cdata;
  this->pioData->GetPIOData("l_eap_version", cdata);
  vtkStdString eap_version(cdata);

  this->pioData->set_scalar_field(simCycle, "hist_cycle");
  this->pioData->set_scalar_field(simTime, "hist_time");
  int curIndex = static_cast<int>(simCycle.size()) - 1;

  this->pioData->GetPIOData("hist_usernm", cdata);
  vtkStdString user_name(cdata);

  this->pioData->GetPIOData("hist_prbnm", cdata);
  vtkStdString problem_name(cdata);

  // Add FieldData array for cycle number
  vtkNew<vtkIntArray> cycleArray;
  cycleArray->SetName("cycle_index");
  cycleArray->SetNumberOfComponents(1);
  cycleArray->SetNumberOfTuples(1);
  cycleArray->SetTuple1(0, (int)simCycle[curIndex]);
  grid->GetFieldData()->AddArray(cycleArray);

  // Add FieldData array for simulation time
  vtkNew<vtkFloatArray> simTimeArray;
  simTimeArray->SetName("simulated_time");
  simTimeArray->SetNumberOfComponents(1);
  simTimeArray->SetNumberOfTuples(1);
  simTimeArray->SetTuple1(0, simTime[curIndex]);
  grid->GetFieldData()->AddArray(simTimeArray);

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
}

///////////////////////////////////////////////////////////////////////////////
//
// Build unstructured grid for tracers
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_tracer_UG(vtkMultiBlockDataSet* grid)
{
  vtkUnstructuredGrid* tgrid = vtkUnstructuredGrid::SafeDownCast(grid->GetBlock(1));
  tgrid->Initialize();

  // Get tracer information from PIOData
  std::valarray<int> tracer_num_pnts;
  std::valarray<int> tracer_num_vars;
  std::valarray<int> tracer_record_count;
  std::valarray<std::valarray<double> > tracer_position;
  std::valarray<std::valarray<double> > tracer_data;

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
    for (int dim = 0; dim < dimension; dim++)
    {
      pointPos[dim] = tracer_position[dim][i];
    }
    points->InsertNextPoint(pointPos[0], pointPos[1], pointPos[2]);
    cell[0] = i;
    tgrid->InsertNextCell(VTK_VERTEX, 1, cell);
  }

  // Add other tracer data which appears by time step, then by tracer, then by variable
  // Variable data starts with cycle time and coordinate[numdim]
  int tracerDataOffset = 1 + dimension;
  if (this->useFloat64 == true)
  {
    std::vector<double*> varData(numberOfTracerVars);
    for (int var = 0; var < numberOfTracerVars; var++)
    {
      vtkNew<vtkDoubleArray> arr;
      arr->SetName(tracer_type[var].c_str());
      arr->SetNumberOfComponents(1);
      arr->SetNumberOfTuples(numberOfTracers);
      varData[var] = arr->GetPointer(0);
      tgrid->GetPointData()->AddArray(arr);
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
      tgrid->GetPointData()->AddArray(arr);
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
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_UG(vtkMultiBlockDataSet* grid,
  int numberOfGlobal,     // Number of XRAGE processors from sim
  int* global_numcell,    // Load balance from the XRAGE sim
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double** cell_center)   // Cell center
{
  // Count the number of cells for load balancing between xrage procs and paraview procs
  std::vector<int> countPerRank(this->TotalRank);
  for (int rank = 0; rank < this->TotalRank; rank++)
  {
    countPerRank[rank] = numberOfGlobal / this->TotalRank;
  }
  countPerRank[this->TotalRank - 1] += (numberOfGlobal % this->TotalRank);

  std::vector<int> startCell(this->TotalRank);
  std::vector<int> endCell(this->TotalRank);
  int currentCell = 0;
  int globalIndx = 0;

  // Calculate the start and stop cell index per rank for redistribution
  for (int rank = 0; rank < this->TotalRank; rank++)
  {
    startCell[rank] = currentCell;
    endCell[rank] = startCell[rank];
    for (int i = 0; i < countPerRank[rank]; i++)
    {
      endCell[rank] += global_numcell[globalIndx++];
    }
    currentCell = endCell[rank];
  }

  firstCell = startCell[this->Rank];
  lastCell = endCell[this->Rank];

  // Based on dimension and cell range build the unstructured grid pieces
  if (dimension == 1)
  {
    create_amr_UG_1D(
      grid, startCell[this->Rank], endCell[this->Rank], cell_level, cell_daughter, cell_center);
  }
  else if (dimension == 2)
  {
    create_amr_UG_2D(
      grid, startCell[this->Rank], endCell[this->Rank], cell_level, cell_daughter, cell_center);
  }
  else
  {
    create_amr_UG_3D(
      grid, startCell[this->Rank], endCell[this->Rank], cell_level, cell_daughter, cell_center);
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// Build 1D geometry of line cells
// Geometry is created for each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::create_amr_UG_1D(vtkMultiBlockDataSet* grid, int startCellIndx,
  int endCellIndx,        // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double* cell_center[1]) // Cell center
{
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(grid->GetBlock(0));
  ugrid->Initialize();

  // Get count of cells which will be created for allocation
  int numberOfActiveCells = 0;
  for (int cell = startCellIndx; cell < endCellIndx; cell++)
    if (cell_daughter[cell] == 0)
      numberOfActiveCells++;

  // Geometry
  vtkIdType* cell = new vtkIdType[numberOfDaughters];
  vtkNew<vtkPoints> points;
  ugrid->SetPoints(points);
  ugrid->Allocate(numberOfActiveCells, numberOfActiveCells);

  double xLine[2];
  int numberOfPoints = 0;

  // Insert regular cells
  for (int i = startCellIndx; i < endCellIndx; i++)
  {
    if (cell_daughter[i] == 0)
    {

      double cell_half = gridScale[0] / pow(2.0f, cell_level[i]);
      xLine[0] = cell_center[0][i] - cell_half;
      xLine[1] = cell_center[0][i] + cell_half;

      for (int j = 0; j < numberOfDaughters; j++)
      {
        points->InsertNextPoint(xLine[j], 0.0, 0.0);
        numberOfPoints++;
        cell[j] = numberOfPoints - 1;
      }
      ugrid->InsertNextCell(VTK_LINE, numberOfDaughters, cell);
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

void PIOAdaptor::create_amr_UG_2D(vtkMultiBlockDataSet* grid, int startCellIndx,
  int endCellIndx,        // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double* cell_center[2]) // Cell center
{
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(grid->GetBlock(0));
  ugrid->Initialize();

  // Get count of cells which will be created for allocation
  int numberOfActiveCells = 0;
  for (int cell = startCellIndx; cell < endCellIndx; cell++)
    if (cell_daughter[cell] == 0)
      numberOfActiveCells++;

  // Geometry
  vtkIdType* cell = new vtkIdType[numberOfDaughters];
  vtkNew<vtkPoints> points;
  ugrid->SetPoints(points);
  ugrid->Allocate(numberOfActiveCells, numberOfActiveCells);
  int numberOfPoints = 0;

  // Create the BHTree to ensure unique points
  BHTree* bhTree = new BHTree(dimension, numberOfDaughters, minLoc, maxLoc);

  float xBox[4], yBox[4];
  double cell_half[2];
  double point[2];

  // Insert regular cells
  for (int i = startCellIndx; i < endCellIndx; i++)
  {
    if (cell_daughter[i] == 0)
    {
      for (int d = 0; d < 2; d++)
      {
        cell_half[d] = gridScale[d] / pow(2.0f, cell_level[i]);
      }

      xBox[0] = cell_center[0][i] - cell_half[0];
      xBox[1] = cell_center[0][i] + cell_half[0];
      xBox[2] = xBox[1];
      xBox[3] = xBox[0];

      yBox[0] = cell_center[1][i] - cell_half[1];
      yBox[1] = yBox[0];
      yBox[2] = cell_center[1][i] + cell_half[1];
      yBox[3] = yBox[2];

      for (int j = 0; j < numberOfDaughters; j++)
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
      ugrid->InsertNextCell(VTK_QUAD, numberOfDaughters, cell);
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

void PIOAdaptor::create_amr_UG_3D(vtkMultiBlockDataSet* grid, int startCellIndx,
  int endCellIndx,        // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
  double* cell_center[3]) // Cell center
{
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(grid->GetBlock(0));
  ugrid->Initialize();

  // Get count of cells which will be created for allocation
  int numberOfActiveCells = 0;
  for (int cell = startCellIndx; cell < endCellIndx; cell++)
    if (cell_daughter[cell] == 0)
      numberOfActiveCells++;

  // Geometry
  vtkIdType* cell = new vtkIdType[numberOfDaughters];
  vtkNew<vtkPoints> points;
  ugrid->SetPoints(points);
  ugrid->Allocate(numberOfActiveCells, numberOfActiveCells);

  // Create the BHTree to ensure unique points IDs
  BHTree* bhTree = new BHTree(dimension, numberOfDaughters, minLoc, maxLoc);

  /////////////////////////////////////////////////////////////////////////
  //
  // Insert regular cells
  //
  float xBox[8], yBox[8], zBox[8];
  double cell_half[3];
  double point[3];
  int numberOfPoints = 0;

  for (int i = startCellIndx; i < endCellIndx; i++)
  {
    if (cell_daughter[i] == 0)
    {

      for (int d = 0; d < 3; d++)
      {
        cell_half[d] = gridScale[d] / pow(2.0f, cell_level[i]);
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

      for (int j = 0; j < numberOfDaughters; j++)
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
      ugrid->InsertNextCell(VTK_HEXAHEDRON, numberOfDaughters, cell);
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
  for (int d = 0; d < numberOfDaughters; d++)
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
  for (int d = 0; d < numberOfDaughters; d++)
  {
    this->indexNodeLeaf.push_back(curDaughter + d);
  }

  // Process each child in the subdivided daughter by descending to that
  // daughter, calculating the index that matches the global value of the
  // daughter, recursing, and finally returning to the parent
  for (int d = 0; d < numberOfDaughters; d++)
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

void PIOAdaptor::create_amr_HTG(vtkMultiBlockDataSet* grid,
  int numberOfCells,      // Number of cells all levels
  int* cell_level,        // Level of the cell in the AMR
  int64_t* cell_daughter, // Daughter
  double* cell_center[3]) // Cell center
{
  vtkHyperTreeGrid* htgrid = vtkHyperTreeGrid::SafeDownCast(grid->GetBlock(0));
  htgrid->Initialize();
  htgrid->SetDimensions(gridSize[0] + 1, gridSize[1] + 1, gridSize[2] + 1);
  htgrid->SetBranchFactor(2);
  int numberOfTrees = htgrid->GetMaxNumberOfTrees();

  for (unsigned int i = 0; i < 3; ++i)
  {
    vtkNew<vtkDoubleArray> coords;
    unsigned int n = gridSize[i] + 1;
    coords->SetNumberOfValues(n);
    for (unsigned int j = 0; j < n; j++)
    {
      double coord = gridOrigin[i] + gridScale[i] * static_cast<double>(j);
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
  std::vector<std::pair<int, int> > treeCount;
  std::vector<int> _myHyperTree;

  int planeSize = gridSize[1] * gridSize[0];
  int rowSize = gridSize[0];
  int gridIndx[3] = { 0, 0, 0 };

  for (int i = 0; i < numberOfCells; i++)
  {
    if (cell_level[i] == 1)
    {
      // Calculate which tree because the XRAGE arrangement does not match the HTG
      for (int dim = 0; dim < dimension; dim++)
      {
        gridIndx[dim] =
          gridSize[dim] * ((cell_center[dim][i] - minLoc[dim]) / (maxLoc[dim] - minLoc[dim]));
      }

      // Collect the count per tree for load balancing
      int whichTree = (gridIndx[2] * planeSize) + (gridIndx[1] * rowSize) + gridIndx[0];
      int gridCount = count_hypertree(i, cell_daughter);
      treeCount.push_back(std::make_pair(gridCount, whichTree));

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
}

///////////////////////////////////////////////////////////////////////////////
//
// Load all requested variable data into the requested Block() structure
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::load_variable_data(
  vtkMultiBlockDataSet* grid, vtkDataArraySelection* PointDataArraySelection)
{
  for (unsigned int var = 0; var < this->variableName.size(); var++)
  {
    int64_t* cell_daughter = &daughter[0];
    if (PointDataArraySelection->ArrayIsEnabled(this->variableName[var].c_str()))
    {
      // Using PIOData fetch the variable data from the file
      int numberOfComponents =
        static_cast<int>(this->pioData->VarMMap.count(this->variableName[var].c_str()));
      double** dataVector = new double*[numberOfComponents];
      std::valarray<double> scalarArray;
      std::valarray<std::valarray<double> > vectorArray;

      if (numberOfComponents == 1)
      {
        this->pioData->set_scalar_field(scalarArray, this->variableName[var].c_str());
        dataVector[0] = &scalarArray[0];
      }
      else
      {
        this->pioData->set_vector_field(vectorArray, this->variableName[var].c_str());
        for (int d = 0; d < numberOfComponents; d++)
        {
          dataVector[d] = &vectorArray[d][0];
        };
      }

      if (this->useHTG == false)
      {
        // Adding data to unstructured uses the daughter to locate leaf cells
        add_amr_UG_scalar(
          grid, this->variableName[var], cell_daughter, dataVector, numberOfComponents);
      }
      else
      {
        // Adding data to hypertree grid uses indirect array built when geometry was built
        add_amr_HTG_scalar(grid, this->variableName[var], dataVector, numberOfComponents);
      }
      delete[] dataVector;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Add scalar data to hypertree grid points
// Called each time step
//
///////////////////////////////////////////////////////////////////////////////

void PIOAdaptor::add_amr_HTG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
  double* data[],         // Data for all cells
  int numberOfComponents) // Number of components
{
  vtkHyperTreeGrid* htgrid = vtkHyperTreeGrid::SafeDownCast(grid->GetBlock(0));
  int numberOfNodesLeaves = static_cast<int>(this->indexNodeLeaf.size());

  // Data array in same order as the geometry cells
  if (this->useFloat64 == true)
  {
    vtkNew<vtkDoubleArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfNodesLeaves);
    htgrid->GetPointData()->AddArray(arr);
    double* varData = arr->GetPointer(0);

    // Copy the data in the order needed for recursive create of HTG
    int varIndex = 0;
    for (int i = 0; i < numberOfNodesLeaves; i++)
    {
      for (int j = 0; j < numberOfComponents; j++)
      {
        varData[varIndex++] = data[j][this->indexNodeLeaf[i]];
      }
    }
  }
  else
  {
    vtkNew<vtkFloatArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfNodesLeaves);
    htgrid->GetPointData()->AddArray(arr);
    float* varData = arr->GetPointer(0);

    // Copy the data in the order needed for recursive create of HTG
    int varIndex = 0;
    for (int i = 0; i < numberOfNodesLeaves; i++)
    {
      for (int j = 0; j < numberOfComponents; j++)
      {
        varData[varIndex++] = (float)data[j][this->indexNodeLeaf[i]];
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
  int64_t* _daughter,     // Indicates top level cell or not
  double* data[],         // Data for all cells
  int numberOfComponents) // Number of components
{
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast(grid->GetBlock(0));

  int numberOfActiveCells = ugrid->GetNumberOfCells();

  // Data array in same order as the geometry cells
  if (this->useFloat64 == true)
  {
    vtkNew<vtkDoubleArray> arr;
    arr->SetName(varName);
    arr->SetNumberOfComponents(numberOfComponents);
    arr->SetNumberOfTuples(numberOfActiveCells);
    ugrid->GetPointData()->AddArray(arr);
    double* varData = arr->GetPointer(0);

    // Set the data in the matching cells skipping lower level cells
    int index = 0;
    for (int cell = firstCell; cell < lastCell; cell++)
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
    ugrid->GetPointData()->AddArray(arr);
    float* varData = arr->GetPointer(0);

    // Set the data in the matching cells skipping lower level cells
    int index = 0;
    for (int cell = firstCell; cell < lastCell; cell++)
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
