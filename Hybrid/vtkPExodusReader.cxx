/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExodusReader.cxx

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

#include "vtkPExodusReader.h"

#ifndef MERGE_CELLS
#ifndef APPEND
#define APPEND
#endif
#endif

#ifdef APPEND
#include "vtkAppendFilter.h"
#else
#include "vtkMergeCells.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkExodusModel.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCommand.h"

#include "netcdf.h"
#include "vtkExodusII.h"
#include <sys/stat.h>
#include <ctype.h>
#include <vtkstd/vector>

#define DEBUG 0
#define vtkPExodusReaderMAXPATHLEN 2048

vtkStandardNewMacro(vtkPExodusReader);

class vtkPExodusReaderUpdateProgress : public vtkCommand
{
public:
  vtkTypeMacro(vtkPExodusReaderUpdateProgress, vtkCommand)
  static vtkPExodusReaderUpdateProgress* New()
  {
    return new vtkPExodusReaderUpdateProgress;
  }
  void SetReader(vtkPExodusReader* r)
  {
    Reader = r;
  }
  void SetIndex(int i)
  {
    Index = i;
  }
protected:

  vtkPExodusReaderUpdateProgress()
  {
    Reader = NULL;
    Index = 0;
  }
  ~vtkPExodusReaderUpdateProgress(){}

  void Execute(vtkObject*, unsigned long event, void* callData)
  {
    if(event == vtkCommand::ProgressEvent)
    {
      int num = Reader->GetNumberOfFileNames();
      if(num <= 1)
        {
        num = Reader->GetNumberOfFiles();
        }
      double* progress = static_cast<double*>(callData);
      //only use half the progress since append uses the other half
      double newProgress = (*progress/(double)num + Index/(double)num)*0.5;
      Reader->UpdateProgress(newProgress);
    }
  }

  vtkPExodusReader* Reader;
  int Index;
};

class vtkPExodusReaderAppendUpdateProgress : public vtkCommand
{
public:
  vtkTypeMacro(vtkPExodusReaderAppendUpdateProgress, vtkCommand)
  static vtkPExodusReaderAppendUpdateProgress* New()
  {
    return new vtkPExodusReaderAppendUpdateProgress;
  }
  void SetReader(vtkPExodusReader* r)
  {
    Reader = r;
  }
protected:

  vtkPExodusReaderAppendUpdateProgress()
  {
    Reader = NULL;
  }
  ~vtkPExodusReaderAppendUpdateProgress(){}

  void Execute(vtkObject*, unsigned long event, void* callData)
  {
    if(event == vtkCommand::ProgressEvent)
    {
      double* progress = static_cast<double*>(callData);
      double newProgress = 0.5*(*progress)+0.5;
      Reader->UpdateProgress(newProgress);
    }
  }

  vtkPExodusReader* Reader;
};

//----------------------------------------------------------------------------
// Description:
// Instantiate object with NULL filename.
vtkPExodusReader::vtkPExodusReader()
{
  this->FilePattern   = 0;
  this->CurrentFilePattern   = 0;
  this->FilePrefix    = 0;
  this->CurrentFilePrefix    = 0;
  this->FileRange[0]  = -1;
  this->FileRange[1]  = -1;
  this->CurrentFileRange[0]  = 0;
  this->CurrentFileRange[1]  = 0;
  this->NumberOfFiles = 1;
  this->FileNames = NULL;
  this->NumberOfFileNames = 0;
  this->MultiFileName = new char[vtkPExodusReaderMAXPATHLEN];
  this->GenerateFileIdArray = 0;
  this->XMLFileName=NULL;
}
 
//----------------------------------------------------------------------------
vtkPExodusReader::~vtkPExodusReader()
{
  this->SetFilePattern(0);
  this->SetFilePrefix(0);

  // If we've allocated filenames than delete them
  if (this->FileNames) 
    {
    for (int i=0; i<this->NumberOfFileNames; i++)
      {
      if (this->FileNames[i])
        {
        delete [] this->FileNames[i];
        }
      }
      delete [] this->FileNames;
    }

  // Delete all the readers we may have
  for(int reader_idx=static_cast<int>( readerList.size() )-1; reader_idx >= 0; --reader_idx)
    {
    readerList[reader_idx]->Delete();
    readerList.pop_back();
    }

  if (this->CurrentFilePrefix)
    {
    delete [] this->CurrentFilePrefix;
    delete [] this->CurrentFilePattern;
    }

  delete [] this->MultiFileName;
}

//----------------------------------------------------------------------------
int vtkPExodusReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Setting maximum number of pieces to -1 indicates to the
  // upstream consumer that I can provide the same number of pieces
  // as there are number of processors
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  int newName = 
    (this->FileName && 
     !vtkExodusReader::StringsEqual(this->FileName, this->CurrentFileName));

  int newPattern = 
    ((this->FilePattern &&
     !vtkExodusReader::StringsEqual(this->FilePattern, this->CurrentFilePattern)) ||
    (this->FilePrefix &&
     !vtkExodusReader::StringsEqual(this->FilePrefix, this->CurrentFilePrefix)) ||
    (this->FilePattern && 
     ((this->FileRange[0] != this->CurrentFileRange[0]) ||
      (this->FileRange[1] != this->CurrentFileRange[1]))));

  // setting filename for the first time builds the prefix/pattern
  // if one clears the prefix/pattern, but the filename stays the same,
  // we should rebuild the prefix/pattern
  int rebuildPattern = newPattern && this->FilePattern[0] == '\0' &&
                       this->FilePrefix[0] == '\0';

  int sanity = ((this->FilePattern && this->FilePrefix) || this->FileName);

  if (!sanity)
    {
    vtkErrorMacro(<< "Must SetFilePattern AND SetFilePrefix, or SetFileName(s)");
    return 0;
    }

  if (newPattern && !rebuildPattern)
    {
    char *nm = 
      new char[strlen(this->FilePattern) + strlen(this->FilePrefix) + 20];  
    sprintf(nm, this->FilePattern, this->FilePrefix, this->FileRange[0]);
    this->Superclass::SetFileName(nm);
    delete [] nm;
    }
  else if (newName || rebuildPattern)
    {
    if (this->NumberOfFileNames == 1)
      {
      // A singleton file may actually be a hint to look for
      // a series of files with the same base name.  Must compute
      // this now for ParaView.

      this->DeterminePattern(this->FileNames[0]);
      }
    }

  int mmd = this->ExodusModelMetadata;
  this->SetExodusModelMetadata(0);    // turn off for now

  // Read in info based on this->FileName
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  this->SetExodusModelMetadata(mmd); // turn it back, will compute in RequestData 

  if (this->CurrentFilePrefix)
    {
    delete [] this->CurrentFilePrefix;
    this->CurrentFilePrefix = NULL;
    delete [] this->CurrentFilePattern;
    this->CurrentFilePattern = NULL;
    this->CurrentFileRange[0] = 0;
    this->CurrentFileRange[1] = 0;
    }
  if (this->FilePrefix)
    {
    this->CurrentFilePrefix = StrDupWithNew(this->FilePrefix);
    this->CurrentFilePattern = StrDupWithNew(this->FilePattern);
    this->CurrentFileRange[0] = this->FileRange[0];
    this->CurrentFileRange[1] = this->FileRange[1];
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkPExodusReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int fileIndex;
  int processNumber;
  int numProcessors;
  int min, max, idx;
  size_t reader_idx;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // get the ouptut
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // The whole notion of pieces for this reader is really
  // just a division of files between processors
  processNumber =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numProcessors =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int numFiles = this->NumberOfFileNames;
  int start = 0;

  if (numFiles <= 1)
    {
    start = this->FileRange[0];   // use prefix/pattern/range
    numFiles = this->NumberOfFiles;
    }
  
  // Someone has requested a file that is above the number
  // of pieces I have. That may have been caused by having
  // more processors than files. So I'm going to create an
  // empty unstructured grid that contains all the meta
  // information but has 0 cells
  if (processNumber >= numFiles)
    {
#if DEBUG
    vtkWarningMacro("Creating empty grid for processor: " << processNumber);
#endif
    this->SetUpEmptyGrid();
    return 1;
    }

  // Divide the files evenly between processors
  int num_files_per_process = numFiles / numProcessors;

  // This if/else logic is for when you don't have a nice even division of files
  // Each process computes which sequence of files it needs to read in
  int left_over_files = numFiles - (num_files_per_process*numProcessors);
  if (processNumber < left_over_files)
    {
    min = (num_files_per_process+1) * processNumber + start;
    max = min + (num_files_per_process+1) - 1;
    }
  else
    {
    min = num_files_per_process * processNumber + left_over_files + start;
    max = min + num_files_per_process - 1;
    }
#if DEBUG
  vtkWarningMacro("Processor: " << processNumber << " reading files: " << min <<" " <<max);
#endif

#if DEBUG
  vtkWarningMacro("Parallel read for processor: " << processNumber);
#endif

  // We are going to read in the files one by one and then
  // append them together. So now we make sure that we have 
  // the correct number of serial exodus readers and we create 
  // our append object that puts the 'pieces' together
  unsigned int numMyFiles = max - min + 1;

#ifdef APPEND
  vtkAppendFilter *append = vtkAppendFilter::New(); 
  vtkPExodusReaderAppendUpdateProgress* appendProgress = 
        vtkPExodusReaderAppendUpdateProgress::New();
  appendProgress->SetReader(this);
  append->AddObserver(vtkCommand::ProgressEvent, appendProgress);
  appendProgress->Delete();
#else
  int totalSets = 0;
  int totalCells = 0;
  int totalPoints = 0;
  vtkUnstructuredGrid **mergeGrid = new vtkUnstructuredGrid * [numMyFiles];
  memset(mergeGrid, 0, sizeof(vtkUnstructuredGrid *));
#endif

  if (this->ExodusModelMetadata)
    {
    this->NewExodusModel();
    }
  if (readerList.size() < numMyFiles)
    {
    for(reader_idx=readerList.size(); reader_idx < numMyFiles; ++reader_idx)
      {
      vtkExodusReader *er = vtkExodusReader::New();
      vtkPExodusReaderUpdateProgress* progress = 
        vtkPExodusReaderUpdateProgress::New();
      progress->SetReader(this);
      progress->SetIndex(static_cast<int>(reader_idx));
      er->AddObserver(vtkCommand::ProgressEvent, progress);
      progress->Delete();


      //begin USE_EXO_DSP_FILTERS
      if(this->DSPFilteringIsEnabled && this->DSPFilters)
        {
        int i;
        er->DSPFilteringIsEnabled = this->DSPFilteringIsEnabled;
        er->DSPFilters = new vtkDSPFilterGroup*[this->GetNumberOfBlockArrays()];
        for(i=0;i<this->GetNumberOfBlockArrays();i++)
          {
          er->DSPFilters[i] = vtkDSPFilterGroup::New();
          er->DSPFilters[i]->Copy( this->DSPFilters[i] );
          }
        }
      //end USE_EXO_DSP_FILTERS
      
      
      readerList.push_back(er);
      }
    }
  else if (readerList.size() > numMyFiles)
    {
    for(reader_idx=readerList.size()-1; reader_idx >= numMyFiles; --reader_idx)
      {
      readerList[reader_idx]->Delete();
      readerList.pop_back();
      }
    }

  // This constructs the filenames
  for (fileIndex = min, reader_idx=0; fileIndex <= max; ++fileIndex, ++reader_idx)
    {
    int fileId = -1;

    if (this->NumberOfFileNames > 1)
      {
      strcpy(this->MultiFileName, this->FileNames[fileIndex]);
      if (this->GenerateFileIdArray)
        {
        fileId = vtkPExodusReader::DetermineFileId(this->FileNames[fileIndex]);
        }
      }
    else if (this->FilePattern)
      {
      sprintf(this->MultiFileName, this->FilePattern, this->FilePrefix, fileIndex);
      if (this->GenerateFileIdArray)
        {
        fileId = fileIndex;
        }
      }
    else
      {
      // hmmm.... shouldn't get here
      vtkErrorMacro("Some weird problem with filename/filepattern");
      return 0;
      }

    readerList[reader_idx]->SetFileName(this->MultiFileName);
    readerList[reader_idx]->SetTimeStep(this->GetTimeStep());
    readerList[reader_idx]->SetGenerateBlockIdCellArray(
      this->GetGenerateBlockIdCellArray());
    readerList[reader_idx]->SetGenerateGlobalElementIdArray(
      this->GetGenerateGlobalElementIdArray());
    readerList[reader_idx]->SetGenerateGlobalNodeIdArray(
      this->GetGenerateGlobalNodeIdArray());
    readerList[reader_idx]->SetApplyDisplacements(
      this->GetApplyDisplacements());
    readerList[reader_idx]->SetDisplacementMagnitude(
      this->GetDisplacementMagnitude());
    readerList[reader_idx]->SetHasModeShapes(this->GetHasModeShapes());

    readerList[reader_idx]->SetExodusModelMetadata(this->ExodusModelMetadata);
    //readerList[reader_idx]->PackExodusModelOntoOutputOff();

    readerList[reader_idx]->UpdateInformation();

    // Copy point requests.
    for (idx = 0; idx < this->GetNumberOfPointArrays(); ++idx)
      {
      readerList[reader_idx]->SetPointArrayStatus(
        idx, this->GetPointArrayStatus(idx));
      }

    // Copy cell requests.
    for (idx = 0; idx < this->GetNumberOfCellArrays(); ++idx)
      {
      readerList[reader_idx]->SetCellArrayStatus(
        idx, this->GetCellArrayStatus(idx));
      }

    // Copy block requests.
    for (idx = 0; idx < this->GetNumberOfBlockArrays(); ++idx)
      {
      readerList[reader_idx]->SetBlockArrayStatus(
        idx, this->GetBlockArrayStatus(idx));
      }
      
    // Copy nodeset requests.
    for (idx = 0; idx < this->GetNumberOfNodeSetArrays(); ++idx)
      {
      readerList[reader_idx]->SetNodeSetArrayStatus(
        idx, this->GetNodeSetArrayStatus(idx));
      }
      
    // Copy sideset requests.
    for (idx = 0; idx < this->GetNumberOfSideSetArrays(); ++idx)
      {
      readerList[reader_idx]->SetSideSetArrayStatus(
        idx, this->GetSideSetArrayStatus(idx));
      }
    
    vtkInformation* tmpOutInfo = 
      readerList[reader_idx]->GetExecutive()->GetOutputInformation(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
      {
      tmpOutInfo->CopyEntry(
        outInfo,
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
      }
    readerList[reader_idx]->Update();

    vtkUnstructuredGrid *subgrid = vtkUnstructuredGrid::New();
    subgrid->ShallowCopy(readerList[reader_idx]->GetOutput());

    int ncells = subgrid->GetNumberOfCells();

    if ((ncells > 0) && this->GenerateFileIdArray)
      {
      vtkIntArray *ia = vtkIntArray::New();
      ia->SetNumberOfValues(ncells);
      for (idx=0; idx < ncells; idx++)
        {
        ia->SetValue(idx, fileId);
        }
      ia->SetName("vtkFileId");
      subgrid->GetCellData()->AddArray(ia);
      ia->Delete();
      }

    // Don't append if you don't have any cells
    if (ncells != 0)
      {
      if (this->ExodusModelMetadata)
        {
        vtkExodusModel *em = readerList[reader_idx]->GetExodusModel();
        this->ExodusModel->MergeExodusModel(em);
        }

#ifdef APPEND
      append->AddInput(subgrid);
      subgrid->Delete();
#else
      totalSets++;
      totalCells += ncells;
      totalPoints += subgrid->GetNumberOfPoints();
      mergeGrid[reader_idx] = subgrid;
#endif
      }
    }

#ifdef APPEND
  // Append complains/barfs if you update it without any inputs
  if (append->GetInput() != NULL) 
    {
    vtkInformation* appendOutInfo = 
      append->GetExecutive()->GetOutputInformation(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
      {
      appendOutInfo->CopyEntry(
        outInfo,
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
      }
    append->Update();
    output->ShallowCopy(append->GetOutput());
    }

  // I've copied append's output to the 'output' so delete append
  append->Delete();
  append = NULL;

  if (this->PackExodusModelOntoOutput)
    {
    // The metadata is written to field arrays and attached
    // to the output unstructured grid.  (vtkMergeCells does this
    // itself, so we only have to do this for vtkAppendFilter.)

    if (this->ExodusModel)  
      {
      vtkModelMetadata::RemoveMetadata(output);
      this->ExodusModel->GetModelMetadata()->Pack(output);
      }
    }
#else

  // Idea: Modify vtkMergeCells to save point Id and cell Id
  // maps (these could be compressed quite a bit), and then
  // to MergeFieldArraysOnly(int idx, vtkDataSet *set) later
  // on if only field arrays have changed.  Would save a major
  // amount of time, and also leave clues to downstream filters
  // like D3 that geometry has not changed.

  vtkMergeCells *mc = vtkMergeCells::New();
  mc->SetUnstructuredGrid(output);
  mc->SetTotalNumberOfDataSets(totalSets);
  mc->SetTotalNumberOfPoints(totalPoints);
  mc->SetTotalNumberOfCells(totalCells);

  if (this->GetGenerateGlobalNodeIdArray())
    {
    // merge duplicate points using global IDs
    mc->SetGlobalIdArrayName("GlobalNodeId");
    }
  else
    {
    // don't bother trying to merge duplicate points with a locator
    mc->MergeDuplicatePointsOff();
    }

  for (reader_idx=0; reader_idx < numMyFiles; ++reader_idx)
    {
    if (mergeGrid[reader_idx]->GetNumberOfCells() != 0)
      {
      mc->MergeDataSet(mergeGrid[reader_idx]);

      mergeGrid[reader_idx]->Delete();
      }
    }

  delete [] mergeGrid;

  mc->Finish();
  mc->Delete();
#endif


  // This should not be necessary. If broken, investigate
  // further.
  //this->GetOutput()->SetMaximumNumberOfPieces(-1);
  return 1;
}

void vtkPExodusReader::SetUpEmptyGrid() {

  int idx;
  vtkUnstructuredGrid *output = this->GetOutput();

  // Set up an empty unstructured grid
  output->Allocate(0);

  // Create new points
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetNumberOfPoints(0);
  output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = NULL;

  // Set up point arrays
  vtkFloatArray *array = vtkFloatArray::New();
  for (idx = 0; idx < this->GetNumberOfPointArrays(); ++idx) {
    
    // If the point array flag in on for this array
    // then set up this array
    if (this->GetPointArrayStatus(idx)) { 

      // How many dimensions is this array
      int dim = this->GetPointArrayNumberOfComponents(idx);
      array->SetNumberOfComponents(dim);
      array->SetName(this->GetPointArrayName(idx));
      output->GetPointData()->AddArray(array); 
    } 
  }

  // Set up cell arrays
  for (idx = 0; idx < this->GetNumberOfCellArrays(); ++idx) {

    // If the cell array flag in on for this array
    // then set up this array
    if (this->GetCellArrayStatus(idx)) { 

      // How many dimensions is this array
      int dim = this->GetCellArrayNumberOfComponents(idx);
      array->SetNumberOfComponents(dim);
      array->SetName(this->GetCellArrayName(idx));
      output->GetCellData()->AddArray(array);
    }
  }
  // Delete array
  array->Delete();
  array = NULL;
 
  // Set up generated arrays

  vtkIntArray *iarray = vtkIntArray::New();

  if (this->GenerateBlockIdCellArray) {
    iarray->SetName("BlockId");
    iarray->SetNumberOfComponents(1);
    output->GetCellData()->AddArray(iarray);
  }
  if (this->GenerateGlobalNodeIdArray) {
    iarray->SetName("GlobalNodeId");
    iarray->SetNumberOfComponents(1);
    output->GetPointData()->AddArray(iarray);
  }
  if (this->GenerateGlobalElementIdArray) {
    iarray->SetName("GlobalElementId");
    iarray->SetNumberOfComponents(1);
    output->GetCellData()->AddArray(iarray);
  }

  // Delete array
  iarray->Delete();
  iarray = NULL;
}

//----------------------------------------------------------------------------
void vtkPExodusReader::SetFileRange(int min, int max)
{
  if (min == this->FileRange[0] && max == this->FileRange[1])
    {  
    return;
    }
  this->FileRange[0] = min;
  this->FileRange[1] = max;
  this->NumberOfFiles = max-min+1;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkPExodusReader::SetFileName(const char *name)
{
  this->SetFileNames(1, &name);
}
void vtkPExodusReader::SetFileNames(int nfiles, const char **names)
{
  // If I have an old list of filename delete them
  if (this->FileNames){

    for (int i=0; i<this->NumberOfFileNames; i++){
      if (this->FileNames[i]) delete [] this->FileNames[i];
    }
    delete [] this->FileNames;
    this->FileNames = NULL;
  }

  // Set the number of files
  this->NumberOfFileNames = nfiles;


  // Allocate memory for new filenames
  this->FileNames = new char * [this->NumberOfFileNames];

  // Copy filenames
  for (int i=0; i<nfiles; i++){
    this->FileNames[i] = StrDupWithNew(names[i]);
  }

  vtkExodusReader::SetFileName(names[0]);
}

//----------------------------------------------------------------------------
int vtkPExodusReader::DetermineFileId(const char* file)
{
  // Assume the file number is the last digits found in the file name.
  int fileId = 0;
  const char *start = file;
  const char *end = file + strlen(file) - 1;
  const char *numString = end;

  if (!isdigit(*numString))
    {
    while(numString > start)
      {
      --numString;
      if (isdigit(*numString)) break;
      }

    if (numString == start)
      {
      if (isdigit(*numString))
        {
        fileId = atoi(numString);
        }
      return fileId;  // no numbers in file name
      }
    }

  while(numString > start)
    {
    --numString;
    if (!isdigit(*numString)) break;
    }

  if ( (numString == start) && (isdigit(*numString)))
    {
    fileId = atoi(numString);
    }
  else
    {
    fileId = atoi(++numString);
    }

  return fileId;
}
int vtkPExodusReader::DeterminePattern(const char* file)
{
  char *prefix = StrDupWithNew(file);
  char pattern[20] = "%s";
  int scount = 0;
  int cc = 0;
  int res =0;
  int min=0, max=0;

  
  // Check for .ex2 or .ex2v2. If either
  // of these extenstions do not look for
  // a numbered sequence
  char *ex2 = strstr(prefix, ".ex2");
  char *ex2v2 = strstr(prefix, ".ex2v2");
  if (ex2 || ex2v2)
    {
    // Set my info
    this->SetFilePattern(pattern);
    this->SetFilePrefix(prefix);
    this->SetFileRange(min, max);
    delete [] prefix;
    return VTK_OK;
    }



  // Find minimum of range, if any
  for ( cc = static_cast<int>( strlen(file) )-1; cc>=0; cc -- )
    {
    if ( prefix[cc] >= '0' && prefix[cc] <= '9' )
      {
      prefix[cc] = 0;
      scount ++;
      }
    else if ( prefix[cc] == '.' )
      {
      prefix[cc] = 0;
      break;
      }
    else
      {
      break;
      }
    }

  // Determine the pattern
  if ( scount > 0 )
    {
    res = sscanf(file + strlen(file)-scount, "%d", &min);
    if ( res )
      {
      sprintf(pattern, "%%s.%%0%ii", scount);
      }
    }

  // Count up the files
  char buffer[vtkPExodusReaderMAXPATHLEN];
  struct stat fs;
  
  // First go every 100
  for (cc = min+100; res; cc+=100 )
    {
    sprintf(buffer, pattern, prefix, cc);

    // Stat returns -1 if file NOT found
    if ( stat(buffer, &fs) == -1)
      break;

    }
  // Okay if I'm here than stat has failed so -100 on my cc
  cc = cc-100;
  for (cc = cc+1; res; cc++ )
    {
    sprintf(buffer, pattern, prefix, cc);

    // Stat returns -1 if file NOT found
    if ( stat(buffer, &fs) == -1)
      break;

    }
  // Okay if I'm here than stat has failed so -1 on my cc
  max = cc-1;

  // If the user did not specify a range before this, 
  // than set the range to the min and max
  if ((this->FileRange[0]==-1) && (this->FileRange[1]==-1))
    {
    this->SetFileRange(min, max);
    }

   // Set my info
  this->SetFilePattern(pattern);
  this->SetFilePrefix(prefix);
  delete [] prefix;

  return VTK_OK;
}

void vtkPExodusReader::SetGenerateFileIdArray(int flag)
{
  this->GenerateFileIdArray = flag;
  this->Modified();
}

 
//----------------------------------------------------------------------------
void vtkPExodusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExodusReader::PrintSelf(os,indent);

  if (this->FilePattern)
    {
    os << indent << "FilePattern: " << this->FilePattern << endl;
    }
  else
    {
    os << indent << "FilePattern: NULL\n";
    }

  if (this->FilePattern)
    {
    os << indent << "FilePrefix: " << this->FilePrefix << endl;
    }
  else
    {
    os << indent << "FilePrefix: NULL\n";
    }

  os << indent << "FileRange: " 
     << this->FileRange[0] << " " << this->FileRange[1] << endl;

  os << indent << "GenerateFileIdArray: " << this->GenerateFileIdArray << endl;
  os << indent << "NumberOfFiles: " << this->NumberOfFiles << endl;
}


//----------------------------------------------------------------------------
//begin USE_EXO_DSP_FILTERS
//
int vtkPExodusReader::GetNumberOfVariableArrays()
{ 
  return this->vtkExodusReader::GetNumberOfVariableArrays(); 
}
const char *vtkPExodusReader::GetVariableArrayName(int a_which)
{ 
  return this->vtkExodusReader::GetVariableArrayName(a_which); 
}

//note: for the rest of these ..... is the this->vtkExodusReader:: part needed?
void vtkPExodusReader::EnableDSPFiltering()
{ 
  this->vtkExodusReader::EnableDSPFiltering(); 
  for(unsigned int i=0;i<this->readerList.size();i++)
    {
    this->readerList[i]->EnableDSPFiltering();
    }
} 
void vtkPExodusReader::AddFilter(vtkDSPFilterDefinition *a_filter)
{ 
  this->vtkExodusReader::AddFilter(a_filter); 
  for(unsigned int i=0;i<this->readerList.size();i++)
    {
    this->readerList[i]->AddFilter(a_filter);
    }
}
void vtkPExodusReader::StartAddingFilter()
{ 
  this->vtkExodusReader::StartAddingFilter(); 
  for(unsigned int i=0;i<this->readerList.size();i++)
    {
    this->readerList[i]->StartAddingFilter();
    }
}
void vtkPExodusReader::AddFilterInputVar(char *name)
{ 
  this->vtkExodusReader::AddFilterInputVar(name); 
  for(unsigned int i=0;i<this->readerList.size();i++)
    {
    this->readerList[i]->AddFilterInputVar(name); 
    }
}
void vtkPExodusReader::AddFilterOutputVar(char *name)
{ 
  this->vtkExodusReader::AddFilterOutputVar(name); 
  for(unsigned int i=0;i<this->readerList.size();i++) 
    {
    this->readerList[i]->AddFilterOutputVar(name); 
    }
}
void vtkPExodusReader::AddFilterNumeratorWeight(double weight)
{ 
  this->vtkExodusReader::AddFilterNumeratorWeight(weight); 
  for(unsigned int i=0;i<this->readerList.size();i++)
    {
    this->readerList[i]->AddFilterNumeratorWeight(weight);
    }
}
void vtkPExodusReader::AddFilterForwardNumeratorWeight(double weight)
{ 
  this->vtkExodusReader::AddFilterForwardNumeratorWeight(weight); 
  for(unsigned int i=0;i<this->readerList.size();i++)
    {
    this->readerList[i]->AddFilterForwardNumeratorWeight(weight); 
    }
}
void vtkPExodusReader::AddFilterDenominatorWeight(double weight)
{ 
  this->vtkExodusReader::AddFilterDenominatorWeight(weight); 
  for(unsigned int i=0;i<this->readerList.size();i++) 
    {
    this->readerList[i]->AddFilterDenominatorWeight(weight);
    }
}
  void vtkPExodusReader::FinishAddingFilter()
{ 
  this->vtkExodusReader::FinishAddingFilter(); 
  for(unsigned int i=0;i<this->readerList.size();i++) 
    {
    this->readerList[i]->FinishAddingFilter();
    }
}
void vtkPExodusReader::RemoveFilter(char *a_outputVariableName)
{ 
  this->vtkExodusReader::RemoveFilter(a_outputVariableName); 
  for(unsigned int i=0;i<this->readerList.size();i++) 
    {
    this->readerList[i]->RemoveFilter(a_outputVariableName); 
    }
}
void vtkPExodusReader::GetDSPOutputArrays(int exoid, vtkUnstructuredGrid* output)
{ 
  this->Superclass::GetDSPOutputArrays(exoid, output); 
  for(unsigned int i=0;i<this->readerList.size();i++) 
    {
    this->readerList[i]->GetDSPOutputArrays(exoid, output);
    }
}
//end USE_EXO_DSP_FILTERS



int vtkPExodusReader::GetTotalNumberOfElements()
{
  int total = 0;
  for(int id=static_cast<int>( readerList.size() )-1; id >= 0; --id)
    {
    total += this->readerList[id]->GetTotalNumberOfElements();
    }
  return total;
}

int vtkPExodusReader::GetTotalNumberOfNodes()
{
  int total = 0;
  for(int id=static_cast<int>( readerList.size() )-1; id >= 0; --id)
    {
    total += this->readerList[id]->GetTotalNumberOfNodes();
    }
  return total;
}
