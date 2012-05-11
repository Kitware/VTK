#include "VPICPart.h"

/////////////////////////////////////////////////////////////////////////////
//
// Create structure to hold field data for one time step, one processor
//
/////////////////////////////////////////////////////////////////////////////

VPICPart::VPICPart(int part)
{
   this->simID = part;
   this->vizID = 0;
   this->fileName = 0;
}

void VPICPart::setFiles(string* name, int numberOfFiles)
{
   if (this->fileName != 0)
      delete [] this->fileName;
   this->fileName = new string[numberOfFiles];
   for (int i = 0; i < numberOfFiles; i++) {
      this->fileName[i] = name[i];
   }
}

void VPICPart::initialize()
{
   FILE* filePtr = fopen(this->fileName[0].c_str(), "r");
   if (filePtr == NULL) {
      cerr << "Could not open file " << this->fileName[0] << endl;
   }

   // Header verifies consistency and gets sizes
   this->header.readHeader(filePtr);
   fclose(filePtr);

   // Count the number of data items per variable
   this->header.getGridSize(this->gridSize);
   this->header.getGhostSize(this->ghostSize);

   this->numberOfGrids = 1;
   this->numberOfGhostGrids = 1;
   for (int dim = 0; dim < DIMENSION; dim++) {
      this->numberOfGrids *= this->gridSize[dim];
      this->numberOfGhostGrids *= this->ghostSize[dim];
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////

VPICPart::~VPICPart()
{
   if (this->fileName != 0)
      delete [] this->fileName;
}

//////////////////////////////////////////////////////////////////////////////
//
// Using the offset of this part within a processor calculate the grid
// offset for this part within the processor grid, taking stride into account
//
//////////////////////////////////////////////////////////////////////////////

void VPICPart::calculatePartLocation(int* stridedPartSize)
{
   for (int dim = 0; dim < DIMENSION; dim++)
      this->gridOffset[dim] = this->partOffset[dim] * stridedPartSize[dim];
}

//////////////////////////////////////////////////////////////////////////////
//
// Load the data for this part into the correct position in an overall grid
// which has been preallocated.  All data is read from the file but only the
// requested stride is copied to the visualizer's array as a float
// Each file contains ghost information for one cell on each side for each
// dimension.  Skip those ghost cells and only fill in internal data.
// Many files will contribute to the data for one processor so use the
// offset for each file part to determine where to fill in the data.
//
//////////////////////////////////////////////////////////////////////////////

void VPICPart::loadVariableData(
        float* varData,         // Grid over all parts to be filled
        int varOffset,          // Offset into varData for loading
        int* subdimension,      // Subdimension for processor owning this part
        int fileKind,           // Field or species data
        int basicType,          // FLOAT or INTEGER
        int byteCount,          // Size of basic type
        long int offset,        // Load data from this offset
        int stride[])           // Stride over data requested
{
   string name = this->fileName[fileKind];

   // Part stores data plus ghost cells, get all information about them
   int localghostSize[DIMENSION];
   this->header.getGhostSize(localghostSize);

   if (basicType == FLOAT && byteCount == 4) {
      float* block = NULL;
      LoadData(vizID, simID, varData, varOffset, block, subdimension,
               localghostSize, this->numberOfGhostGrids,
               this->gridOffset, name, offset, stride);

   } else if (basicType == FLOAT && byteCount == 8) {
      double* block = NULL;
      LoadData(vizID, simID, varData, varOffset, block, subdimension,
               localghostSize, this->numberOfGhostGrids,
               this->gridOffset, name, offset, stride);

   } else if (basicType == INTEGER && byteCount == 4) {
      int* block = NULL;
      LoadData(vizID, simID, varData, varOffset, block, subdimension,
               localghostSize, this->numberOfGhostGrids,
               this->gridOffset, name, offset, stride);

   } else if (basicType == INTEGER && byteCount == 2) {
      short* block = NULL;
      LoadData(vizID, simID, varData, varOffset, block, subdimension,
               localghostSize, this->numberOfGhostGrids,
               this->gridOffset, name, offset, stride);
   }
}

/////////////////////////////////////////////////////////////////////////////
//
// Print information about this object
//
/////////////////////////////////////////////////////////////////////////////

void VPICPart::PrintSelf(ostream& os, int indent)
{
   this->header.PrintSelf(os, indent);
}
