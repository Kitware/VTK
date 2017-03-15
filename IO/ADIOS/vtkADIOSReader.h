/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkADIOSReader
 * @brief   Read ADIOS files.
 *
 * vtkADIOSReader is the base class for all ADIOS readers
*/

#ifndef vtkADIOSReader_h
#define vtkADIOSReader_h

#include <map>    // For independently time stepped array indexing
#include <queue>  // For post read operations
#include <string> // For variable name index mapping
#include <vector> // For independently time stepped array indexing

#include "vtkDataObjectAlgorithm.h"
#include "vtkMultiProcessController.h" // For the MPI controller member
#include "vtkSetGet.h"                 // For property get/set macros
#include "vtkSmartPointer.h"           // For the object cache

#include "ADIOSDefs.h"                 // For enum definitions

#include "vtkIOADIOSModule.h"          // For export macro

namespace ADIOS
{
class VarInfo;
class Reader;
}
class vtkADIOSDirTree;
class BaseFunctor;

class vtkCellArray;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkImageData;
class vtkPolyData;
class vtkUnstructuredGrid;

//----------------------------------------------------------------------------

class VTKIOADIOS_EXPORT vtkADIOSReader : public vtkDataObjectAlgorithm
{
public:
  static vtkADIOSReader* New(void);
  vtkTypeMacro(vtkADIOSReader,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Test wether or not a given file should even be attempted for use with this
   * reader.
   */
  int CanReadFile(const char* name);

  //@{
  /**
   * Get/Set the inut filename
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the ADIOS read method
   */
  vtkGetMacro(ReadMethod, int);
  vtkSetClampMacro(ReadMethod, int,
                   static_cast<int>(ADIOS::ReadMethod_BP),
                   static_cast<int>(ADIOS::ReadMethod_FlexPath));
  void SetReadMethodBP()          { this->SetReadMethod(static_cast<int>(ADIOS::ReadMethod_BP)); }
  void SetReadMethodBPAggregate() { this->SetReadMethod(static_cast<int>(ADIOS::ReadMethod_BP_AGGREGATE)); }
  void SetReadMethodDataSpaces()  { this->SetReadMethod(static_cast<int>(ADIOS::ReadMethod_DataSpaces)); }
  void SetReadMethodDIMES()       { this->SetReadMethod(static_cast<int>(ADIOS::ReadMethod_DIMES)); }
  void SetReadMethodFlexPath()    { this->SetReadMethod(static_cast<int>(ADIOS::ReadMethod_FlexPath)); }
  //@}


  //@{
  /**
   * Get/Set arguments to the ADIOS read method.
   */
  vtkSetStringMacro(ReadMethodArguments);
  vtkGetStringMacro(ReadMethodArguments);
  //@}

  //@{
  /**
   * Set the MPI controller.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * The main interface which triggers the reader to start
   */
  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*);

protected:

  /**
   * Open an ADIOS file and build the directory structure
   */
  bool OpenAndReadMetadata(void);

  /**
   * Wait for all scheduled array reads to finish
   */
  void WaitForReads(void);

  /**
   * Create a VTK object with it's scalar values and allocate any arrays, and
   * schedule them for reading
   */
  template<typename T>
  T* ReadObject(const std::string& path, int blockId);

  //@{
  /**
   * Initialize a pre-allocated object with it's appropriate scalars.  These
   * methods do not perform any validation and assume that the provides ADIOS
   * structures and vtk objects are properly formed.  Arrays will be scheduled
   * for reading afterwards
   */
  void ReadObject(const ADIOS::VarInfo* info, const vtkADIOSDirTree *subDir,
    vtkDataArray* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkCellArray* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkFieldData* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkDataSetAttributes* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkDataSet* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkImageData* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkPolyData* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkUnstructuredGrid* data, int blockId);
  //@}

  char *FileName;
  int ReadMethod;
  char *ReadMethodArguments;
  vtkADIOSDirTree *Tree;
  ADIOS::Reader *Reader;
  vtkMultiProcessController *Controller;

  // Index information for independently stepped variables

  // Map variable names to their position in the block step index
  // [BlockId][VarName] = IndexId
  std::vector<std::map<std::string, size_t> > BlockStepIndexIdMap;

  // [BlockId][GlobalStep][IndexId] = LocalStep
  // Ex: The file has 30 steps, but the Variable "/Foo/Bar" in block 3 only
  // has 2 steps, written out at global step 10 and global step 17.  To
  // lookup the local step for the variable at global time step 25:
  //
  // size_t idx = this->BlockStepIndexIdMap[3]["/Foo/Bar"];
  // int localStep = this->BlockStepIndex[3][25][idx];
  //
  // At this point, localStep = 2, since at global step 25, local step 2 is the
  // most recent version of "/Foo/Bar" available
  std::vector<std::vector<std::vector<int> > > BlockStepIndex;

  // Cache the VTK objects as they are read
  // Key = <BlockId, IndexId>, Value = <LocalStep, Object>
  std::map<std::pair<int, size_t>,
           std::pair<int, vtkSmartPointer<vtkObject> > >
    ObjectCache;

  vtkADIOSReader();
  virtual ~vtkADIOSReader();

  /* The design of ADIOS is such that array IO is not directly performed
   * upon request, but instead is scheduled to be performed later, at which
   * time all IO operations are processed at once in bulk.  This creates
   * an odd situation for data management since arrays will be allocated with
   * junk data and scheduled to be filled, but they cannot be safely assigned
   * to a VTK object until the data contained in them is valid, e.g. through
   * a call to vtkUnstructuredGrid::SetPoints or similar.  Similary,
   * they cannot have their reference cound safely decremented until after
   * they have been assigned to a vtk object.  To work around this, a generic
   * action queue is created to hold a list of arbitrary functions that need
   * to be called in a particular order after the reads have been
   * processed.  The AddPostReadOperation prototypes use a large number of
   * template parameters in order to permit the compiler to automatically
   * perform the correct type deduction necessary to translate between
   * member function signatures and the objects and arguments they get called
   * with.  This allows for arbitrary functions with arbitrary return types
   * and arbitrary argument types to be collected into a single event queue
   */

  // A set of operations to perform after reading is complete
  std::queue<BaseFunctor*> PostReadOperations;

  // A set of shortcuts to allow automatic parameter deduction

  template<typename TObjectFun, typename TObjectData, typename TReturn>
  void AddPostReadOperation(TObjectData*, TReturn (TObjectFun::*)());

  template<typename TObjectFun, typename TObjectData, typename TReturn,
    typename TArg1Fun, typename TArg1Data>
  void AddPostReadOperation(TObjectData*,
    TReturn (TObjectFun::*)(TArg1Fun), TArg1Data);

  template<typename TObjectFun, typename TObjectData, typename TReturn,
    typename TArg1Fun, typename TArg1Data,
    typename TArg2Fun, typename TArg2Data>
  void AddPostReadOperation(TObjectData*,
    TReturn (TObjectFun::*)(TArg1Fun, TArg2Fun),
    TArg1Data, TArg2Data);

  template<typename TObjectFun, typename TObjectData, typename TReturn,
    typename TArg1Fun, typename TArg1Data,
    typename TArg2Fun, typename TArg2Data,
    typename TArg3Fun, typename TArg3Data>
  void AddPostReadOperation(TObjectData*,
    TReturn (TObjectFun::*)(TArg1Fun, TArg2Fun, TArg3Fun),
    TArg1Data, TArg2Data, TArg3Data);

  // Used to implement vtkAlgorithm

  int FillOutputPortInformation(int, vtkInformation*);

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **input,
                                 vtkInformationVector *output);
  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **input,
                                  vtkInformationVector *output);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **input,
                          vtkInformationVector *output);

  int NumberOfPieces;
  std::vector<double> TimeSteps;
  std::map<double, size_t> TimeStepsIndex;

  double RequestStep;
  int RequestStepIndex;
  int RequestNumberOfPieces;
  int RequestPiece;

private:
  vtkADIOSReader(const vtkADIOSReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkADIOSReader&) VTK_DELETE_FUNCTION;
};

#define DECLARE_EXPLICIT(T) \
template<> T* vtkADIOSReader::ReadObject<T>(const std::string& path, \
  int blockId);
DECLARE_EXPLICIT(vtkImageData)
DECLARE_EXPLICIT(vtkPolyData)
DECLARE_EXPLICIT(vtkUnstructuredGrid)
#undef DECLARE_EXPLICIT

#endif
