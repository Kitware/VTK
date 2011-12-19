/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCTHPart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractCTHPart.h"

#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkClipPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkUniformGrid.h"

#include <math.h>
#include <string>
#include <vector>
#include <assert.h>

vtkStandardNewMacro(vtkExtractCTHPart);
vtkCxxSetObjectMacro(vtkExtractCTHPart,ClipPlane,vtkPlane);
vtkCxxSetObjectMacro(vtkExtractCTHPart,Controller,vtkMultiProcessController);

vtkInformationKeyMacro(vtkExtractCTHPart,BOUNDS, DoubleVector);

const double CTH_AMR_SURFACE_VALUE=0.499;
const double CTH_AMR_SURFACE_VALUE_FLOAT=1;
const double CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR=255;

//-----------------------------------------------------------------------------
//=============================================================================
class vtkExtractCTHPartInternal
{
public:
  std::vector<std::string> VolumeArrayNames;
  int DataType;
};
//=============================================================================
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkExtractCTHPart::vtkExtractCTHPart()
{
  this->Internals = new vtkExtractCTHPartInternal;
  this->Internals->DataType=0;

  this->Bounds = new vtkBoundingBox;
  this->ClipPlane = 0;
  
  this->PointVolumeFraction=0;
  
  this->Data=0;
  this->Contour=0;
  this->Append2=0;
  this->Clip1=0;
  this->Cut=0;
  this->Clip2=0;
  
  this->PolyData=0;
  this->SurfacePolyData=0;
  this->RPolyData=0;
  
  this->RData=0;
  this->RContour=0;
  this->RAppend2=0;
  this->RClip1=0;
  this->RCut=0;
  this->RClip2=0;
  this->VolumeFractionType = -1;
  this->VolumeFractionSurfaceValueInternal = CTH_AMR_SURFACE_VALUE;
  this->VolumeFractionSurfaceValue = CTH_AMR_SURFACE_VALUE;
  
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkExtractCTHPart::~vtkExtractCTHPart()
{
  this->SetClipPlane(NULL);
  delete this->Internals;
  delete this->Bounds;
  this->Internals = 0;
  this->DeleteInternalPipeline();
  this->SetController(0);
}

//-----------------------------------------------------------------------------
// Overload standard modified time function. If clip plane is modified,
// then this object is modified as well.
unsigned long vtkExtractCTHPart::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->ClipPlane)
    {
    time = this->ClipPlane->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::RemoveAllVolumeArrayNames()
{
  this->Internals->VolumeArrayNames.erase(
    this->Internals->VolumeArrayNames.begin(),
    this->Internals->VolumeArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::RemoveDoubleVolumeArrayNames()
{
  if (this->Internals->DataType != VTK_DOUBLE)
    {
    return;
    }

  this->Internals->VolumeArrayNames.erase(
    this->Internals->VolumeArrayNames.begin(),
    this->Internals->VolumeArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::RemoveFloatVolumeArrayNames()
{
  if (this->Internals->DataType != VTK_FLOAT)
    {
    return;
    }

  this->Internals->VolumeArrayNames.erase(
    this->Internals->VolumeArrayNames.begin(),
    this->Internals->VolumeArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::RemoveUnsignedCharVolumeArrayNames()
{
  if (this->Internals->DataType != VTK_UNSIGNED_CHAR)
    {
    return;
    }

  this->Internals->VolumeArrayNames.erase(
    this->Internals->VolumeArrayNames.begin(),
    this->Internals->VolumeArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::AddVolumeArrayName(char* arrayName)
{
  if(arrayName==0)
    {
    return;
    }
  
  this->Internals->DataType = 0;
  this->Internals->VolumeArrayNames.push_back(arrayName);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::AddDoubleVolumeArrayName(char* arrayName)
{
  if(arrayName==0)
    {
    return;
    }
  
  if (this->Internals->DataType != VTK_DOUBLE)
    {
    this->RemoveAllVolumeArrayNames();
    this->Internals->DataType = VTK_DOUBLE;
    }

  this->Internals->VolumeArrayNames.push_back(arrayName);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::AddFloatVolumeArrayName(char* arrayName)
{
  if(arrayName==0)
    {
    return;
    }
  
  if (this->Internals->DataType != VTK_FLOAT)
    {
    this->RemoveAllVolumeArrayNames();
    this->Internals->DataType = VTK_FLOAT;
    }

  this->Internals->VolumeArrayNames.push_back(arrayName);  
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::AddUnsignedCharVolumeArrayName(char* arrayName)
{
  if(arrayName==0)
    {
    return;
    }
  
  if (this->Internals->DataType != VTK_UNSIGNED_CHAR)
    {
    this->RemoveAllVolumeArrayNames();
    this->Internals->DataType = VTK_UNSIGNED_CHAR;
    }

  this->Internals->VolumeArrayNames.push_back(arrayName);
  this->Modified();
}


//-----------------------------------------------------------------------------
int vtkExtractCTHPart::GetNumberOfVolumeArrayNames()
{
  return static_cast<int>(this->Internals->VolumeArrayNames.size());
}

//-----------------------------------------------------------------------------
const char* vtkExtractCTHPart::GetVolumeArrayName(int idx)
{
  if ( idx < 0 ||
       idx > static_cast<int>(this->Internals->VolumeArrayNames.size()) )
    {
    return 0;
    }
  return this->Internals->VolumeArrayNames[idx].c_str();
}


//----------------------------------------------------------------------------
int vtkExtractCTHPart::FillInputPortInformation(int port,
                                                vtkInformation *info)
{ 
  if(!this->Superclass::FillInputPortInformation(port,info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractCTHPart::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo;
  
  int num=this->GetNumberOfOutputPorts();
  int port;
  for(port = 0; port<num; port++)
    {
    outInfo=outputVector->GetInformationObject(port);
    // RequestData() synchronizes (communicates among processes), so we need
    // all procs to call RequestData().
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                 -1);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractCTHPart::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->VolumeFractionType = -1;
  // get the info objects
  vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int processNumber = 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numProcessors =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  if(this->Controller==0)
    {
    processNumber=0;
    numProcessors=1;
    }

  // get the input and output
  vtkCompositeDataSet* input = vtkCompositeDataSet::GetData(inInfo);

  vtkMultiBlockDataSet* mbOutput= vtkMultiBlockDataSet::GetData(outInfo);
  unsigned int numBlocks = static_cast<unsigned int>(
    this->Internals->VolumeArrayNames.size());
  mbOutput->SetNumberOfBlocks(numBlocks);
  for (unsigned int i=0; i<numBlocks; i++)
    {
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::New();
    block->SetNumberOfBlocks(numProcessors);
    vtkPolyData* pd = vtkPolyData::New();
    block->SetBlock(processNumber, pd);
    mbOutput->SetBlock(i, block);
    pd->Delete();
    block->Delete();
    }
  
  vtkRectilinearGrid *rg=0;
  
  if(input!=0)
    {
    vtkCompositeDataIterator* iter = input->NewIterator();
    iter->InitTraversal();
    int empty_input =  iter->IsDoneWithTraversal();
    iter->Delete();
    if(empty_input)
      {
      // empty input, do nothing.
      return 1;
      }
    if(inInfo->Has(vtkExtractCTHPart::BOUNDS()))
      {
      double b[6];
      inInfo->Get(vtkExtractCTHPart::BOUNDS(), b);
      this->Bounds->SetBounds(b);
      }
    else
      {
      // compute the bounds
      this->ComputeBounds(input,processNumber,numProcessors);
      }
    }
  else
    {
    rg=vtkRectilinearGrid::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if(rg==0)
      {
      vtkErrorMacro(<<"No valid input.");
      return 0;
      }
    double b[6];
    rg->GetBounds(b);
    this->Bounds->SetBounds(b);
    }
  
  // Here, either input or rg is not null.
  //
  this->EvaluateVolumeFractionType(rg, input);
  
  int idx, num;
  const char* arrayName;
  vtkPolyData *output;
  
  num = this->GetNumberOfVolumeArrayNames();
  
  // Create an append for each part (one part per output).
  vtkAppendPolyData **appendSurface=new  vtkAppendPolyData *[num];
  vtkAppendPolyData **tmps=new vtkAppendPolyData *[num];
  for (idx = 0; idx < num; ++idx)
    {
    appendSurface[idx]= vtkAppendPolyData::New();
    tmps[idx]=vtkAppendPolyData::New();
    }
  int needPartIndex=num>1;
  
  vtkGarbageCollector::DeferredCollectionPush();
  this->CreateInternalPipeline();
  
  float progress, nextProgress;
  if(input!=0)
    {
    for (idx = 0; idx < num; ++idx)
      {
      vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(
        mbOutput->GetBlock(idx));
      arrayName = this->GetVolumeArrayName(idx);
      output = vtkPolyData::SafeDownCast(block->GetBlock(processNumber));
      if(output==0)
        {
        vtkErrorMacro(<<"No output.");
        return 0;
        }
      progress = (1.0/num)*idx;
      nextProgress = progress + 1.0/num;
      this->ExecutePart(arrayName,input,appendSurface[idx],tmps[idx],
                          progress,nextProgress);
      }
    }
  else // rg!=0
    {
    for (idx = 0; idx < num; ++idx)
      {
      arrayName = this->GetVolumeArrayName(idx);
      vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(
        mbOutput->GetBlock(idx));
      output = vtkPolyData::SafeDownCast(block->GetBlock(processNumber));
      if(output==0)
        {
        vtkErrorMacro(<<"No output.");
        return 0;
        }
      progress = (1.0/num)*idx;
      nextProgress = progress + 1.0/num;
      // Does the grid have the requested cell data?
      if (rg->GetCellData()->GetArray(arrayName))
        {
        this->ExecutePartOnRectilinearGrid(arrayName,rg,appendSurface[idx],
                                           tmps[idx], progress, nextProgress);
        }
      else 
        {
        vtkWarningMacro("RectilinearGrid does not contain CellData named "
                        << arrayName);
        vtkPolyData *tmp=vtkPolyData::New();
        tmps[idx]->AddInput(tmp);
        tmp->Delete();
        }
      }
    }

  //vtkDataArray* cellVolumeFraction;
  //cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  //if (cellVolumeFraction == NULL)
  //  {
  //  vtkErrorMacro("Could not find cell array " << arrayName);
  //  return;
  //  }
  //double* range = cellVolumeFraction->GetRange();
  //cout << "@@@@ Range: " << range[0] << " " range[1] << " midpoint: " << ((range[0] + range[1]) * .5) << endl;

  vtkClipPolyData* clip = vtkClipPolyData::New();
  clip->SetValue(this->VolumeFractionSurfaceValueInternal);
  vtkClipPolyData *clip2=clip;
  if (this->ClipPlane)
    {
    // We need another clip for the plane.  Sneak it in.
    clip2 = vtkClipPolyData::New();
    clip2->SetInput(clip->GetOutput());
    clip->Delete();
    clip2->SetClipFunction(this->ClipPlane);
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    arrayName=this->GetVolumeArrayName(idx);
    vtkMultiBlockDataSet* pieces = vtkMultiBlockDataSet::SafeDownCast(
      mbOutput->GetBlock(idx));
    int inputConns = appendSurface[idx]->GetNumberOfInputConnections(0);


    if (inputConns > 0)
      {
      // we have to update the output before get its point data.
      appendSurface[idx]->Update();
#ifndef NDEBUG
      int checkIndex=appendSurface[idx]->GetOutput()->GetPointData()->SetActiveScalars(arrayName);
      assert("check: SetActiveScalar succeeded" && checkIndex>=0);
#else
      appendSurface[idx]->GetOutput()->GetPointData()->SetActiveScalars(arrayName);
#endif
      clip->SetInput(appendSurface[idx]->GetOutput());
      clip2->Update();
      }

#if 1
    tmps[idx]->AddInput(clip2->GetOutput());
#else
    tmps[idx]->AddInput(appendSurface[idx]->GetOutput());
#endif
    
    output = vtkPolyData::SafeDownCast(pieces->GetBlock(processNumber));
    if (inputConns > 0)
      {
      vtkTimerLog::MarkStartEvent("BlockAppend");
      tmps[idx]->Update();
      vtkTimerLog::MarkEndEvent("BlockAppend");     
      }
        
    vtkPolyData* tmpOut = tmps[idx]->GetOutput();
    output->CopyStructure(tmpOut);
    output->GetPointData()->PassData(tmpOut->GetPointData());
    output->GetCellData()->PassData(tmpOut->GetCellData());
    output->GetFieldData()->PassData(tmpOut->GetFieldData());
    // Hopping to avoid some garbage collection time.
    tmps[idx]->RemoveAllInputs();
    tmps[idx]->Delete();
    appendSurface[idx]->Delete();
    tmps[idx] = 0;

    // In the future we might be able to select the rgb color here.
    if (needPartIndex)
      {
      // Add scalars to color this part.
      int numPts = output->GetNumberOfPoints();
      vtkDoubleArray *partArray = vtkDoubleArray::New();
      partArray->SetName("Part Index");
      double *p = partArray->WritePointer(0, numPts);
      for (int idx2 = 0; idx2 < numPts; ++idx2)
        {
        p[idx2] = static_cast<double>(idx);
        }
      output->GetPointData()->SetScalars(partArray);
      partArray->Delete();
      }
    
    // Add a name for this part.
    vtkCharArray *nameArray = vtkCharArray::New();
    nameArray->SetName("Name");
    char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
    sprintf(str, "%s", arrayName);
    output->GetFieldData()->AddArray(nameArray);
    nameArray->Delete();
    }
  delete[] tmps;
  delete[] appendSurface;
  clip2->Delete();
  this->DeleteInternalPipeline();
  vtkGarbageCollector::DeferredCollectionPop();
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::ComputeBounds(vtkCompositeDataSet *input,
                                      int processNumber,
                                      int numProcessors)
{
  assert("pre: input_exists" && input!=0);
  assert("pre: positive_numProcessors" && numProcessors>0);
  assert("pre: valid_processNumber" && processNumber>=0 &&
         processNumber<numProcessors);
 
  vtkCompositeDataIterator* iter = input->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet *ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!ds)// can be null if on another processor
      {
      continue;
      }
    double realBounds[6];
    ds->GetBounds(realBounds);
    this->Bounds->AddBounds(realBounds);
    }
  iter->Delete();

  // Here we have the bounds according to our local datasets.
  // If we are not running in parallel then the local
  // bounds are the global bounds
  if (!this->Controller)
    {
    return;
    }
  vtkCommunicator *comm = this->Controller->GetCommunicator();
  if (!comm)
    {
    return;
    }

  if (!comm->ComputeGlobalBounds(processNumber, numProcessors,
                                 this->Bounds))
    {
    vtkErrorMacro("Problem occurred getting the global bounds");
    }
  // At this point, the global bounds is set in each processor.
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::EvaluateVolumeFractionType(vtkRectilinearGrid* rg, 
  vtkCompositeDataSet* input)
{
  int num = this->GetNumberOfVolumeArrayNames();
  int cc;
  for ( cc = 0; cc < num; ++ cc )
    {
    const char* arrayName = this->GetVolumeArrayName(cc);
    if ( input )
      {
      vtkCompositeDataIterator* iter = input->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        vtkDataSet* dataSet = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if( dataSet== NULL)// cannot really be null since iter skips empty datasets.
          {
          continue;
          }

        vtkDataArray* cellVolumeFraction;
        // Only convert single volume fraction array to point data.
        // Other attributes will have to be viewed as cell data.
        cellVolumeFraction = dataSet->GetCellData()->GetArray(arrayName);
        if (cellVolumeFraction == NULL)
          {
          vtkErrorMacro("Could not find cell array " << arrayName);
          return;
          }
        if (cellVolumeFraction->GetDataType() != VTK_DOUBLE &&
          cellVolumeFraction->GetDataType() != VTK_FLOAT &&
          cellVolumeFraction->GetDataType() != VTK_UNSIGNED_CHAR )
          {
          vtkErrorMacro("Expecting volume fraction to be of type float, "
            "double, or unsigned char.");
          return;
          }
        if ( this->VolumeFractionType >= 0 && 
          this->VolumeFractionType != cellVolumeFraction->GetDataType() )
          {
          vtkErrorMacro("Volume fraction arrays are different type. They "
            "should all be float, double, or unsigned char");
          return;
          }
        if ( this->VolumeFractionType < 0 )
          {
          this->VolumeFractionType = cellVolumeFraction->GetDataType();
          switch ( this->VolumeFractionType )
            {
          case VTK_UNSIGNED_CHAR:
            this->VolumeFractionSurfaceValueInternal
              = CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR * 
              this->VolumeFractionSurfaceValue;
            break;
          default:
            this->VolumeFractionSurfaceValueInternal
              = CTH_AMR_SURFACE_VALUE_FLOAT * 
              this->VolumeFractionSurfaceValue;
            }
          }
        }
      iter->Delete();
      }
    else
      {
      vtkDataArray* cellVolumeFraction;
      // Only convert single volume fraction array to point data.
      // Other attributes will have to be viewed as cell data.
      cellVolumeFraction = rg->GetCellData()->GetArray(arrayName);
      if (cellVolumeFraction == NULL)
        {
        vtkErrorMacro("Could not find cell array " << arrayName);
        return;
        }
      if (cellVolumeFraction->GetDataType() != VTK_DOUBLE &&
        cellVolumeFraction->GetDataType() != VTK_FLOAT &&
        cellVolumeFraction->GetDataType() != VTK_UNSIGNED_CHAR )
        {
        vtkErrorMacro("Expecting volume fraction to be of type float, double, or unsigned char.");
        return;
        }
      if ( this->VolumeFractionType >= 0 && this->VolumeFractionType != cellVolumeFraction->GetDataType() )
        {
        vtkErrorMacro("Volume fraction arrays are different type. They should all be float, double, or unsigned char");
        return;
        }
      if ( this->VolumeFractionType < 0 )
        {
        this->VolumeFractionType = cellVolumeFraction->GetDataType();
        switch ( this->VolumeFractionType )
          {
        case VTK_UNSIGNED_CHAR:
          this->VolumeFractionSurfaceValueInternal
            = CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR * this->VolumeFractionSurfaceValue;
          break;
        default:
          this->VolumeFractionSurfaceValueInternal
            = CTH_AMR_SURFACE_VALUE_FLOAT * this->VolumeFractionSurfaceValue;
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
// the input is a hierarchy of vtkUniformGrid or one level of
// vtkRectilinearGrid. The output is a hierarchy of vtkPolyData.
void vtkExtractCTHPart::ExecutePart(const char *arrayName,
                                    vtkCompositeDataSet *input,
                                    vtkAppendPolyData *appendSurface,
                                    vtkAppendPolyData *append,
                                    float minProgress,
                                    float maxProgress)
{

  // Determine total number of leaf nodes which helps is firing progress events.
  int totalNumberOfDatasets = 0;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    totalNumberOfDatasets++;
    }

  float delProg = (maxProgress-minProgress)/totalNumberOfDatasets;
  int counter = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), counter++)
    {
    float progress = minProgress + delProg*counter;
    if (counter % 30 == 0)
      {
      this->UpdateProgress(progress);
      }
    vtkDataObject *dataObj = iter->GetCurrentDataObject();
    if(dataObj!=0)// can be null if on another processor
      {
      vtkRectilinearGrid *rg=vtkRectilinearGrid::SafeDownCast(dataObj);
      if(rg!=0)
        {
        // Does the input have the requested cell data?
        if (rg->GetCellData()->GetArray(arrayName))
          {
          this->ExecutePartOnRectilinearGrid(arrayName,rg,appendSurface,
            append, progress, progress + delProg);
          }
        else 
          {
          vtkWarningMacro("Rectilinear Grid does not contain CellData named "
            << arrayName << " aborting extraction");
          vtkPolyData *tmp=vtkPolyData::New();
          append->AddInput(tmp);
          tmp->Delete();
          return;
          }
        }
      else
        {
#ifdef EXTRACT_USE_IMAGE_DATA
        vtkImageData *ug=vtkImageData::SafeDownCast(dataObj);
#else
        vtkUniformGrid *ug=vtkUniformGrid::SafeDownCast(dataObj);
#endif
        if(ug!=0)
          {
          // Does the input have the requested cell data?
          if (ug->GetCellData()->GetArray(arrayName))
            {
            this->ExecutePartOnUniformGrid(arrayName,ug,appendSurface,
              append, progress, progress+delProg);
            }
          else 
            {
            vtkWarningMacro("Uniform Grid does not contain CellData named "
              << arrayName << " aborting extraction");
            vtkPolyData *tmp=vtkPolyData::New();
            append->AddInput(tmp);
            tmp->Delete();
            return;
            }
          }
        else
          {
          vtkErrorMacro(<<" cannot handle a block of this type.");
          }
        }
      }
    }

}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::ExecutePartOnUniformGrid(
  const char *arrayName,
#ifdef EXTRACT_USE_IMAGE_DATA
  vtkImageData *input,
#else
  vtkUniformGrid *input,
#endif
  vtkAppendPolyData *appendSurface,
  vtkAppendPolyData *append,
  float minProgress,
  float maxProgress)
{
  vtkPolyData* tmp;
  vtkDataArray* cellVolumeFraction;
  int* dims;
  float delProgress = maxProgress - minProgress;
  int reportProgress = 0;
  if (delProgress > 0.1)
    {
    reportProgress = 1;
    }

  if (reportProgress)
    {
    this->UpdateProgress(minProgress);
    }

  vtkTimerLog::MarkStartEvent("Execute Part");

  // First things first.
  // Convert Cell data array to point data array.
  // Pass cell data.
 
  // Only convert single volume fraction array to point data.
  // Other attributes will have to be viewed as cell data.
  cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  if (cellVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find cell array " << arrayName);
    return;
    }
  if (cellVolumeFraction->GetDataType() != VTK_DOUBLE &&
      cellVolumeFraction->GetDataType() != VTK_FLOAT &&
      cellVolumeFraction->GetDataType() != VTK_UNSIGNED_CHAR )
    {
    vtkErrorMacro("Expecting volume fraction to be of type float, double, or unsigned char.");
    return;
    }
  if ( this->VolumeFractionType >= 0 && this->VolumeFractionType != cellVolumeFraction->GetDataType() )
    {
    vtkErrorMacro("Volume fraction arrays are different type. They should all be float, double, or unsigned char");
    return;
    }
  if ( this->VolumeFractionType < 0 )
    {
    this->VolumeFractionType = cellVolumeFraction->GetDataType();
    switch ( this->VolumeFractionType )
      {
    case VTK_UNSIGNED_CHAR:
      this->VolumeFractionSurfaceValueInternal
        = CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR * this->VolumeFractionSurfaceValue;
      break;
    default:
      this->VolumeFractionSurfaceValueInternal
        = CTH_AMR_SURFACE_VALUE_FLOAT * this->VolumeFractionSurfaceValue;
      }
    }
  
  this->Data->CopyStructure(input);

  vtkDataArray* scalars = input->GetCellData()->GetScalars();
  if (scalars && strcmp(arrayName, scalars->GetName()) == 0)
    { // I do not know why the reader sets attributes, but ....
    this->Data->GetCellData()->CopyScalarsOff();
    }
  
  this->Data->GetCellData()->PassData(input->GetCellData());
  dims = input->GetDimensions();
  this->PointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                   this->PointVolumeFraction, dims,
                                   minProgress, minProgress+delProgress/3, reportProgress);

  

  this->Data->GetPointData()->SetScalars(this->PointVolumeFraction);

  if (reportProgress)
    {
    this->UpdateProgress(minProgress+2*delProgress/3);
    }
  
  int isNotEmpty=this->ExtractUniformGridSurface(this->Data,this->SurfacePolyData);
  if(isNotEmpty)
    {
    tmp=vtkPolyData::New();
    tmp->ShallowCopy(this->SurfacePolyData);
    appendSurface->AddInput(tmp);
    tmp->Delete();
    }
  
  // All outside never has any polydata.
  // Be sure to to that only after the surface filter. 
  double range[2];
  cellVolumeFraction->GetRange(range);
  if (range[1] < this->VolumeFractionSurfaceValueInternal)
    {
    vtkTimerLog::MarkEndEvent("Execute Part");
    return;
    }
  if (this->ClipPlane == 0 && range[0] > this->VolumeFractionSurfaceValueInternal)
    {
    vtkTimerLog::MarkEndEvent("Execute Part");
    return;
    }

  this->PolyData->Update();
  if (reportProgress)
    {
    this->UpdateProgress(minProgress+delProgress);
    }
  
  tmp=vtkPolyData::New();
  tmp->ShallowCopy(this->PolyData);
  append->AddInput(tmp);
  tmp->Delete();
  
  vtkTimerLog::MarkEndEvent("Execute Part");
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::CreateInternalPipeline()
{
  // Objects common to both pipelines
  this->PointVolumeFraction=vtkDoubleArray::New();
  this->SurfacePolyData=vtkPolyData::New();
  
  // Uniform grid case pipeline
  
#ifdef EXTRACT_USE_IMAGE_DATA
  this->Data = vtkImageData::New();
#else
 this->Data = vtkUniformGrid::New();
#endif

  this->Contour=vtkContourFilter::New();
  this->Contour->SetInput(this->Data);
  this->Contour->SetValue(0, this->VolumeFractionSurfaceValueInternal);
  
 
  if(this->ClipPlane)
    {
    // We need to append iso and capped surfaces.
    this->Append2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    this->Clip1=vtkClipPolyData::New();
    this->Clip1->SetInput(this->Contour->GetOutput());
    this->Clip1->SetClipFunction(this->ClipPlane);
    this->Append2->AddInput(this->Clip1->GetOutput());
    
    // We need to create a capping surface.
    this->Cut = vtkCutter::New();
    this->Cut->SetCutFunction(this->ClipPlane);
    this->Cut->SetValue(0, 0.0);
    this->Cut->SetInput(this->Data);
    this->Clip2 = vtkClipPolyData::New();
    this->Clip2->SetInput(this->Cut->GetOutput());
    this->Clip2->SetValue(this->VolumeFractionSurfaceValueInternal);
    this->Append2->AddInput(this->Clip2->GetOutput());
    this->PolyData = this->Append2->GetOutput();
    }
  else
    {
    this->PolyData = this->Contour->GetOutput();
    }
  
  // Rectilinear grid case pipeline
  
  this->RData = vtkRectilinearGrid::New();
  
  this->RContour=vtkContourFilter::New();
  this->RContour->SetInput(this->RData);
  this->RContour->SetValue(0,this->VolumeFractionSurfaceValueInternal);
  
  if(this->ClipPlane)
    {
    // We need to append iso and capped surfaces.
    this->RAppend2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    this->RClip1=vtkClipPolyData::New();
    this->RClip1->SetInput(this->RContour->GetOutput());
    this->RClip1->SetClipFunction(this->ClipPlane);
    this->RAppend2->AddInput(this->RClip1->GetOutput());
    
    // We need to create a capping surface.
    this->RCut = vtkCutter::New();
    this->RCut->SetInput(this->RData);
    this->RCut->SetCutFunction(this->ClipPlane);
    this->RCut->SetValue(0, 0.0);
    this->RClip2 = vtkClipPolyData::New();
    this->RClip2->SetInput(this->RCut->GetOutput());
    this->RClip2->SetValue(this->VolumeFractionSurfaceValueInternal);
    this->RAppend2->AddInput(this->RClip2->GetOutput());
    this->RPolyData = this->RAppend2->GetOutput();
    }
  else
    {
    this->RPolyData = this->RContour->GetOutput();
    }
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::DeleteInternalPipeline()
{
  if(this->PointVolumeFraction!=0)
    {
    this->PointVolumeFraction->Delete();
    this->PointVolumeFraction=0;
    }
  
  if(this->SurfacePolyData!=0)
    {
    this->SurfacePolyData->Delete();
    this->SurfacePolyData=0;
    }
  
  // Uniform grid
  if(this->Data!=0)
    {
    this->Data->Delete();
    this->Data=0;
    }
 
  if(this->Contour!=0)
    {
    this->Contour->Delete();
    this->Contour=0;
    }

   if(this->Append2!=0)
    {
    this->Append2->Delete();
    this->Append2=0;
    }
   if(this->Cut!=0)
     {
     this->Cut->Delete();
     this->Cut=0;
     }
   if(this->Clip1!=0)
     {
     this->Clip1->Delete();
     this->Clip1=0;
    } 
   if(this->Clip2!=0)
     {
     this->Clip2->Delete();
     this->Clip2=0;
     } 
   
   
   // Rectilinear grid
   if(this->RData!=0)
    {
    this->RData->Delete();
    this->RData=0;
    }
  if(this->RContour!=0)
    {
    this->RContour->Delete();
    this->RContour=0;
    }
 
   if(this->RAppend2!=0)
    {
    this->RAppend2->Delete();
    this->RAppend2=0;
    }
   if(this->RCut!=0)
     {
     this->RCut->Delete();
     this->RCut=0;
     }
   if(this->RClip1!=0)
     {
     this->RClip1->Delete();
     this->RClip1=0;
    } 
   if(this->RClip2!=0)
     {
     this->RClip2->Delete();
     this->RClip2=0;
     }
}

//-----------------------------------------------------------------------------
// the input is either a vtkRectilinearGrid or a vtkUniformGrid
void vtkExtractCTHPart::ExecutePartOnRectilinearGrid(
  const char *arrayName,
  vtkRectilinearGrid *input,
  vtkAppendPolyData *appendSurface,
  vtkAppendPolyData *append,
  float minProgress,
  float maxProgress)
{
  assert("pre: valid_input" && input->CheckAttributes()==0);
  
  vtkPolyData* tmp;
  vtkDataArray* cellVolumeFraction;
  int* dims;

  float delProgress = maxProgress - minProgress;
  int reportProgress = 0;
  if (delProgress > 0.1)
    {
    reportProgress = 1;
    }

  vtkTimerLog::MarkStartEvent("Execute Part");

  // First things first.
  // Convert Cell data array to point data array.
  // Pass cell data.
 
  // Only convert single volume fraction array to point data.
  // Other attributes will have to be viewed as cell data.
  cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  if (cellVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find cell array " << arrayName);
    return;
    }
  if (cellVolumeFraction->GetDataType() != VTK_DOUBLE &&
      cellVolumeFraction->GetDataType() != VTK_FLOAT &&
      cellVolumeFraction->GetDataType() != VTK_UNSIGNED_CHAR )
    {
    vtkErrorMacro("Expecting volume fraction to be of type float, double, or unsigned char.");
    return;
    }
  if ( this->VolumeFractionType >= 0 && this->VolumeFractionType != cellVolumeFraction->GetDataType() )
    {
    vtkErrorMacro("Volume fraction arrays are different type. They should all be float, double, or unsigned char");
    return;
    }
  if ( this->VolumeFractionType < 0 )
    {
    this->VolumeFractionType = cellVolumeFraction->GetDataType();
    switch ( this->VolumeFractionType )
      {
    case VTK_UNSIGNED_CHAR:
      this->VolumeFractionSurfaceValueInternal
        = CTH_AMR_SURFACE_VALUE_UNSIGNED_CHAR * this->VolumeFractionSurfaceValue;
      break;
    default:
      this->VolumeFractionSurfaceValueInternal
        = CTH_AMR_SURFACE_VALUE_FLOAT * this->VolumeFractionSurfaceValue;
      }
    }
 
  this->RData->CopyStructure(input);

  vtkDataArray* scalars = input->GetCellData()->GetScalars();
  if (scalars && strcmp(arrayName, scalars->GetName()) == 0)
    { // I do not know why the reader sets attributes, but ....
    this->RData->GetCellData()->CopyScalarsOff();
    }
  
  this->RData->GetCellData()->PassData(input->GetCellData());
  dims = input->GetDimensions();
  this->PointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                   this->PointVolumeFraction, dims,
                                   minProgress, minProgress+delProgress/3, reportProgress);
  

  this->RData->GetPointData()->SetScalars(this->PointVolumeFraction);

  assert("check: valid_rdata" && this->RData->CheckAttributes()==0);
  
  int isNotEmpty=this->ExtractRectilinearGridSurface(this->RData,this->SurfacePolyData);
  if(isNotEmpty)
    {
    tmp=vtkPolyData::New();
    tmp->ShallowCopy(this->SurfacePolyData);
    assert("check: valid_copy" && tmp->CheckAttributes()==0);
    appendSurface->AddInput(tmp);
    tmp->Delete();
    }

  if (reportProgress)
    {
    this->UpdateProgress(minProgress+2*delProgress/3);
    }
  
  // All outside never has any polydata.
  // Be sure to to that only after the surface filter. 
  double range[2];
  cellVolumeFraction->GetRange(range);
  if (range[1] < this->VolumeFractionSurfaceValueInternal)
    {
    vtkTimerLog::MarkEndEvent("Execute Part");
    return;
    }
  if (this->ClipPlane == 0 && range[0] > this->VolumeFractionSurfaceValueInternal)
    {
    vtkTimerLog::MarkEndEvent("Execute Part");
    return;
    }
  
  this->RPolyData->Update();

  if (reportProgress)
    {
    this->UpdateProgress(minProgress+delProgress);
    }

  tmp=vtkPolyData::New();
  tmp->ShallowCopy(this->RPolyData);
  append->AddInput(tmp);
  tmp->Delete();
  
  vtkTimerLog::MarkEndEvent("Execute Part");
}
//-----------------------------------------------------------------------------
// Description:
// Append quads for faces of the block that actually on the bounds
// of the hierarchical dataset. Deals with ghost cells.
// Return true if the output is not empty.
int vtkExtractCTHPart::ExtractRectilinearGridSurface(
  vtkRectilinearGrid *input,
  vtkPolyData *output
  )
{
  assert("pre: valid_input" && input!=0 && input->CheckAttributes()==0);
  assert("pre: output_exists" && output!=0);
  
  int result=0;
#if 1
  int dims[3];
  input->GetDimensions(dims);
  int ext[6];
  int originalExtents[6];
  input->GetExtent(ext);
  input->GetExtent(originalExtents);
  
//  vtkUnsignedCharArray *ghostArray=static_cast<vtkUnsignedCharArray *>(input->GetCellData()->GetArray("vtkGhostLevels"));
  

  
  // bounds without taking ghost cells into account
  double bounds[6];
  
  input->GetBounds(bounds);
  
#if 0
  // block face min x
  if(this->IsGhostFace(0,0,dims,ghostArray))
    {
    // downsize this!
    bounds[0]+=spacing[0];
    ++ext[0];
    }
  if(this->IsGhostFace(0,1,dims,ghostArray))
    {
    // downsize this!
    bounds[1]-=spacing[0];
    --ext[1];
    }
  if(this->IsGhostFace(1,0,dims,ghostArray))
    {
    // downsize this!
    bounds[2]+=spacing[1];
    ++ext[2];
    }
  if(this->IsGhostFace(1,1,dims,ghostArray))
    {
    // downsize this!
    bounds[3]-=spacing[1];
    --ext[3];
    }
  if(this->IsGhostFace(2,0,dims,ghostArray))
    {
    // downsize this!
    bounds[4]+=spacing[2];
    ++ext[4];
    }
  if(this->IsGhostFace(2,1,dims,ghostArray))
    {
    // downsize this!
    bounds[5]-=spacing[2];
    --ext[5];
    }
#endif
  // here, bounds are real block bounds without ghostcells.
  
  const double *minP = this->Bounds->GetMinPoint();
  const double *maxP = this->Bounds->GetMaxPoint();
#if 0
  const double epsilon=0.001;
  int doFaceMinX=fabs(bounds[0]- minP[0])<epsilon;
  int doFaceMaxX=fabs(bounds[1]- maxP[0])<epsilon;
  int doFaceMinY=fabs(bounds[2]- minP[1])<epsilon;
  int doFaceMaxY=fabs(bounds[3]- maxP[1])<epsilon;
  int doFaceMinZ=fabs(bounds[4]- minP[2])<epsilon;
  int doFaceMaxZ=fabs(bounds[5]- maxP[2])<epsilon;
#endif
  
#if 1
  int doFaceMinX=bounds[0]<= minP[0];
  int doFaceMaxX=bounds[1]>= maxP[0];
  int doFaceMinY=bounds[2]<= minP[1];
  int doFaceMaxY=bounds[3]>= maxP[1];
  int doFaceMinZ=bounds[4]<= minP[2];
  int doFaceMaxZ=bounds[5]>= maxP[2];
#endif
#if 0
  int doFaceMinX=1;
  int doFaceMaxX=1;
  int doFaceMinY=1;
  int doFaceMaxY=1;
  int doFaceMinZ=1;
  int doFaceMaxZ=1;
#endif
#if 0
  int doFaceMinX=0;
  int doFaceMaxX=0;
  int doFaceMinY=0;
  int doFaceMaxY=0;
  int doFaceMinZ=0;
  int doFaceMaxZ=0;
#endif
#if 0
  doFaceMaxX=0;
  doFaceMaxY=0;
  doFaceMaxZ=0;
#endif
  
  result=doFaceMinX||doFaceMaxX||doFaceMinY||doFaceMaxY||doFaceMinZ
    ||doFaceMaxZ;
  
  if(result)
    {
    output->Initialize();
    
    vtkIdType numPoints=0;
    vtkIdType cellArraySize=0;
    
//  input->GetExtent(ext);
    
    // Compute an upper bound for the number of points and cells.
    // xMin face
    if (doFaceMinX && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
      {
      cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      }
    // xMax face
    if (doFaceMaxX && ext[2] != ext[3] && ext[4] != ext[5])
      {
      cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      }
    // yMin face
    if (doFaceMinY && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
      {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      }
    // yMax face
    if (doFaceMaxY && ext[0] != ext[1] && ext[4] != ext[5])
      {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      }
    // zMin face
    if (doFaceMinZ && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
      {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      }
    // zMax face
    if (doFaceMaxZ && ext[0] != ext[1] && ext[2] != ext[3])
      {
      cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      }
    
    vtkCellArray *outPolys = vtkCellArray::New();
    outPolys->Allocate(cellArraySize);
    output->SetPolys(outPolys);
    outPolys->Delete();
    
    vtkPoints *outPoints = vtkPoints::New();
    outPoints->Allocate(numPoints);
    output->SetPoints(outPoints);
    outPoints->Delete();
    
    // Allocate attributes for copying.
    output->GetPointData()->CopyAllocate(input->GetPointData());
    output->GetCellData()->CopyAllocate(input->GetCellData());
    
    // Extents are already corrected for ghostcells.
    
    // make each face that is actually on the ds boundary
    if(doFaceMinX)
      {
      this->ExecuteFaceQuads(input,output,0,originalExtents,ext,0,1,2);
      }
    if(doFaceMaxX)
      {
      this->ExecuteFaceQuads(input,output,1,originalExtents,ext,0,2,1);
      }
    if(doFaceMinY)
      {
      this->ExecuteFaceQuads(input,output,0,originalExtents,ext,1,2,0);
      }
    if(doFaceMaxY)
      {
      this->ExecuteFaceQuads(input,output,1,originalExtents,ext,1,0,2);
      }
    if(doFaceMinZ)
      {
      this->ExecuteFaceQuads(input,output,0,originalExtents,ext,2,0,1);
      }
    if(doFaceMaxZ)
      {
      this->ExecuteFaceQuads(input,output,1,originalExtents,ext,2,1,0);
      }
    
    output->Squeeze();
    }
#endif
// result=>valid_surface: A=>B !A||B
  assert("post: valid_surface" && (!result || output->CheckAttributes()==0));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Append quads for faces of the block that actually on the bounds
// of the hierarchical dataset. Deals with ghost cells.
// Return true if the output is not empty.
int vtkExtractCTHPart::ExtractUniformGridSurface(
#ifdef EXTRACT_USE_IMAGE_DATA
  vtkImageData *input,
#else
  vtkUniformGrid *input,
#endif
  vtkPolyData *output
  )
{
  assert("pre: valid_input" && input!=0 && input->CheckAttributes()==0);
  assert("pre: output_exists" && output!=0);
  
  int result=0;
#if 1
  double origin[3];
  input->GetOrigin(origin);
  double spacing[3];
  input->GetSpacing(spacing);
  int dims[3];
  input->GetDimensions(dims);
  int ext[6];
  int originalExtents[6];
  input->GetExtent(ext);
  input->GetExtent(originalExtents);
  
//vtkUnsignedCharArray *ghostArray=static_cast<vtkUnsignedCharArray *>(input->GetCellData()->GetArray("vtkGhostLevels"));
  
  // bounds without taking ghost cells into account
  double bounds[6];
  
  int i, j;
  for (i = 0, j = 0; i < 3; i++, j+=2)
    {
    bounds[j]=origin[i];
    bounds[j+1]=bounds[j]+spacing[i]*(dims[i]-1);
    }
  
#if 0
  // block face min x
  if(this->IsGhostFace(0,0,dims,ghostArray))
    {
    // downsize this!
    bounds[0]+=spacing[0];
    ++ext[0];
    }
  if(this->IsGhostFace(0,1,dims,ghostArray))
    {
    // downsize this!
    bounds[1]-=spacing[0];
    --ext[1];
    }
  if(this->IsGhostFace(1,0,dims,ghostArray))
    {
    // downsize this!
    bounds[2]+=spacing[1];
    ++ext[2];
    }
  if(this->IsGhostFace(1,1,dims,ghostArray))
    {
    // downsize this!
    bounds[3]-=spacing[1];
    --ext[3];
    }
  if(this->IsGhostFace(2,0,dims,ghostArray))
    {
    // downsize this!
    bounds[4]+=spacing[2];
    ++ext[4];
    }
  if(this->IsGhostFace(2,1,dims,ghostArray))
    {
    // downsize this!
    bounds[5]-=spacing[2];
    --ext[5];
    }
#endif
  // here, bounds are real block bounds without ghostcells.
  
  const double *minP = this->Bounds->GetMinPoint();
  const double *maxP = this->Bounds->GetMaxPoint();
#if 0
  const double epsilon=0.001;
  int doFaceMinX=fabs(bounds[0]- minP[0])<epsilon;
  int doFaceMaxX=fabs(bounds[1]- maxP[0])<epsilon;
  int doFaceMinY=fabs(bounds[2]- minP[1])<epsilon;
  int doFaceMaxY=fabs(bounds[3]- maxP[1])<epsilon;
  int doFaceMinZ=fabs(bounds[4]- minP[2])<epsilon;
  int doFaceMaxZ=fabs(bounds[5]- maxP[2])<epsilon;
#endif
  
#if 1
  int doFaceMinX=bounds[0]<= minP[0];
  int doFaceMaxX=bounds[1]>= maxP[0];
  int doFaceMinY=bounds[2]<= minP[1];
  int doFaceMaxY=bounds[3]>= maxP[1];
  int doFaceMinZ=bounds[4]<= minP[2];
  int doFaceMaxZ=bounds[5]>= maxP[2];
#endif

#if 0
  int doFaceMinX=1;
  int doFaceMaxX=1;
  int doFaceMinY=1;
  int doFaceMaxY=1;
  int doFaceMinZ=1;
  int doFaceMaxZ=1;
#endif
#if 0
  int doFaceMinX=0;
  int doFaceMaxX=0;
  int doFaceMinY=0;
  int doFaceMaxY=0;
  int doFaceMinZ=0;
  int doFaceMaxZ=0;
#endif
#if 0
  doFaceMaxX=0;
  doFaceMaxY=0;
  doFaceMaxZ=0;
#endif
  
  result=doFaceMinX||doFaceMaxX||doFaceMinY||doFaceMaxY||doFaceMinZ
    ||doFaceMaxZ;

  if(result)
    {
    output->Initialize();
  vtkIdType numPoints=0;
  vtkIdType cellArraySize=0;
  
//  input->GetExtent(ext);
  
  // Compute an upper bound for the number of points and cells.
  // xMin face
  if (doFaceMinX && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
    {
    cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
  // xMax face
  if (doFaceMaxX && ext[2] != ext[3] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
  // yMin face
  if (doFaceMinY && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    }
  // yMax face
  if (doFaceMaxY && ext[0] != ext[1] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    }
  // zMin face
  if (doFaceMinZ && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    }
  // zMax face
  if (doFaceMaxZ && ext[0] != ext[1] && ext[2] != ext[3])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    }
  
  vtkCellArray *outPolys = vtkCellArray::New();
  outPolys->Allocate(cellArraySize);
  output->SetPolys(outPolys);
  outPolys->Delete();
  
  vtkPoints *outPoints = vtkPoints::New();
  outPoints->Allocate(numPoints);
  output->SetPoints(outPoints);
  outPoints->Delete();

  // Allocate attributes for copying.
  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetCellData()->CopyAllocate(input->GetCellData());
  
  // Extents are already corrected for ghostcells.
  
  // make each face that is actually on the ds boundary
  if(doFaceMinX)
    {
    this->ExecuteFaceQuads(input,output,0,originalExtents,ext,0,1,2);
    }
  if(doFaceMaxX)
    {
    this->ExecuteFaceQuads(input,output,1,originalExtents,ext,0,2,1);
    }
  if(doFaceMinY)
    {
    this->ExecuteFaceQuads(input,output,0,originalExtents,ext,1,2,0);
    }
  if(doFaceMaxY)
    {
    this->ExecuteFaceQuads(input,output,1,originalExtents,ext,1,0,2);
    }
  if(doFaceMinZ)
    {
    this->ExecuteFaceQuads(input,output,0,originalExtents,ext,2,0,1);
    }
  if(doFaceMaxZ)
    {
    this->ExecuteFaceQuads(input,output,1,originalExtents,ext,2,1,0);
    }
  
  output->Squeeze();
    }
#endif
  // result=>valid_surface: A=>B !A||B
  assert("post: valid_surface" && (!result || output->CheckAttributes()==0));
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Is block face on axis0 (either min or max depending on the maxFlag)
// composed of only ghost cells?
int vtkExtractCTHPart::IsGhostFace(int axis0,
                                   int maxFlag,
                                   int dims[3],
                                   vtkUnsignedCharArray *ghostArray)
{
  assert("pre: valid_axis0" && axis0>=0 && axis0<=2);
  
  int axis1=axis0+1;
  if(axis1>2)
    {
    axis1=0;
    }
  int axis2=axis0+2;
  if(axis2>2)
    {
    axis2=0;
    }
  
  int ijk[3]; // index of the cell.
  
  if(maxFlag)
    {
    ijk[axis0]=dims[axis0]-2;
    }
  else
    {
    ijk[axis0]=0;
    }
  
  // We test the center cell of the block face.
  // in the worst case (2x2 cells), we need to know if at least
  // three cells are ghost to say that the face is a ghost face.
  
  ijk[axis1]=dims[axis1]/2-1; // (dims[axis1]-2)/2
  ijk[axis2]=dims[axis2]/2-1; // (dims[axis2]-2)/2
  int result=ghostArray->GetValue(vtkStructuredData::ComputeCellId(dims,ijk));

  if(dims[axis1]==3)
    {
    // axis1 requires 2 cells to be tested.
    // if so, axis1index=0 and axis1index+1=1
    ijk[axis1]=1;
    result=result &&
      ghostArray->GetValue(vtkStructuredData::ComputeCellId(dims,ijk));
    }
 
  if(dims[axis2]==3)
    {
    // herem axis1 may have moved from the previous test.
    // axis2 requires 2 cells to be tested.
    // if so, axis2index=0 and axis2index+1=1
    ijk[axis2]=1;
    result=result &&
      ghostArray->GetValue(vtkStructuredData::ComputeCellId(dims,ijk));
    }
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Merly the same implementation than in vtkDataSetSurfaceFilter, without
// dealing with the whole extents.
void vtkExtractCTHPart::ExecuteFaceQuads(vtkDataSet *input,
                                         vtkPolyData *output,
                                         int maxFlag,
                                         int originalExtents[6],
                                         int ext[6],
                                         int aAxis,
                                         int bAxis,
                                         int cAxis)
{
  assert("pre: input_exists" && input!=0);
  assert("pre: output_exists" && output!=0);
  assert("pre: originalExtents_exists" && originalExtents!=0);
  assert("pre: ext_exists" && ext!=0);
  assert("pre: valid_axes"
         && aAxis>=0 && aAxis<=2
         && bAxis>=0 && bAxis<=2
         && cAxis>=0 && cAxis<=2
         && aAxis!=bAxis
         && aAxis!=cAxis
         && bAxis!=cAxis);
  
  vtkPoints    *outPts;
  vtkCellArray *outPolys;
  vtkPointData *inPD, *outPD;
  vtkCellData  *inCD, *outCD;
  int          pInc[3];
  int          qInc[3];
  int          cOutInc;
  double        pt[3];
  vtkIdType    inStartPtId;
  vtkIdType    inStartCellId;
  vtkIdType    outStartPtId;
  vtkIdType    outPtId;
  vtkIdType    inId, outId;
  int          ib, ic;
  int          aA2, bA2, cA2;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();
  
  pInc[0] = 1;
  pInc[1] = (originalExtents[1]-originalExtents[0]+1);
  pInc[2] = (originalExtents[3]-originalExtents[2]+1) * pInc[1];
  // quad increments (cell incraments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = originalExtents[1]-originalExtents[0];
  // The conditions are for when we have one or more degenerate axes (2d or 1d cells).
  if (qInc[1] == 0)
    {
    qInc[1] = 1;
    }
  qInc[2] = (originalExtents[3]-originalExtents[2]) * qInc[1];
  if (qInc[2] == 0)
    {
    qInc[2] = qInc[1];
    }

  // Temporary variables to avoid many multiplications.
  aA2 = aAxis<<1; // * 2;
  bA2 = bAxis<<1; // * 2;
  cA2 = cAxis<<1; //  * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2+1] || ext[cA2] == ext[cA2+1])
    {
    return;
    }
#if 0
  if (maxFlag)
    { 
    if (ext[aA2+1] < wholeExt[aA2+1])
      {
      return;
      } 
    }
  else
    { // min faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2+1] || ext[aA2] > wholeExt[aA2])
      {
      return;
      }
    }
#endif
  
  if(!maxFlag)
    {
    if (ext[aA2] == ext[aA2+1])
      {
      return;
      }
    }
  
  // Assuming no ghost cells ...
//  inStartPtId = inStartCellId = 0;
  inStartPtId=0; //ext[aA2];
  inStartCellId=0; //ext[aA2];
  
  
  // I put this confusing conditional to fix a regression test.
  // If we are creating a maximum face, then we indeed have to offset the input cell Ids.
  // However, vtkGeometryFilter created a 2d image as a max face, but the cells are copied
  // as a min face (no offset).  Hence maxFlag = 1 and there should be no offset.
  if (maxFlag && ext[aA2] < ext[1+aA2])
    {
    inStartPtId = pInc[aAxis]*(ext[aA2+1]-originalExtents[aA2]); // -ext[aA2]
    inStartCellId = qInc[aAxis]*(ext[aA2+1]-originalExtents[aA2]-1); // -ext[aA2]
    }

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2+1]; ++ic)
    {
    for (ib = ext[bA2]; ib <= ext[bA2+1]; ++ib)
      {
//      inId = inStartPtId + (ib-ext[bA2]+originExtents[bAxis])*pInc[bAxis] 
//                         + (ic-ext[cA2]+originExtents[cAxis])*pInc[cAxis];
      
      inId = inStartPtId + (ib-originalExtents[bA2])*pInc[bAxis] 
        + (ic-originalExtents[cA2])*pInc[cAxis];
      
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD,inId,outId);
      }
    }

  // Do the cells.
  cOutInc = ext[bA2+1] - ext[bA2] + 1;

  outPolys = output->GetPolys();

  // Old method for creating quads (needed for cell data.).
  for (ic = ext[cA2]; ic < ext[cA2+1]; ++ic)
    {
    for (ib = ext[bA2]; ib < ext[bA2+1]; ++ib)
      {
      outPtId = outStartPtId + (ib-ext[bA2]) + (ic-ext[cA2])*cOutInc;
//      inId = inStartCellId + (ib-ext[bA2]+originExtents[bAxis])*qInc[bAxis] + (ic-ext[cA2]+originExtents[cAxis])*qInc[cAxis];
      
      inId = inStartCellId + (ib-originalExtents[bA2])*qInc[bAxis] + (ic-originalExtents[cA2])*qInc[cAxis];

      outId = outPolys->InsertNextCell(4);
      outPolys->InsertCellPoint(outPtId);
      outPolys->InsertCellPoint(outPtId+cOutInc);
      outPolys->InsertCellPoint(outPtId+cOutInc+1);
      outPolys->InsertCellPoint(outPtId+1);

      // Copy cell data.
      outCD->CopyData(inCD,inId,outId);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::ExecuteCellDataToPointData(
  vtkDataArray *cellVolumeFraction, 
  vtkDoubleArray *pointVolumeFraction,
  int *dims,
  float minProgress,
  float maxProgress,
  int reportProgress)
{
  int count;
  int i, j, k;
  int iEnd, jEnd, kEnd;
  int jInc, kInc;
  double *pPoint;
 
  pointVolumeFraction->SetName(cellVolumeFraction->GetName());

  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;
  
  // Deals with none 3D images, otherwise it will never enter into the loop.
  // And then the data will be not initialized and the output of the contour
  // will be empty.
  
  int dimensionality=3;
  
  if(kEnd==0)
    {
    --dimensionality;
    kEnd=1;
    }
  
  // Increments are for the point array.
  jInc = dims[0];
  kInc = (dims[1]) * jInc;
  
  pPoint = pointVolumeFraction->GetPointer(0);
//  pCell = static_cast<double*>(cellVolumeFraction->GetVoidPointer(0));

  // Initialize the point data to 0.
  memset(pPoint, 0,  dims[0]*dims[1]*dims[2]*sizeof(double));

#ifndef NDEBUG
  // for debugging and check out of range.
  double *endPtr=pPoint+dims[0]*dims[1]*dims[2];
#endif
  
  float delProgress = (maxProgress - minProgress) / (kEnd*jEnd*iEnd) / 2;
  vtkIdType counter = 0;

  int index=0;
  // Loop thorugh the cells.
  for (k = 0; k < kEnd; ++k)
    {
    for (j = 0; j < jEnd; ++j)
      {
      for (i = 0; i < iEnd; ++i)
        {
        if (counter % 1000 == 0 && reportProgress)
          {
          this->UpdateProgress(minProgress + delProgress*(i+j*iEnd+k*iEnd*jEnd));
          }
        counter++;
        // Add cell value to all points of cell.
        double value=cellVolumeFraction->GetTuple1(index);
        
        assert("check: valid_range" && pPoint<endPtr);
        assert("check: valid_range" && pPoint+1<endPtr);
        assert("check: valid_range" && pPoint+jInc<endPtr);
        assert("check: valid_range" && pPoint+jInc+1<endPtr);
        
        *pPoint += value;
        pPoint[1] += value;
        pPoint[jInc] += value;
        pPoint[1+jInc] += value;
        
        if(dimensionality==3)
          {
          assert("check: valid_range" && pPoint+kInc<endPtr);
          assert("check: valid_range" && pPoint+kInc+1<endPtr);
          assert("check: valid_range" && pPoint+kInc+jInc<endPtr);
          assert("check: valid_range" && pPoint+kInc+jInc+1<endPtr);
          
          pPoint[kInc] += value;
          pPoint[kInc+1] += value;
          pPoint[kInc+jInc] += value;
          pPoint[kInc+jInc+1] += value;
          }

        // Increment pointers
        ++pPoint;
        ++index;
        }
      // Skip over last point to the start of the next row.
      ++pPoint;
      }
    // Skip over the last row to the start of the next plane.
    pPoint += jInc;
    }

  // Now a second pass to normalize the point values.
  // Loop through the points.
  count = 1;
  pPoint = pointVolumeFraction->GetPointer(0);
  
  // because we eventually modified iEnd, jEnd, kEnd to handle the
  // 2D image case, we have to recompute them.
  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;
  
  counter = 0;
  for (k = 0; k <= kEnd; ++k)
    {
    // Just a fancy fast way to compute the number of cell neighbors of a
    // point.
    if (k == 1)
      {
      count = count << 1;
      }
    if (k == kEnd && kEnd>0)
      {
      // only in 3D case, otherwise count may become zero
      // and be involved in a division by zero later on
      count = count >> 1;
      }
    for (j = 0; j <= jEnd; ++j)
      {
      // Just a fancy fast way to compute the number of cell neighbors of a
      // point.
      if (j == 1)
        {
        count = count << 1;
        }
      if (j == jEnd)
        {
        count = count >> 1;
        }
      for (i = 0; i <= iEnd; ++i)
        {
        if (counter % 1000 == 0 && reportProgress)
          {
          this->UpdateProgress(minProgress + delProgress/2 + delProgress*(i+j*iEnd+k*iEnd*jEnd));
          }
        counter++;
        // Just a fancy fast way to compute the number of cell neighbors of a
        // point.
        if (i == 1)
          {
          count = count << 1;
          }
        if (i == iEnd)
          {
          count = count >> 1;
          }
        assert("check: valid_range" && pPoint<endPtr);
        assert("check: strictly_positive_count" && count>0);
        *pPoint = *pPoint / static_cast<double>(count);
        ++pPoint;
        }
      }
    }
}


//-----------------------------------------------------------------------------
void vtkExtractCTHPart::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeArrayNames: \n";
  vtkIndent i2 = indent.GetNextIndent();
  std::vector<std::string>::iterator it;
  for ( it = this->Internals->VolumeArrayNames.begin();
    it != this->Internals->VolumeArrayNames.end();
    ++ it )
    {
    os << i2 << it->c_str() << endl;
    }
  os << indent << "VolumeFractionSurfaceValue: "
    << this->VolumeFractionSurfaceValue << endl;
  if (this->ClipPlane)
    {
    os << indent << "ClipPlane:\n";
    this->ClipPlane->PrintSelf(os, indent.GetNextIndent());
    }
  else  
    {
    os << indent << "ClipPlane: NULL\n";
    }
  
  if ( this->Controller!=0)
    {
    os << "Controller:" << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "No Controller." << endl;
    }
}

