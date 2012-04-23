/////////////////////////////////////////////////////////////////////////////
// 
// VPICHeader class contains information for a single VPIC
//
/////////////////////////////////////////////////////////////////////////////

#ifndef VPICHeader_h
#define VPICHeader_h

#include "VPICDefinition.h"
#include <iostream>
#include <string>
#include <vector>
#include <set>

#include <vtkSystemIncludes.h>

using namespace std;

class VPIC_EXPORT VPICHeader {
public:
   VPICHeader();
   VPICHeader(FILE* fp);
   ~VPICHeader();

   void PrintSelf(ostream& os, int indent);

   int readHeader(FILE* fp);
   int parseBoilerPlate(FILE* fp);

   void  getGridSize(int gridsize[]);
   void  getGhostSize(int ghostsize[]);
   void  getOrigin(float origin[]);
   void  getStep(float step[]);

   int   getDumpTime()                  { return this->dumpTime; }
   int   getNumberOfDimensions()        { return this->numberOfDimensions; }
   int   getTotalRank()                 { return this->totalRank; }

private:
   int   rank;                  // Rank of processor that wrote file
   int   totalRank;             // Total number of processor parts

   int   version;               // Version number
   int   dumpType;              // Field or hydro data
   int   headerSize;            // Number of common bytes
   int   recordSize;            // Number of bytes of data per grid
   int   numberOfDimensions;

   int   dumpTime;              // Number for this time step
   float deltaTime;             // Time step difference

   int   gridSize[DIMENSION];   // Non ghost cell size this rank
   int   ghostSize[DIMENSION];  // Total size including ghost cells
   float gridOrigin[DIMENSION]; // Actual origin for the grid
   float gridStep[DIMENSION];   // Actual step within the grid

   float cvac;
   float epsilon;
   float damp;
   int   spid;
   float spqm;

};

#endif
