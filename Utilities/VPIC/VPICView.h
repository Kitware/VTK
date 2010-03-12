/////////////////////////////////////////////////////////////////////////////
// 
// VPICView class contains information for a subset of a VPIC application
// for all time steps across all processors.  That subset might be the
// entire dataset.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef VPICView_h
#define VPICView_h

#include "VPICDefinition.h"
#include "VPICGlobal.h"
#include "VPICHeader.h"
#include "VPICPart.h"
#include <iostream>
#include <string>
#include <vector>
#include <set>

using namespace std;

class VPIC_EXPORT VPICView {
public:
   VPICView(int r, int t, VPICGlobal& global);
   ~VPICView();

   // Initialize the view which is total dataset or a subset
   void initialize(
        int timeStep,           // Current time step
        int* layoutSize,        // Dimensions in complete files
        int*** layoutID,        // File ids included in the view
        int* partSize,          // Size of data on one file
        float* origin,          // Physical origin
        float* step);           // Physical step

   // Partition the subset of files across available processors
   void partitionFiles();
   void partition();
   void getPartFileNames(string* partFileName, int time, int part);

   // Set grid sizes, origin, step based on stride over problem
   void calculateGridExtents();

   // Have each part load data into appropriate part of viz data on processor
   void loadVariableData(
        float* varData,         // Pre allocated array to fill
        int timeStep,           // Dump to load from
        int variable,           // Variable index to load
        int component);         // Component of variable to load

   bool needsGridCalculation()  { return this->calculateGridNeeded; }

   // Check main directory for additional time steps and adjust structures
   void addNewTimeSteps();

   void PrintSelf(ostream& os, int indent);

   // Setting the stride requires recalculation of grid and extents
   void setStride(int s[]);

   void getGridSize(int gridsize[]);
   void getLayoutSize(int layout[]);

   void getOrigin(float origin[]);
   void getOrigin(double origin[]);

   void getStep(float step[]);
   void getStep(double step[]);

   void getPhysicalExtent(float extent[]);
   void getPhysicalExtent(double extent[]);

   void getWholeExtent(int extent[]);
   void getSubExtent(int piece, int extent[]);
   void getSubDimension(int piece, int dimension[]);

   int  getNumberOfCells()      { return this->numberOfCells; }
   int  getNumberOfNodes()      { return this->numberOfNodes; }
   int  getNumberOfParts()      { return this->numberOfMyParts; }

private:
   VPICGlobal& global;                  // Common information for overall data

   int   rank;                          // Processor number
   int   totalRank;                     // Number of graphics processors

   // Visualization information
   int   gridSize[DIMENSION];           // Visualization grid size for all parts
   int   ghostSize[DIMENSION];          // Visualization ghost grid size

   float physicalOrigin[DIMENSION];     // Physical origin
   float physicalStep[DIMENSION];       // Physical step
   float physicalSize[DIMENSION];       // Physical upper extent

   int   numberOfCells;                 // Visualization grid positions
   int   numberOfCellsWithGhosts;       // Visualization ghost grid positions
   int   numberOfNodes;                 // Points within grid (gridsize + 1)

   int   stride[DIMENSION];             // Stride in each dimension
   int   currentTimeStep;               // VPICParts set to this step

   // Graphic processor partition information
   int** range;                         // Range of layoutSize for processor
   int** subextent;                     // Subextent grid for every processor
   int** subdimension;                  // Subdimension for every processor

   bool  calculateGridNeeded;           // Grid extent calculation required

   // Data access structure
   int*** layoutID;                     // Numerical ID of file
   int layoutSize[DIMENSION];           // # of parts in each dim of simulation
   int partSize[DIMENSION];             // # of grids in each dim per part

   vector<VPICPart*> myParts;           // Every VPICPart this processor handles
   int numberOfMyParts;

  VPICView(const VPICView&);  // Not implemented.
  void operator=(const VPICView&);  // Not implemented.
};

#endif
