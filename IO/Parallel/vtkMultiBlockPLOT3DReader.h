// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiBlockPLOT3DReader
 * @brief   read PLOT3D data files
 *
 * vtkMultiBlockPLOT3DReader is a reader object that reads PLOT3D formatted
 * files and generates structured grid(s) on output. PLOT3D is a computer
 * graphics program designed to visualize the grids and solutions of
 * computational fluid dynamics. This reader also supports the variant
 * of the PLOT3D format used by NASA's OVERFLOW CFD software, including
 * full support for all Q variables. Please see the "PLOT3D User's Manual"
 * available from NASA Ames Research Center, Moffett Field CA.
 *
 * PLOT3D files consist of a grid file (also known as XYZ file), an
 * optional solution file (also known as a Q file), and an optional function
 * file that contains user created data (currently unsupported). The Q file
 * contains solution information as follows: the four parameters free stream
 * mach number (Fsmach), angle of attack (Alpha), Reynolds number (Re), and
 * total integration time (Time). This information is stored in an array
 * called Properties in the FieldData of each output (tuple 0: fsmach, tuple 1:
 * alpha, tuple 2: re, tuple 3: time). In addition, the solution file contains
 * the flow density (scalar), flow momentum (vector), and flow energy (scalar).
 *
 * This reader supports a limited form of time series data which are stored
 * as a series of Q files. Using the AddFileName() method provided by the
 * superclass, one can define a file series. For other cases, for example where
 * the XYZ or function files vary over time, use vtkPlot3DMetaReader.
 *
 * The reader can generate additional scalars and vectors (or "functions")
 * from this information. To use vtkMultiBlockPLOT3DReader, you must specify the
 * particular function number for the scalar and vector you want to visualize.
 * This implementation of the reader provides the following functions. The
 * scalar functions are:
 *    -1  - don't read or compute any scalars
 *    100 - density
 *    110 - pressure
 *    111 - pressure coefficient (requires Overflow file with Gamma)
 *    112 - mach number (requires Overflow file with Gamma)
 *    113 - sounds speed (requires Overflow file with Gamma)
 *    120 - temperature
 *    130 - enthalpy
 *    140 - internal energy
 *    144 - kinetic energy
 *    153 - velocity magnitude
 *    163 - stagnation energy
 *    170 - entropy
 *    184 - swirl
 *    211 - vorticity magnitude
 *
 * The vector functions are:
 *    -1  - don't read or compute any vectors
 *    200 - velocity
 *    201 - vorticity
 *    202 - momentum
 *    210 - pressure gradient.
 *    212 - strain rate
 *
 * (Other functions are described in the PLOT3D spec, but only those listed are
 * implemented here.) Note that by default, this reader creates the density
 * scalar (100), stagnation energy (163) and momentum vector (202) as output.
 * (These are just read in from the solution file.) Please note that the validity
 * of computation is a function of this class's gas constants (R, Gamma) and the
 * equations used. They may not be suitable for your computational domain.
 *
 * Additionally, you can read other data and associate it as a vtkDataArray
 * into the output's point attribute data. Use the method AddFunction()
 * to list all the functions that you'd like to read. AddFunction() accepts
 * an integer parameter that defines the function number.
 *
 * @sa
 * vtkMultiBlockDataSet vtkStructuredGrid vtkPlot3DMetaReader
 */

#ifndef vtkMultiBlockPLOT3DReader_h
#define vtkMultiBlockPLOT3DReader_h

#include "vtkIOParallelModule.h" // For export macro
#include "vtkParallelReader.h"
#include <vector> // For holding function-names

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSetAttributes;
class vtkIntArray;
class vtkMultiBlockPLOT3DReaderRecord;
class vtkMultiProcessController;
class vtkStructuredGrid;
class vtkUnsignedCharArray;
struct vtkMultiBlockPLOT3DReaderInternals;
class vtkMultiBlockDataSet;
VTK_ABI_NAMESPACE_END

namespace Functors
{
VTK_ABI_NAMESPACE_BEGIN
class ComputeFunctor;
class ComputeTemperatureFunctor;
class ComputePressureFunctor;
class ComputePressureCoefficientFunctor;
class ComputeMachNumberFunctor;
class ComputeSoundSpeedFunctor;
class ComputeEnthalpyFunctor;
class ComputeKinecticEnergyFunctor;
class ComputeVelocityMagnitudeFunctor;
class ComputeEntropyFunctor;
class ComputeSwirlFunctor;
class ComputeVelocityFunctor;
class ComputeVorticityMagnitudeFunctor;
class ComputePressureGradientFunctor;
class ComputeVorticityFunctor;
class ComputeStrainRateFunctor;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

class VTKIOPARALLEL_EXPORT vtkMultiBlockPLOT3DReader : public vtkParallelReader
{
  friend class Functors::ComputeFunctor;
  friend class Functors::ComputeTemperatureFunctor;
  friend class Functors::ComputePressureFunctor;
  friend class Functors::ComputePressureCoefficientFunctor;
  friend class Functors::ComputeMachNumberFunctor;
  friend class Functors::ComputeSoundSpeedFunctor;
  friend class Functors::ComputeEnthalpyFunctor;
  friend class Functors::ComputeKinecticEnergyFunctor;
  friend class Functors::ComputeVelocityMagnitudeFunctor;
  friend class Functors::ComputeEntropyFunctor;
  friend class Functors::ComputeSwirlFunctor;
  friend class Functors::ComputeVelocityFunctor;
  friend class Functors::ComputeVorticityMagnitudeFunctor;
  friend class Functors::ComputePressureGradientFunctor;
  friend class Functors::ComputeVorticityFunctor;
  friend class Functors::ComputeStrainRateFunctor;

public:
  static vtkMultiBlockPLOT3DReader* New();
  vtkTypeMacro(vtkMultiBlockPLOT3DReader, vtkParallelReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkMultiBlockDataSet* GetOutput();
  vtkMultiBlockDataSet* GetOutput(int);
  ///@}

  ///@{
  /**
   * Set/Get the PLOT3D geometry filename.
   */
  void SetFileName(VTK_FILEPATH const char* name) { this->SetXYZFileName(name); }
  VTK_FILEPATH const char* GetFileName() { return this->GetXYZFileName(); }
  VTK_FILEPATH const char* GetFileName(int i) { return this->vtkParallelReader::GetFileName(i); }
  virtual void SetXYZFileName(VTK_FILEPATH const char*);
  vtkGetFilePathMacro(XYZFileName);
  ///@}

  ///@{
  /**
   * Set/Get the PLOT3D solution filename. This adds a filename
   * using the superclass' AddFileName() method. To read a series
   * of q files, use the AddFileName() interface directly to add
   * multiple q filenames in the appropriate order. If the files
   * are of Overflow format, the reader will read the time values
   * from the files. Otherwise, it will use an integer sequence.
   * Use a meta reader to support time values for non-Overflow file
   * sequences.
   */
  void SetQFileName(VTK_FILEPATH const char* name);
  VTK_FILEPATH const char* GetQFileName();
  ///@}

  ///@{
  /**
   * Set/Get the PLOT3D function filename.
   */
  vtkSetFilePathMacro(FunctionFileName);
  vtkGetFilePathMacro(FunctionFileName);
  ///@}

  ///@{
  /**
   * When this option is turned on, the reader will try to figure
   * out the values of various options such as byte order, byte
   * count etc. automatically. This options works only for binary
   * files. When it is turned on, the reader should be able to read
   * most Plot3D files automatically. The default is OFF for backwards
   * compatibility reasons. For binary files, it is strongly recommended
   * that you turn on AutoDetectFormat and leave the other file format
   * related options untouched.
   */
  vtkSetMacro(AutoDetectFormat, vtkTypeBool);
  vtkGetMacro(AutoDetectFormat, vtkTypeBool);
  vtkBooleanMacro(AutoDetectFormat, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Is the file to be read written in binary format (as opposed
   * to ascii).
   */
  vtkSetMacro(BinaryFile, vtkTypeBool);
  vtkGetMacro(BinaryFile, vtkTypeBool);
  vtkBooleanMacro(BinaryFile, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Does the file to be read contain information about number of
   * grids. In some PLOT3D files, the first value contains the number
   * of grids (even if there is only 1). If reading such a file,
   * set this to true.
   */
  vtkSetMacro(MultiGrid, vtkTypeBool);
  vtkGetMacro(MultiGrid, vtkTypeBool);
  vtkBooleanMacro(MultiGrid, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Were the arrays written with leading and trailing byte counts ?
   * Usually, files written by a fortran program will contain these
   * byte counts whereas the ones written by C/C++ won't.
   */
  vtkSetMacro(HasByteCount, vtkTypeBool);
  vtkGetMacro(HasByteCount, vtkTypeBool);
  vtkBooleanMacro(HasByteCount, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Is there iblanking (point visibility) information in the file.
   * If there is iblanking arrays, these will be read and assigned
   * to the PointVisibility array of the output.
   */
  vtkSetMacro(IBlanking, vtkTypeBool);
  vtkGetMacro(IBlanking, vtkTypeBool);
  vtkBooleanMacro(IBlanking, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If only two-dimensional data was written to the file,
   * turn this on.
   */
  vtkSetMacro(TwoDimensionalGeometry, vtkTypeBool);
  vtkGetMacro(TwoDimensionalGeometry, vtkTypeBool);
  vtkBooleanMacro(TwoDimensionalGeometry, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Is this file in double precision or single precision.
   * This only matters for binary files.
   * Default is single.
   */
  vtkSetMacro(DoublePrecision, vtkTypeBool);
  vtkGetMacro(DoublePrecision, vtkTypeBool);
  vtkBooleanMacro(DoublePrecision, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Try to read a binary file even if the file length seems to be
   * inconsistent with the header information. Use this with caution,
   * if the file length is not the same as calculated from the header.
   * either the file is corrupt or the settings are wrong.
   */
  vtkSetMacro(ForceRead, vtkTypeBool);
  vtkGetMacro(ForceRead, vtkTypeBool);
  vtkBooleanMacro(ForceRead, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the byte order of the file (remember, more Unix workstations
   * write big endian whereas PCs write little endian). Default is
   * big endian (since most older PLOT3D files were written by
   * workstations).
   */
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);
  const char* GetByteOrderAsString();
  ///@}

  ///@{
  /**
   * Set/Get the gas constant. Default is 1.0.
   */
  vtkSetMacro(R, double);
  vtkGetMacro(R, double);
  ///@}

  ///@{
  /**
   * Set/Get the ratio of specific heats. Default is 1.4.
   */
  vtkSetMacro(Gamma, double);
  vtkGetMacro(Gamma, double);
  ///@}

  ///@{
  /**
   * When set to true (default), the reader will preserve intermediate computed
   * quantities that were not explicitly requested e.g. if `VelocityMagnitude` is
   * enabled, but not `Velocity`, the reader still needs to compute `Velocity`.
   * If `PreserveIntermediateFunctions` if false, then the output will not have
   * `Velocity` array, only the requested `VelocityMagnitude`. This is useful to
   * avoid using up memory for arrays that are not relevant for the analysis.
   */
  vtkSetMacro(PreserveIntermediateFunctions, bool);
  vtkGetMacro(PreserveIntermediateFunctions, bool);
  vtkBooleanMacro(PreserveIntermediateFunctions, bool);

  ///@{
  /**
   * Specify the scalar function to extract. If ==(-1), then no scalar
   * function is extracted.
   */
  void SetScalarFunctionNumber(int num);
  vtkGetMacro(ScalarFunctionNumber, int);
  ///@}

  ///@{
  /**
   * Specify the vector function to extract. If ==(-1), then no vector
   * function is extracted.
   */
  void SetVectorFunctionNumber(int num);
  vtkGetMacro(VectorFunctionNumber, int);
  ///@}

  ///@{
  /**
   * Specify additional functions to read. These are placed into the
   * point data as data arrays. Later on they can be used by labeling
   * them as scalars, etc.
   */
  void AddFunction(int functionNumber);
  void RemoveFunction(int);
  void RemoveAllFunctions();
  ///@}

  /**
   * Return 1 if the reader can read the given file name. Only meaningful
   * for binary files.
   */
  virtual int CanReadBinaryFile(VTK_FILEPATH const char* fname);

  ///@{
  /**
   * Set/Get the communicator object (we'll use global World controller
   * if you don't set a different one).
   */
  void SetController(vtkMultiProcessController* c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  void AddFunctionName(const std::string& name) { FunctionNames.push_back(name); }

  enum
  {
    FILE_BIG_ENDIAN = 0,
    FILE_LITTLE_ENDIAN = 1
  };

  ///@{
  /**
   * These methods have to be overwritten from superclass
   * because Plot3D actually uses the XYZ file to read these.
   * This is not recognized by the superclass which returns
   * an error when a filename (Q filename) is not set.
   */
  int ReadMetaData(vtkInformation* metadata) override;
  int ReadMesh(int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) override;
  int ReadPoints(int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) override;
  int ReadArrays(int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) override;
  ///@}

protected:
  vtkMultiBlockPLOT3DReader();
  ~vtkMultiBlockPLOT3DReader() override;

  ///@{
  /**
   * Overridden from superclass to do actual reading.
   */
  double GetTimeValue(const std::string& fname) override;
  int ReadMesh(
    const std::string& fname, int piece, int npieces, int nghosts, vtkDataObject* output) override;
  int ReadPoints(
    const std::string& fname, int piece, int npieces, int nghosts, vtkDataObject* output) override;
  int ReadArrays(
    const std::string& fname, int piece, int npieces, int nghosts, vtkDataObject* output) override;
  ///@}

  vtkDataArray* CreateFloatArray();

  int CheckFile(FILE*& fp, const char* fname);
  int CheckGeometryFile(FILE*& xyzFp);
  int CheckFunctionFile(FILE*& fFp);

  int GetByteCountSize();
  int SkipByteCount(FILE* fp);
  int ReadIntBlock(FILE* fp, int n, int* block);

  vtkIdType ReadValues(FILE* fp, int n, vtkDataArray* scalar);
  virtual int ReadIntScalar(void* vfp, int extent[6], int wextent[6], vtkDataArray* scalar,
    vtkTypeUInt64 offset, const vtkMultiBlockPLOT3DReaderRecord& currentRecord);
  virtual int ReadScalar(void* vfp, int extent[6], int wextent[6], vtkDataArray* scalar,
    vtkTypeUInt64 offset, const vtkMultiBlockPLOT3DReaderRecord& currentRecord);
  virtual int ReadVector(void* vfp, int extent[6], int wextent[6], int numDims,
    vtkDataArray* vector, vtkTypeUInt64 offset,
    const vtkMultiBlockPLOT3DReaderRecord& currentRecord);
  virtual int OpenFileForDataRead(void*& fp, const char* fname);
  virtual void CloseFile(void* fp);

  int GetNumberOfBlocksInternal(FILE* xyzFp, int allocate);

  int ReadGeometryHeader(FILE* fp);
  int ReadQHeader(FILE* fp, bool checkGrid, int& nq, int& nqc, int& overflow);
  int ReadFunctionHeader(FILE* fp, int* nFunctions);

  void CalculateFileSize(FILE* fp);

  int AutoDetectionCheck(FILE* fp);

  void AssignAttribute(int fNumber, vtkStructuredGrid* output, int attributeType);
  void MapFunction(int fNumber, vtkStructuredGrid* output);

  ///@{
  /**
   * Each of these methods compute a derived quantity. On success, the array is
   * added to the output and a pointer to the same is returned.
   */
  vtkDataArray* ComputeTemperature(vtkStructuredGrid* output);
  vtkDataArray* ComputePressure(vtkStructuredGrid* output);
  vtkDataArray* ComputeEnthalpy(vtkStructuredGrid* output);
  vtkDataArray* ComputeKineticEnergy(vtkStructuredGrid* output);
  vtkDataArray* ComputeVelocityMagnitude(vtkStructuredGrid* output);
  vtkDataArray* ComputeEntropy(vtkStructuredGrid* output);
  vtkDataArray* ComputeSwirl(vtkStructuredGrid* output);
  vtkDataArray* ComputeVelocity(vtkStructuredGrid* output);
  vtkDataArray* ComputeVorticity(vtkStructuredGrid* output);
  vtkDataArray* ComputePressureGradient(vtkStructuredGrid* output);
  vtkDataArray* ComputePressureCoefficient(vtkStructuredGrid* output);
  vtkDataArray* ComputeMachNumber(vtkStructuredGrid* output);
  vtkDataArray* ComputeSoundSpeed(vtkStructuredGrid* output);
  vtkDataArray* ComputeVorticityMagnitude(vtkStructuredGrid* output);
  vtkDataArray* ComputeStrainRate(vtkStructuredGrid* output);
  ///@}

  // Returns a vtkFloatArray or a vtkDoubleArray depending
  // on DoublePrecision setting
  vtkDataArray* NewFloatArray();

  // Delete references to any existing vtkPoints and
  // I-blank arrays. The next Update() will (re)read
  // the XYZ file.
  void ClearGeometryCache();

  double GetGamma(vtkIdType idx, vtkDataArray* gamma);

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  // plot3d FileNames
  char* XYZFileName;
  char* QFileName;
  char* FunctionFileName;
  vtkTypeBool BinaryFile;
  vtkTypeBool HasByteCount;
  vtkTypeBool TwoDimensionalGeometry;
  vtkTypeBool MultiGrid;
  vtkTypeBool ForceRead;
  int ByteOrder;
  vtkTypeBool IBlanking;
  vtkTypeBool DoublePrecision;
  vtkTypeBool AutoDetectFormat;

  int ExecutedGhostLevels;

  size_t FileSize;

  // parameters used in computing derived functions
  double R;
  double Gamma;
  double GammaInf;

  bool PreserveIntermediateFunctions;

  // named functions from meta data
  std::vector<std::string> FunctionNames;

  // functions to read that are not scalars or vectors
  vtkIntArray* FunctionList;

  int ScalarFunctionNumber;
  int VectorFunctionNumber;

  vtkMultiBlockPLOT3DReaderInternals* Internal;

  vtkMultiProcessController* Controller;

private:
  vtkMultiBlockPLOT3DReader(const vtkMultiBlockPLOT3DReader&) = delete;
  void operator=(const vtkMultiBlockPLOT3DReader&) = delete;

  // Key used to flag intermediate results.
  static vtkInformationIntegerKey* INTERMEDIATE_RESULT();

  /**
   * Remove intermediate results
   */
  void RemoveIntermediateFunctions(vtkDataSetAttributes* dsa);
};

VTK_ABI_NAMESPACE_END
#endif
