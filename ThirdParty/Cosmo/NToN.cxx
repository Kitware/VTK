//////////////////////////////////////////////////////////////////////////////
//
// Read a series of .cosmo binary file files and redistribute into 
// N rectilinear parts such that there is a locality of particles in the files
// decomposed over a space with either X or Z varying fastest
//
// This will allow reading the files one-to-one in ParticleDistribute
//
// Format of the binary file is:
//	X location	(POSVEL_T bit float)
//	X velocity	(POSVEL_T bit float)
//	Y location	(POSVEL_T bit float)
//	Y velocity	(POSVEL_T bit float)
//	Z location	(POSVEL_T bit float)
//	X velocity	(POSVEL_T bit float)
//	Particle mass	(POSVEL_T bit float)
//	ID value	(ID_T bit int)
//
//////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <math.h>
#include <dirent.h>
#include <vector>

#include "BasicDefinition.h"

using namespace std;
using namespace cosmologytools;

////////////////////////////////////////////////////////////////////////////
//
// Given the base name file all input files in this directory
//
////////////////////////////////////////////////////////////////////////////

void findInputFiles(string baseFile, vector<string>* files)
{
  // Find number of input files for this problem given the base input name
  // Get the subdirectory name containing the input files
  string::size_type dirPos = baseFile.rfind("/");
  string subdirectory;
  string baseName;

  // If the directory is not given use the current directory
  if (dirPos == string::npos) {
    subdirectory = "./";
    baseName = baseFile;
  } else {
    subdirectory = baseFile.substr(0, dirPos + 1);
    baseName = baseFile.substr(dirPos + 1);
  }

  // strip everything back to the first non-number
  string::size_type pos = baseName.size() - 1;
  int numbersOK = 1;

  while (numbersOK) {
    if (baseName[pos] >= '0' && baseName[pos] <= '9') {
      if (pos > 0) {
        pos = pos - 1;
      } else {
        break;
      }
    } else {
      numbersOK = 0;
    }
  }

  // base name is everything up to the numbers
  baseName = baseName.substr(0, pos + 1);

  // Open the subdirectory and make a list of input files
  DIR* directory = opendir(subdirectory.c_str());
  struct dirent* directoryEntry;

  if (directory != NULL) {
    while ((directoryEntry = readdir(directory))) {
      // get the name
      string fileName = directoryEntry->d_name;
      pos = fileName.find(baseName.c_str());

      // if it starts with the base name
      if (pos == 0) {
        // check to see if it is all numbers on the end
        pos = baseName.size() + 1;
        numbersOK = 1;

        while (pos < fileName.size()) {
          if (fileName[pos] < '0' || fileName[pos] > '9') {
            numbersOK = 0;
            break;
          }
          pos = pos + 1;
        }
        if (numbersOK) {
          fileName = subdirectory + fileName;
          files->push_back(fileName);
        }
      }
    }
    closedir(directory);
  }
}

////////////////////////////////////////////////////////////////////////////
//
// Read one input file and redistribute its particles according to location
// to the proper output file
//
////////////////////////////////////////////////////////////////////////////

void NToN(
	const string& inFile, 
	int varyFastest,
	int numberOfDimensions, 
	int* layoutSize,
	int* slot,
	float* step,
	ofstream* outStream,
	ID_T* numberOfInParticles,
	ID_T* numberOfOutParticles)
{
  // Size of .cosmo format
  int nfloat = 7, nint = 1;

  // Open the file and make sure everything is ok.
  ifstream *inStream = new ifstream(inFile.c_str(), ios::in);
  if (inStream->fail()) {
    cout << "File: " << inFile << " cannot be opened" << endl;
    exit (-1);
  }

  // compute the number of particles
  int recordSize = nfloat * sizeof(POSVEL_T) + nint* sizeof(ID_T);
  inStream->seekg(0L, ios::end);
  *numberOfInParticles = inStream->tellg() / recordSize;

  // rewind file to beginning for particle reads
  inStream->seekg(0L, ios::beg);

  // declare temporary read buffers
  POSVEL_T* fBlock = new POSVEL_T[nfloat];
  ID_T* iBlock = new ID_T[nint];

  // Collect range information per input file
  POSVEL_T minLoc[DIMENSION], maxLoc[DIMENSION];
  POSVEL_T minVel[DIMENSION], maxVel[DIMENSION];
  POSVEL_T minMass = MAX_FLOAT;
  POSVEL_T maxMass = MIN_FLOAT;
  ID_T minTag = *numberOfInParticles;
  ID_T maxTag = -1;
  for (int dim = 0; dim < DIMENSION; dim++) {
    minLoc[dim] = MAX_FLOAT;
    maxLoc[dim] = MIN_FLOAT;
    minVel[dim] = MAX_FLOAT;
    maxVel[dim] = MIN_FLOAT;
  }

  POSVEL_T location[DIMENSION];
  POSVEL_T velocity[DIMENSION];
  POSVEL_T mass;
  ID_T tag;

  // Read particles and write to individual files
  int index;
  for (int i = 0; i < *numberOfInParticles; i++) {

    inStream->read(reinterpret_cast<char*>(fBlock), nfloat * sizeof(POSVEL_T));
    long sizeFloatBlock = nfloat * sizeof(POSVEL_T);
    if (inStream->gcount() != sizeFloatBlock) {
      cout << "Premature end-of-file" << endl;
      exit (-1);
    }

    inStream->read(reinterpret_cast<char*>(iBlock), nint * sizeof(ID_T));
    long sizeIntBlock = nint * sizeof(ID_T);
    if (inStream->gcount() != sizeIntBlock) {
      cout << "Premature end-of-file" << endl;
      exit (-1);
    }

    location[0] = fBlock[0];
    location[1] = fBlock[2];
    location[2] = fBlock[4];
    velocity[0] = fBlock[1];
    velocity[1] = fBlock[3];
    velocity[2] = fBlock[5];
    mass = fBlock[6];
    tag = iBlock[0];

    // Collect range information per input file
    if (minTag > tag) minTag = tag;
    if (maxTag < tag) maxTag = tag;

    if (minMass > mass) minMass = mass;
    if (maxMass < mass) maxMass = mass;

    for (int dim = 0; dim < DIMENSION; dim++) {
      if (minLoc[dim] > location[dim])
        minLoc[dim] = location[dim];
      if (maxLoc[dim] < location[dim])
        maxLoc[dim] = location[dim];
      if (minVel[dim] > velocity[dim])
        minVel[dim] = velocity[dim];
      if (maxVel[dim] < velocity[dim])
        maxVel[dim] = velocity[dim];
    }

    for (int dim = 0; dim < numberOfDimensions; dim++) {
      slot[dim] = 0;
      while (location[dim] >= ((slot[dim] + 1) * step[dim]))
        slot[dim]++;
      if (slot[dim] >= layoutSize[dim])
         slot[dim] = layoutSize[dim] - 1;
    }

    // Decide which file to write to
    if (varyFastest == 0)	// Fortran ordering
      index = (slot[2] * layoutSize[1] * layoutSize[0]) +
              (slot[1] * layoutSize[0]) +
               slot[0];
    else			// C+ ordering
      index = (slot[0] * layoutSize[1] * layoutSize[2]) +
              (slot[1] * layoutSize[2]) +
               slot[2];

    outStream[index].write(reinterpret_cast<const char*>(fBlock), 
                         nfloat * sizeof(POSVEL_T));
    outStream[index].write(reinterpret_cast<const char*>(iBlock),
                         nint * sizeof(ID_T));
    numberOfOutParticles[index]++;
  }
  inStream->close();

  // Print the range information for the file
  cout << endl;
  cout << "In File: " << inFile << endl;
  cout << "   Number of particles: " << *numberOfInParticles << endl;
  cout << "   Location: ";
  for (int dim = 0; dim < DIMENSION; dim++)
    cout << " [" << minLoc[dim] << ":" << maxLoc[dim] << "] ";
  cout << endl;
  cout << "   Velocity: ";
  for (int dim = 0; dim < DIMENSION; dim++)
    cout << " [" << minVel[dim] << ":" << maxVel[dim] << "] ";
  cout << endl;
  cout << "   Mass:      [" << minMass << ":" << maxMass << "]" << endl;
  cout << "   Tag:       [" << minTag << ":" << maxTag << "]" << endl << endl;

  delete inStream;
  delete [] fBlock;
  delete [] iBlock;

  return;
}

//////////////////////////////////////////////////////////////////////////
//
// Take a series of .cosmo files and redistribute them into the specified layout
//
// Arg 1:	Name of input base files
// Arg 2:	Name of output base files
// Arg 3:	Physical box size of the particle data
// Arg 4:	In decomposing which dimension varies fastest
// Arg 5:	Number of dimensions for decomposition
// Arg 6,7,8	Size of decomposition in each dimension
//
// Output will be the output name with the processor id appended
// NToN FullBox.cosmo. Redistribute.cosmo. 500000 2 3 4 4 4
//
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  if (argc != 9) {
    cout << "Usage: NToN inBase outBase box_size ";
    cout << "dim_vary_fastest numDim sizeX sizeY sizeZ" << endl;
    exit(-1);
  }

  int i = 1;

  // File names
  string inFile = argv[i++];
  string outFile = argv[i++];

  // Box size
  float boxSize = atof(argv[i++]);

  // Decomposition ids set with which dimension varying fastest
  int varyFastest = atoi(argv[i++]);
  if (varyFastest == 0) {
    cout << endl << "**** NOTE ****" << endl << endl;
    cout << "  NToN inBase outBase_with_dot box_size ";
    cout << "vary_fastest numDim sizeX sizeY sizeZ" << endl << endl;
    cout << "  MPI decomposition for halo analysis is C ordering" << endl;
    cout << "  with the Z dimension varying fastest." << endl;;
    cout << "  To use the RRU software set vary_fastest = 2." << endl;
    cout << endl << "**** NOTE ****" << endl << endl;
  }

  // Number of dimensions to decompose data into, and size in each dimension
  int numberOfDimensions = atoi(argv[i++]);
  int* layout = new int[numberOfDimensions];
  for (int dim = 0; dim < numberOfDimensions; dim++)
    layout[dim] = atoi(argv[i++]);

  cout << "Input file: " << inFile << endl;
  cout << "Output file: " << outFile << endl;
  cout << "Box size: " << boxSize << endl;
  cout << "Vary fastest in: " << varyFastest << endl;
  cout << "Dimensions: " << numberOfDimensions << endl;
  cout << "Layout: [" 
       << layout[0] << "," << layout[1] << "," << layout[2] << "]" << endl;

  // Find all the input files
  vector<string>* inFileName = new vector<string>;
  findInputFiles(inFile, inFileName);
  int numberOfInFiles = inFileName->size();
  cout << "Number of input files: " << numberOfInFiles << endl;
  sort(inFileName->begin(), inFileName->end());

  // Calculate number of output files and open matching decomposition
  float* step = new float[numberOfDimensions];
  int* slot = new int[numberOfDimensions];
  int numberOfOutFiles = 1;

  for (int dim = 0; dim < numberOfDimensions; dim++) {
    step[dim] = boxSize / layout[dim];
    numberOfOutFiles *= layout[dim];
  }

  // Create each of the N output files
  vector<string>* outFileName = new vector<string>;
  ofstream* outStream = new ofstream[numberOfOutFiles];
  ID_T* numberOfOutParticles = new ID_T[numberOfOutFiles];

  for (int file = 0; file < numberOfOutFiles; file++) {
    ostringstream fileName;
    fileName << outFile << file;
    outFileName->push_back(fileName.str());
    outStream[file].open(fileName.str().c_str(), ios::out|ios::binary);
    numberOfOutParticles[file] = 0;
  }

  // Read each each input file, appending particles to correct output file
  ID_T numberOfInParticles;
  ID_T totalInParticles = 0;

  for (int file = 0; file < numberOfInFiles; file++) {
    NToN((*inFileName)[file], varyFastest, numberOfDimensions, layout,
         slot, step, outStream, 
         &numberOfInParticles, numberOfOutParticles);
    totalInParticles += numberOfInParticles;
  }

  // Particles written to each file
  ID_T totalOutParticles = 0;
  for (int file = 0; file < numberOfOutFiles; file++) {
    outStream[file].close();
    totalOutParticles += numberOfOutParticles[file];
    cout << "Out File: " << (*outFileName)[file]
         << "   NumberOfParticles: " << numberOfOutParticles[file] << endl;
  }

  cout << "Total input particles: " << totalInParticles << endl;
  cout << "Total output particles: " << totalOutParticles << endl;
  
  delete inFileName;
  delete outFileName;
  delete [] outStream;
  delete [] numberOfOutParticles;
  delete [] step;
  delete [] slot;

  return 0;
}
