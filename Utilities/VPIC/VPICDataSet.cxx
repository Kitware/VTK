#include "VPICDataSet.h"

//////////////////////////////////////////////////////////////////////////////
//
// Top level structure for VPIC data file components
//
//////////////////////////////////////////////////////////////////////////////

VPICDataSet::VPICDataSet()
{
   this->rank = 0;
   this->totalRank = 1;
   this->currentTimeStep = 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////

VPICDataSet::~VPICDataSet()
{
}

//////////////////////////////////////////////////////////////////////////////
//
// Initialize an empty VPICDataSet by setting variable names and
// checking for block or point structured data
//
//////////////////////////////////////////////////////////////////////////////

void VPICDataSet::initialize(const string& inFile)
{
   // Read the information about variables in the run from the .vpc file
   this->global.readGlobal(inFile);

   // Build all name information for file access
   this->global.buildFileNames();

   // Build the table which shows distribution of files over problem space
   this->global.buildFileLayoutTable();

   // Initialize the variable structures
   this->global.initializeVariables();

   // Create the initial view which is the entire problem
   this->view = new VPICView(this->rank, this->totalRank, this->global);
   this->view->initialize(
                this->currentTimeStep,
                this->global.getLayoutSize(),
                this->global.getLayoutID(),
                this->global.getPartSize(),
                this->global.getPhysicalOrigin(),
                this->global.getPhysicalStep());

   // Save the initial view extents which are the entire problem
   int* layoutSize = this->global.getLayoutSize();
   this->curXExtent[0] = 0;     this->curXExtent[1] = layoutSize[0] - 1;
   this->curYExtent[0] = 0;     this->curYExtent[1] = layoutSize[1] - 1;
   this->curZExtent[0] = 0;     this->curZExtent[1] = layoutSize[2] - 1;
}

//////////////////////////////////////////////////////////////////////////////
//
// If the extents of decomposition are different from those of the total
// problem, add a view and set current view to point at it
// extent[DIMENSION][low|high]
//
//////////////////////////////////////////////////////////////////////////////

void VPICDataSet::setView(int* xExtent, int* yExtent, int* zExtent)
{
   // If extents haven't been set yet return
   if (xExtent[0] == -1)
      return;

   // If the view extents have not been changed return
   if (xExtent[0] == curXExtent[0] && xExtent[1] == curXExtent[1] &&
       yExtent[0] == curYExtent[0] && yExtent[1] == curYExtent[1] &&
       zExtent[0] == curZExtent[0] && zExtent[1] == curZExtent[1])
            return;

   // Fetch the global information about the problem size and decomposition
   // int* layoutSize = this->global.getLayoutSize(); not used?
   int*** layoutID = this->global.getLayoutID();
   int* partSize = this->global.getPartSize();
   float* origin = this->global.getPhysicalOrigin();
   float* step = this->global.getPhysicalStep();

   // Verify that the view extents requested match the extents available
   if (xExtent[1] < xExtent[0]) xExtent[1] = xExtent[0];
   if (yExtent[1] < yExtent[0]) yExtent[1] = yExtent[0];
   if (zExtent[1] < zExtent[0]) zExtent[1] = zExtent[0];

   // Save the new current extents
   this->curXExtent[0] = xExtent[0];    this->curXExtent[1] = xExtent[1];
   this->curYExtent[0] = yExtent[0];    this->curYExtent[1] = yExtent[1];
   this->curZExtent[0] = zExtent[0];    this->curZExtent[1] = zExtent[1];

   // Set the layout size for the new view
   int subLayoutSize[DIMENSION];
   subLayoutSize[0] = xExtent[1] - xExtent[0] + 1;
   subLayoutSize[1] = yExtent[1] - yExtent[0] + 1;
   subLayoutSize[2] = zExtent[1] - zExtent[0] + 1;

   // Create the matching file ID structure to match the extents in the new view
   int*** subLayoutID = new int**[subLayoutSize[0]];
   for (int i = 0; i < subLayoutSize[0]; i++) {
      subLayoutID[i] = new int*[subLayoutSize[1]];
      for (int j = 0; j < subLayoutSize[1]; j++)
         subLayoutID[i][j] = new int[subLayoutSize[2]];
   }

   // Assign the file IDs controlled by this view
   int kindx = 0;
   for (int k = zExtent[0]; k <= zExtent[1]; k++) {
      int jindx = 0;
      for (int j = yExtent[0]; j <= yExtent[1]; j++) {
         int iindx = 0;
         for (int i = xExtent[0]; i <= xExtent[1]; i++) {
            subLayoutID[iindx][jindx][kindx] = layoutID[i][j][k];
            iindx++;
         }
         jindx++;
      }
      kindx++;
   }

   float subOrigin[DIMENSION];
   subOrigin[0] = origin[0] + (xExtent[0] * partSize[0] * step[0]);
   subOrigin[1] = origin[1] + (yExtent[0] * partSize[1] * step[1]);
   subOrigin[2] = origin[2] + (zExtent[0] * partSize[2] * step[2]);

   // Create a new view with new size and file IDs
   delete this->view;
   this->view = new VPICView(this->rank, this->totalRank, this->global);
   this->view->initialize(
                this->currentTimeStep,
                subLayoutSize,
                subLayoutID,
                this->global.getPartSize(),
                subOrigin,
                this->global.getPhysicalStep());
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

void VPICDataSet::loadVariableData(
        float* varData,
        int timeStep,
        int variable,
        int component)
{
   this->currentTimeStep = timeStep;
   this->view->loadVariableData(varData, timeStep, variable, component);
}

//////////////////////////////////////////////////////////////////////////////
//
// Access methods
//
//////////////////////////////////////////////////////////////////////////////

void VPICDataSet::getLayoutSize(int size[])
{
  int* layoutSize = this->global.getLayoutSize();
  for (int dim = 0; dim < DIMENSION; dim++)
    size[dim] = layoutSize[dim];
}

//////////////////////////////////////////////////////////////////////////////
//
// Is this processor used to render the current view
//
//////////////////////////////////////////////////////////////////////////////

int VPICDataSet::getProcessorUsed()
{
   if (this->view->getNumberOfParts() == 0)
      return 0;
   else
      return 1;
}

//////////////////////////////////////////////////////////////////////////////
//
// Print information about the data set
//
//////////////////////////////////////////////////////////////////////////////

void VPICDataSet::PrintSelf(ostream& os, int indent)
{
   if (this->rank == 0) {
      os << endl;
      this->global.PrintSelf(os, indent);
   }
}
