#include "VPICView.h"
#include "VPICGlobal.h"

#include <sys/types.h>
#include <set>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "math.h"

#ifdef _WIN32
const static char * Slash = "\\";
#else
const static char * Slash = "/";
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Structure for view of VPIC data file components
//
//////////////////////////////////////////////////////////////////////////////

VPICView::VPICView(int r, int t, VPICGlobal& dsGlobal) :
                rank(r),
                totalRank(t),
                global(dsGlobal),
                calculateGridNeeded(true)
{
  for (int i = 0; i < DIMENSION; ++i)
    {
    this->stride[i] = 1;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////

VPICView::~VPICView()
{
   for (int i = 0; i < this->layoutSize[0]; i++) {
      for (int j = 0; j < this->layoutSize[1]; j++) {
         delete [] this->layoutID[i][j];
      }
      delete [] this->layoutID[i];
   }
   delete [] this->layoutID;

   for (int i = 0; i < this->totalRank; i++) {
      delete [] this->range[i];
      delete [] this->subextent[i];
      delete [] this->subdimension[i];
   }
   delete [] this->range;
   delete [] this->subextent;
   delete [] this->subdimension;

   for (int i = 0; i < this->numberOfMyParts; i++)
      delete this->myParts[i];
}

//////////////////////////////////////////////////////////////////////////////
//
// Initialize a view with the layoutSize which is number of files in each
// dimension, the layoutID matrix corresponding to the layout, and the
// stride over this view
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::initialize(
                int timeStep,
                int* dsLayoutSize,
                int*** dsLayoutID,
                int* dsPartSize,
                float* dsPhysicalOrigin,
                float* dsPhysicalStep)
{
   // View uses the current time step
   this->currentTimeStep = timeStep;

   // Size specific information for this view
   for (int dim = 0; dim < DIMENSION; dim++) {
      this->layoutSize[dim] = dsLayoutSize[dim];
      this->partSize[dim] = dsPartSize[dim];

      this->physicalOrigin[dim] = dsPhysicalOrigin[dim];
      this->physicalStep[dim] = dsPhysicalStep[dim];

      int gridCount = this->layoutSize[dim] * this->partSize[dim];
      this->physicalSize[dim] = gridCount * this->physicalStep[dim];
   }

   // Allocate the partition ID table with one entry for every file
   this->layoutID = new int**[this->layoutSize[0]];
   for (int i = 0; i < this->layoutSize[0]; i++) {
      this->layoutID[i] = new int*[this->layoutSize[1]];
      for (int j = 0; j < this->layoutSize[1]; j++)
         this->layoutID[i][j] = new int[this->layoutSize[2]];
   }

   for (int k = 0; k < this->layoutSize[2]; k++)
      for (int j = 0; j < this->layoutSize[1]; j++)
         for (int i = 0; i < this->layoutSize[0]; i++)
            this->layoutID[i][j][k] = dsLayoutID[i][j][k];

   // Partition graphics processors across this view
   partitionFiles();
}

//////////////////////////////////////////////////////////////////////////////
//
// Given the already built file layout table and the number of processors
// to partition the display across and the number of this processor,
// assign parts and set part offsets within the processor's subextent
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::partitionFiles()
{
   // First and last index into partition table for each processor
   this->range = new int*[this->totalRank];
   this->subextent = new int*[this->totalRank];
   this->subdimension = new int*[this->totalRank];

   for (int piece = 0; piece < this->totalRank; piece++) {
      this->range[piece] = new int[DIMENSION*2];
      this->subextent[piece] = new int[DIMENSION*2];
      this->subdimension[piece] = new int[DIMENSION];
      for (int i = 0; i < DIMENSION*2; i++) {
         this->range[piece][i] = -1;
         this->subextent[piece][i] = 0;
      }
   }

   /*
   if (this->rank == 0) {
      cout << endl << "New partition of files" << endl;
      cout << "File grid size: ["
           << this->partSize[0] << ","
           << this->partSize[1] << ","
           << this->partSize[2] << "]" << endl;
      cout << "Simulation decomposition: ["
           << this->layoutSize[0] << ","
           << this->layoutSize[1] << ","
           << this->layoutSize[2] << "]" << endl;
   }
   */

   // Partition graphics processors over the file decomposition
   partition();

   // Decomposition ranges indicate the index number of the name of
   // the file which is read on a particular processor
   string* partFileNames = new string[this->global.getNumberOfDirectories()];

   // Relative offset of part within one processor for calculating the grid
   // offset to place data at for display
   int iindx, jindx, kindx;
   kindx = 0;

   // If the partitioned range has -1 in it, this processor is not used
   if (this->range[this->rank][0] != -1) {
      for (int k = this->range[this->rank][4];
               k <= this->range[this->rank][5]; k++, kindx++) {
         jindx = 0;
         for (int j = this->range[this->rank][2];
                  j <= this->range[this->rank][3]; j++, jindx++) {
            iindx = 0;
            for (int i = this->range[this->rank][0];
                     i <= this->range[this->rank][1]; i++, iindx++) {

               // Create the VPICPart for this processor which will have the
               // file names containing data for its part of the total and
               // will have its offset within one graphics processor so that
               // data is read into the correct spot

               int part = this->layoutID[i][j][k];
               getPartFileNames(partFileNames, this->currentTimeStep, part);

               VPICPart* vpicPart = new VPICPart(part);
               vpicPart->setFiles(partFileNames,
                                  this->global.getNumberOfDirectories());
               vpicPart->initialize();
               vpicPart->setVizID(this->rank);
               vpicPart->setPartOffset(iindx, jindx, kindx);
               this->myParts.push_back(vpicPart);
            }
         }
      }
   }
   this->numberOfMyParts = static_cast<int>(this->myParts.size());
   delete [] partFileNames;
}

//////////////////////////////////////////////////////////////////////////////
//
// Partition files into the number of processors
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::partition()
{
   int totalParts = 1;
   for (int dim = 0; dim < DIMENSION; dim++)
      totalParts *= this->layoutSize[dim];

   // One graphics processor gets entire range
   for (int dim = 0; dim < DIMENSION; dim++)
      this->decomposition[dim] = 1;

   // More than one graphics processors
   if (this->totalRank > 1) {

      // Number of graphics processor is <= number of parts
      if (totalParts <= this->totalRank) {
         for (int dim = 0; dim < DIMENSION; dim++)
            this->decomposition[dim] = this->layoutSize[dim];
      }

      // Number of graphics processors is > number of parts
      else {
         int rangeSize[DIMENSION];
         for (int dim = 0; dim < DIMENSION; dim++)
            rangeSize[dim] = this->layoutSize[dim];

         int gcd[DIMENSION];
         int processorFactor = this->totalRank;
         bool done = false;

         // Use greatest common divisor to factor processors and decomposition
         while (processorFactor > 1 && done == false) {
            int maxGCD = 1;
            int maxGCDdim = 0;
            for (int dim = 0; dim < DIMENSION; dim++) {
               gcd[dim] = GCD(rangeSize[dim], processorFactor);
               if (gcd[dim] > maxGCD) {
                  maxGCD = gcd[dim];
                  maxGCDdim = dim;
               }
            }
            if (maxGCD == 1)
               done = true;

            // Apply the GCD to the number of processor and selected dimension
            processorFactor /= maxGCD;
            this->decomposition[maxGCDdim] *= maxGCD;
            rangeSize[maxGCDdim] = rangeSize[maxGCDdim] / maxGCD;
         }

         // If only divisor is 1 then divide unevenly
         if (processorFactor > 1) {
            // Choose the largest part dimension for the remaining processors
            int maxDim = 0;
            int maxSize = rangeSize[0];
            for (int dim = 1; dim < DIMENSION; dim++) {
               if (rangeSize[dim] > maxSize) {
                  maxSize = rangeSize[dim];
                  maxDim = dim;
               }
            }
            this->decomposition[maxDim] *= processorFactor;
         }
         // Make sure processoLayout is not larger than file layoutSize
         for (int dim = 0; dim < DIMENSION; dim++)
            if (this->decomposition[dim] > layoutSize[dim])
               this->decomposition[dim] = layoutSize[dim];
      }
   }
   /*
   if (this->rank == 0) {
      cout << "Graphics decomposition: ["
           << this->decomposition[0] << ","
           << this->decomposition[1] << ","
           << this->decomposition[2] << "]" << endl;
   }
   */

   // Using the part partition and the processor partition assign
   // part ranges for each processor which will be used for subextents
   // Note that the order of processors assigned has to be kept which
   // means assigning
   //     0   2   1   3
   //     4   6   5   7
   // in a block will cause trouble at least for EnSight where a row of
   // ghost cells will not be correct at the 2-1 6-5 boundary
   //

   // Calculate the number of files per processor and the number of
   // processors that need one more than this for a good distribution
   int step[DIMENSION];
   int needMore[DIMENSION];
   for (int dim = 0; dim < DIMENSION; dim++) {
      step[dim] = (int) floor((double) this->layoutSize[dim] /
                              (double) this->decomposition[dim]);
      needMore[dim] = this->layoutSize[dim] -
                      (step[dim] * this->decomposition[dim]);
   }

   int zStart = 0;
   for (int z = 0; z < this->decomposition[2]; z++) {

      int zStep = step[2];
      if (z < needMore[2])
         zStep++;

      int yStart = 0;
      for (int y = 0; y < this->decomposition[1]; y++) {

         int yStep = step[1];
         if (y < needMore[1])
            yStep++;

         int xStart = 0;
         for (int x = 0; x < this->decomposition[0]; x++) {

            int xStep = step[0];
            if (x < needMore[0])
               xStep++;

            int proc = z * (this->decomposition[0] * this->decomposition[1]) +
                       y * this->decomposition[0] + x;

            if (proc < totalRank) {
               this->range[proc][0] = xStart;
               this->range[proc][1] = xStart + xStep - 1;

               this->range[proc][2] = yStart;
               this->range[proc][3] = yStart + yStep - 1;

               this->range[proc][4] = zStart;
               this->range[proc][5] = zStart + zStep - 1;
            }
            xStart += xStep;
         }
         yStart += yStep;
      }
      zStart += zStep;
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Partitioning has already been done so calculate grid extents for this
// processor with the given stride, and calculate the offset of each part
// within this processor's subextent of the total grid
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::calculateGridExtents()
{
   // Reset to false so this won't execute unless the stride changes
   this->calculateGridNeeded = false;

   // Calculate the total grid, processor grid and part grid for the
   // current stride.  Since we want processors to continue to control
   // only their files start at the part level to calculate the grid and
   // multiply to get the higher level grid sizes.
   int stridedPartSize[DIMENSION];
   for (int dim = 0; dim < DIMENSION; dim++)
      stridedPartSize[dim] = this->partSize[dim] / this->stride[dim];

   // Total problem grid
   this->numberOfCells = 1;
   this->numberOfCellsWithGhosts = 1;
   this->numberOfNodes = 1;

   for (int dim = 0; dim < DIMENSION; dim++) {
      this->gridSize[dim] = stridedPartSize[dim] * this->layoutSize[dim];
      this->ghostSize[dim] = this->gridSize[dim] + 2;

      this->physicalStep[dim] = this->physicalSize[dim] /
                                this->gridSize[dim];

      this->numberOfCells *= this->gridSize[dim];
      this->numberOfCellsWithGhosts *= this->ghostSize[dim];
      this->numberOfNodes *= (this->gridSize[dim] + 1);
   }

   // At this point we have a range partition for each processor
   // Find the subextent for every processor within the range
   // Take into account the stride on the regular (non ghost) data
   for (int piece = 0; piece < this->totalRank; piece++) {
      for (int dim = 0; dim < DIMENSION; dim++) {

         int first = dim * 2;
         int last = first + 1;

         if (this->range[piece][first] == -1) {
            this->subextent[piece][first] = 0;
            this->subextent[piece][last] = 0;
            this->subdimension[piece][dim] = 0;
         } else {
            this->subextent[piece][first] =
              this->range[piece][first] * stridedPartSize[dim];
            this->subextent[piece][last] =
              (this->range[piece][last] + 1) * stridedPartSize[dim];

            if (this->subextent[piece][first] < 0)
               this->subextent[piece][first] = 0;
            if (this->subextent[piece][last] >= this->gridSize[dim])
               this->subextent[piece][last] = this->gridSize[dim] - 1;
            this->subdimension[piece][dim] = this->subextent[piece][last] -
                                             this->subextent[piece][first] + 1;
         }
      }
   }

   // Each part calculates where it fits in the overall grid for processor
   // Must take into account the stride which affects the offset in subgrid
   for (int i = 0; i < this->numberOfMyParts; i++)
      this->myParts[i]->calculatePartLocation(stridedPartSize);
}

//////////////////////////////////////////////////////////////////////////////
//
// Load the variable data for the given time step for this processor
// Each processor has many file parts which supply pieces of data
// Have each file part load into the overall data block by using its
// offset into that data block.  Each data part has a set format but
// in order to do different time steps, change the name of the file
// which is to be accessed
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::loadVariableData(
        float* varData,
        int varOffset,
        int* _subdimension,
        int timeStep,
        int var,
        int comp)
{
   // Change the files in my VPICParts if the time step has changed
   if (timeStep != this->currentTimeStep) {

      this->currentTimeStep = timeStep;

      // Each part will access a file for field and one for each species
      string* partFileNames = new string[this->global.getNumberOfDirectories()];

      for (int part = 0; part < this->numberOfMyParts; part++) {
         int id = this->myParts[part]->getSimID();
         getPartFileNames(partFileNames, this->currentTimeStep, id);
         this->myParts[part]->setFiles(partFileNames,
                                       this->global.getNumberOfDirectories());
      }
      delete [] partFileNames;
   }

   // Read the variable data from file and store into overall var_array
   // Load the appropriate part of the data from the part
   for (int part = 0; part < this->numberOfMyParts; part++) {
      this->myParts[part]->loadVariableData(
                            varData,
                            varOffset,
                            _subdimension,
                            this->global.getVariableKind(var),
                            this->global.getVariableType(var),
                            this->global.getVariableByteCount(var),
                            this->global.getVariableOffset(var, comp),
                            stride);
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Set an array of file names for a specific part to access fields and
// all species
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::getPartFileNames(string* partFileName, int timeStep, int part)
{
   int timeFieldLen = this->global.getTimeFieldLen();
   int procFieldLen = this->global.getProcFieldLen();
   int dumpTime = this->global.getDumpTime(timeStep);
   string dumpName = this->global.getDumpName(timeStep);

   for (int i = 0; i < this->global.getNumberOfDirectories(); i++) {
      ostringstream name;
      name << this->global.getDirectoryName(i)
           << dumpName << Slash
           << this->global.getBaseFileName(i) << ".";

      if (timeFieldLen == 1)
         name << dumpTime << ".";
      else
         name << setw(timeFieldLen) << setfill('0') << dumpTime << ".";

      if (procFieldLen == 1)
         name << part;
      else
         name << setw(procFieldLen) << setfill('0') << part;

      partFileName[i] = name.str();
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Access methods
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::getDecomposition(int decomp[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      decomp[dim] = this->decomposition[dim];
}

void VPICView::getGridSize(int gridsize[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      gridsize[dim] = this->gridSize[dim];
}

void VPICView::getLayoutSize(int layout[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      layout[dim] = this->layoutSize[dim];
}

void VPICView::getOrigin(float origin[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      origin[dim] = this->physicalOrigin[dim];
}

void VPICView::getOrigin(double origin[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      origin[dim] = (double) this->physicalOrigin[dim];
}

void VPICView::getStep(float step[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      step[dim] = this->physicalStep[dim];
}

void VPICView::getStep(double step[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      step[dim] = (double) this->physicalStep[dim];
}

void VPICView::getPhysicalExtent(float extent[])
{
   for (int dim = 0; dim < DIMENSION; dim++) {
      extent[dim*2] = this->physicalOrigin[dim];
      extent[dim*2+1] = this->physicalOrigin[dim] +
                        (this->gridSize[dim] * this->physicalStep[dim]);
   }
}

void VPICView::getPhysicalExtent(double extent[])
{
   for (int dim = 0; dim < DIMENSION; dim++) {
      extent[dim*2] = (double) this->physicalOrigin[dim];
      extent[dim*2+1] = (double) (this->physicalOrigin[dim] +
                        (this->gridSize[dim] * this->physicalStep[dim]));
   }
}

void VPICView::getWholeExtent(int extent[])
{
   for (int dim = 0; dim < DIMENSION; dim++) {
      extent[dim*2] = 0;
      extent[dim*2+1] = this->gridSize[dim] - 1;
   }
}

void VPICView::getSubExtent(int piece, int extent[])
{
   for (int ext = 0; ext < 6; ext++)
      extent[ext] = this->subextent[piece][ext];
}

void VPICView::getSubDimension(int piece, int dimension[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      dimension[dim] = this->subdimension[piece][dim];
}

//////////////////////////////////////////////////////////////////////////////
//
// Reset the stride and set flag to indicate repartition is needed to
// calculate new extents
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::setStride(int s[])
{
   // Stride was not changed
   if (this->stride[0] == s[0] &&
       this->stride[1] == s[1] &&
       this->stride[2] == s[2])
          return;

   int oldStride[DIMENSION];
   for (int dim = 0; dim < DIMENSION; dim++)
      oldStride[dim] = this->stride[dim];

   // Since we stride on individual file parts make sure requested stride fits
   for (int dim = 0; dim < DIMENSION; dim++) {
      this->stride[dim] = s[dim];
      if (s[dim] > this->partSize[dim])
         this->stride[dim] = this->partSize[dim];
   }

   if (oldStride[0] != this->stride[0] ||
       oldStride[1] != this->stride[1] ||
       oldStride[2] != this->stride[2])
         this->calculateGridNeeded = true;

   /*
   if (this->rank == 0)
      cout << "Stride set to (" << this->stride[0] << ","
           << this->stride[1] << "," << this->stride[2] << ")" << endl;
   */
}

//////////////////////////////////////////////////////////////////////////////
//
// Print information about the data set
//
//////////////////////////////////////////////////////////////////////////////

void VPICView::PrintSelf(ostream& os, int vpicNotUsed(indent))
{
   if (this->rank == 0) {
      os << endl;
      os << "Stride: [" << this->stride[0] << "," << this->stride[1] << ","
                        << this->stride[2] << "]" << endl << endl;
   }
}
