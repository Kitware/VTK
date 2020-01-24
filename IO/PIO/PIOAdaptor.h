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

#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkMultiBlockDataSet.h"

#include "PIOData.h"

#include <vector>

class PIOAdaptor
{
public:
  PIOAdaptor(int rank, int totalRank);
  ~PIOAdaptor();

  int initializeGlobal(const char* DumpDescFile);
  int initializeDump(int timeStep);

  void create_geometry(vtkMultiBlockDataSet* grid);

  void load_variable_data(vtkMultiBlockDataSet* grid);

  int GetNumberOfTimeSteps() { return this->numberOfTimeSteps; }
  double GetTimeStep(int step) { return this->timeSteps[step]; }

  int GetNumberOfVariables() { return (int)this->variableName.size(); }
  const char* GetVariableName(int indx) { return this->variableName[indx].c_str(); }

protected:
  // Create the unstructured grid for tracers
  void create_tracer_UG(vtkMultiBlockDataSet* grid);

  // Create the unstructured grid for AMR
  void create_amr_UG(vtkMultiBlockDataSet* grid, int numProc, int* global_numcell,
    int numberOfCells,      // Number of cells all levels
    int* cell_level,        // Level within AMR
    int64_t* cell_daughter, // Daughter ID, 0 indicates no daughter
    double** cell_center);  // Cell center

  void create_amr_UG_1D(vtkMultiBlockDataSet* grid,
    int startCellIndx,       // Number of cells all levels
    int endCellIndx,         // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter ID, 0 indicates no daughter
    double* cell_center[1]); // Cell center

  void create_amr_UG_2D(vtkMultiBlockDataSet* grid,
    int startCellIndx,       // Number of cells all levels
    int endCellIndx,         // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter ID, 0 indicates no daughter
    double* cell_center[2]); // Cell center

  void create_amr_UG_3D(vtkMultiBlockDataSet* grid,
    int startCellIndx,       // Number of cells all levels
    int endCellIndx,         // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter ID, 0 indicates no daughter
    double* cell_center[3]); // Cell center

  // Create the hypertree grid
  void create_amr_HTG(vtkMultiBlockDataSet* grid,
    int numberOfCells,       // Number of cells all levels
    int* cell_level,         // Level within AMR
    int64_t* cell_daughter,  // Daughter
    double* cell_center[3]); // Cell center

  int count_hypertree(int64_t curIndex, int64_t* daughter);

  void build_hypertree(
    vtkHyperTreeGridNonOrientedCursor* treeCursor, int64_t curIndex, int64_t* daughter);

  // Add variable data to the unstructured grid
  void add_amr_UG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
    int64_t* daughter,      // Indicates top level cell or not
    double* data[],         // Data for all cells
    int numberOfComponents, // Number of components in data
    int numberOfCells);     // Number of cells all levels

  // Add variable data to the hypertree grid
  void add_amr_HTG_scalar(vtkMultiBlockDataSet* grid, vtkStdString varName,
    double* data[],         // Data for all cells
    int numberOfComponents, // Number of components in data
    int numberOfCells);     // Number of all cells

  // Used in parallel reader and load balancing
  int Rank;
  int TotalRank;

  // Structure to access the dump file data
  PIO_DATA* pioData;

  // Fields of interest in dump file
  std::list<std::string> fieldsToRead;

  // Time series of dumps
  std::string descFileName;  // name.pio
  std::string dumpDirectory; // directory holding dumps
  std::string dumpBaseName;  // base name to use for dumps

  std::vector<std::string> dumpFileName;
  int numberOfTimeSteps;
  double* timeSteps;
  int currentTimeStep;

  // Type of block structures to create within multiblock dataset
  bool useHTG;
  bool useTracer;

  // Requested variables for block structures
  std::vector<std::string> variableName;

  // Record the ordering of the cells when building the hypertree grid
  // Needed so that the data will line up correctly
  std::vector<int> indexNodeLeaf;
};

#endif
