#include "VPICGlobal.h"
#include "VPICDefinition.h"



#include <sys/types.h>
#include <vtksys/Directory.hxx>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef WIN32
const static char * Slash = "\\";
#else
const static char * Slash = "/";
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Global information for a VPIC run tells the problem size, location of
// data files relative to the global *.vpc file, and what variables in
// which order have been dumped to the data files
//
//////////////////////////////////////////////////////////////////////////////

VPICGlobal::VPICGlobal()
{
   this->numberOfTimeSteps = 1;
}

VPICGlobal::~VPICGlobal()
{
  delete [] this->fieldName;
  delete [] this->fieldStructType;
  delete [] this->fieldCompSize;
  delete [] this->fieldBasicType;
  delete [] this->fieldByteCount;

  for (int s = 0; s < this->speciesCount; s++) {
     delete [] this->speciesName[s];
     delete [] this->speciesStructType[s];
     delete [] this->speciesCompSize[s];
     delete [] this->speciesBasicType[s];
     delete [] this->speciesByteCount[s];
  }
  delete [] this->speciesName;
  delete [] this->speciesStructType;
  delete [] this->speciesCompSize;
  delete [] this->speciesBasicType;
  delete [] this->speciesByteCount;

   delete [] this->variableName;
   delete [] this->variableStruct;
   delete [] this->variableType;
   delete [] this->variableByteCount;
   delete [] this->variableKind;
   for (int var = 0; var < this->numberOfVariables; var++)
      delete [] this->variableOffset[var];
   delete [] this->variableOffset;

   delete [] this->directoryName;
   delete [] this->baseFileName;
}

//////////////////////////////////////////////////////////////////////////////
//
// Read the global information
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::readGlobal(const string& inFile)
{
  this->globalFile = inFile;

  ifstream inStr(this->globalFile.c_str());
  if (!inStr) {
    cout << "Could not open the global .vpc file" << endl;
  }

  char inBuf[LINESIZE];
  string keyword;
  string rest;
  float gridDelta, gridCVac, gridEps;

  while (inStr.getline(inBuf, LINESIZE)) {
    if (inBuf[0] != '#' && inStr.gcount() > 1) {

      getKeyword(inBuf, keyword, rest);
      istringstream line(rest.c_str());

      // Header information
      if (keyword == "VPIC_HEADER_VERSION")
        line >> this->headerVersion;
      else if (keyword == "DATA_HEADER_SIZE")
        line >> this->headerSize;

      // Parameters
      else if (keyword == "GRID_DELTA_T")
        line >> gridDelta;
      else if (keyword == "GRID_CVAC")
        line >> gridCVac;
      else if (keyword == "GRID_EPS")
        line >> gridEps;

      // Physical extents
      else if (keyword == "GRID_EXTENTS_X")
      {
        line >> this->physicalExtent[0] >> this->physicalExtent[1];
        this->physicalOrigin[0] = this->physicalExtent[0];
      }
      else if (keyword == "GRID_EXTENTS_Y")
      {
        line >> this->physicalExtent[2] >> this->physicalExtent[3];
        this->physicalOrigin[1] = this->physicalExtent[2];
      }
      else if (keyword == "GRID_EXTENTS_Z")
      {
        line >> this->physicalExtent[4] >> this->physicalExtent[5];
        this->physicalOrigin[2] = this->physicalExtent[4];
      }

      // Physical steps
      else if (keyword == "GRID_DELTA_X")
        line >> this->physicalStep[0];
      else if (keyword == "GRID_DELTA_Y")
        line >> this->physicalStep[1];
      else if (keyword == "GRID_DELTA_Z")
        line >> this->physicalStep[2];

      // Simulation topology
      else if (keyword == "GRID_TOPOLOGY_X")
        line >> this->layoutSize[0];
      else if (keyword == "GRID_TOPOLOGY_Y")
        line >> this->layoutSize[1];
      else if (keyword == "GRID_TOPOLOGY_Z")
        line >> this->layoutSize[2];

      // Field variables
      else if (keyword == "FIELD_DATA_DIRECTORY")
        this->fieldDirectory = rest;
      else if (keyword == "FIELD_DATA_BASE_FILENAME")
        this->fieldBaseName = rest;
      else if (keyword == "FIELD_DATA_VARIABLES") {
        line >> this->fieldVarCount;
        readFieldVariables(inStr);
      }

      // Species variables
      else if (keyword == "NUM_OUTPUT_SPECIES") {
        line >> this->speciesCount;
        readSpeciesVariables(inStr);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// Read the field variable information
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::readFieldVariables(ifstream& inStr)
{
  char inBuf[LINESIZE];
  string structType, basicType;

  this->fieldName = new string[this->fieldVarCount];
  this->fieldStructType = new int[this->fieldVarCount];
  this->fieldCompSize = new int[this->fieldVarCount];
  this->fieldBasicType = new int[this->fieldVarCount];
  this->fieldByteCount = new int[this->fieldVarCount];

  for (int i = 0; i < this->fieldVarCount; i++) {
    inStr.getline(inBuf, LINESIZE);

    // Variable name
    string varLine(inBuf);
    string::size_type lastPos = varLine.rfind('"');
    this->fieldName[i] = varLine.substr(1, lastPos-1);

    // Structure, number of components, type, number of bytes
    string rest = varLine.substr(lastPos+1);
    istringstream line(rest);

    line >> structType;
    line >> this->fieldCompSize[i];

    if (structType == "SCALAR")
      this->fieldStructType[i] = SCALAR;
    else if (structType == "VECTOR")
      this->fieldStructType[i] = VECTOR;
    else if (structType == "TENSOR" && this->fieldCompSize[i] == 6)
      this->fieldStructType[i] = TENSOR;
    else if (structType == "TENSOR" && this->fieldCompSize[i] == 9)
      this->fieldStructType[i] = TENSOR9;
    else
      cout << "Error in structure type " << structType << endl;

    line >> basicType;
    line >> this->fieldByteCount[i];

    if (basicType == "FLOATING_POINT")
      this->fieldBasicType[i] = FLOAT;
    else if (basicType == "INTEGER")
      this->fieldBasicType[i] = INTEGER;
    else
      cout << "Error in basic type " << basicType << endl;
  }
}

//////////////////////////////////////////////////////////////////////////////
//
// Read the species variable information
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::readSpeciesVariables(ifstream& inStr)
{
  char inBuf[LINESIZE];
  string keyword, rest;
  string structType, basicType;

  this->speciesDirectory = new string[this->speciesCount];
  this->speciesBaseName = new string[this->speciesCount];
  this->speciesVarCount = new int[this->speciesCount];
  this->speciesName = new string*[this->speciesCount];
  this->speciesStructType = new int*[this->speciesCount];
  this->speciesCompSize = new int*[this->speciesCount];
  this->speciesBasicType = new int*[this->speciesCount];
  this->speciesByteCount = new int*[this->speciesCount];

  int s = 0;
  while (inStr.getline(inBuf, LINESIZE)) {
    if (inBuf[0] != '#' && inStr.gcount() > 1) {

      getKeyword(inBuf, keyword, rest);
      istringstream line(rest.c_str());

      if (keyword == "SPECIES_DATA_DIRECTORY")
        this->speciesDirectory[s] = rest;
      else if (keyword == "SPECIES_DATA_BASE_FILENAME")
        this->speciesBaseName[s] = rest;
      else if (keyword == "HYDRO_DATA_VARIABLES") {

        line >> this->speciesVarCount[s];
        this->speciesName[s] = new string[this->speciesVarCount[s]];
        this->speciesStructType[s] = new int[this->speciesVarCount[s]];
        this->speciesCompSize[s] = new int[this->speciesVarCount[s]];
        this->speciesBasicType[s] = new int[this->speciesVarCount[s]];
        this->speciesByteCount[s] = new int[this->speciesVarCount[s]];
                  
        for (int i = 0; i < this->speciesVarCount[s]; i++) {
          inStr.getline(inBuf, LINESIZE);

          // Variable name
          string varLine(inBuf);
          string::size_type lastPos = varLine.rfind('"');
          //this->speciesName[s][i] = varLine.substr(1, lastPos-1);
          this->speciesName[s][i] = varLine.substr(1, lastPos-1);
          this->speciesName[s][i] += "(";
          this->speciesName[s][i] += this->speciesBaseName[s];
          this->speciesName[s][i] += ")";

          // Structure, number of components, type, number of bytes
          string rest = varLine.substr(lastPos+1);
          istringstream line(rest.c_str());

          line >> structType;
          line >> this->speciesCompSize[s][i];

          if (structType == "SCALAR")
            this->speciesStructType[s][i] = SCALAR;
          else if (structType == "VECTOR")
            this->speciesStructType[s][i] = VECTOR;
          else if (structType == "TENSOR" && this->speciesCompSize[s][i] == 6)
            this->speciesStructType[s][i] = TENSOR;
          else if (structType == "TENSOR" && this->speciesCompSize[s][i] == 9)
            this->speciesStructType[s][i] = TENSOR9;
          else
            cout << "Error in structure type " << structType << endl;
      
          line >> basicType;
          line >> this->speciesByteCount[s][i];

          if (basicType == "FLOATING_POINT")
            this->speciesBasicType[s][i] = FLOAT;
          else if (basicType == "INTEGER")
            this->speciesBasicType[s][i] = INTEGER;
          else
            cout << "Error in basic type " << basicType << endl;
        }
        s++;
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Keywords start in position 0 and are delimited by white space
//
/////////////////////////////////////////////////////////////////////////////

void VPICGlobal::getKeyword(char* inBuf, string& keyword, string& rest)
{
  string line(inBuf);
  string::size_type keyPos = line.find(' ');
  keyword = line.substr(0, keyPos);
  rest = line.substr(keyPos + 1);
}

//////////////////////////////////////////////////////////////////////////////
//
// Build the subdirectory names for each dump and each type of data
// Locate enough information so that all part names can be built
//
// Each of those has a subdirectory per time step of the form "T.time"
// Each time step has files of the form "name.tttttt.pppp"
//   where name is "fields", "ehydro", "Hhydro"
//   where tttttt is zero filled integer time
//   where pppp is zero filled simulation processor id
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::buildFileNames()
{
   ostringstream tempStr;

   // Get the number of data directories in this data directory
   // Field directory plus a number of species directories
   this->numberOfDirectories = this->speciesCount + 1;
   this->directoryName = new string[this->numberOfDirectories];
   this->baseFileName = new string[this->numberOfDirectories];

   // From the full path name of the .vpc file find the directory name
   string::size_type dirPos = this->globalFile.rfind(Slash);
   if (dirPos == string::npos) {
      cout << "Bad input file name " << this->globalFile << endl;
      exit(1);
   }
   string dirName = this->globalFile.substr(0, dirPos);

   // Field directory information in first index position
   tempStr << dirName << Slash << this->fieldDirectory << Slash;
   this->directoryName[0] = tempStr.str();
   this->baseFileName[0] = this->fieldBaseName;

   // Species directory information follows
   for (int s = 0; s < this->speciesCount; s++) {
      tempStr.str("");
      tempStr << dirName << Slash << this->speciesDirectory[s] << Slash;
      this->directoryName[s+1] = tempStr.str();
      this->baseFileName[s+1] = this->speciesBaseName[s];
   }


   // Get the dump subdirectory names which give the time steps
   char dummy;
   int dtime;

   vtksys::Directory * dir = new vtksys::Directory();
   unsigned long numFiles = 0;

   if (dir->Load(this->directoryName[0].c_str()) != false) {
     numFiles = dir->GetNumberOfFiles();
     for(unsigned long i = 0; i < numFiles; i++) {
         string fileName = dir->GetFile(i);
         if (fileName[0] == 'T') {
            istringstream timeStr(fileName);
            timeStr >> dummy >> dummy >> dtime;
            this->dumpTime.push_back(dtime);
         }
      }
   }
   dir->Clear();

   // Names are T.time which is not 0 filled so we must sort
   sort(this->dumpTime.begin(), this->dumpTime.end());
   this->numberOfTimeSteps = static_cast<int>(this->dumpTime.size());

   // Recompose the dump names using the sorted times
   for (int dump = 0; dump < this->numberOfTimeSteps; dump++) {
      tempStr.str("");
      tempStr << "T." << this->dumpTime[dump];
      this->dumpName.push_back(tempStr.str());
   }

   // Get actual data file to use as a template in forming the names
   // Sort so that we can look at the first (processor 0) file
   vector<string> fieldNames;
   tempStr.str("");
   tempStr << this->directoryName[0] << this->dumpName[0];
   dirName = tempStr.str();

   if (dir->Load(dirName.c_str()) != false) {
     numFiles = dir->GetNumberOfFiles();
     for(unsigned long i = 0; i < numFiles; i++) {
       string fileName = dir->GetFile(i);
       if (fileName.find(this->baseFileName[0]) != string::npos) {
         fieldNames.push_back(fileName);
       }
     }
   }
   sort(fieldNames.begin(), fieldNames.end());
   string fieldName = fieldNames[0];

   dir->Clear();
   delete dir;

   // Get the size of data per variable per part for calculating offsets
   tempStr << Slash << fieldName;
   FILE* filePtr = fopen(tempStr.str().c_str(), "r");
   this->header.readHeader(filePtr);
   this->numberOfFiles = this->header.getTotalRank();
   this->header.getGridSize(this->partSize);
   fclose(filePtr);
 
   // Use the template of the input file to determine the name format
   // so that file names can be built knowing the time step and part
   // Back up from end to get proc field size to first '.'
   // Back up from that point to get the time field size
   // fields.tttttt.pppp for instance
   //
   string::size_type ppos = fieldName.rfind(".");
   this->procFieldLen = static_cast<int>(fieldName.size() - ppos - 1);
   string::size_type tpos = fieldName.rfind(".", ppos-1);
   this->timeFieldLen = static_cast<int>(fieldName.size() - tpos - this->procFieldLen - 2);
}

//////////////////////////////////////////////////////////////////////////////
//
// Simulation decomposition (arrangement of input files within the problem)
// is contained in VPICGlobal which gives the number of processors in each
// dimension which produced data.  Assume first dimension varies the fastest
// and build the 3D table with the part id.
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::buildFileLayoutTable()
{
   // Allocate the partition ID table with one entry for every file
   this->layoutID = new int**[this->layoutSize[0]];
   for (int i = 0; i < this->layoutSize[0]; i++) {
      this->layoutID[i] = new int*[this->layoutSize[1]];
      for (int j = 0; j < this->layoutSize[1]; j++)
         this->layoutID[i][j] = new int[this->layoutSize[2]];
   }

   int id = 0;
   for (int k = 0; k < this->layoutSize[2]; k++)
      for (int j = 0; j < this->layoutSize[1]; j++)
         for (int i = 0; i < this->layoutSize[0]; i++)
            this->layoutID[i][j][k] = id++;
}

/////////////////////////////////////////////////////////////////////////////
//
// Initialize variables for the VPIC field and hydro grids
//
/////////////////////////////////////////////////////////////////////////////

void VPICGlobal::initializeVariables()
{  
   // Initialize the variables in this data set
   int partGhostSize[DIMENSION];
   this->header.getGhostSize(partGhostSize);
   int blockSize = 1;
   for (int dim = 0; dim < DIMENSION; dim++)
      blockSize *= partGhostSize[dim];

   // Total variables in fields and all species
   this->numberOfVariables = this->fieldVarCount;
   for (int s = 0; s < this->speciesCount; s++)
      this->numberOfVariables += this->speciesVarCount[s];
   
   // Allocate storage for variable descriptions
   this->variableName = new string[this->numberOfVariables];
   this->variableStruct = new int[this->numberOfVariables];
   this->variableType = new int[this->numberOfVariables];
   this->variableByteCount = new int[this->numberOfVariables];
   this->variableKind = new int[this->numberOfVariables];
   this->variableOffset = new long int*[this->numberOfVariables];
   for (int var = 0; var < this->numberOfVariables; var++)
      this->variableOffset[var] = new long int[TENSOR_DIMENSION];
   
   // Offset to first data block is header size
   long int offset = this->headerSize;
   int varIndex = 0;
   int fileIndex = 0;

   for (int i = 0; i < this->fieldVarCount; i++) {
      this->variableName[varIndex] = this->fieldName[i];
      this->variableStruct[varIndex] = this->fieldStructType[i];
      this->variableType[varIndex] = this->fieldBasicType[i];
      this->variableByteCount[varIndex] = this->fieldByteCount[i];
      this->variableKind[varIndex] = fileIndex;

      for (int comp = 0; comp < this->fieldCompSize[i]; comp++) {
         this->variableOffset[varIndex][comp] = offset;
         offset += (blockSize * this->fieldByteCount[i]);
      }
      varIndex++;
   }
   fileIndex++;

   // Species variables
   for (int s = 0; s < this->speciesCount; s++) {

      offset = this->headerSize;
      for (int i = 0; i < this->speciesVarCount[s]; i++) {

         this->variableName[varIndex] = this->speciesName[s][i];
         this->variableStruct[varIndex] = this->speciesStructType[s][i];
         this->variableType[varIndex] = this->speciesBasicType[s][i];
         this->variableByteCount[varIndex] = this->speciesByteCount[s][i];
         this->variableKind[varIndex] = fileIndex;

         for (int comp = 0; comp < this->speciesCompSize[s][i]; comp++) {
            this->variableOffset[varIndex][comp] = offset;
            offset += (blockSize * this->speciesByteCount[s][i]);
         }
         varIndex++;
      }
      fileIndex++;
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Search main directory for additional time step subdirectories
// If found increase number of time steps and add name and time to
// vectors so that they are available for use
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::addNewTimeSteps()
{
   // Get the dump subdirectory names
   char dummy;
   int dtime;

   vtksys::Directory * dir = new vtksys::Directory();
   unsigned long numFiles = 0;

   vector<int> newTime;
   if (dir->Load(this->directoryName[0].c_str()) != false) {
     numFiles = dir->GetNumberOfFiles();
     for(unsigned long i = 0; i < numFiles; i++) {
         string fileName = dir->GetFile(i);
         if (fileName[0] == 'T') {
            istringstream timeStr(fileName);
            timeStr >> dummy >> dummy >> dtime;
            newTime.push_back(dtime);
         }
      }
   }
   dir->Clear();
   delete dir;

   // If we have additional time subdirectories add to list of times and names
   if (newTime.size() > this->numberOfTimeSteps) {

      this->dumpTime.clear();
      this->dumpName.clear();

      // Names are T.time which is not 0 filled so we must sort
      sort(newTime.begin(), newTime.end());
      this->numberOfTimeSteps = static_cast<int>(newTime.size());

      // Recompose the dump names using the sorted times
      for (int dump = 0; dump < this->numberOfTimeSteps; dump++) {
         this->dumpTime.push_back(newTime[dump]);
         ostringstream dname;
         dname << "T." << this->dumpTime[dump];
         this->dumpName.push_back(dname.str());
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
//
// Print global information about the VPIC data
//
//////////////////////////////////////////////////////////////////////////////

void VPICGlobal::PrintSelf(ostream& os, int vpicNotUsed(indent))
{
  os << endl;
  os << "Header version:\t" << this->headerVersion << endl;
  os << "Header size:\t" << this->headerSize << endl;
  os << endl;

  os << "Physical extent:\t"
     << "[" << this->physicalExtent[0] << ":" << this->physicalExtent[1] << "]"
     << "[" << this->physicalExtent[2] << ":" << this->physicalExtent[3] << "]"
     << "[" << this->physicalExtent[4] << ":" << this->physicalExtent[5] << "]"
     << endl;
  os << "Physical delta:\t" << "[" 
     << this->physicalStep[0] << "," 
     << this->physicalStep[1] << "," 
     << this->physicalStep[2] << "]" << endl;
  os << "Simulation topology:\t" << "[" 
     << this->layoutSize[0] << "," 
     << this->layoutSize[1] << "," 
     << this->layoutSize[2] << "]" << endl;
  os << endl;

  os << "Field directory: " << this->fieldDirectory << endl;
  os << "Field base name: " << this->fieldBaseName << endl;
  os << "Field variable count: " << this->fieldVarCount << endl;
  for (int i = 0; i < this->fieldVarCount; i++) {
    os << "\t" << left << setw(25) << this->fieldName[i];
    if (this->fieldStructType[i] == SCALAR)
      os << "\tSCALAR";
    else if (this->fieldStructType[i] == VECTOR)
      os << "\tVECTOR";
    else if (this->fieldStructType[i] == TENSOR)
      os << "\tTENSOR";
    else if (this->fieldStructType[i] == TENSOR9)
      os << "\tTENSOR9";
    os << "\t" << this->fieldCompSize[i]
       << "\t" << this->fieldBasicType[i]
       << "\t" << this->fieldByteCount[i] << endl;
    os << endl;
  }

  for (int s = 0; s < this->speciesCount; s++) {
    os << "Species directory: " << this->speciesDirectory[s] << endl;
    os << "Species base name: " << this->speciesBaseName[s] << endl;
    os << "Species variable count: " << this->speciesVarCount[s] << endl;
    for (int i = 0; i < this->speciesVarCount[s]; i++) {
      os << "\t" << left << setw(25) << this->speciesName[s][i];
      if (this->speciesStructType[s][i] == SCALAR)
        os << "\tSCALAR";
      else if (this->speciesStructType[s][i] == VECTOR)
        os << "\tVECTOR";
      else if (this->speciesStructType[s][i] == TENSOR)
        os << "\tTENSOR";
      else if (this->speciesStructType[s][i] == TENSOR9)
        os << "\tTENSOR9";
      os << "\t" << this->speciesCompSize[s][i]
         << "\t" << this->speciesBasicType[s][i]
         << "\t" << this->speciesByteCount[s][i] << endl;
    }
  }
  os << endl;
}
