/*=========================================================================
                                                                                
Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
                                                                                
=========================================================================*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <sys/types.h>

#ifdef _WIN32
#include "winDirent.h"
#else
#include <dirent.h>
#endif

#include "Partition.h"
#include "ParticleDistribute.h"

#ifdef USE_VTK_COSMO
#include "vtkStdString.h"
#include "vtkSetGet.h"
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// Particle data space is partitioned for the number of processors
// which currently is a factor of two but is easily extended.  Particles
// are read in from files where each processor reads one file into a buffer,
// extracts the particles which really belong on the processor (ALIVE) and
// those in a buffer region around the edge (DEAD).  The buffer is then
// passed round robin to every other processor so that all particles are
// examined by all processors.  All dead particles are tagged with the
// neighbor zone (26 neighbors in 3D) so that later halos can be associated
// with zones.
//
/////////////////////////////////////////////////////////////////////////

ParticleDistribute::ParticleDistribute()
{
  // Get the number of processors running this problem and rank
  this->numProc = Partition::getNumProc();
  this->myProc = Partition::getMyProc();

  // Get the number of processors in each dimension
  Partition::getDecompSize(this->layoutSize);

  // Get my position within the Cartesian topology
  Partition::getMyPosition(this->layoutPos);

  // Get neighbors of this processor including the wraparound
  Partition::getNeighbors(this->neighbor);

  this->numberOfAliveParticles = 0;
}

ParticleDistribute::~ParticleDistribute()
{
}

/////////////////////////////////////////////////////////////////////////
//
// Set parameters for particle distribution
//
/////////////////////////////////////////////////////////////////////////

void ParticleDistribute::setParameters(
                        const string& baseName,
                        POSVEL_T rL,
                        string dataType)
{
  // Base file name which will have processor id appended for actual files
  this->baseFile = baseName;

  // Physical total space and amount of physical space to use for dead particles
  this->boxSize = rL;

  // RECORD format is the binary .cosmo of one particle with all information
  if (dataType == "RECORD")
    this->inputType = RECORD;

  // BLOCK format is Gadget format with a header and x,y,z locations for
  // all particles, then x,y,z velocities for all particles, and all tags
  else if (dataType == "BLOCK")
    this->inputType = BLOCK;

#ifndef USE_VTK_COSMO
  if (this->myProc == MASTER) {
    cout << endl << "------------------------------------" << endl;
    cout << "boxSize:  " << this->boxSize << endl;
  }
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Set box sizes for determining if a particle is in the alive or dead
// region of this processor.  Data space is a DIMENSION torus.
//
/////////////////////////////////////////////////////////////////////////

void ParticleDistribute::initialize()
{
#ifndef USE_VTK_COSMO
#ifdef DEBUG
  if (this->myProc == MASTER)
    cout << "Decomposition: [" << this->layoutSize[0] << ":"
         << this->layoutSize[1] << ":" << this->layoutSize[2] << "]" << endl;
#endif
#endif

  // Set subextents on particle locations for this processor
  POSVEL_T boxStep[DIMENSION];
  for (int dim = 0; dim < DIMENSION; dim++) {
    boxStep[dim] = this->boxSize / this->layoutSize[dim];

    // Alive particles
    this->minAlive[dim] = this->layoutPos[dim] * boxStep[dim];
    this->maxAlive[dim] = this->minAlive[dim] + boxStep[dim];
    if (this->maxAlive[dim] > this->boxSize)
      this->maxAlive[dim] = this->boxSize;
  }
}


void ParticleDistribute::setParticles(vector<POSVEL_T>* xLoc,
                                      vector<POSVEL_T>* yLoc,
                                      vector<POSVEL_T>* zLoc,
                                      vector<POSVEL_T>* xVel,
                                      vector<POSVEL_T>* yVel,
                                      vector<POSVEL_T>* zVel,
                                      vector<POSVEL_T>* mass,
                                      vector<ID_T>* id)
{
  this->xx = xLoc;
  this->yy = yLoc;
  this->zz = zLoc;
  this->vx = xVel;
  this->vy = yVel;
  this->vz = zVel;
  this->ms = mass;
  this->tag = id;
}



/////////////////////////////////////////////////////////////////////////
//
// Each processor reads 0 or more files, a buffer at a time, and shares
// the particles by passing the buffer round robin to every other processor
//
/////////////////////////////////////////////////////////////////////////

void ParticleDistribute::readParticlesRoundRobin(int reserveQ)
{
  // Find how many input files there are and deal them between the processors
  // Calculates the max number of files per processor and max number of
  // particles per file so that buffering can be done
  // For round robin sharing determine where to send and receive buffers from
  partitionInputFiles();

  // Compute the total number of particles in the problem
  // Compute the maximum number of particles in any one file to set buffer size
  findFileParticleCount();

  // MPI buffer size might limit the number of particles read from a file
  // and passed round robin
  // Largest file will have a number of buffer chunks to send if it is too large
  // Every processor must send that number of chunks even if its own file
  // does not have that much information

  if (ENFORCE_MAX_READ == true && this->maxParticles > MAX_READ) {
    this->maxRead = MAX_READ;
    this->maxReadsPerFile = (this->maxParticles / this->maxRead) + 1;
  } else {
    this->maxRead = this->maxParticles;
    this->maxReadsPerFile = 1;
  }

  // Allocate space to hold buffer information for reading of files
  // Mass is constant use that float to store the tag
  // Number of particles is the first integer in the buffer
  int bufferSize = (1 * sizeof(int)) +          // number of particles
                   (this->maxRead *
                     ((COSMO_FLOAT * sizeof(POSVEL_T)) +
                      (COSMO_INT * sizeof(ID_T))));

  Message* message1 = new Message(bufferSize);
  Message* message2 = new Message(bufferSize);

  // Allocate space for the data read from the file
  POSVEL_T *fBlock = 0;
  POSVEL_T *lBlock = 0;
  POSVEL_T *vBlock = 0;
  ID_T* iBlock = 0;

  // RECORD format reads one particle at a time
  if (this->inputType == RECORD) {
    fBlock = new POSVEL_T[COSMO_FLOAT];
    iBlock = new ID_T[COSMO_INT];
  }

  // BLOCK format reads all particles at one time for triples
  else if (this->inputType == BLOCK) {
    lBlock = new POSVEL_T[this->maxRead * 3];
    vBlock = new POSVEL_T[this->maxRead * 3];
    iBlock = new ID_T[this->maxRead];
  }

  // Reserve particle storage to minimize reallocation
  int reserveSize = (int) (this->maxFiles * this->maxParticles * DEAD_FACTOR);

  // If multiple processors are reading the same file we can reduce size
  reserveSize /= this->processorsPerFile;

  if(reserveQ) {
#ifndef USE_VTK_COSMO
    cout << "readParticlesRoundRobin reserving vectors" << endl;
#endif
    this->xx->reserve(reserveSize);
    this->yy->reserve(reserveSize);
    this->zz->reserve(reserveSize);
    this->vx->reserve(reserveSize);
    this->vy->reserve(reserveSize);
    this->vz->reserve(reserveSize);
    this->ms->reserve(reserveSize);
    this->tag->reserve(reserveSize);
  }

  // Running total and index into particle data on this processor
  this->particleCount = 0;

  // Using the input files assigned to this processor, read the input
  // and push round robin to every other processor
  // this->maxFiles is the maximum number to read on any processor
  // Some processors may have no files to read but must still participate 
  // in the round robin distribution

  for (int file = 0; file < this->maxFiles; file++) {

    // Open file to read the data if any for this processor
    ifstream* inStream = 0;
    int firstParticle = 0;
    int numberOfParticles = 0;
    int remainingParticles = 0;

    if ((int)this->inFiles.size() > file) {
      inStream = new ifstream(this->inFiles[file].c_str(), ios::in);

#ifndef USE_VTK_COSMO
      cout << "Rank " << this->myProc << " open file " << inFiles[file]
           << " with " << this->fileParticles[file] << " particles" << endl;
#endif

      // Number of particles read at one time depends on MPI buffer size
      numberOfParticles = this->fileParticles[file];
      if (numberOfParticles > this->maxRead)
        numberOfParticles = this->maxRead;

      // If a file is too large to be passed as an MPI message divide it up
      remainingParticles = this->fileParticles[file];

    } else {
#ifndef USE_VTK_COSMO
      cout << "Rank " << this->myProc << " no file to open " << endl;
#endif
    }

    for (int piece = 0; piece < this->maxReadsPerFile; piece++) {

      // Reset each MPI message for each file read
      message1->reset();
      message2->reset();

      // Processor has a file to read and share via round robin with others
      if (file < (int)this->inFiles.size()) {
        if (this->inputType == RECORD) {
          readFromRecordFile(inStream, firstParticle, numberOfParticles,
                             fBlock, iBlock, message1);
        } else {
          readFromBlockFile(inStream, firstParticle, numberOfParticles,
                           this->fileParticles[file],
                           lBlock, vBlock, iBlock, message1);
        }
        firstParticle += numberOfParticles;
        remainingParticles -= numberOfParticles;
        if (remainingParticles <= 0)
          numberOfParticles = 0;
        else if (remainingParticles < numberOfParticles)
          numberOfParticles = remainingParticles;
      }

      // Processor does not have a file to open but must participate in the
      // round robin with an empty buffer
      else {
        // Store number of particles used in first position
        int zero = 0;
        message1->putValue(&zero);
      }

      // Particles belonging to this processor are put in vectors
      distributeParticles(message1, message2);
    }

    // Can delete the read buffers as soon as last file is read because
    // information has been transferred into the double buffer1
    if (file == (this->maxFiles - 1)) {
      if (this->inputType == RECORD) {
        delete [] fBlock;
        delete [] iBlock;
      } else if (this->inputType == BLOCK) {
        delete [] lBlock;
        delete [] vBlock;
        delete [] iBlock;
      }
    }

    if ((int)this->inFiles.size() > file)
      inStream->close();
  }

  // After all particles have been distributed to vectors the double
  // buffers can be deleted
  delete message1;
  delete message2;

  // Count the particles across processors
  long totalAliveParticles = 0;
#ifdef USE_SERIAL_COSMO
  totalAliveParticles = this->numberOfAliveParticles;
#else
  MPI_Allreduce((void*) &this->numberOfAliveParticles, 
                (void*) &totalAliveParticles, 
                1, MPI_LONG, MPI_SUM, Partition::getComm());
#endif

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  cout << "Rank " << setw(3) << this->myProc 
       << " #alive = " << this->numberOfAliveParticles << endl;
#endif
 
  if (this->myProc == MASTER) {
    cout << "TotalAliveParticles " << totalAliveParticles << endl;
  }
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Using the base name of the data, go to the subdirectory and determine
// how many input files there are.  Parcel those files between all the
// processors which will be responsible for actually reading 0 or more.
//
/////////////////////////////////////////////////////////////////////////

void ParticleDistribute::partitionInputFiles()
{
  // Find number of input files for this problem given the base input name
  // Get the subdirectory name containing the input files
  string::size_type dirPos = this->baseFile.rfind("/");
  string subdirectory;
  string baseName;

  // If the directory is not given use the current directory
  if (dirPos == string::npos) {
    subdirectory = "./";
    baseName = this->baseFile;
  } else {
    subdirectory = this->baseFile.substr(0, dirPos + 1);
    baseName = this->baseFile.substr(dirPos + 1);
  }

  if(this->baseFile.size() == 0)
    {
    return;
    }

  // strip everything back to the first non-number
  string::size_type pos = baseName.size() - 1;
  int numbersOK = 1;

  while(numbersOK)
    {
    if(baseName[pos] >= '0' && baseName[pos] <= '9')
      {
      if(pos > 0) 
        {
        pos = pos - 1;
        }
      else
        {
        break;
        }
      }
    else
      {
      numbersOK = 0;
      }
    }
  
  // base name is everything up to the numbers
  baseName = baseName.substr(0, pos + 1);

  // Open the subdirectory and make a list of input files
  DIR* directory = opendir(subdirectory.c_str());
  struct dirent* directoryEntry;
  vector<string> files;

  if (directory != NULL) {
  while ((directoryEntry = readdir(directory))) 
    {
    // get the name
    string fileName = directoryEntry->d_name;
    pos = fileName.find(baseName.c_str());
    
    // if it starts with the base name
    if(pos == 0)
      {
      // check to see if it is all numbers on the end
      pos = baseName.size() + 1;
      numbersOK = 1;

      while(pos < fileName.size())
        {
        if(fileName[pos] < '0' || fileName[pos] > '9')
          {
          numbersOK = 0;
          break;
          }

        pos = pos + 1;
        }
      
      if(numbersOK)
        {
        fileName = subdirectory + fileName;
        files.push_back(fileName);
        }
      }
    }

  closedir(directory);
  }

  this->numberOfFiles = (int)files.size();

  if (this->numberOfFiles == 0) {
#ifdef USE_VTK_COSMO
    vtkStdString temp = "Processor ";
    temp += this->myProc; 
    temp += " found no input files.\n";
    vtkOutputWindowDisplayErrorText(temp.c_str());

    return;
#else
    cout << "Rank " << this->myProc << " found no input files" << endl;
    exit(1);
#endif 
  }

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  if (this->myProc == MASTER) {
    for (int i = 0; i < this->numberOfFiles; i++)
      cout << "   File " << i << ": " << files[i] << endl;
  }
#endif
#endif

  // Divide the files between all the processors
  // If there are 1 or more files per processor set the
  // buffering up with a full round robin between all processors
  if (this->numberOfFiles >= this->numProc) {

    // Number of round robin sends to share all the files
    this->processorsPerFile = 1;
    this->numberOfFileSends = this->numProc - 1;
    this->maxFileSends = this->numberOfFileSends;

    // Which files does this processor read
    for (int i = 0; i < this->numberOfFiles; i++)
      if ((i % this->numProc) == this->myProc)
        this->inFiles.push_back(files[i]);

    // Where is the file sent, and where is it received
    if (this->myProc == this->numProc - 1)
      this->nextProc = 0;
    else
      this->nextProc = this->myProc + 1;
    if (this->myProc == 0)
      this->prevProc = this->numProc - 1;
    else
      this->prevProc = this->myProc - 1;
  }

  // If there are more processors than file set up as many round robin loops
  // as possible so that multiple processors read the same file. If the number
  // of files does not divide evenly into the number of processors the last
  // round robin loop will be bigger and some processors will contribute
  // buffers of 0 size to send

  else {

    // Assign the round robin circle (last circle is bigger than others)
    this->processorsPerFile = this->numProc / this->numberOfFiles;
    int numberOfRoundRobinCircles = this->processorsPerFile;
    int myCircle = this->myProc / this->numberOfFiles;
    int extraProcessors = this->numProc - 
            (numberOfRoundRobinCircles * this->numberOfFiles);
    if (myCircle == numberOfRoundRobinCircles)
      myCircle--;

    int firstInCircle = myCircle * this->numberOfFiles;
    int lastInCircle = firstInCircle + this->numberOfFiles - 1;
    if (myCircle == (numberOfRoundRobinCircles - 1))
      lastInCircle += extraProcessors;

    // How big is the round robin circle this processor is in
    // What is the biggest round robin circle (needed because of MPI_Barrier)
    this->numberOfFileSends = lastInCircle - firstInCircle;
    this->maxFileSends = this->numberOfFiles + extraProcessors;

    // Which file does this processor read
    int index = this->myProc % this->numberOfFiles;
    if (myCircle == (this->myProc / this->numberOfFiles))
      this->inFiles.push_back(files[index]);

    // Where is the file sent, and where is it received
    if (this->myProc == lastInCircle)
      this->nextProc = firstInCircle;
    else
      this->nextProc = this->myProc + 1;
    if (this->myProc == firstInCircle)
      this->prevProc = lastInCircle;
    else 
      this->prevProc = this->myProc - 1;
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Open each input file belonging to this processor and find the number
// of particles for setting buffer sizes
//
/////////////////////////////////////////////////////////////////////////

void ParticleDistribute::findFileParticleCount()
{
  // Compute the total number of particles in the problem
  // Compute the maximum number of particles in any one file to set buffer size
  long numberOfParticles = 0;
  long maxNumberOfParticles = 0;
  int numberOfMyFiles = (int)this->inFiles.size();

  // Each processor counts the particles in its own files
  for (int i = 0; i < numberOfMyFiles; i++) {

    // Open my file
    ifstream *inStream = new ifstream(this->inFiles[i].c_str(), ios::in);
    if (inStream->fail()) {
      delete inStream;
#ifdef USE_VTK_COSMO
      vtkStdString message = "File ";
      message += this->inFiles[i];
      message += " cannot be opened.\n";
      vtkOutputWindowDisplayErrorText(message.c_str());

      this->totalParticles = 0;
      this->maxParticles = 0;
      return;
#else
      cout << "File: " << this->inFiles[i] << " cannot be opened" << endl;
      exit (-1);
#endif
    }

    if (this->inputType == RECORD) {

      // Compute the number of particles from file size
      inStream->seekg(0L, ios::end);
      int numberOfRecords = inStream->tellg() / RECORD_SIZE;
      this->fileParticles.push_back(numberOfRecords);

      numberOfParticles += numberOfRecords;
      if (maxNumberOfParticles < numberOfRecords)
        maxNumberOfParticles = numberOfRecords;
    }

    else if (this->inputType == BLOCK) {

      // Find the number of particles in the header
      inStream->read(reinterpret_cast<char*>(&this->cosmoHeader),
                     sizeof(this->cosmoHeader));

      this->headerSize = this->cosmoHeader.npart[0];
      if (sizeof(this->cosmoHeader) != this->headerSize)
#ifdef USE_VTK_COSMO
        vtkOutputWindowDisplayErrorText("Mismatch of header size and header structure.\n");
#else
        cout << "Mismatch of header size and header structure" << endl;
#endif

      int numberOfRecords = this->cosmoHeader.npart[2];
      this->fileParticles.push_back(numberOfRecords);

      numberOfParticles += numberOfRecords;
      if (maxNumberOfParticles < numberOfRecords)
        maxNumberOfParticles = numberOfRecords;
    }

    inStream->close();
    delete inStream;
  }

  // If multiple processors read the same file, just do the reduce on one set
  if (this->processorsPerFile > 1) {
    if (this->myProc >= this->numberOfFiles) {
      numberOfParticles = 0;
      maxNumberOfParticles = 0;
    }
  }

  // Share the information about total particles
#ifdef USE_SERIAL_COSMO
  this->totalParticles = numberOfParticles;
#else
  MPI_Allreduce((void*) &numberOfParticles,
                (void*) &this->totalParticles,
                1, MPI_LONG, MPI_SUM, Partition::getComm());
#endif

  // Share the information about max particles in a file for setting buffer size
#ifdef USE_SERIAL_COSMO
  this->maxParticles = maxNumberOfParticles;
#else
  MPI_Allreduce((void*) &maxNumberOfParticles,
                (void*) &this->maxParticles,
                1, MPI_LONG, MPI_MAX, Partition::getComm());
#endif

  // Share the maximum number of files on a processor for setting the loop
#ifdef USE_SERIAL_COSMO
  this->maxFiles = numberOfMyFiles;
#else
  MPI_Allreduce((void*) &numberOfMyFiles,
                (void*) &this->maxFiles,
                1, MPI_INT, MPI_MAX, Partition::getComm());
#endif

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  if (this->myProc == MASTER) {
    cout << "Total particle count: " << this->totalParticles << endl;
    cout << "Max particle count:   " << this->maxParticles << endl;
  }
#endif
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// Each processor reads 0 or more files, a buffer at a time.
// The particles are processed by seeing if they are in the subextent of
// this processor and are tagged either ALIVE or if dead, by the index of
// the neighbor zone which contains that particle.  That buffer is sent
// round robin to (myProc + 1) % numProc where it is processed and sent on.
// After each processor reads one buffer and sends and receives numProc - 1
// times the next buffer from the file is read.  Must use a double buffering
// scheme so that on each send/recv we switch buffers.
//
// Input files may be BLOCK or RECORD structured
//
/////////////////////////////////////////////////////////////////////////

void ParticleDistribute::distributeParticles(
                Message* message1,      // Send/receive buffers
                Message* message2)      // Send/receive buffers
{
  // Each processor has filled a buffer with particles read from a file
  // or had no particles to read but set the count in the buffer to 0
  // Process the buffer to keep only those within range
  Message* sendMessage = message1;
  Message* recvMessage = message2;

  // Process the original send buffer of particles from the file
  collectLocalParticles(sendMessage);

  // Distribute buffer round robin so that all processors see it
  for (int step = 0; step < this->maxFileSends; step++) {

    if (step < this->numberOfFileSends)
      {
      // Send buffer to the next processor if round robin loop is still active
      sendMessage->send(this->nextProc);

      // Receive buffer from the previous processor
      recvMessage->receive(this->prevProc);
      }

#ifndef USE_SERIAL_COSMO
    MPI_Barrier(Partition::getComm());
#endif

    // Process the send buffer for alive and dead before sending on
    if (step < this->numberOfFileSends)
      collectLocalParticles(recvMessage);

    // Swap buffers for next round robin send
    Message* tmp = sendMessage;
    sendMessage = recvMessage;
    recvMessage = tmp;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Input file is RECORD structured so read each particle record and populate
// the double buffer in particle order for the rest of the processing
//
/////////////////////////////////////////////////////////////////////////////

void ParticleDistribute::readFromRecordFile(
                        ifstream* inStream,     // Stream to read from
                        int firstParticle,      // First particle index
                        int numberOfParticles,  // Number to read this time
                        POSVEL_T* fBlock,       // Buffer for read in data
                        ID_T* iBlock,           // Buffer for read in data
                        Message* message)       // Reordered data
{
  // Store number of particles used in first position
  message->putValue(&numberOfParticles);
  if (numberOfParticles == 0)
    return;

  // Seek to the first particle locations and read
  int floatSkip = COSMO_FLOAT * sizeof(POSVEL_T);
  int intSkip = COSMO_INT * sizeof(ID_T);
  int skip = (floatSkip + intSkip) * firstParticle;
  inStream->seekg(skip, ios::beg);

  // Store each particle location, velocity, mass and tag (as float) in buffer
  int changeCount = 0;
  for (int p = 0; p < numberOfParticles; p++) {

    // Set file pointer to the requested particle
    inStream->read(reinterpret_cast<char*>(fBlock),
                   COSMO_FLOAT * sizeof(POSVEL_T));

    if (inStream->gcount() != COSMO_FLOAT * sizeof(POSVEL_T)) {
#ifdef USE_VTK_COSMO
      vtkOutputWindowDisplayErrorText("Premature end-of-file.\n");
      return;
#else
      cout << "Premature end-of-file" << endl;
      exit (-1);
#endif
    }

    inStream->read(reinterpret_cast<char*>(iBlock),
                   COSMO_INT * sizeof(ID_T));

    if (inStream->gcount() != COSMO_INT * sizeof(ID_T)) {
#ifdef USE_VTK_COSMO
      vtkOutputWindowDisplayErrorText("Premature end-of-file.\n");
      return;
#else
      cout << "Premature end-of-file" << endl;
      exit (-1);
#endif
    }

    // If the location is not within the bounding box wrap around
    for (int i = 0; i <= 4; i = i + 2) {
      if (fBlock[i] >= this->boxSize) {
#ifndef USE_VTK_COSMO
#ifdef DEBUG
        cout << "Location at " << i << " changed from " << fBlock[i] << endl;
#endif
#endif
        fBlock[i] -= this->boxSize;
        changeCount++;
      }
    }

    // Store location and velocity and mass in message buffer
    // Reorder so that location vector is followed by velocity vector
    message->putValue(&fBlock[0]);
    message->putValue(&fBlock[2]);
    message->putValue(&fBlock[4]);
    message->putValue(&fBlock[1]);
    message->putValue(&fBlock[3]);
    message->putValue(&fBlock[5]);
    message->putValue(&fBlock[6]);

    // Store the integer tag
    message->putValue(&iBlock[0]);
  }

#ifndef USE_VTK_COSMO
  cout << "Rank " << this->myProc << " wrapped around " << changeCount
       << " particles" << endl;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// Input file is BLOCK structured so read head and each block of data.
// Gadget-2 format:
//    SKIP_H 4 bytes (size of header)
//    Header
//    SKIP_H 4 bytes (size of header)
//
//    SKIP_L 4 bytes (size of location block in bytes)
//    Block of location data where each particle's x,y,z is stored together
//    SKIP_L 4 bytes (size of location block in bytes)
//
//    SKIP_V 4 bytes (size of velocity block in bytes)
//    Block of velocity data where each particle's xv,yv,zv is stored together
//    SKIP_V 4 bytes (size of velocity block in bytes)
//
//    SKIP_T 4 bytes (size of tag block in bytes)
//    Block of tag data
//    SKIP_T 4 bytes (size of tag block in bytes)
//
// Reorder the data after it is read into the same structure as the
// RECORD data so that the rest of the code does not have to be changed
//
/////////////////////////////////////////////////////////////////////////////

void ParticleDistribute::readFromBlockFile(
                        ifstream* inStream,     // Stream to read from
                        int firstParticle,      // First particle index
                        int numberOfParticles,  // Number to read this time
                        int totParticles,       // Total particles in file
                        POSVEL_T* lBlock,       // Buffer for read of location
                        POSVEL_T* vBlock,       // Buffer for read of velocity
                        ID_T* iBlock,           // Buffer for read in data
                        Message* message)       // Reordered data
{
  // Store number of particles used in first position
  message->putValue(&numberOfParticles);
  if (numberOfParticles == 0)
    return;

  // Seek to the first particle locations and read triples
  // Skip (SKIP_H, header, SKIP_H, SKIP_L) to get to locations
  //
  int skip = this->headerSize + (3 * sizeof(int)) + 
             (3 * sizeof(POSVEL_T) * firstParticle);
  inStream->seekg(skip, ios::beg);
  inStream->read(reinterpret_cast<char*>(lBlock),
                 3 * numberOfParticles * sizeof(POSVEL_T));

  // If the location is not within the bounding box wrap around
  int changeCount = 0;
  for (int i = 0; i < 3*numberOfParticles; i++) {
    if (lBlock[i] >= this->boxSize) {

#ifndef USE_VTK_COSMO
#ifdef DEBUG
      cout << "Location at " << i << " changed from " << lBlock[i] << endl;
#endif
#endif

      lBlock[i] -= this->boxSize;
      changeCount++;
    }
  }

#ifndef USE_VTK_COSMO
  cout << "Rank " << this->myProc << " wrapped around " << changeCount
       << " particles" << endl;
#endif

  // Seek to first particle velocities and read triples
  // Skip (SKIP_H, header, SKIP_H, SKIP_L) to get to locations
  // Skip (3 * totParticles) to get to end of locations
  // Skip (SKIP_L, SKIP_V) to get to start of velocities
  //
  skip = this->headerSize + (5 * sizeof(int)) +
         (3 * sizeof(POSVEL_T) * totParticles) + // skip all locations
         (3 * sizeof(POSVEL_T) * firstParticle); // skip to first velocity
  inStream->seekg(skip, ios::beg);
  inStream->read(reinterpret_cast<char*>(vBlock),
                 3 * numberOfParticles * sizeof(POSVEL_T));

  // Seek to first particle tags and read
  // Skip (SKIP_H, header, SKIP_H, SKIP_L) to get to locations
  // Skip (3 * totParticles) to get to end of locations
  // Skip (SKIP_L, SKIP_V) to get to start of velocities
  // Skip (3 * totParticles) to get to end of velocities
  // Skip (SKIP_V, SKIP_T) to get to start of tags
  //
  skip = this->headerSize + (7 * sizeof(int)) +
         (3 * sizeof(POSVEL_T) * totParticles) + // skip all locations
         (3 * sizeof(POSVEL_T) * totParticles) + // skip all velocities
         (1 * sizeof(ID_T) * firstParticle);     // skip to first tag
  inStream->seekg(skip, ios::beg);
  inStream->read(reinterpret_cast<char*>(iBlock),
                 1 * numberOfParticles * sizeof(ID_T));

  // Store the locations in the message buffer in record order
  // so that the same distribution method for RECORD will work
  int indx = 0;
  POSVEL_T mass = 1.0;
  for (int p = 0; p < numberOfParticles; p++) {

    // Locations
    message->putValue(&lBlock[indx]);           // X location
    message->putValue(&lBlock[indx+1]);         // Y location
    message->putValue(&lBlock[indx+2]);         // Z location

    // Velocities
    message->putValue(&vBlock[indx]);           // X velocity
    message->putValue(&vBlock[indx+1]);         // Y velocity
    message->putValue(&vBlock[indx+2]);         // Z velocity

    // No mass in gadget files so put a constant
    message->putValue(&mass);

    // Id tag
    message->putValue(&iBlock[p]);
    indx += 3;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Process the data buffer of particles to choose those which are ALIVE
// or DEAD on this processor.  Do wraparound tests to populate as for a
// 3D torus.  Dead particle status is the zone id of the neighbor processor
// which contains it as an ALIVE particle.
//
/////////////////////////////////////////////////////////////////////////////

void ParticleDistribute::collectLocalParticles(Message* message)
{
  // In order to read a buffer, reset position to the beginning
  message->reset();

  int numParticles;
  message->getValue(&numParticles);
  POSVEL_T loc[DIMENSION], vel[DIMENSION], mass;
  ID_T id;

  // Test each particle in the buffer to see if it is ALIVE or DEAD
  // If it is DEAD assign it to the neighbor zone that it is in
  // Check all combinations of wraparound

  for (int i = 0; i < numParticles; i++) {
    for (int dim = 0; dim < DIMENSION; dim++)
      message->getValue(&loc[dim]);
    for (int dim = 0; dim < DIMENSION; dim++)
      message->getValue(&vel[dim]);
    message->getValue(&mass);
    message->getValue(&id);

    // Is the particle ALIVE on this processor
    if ((loc[0] >= minAlive[0] && loc[0] < maxAlive[0]) &&
        (loc[1] >= minAlive[1] && loc[1] < maxAlive[1]) &&
        (loc[2] >= minAlive[2] && loc[2] < maxAlive[2])) {

          this->xx->push_back(loc[0]);
          this->yy->push_back(loc[1]);
          this->zz->push_back(loc[2]);
          this->vx->push_back(vel[0]);
          this->vy->push_back(vel[1]);
          this->vz->push_back(vel[2]);
          this->ms->push_back(mass);
          this->tag->push_back(id);

          this->numberOfAliveParticles++;
          this->particleCount++;
    }
  }
}

/////////////////////////////////////////////////////////////////////////
//
// Each processor reads 1 file or gets a pointer to data eventually
// As the particle is read it will be stored as an alive particle on this
// processor and will be checked about neighbor ranges to see if it must
// be exchanged
//
/////////////////////////////////////////////////////////////////////////
  
void ParticleDistribute::readParticlesOneToOne(int reserveQ)
{
  // File name is the base file name with processor id appended
  // Because MPI Cartesian topology is used the arrangement of files in
  // physical space must follow the rule of last dimension varies fastest
  ostringstream fileName;
  fileName << this->baseFile << "." << this->myProc;
  this->inFiles.push_back(fileName.str());

  // Compute the total number of particles in the problem
  // Compute the maximum number of particles in any one file to set buffer size
  findFileParticleCount();
  
  // Reserve particle storage to minimize reallocation
  int reserveSize = (int) (this->maxParticles * DEAD_FACTOR);
  
  if(reserveQ) {
#ifndef USE_VTK_COSMO
    cout << "readParticlesOneToOne reserving vectors" << endl;
#endif
    this->xx->reserve(reserveSize);
    this->yy->reserve(reserveSize);
    this->zz->reserve(reserveSize);
    this->vx->reserve(reserveSize);
    this->vy->reserve(reserveSize);
    this->vz->reserve(reserveSize);
    this->ms->reserve(reserveSize);
    this->tag->reserve(reserveSize);
  }

  // Running total and index into particle data on this processor
  this->particleCount = 0;
  
  // Read the input file storing particles immediately because all are alive
  if (this->inputType == RECORD) {
    readFromRecordFile();
  } else {
    readFromBlockFile();
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Input file is RECORD structured so read each particle record and populate
// the vectors of particles marking all as ALIVE
//
/////////////////////////////////////////////////////////////////////////////

void ParticleDistribute::readFromRecordFile()
{
  // Only one file per processor named in index 0
  ifstream inStream(this->inFiles[0].c_str(), ios::in);
  int numberOfParticles = this->fileParticles[0];

#ifndef USE_VTK_COSMO
  cout << "Rank " << this->myProc << " open file " << this->inFiles[0]
       << " with " << numberOfParticles << " particles" << endl;
#endif

  POSVEL_T* fBlock = new POSVEL_T[COSMO_FLOAT];
  ID_T* iBlock = new ID_T[COSMO_INT];

  // Store each particle location, velocity and tag
  for (int i = 0; i < numberOfParticles; i++) {

    // Set file pointer to the requested particle
    inStream.read(reinterpret_cast<char*>(fBlock),
                   COSMO_FLOAT * sizeof(POSVEL_T));

    if (inStream.gcount() != COSMO_FLOAT * sizeof(POSVEL_T)) {
#ifdef USE_VTK_COSMO
      vtkOutputWindowDisplayErrorText("Premature end-of-file.\n");
      inStream.close();
      delete [] fBlock;
      delete [] iBlock;

      return;
#else
      cout << "Premature end-of-file" << endl;
      exit (-1);
#endif
    }

    inStream.read(reinterpret_cast<char*>(iBlock),
                   COSMO_INT * sizeof(ID_T));

    if (inStream.gcount() != COSMO_INT * sizeof(ID_T)) {
#ifdef USE_VTK_COSMO
      vtkOutputWindowDisplayErrorText("Premature end-of-file.\n");
      inStream.close();
      delete [] fBlock;
      delete [] iBlock;

      return;
#else
      cout << "Premature end-of-file" << endl;
      exit (-1);
#endif
    }

    // Store location and velocity in buffer but not the constant mass
    this->xx->push_back(fBlock[0]);
    this->vx->push_back(fBlock[1]);
    this->yy->push_back(fBlock[2]);
    this->vy->push_back(fBlock[3]);
    this->zz->push_back(fBlock[4]);
    this->vz->push_back(fBlock[5]);
    this->ms->push_back(fBlock[6]);
    this->tag->push_back(iBlock[0]);

    this->numberOfAliveParticles++;
    this->particleCount++;
  }

  inStream.close();
  delete [] fBlock;
  delete [] iBlock;
}

/////////////////////////////////////////////////////////////////////////////
//
// Input file is BLOCK structured so read head and each block of data.
// Gadget-2 format:
//    SKIP_H 4 bytes (size of header)
//    Header
//    SKIP_H 4 bytes (size of header)
//
//    SKIP_L 4 bytes (size of location block in bytes)
//    Block of location data where each particle's x,y,z is stored together
//    SKIP_L 4 bytes (size of location block in bytes)
//
//    SKIP_V 4 bytes (size of velocity block in bytes)
//    Block of velocity data where each particle's xv,yv,zv is stored together
//    SKIP_V 4 bytes (size of velocity block in bytes)
//
//    SKIP_T 4 bytes (size of tag block in bytes)
//    Block of tag data
//    SKIP_T 4 bytes (size of tag block in bytes)
//
/////////////////////////////////////////////////////////////////////////////

void ParticleDistribute::readFromBlockFile()
{
  // Only one file per processor named in index 0
  ifstream inStream(this->inFiles[0].c_str(), ios::in);
  int numberOfParticles = this->fileParticles[0];

#ifndef USE_VTK_COSMO
  cout << "Rank " << this->myProc << " open file " << this->inFiles[0]
       << " with " << numberOfParticles << " particles" << endl;
#endif

  // Allocate buffers for block reads
  POSVEL_T* fBlock = new POSVEL_T[numberOfParticles * 3];
  ID_T* iBlock = new ID_T[numberOfParticles];

  // Seek to particle locations and read triples
  // Skip (SKIP_H, header, SKIP_H, SKIP_L) to get to locations
  int skip = this->headerSize + (3 * sizeof(int));
  inStream.seekg(skip, ios::beg);
  inStream.read(reinterpret_cast<char*>(fBlock),
                3 * numberOfParticles * sizeof(POSVEL_T));

  // Store the locations in the double buffer in record order
  for (int p = 0; p < numberOfParticles; p++) {
    this->xx->push_back(fBlock[0]);
    this->yy->push_back(fBlock[1]);
    this->zz->push_back(fBlock[2]);
  }

  // Seek to particle velocities and read triples
  // Skip (SKIP_L, SKIP_V)
  skip = 2 * sizeof(int);
  inStream.seekg(skip, ios::cur);
  inStream.read(reinterpret_cast<char*>(fBlock),
                3 * numberOfParticles * sizeof(POSVEL_T));

  // Store the velocities in the double buffer in record order
  for (int p = 0; p < numberOfParticles; p++) {
    this->vx->push_back(fBlock[0]);
    this->vy->push_back(fBlock[1]);
    this->vz->push_back(fBlock[2]);
  }

  // Store the constant mass
  POSVEL_T mass = 1.0;
  for (int p = 0; p < numberOfParticles; p++) {
    this->ms->push_back(mass);
  }

  // Seek to first particle tags and read
  // Skip (SKIP_V, SKIP_T)
  skip = 2 * sizeof(int);
  inStream.seekg(skip, ios::cur);
  inStream.read(reinterpret_cast<char*>(iBlock),
                1 * numberOfParticles * sizeof(ID_T));

  // Store the tags in the double buffer for sharing in record order
  for (int p = 0; p < numberOfParticles; p++) {
    this->tag->push_back(iBlock[0]);
    this->numberOfAliveParticles++;
    this->particleCount++;
  }

  inStream.close();
  delete [] fBlock;
  delete [] iBlock;
}
