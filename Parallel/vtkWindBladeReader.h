/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindBladeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWindBladeReader - class for reading WindBlade data files
// .SECTION Description
// vtkWindBladeReader is a source object that reads WindBlade files
// which are block binary files with tags before and after each block
// giving the number of bytes within the block.  The number of data
// variables dumped varies.  There are 3 output ports with the first
// being a structured grid with irregular spacing in the Z dimension.
// The second is an unstructured grid only read on on process 0 and
// used to represent the blade.  The third is also a structured grid
// with irregular spacing on the Z dimension.  Only the first and
// second output ports have time dependent data.

#ifndef __vtkWindBladeReader_h
#define __vtkWindBladeReader_h


#include "vtkStructuredGridAlgorithm.h"

class vtkWindBladeReaderPiece;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkStringArray;
class vtkFloatArray;
class vtkIntArray;
class vtkPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkMultiBlockDataSetAglorithm;
class vtkStructuredGridAlgorithm;
class WindBladeReaderInternal;

class VTK_PARALLEL_EXPORT vtkWindBladeReader : public vtkStructuredGridAlgorithm
{
public:
  static vtkWindBladeReader *New();
  vtkTypeMacro(vtkWindBladeReader,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  vtkSetVector6Macro(SubExtent, int);
  vtkGetVector6Macro(SubExtent, int);

  // Description:
  // Get the reader's output
  vtkStructuredGrid *GetFieldOutput();    // Output port 0
  vtkUnstructuredGrid *GetBladeOutput();  // Output port 1
  vtkStructuredGrid *GetGroundOutput();    // Output port 2

  // Description:
  // The following methods allow selective reading of solutions fields.
  // By default, ALL data fields on the nodes are read, but this can
  // be modified.
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);

  int  GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);

  void DisableAllPointArrays();
  void EnableAllPointArrays();

  // Description:
  // We intercept the requests to check for which port
  // information is being requested for and if there is
  // a REQUEST_DATA_NOT_GENERATED request then we mark
  // which ports won't have data generated for that request.
  virtual int ProcessRequest(vtkInformation *request,
                             vtkInformationVector **inInfo,
                             vtkInformationVector *outInfo);

protected:
  vtkWindBladeReader();
  ~vtkWindBladeReader();

  char* Filename;   // Base file name

  int Rank;    // Number of this processor
  int TotalRank;   // Number of processors

  // Extent information
  vtkIdType NumberOfTuples;  // Number of tuples in subextent

  // Field
  int WholeExtent[6];  // Extents of entire grid
  int SubExtent[6];  // Processor grid extent
  int UpdateExtent[6];
  int Dimension[3];  // Size of entire grid
  int SubDimension[3];  // Size of processor grid

  // Ground
  int GExtent[6];      // Extents of ground grid
  int GSubExtent[6];  // Processor grid extent
  int GDimension[3];   // Size of ground grid

  float Step[3];  // Spacing delta
  int UseTopographyFile;  // Topography or flat
  vtkStdString TopographyFile;  // Name of topography data file
  vtkPoints* Points;   // Structured grid geometry
  vtkPoints* GPoints;   // Structured grid geometry for ground
  vtkPoints* BPoints;   // Unstructured grid geometry
  float Compression;   // Stretching at Z surface [0,1]
  float Fit;    // Cubic or quadratic [0,1]

  // Rectilinear coordinate spacing
  vtkFloatArray* XSpacing;
  vtkFloatArray* YSpacing;
  vtkFloatArray* ZSpacing;
  float* ZTopographicValues;
  float ZMinValue;

  // Variable information
  int NumberOfFileVariables;  // Number of variables in data file
  int NumberOfDerivedVariables;  // Number of variables derived from file
  int NumberOfVariables;  // Number of variables to display

  vtkStringArray* DivideVariables; // Divide data by density at read
  vtkStdString* VariableName;  // Names of each variable
  int* VariableStruct;   // SCALAR or VECTOR
  int* VariableCompSize;  // Number of components
  int* VariableBasicType;  // FLOAT or INTEGER
  int* VariableByteCount;  // Number of bytes in basic type
  long int* VariableOffset;  // Offset into data file
  unsigned int BlockSize;   // Size of every data block
  int GBlockSize;  // Size of every data block

  vtkFloatArray** Data;   // Actual data arrays
  vtkStdString RootDirectory; // Directory where the .wind file is.
  vtkStdString DataDirectory;  // Location of actual data
  vtkStdString DataBaseName;  // Base name of files

  // Time step information
  int NumberOfTimeSteps;  // Number of time steps
  int TimeStepFirst;   // First time step
  int TimeStepLast;   // Last time step
  int TimeStepDelta;   // Delta on time steps
  double* TimeSteps;   // Actual times available for request

  // Turbine information
  int NumberOfBladeTowers;  // Number of turbines
  int NumberOfBladePoints;  // Points for drawing parts of blades
  int NumberOfBladeCells;  // Turbines * Blades * Parts

  vtkFloatArray* XPosition;  // Location of tower
  vtkFloatArray* YPosition;  // Location of tower
  vtkFloatArray* HubHeight;  // Height of tower
  vtkFloatArray* AngularVeloc; // Angular Velocity
  vtkFloatArray* BladeLength; // Blade length
  vtkIntArray* BladeCount;  // Number of blades per tower

  int UseTurbineFile;   // Turbine data available
  vtkStdString TurbineDirectory; // Turbine unstructured data
  vtkStdString TurbineTowerName; // Name of tower file
  vtkStdString TurbineBladeName; // Base name of time series blade data
  int NumberOfLinesToSkip;  // New format has lines that need to be skipped in
                            // blade files

  // Selected field of interest
  vtkDataArraySelection* PointDataArraySelection;

  // Observer to modify this object when array selections are modified
  vtkCallbackCommand* SelectionObserver;

  // Controlls initializing and querrying MPI
  vtkMultiProcessController * MPIController;

  // Read the header file describing the dataset
  bool ReadGlobalData();
  void ReadDataVariables(istream& inStr);
  bool FindVariableOffsets();

  // Turbine methods
  void SetupBladeData();
  void LoadBladeData(int timeStep);

  // Calculate the coordinates
  void FillCoordinates();
  void FillGroundCoordinates();
  void CreateCoordinates();
  void CreateZTopography(float* zdata);
  float GDeform(float sigma, float sigmaMax, int flag);
  void Spline(float* x, float* y, int n, float yp1, float ypn, float* y2);
  void Splint(float* xa, float* ya, float* y2a, int n, float x, float* y, int);

  // Load a variable from data file
  void LoadVariableData(int var);

  // Variables which must be divided by density after being read from file
  void DivideByDensity(const char* name);

  // Calculate derived variables
  void CalculateVorticity(int vort, int uvw, int density);
  void CalculatePressure(int pres, int prespre, int tempg, int density);

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  static void SelectionCallback(
    vtkObject *caller,
    unsigned long eid,
    void *clientdata,
    void *calldata);

  static void EventCallback(
    vtkObject* caller,
    unsigned long eid,
    void* clientdata, void* calldata);

  virtual int FillOutputPortInformation(int, vtkInformation*);

  WindBladeReaderInternal * Internal;

private:
  vtkWindBladeReader(const vtkWindBladeReader&);  // Not implemented.
  void operator=(const vtkWindBladeReader&);  // Not implemented.
};
#endif

