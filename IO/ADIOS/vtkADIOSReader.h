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
// .NAME vtkADIOSReader - Read ADIOS files.
// .SECTION Description
// vtkADIOSReader is the base class for all ADIOS writers

#ifndef __vtkADIOSReader_h
#define __vtkADIOSReader_h

#include <string> // For variable name index mapping
#include <vector> // For independently time stepped array indexing
#include <map>    // For independently time stepped array indexing
#include <queue>  // For post read operations

#include <vtkAlgorithm.h>
#include <vtkMultiProcessController.h> // For the MPI controller member
#include <vtkSetGet.h>                 // For property get/set macros
#include <vtkSmartPointer.h>           // For the object cache
#include <vtkIOADIOSModule.h>          // For export macro

class ADIOSVarInfo;
class ADIOSReader;
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

class VTKIOADIOS_EXPORT vtkADIOSReader : public vtkAlgorithm
{
public:
  static vtkADIOSReader* New(void);
  vtkTypeMacro(vtkADIOSReader,vtkAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the inut filename
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  enum
  {
    BP          = 0,
    BPAggregate = 1,
    DataSpaces  = 3,
    DIMES       = 4,
    FlexPath    = 5
  };

  // Description:
  // Get/Set the ADIOS read method
  vtkSetClampMacro(ReadMethod, int, BP, FlexPath);
  vtkGetMacro(ReadMethod, int);
  void SetReadMethodBP()          { this->SetReadMethod(BP); }
  void SetReadMethodBPAggregate() { this->SetReadMethod(BPAggregate); }
  void SetReadMethodDataSpaces()  { this->SetReadMethod(DataSpaces); }
  void SetReadMethodDIMES()       { this->SetReadMethod(DIMES); }
  void SetReadMethodFlexPath()    { this->SetReadMethod(FlexPath); }


  // Description:
  // Get/Set arguments to the ADIOS read method.
  vtkSetStringMacro(ReadMethodArguments);
  vtkGetStringMacro(ReadMethodArguments);

  // Description:
  // Set the MPI controller.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // The main interface which triggers the reader to start
  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*);

protected:

  // Description:
  // Open an ADIOS file and build the directory structure
  bool OpenAndReadMetadata(void);

  // Description:
  // Wait for all scheduled array reads to finish
  void WaitForReads(void);

  // Description:
  // Create a VTK object with it's scalar values and allocate any arrays, and
  // schedule them for reading
  template<typename T>
  T* ReadObject(const std::string& path, int blockId);

  // Description:
  // Initialize a pre-allocated object with it's appropriate scalars.  These
  // methods do not perform any validation and assume that the provides ADIOS
  // structures and vtk objects are properly formed.  Arrays will be scheduled
  // for reading afterwards
  void ReadObject(const ADIOSVarInfo* info, vtkDataArray* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkCellArray* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkFieldData* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkDataSetAttributes* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkDataSet* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkImageData* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkPolyData* data, int blockId);
  void ReadObject(const vtkADIOSDirTree *dir, vtkUnstructuredGrid* data, int blockId);

  char *FileName;
  int ReadMethod;
  char *ReadMethodArguments;
  vtkADIOSDirTree *Tree;
  ADIOSReader *Reader;
  vtkMultiProcessController *Controller;

  // Index information for independently stepped variables

  // Map variable names to thier position in the block step index
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
   * they cannot have thier reference cound safely decremented until after
   * they have been assigned to a vtk object.  To work around this, a generic
   * action queue is created to hold a list of arbitrary functions that need
   * to be called in a particular order after the reads have been
   * processed.  The AddPostReadOperation prototypes use a high number of
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

protected:
  // Used to implement vtkAlgorithm

  int FillOutputPortInformation(int, vtkInformation*);

  bool RequestInformation(vtkInformation *request,
    vtkInformationVector **input, vtkInformationVector *output);
  bool RequestUpdateExtent(vtkInformation *request,
    vtkInformationVector **input, vtkInformationVector *output);
  bool RequestData(vtkInformation *request,
    vtkInformationVector **input, vtkInformationVector *output);

  int NumberOfPieces;
  std::vector<double> TimeSteps;
  std::map<double, size_t> TimeStepsIndex;

  double RequestStep;
  int RequestStepIndex;
  int RequestNumberOfPieces;
  int RequestPiece;

private:
  vtkADIOSReader(const vtkADIOSReader&);  // Not implemented.
  void operator=(const vtkADIOSReader&);  // Not implemented.
};

#define DECLARE_EXPLICIT(T) \
template<> T* vtkADIOSReader::ReadObject<T>(const std::string& path, \
  int blockId);
DECLARE_EXPLICIT(vtkImageData)
DECLARE_EXPLICIT(vtkPolyData)
DECLARE_EXPLICIT(vtkUnstructuredGrid)
#undef DECLARE_EXPLICIT

#endif
