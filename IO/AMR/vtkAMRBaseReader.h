/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRBaseReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRBaseReader.h -- Base class for all AMR Readers
//
// .SECTION Description
// An abstract class that encapsulates common functionality for all AMR readers.

#ifndef VTKAMRBASEREADER_H_
#define VTKAMRBASEREADER_H_

#include "vtkIOAMRModule.h" // For export macro
#include "vtkOverlappingAMRAlgorithm.h"
#include <vector>    // STL vector header
#include <map>       // STL map header
#include <utility>   // for STL pair

// Forward Declarations
class vtkOverlappingAMR;
class vtkMultiProcessController;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkIndent;
class vtkAMRDataSetCache;
class vtkUniformGrid;
class vtkDataArray;

class VTKIOAMR_EXPORT vtkAMRBaseReader :
  public vtkOverlappingAMRAlgorithm
{
public:
  vtkTypeMacro( vtkAMRBaseReader, vtkOverlappingAMRAlgorithm );
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Initializes the AMR reader.
  // All concrete instances must call this method in their constructor.
  void Initialize();

  // Description:
  // Set/Get Reader caching property
  vtkSetMacro( EnableCaching, int );
  vtkGetMacro( EnableCaching, int );
  vtkBooleanMacro( EnableCaching, int );
  bool IsCachingEnabled() const
     {
     return( (this->EnableCaching)?true:false);
     };

  // Description:
  // Set/Get a multiprocess-controller for reading in parallel.
  // By default this parameter is set to NULL by the constructor.
  vtkSetMacro( Controller, vtkMultiProcessController* );
  vtkGetMacro( Controller, vtkMultiProcessController* );

  // Description:
  // Set the level, up to which the blocks are loaded.
  vtkSetMacro( MaxLevel,int);

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);

  // Description:
  // Get the number of point or cell arrays available in the input.
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();

  // Description:
  // Get the name of the point or cell array with the given index in
  // the input.
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);

  // Description:
  // Get/Set whether the point or cell array with the given name is to
  // be read.
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);

  // Description:
  // Set/Get the filename. Concrete instances of this class must implement
  // the SetFileName method accordingly.
  vtkGetStringMacro( FileName );
  virtual void SetFileName( const char *fileName ) = 0;

  // Description:
  // Returns the total number of blocks. Implemented by concrete instances.
  virtual int GetNumberOfBlocks() = 0;

  // Description:
  // Returns the total number of levels. Implemented by concrete instances.
  virtual int GetNumberOfLevels() = 0;

protected:
  vtkAMRBaseReader();
  virtual ~vtkAMRBaseReader();

  // Desscription:
  // Checks if this reader instance is attached to a communicator
  // with more than one MPI processes.
  bool IsParallel();

  // Description:
  // Determines if the block is owned by this process based on the
  // the block index and total number of processes.
  bool IsBlockMine( const int blockIdx );

  // Description:
  // Loads the AMR block corresponding to the given index. The block
  // is either loaded from the file, or, from the cache if caching is
  // enabled.
  vtkUniformGrid* GetAMRBlock( const int blockIdx );

  // Description:
  // This method assigns blocks to processes using block-cyclic distribution.
  // It is the method that is used to load distributed AMR data by default.
  void AssignAndLoadBlocks( vtkOverlappingAMR *amrds );

  // Description:
  // This method loads all the blocks in the BlockMap for the given process.
  // It assumes that the downstream module is doing an upstream request with
  // the flag LOAD_REQUESTED_BLOCKS which indicates that the downstream filter
  // has already assigned which blocks are needed for each process.
  void LoadRequestedBlocks( vtkOverlappingAMR *amrds );

  // Description:
  // Loads the AMR data corresponding to the given field name.
  // NOTE: Currently, only cell-data are supported.
  void GetAMRData(
    const int blockIdx, vtkUniformGrid *block, const char *fieldName );


  // Description:
  // Loads the AMR point data corresponding to the given field name.
  void GetAMRPointData(
    const int blockIdx, vtkUniformGrid *block, const char *fieldName );

  // Description:
  // A wrapper that loops over point arrays and load the point
  // arrays that are enabled, i.e., selected for the given block.
  // NOTE: This method is currently not implemented.
  void LoadPointData( const int blockIdx, vtkUniformGrid *block );

  // Description:
  // A wrapper that loops over all cell arrays and loads the cell
  // arrays that are enabled, i.e., selected for the given block.
  // The data are either loaded from the file, or, from the cache if
  // caching is enabled.
  void LoadCellData( const int blockIdx, vtkUniformGrid *block );

  // Description:
  // Returns the block process ID for the block corresponding to the
  // given block index. If this reader instance is serial, i.e., there
  // is no controller associated, the method returns 0. Otherwise, static
  // block-cyclic-distribution is assumed and each block is assigned to
  // a process according to blockIdx%N, where N is the total number of
  // processes.
  int GetBlockProcessId( const int blockIdx );

  // Description:
  // Initializes the request of blocks to be loaded. This method checks
  // if an upstream request has been issued from a downstream module which
  // specifies which blocks are to be loaded, otherwise, it uses the max
  // level associated with this reader instance to determine which blocks
  // are to be loaded.
  void SetupBlockRequest( vtkInformation *outputInfo );

  // Description:
  // Reads all the metadata from the file. Implemented by concrete classes.
  virtual void ReadMetaData() = 0;

  // Description:
  // Returns the block level for the given block
  virtual int GetBlockLevel( const int blockIdx ) = 0;

  // Description:
  // Loads all the AMR metadata & constructs the LevelIdxPair12InternalIdx
  // datastructure which maps (level,id) pairs to an internal linear index
  // used to identify the corresponding block.
  virtual int FillMetaData( ) = 0;

  // Description:
  // Loads the block according to the index w.r.t. the generated BlockMap.
  virtual vtkUniformGrid* GetAMRGrid( const int blockIdx ) = 0;

  // Description:
  // Loads the block data
  virtual void GetAMRGridData(
      const int blockIdx, vtkUniformGrid *block, const char *field ) = 0;

  // Description:
  // Loads the block Point data
  virtual void GetAMRGridPointData(
      const int blockIdx, vtkUniformGrid *block, const char *field ) = 0;

  // Description:
  // Standard Pipeline methods, subclasses may override this method if needed.
 virtual int RequestData(
      vtkInformation* vtkNotUsed(request),
      vtkInformationVector** vtkNotUsed(inputVector),
      vtkInformationVector* outputVector );
  virtual int RequestInformation(
      vtkInformation* rqst,
      vtkInformationVector** inputVector,
      vtkInformationVector* outputVector );
  int FillOutputPortInformation(int port,vtkInformation *info);

  // Array selection member variables and methods
  vtkDataArraySelection *PointDataArraySelection;
  vtkDataArraySelection *CellDataArraySelection;
  vtkCallbackCommand    *SelectionObserver;

  // Description:
  // Initializes the array selections. If this is an initial request,
  // i.e., the first load from the file, all the arrays are deselected,
  // and the IntialRequest ivar is set to false.
  void InitializeArraySelections();

  // Description:
  // Initializes the PointDataArraySelection & CellDataArraySelection
  virtual void SetUpDataArraySelections() = 0;

  // Descriptions
  // Call-back registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject *caller,unsigned long eid,void *clientdata,void *calldata );

  bool InitialRequest;
  int MaxLevel;
  char *FileName;
  vtkMultiProcessController *Controller;

  int EnableCaching;
  vtkAMRDataSetCache *Cache;
  int NumBlocksFromFile;
  int NumBlocksFromCache;

  vtkOverlappingAMR *Metadata;
  bool LoadedMetaData;


  //BTX
    std::vector<int> BlockMap;
  //ETX

private:
  vtkAMRBaseReader( const vtkAMRBaseReader& ); // Not implemented
  void operator=( const vtkAMRBaseReader& ); // Not implemented
};

#endif /* VTKAMRBASEREADER_H_ */
