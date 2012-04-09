/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockPLOT3DReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockPLOT3DReader - read PLOT3D data files
// .SECTION Description
// vtkMultiBlockPLOT3DReader is a reader object that reads PLOT3D formatted
// files and generates structured grid(s) on output. PLOT3D is a computer
// graphics program designed to visualize the grids and solutions of
// computational fluid dynamics. Please see the "PLOT3D User's Manual"
// available from NASA Ames Research Center, Moffett Field CA.
//
// PLOT3D files consist of a grid file (also known as XYZ file), an 
// optional solution file (also known as a Q file), and an optional function 
// file that contains user created data (currently unsupported). The Q file 
// contains solution  information as follows: the four parameters free stream 
// mach number (Fsmach), angle of attack (Alpha), Reynolds number (Re), and 
// total integration time (Time). This information is stored in an array
// called Properties in the FieldData of each output (tuple 0: fsmach, tuple 1:
// alpha, tuple 2: re, tuple 3: time). In addition, the solution file contains 
// the flow density (scalar), flow momentum (vector), and flow energy (scalar).
//
// The reader can generate additional scalars and vectors (or "functions")
// from this information. To use vtkMultiBlockPLOT3DReader, you must specify the 
// particular function number for the scalar and vector you want to visualize.
// This implementation of the reader provides the following functions. The
// scalar functions are:
//    -1  - don't read or compute any scalars
//    100 - density
//    110 - pressure
//    120 - temperature
//    130 - enthalpy
//    140 - internal energy
//    144 - kinetic energy
//    153 - velocity magnitude
//    163 - stagnation energy
//    170 - entropy
//    184 - swirl.
//
// The vector functions are:
//    -1  - don't read or compute any vectors
//    200 - velocity
//    201 - vorticity
//    202 - momentum
//    210 - pressure gradient.
//
// (Other functions are described in the PLOT3D spec, but only those listed are
// implemented here.) Note that by default, this reader creates the density 
// scalar (100) and momentum vector (202) as output. (These are just read in
// from the solution file.) Please note that the validity of computation is
// a function of this class's gas constants (R, Gamma) and the equations used.
// They may not be suitable for your computational domain.
//
// Additionally, you can read other data and associate it as a vtkDataArray
// into the output's point attribute data. Use the method AddFunction()
// to list all the functions that you'd like to read. AddFunction() accepts
// an integer parameter that defines the function number.
//
// .SECTION See Also
// vtkStructuredGridSource vtkStructuredGrid

#ifndef __vtkMultiBlockPLOT3DReader_h
#define __vtkMultiBlockPLOT3DReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkUnsignedCharArray;
class vtkIntArray;
class vtkFloatArray;
class vtkStructuredGrid;
//BTX
struct vtkMultiBlockPLOT3DReaderInternals;
//ETX
class VTK_IO_EXPORT vtkMultiBlockPLOT3DReader : public vtkMultiBlockDataSetAlgorithm 
{
public:
  static vtkMultiBlockPLOT3DReader *New();
  vtkTypeMacro(vtkMultiBlockPLOT3DReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the PLOT3D geometry filename.
  void SetFileName(const char* name) { this->SetXYZFileName(name); }
  const char* GetFileName() { return this->GetXYZFileName(); }
  virtual void SetXYZFileName( const char* );
  vtkGetStringMacro(XYZFileName);

  // Description:
  // Set/Get the PLOT3D solution filename.
  vtkSetStringMacro(QFileName);
  vtkGetStringMacro(QFileName);

  // Description:
  // This returns the number of outputs this reader will produce.
  // This number is equal to the number of grids in the current 
  // file. This method has to be called before getting any output
  // if the number of outputs will be greater than 1 (the first
  // output is always the same). Note that every time this method
  // is invoked, the header file is opened and part of the header 
  // is read.
  int GetNumberOfBlocks();
  int GetNumberOfGrids() { return this->GetNumberOfBlocks(); }

  // Description:
  // Is the file to be read written in binary format (as opposed
  // to ascii).
  vtkSetMacro(BinaryFile, int);
  vtkGetMacro(BinaryFile, int);
  vtkBooleanMacro(BinaryFile, int);

  // Description:
  // Does the file to be read contain information about number of
  // grids. In some PLOT3D files, the first value contains the number 
  // of grids (even if there is only 1). If reading such a file,
  // set this to true.
  vtkSetMacro(MultiGrid, int);
  vtkGetMacro(MultiGrid, int);
  vtkBooleanMacro(MultiGrid, int);

  // Description:
  // Were the arrays written with leading and trailing byte counts ?
  // Usually, files written by a fortran program will contain these
  // byte counts whereas the ones written by C/C++ won't.
  vtkSetMacro(HasByteCount, int);
  vtkGetMacro(HasByteCount, int);
  vtkBooleanMacro(HasByteCount, int);

  // Description:
  // Is there iblanking (point visibility) information in the file.
  // If there is iblanking arrays, these will be read and assigned
  // to the PointVisibility array of the output.
  vtkSetMacro(IBlanking, int);
  vtkGetMacro(IBlanking, int);
  vtkBooleanMacro(IBlanking, int);

  // Description:
  // If only two-dimensional data was written to the file,
  // turn this on.
  vtkSetMacro(TwoDimensionalGeometry, int);
  vtkGetMacro(TwoDimensionalGeometry, int);
  vtkBooleanMacro(TwoDimensionalGeometry, int);

  // Description:
  // Try to read a binary file even if the file length seems to be
  // inconsistent with the header information. Use this with caution,
  // if the file length is not the same as calculated from the header.
  // either the file is corrupt or the settings are wrong. 
  vtkSetMacro(ForceRead, int);
  vtkGetMacro(ForceRead, int);
  vtkBooleanMacro(ForceRead, int);

  // Description:
  // Set the byte order of the file (remember, more Unix workstations
  // write big endian whereas PCs write little endian). Default is
  // big endian (since most older PLOT3D files were written by
  // workstations).
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  const char *GetByteOrderAsString();

  // Description:
  // Set/Get the gas constant. Default is 1.0.
  vtkSetMacro(R,double);
  vtkGetMacro(R,double);

  // Description:
  // Set/Get the ratio of specific heats. Default is 1.4.
  vtkSetMacro(Gamma,double);
  vtkGetMacro(Gamma,double);

  // Description:
  // Set/Get the x-component of the free-stream velocity. Default is 1.0.
  vtkSetMacro(Uvinf,double);
  vtkGetMacro(Uvinf,double);

  // Description:
  // Set/Get the y-component of the free-stream velocity. Default is 1.0.
  vtkSetMacro(Vvinf,double);
  vtkGetMacro(Vvinf,double);

  // Description:
  // Set/Get the z-component of the free-stream velocity. Default is 1.0.
  vtkSetMacro(Wvinf,double);
  vtkGetMacro(Wvinf,double);

  // Description:
  // Specify the scalar function to extract. If ==(-1), then no scalar 
  // function is extracted.
  void SetScalarFunctionNumber(int num);
  vtkGetMacro(ScalarFunctionNumber,int);

  // Description:
  // Specify the vector function to extract. If ==(-1), then no vector
  // function is extracted.
  void SetVectorFunctionNumber(int num);
  vtkGetMacro(VectorFunctionNumber,int);

  // Description:
  // Specify additional functions to read. These are placed into the
  // point data as data arrays. Later on they can be used by labeling
  // them as scalars, etc.
  void AddFunction(int functionNumber);
  void RemoveFunction(int);
  void RemoveAllFunctions();

  // Description:
  // Return 1 if the reader can read the given file name. Only meaningful
  // for binary files.
  virtual int CanReadBinaryFile(const char* fname);

//BTX
  enum 
  {
    FILE_BIG_ENDIAN=0,
    FILE_LITTLE_ENDIAN=1
  };
//ETX

protected:
  vtkMultiBlockPLOT3DReader();
  ~vtkMultiBlockPLOT3DReader();

  int CheckFile(FILE*& fp, const char* fname);
  int CheckGeometryFile(FILE*& xyzFp);
  int CheckSolutionFile(FILE*& qFp);

  void SkipByteCount (FILE* fp);
  int ReadIntBlock  (FILE* fp, int n, int*   block);
  int ReadFloatBlock(FILE* fp, int n, float* block);

  int GetNumberOfBlocksInternal(FILE* xyzFp, int verify=1);

  int ReadGeometryHeader(FILE* fp);
  int ReadQHeader(FILE* fp);

  void CalculateFileSize(FILE* fp);
  long EstimateSize(int ni, int nj, int nk);

  void AssignAttribute(int fNumber, vtkStructuredGrid* output,
                       int attributeType);
  void MapFunction(int fNumber, vtkStructuredGrid* output);
  void ComputeTemperature(vtkStructuredGrid* output);
  void ComputePressure(vtkStructuredGrid* output);
  void ComputeEnthalpy(vtkStructuredGrid* output);
  void ComputeKineticEnergy(vtkStructuredGrid* output);
  void ComputeVelocityMagnitude(vtkStructuredGrid* output);
  void ComputeEntropy(vtkStructuredGrid* output);
  void ComputeSwirl(vtkStructuredGrid* output);
  void ComputeVelocity(vtkStructuredGrid* output);
  void ComputeVorticity(vtkStructuredGrid* output);
  void ComputePressureGradient(vtkStructuredGrid* output);

  // Delete references to any existing vtkPoints and
  // I-blank arrays. The next Update() will (re)read
  // the XYZ file.
  void ClearGeometryCache();

  //plot3d FileNames
  char *XYZFileName;
  char *QFileName;

  int BinaryFile;
  int HasByteCount;
  int TwoDimensionalGeometry;
  int MultiGrid;
  int ForceRead;
  int ByteOrder;
  int IBlanking;

  long FileSize;

  //parameters used in computing derived functions
  double R; 
  double Gamma;
  double Uvinf;
  double Vvinf;
  double Wvinf;

  //functions to read that are not scalars or vectors
  vtkIntArray *FunctionList;

  int ScalarFunctionNumber;
  int VectorFunctionNumber;

  // Cache of geometry
  vtkFloatArray** PointCache;
  vtkUnsignedCharArray** IBlankCache;

  // First pass at automatically detecting configuration
  int GenerateDefaultConfiguration();
  int VerifySettings(char* buf, int bufSize);
  
  void ReadIntBlockV(char** buf, int n, int* block);
  void SkipByteCountV(char** buf);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);
  
private:

  vtkMultiBlockPLOT3DReaderInternals* Internal;

  vtkMultiBlockPLOT3DReader(const vtkMultiBlockPLOT3DReader&);  // Not implemented.
  void operator=(const vtkMultiBlockPLOT3DReader&);  // Not implemented.
};

#endif


