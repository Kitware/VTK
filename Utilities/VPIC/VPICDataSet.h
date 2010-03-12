/////////////////////////////////////////////////////////////////////////////
// 
// VPICDataSet class contains information for VPIC application
// for all time steps across all processors
//
/////////////////////////////////////////////////////////////////////////////

#ifndef VPICDataSet_h
#define VPICDataSet_h

#include "VPICDefinition.h"
#include "VPICGlobal.h"
#include "VPICView.h"

#include <iostream>
#include <string>

using namespace std;

class VPIC_EXPORT VPICDataSet {
public:
   VPICDataSet();
   ~VPICDataSet();

   // Initialize with global information about files, sizes and variables
   void initialize(const string& inFile);

   // Initialize a subview
   void setView(
        int* xDecomposition,    // Extent of files to include in view
        int* yDecomposition,    // Extent of files to include in view
        int* zDecomposition);   // Extent of files to include in view

   // Have each part load data into appropriate part of viz data on processor
   void loadVariableData(
        float* varData,         // Pre allocated array to fill
        int timeStep,           // Dump to load from
        int variable,           // Variable index to load
        int component);         // Component of variable to load

   bool needsGridCalculation()  { return this->view->needsGridCalculation(); }
   void calculateGridExtents()  { this->view->calculateGridExtents(); }

   // Check main directory for additional time steps and adjust structures
   void addNewTimeSteps()       { this->global.addNewTimeSteps(); }

   void PrintSelf(ostream& os, int indent);

   void setRank(int r)          { this->rank = r; }
   void setTotalRank(int t)     { this->totalRank = t; }
   int  getRank()               { return this->rank; }
   int  getTotalRank()          { return this->totalRank; }

   // Global common information
   int  getNumberOfParts()      { return this->global.getNumberOfParts(); }

   int  getNumberOfTimeSteps()  { return this->global.getNumberOfTimeSteps(); }
   int  getTimeStep(int dump)   { return this->global.getDumpTime(dump); }

   int  getNumberOfVariables()  { return this->global.getNumberOfVariables(); }
   string getVariableName(int v){ return this->global.getVariableName(v); }
   int  getVariableStruct(int v){ return this->global.getVariableStruct(v); }

   void getLayoutSize(int layout[]);

   // View specific information
   void setStride(int s[])              { this->view->setStride(s); }

   void getGridSize(int grid[])         { this->view->getGridSize(grid); }

   void getOrigin(float origin[])       { this->view->getOrigin(origin); }
   void getOrigin(double origin[])      { this->view->getOrigin(origin); }
   void getStep(float step[])           { this->view->getStep(step); }
   void getStep(double step[])          { this->view->getStep(step); }

   void getPhysicalExtent(float extent[])
                                { this->view->getPhysicalExtent(extent); }
   void getPhysicalExtent(double extent[])
                                { this->view->getPhysicalExtent(extent); }

   void getWholeExtent(int extent[])
                                { this->view->getWholeExtent(extent); }
   void getSubExtent(int piece, int extent[])
                                { this->view->getSubExtent(piece, extent); }
   void getSubDimension(int piece, int dim[])
                                { this->view->getSubDimension(piece, dim); }

   int getNumberOfCells()       { return this->view->getNumberOfCells(); }
   int getNumberOfNodes()       { return this->view->getNumberOfNodes(); }

   int getProcessorUsed();      // Is this processor used to render view

private:
   int rank;                    // Processor number
   int totalRank;               // Number of graphics processors

   VPICGlobal global;           // Global information about overall data

   VPICView* view;              // Current view
   int currentTimeStep;         // When changing view, keep the same timestep
   int curXExtent[DIMENSION];   // Current view extent
   int curYExtent[DIMENSION];   // Current view extent
   int curZExtent[DIMENSION];   // Current view extent
};

#endif
