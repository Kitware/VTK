#include "VPICHeader.h"

//////////////////////////////////////////////////////////////////////////////
//
// Header information for every complete file where each time step has
// a file for each processor which ran the original application
//
//////////////////////////////////////////////////////////////////////////////

VPICHeader::VPICHeader()
{
   this->headerSize = 123;
}

VPICHeader::~VPICHeader()
{
}

//////////////////////////////////////////////////////////////////////////////
//
// Read the header including consistency information and size information
// Return the header size in bytes if successful
//
//////////////////////////////////////////////////////////////////////////////

int VPICHeader::readHeader(FILE* filePtr)
{
   // Consistency check to see if file can be read on this machine
   int consistent = parseBoilerPlate(filePtr);
   if (consistent == 0) {
      cout << "Data file is not consistent on this machine" << endl;
   }

   // Version and dump type
   fread(&this->version, sizeof(int), 1, filePtr);
   fread(&this->dumpType, sizeof(int), 1, filePtr);
   if (this->dumpType != VPIC_FIELD && this->dumpType != VPIC_HYDRO)
      cout << "Bad VPIC dump type (not field or hydro)" << endl;

   // Information
   fread(&this->dumpTime, sizeof(int), 1, filePtr);
   fread(this->gridSize, sizeof(int), DIMENSION, filePtr);

   fread(&this->deltaTime, sizeof(float), 1, filePtr);
   fread(this->gridStep, sizeof(float), DIMENSION, filePtr);
   fread(this->gridOrigin, sizeof(float), DIMENSION, filePtr);
   fread(&this->cvac, sizeof(float), 1, filePtr);
   fread(&this->epsilon, sizeof(float), 1, filePtr);
   fread(&this->damp, sizeof(float), 1, filePtr);
   fread(&this->rank, sizeof(int), 1, filePtr);
   fread(&this->totalRank, sizeof(int), 1, filePtr);

   fread(&this->spid, sizeof(int), 1, filePtr);
   fread(&this->spqm, sizeof(float), 1, filePtr);

   // Array size/dimension
   fread(&this->recordSize, sizeof(int), 1, filePtr);
   fread(&this->numberOfDimensions, sizeof(int), 1, filePtr);
   fread(this->ghostSize, sizeof(int), DIMENSION, filePtr);

   return this->headerSize;
}

//////////////////////////////////////////////////////////////////////////////
//
// Read the first 23 bytes of the file to verify the word boundaries,
// sizes of numerical types, and endianness
//
//   5 bytes of sizes for long, short, int, real, double (8,2,4,4,8)
//   2 bytes where the hex word spells "cafe"
//   4 bytes where the hex word spells "deadbeef"
//   4 bytes of real with value 1.0
//   8 bytes of double with value 1.0
//
// With UNIX octal dump utility do:
//   od -b          (to see sizes)
//   od -h -j 5     (to see hex "words")
//   od -f -j 11    (to see real 1.0)
//   od -F -j 15    (to see double 1.0)
//
//////////////////////////////////////////////////////////////////////////////

int VPICHeader::parseBoilerPlate(FILE* fp)
{
   char byteSize[5];
   short int cafe;
   int deadbeef;
   float floatone;
   double doubleone;

   fread(byteSize, sizeof(char), 5, fp);
   if (byteSize[0] != sizeof(long long) ||
       byteSize[1] != sizeof(short) ||
       byteSize[2] != sizeof(int) ||
       byteSize[3] != sizeof(float) ||
       byteSize[4] != sizeof(double))
   {
      cout << "Numerical type byte sizes do not match:" << endl;
      cout << "long: " << (short) byteSize[0] << " != " 
           << sizeof(long long) << endl;
      cout << "short: " << (short) byteSize[1] << " != " 
           << sizeof(short) << endl;
      cout << "int: " << (short) byteSize[2] << " != " 
           << sizeof(int) << endl;
      cout << "float: " << (short) byteSize[3] << " != "
           << sizeof(float) << endl;
      cout << "double: " << (short) byteSize[4] << " != " 
           << sizeof(double) << endl;
      return 0;
   }

   fread(&cafe, sizeof(short int), 1, fp);
   if (cafe != (short int) 0xcafe) {
      cout << "Endianness does not match" << endl;
      return 0;
   }

   fread(&deadbeef, sizeof(int), 1, fp);
   if (deadbeef != (int) 0xdeadbeef) {
      cout << "Endianness does not match" << endl;
      return 0;
   }

   fread(&floatone, sizeof(float), 1, fp);
   if (floatone != 1.0) {
      cout << "Could not locate float 1.0" << endl;
      return 0;
   }

   fread(&doubleone, sizeof(double), 1, fp);
   if (doubleone != 1.0) {
      cout << "Could not locate double 1.0" << endl;
      return 0;
   }
   return 1;
}

//////////////////////////////////////////////////////////////////////////////
//
// Access methods
//
//////////////////////////////////////////////////////////////////////////////

void VPICHeader::getGridSize(int gridsize[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      gridsize[dim] = this->gridSize[dim];
}

void VPICHeader::getGhostSize(int ghostsize[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      ghostsize[dim] = this->ghostSize[dim];
}

void VPICHeader::getOrigin(float origin[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      origin[dim] = this->gridOrigin[dim];
}

void VPICHeader::getStep(float step[])
{
   for (int dim = 0; dim < DIMENSION; dim++)
      step[dim] = this->gridStep[dim];
}

//////////////////////////////////////////////////////////////////////////////
//
// Print header information
//
//////////////////////////////////////////////////////////////////////////////

void VPICHeader::PrintSelf(ostream& os, int vpicNotUsed(indent))
{
   os << "Version: " << this->version << endl;
   if (this->dumpType == VPIC_FIELD)
      os << "Dump type: VPIC FIELD DATA" << endl;
   else
      os << "Dump type: VPIC HYDRO DATA" << endl;
   os << "step: " << this->dumpTime << endl;
   for (int i = 0; i < DIMENSION; i++)
      os << "Grid size[" << i << "]: " << this->gridSize[i] << endl;
   os << "Delta time: " << this->deltaTime << endl;
   for (int i = 0; i < DIMENSION; i++)
      os << "Delta grid[" << i << "]: " << this->gridStep[i] << endl;
   for (int i = 0; i < DIMENSION; i++)
      os << "Origin grid[" << i << "]: " << this->gridOrigin[i] << endl;
   os << "cvac: " << this->cvac << endl;
   os << "epsilon: " << this->epsilon << endl;
   os << "damp: " << this->damp << endl;
   os << "Rank: " << this->rank << endl;
   os << "Total ranks: " << this->totalRank << endl;
   os << "spid: " << this->spid << endl;
   os << "spqm: " << this->spqm << endl;
   os << "Record size: " << this->recordSize << endl;
   os << "Number of dimensions: " << this->numberOfDimensions << endl;
   for (int i = 0; i < DIMENSION; i++) {
      os << "Ghost grid size[" << i << "]: " 
         << this->ghostSize[i] << endl;
   }
}
