/////////////////////////////////////////////////////////////////////////////
// 
// VPICGlobal class contains common information for a single VPICDataSet run
// including information about the directory structure, file names, problem
// size physical and grid information, variable information
//
/////////////////////////////////////////////////////////////////////////////

#ifndef VPICGlobal_h
#define VPICGlobal_h

#include "VPICDefinition.h"
#include "VPICHeader.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class VPIC_EXPORT VPICGlobal {
public:
   VPICGlobal();
   ~VPICGlobal();

   // Read basic information about files, sizes and variables from .vpc file
   void readGlobal(const string& inFile);
   void readFieldVariables(ifstream& inStr);
   void readSpeciesVariables(ifstream& inStr);
   void getKeyword(char* inBuf, string& keyword, string& rest);

   // Build the directory structure for accessing data files
   void buildFileNames();

   // Build the file decomposition structure for file access
   void buildFileLayoutTable();

   // Gather information about variables for general use in reader
   void initializeVariables();

   // For dynamic viewing of a running VPIC, to collect new time steps
   void addNewTimeSteps();

   // Variable information
   int    getNumberOfVariables()        { return this->numberOfVariables; }
   string getVariableName(int var)      { return this->variableName[var]; }
   int    getVariableStruct(int var)    { return this->variableStruct[var]; }
   int    getVariableKind(int var)      { return this->variableKind[var]; }
   int    getVariableType(int var)      { return this->variableType[var]; }
   int    getVariableByteCount(int var) { return this->variableByteCount[var]; }
   long int getVariableOffset(int var, int comp)
                                { return this->variableOffset[var][comp]; }

   // File information
   int*   getLayoutSize()               { return this->layoutSize; }
   int*** getLayoutID()                 { return this->layoutID; }
   int*   getPartSize()                 { return this->partSize; }
   int    getNumberOfParts()            { return this->numberOfFiles; }

   int    getNumberOfDirectories()      { return this->numberOfDirectories; }
   int    getNumberOfTimeSteps()        { return this->numberOfTimeSteps; }

   string getDirectoryName(int i)       { return this->directoryName[i]; }
   string getBaseFileName(int i)        { return this->baseFileName[i]; }
   string getDumpName(int time)         { return this->dumpName[time]; }
   int    getDumpTime(int time)         { return this->dumpTime[time]; }

   int    getTimeFieldLen()             { return this->timeFieldLen; }
   int    getProcFieldLen()             { return this->procFieldLen; }

   // Grid and physical grid information
   float* getPhysicalOrigin()           { return this->physicalOrigin; }
   float* getPhysicalStep()             { return this->physicalStep; }

   void   PrintSelf(ostream& os, int indent);

private:
   string   globalFile;                 // Name of .vpc file
   string   headerVersion;              // Version of VPIC
   int      headerSize;                 // Size of header on every data file
   VPICHeader header;                   // Header information

   // File information
   int      numberOfDirectories;        // Field plus species directories
   string*  directoryName;              // Full name of field and species dirs
   string*  baseFileName;               // Base file name matching directory
   int      numberOfFiles;              // Number of data files for problem

   int      layoutSize[DIMENSION];      // Simulation decomposition of files
   int***   layoutID;                   // Numerical ID of file
   int      partSize[DIMENSION];        // Size of data on each file

   // Physical information
   float    physicalExtent[DIMENSION*2];
   float    physicalOrigin[DIMENSION];
   float    physicalStep[DIMENSION];

   // Field variable information
   string   fieldDirectory;             // Directory
   string   fieldBaseName;              // Start of every file name
   int      fieldVarCount;              // Number of variables
   string*  fieldName;                  // Variable name
   int*     fieldStructType;            // SCALAR, VECTOR, TENSOR
   int*     fieldCompSize;              // Number of components
   int*     fieldBasicType;             // FLOAT, INTEGER
   int*     fieldByteCount;             // Bytes per basic type

   // Species variable information
   int      speciesCount;               // Number of other data directories
   string*  speciesDirectory;           // Directory
   string*  speciesBaseName;            // Start of every file name
   int*     speciesVarCount;            // Number of variables
   string** speciesName;                // Variable name
   int**    speciesStructType;          // SCALAR, VECTOR, TENSOR
   int**    speciesCompSize;            // Number of components
   int**    speciesBasicType;           // FLOAT, INTEGER
   int**    speciesByteCount;           // Bytes per basic type

   // Time information
   int      numberOfTimeSteps;          // Number of time steps
   vector<string> dumpName;             // Dump subdirectory names
   vector<int>    dumpTime;             // Dump subdirectory times
   int      procFieldLen;               // fields.tttttt.pppp part names
   int      timeFieldLen;               // fields.tttttt.pppp part names

   // Variable information
   int      numberOfVariables;          // All field and species variables
   string*  variableName;               // Names of variables for reader
   int*     variableStruct;             // SCALAR, VECTOR, TENSOR
   int*     variableType;               // DOUBLE, FLOAT, INTEGER
   int*     variableByteCount;          // Bytes per basic type
   int*     variableKind;               // Field or which species
   long int** variableOffset;           // Offset in file to variable for fseek
};

#endif
