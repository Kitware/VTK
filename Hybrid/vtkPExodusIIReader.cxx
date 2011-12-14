/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExodusIIReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPExodusIIReader.h"

#include "vtkAppendCompositeDataLeaves.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDoubleArray.h"
#include "vtkExodusIIReaderPrivate.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkExodusModel.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "vtk_netcdf.h"
#include "vtk_exodusII.h"

#include "vtksys/SystemTools.hxx"

#include <vector>

#include <vtksys/RegularExpression.hxx>

#include <sys/stat.h>
#include <ctype.h>

#undef DBG_PEXOIIRDR
#define vtkPExodusIIReaderMAXPATHLEN 2048

static const int objTypes[] = {
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::SIDE_SET,
  vtkExodusIIReader::ELEM_SET,
  vtkExodusIIReader::NODE_MAP,
  vtkExodusIIReader::EDGE_MAP,
  vtkExodusIIReader::FACE_MAP,
  vtkExodusIIReader::ELEM_MAP
};
static const int numObjTypes = sizeof(objTypes)/sizeof(objTypes[0]);

static const int objResultTypes[] = {
  vtkExodusIIReader::NODAL,
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::SIDE_SET,
  vtkExodusIIReader::ELEM_SET,
  vtkExodusIIReader::GLOBAL
};
static const int numObjResultTypes = sizeof(objResultTypes)/sizeof(objResultTypes[0]);

static const int objAttribTypes[] = {
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK
};
static const int numObjAttribTypes = sizeof(objAttribTypes)/sizeof(objAttribTypes[0]);


vtkStandardNewMacro(vtkPExodusIIReader);

class vtkPExodusIIReaderUpdateProgress : public vtkCommand
{
public:
  vtkTypeMacro(vtkPExodusIIReaderUpdateProgress, vtkCommand)
  static vtkPExodusIIReaderUpdateProgress* New()
  {
    return new vtkPExodusIIReaderUpdateProgress;
  }
  void SetReader(vtkPExodusIIReader* r)
  {
    Reader = r;
  }
  void SetIndex(int i)
  {
    Index = i;
  }
protected:

  vtkPExodusIIReaderUpdateProgress()
  {
    Reader = NULL;
    Index = 0;
  }
  ~vtkPExodusIIReaderUpdateProgress(){}

  void Execute(vtkObject*, unsigned long event, void* callData)
  {
    if(event == vtkCommand::ProgressEvent)
    {
      double num = Reader->GetNumberOfFileNames();
      if (num <= 1)
        {
        num = Reader->GetNumberOfFiles();
        }
      double* progress = static_cast<double*>(callData);
      double newProgress = *progress/num + Index/num;
      Reader->UpdateProgress(newProgress);
    }
  }

  vtkPExodusIIReader* Reader;
  int Index;
};


//----------------------------------------------------------------------------
// Description:
// Instantiate object with NULL filename.
vtkPExodusIIReader::vtkPExodusIIReader()
{
  this->ProcRank = 0;
  this->ProcSize = 1;
  // NB. SetController will initialize ProcSize and ProcRank
  this->Controller = 0;
  this->SetController( vtkMultiProcessController::GetGlobalController() );
  this->FilePattern = 0;
  this->CurrentFilePattern = 0;
  this->FilePrefix = 0;
  this->CurrentFilePrefix = 0;
  this->FileRange[0] = -1;
  this->FileRange[1] = -1;
  this->CurrentFileRange[0] = 0;
  this->CurrentFileRange[1] = 0;
  this->NumberOfFiles = 1;
  this->FileNames = NULL;
  this->NumberOfFileNames = 0;
  this->MultiFileName = new char[vtkPExodusIIReaderMAXPATHLEN];
  this->XMLFileName=NULL;
  this->LastCommonTimeStep = -1;
  this->VariableCacheSize = 100;
}
 
//----------------------------------------------------------------------------
vtkPExodusIIReader::~vtkPExodusIIReader()
{
  this->SetController( 0 );
  this->SetFilePattern( 0 );
  this->SetFilePrefix( 0 );

  // If we've allocated filenames then delete them
  if ( this->FileNames ) 
    {
    for (int i=0; i<this->NumberOfFileNames; i++)
      {
      if ( this->FileNames[i] )
        {
        delete [] this->FileNames[i];
        }
      }
      delete [] this->FileNames;
    }

  // Delete all the readers we may have
  std::vector<vtkExodusIIReader*>::iterator it;
  for ( it = this->ReaderList.begin(); it != this->ReaderList.end(); ++ it )
    {
    (*it)->Delete();
    }

  if ( this->CurrentFilePrefix )
    {
    delete [] this->CurrentFilePrefix;
    delete [] this->CurrentFilePattern;
    }

  delete [] this->MultiFileName;
}

//----------------------------------------------------------------------------
void vtkPExodusIIReader::SetController( vtkMultiProcessController* c )
{
  if ( this->Controller == c )
    {
    return;
    }

  this->Modified();

  if ( this->Controller )
    {
    this->Controller->UnRegister( this );
    }

  this->Controller = c;

  if ( this->Controller )
    {
    this->Controller->Register( this );
    this->ProcRank = this->Controller->GetLocalProcessId();
    this->ProcSize = this->Controller->GetNumberOfProcesses();
    }

  if ( ! this->Controller || this->ProcSize <= 0 )
    {
    this->ProcRank = 0;
    this->ProcSize = 1;    
    }
}

//----------------------------------------------------------------------------
int vtkPExodusIIReader::RequestInformation(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  // Setting maximum number of pieces to -1 indicates to the
  // upstream consumer that I can provide the same number of pieces
  // as there are number of processors
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1 );

#ifdef DBG_PEXOIIRDR
  this->Controller->Barrier();
#endif // DBG_PEXOIIRDR
  if ( this->ProcRank == 0 )
    {
    int newName = this->GetMetadataMTime() < this->FileNameMTime;

    int newPattern = 
      (
       ( this->FilePattern &&
         ( ! this->CurrentFilePattern ||
           ! vtksys::SystemTools::ComparePath(
           this->FilePattern, this->CurrentFilePattern ) ||
           ( ( this->FileRange[0] != this->CurrentFileRange[0] ) ||
             ( this->FileRange[1] != this->CurrentFileRange[1] ) ) ) ) ||
       ( this->FilePrefix &&
         ! vtksys::SystemTools::ComparePath(
           this->FilePrefix, this->CurrentFilePrefix ) )
      );

    // setting filename for the first time builds the prefix/pattern
    // if one clears the prefix/pattern, but the filename stays the same,
    // we should rebuild the prefix/pattern
    int rebuildPattern =
      newPattern && this->FilePattern[0] == '\0' && this->FilePrefix[0] == '\0';

    int sanity = ( ( this->FilePattern && this->FilePrefix ) || this->FileName );

    if ( ! sanity )
      {
      vtkErrorMacro( << "Must SetFilePattern AND SetFilePrefix, or SetFileName(s)" );
      this->Broadcast( this->Controller );
      return 0;
      }

    if ( newPattern && ! rebuildPattern )
      {
      char* nm = 
        new char[strlen( this->FilePattern ) + strlen( this->FilePrefix ) + 20];  
      sprintf( nm, this->FilePattern, this->FilePrefix, this->FileRange[0] );
      if ( this->FileName )
        delete [] this->FileName;
      this->FileName = nm;
      //this->Superclass::SetFileName( nm ); // XXX Bad set
      //delete [] nm;
      }
    else if ( newName || rebuildPattern )
      {
      if ( this->NumberOfFileNames == 1 )
        {
        // A singleton file may actually be a hint to look for
        // a series of files with the same base name.  Must compute
        // this now for ParaView.

        this->DeterminePattern( this->FileNames[0] );
        }
      }

    int mmd = this->ExodusModelMetadata;
    this->ExodusModelMetadata = 0;
    //this->SetExodusModelMetadata( 0 );    // turn off for now // XXX Bad set

    /*
    std::string barfle( "/tmp/barfle_" );
    barfle += this->ProcRank + 97;
    barfle += ".txt";
    ofstream fout( barfle.c_str() );
    fout
      << "Proc " << ( this->ProcRank + 1 ) << " of " << this->ProcSize
      << " reading metadata from \"" << this->GetFileName() << "\"\n";
    fout.close();
    */

    // Read in info based on this->FileName
    if ( ! this->Superclass::RequestInformation( request, inputVector, outputVector ) )
      {
      this->Broadcast( this->Controller );
      return 0;
      }

    //this->SetExodusModelMetadata( mmd ); // turn it back, will compute in RequestData // XXX Bad set
    this->ExodusModelMetadata = mmd;
    }
  if ( this->ProcSize > 1 )
    {
    this->Broadcast( this->Controller );
    if ( this->ProcRank )
      {
      // The rank 0 node's RequestInformation annotates the output with the available
      // time steps. Now that we've received time steps, advertise them on other procs.
      this->AdvertiseTimeSteps( outInfo );
      }
    }

  // Check whether we have been given a certain timestep to stop at. If so,
  // override the output time keys with the actual range that ALL readers can read.
  // If files are still being written to, some files might be on different timesteps
  // than others.
  if ( (this->LastCommonTimeStep >= 0) && !this->GetHasModeShapes() )
    {
    double* times = outInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    int numTimes = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    numTimes = this->LastCommonTimeStep + 1 < numTimes ? this->LastCommonTimeStep + 1 : numTimes;
    std::vector<double> commonTimes;
    commonTimes.insert( commonTimes.begin(), times, times + numTimes );
    double timeRange[2];
    timeRange[1] = commonTimes[numTimes - 1];
    timeRange[0] = commonTimes[0];

    outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2 );
    outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &commonTimes[0], numTimes );
    }

  if ( this->CurrentFilePrefix )
    {
    delete [] this->CurrentFilePrefix;
    this->CurrentFilePrefix = NULL;
    delete [] this->CurrentFilePattern;
    this->CurrentFilePattern = NULL;
    this->CurrentFileRange[0] = 0;
    this->CurrentFileRange[1] = 0;
    }

  if ( this->FilePrefix )
    {
    this->CurrentFilePrefix = vtksys::SystemTools::DuplicateString( this->FilePrefix );
    this->CurrentFilePattern = vtksys::SystemTools::DuplicateString( this->FilePattern );
    this->CurrentFileRange[0] = this->FileRange[0];
    this->CurrentFileRange[1] = this->FileRange[1];
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkPExodusIIReader::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector )
{
  int fileIndex;
  int processNumber;
  int numProcessors;
  int min, max, idx;
  int reader_idx;

  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );
  // get the ouptut
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  // The whole notion of pieces for this reader is really
  // just a division of files between processors
  processNumber =
    outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER() );
  numProcessors =
    outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES() );

  int numFiles = this->NumberOfFileNames;
  int start = 0;

  if ( numFiles <= 1 )
    {
    start = this->FileRange[0];   // use prefix/pattern/range
    numFiles = this->NumberOfFiles;
    }


  // Someone has requested a file that is above the number
  // of pieces I have. That may have been caused by having
  // more processors than files. So I'm going to create an
  // empty unstructured grid that contains all the meta
  // information but has 0 cells
  if ( processNumber >= numFiles )
    {
#ifdef DBG_PEXOIIRDR
    vtkWarningMacro("Creating empty grid for processor: " << processNumber);
#endif
    this->Metadata->SetUpEmptyGrid( output );
    return 1;
    }

  // Divide the files evenly between processors
  int num_files_per_process = numFiles / numProcessors;

  // This if/else logic is for when you don't have a nice even division of files
  // Each process computes which sequence of files it needs to read in
  int left_over_files = numFiles - (num_files_per_process*numProcessors);
  if ( processNumber < left_over_files )
    {
    min = (num_files_per_process+1) * processNumber + start;
    max = min + (num_files_per_process+1) - 1;
    }
  else
    {
    min = num_files_per_process * processNumber + left_over_files + start;
    max = min + num_files_per_process - 1;
    }
#ifdef DBG_PEXOIIRDR
  vtkWarningMacro("Processor: " << processNumber << " reading files: " << min <<" " <<max);
#endif

#ifdef DBG_PEXOIIRDR
  vtkWarningMacro("Parallel read for processor: " << processNumber);
#endif

  // We are going to read in the files one by one and then
  // append them together. So now we make sure that we have 
  // the correct number of serial exodus readers and we create 
  // our append object that puts the 'pieces' together
  unsigned int numMyFiles = max - min + 1;

  vtkSmartPointer<vtkAppendCompositeDataLeaves> append =
    vtkSmartPointer<vtkAppendCompositeDataLeaves>::New();
  append->AppendFieldDataOn();

  if ( this->ExodusModelMetadata )
    {
    this->NewExodusModel();
    }

  if ( ReaderList.size() < numMyFiles )
    {
    for ( reader_idx = static_cast<int>( this->ReaderList.size() ); reader_idx < static_cast<int>(numMyFiles); ++reader_idx )
      {
      vtkExodusIIReader* er = vtkExodusIIReader::New();
      vtkPExodusIIReaderUpdateProgress* progress = vtkPExodusIIReaderUpdateProgress::New();
      progress->SetReader( this );
      progress->SetIndex( reader_idx );
      er->AddObserver( vtkCommand::ProgressEvent, progress );
      progress->Delete();      

      this->ReaderList.push_back( er );
      }
    }
  else if ( this->ReaderList.size() > numMyFiles )
    {
    for ( reader_idx = static_cast<int>( this->ReaderList.size() ) - 1; reader_idx >= static_cast<int>(numMyFiles); --reader_idx )
      {
      this->ReaderList[reader_idx]->Delete();
      ReaderList.pop_back();
      }
    }

  // If this is the first execution, we need to initialize the arrays
  // that store the number of points/cells output by each reader
  if(this->NumberOfCellsPerFile.size()==0)
    {
    this->NumberOfCellsPerFile.resize(max-min+1,0);
    }
  if(this->NumberOfPointsPerFile.size()==0)
    {
    this->NumberOfPointsPerFile.resize(max-min+1,0);
    }

#ifdef DBG_PEXOIIRDR
  cout << "\n\n ************************************* Parallel master reader dump\n";
  this->Dump();
#endif // DBG_PEXOIIRDR

  //setup the cache size for each reader
  double fractionalCacheSize = 0;
  if (this->VariableCacheSize > 0 )
    {
    fractionalCacheSize = this->VariableCacheSize / static_cast<int>( this->ReaderList.size() );
    }

  // This constructs the filenames
  int fast_path_reader_index = -1;
  for ( fileIndex = min, reader_idx=0; fileIndex <= max; ++fileIndex, ++reader_idx )
    {
    int fileId = -1;
    if ( this->NumberOfFileNames > 1 )
      {
      strcpy( this->MultiFileName, this->FileNames[fileIndex] );
      if ( this->GetGenerateFileIdArray() )
        {
        fileId = vtkPExodusIIReader::DetermineFileId( this->FileNames[fileIndex] );
        }
      }
    else if ( this->FilePattern )
      {
      sprintf( this->MultiFileName, this->FilePattern, this->FilePrefix, fileIndex );
      if ( this->GetGenerateFileIdArray() )
        {
        fileId = fileIndex;
        }
      }
    else
      {
      vtkErrorMacro("Some weird problem with filename/filepattern");
      return 0;
      }

    if ( outInfo->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() ) )
      { // Get the requested time step. We only support requests of a single time step in this reader right now
      double* requestedTimeSteps = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() );

      // Save the time value in the output data information.
      int length = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
      double* steps = outInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );

      if ( ! this->GetHasModeShapes() )
        {
        int cnt = 0;
        int closestStep = 0;
        double minDist = -1;
        for ( cnt = 0; cnt < length; ++ cnt )
          {
          double tdist =
            ( steps[cnt] - requestedTimeSteps[0] > requestedTimeSteps[0] - steps[cnt] ) ?
            steps[cnt] - requestedTimeSteps[0] : requestedTimeSteps[0] - steps[cnt];
          if ( minDist < 0 || tdist < minDist )
            {
            minDist = tdist;
            closestStep = cnt;
            }
          }
        this->TimeStep = closestStep;
        this->ReaderList[reader_idx]->SetTimeStep( this->TimeStep );
        output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(), steps + this->TimeStep, 1 );
        }
      else
        {
        // Let the metadata know the time value so that the
        // Metadata->RequestData call below will generate the animated mode
        // shape properly.

        // Don't use this->SetModeShapeTime because that will cause Modified
        // to be called.
        //this->SetModeShapeTime( requestedTimeSteps[0] );
        double phase = requestedTimeSteps[0] - floor(requestedTimeSteps[0]);
        this->Metadata->ModeShapeTime = phase;

        this->ReaderList[reader_idx]->SetTimeStep( this->TimeStep );
        this->ReaderList[reader_idx]->SetModeShapeTime( requestedTimeSteps[0] );
        output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(), requestedTimeSteps, 1 );
        //output->GetInformation()->Remove( vtkDataObject::DATA_TIME_STEPS() );
        }
      }
    else
      {
      this->ReaderList[reader_idx]->SetTimeStep( this->TimeStep );
      }

    this->ReaderList[reader_idx]->SetGenerateObjectIdCellArray( this->GetGenerateObjectIdCellArray() );
    this->ReaderList[reader_idx]->SetGenerateGlobalElementIdArray( this->GetGenerateGlobalElementIdArray() );
    this->ReaderList[reader_idx]->SetGenerateGlobalNodeIdArray( this->GetGenerateGlobalNodeIdArray() );
    this->ReaderList[reader_idx]->SetGenerateImplicitElementIdArray( this->GetGenerateImplicitElementIdArray() );
    this->ReaderList[reader_idx]->SetGenerateImplicitNodeIdArray( this->GetGenerateImplicitNodeIdArray() );
    this->ReaderList[reader_idx]->SetGenerateFileIdArray( this->GetGenerateFileIdArray() );
    this->ReaderList[reader_idx]->SetFileId( fileId );
    this->ReaderList[reader_idx]->SetApplyDisplacements( this->GetApplyDisplacements() );
    this->ReaderList[reader_idx]->SetDisplacementMagnitude( this->GetDisplacementMagnitude() );
    this->ReaderList[reader_idx]->SetHasModeShapes( this->GetHasModeShapes() );
    this->ReaderList[reader_idx]->SetAnimateModeShapes( this->GetAnimateModeShapes() );

    this->ReaderList[reader_idx]->SetExodusModelMetadata( this->ExodusModelMetadata );
    // For now, this *must* come last before the UpdateInformation() call because its MTime is compared to the metadata's MTime,
    // which is modified by the calls above.
    this->ReaderList[reader_idx]->SetFileName( this->MultiFileName );
    //this->ReaderList[reader_idx]->PackExodusModelOntoOutputOff();

    this->ReaderList[reader_idx]->UpdateInformation();
#ifdef DBG_PEXOIIRDR
    cout << "\n\n ************************************* Reader " << reader_idx << " dump\n";
    this->ReaderList[reader_idx]->Dump();
#endif // DBG_PEXOIIRDR

    int typ;
    for ( typ = 0; typ < numObjTypes; ++typ )
      {
      int nObj = this->ReaderList[reader_idx]->GetNumberOfObjects( objTypes[typ] );
      for ( idx = 0; idx < nObj; ++idx )
        {
        this->ReaderList[reader_idx]->SetObjectStatus( objTypes[typ], idx, this->GetObjectStatus( objTypes[typ], idx ) );
        }
      }

    for ( typ = 0; typ < numObjAttribTypes; ++typ )
      {
      int nObj = this->ReaderList[reader_idx]->GetNumberOfObjects( objAttribTypes[typ] );
      for ( idx = 0; idx < nObj; ++idx )
        {
        int nObjAtt = this->GetNumberOfObjectAttributes( objAttribTypes[typ], idx );
        for ( int aidx = 0; aidx < nObjAtt; ++aidx )
          {
          this->ReaderList[reader_idx]->SetObjectAttributeStatus( objAttribTypes[typ], idx, aidx,
            this->GetObjectAttributeStatus( objAttribTypes[typ], idx, aidx ) );
          }
        }
      }

    for ( typ = 0; typ < numObjResultTypes; ++typ )
      {
      int nObjArr = this->GetNumberOfObjectArrays( objResultTypes[typ] );
      for ( idx = 0; idx < nObjArr; ++idx )
        {
        this->ReaderList[reader_idx]->SetObjectArrayStatus(
          objResultTypes[typ], idx, this->GetObjectArrayStatus( objResultTypes[typ], idx ) );
        }
      }

    // All keys must be present for the fast-path to work.
    if ( outInfo->Has( vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_TYPE() ) && 
         outInfo->Has( vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_ID() ) && 
         outInfo->Has( vtkStreamingDemandDrivenPipeline::FAST_PATH_ID_TYPE() ) )
      {
      const char *objectType = outInfo->Get(
            vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_TYPE() );
      vtkIdType objectId = outInfo->Get(
            vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_ID() );
      const char *idType = outInfo->Get(
            vtkStreamingDemandDrivenPipeline::FAST_PATH_ID_TYPE() );

      this->ReaderList[reader_idx]->SetFastPathObjectType( objectType );
      this->ReaderList[reader_idx]->SetFastPathObjectId( objectId );
      this->ReaderList[reader_idx]->SetFastPathIdType( idType );
      }
    else
      {
      this->ReaderList[reader_idx]->SetFastPathObjectType("CELL");
      this->ReaderList[reader_idx]->SetFastPathObjectId(-1);
      this->ReaderList[reader_idx]->SetFastPathIdType(0);
      }

    //set this reader to use the full amount of the cache
    this->ReaderList[reader_idx]->SetCacheSize(this->VariableCacheSize);

    //call the reader
    this->ReaderList[reader_idx]->Update();

    //set the reader back to the fractional amount
    this->ReaderList[reader_idx]->SetCacheSize(fractionalCacheSize);

    if (this->ReaderList[reader_idx]->GetProducedFastPathOutput())
      {
      //if (fast_path_reader_index != -1)
      //  {
      //  Requested fast-path Global ID was provided by two readers. This
      //  typically happens for points since points are duplicated among 
      //  pieces. Nothing to worry about, just pick one. 
      //  }
      fast_path_reader_index = reader_idx;
      }

#if 0
    vtkCompositeDataSet* subgrid = this->ReaderList[reader_idx]->GetOutput();
    //subgrid->ShallowCopy( this->ReaderList[reader_idx]->GetOutput() );

    int ncells = subgrid->GetNumberOfCells();

    if ( ( ncells > 0 ) && this->Metadata->GetGenerateFileIdArray() )
      {
      vtkIntArray* ia = vtkIntArray::New();
      ia->SetNumberOfValues(ncells);
      for ( idx = 0; idx < ncells; ++ idx )
        {
        ia->SetValue( idx, fileId );
        }
      ia->SetName( "vtkFileId" );
      subgrid->GetCellData()->AddArray( ia );
      ia->Delete();
      }

    // Don't append if you don't have any cells
    if ( ncells != 0 )
      {
      if ( this->ExodusModelMetadata )
        {
        vtkExodusModel* em = this->ReaderList[reader_idx]->GetExodusModel();
        if ( em )
          {
          this->ExodusModel->MergeExodusModel( em );
          }
        }

      totalCells += ncells;
      totalPoints += subgrid->GetNumberOfPoints();
      this->NumberOfCellsPerFile[reader_idx] = ncells;
      this->NumberOfPointsPerFile[reader_idx] = subgrid->GetNumberOfPoints();

      append->AddInput( subgrid );
      subgrid->Delete();
      }
#else // 0
    append->AddInputConnection( this->ReaderList[reader_idx]->GetOutputPort() );
#endif // 0
    }

  // Append complains/barfs if you update it without any inputs
  if ( append->GetInput() != NULL ) 
    {
    append->Update();
    output->ShallowCopy( append->GetOutput() );
    }

  if (fast_path_reader_index != -1 && fast_path_reader_index !=0)
    {
    // if fast_path_reader_index==0, then the field data is copied over by
    // vtkAppendCompositeDataLeaves so only copy the "OverTime" arrays if the
    // field id > 0 (BUG #9335).
    vtkFieldData* ofd = output->GetFieldData();
    vtkFieldData* ifd = this->ReaderList[fast_path_reader_index]->
      GetOutputDataObject(0)->GetFieldData();
    // Copy all over-time arrays
    int numFieldArrays = ifd->GetNumberOfArrays();
    for (int j=0; j<numFieldArrays; j++)
      {  
      vtkAbstractArray* inFieldArray = ifd->GetAbstractArray(j);
      if (inFieldArray && inFieldArray->GetName())
        {
        vtkStdString fieldName = inFieldArray->GetName();
        
        if (fieldName.find("OverTime",0) != vtkStdString::npos)
          {
          ofd->AddArray(inFieldArray);
          }
        }
      }
    }

  // I've copied append's output to the 'output' so delete append
  append = NULL;

#if 0 // FIXME: Need multiblock version... or not?
  if ( this->PackExodusModelOntoOutput )
    {
    // The metadata is written to field arrays and attached
    // to the output unstructured grid.
    if ( this->ExodusModel ) 
      {
      vtkModelMetadata::RemoveMetadata( output );
      this->ExodusModel->GetModelMetadata()->Pack( output );
      }
    }
#endif // 0

  return 1;
}

//----------------------------------------------------------------------------
void vtkPExodusIIReader::SetFileRange(int min, int max)
{
  if ( min == this->FileRange[0] && max == this->FileRange[1] )
    {  
    return;
    }
  this->FileRange[0] = min;
  this->FileRange[1] = max;
  this->NumberOfFiles = max-min+1;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkPExodusIIReader::SetFileName(const char *name)
{
  this->SetFileNames( 1, &name );
}

void vtkPExodusIIReader::SetFileNames( int nfiles, const char** names )
{
  // If I have an old list of filename delete them
  if ( this->FileNames )
    {
    for ( int i = 0; i < this->NumberOfFileNames; ++ i )
      {
      if ( this->FileNames[i] )
        {
        delete [] this->FileNames[i];
        }
      }
    delete [] this->FileNames;
    this->FileNames = NULL;
    }

  // Set the number of files
  this->NumberOfFileNames = nfiles;

  // Allocate memory for new filenames
  this->FileNames = new char*[this->NumberOfFileNames];

  // Copy filenames
  for (int i = 0; i < nfiles; ++ i )
    {
    this->FileNames[i] = vtksys::SystemTools::DuplicateString( names[i] );
    }

  this->Superclass::SetFileName( names[0] );
}

//----------------------------------------------------------------------------
int vtkPExodusIIReader::DetermineFileId( const char* file )
{
  // Assume the file number is the last digits found in the file name.
  int fileId = 0;
  const char* start = file;
  const char* end = file + strlen(file) - 1;
  const char* numString = end;

  if ( ! isdigit( *numString ) )
    {
    while ( numString > start )
      {
      --numString;
      if ( isdigit( *numString ) ) break;
      }

    if ( numString == start )
      {
      if ( isdigit( *numString ) )
        {
        fileId = atoi( numString );
        }
      return fileId;  // no numbers in file name
      }
    }

  while(numString > start)
    {
    --numString;
    if ( ! isdigit( *numString ) ) break;
    }

  if ( ( numString == start ) && ( isdigit( *numString ) ) )
    {
    fileId = atoi( numString );
    }
  else
    {
    fileId = atoi( ++ numString );
    }

  return fileId;
}

int vtkPExodusIIReader::DeterminePattern( const char* file )
{
  char pattern[20] = "%s";
  int scount = 0;
  int cc = 0;
  int min = 0, max = 0;

  // First check for file names for which we should _not_ look for a numbered
  // sequence.  If using the extension .ex2 or .ex2v2, then we should not.
  // Furthermore, if the filename ends in .e-s#, then this number is indicative
  // of a restart number, not a partition number, so we should not look for
  // numbered sequences there either.
  vtksys::RegularExpression ex2RegEx("\\.ex2$");
  vtksys::RegularExpression ex2v2RegEx("\\.ex2v2$");
  vtksys::RegularExpression restartRegEx("\\.e-s\\.?[0-9]+(\\.ex2v[0-9]+)?$");

  // This regular expression finds the number for a numbered sequence.  This
  // number appears at the end of file (or potentially right before an extension
  // like .ex2v3 or perhaps a future version of this extension).  The matches
  // (in parentheses) are as follows:
  // 1 - The prefix.
  // 2 - The sequence number.
  // 3 - The optional extension.
  vtksys::RegularExpression
    numberRegEx("^(.*[^0-9])([0-9]+)(\\.ex2v[0-9]+)?$");

  if (   ex2RegEx.find(file) || ex2v2RegEx.find(file)
      || restartRegEx.find(file) || !numberRegEx.find(file) )
    {
    // Set my info
    //this->SetFilePattern( pattern ); // XXX Bad set
    //this->SetFilePrefix( file ); // XXX Bad set
    //this->SetFileRange( min, max ); // XXX Bad set
    if ( this->FilePattern )
      delete [] this->FilePattern;
    if ( this->FilePrefix )
      delete [] this->FilePrefix;
    this->FilePattern = vtksys::SystemTools::DuplicateString( pattern );
    this->FilePrefix = vtksys::SystemTools::DuplicateString( file );
    this->FileRange[0] = min;
    this->FileRange[1] = max;
    this->NumberOfFiles = max - min + 1;
    return VTK_OK;
    }

  // If we are here, then numberRegEx matched and we have found the part of
  // the filename that is the number.  Extract the filename parts.
  std::string prefix = numberRegEx.match(1);
  scount = static_cast<int>(numberRegEx.match(2).size());
  std::string extension = numberRegEx.match(3);

  // Determine the pattern
  sprintf(pattern, "%%s%%0%ii%s", scount, extension.c_str());

  // Count up the files
  char buffer[1024];
  struct stat fs;
  
  // First go up every 100
  for ( cc = min + 100; true; cc += 100 )
    {
    sprintf( buffer, pattern, prefix.c_str(), cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1 )
      break;

    }
  // Okay if I'm here than stat has failed so -100 on my cc
  cc = cc - 100;
  for ( cc = cc + 1; true; ++cc )
    {
    sprintf( buffer, pattern, prefix.c_str(), cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1 )
      break;
    }
  // Okay if I'm here than stat has failed so -1 on my cc
  max = cc - 1;

  // Second, go down every 100
  // We can't assume that we're starting at 0 because the file selector
  // will pick up every file that ends in .ex2v3... not just the first one.
  for ( cc = min - 100; true; cc -= 100 )
    {
    if ( cc < 0 )
      break;

    sprintf( buffer, pattern, prefix.c_str(), cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1 )
      break;

    }

  cc += 100;
  // Okay if I'm here than stat has failed so -100 on my cc
  for (cc = cc - 1; true; --cc )
    {
    if ( cc < 0 )
      break;

    sprintf( buffer, pattern, prefix.c_str(), cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1 )
      break;
    }
  min = cc + 1;

  // If the user did not specify a range before this, 
  // than set the range to the min and max
  if ( ( this->FileRange[0] == -1 ) && ( this->FileRange[1] == -1 ) )
    {
    //this->SetFileRange( min, max ); // XXX Bad set
    this->FileRange[0] = min;
    this->FileRange[1] = max;
    this->NumberOfFiles = max - min + 1;
    }

   // Set my info
  //this->SetFilePattern( pattern ); // XXX Bad set
  //this->SetFilePrefix( prefix ); // XXX Bad set
  //delete [] prefix;
  if ( this->FilePattern )
    delete [] this->FilePattern;
  if ( this->FilePrefix )
    delete [] this->FilePrefix;
  this->FilePattern = vtksys::SystemTools::DuplicateString( pattern );
  this->FilePrefix = vtksys::SystemTools::DuplicateString(prefix.c_str());

  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkPExodusIIReader::PrintSelf( ostream& os, vtkIndent indent )
{
  vtkExodusIIReader::PrintSelf( os, indent );

  if ( this->FilePattern )
    {
    os << indent << "FilePattern: " << this->FilePattern << endl;
    }
  else
    {
    os << indent << "FilePattern: NULL\n";
    }

  if ( this->FilePattern )
    {
    os << indent << "FilePrefix: " << this->FilePrefix << endl;
    }
  else
    {
    os << indent << "FilePrefix: NULL\n";
    }

  os << indent << "FileRange: " 
     << this->FileRange[0] << " " << this->FileRange[1] << endl;

  os << indent << "NumberOfFiles: " << this->NumberOfFiles << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "VariableCacheSize: " << this->VariableCacheSize << endl;
}

vtkIdType vtkPExodusIIReader::GetTotalNumberOfElements()
{
  vtkIdType total = 0;
  std::vector<vtkExodusIIReader*>::iterator it;
  for ( it = this->ReaderList.begin(); it != this->ReaderList.end(); ++ it )
    {
    total += (*it)->GetTotalNumberOfElements();
    }
  return total;
}

vtkIdType vtkPExodusIIReader::GetTotalNumberOfNodes()
{
  vtkIdType total = 0;
  std::vector<vtkExodusIIReader*>::iterator it;
  for ( it = this->ReaderList.begin(); it != this->ReaderList.end(); ++ it )
    {
    total += (*it)->GetTotalNumberOfNodes();
    }
  return total;
}

void vtkPExodusIIReader::UpdateTimeInformation()
{
  // Before we start, make sure that we have readers to read (i.e. that 
  // RequestData() has been called. 
  if ( this->ReaderList.size() == 0 )
    {
    return;
    }

  int lastTimeStep = VTK_INT_MAX;
  int numTimeSteps = 0;
  for ( size_t reader_idx = 0; reader_idx < this->ReaderList.size(); ++ reader_idx )
    {
    vtkExodusIIReader *reader = this->ReaderList[reader_idx];

    // In order to get an up-to-date number of timesteps, update the reader's
    // time information first
    reader->UpdateTimeInformation();
    numTimeSteps = reader->GetNumberOfTimeSteps();

    // if this reader's last time step is less than the one we have, use it instead
    lastTimeStep = numTimeSteps-1 < lastTimeStep ? numTimeSteps-1 : lastTimeStep;
    }

  this->LastCommonTimeStep = lastTimeStep;

  this->Superclass::UpdateTimeInformation();
  this->Modified();
  this->UpdateInformation();
}

static void BroadcastXmitString( vtkMultiProcessController* ctrl, char* str )
{
  int len;
  if ( str )
    {
    len = static_cast<int>( strlen( str ) ) + 1;
    ctrl->Broadcast( &len, 1, 0 );
    ctrl->Broadcast( str, len, 0 );
    }
  else
    {
    len = 0;
    ctrl->Broadcast( &len, 1, 0 );
    }
}

static bool BroadcastRecvString( vtkMultiProcessController* ctrl, std::vector<char>& str )
{
  int len;
  ctrl->Broadcast( &len, 1, 0 );
  if ( len )
    {
    str.resize( len );
    ctrl->Broadcast( &str[0], len, 0 );
    return true;
    }
  return false;
}

void vtkPExodusIIReader::Broadcast( vtkMultiProcessController* ctrl )
{
  if ( ctrl )
    {
    this->Metadata->Broadcast( ctrl );
    ctrl->Broadcast( this->TimeStepRange, 2, 0 );
    int rank = ctrl->GetLocalProcessId();
    if ( rank == 0 )
      {
      BroadcastXmitString( ctrl, this->FilePattern );
      BroadcastXmitString( ctrl, this->FilePrefix );
      }
    else
      {
      std::vector<char> tmp;
      if ( this->FilePattern )
        delete [] this->FilePattern;
      if ( this->FilePrefix )
        delete [] this->FilePrefix;
      //this->SetFilePattern( BroadcastRecvString( ctrl, tmp ) ? &tmp[0] : 0 ); // XXX Bad set
      //this->SetFilePrefix(  BroadcastRecvString( ctrl, tmp ) ? &tmp[0] : 0 ); // XXX Bad set
      this->FilePattern = BroadcastRecvString( ctrl, tmp ) ? vtksys::SystemTools::DuplicateString( &tmp[0] ) : 0;
      this->FilePrefix =  BroadcastRecvString( ctrl, tmp ) ? vtksys::SystemTools::DuplicateString( &tmp[0] ) : 0;
      }
    ctrl->Broadcast( this->FileRange, 2, 0 );
    ctrl->Broadcast( &this->NumberOfFiles, 1, 0 );
    }
}
