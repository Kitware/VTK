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
// computational fluid dynamics. This reader also supports the variant
// of the PLOT3D format used by NASA's OVERFLOW CFD software, including
// full support for all Q variables. Please see the "PLOT3D User's Manual"
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
// Note that this reader does not support time series data which is usually
// stored as a series of Q and optionally XYZ files. If you want to read such
// a file series, use vtkPlot3DMetaReader.
//
// The reader can generate additional scalars and vectors (or "functions")
// from this information. To use vtkMultiBlockPLOT3DReader, you must specify the
// particular function number for the scalar and vector you want to visualize.
// This implementation of the reader provides the following functions. The
// scalar functions are:
//    -1  - don't read or compute any scalars
//    100 - density
//    110 - pressure
//    111 - pressure coefficient (requires Overflow file with Gamma)
//    112 - mach number (requires Overflow file with Gamma)
//    113 - sounds speed (requires Overflow file with Gamma)
//    120 - temperature
//    130 - enthalpy
//    140 - internal energy
//    144 - kinetic energy
//    153 - velocity magnitude
//    163 - stagnation energy
//    170 - entropy
//    184 - swirl
//    211 - vorticity magnitude
//
// The vector functions are:
//    -1  - don't read or compute any vectors
//    200 - velocity
//    201 - vorticity
//    202 - momentum
//    210 - pressure gradient.
//    212 - strain rate
//
// (Other functions are described in the PLOT3D spec, but only those listed are
// implemented here.) Note that by default, this reader creates the density
// scalar (100), stagnation energy (163) and momentum vector (202) as output.
// (These are just read in from the solution file.) Please note that the validity
// of computation is a function of this class's gas constants (R, Gamma) and the
// equations used. They may not be suitable for your computational domain.
//
// Additionally, you can read other data and associate it as a vtkDataArray
// into the output's point attribute data. Use the method AddFunction()
// to list all the functions that you'd like to read. AddFunction() accepts
// an integer parameter that defines the function number.
//
// .SECTION See Also
// vtkMultiBlockDataSet vtkStructuredGrid vtkPlot3DMetaReader

#ifndef vtkMultiBlockPLOT3DReader_h
#define vtkMultiBlockPLOT3DReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkDataArray;
class vtkUnsignedCharArray;
class vtkIntArray;
class vtkStructuredGrid;
//BTX
struct vtkMultiBlockPLOT3DReaderInternals;
//ETX
class VTKIOGEOMETRY_EXPORT vtkMultiBlockPLOT3DReader : public vtkMultiBlockDataSetAlgorithm
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
  // Set/Get the PLOT3D function filename.
  vtkSetStringMacro(FunctionFileName);
  vtkGetStringMacro(FunctionFileName);

  // Description:
  // When this option is turned on, the reader will try to figure
  // out the values of various options such as byte order, byte
  // count etc. automatically. This options works only for binary
  // files. When it is turned on, the reader should be able to read
  // most Plot3D files automatically. The default is OFF for backwards
  // compatibility reasons. For binary files, it is strongly recommended
  // that you turn on AutoDetectFormat and leave the other file format
  // related options untouched.
  vtkSetMacro(AutoDetectFormat, int);
  vtkGetMacro(AutoDetectFormat, int);
  vtkBooleanMacro(AutoDetectFormat, int);

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
  // Is this file in double precision or single precision.
  // This only matters for binary files.
  // Default is single.
  vtkSetMacro(DoublePrecision, int);
  vtkGetMacro(DoublePrecision, int);
  vtkBooleanMacro(DoublePrecision, int);

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

  // Description:
  // Overwritten to make sure that RequestInformation reads the meta-data
  // again after the reader parameters were changed.
  virtual void Modified();

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

  vtkDataArray* CreateFloatArray();

  int CheckFile(FILE*& fp, const char* fname);
  int CheckGeometryFile(FILE*& xyzFp);
  int CheckSolutionFile(FILE*& qFp);
  int CheckFunctionFile(FILE*& fFp);

  int SkipByteCount (FILE* fp);
  int ReadIntBlock  (FILE* fp, int n, int*   block);

  int ReadScalar(FILE* fp, int n, vtkDataArray* scalar);
  int ReadVector(FILE* fp, int n, int numDims, vtkDataArray* vector);

  int GetNumberOfBlocksInternal(FILE* xyzFp, int allocate);

  int ReadGeometryHeader(FILE* fp);
  int ReadQHeader(FILE* fp, bool checkGrid, int& nq, int& nqc, int& overflow);
  int ReadFunctionHeader(FILE* fp, int* nFunctions);

  void CalculateFileSize(FILE* fp);
  long EstimateSize(int ni, int nj, int nk);

  int AutoDetectionCheck(FILE* fp);


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
  void ComputePressureCoefficient(vtkStructuredGrid* output);
  void ComputeMachNumber(vtkStructuredGrid* output);
  void ComputeSoundSpeed(vtkStructuredGrid* output);
  void ComputeVorticityMagnitude(vtkStructuredGrid* output);
  void ComputeStrainRate(vtkStructuredGrid* output);

  // Returns a vtkFloatArray or a vtkDoubleArray depending
  // on DoublePrecision setting
  vtkDataArray* NewFloatArray();

  // Delete references to any existing vtkPoints and
  // I-blank arrays. The next Update() will (re)read
  // the XYZ file.
  void ClearGeometryCache();

  //plot3d FileNames
  char *XYZFileName;
  char *QFileName;
  char *FunctionFileName;

  int BinaryFile;
  int HasByteCount;
  int TwoDimensionalGeometry;
  int MultiGrid;
  int ForceRead;
  int ByteOrder;
  int IBlanking;
  int DoublePrecision;
  int AutoDetectFormat;

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

  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*);

  vtkMultiBlockPLOT3DReaderInternals* Internal;

private:
  vtkMultiBlockPLOT3DReader(const vtkMultiBlockPLOT3DReader&);  // Not implemented.
  void operator=(const vtkMultiBlockPLOT3DReader&);  // Not implemented.
};

#endif


