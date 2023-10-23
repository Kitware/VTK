// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
 *
 * @class PIOAdaptor
 * @brief   class for reading PIO (Parallel Input Output) data files
 *
 * This class reads in dump files generated from xRage, a LANL physics code.
 * The PIO (Parallel Input Output) library is used to create the dump files.
 *
 * @par Thanks:
 * Developed by Patricia Fasel at Los Alamos National Laboratory
 */

#ifndef PIOAdaptor_h
#define PIOAdaptor_h

#include "vtkDataArraySelection.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkMultiBlockDataSet.h"

#include "PIOData.h"
#include "PIODataHDF5.h"
#include "PIODataPIO.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

// class to hold information about chunk/material variables
class PIOMaterialVariable
{
public:
  std::string prefix;
  std::string var;           // actual variable
  std::string baseVar;       // variable used to derive actual variable
  std::string material_name; // full name of the material
  uint32_t material_number;
};

class PIOAdaptor
{
public:
  PIOAdaptor(vtkMultiProcessController* ctrl);
  ~PIOAdaptor();

  int initializeGlobal(const char* DumpDescFile);
  int initializeDump(int timeStep);

  // Time step change requires new geometry and data
  void create_geometry(vtkMultiBlockDataSet* grid);
  void load_variable_data(vtkMultiBlockDataSet* grid, vtkDataArraySelection* cellSelection);

  int GetNumberOfTimeSteps() { return static_cast<int>(this->CycleIndex.size()); }
  double GetSimulationTime(int step) { return this->SimulationTime[step]; }
  double GetCycleIndex(int step) { return this->CycleIndex[step]; }
  double GetPIOFileIndex(int step) { return this->PIOFileIndex[step]; }

  int GetNumberOfVariables() { return (int)this->variableName.size(); }
  const char* GetVariableName(int indx) { return this->variableName[indx].c_str(); }
  int GetNumberOfDefaultVariables() { return (int)this->variableDefault.size(); }
  const char* GetVariableDefault(int indx) { return this->variableDefault[indx].c_str(); }

  // Read pio dump file AMR as hypertree grid rather than unstructured grid
  bool GetHyperTreeGrid() { return this->useHTG; }
  void SetHyperTreeGrid(bool val) { this->useHTG = val; }

  // Read pio dump file tracer information
  bool GetTracers() { return this->useTracer; }
  void SetTracers(bool val) { this->useTracer = val; }

  // Read pio dump file variable data as 64 bit float
  bool GetFloat64() { return this->useFloat64; }
  void SetFloat64(bool val) { this->useFloat64 = val; }

protected:
  // Collect the metadata
  int parsePIOFile(const char* DumpDescFile);
  PIO_DATA* openPIODataFile(const char* filename);
  int collectMetaData(const char* DumpDescFile);
  void collectVariableMetaData();
  void collectMaterialVariableMetaData();
  void addMaterialVariable(vtkStdString& pioFieldName, std::valarray<std::string> matident);
  void addMaterialVariableEntries(std::string& prefix, std::string& baseVar, std::string& var,
    std::valarray<std::string> matident);
  std::string trimString(const std::string& str);

  // Create the unstructured grid for tracers
  void create_tracer_UG(vtkMultiBlockDataSet* grid);

  // Create the unstructured grid for AMR
  void create_amr_UG(vtkMultiBlockDataSet* grid);

  void create_amr_UG_1D(vtkMultiBlockDataSet* grid,
    int numberOfCells,       // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter ID, 0 indicates no daughter
    double* cell_center[1]); // Cell center

  void create_amr_UG_2D(vtkMultiBlockDataSet* grid,
    int numberOfCells,       // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter ID, 0 indicates no daughter
    double* cell_center[2]); // Cell center

  void create_amr_UG_3D(vtkMultiBlockDataSet* grid,
    int numberOfCells,       // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter ID, 0 indicates no daughter
    double* cell_center[3]); // Cell center

  // Create the hypertree grid
  void create_amr_HTG(vtkMultiBlockDataSet* grid);

  int count_hypertree(int64_t curIndex, int64_t* daughter);

  void build_hypertree(
    vtkHyperTreeGridNonOrientedCursor* treeCursor, int64_t curIndex, int64_t* daughter);

  // Add variable data to the unstructured grid
  void load_variable_data_UG(vtkMultiBlockDataSet* grid, vtkDataArraySelection* cellSelection);
  void add_amr_UG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
    int64_t* daughter, // Indicates top level cell or not
    double* data[],    // Data for all cells
    int numberOfCells,
    int numberOfComponents); // Number of components in data

  // Add variable data to the hypertree grid
  void load_variable_data_HTG(vtkMultiBlockDataSet* grid, vtkDataArraySelection* cellSelection);
  void add_amr_HTG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
    double* data[],          // Data for all cells
    int numberOfComponents); // Number of components in data

  // Used in parallel reader and load balancing
  vtkMultiProcessController* Controller;
  int Rank;
  int TotalRank;

  // Structure to access the dump file data
  PIO_DATA* pioData;

  bool knownFormat; // whether the pio format is known or not
  bool isHDF5; // what type of internal format the pio file is, either native pio or hdf5 variant

  // Time series of dumps
  std::string descFileName;               // name.pio
  std::string dumpBaseName;               // base name to use for dumps
  std::vector<std::string> dumpDirectory; // directories holding dumps
  std::vector<std::string> dumpFileName;  // all dump files

  // Time step information
  std::vector<double> CycleIndex;     // Times as cycle index
  std::vector<double> SimulationTime; // Times as simulation time
  std::vector<double> PIOFileIndex;   // Index into dump files

  // Type of block structures to create within multiblock dataset
  bool useHTG;
  bool useTracer;
  bool useFloat64;
  bool hasTracers;

  // Cell variable data and initially enabled variables
  std::vector<std::string> variableName;
  std::vector<std::string> variableDefault;

  // total number of cells in the mesh. needed when loading material variables.
  // obtained by summing all values in pio field global_numcells
  int64_t numCells;

  // Record the ordering of the cells when building the hypertree grid
  // Needed so that the data will line up correctly
  std::vector<int> indexNodeLeaf;

  // list of material variables
  std::map<std::string, PIOMaterialVariable*> matVariables;
  int numMaterials;

  struct AdaptorImpl;
  AdaptorImpl* Impl;
};

VTK_ABI_NAMESPACE_END
#endif
