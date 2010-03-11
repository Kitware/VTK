//////////////////////////////////////////////////////////////////////////////
//
// VPICPart class contains data for a time step on one processor
//
//////////////////////////////////////////////////////////////////////////////

#ifndef VPICPart_h
#define VPICPart_h

#include "VPICDefinition.h"
#include "VPICHeader.h"
#include <fstream>

using namespace std;

class VPIC_EXPORT VPICPart {
public:
   VPICPart(int id);
   ~VPICPart();

   // Initialize for a partition
   void setFiles(string* names, int count);
   void initialize();

   // Calculate the location of this part in the subgrid for a processor
   void calculatePartLocation(int* stride);

   // Load variable data from file part into array
   void loadVariableData(
        float* varData,         // Pre allocated array to fill
        int* subdimension,      // Dimension on this processor
        int fileKind,           // Field or species
        int basicType,          // FLOAT or INTEGER
        int byteCount,          // Size of basic type
        long int offset,        // Offset to variable to load
        int stride[]);          // Stride over the data

   // Relative offset of this part within this processor
   void setPartOffset(int x, int y, int z)
                                {
                                  this->partOffset[0] = x;
                                  this->partOffset[1] = y;
                                  this->partOffset[2] = z;
                                }

   void setSimID(int id)        { this->simID = id; }
   void setVizID(int id)        { this->vizID = id; }
   int  getSimID()              { return this->simID; }
   int  getVizID()              { return this->vizID; }

   int  getDumpTime()           { return this->header.getDumpTime(); }
   int  getNumberOfDimensions() { return this->header.getNumberOfDimensions(); }
   int  getNumberOfGhostGrids() { return this->numberOfGhostGrids; }

   void getGridSize(int gridsize[])   { this->header.getGridSize(gridsize); }
   void getGhostSize(int ghostsize[]) { this->header.getGhostSize(ghostsize); }
   void getOrigin(float origin[])     { this->header.getOrigin(origin); }
   void getStep(float step[])         { this->header.getStep(step); }

   void PrintSelf(ostream& os, int indent);

private:
   string* fileName;            // field, ehydro, hhydro data files
   int  simID;                  // Simulation processor that wrote file
   int  vizID;                  // Visualization processor that draws part

   VPICHeader header;           // Header information for part

   int  gridSize[DIMENSION];    // Grid size for this part
   int  ghostSize[DIMENSION];   // Grid size for this part with ghost border
   int  numberOfGrids;          // Size of this part of grid
   int  numberOfGhostGrids;     // Size of this part of grid with ghost cells

   int  partOffset[DIMENSION];  // Where this part fits in the processor
   int  gridOffset[DIMENSION];  // Where this part fits in the grid
};

/////////////////////////////////////////////////////////////////////////////
//
// Templated read of a basic data type from a file, to be stored in a
// block of float supplied by the visualizer
//
/////////////////////////////////////////////////////////////////////////////

template< class basicType >
void LoadData(
        float* varData,         // Grid over all parts to be filled
        basicType* block,       // Type of data to read from file
        int* subdimension,      // Subdimension for processor owning this part
        int* ghostSize,         // Dimension of data in the file
        int numberOfGhostGrids, // Amount of data for variable in file
        int* gridOffset,        // Offset with total data on proc for this part
        string fileName,        // Field or species data file
        long int offset,        // Load data from this offset
        int stride[])           // Stride over data requested
{
   // Get the file pointer to the offset for this variable and component
   // Read the contiguous variable data from the file
   FILE* filePtr = fopen(fileName.c_str(), "r");
   fseek(filePtr, offset, SEEK_SET);

   // Data read in includes ghost cells
   block = new basicType[numberOfGhostGrids];
   fread(block, sizeof(basicType), numberOfGhostGrids, filePtr);
   fclose(filePtr);

   // Iterate over all data which includes ghost cells
   // Transfer the non-ghost data to the correct offset within varData
   int varIndex;        // Index into returned visualizer block
   int blockIndex;      // Index into file part data block

   int bx, by, bz;      // Block data index from VPIC file with strides
   int vx, vy, vz;      // Visualizer data index with no strides

   // Always skip the first ghost position because value is 0
   for (bz = 1, vz = 0; bz < ghostSize[2]; bz += stride[2], vz++) {

      // Offset into entire viz data block for this file's part of data
      int offsetz = gridOffset[2] + vz;

      for (by = 1, vy = 0; by < ghostSize[1]; by += stride[1], vy++) {

         // Offset into entire viz data block for this file's part of data
         int offsety = gridOffset[1] + vy;

         for (bx = 1, vx = 0; bx < ghostSize[0]; bx += stride[0], vx++) {

            // Offset into entire viz data block for this file's part of data
            int offsetx = gridOffset[0] + vx;

            blockIndex = (bz * ghostSize[0] * ghostSize[1]) +
                         (by * ghostSize[0]) + bx;


            // Calculate the index into the sub grid for this processor
            // Store the final ghost cell unless it is beyond the subextent
            if (offsetx != subdimension[0] &&
                offsety != subdimension[1] &&
                offsetz != subdimension[2]) {

                varIndex = (offsetz * subdimension[0] * subdimension[1]) +
                           (offsety * subdimension[0]) + offsetx;

                varData[varIndex] = (float) block[blockIndex];
            }
         }
      }
   }
   delete [] block;
}

#endif
