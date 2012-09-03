/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRBaseParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRBaseParticlesReader.h -- Base class for all AMR particle readers
//
// .SECTION Description
//  An abstract base class that implements all the common functionality for
//  all particle readers.

#ifndef VTKAMRBASEPARTICLESREADER_H_
#define VTKAMRBASEPARTICLESREADER_H_

#include "vtkIOAMRModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkMultiProcessController;
class vtkPolyData;
class vtkDataArraySelection;
class vtkCallbackCommand;

class VTKIOAMR_EXPORT vtkAMRBaseParticlesReader :
  public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro( vtkAMRBaseParticlesReader, vtkMultiBlockDataSetAlgorithm );
  void PrintSelf(ostream &os, vtkIndent indent );

  // Description:
  // Set & Get the frequency.
  vtkGetMacro(Frequency,int);
  vtkSetMacro(Frequency,int);

  // Description:
  // Set & Get the multi-process controller.
  vtkGetMacro(Controller, vtkMultiProcessController* );
  vtkSetMacro(Controller, vtkMultiProcessController* );

  // Description:
  // Set & Get for filter location and boolean macro
  vtkSetMacro(FilterLocation,int);
  vtkGetMacro(FilterLocation,int);
  vtkBooleanMacro(FilterLocation,int);


  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(ParticleDataArraySelection,vtkDataArraySelection);

  // Description:
  // Get the number of particles arrays available in the input.
  int GetNumberOfParticleArrays();

  // Description:
  // Get the particle array name of the array associated with the given
  // index.
  const char* GetParticleArrayName( int index );

  // Description:
  // Get/Set whether the particle array status.
  int GetParticleArrayStatus( const char* name );
  void SetParticleArrayStatus( const char* name, int status );


  virtual void SetFileName( const char *fileName );
  vtkGetStringMacro(FileName);

  // Description:
  // Sets the min location
  inline void SetMinLocation(
      const double minx, const double miny, const double minz )
    {
    this->MinLocation[ 0 ] = minx;
    this->MinLocation[ 1 ] = miny;
    this->MinLocation[ 2 ] = minz;
    }

  // Description:
  // Sets the max location
  inline void SetMaxLocation(
      const double maxx, const double maxy, const double maxz )
    {
    this->MaxLocation[ 0 ] = maxx;
    this->MaxLocation[ 1 ] = maxy;
    this->MaxLocation[ 2 ] = maxz;
    }

  // Description:
  // Returns the total number of particles
  virtual int GetTotalNumberOfParticles() = 0;

protected:
  vtkAMRBaseParticlesReader();
  virtual ~vtkAMRBaseParticlesReader();

  // Description:
  // Reads the metadata, e.g., the number of blocks in the file.
  // After the metadata is read, this->Initialized is set to true.
  // Furthermore, to limit I/O, all concrete classes must make sure
  // that this method returns immediately if this->Initialized is true.
  virtual void ReadMetaData() = 0;

  // Description:
  // Reads the particles corresponding to the block associated with the
  // given supplied block index.
  virtual vtkPolyData* ReadParticles( const int blkIdx ) = 0;

  // Description:
  // Filters particles by their location. If FilterLocation is ON, this
  // method returns whether or not the particle with the supplied xyz
  // coordiantes flass within the bouning box spefixied by the user using
  // the SetMinLocation & SetMaxLocation.
  bool CheckLocation( const double x, const double y, const double z );

  // Description:
  // Determines whether this reader instance is running in parallel or not.
  bool IsParallel( );

  // Description:
  // Determines if the block associated with the given block index belongs
  // to the process that executes the current instance of the reader.
  bool IsBlockMine( const int blkIdx );

  // Description:
  // Given the block index, this method determines the process Id.
  // If the reader instance is serial this method always returns 0.
  // Otherwise, static block-cyclic-distribution is assumed and each
  // block is assigned to a process according to blkIdx%N, where N is
  // the total number of processes.
  int GetBlockProcessId( const int blkIdx );

  // Description:
  // Initializes the AMR Particles reader
  // NOTE: must be called in the constructor of concrete classes.
  void Initialize();

  // Description:
  // Standard Array selection variables & methods
  vtkDataArraySelection *ParticleDataArraySelection;
  vtkCallbackCommand *SelectionObserver;

  // Description:
  // Initializes the ParticleDataArraySelection object. This method
  // only executes for an intial request in which case all arrays are
  // deselected.
  void InitializeParticleDataSelections();

  // Description:
  // Sets up the ParticleDataArraySelection. Implemented
  // by concrete classes.
  virtual void SetupParticleDataSelections() = 0;

  // Description:
  // Call-back registered with the SelectionObserver for selecting/deselecting
  // particles
  static void SelectionModifiedCallback(
   vtkObject *caller,unsigned long eid,void *clientdata,void *calldata );

  // Description:
  // Standard pipeline operations
  virtual int RequestData( vtkInformation *request,
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector );
  virtual int FillOutputPortInformation( int port, vtkInformation *info );

  int NumberOfBlocks;

  int FilterLocation;
  double MinLocation[3];
  double MaxLocation[3];

  int Frequency;
  vtkMultiProcessController *Controller;

  bool  InitialRequest;
  bool  Initialized;
  char *FileName;

private:
  vtkAMRBaseParticlesReader( const vtkAMRBaseParticlesReader& ); // Not implemented
  void operator=(const vtkAMRBaseParticlesReader& ); // Not implemented
};

#endif /* VTKAMRBASEPARTICLESREADER_H_ */
