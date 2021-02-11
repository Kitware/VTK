/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile PIOAdaptor.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef PIOAdaptor_h
#define PIOAdaptor_h

#include "vtkDataArraySelection.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkMultiBlockDataSet.h"

#include "PIOData.h"

#include <vector>

class vtkMultiProcessController;

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
  int collectMetaData(const char* DumpDescFile);
  void collectVariableMetaData();
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

  // Fields of interest in dump file
  std::list<std::string> fieldsToRead;

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

  // Record the ordering of the cells when building the hypertree grid
  // Needed so that the data will line up correctly
  std::vector<int> indexNodeLeaf;
};

#endif
