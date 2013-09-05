//////////////////////////////////////////////////////////////////////////////
//
// Read a .cosmo binary file file and partition it into N rectilinear parts
// Format of the binary file is:
//	X location	(64 bit float)
//	X velocity	(64 bit float)
//	Y location	(64 bit float)
//	Y velocity	(64 bit float)
//	Z location	(64 bit float)
//	X velocity	(64 bit float)
//	Particle mass	(64 bit float = 1.0)
//	ID value	(32 bit int)
//
//////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <math.h>

using namespace std;

void OneToN(
  const string& inFile,
  float boxSize,
  int varyFastest,
  int numberOfDimensions,
  int* layoutSize)
{
  // Open the file and make sure everything is ok.
  ifstream *inStream = new ifstream(inFile.c_str(), ios::in);
  if (inStream->fail()) {
    delete inStream;
    cout << "File: " << inFile << " cannot be opened" << endl;
    exit (-1);
  }

  // compute the number of particles
  inStream->seekg(0L, ios::end);
  int numberOfParticles = inStream->tellg() / 32;
  cout << "NumberOfParticles:    " << numberOfParticles << endl;

  // rewind file to beginning for particle reads
  inStream->seekg(0L, ios::beg);

  // declare temporary read buffers
  int nfloat = 7, nint = 1;
  float* fBlock = new float[nfloat];
  int* iBlock = new int[nint];

  float* step = new float[numberOfDimensions];
  int* slot = new int[numberOfDimensions];
  int numberOfFiles = 1;

  for (int dim = 0; dim < numberOfDimensions; dim++) {
    step[dim] = boxSize / layoutSize[dim];
    numberOfFiles *= layoutSize[dim];
  }

  // Create each of the N output files
  ofstream* outStream = new ofstream[numberOfFiles];
  int* numberOfOutParticles = new int[numberOfFiles];

  for (int file = 0; file < numberOfFiles; file++) {
    ostringstream fileName;
    fileName << inFile << "." << file;
    cout << "Output: " << fileName.str() << endl;
    outStream[file].open(fileName.str().c_str(), ios::out|ios::binary);
    numberOfOutParticles[file] = 0;
  }

  // Read particles and write to individual files
  int index;
  for (int i = 0; i < numberOfParticles; i++) {

    inStream->read(reinterpret_cast<char*>(fBlock), nfloat * sizeof(float));
    long sizeFloatBlock = nfloat * sizeof(float);
    if (inStream->gcount() != sizeFloatBlock) {
      cout << "Premature end-of-file" << endl;
      exit (-1);
    }

    inStream->read(reinterpret_cast<char*>(iBlock), nint * sizeof(int));
    long sizeIntBlock = nint * sizeof(int);
    if (inStream->gcount() != sizeIntBlock) {
      cout << "Premature end-of-file" << endl;
      exit (-1);
    }

    for (int dim = 0; dim < numberOfDimensions; dim++) {
      int bdim = dim * 2;
      slot[dim] = 0;
      while (fBlock[bdim] >= ((slot[dim] + 1) * step[dim]))
        slot[dim]++;
      if (slot[dim] >= layoutSize[dim])
         slot[dim] = layoutSize[dim] - 1;
    }

    // Decide which file to write to
    if (varyFastest == 0)	// Fortran
      index = (slot[2] * layoutSize[1] * layoutSize[0]) +
              (slot[1] * layoutSize[0]) +
               slot[0];
    else			// C++
      index = (slot[0] * layoutSize[1] * layoutSize[2]) +
              (slot[1] * layoutSize[2]) +
               slot[2];

    outStream[index].write(reinterpret_cast<const char*>(fBlock),
                         nfloat * sizeof(float));
    outStream[index].write(reinterpret_cast<const char*>(iBlock),
                         nint * sizeof(int));
    numberOfOutParticles[index]++;
  }
  inStream->close();

  int totalOutParticles = 0;
  for (int file = 0; file < numberOfFiles; file++) {
    outStream[file].close();
    totalOutParticles += numberOfOutParticles[file];
    cout << "NumberOfParticles " << file << ": "
         << numberOfOutParticles[file] << endl;
  }
  cout << "Number of out particles: " << totalOutParticles << endl;

  delete inStream;
  delete [] outStream;
  delete [] numberOfOutParticles;
  delete [] step;
  delete [] slot;
  delete [] fBlock;
  delete [] iBlock;

  return;
}

//////////////////////////////////////////////////////////////////////////
//
// Take a .cosmo file and divide it into the specified layout
//
// Arg 1:	Name of input file
// Arg 2:	Physical box size of the particle data
// Arg 3:	In decomposing which dimension varies fastest
// Arg 4:	Number of dimensions for decomposition
// Arg 5+	Size of decomposition in each dimension
//
// Output will be the input name with the processor id appended
// OneToN FullBox.cosmo 500000 0 3 2 2 2
//
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  if (argc != 8) {
    cout << "Usage: OneToN inFile box_size ";
    cout << "dim_vary_fastest numDim sizeDim" << endl;
    exit(-1);
  }

  int i = 1;

  // Input file
  string inFile = argv[i++];

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
  cout << "Box size: " << boxSize << endl;
  cout << "Vary fastest in: " << varyFastest << endl;
  cout << "Dimensions: " << numberOfDimensions << endl;
  cout << "Layout: ["
       << layout[0] << "," << layout[1] << "," << layout[2] << "]" << endl;

  OneToN(inFile, boxSize, varyFastest, numberOfDimensions, layout);

  return 0;
}
