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

#include "vtkToolkits.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkClipPolyData.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkCharArray.h"
#include "vtkTimerLog.h"

#include "vtkExecutive.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"

#include <math.h>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkExtractCTHPart, "1.8");
vtkStandardNewMacro(vtkExtractCTHPart);
vtkCxxSetObjectMacro(vtkExtractCTHPart,ClipPlane,vtkPlane);

//-----------------------------------------------------------------------------
//=============================================================================
class vtkExtractCTHPartInternal
{
public:
  vtkstd::vector<vtkstd::string> VolumeArrayNames;

};
//=============================================================================
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkExtractCTHPart::vtkExtractCTHPart()
{
  this->Internals = new vtkExtractCTHPartInternal;
  this->ClipPlane = 0;
  this->SetNumberOfOutputPorts(0);
  
  this->PointVolumeFraction=0;
  
  this->Data=0;
  this->Contour=0;
  this->Append1=0;
  this->Surface=0;
  this->Clip0=0;
  this->Append2=0;
  this->Clip1=0;
  this->Cut=0;
  this->Clip2=0;
  
  this->RData=0;
  this->RContour=0;
  this->RAppend1=0;
  this->RSurface=0;
  this->RClip0=0;
  this->RAppend2=0;
  this->RClip1=0;
  this->RCut=0;
  this->RClip2=0; 
}

//-----------------------------------------------------------------------------
vtkExtractCTHPart::~vtkExtractCTHPart()
{
  this->SetClipPlane(NULL);
  delete this->Internals;
  this->Internals = 0;
  this->DeleteInternalPipeline();
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
  this->SetNumberOfOutputPorts(0);
  
  this->Internals->VolumeArrayNames.erase(
    this->Internals->VolumeArrayNames.begin(),
    this->Internals->VolumeArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::AddVolumeArrayName(char* arrayName)
{
  if ( !arrayName )
    {
    return;
    }
  vtkHierarchicalDataSet *hd=vtkHierarchicalDataSet::New();
  
  this->Internals->VolumeArrayNames.push_back(arrayName);
  
  int num = this->GetNumberOfOutputPorts();
  this->SetNumberOfOutputPorts(num+1);
  this->SetOutputData(num, hd);
  hd->FastDelete();
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkExtractCTHPart::GetNumberOfVolumeArrayNames()
{
  return this->Internals->VolumeArrayNames.size();
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
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkCompositeDataSet");
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractCTHPart::SetOutputData(int idx, vtkHierarchicalDataSet* d)
{
  this->GetExecutive()->SetOutputData(idx, d);
}

//----------------------------------------------------------------------------
int vtkExtractCTHPart::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo;
  
  int num=this->GetNumberOfOutputPorts();
  int port=0;
  while(port<num)
    {
    outInfo=outputVector->GetInformationObject(port);
    // RequestData() synchronizes (communicates among processes), so we need
    // all procs to call RequestData().
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                 -1);
    ++port;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractCTHPart::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // get the input and output
  vtkHierarchicalDataSet *input=vtkHierarchicalDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  
  if(input!=0)
    {
    int idx, num;
    const char* arrayName;
    vtkHierarchicalDataSet *output;
    
    num = this->GetNumberOfVolumeArrayNames();
    int needPartIndex=num>1;
    
    vtkGarbageCollector::DeferredCollectionPush();
    this->CreateInternalPipeline();
    
    for (idx = 0; idx < num; ++idx)
      {
      arrayName = this->GetVolumeArrayName(idx);
      output = this->GetOutput(idx);
      if(output==0)
        {
        vtkErrorMacro(<<"No output.");
        return 0;
        }
      this->ExecutePart(arrayName, idx,input,output,needPartIndex);
      }
    this->DeleteInternalPipeline();
    vtkGarbageCollector::DeferredCollectionPop();
    }
  else
    {
    vtkRectilinearGrid *old=vtkRectilinearGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if(old==0)
      {
      vtkErrorMacro(<<"No input.");
      return 0;
      }
    
    int idx;
    const char* arrayName;
    vtkHierarchicalDataSet *output;   
    
    int num = this->GetNumberOfVolumeArrayNames();
    int needPartIndex=num>1;
    
    vtkGarbageCollector::DeferredCollectionPush();
    this->CreateInternalPipeline();
    
    for (idx = 0; idx < num; ++idx)
      {
      arrayName = this->GetVolumeArrayName(idx);
      output = this->GetOutput(idx);
      if(output==0)
        {
        vtkErrorMacro(<<"No output.");
        return 0;
        }
      vtkPolyData *pd=vtkPolyData::New();
      output->SetNumberOfLevels(1);
      output->SetNumberOfDataSets(0,1);
      output->SetDataSet(0,0,pd);
      pd->FastDelete();
      this->ExecutePartOnRectilinearGrid(arrayName,old,pd);
      if(needPartIndex)
        {
        // Add scalars to color this part.
        int numPts = pd->GetNumberOfPoints();
        vtkDoubleArray *partArray = vtkDoubleArray::New();
        partArray->SetName("Part Index");
        double *p = partArray->WritePointer(0, numPts);
        for (int idx2 = 0; idx2 < numPts; ++idx2)
          {
          p[idx2] = static_cast<double>(idx);
          }
        pd->GetPointData()->SetScalars(partArray);
        partArray->FastDelete();
        }
      }
    this->DeleteInternalPipeline();
    vtkGarbageCollector::DeferredCollectionPop();
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
// the input is a hierarchy of vtkUniformGrid or one level of
// vtkRectilinearGrid. The output is a hierarchy of vtkPolyData.
void vtkExtractCTHPart::ExecutePart(const char *arrayName,
                                    int partIndex,
                                    vtkHierarchicalDataSet *input,
                                    vtkHierarchicalDataSet *output,
                                    int needPartIndex)
{
  int numberOfLevels=input->GetNumberOfLevels();
  
  output->SetNumberOfLevels(numberOfLevels);
  
  vtkPolyData *pd;
  int level=0;
  while(level<numberOfLevels)
    {
    int numberOfDataSets=input->GetNumberOfDataSets(level);
    output->SetNumberOfDataSets(level,numberOfDataSets);
    
    int dataset=0;
    while(dataset<numberOfDataSets)
      {
//      cout<<"level="<<level<<" dataset="<<dataset<<endl;
      vtkDataObject *dataObj=input->GetDataSet(level,dataset);
      if(dataObj!=0)// can be null if on another processor
        {
        pd=0;
        if(level==0)
          {
          vtkRectilinearGrid *rg=vtkRectilinearGrid::SafeDownCast(dataObj);
          if(rg!=0)
            {
            pd=vtkPolyData::New();
            output->SetDataSet(level,dataset,pd);
            pd->FastDelete();
            this->ExecutePartOnRectilinearGrid(arrayName,rg,pd);
            }
          else
            {
//            vtkImageData *ug=vtkImageData::SafeDownCast(dataObj);
            vtkUniformGrid *ug=vtkUniformGrid::SafeDownCast(dataObj);
            if(ug!=0)
              {
              pd=vtkPolyData::New();
              output->SetDataSet(level,dataset,pd);
              pd->FastDelete();
              this->ExecutePartOnUniformGrid(arrayName,ug,pd);
              }
            else
              {
              vtkErrorMacro(<<" cannot handle a block of this type.");
              }
            }
          }
        else
          {
//          vtkImageData *ug=vtkImageData::SafeDownCast(dataObj);
          vtkUniformGrid *ug=vtkUniformGrid::SafeDownCast(dataObj);
          if(ug!=0)
            {
            pd=vtkPolyData::New();
            output->SetDataSet(level,dataset,pd);
            pd->FastDelete();
            this->ExecutePartOnUniformGrid(arrayName,ug,pd);
            }
          else
            {
            vtkErrorMacro(<<" cannot handle a block of this type.");
            }
          }
        if(pd!=0 && needPartIndex) // no error
          {
          // Add scalars to color this part.
          int numPts = pd->GetNumberOfPoints();
          vtkDoubleArray *partArray = vtkDoubleArray::New();
          partArray->SetName("Part Index");
          double *p = partArray->WritePointer(0, numPts);
          for (int idx2 = 0; idx2 < numPts; ++idx2)
            {
            p[idx2] = static_cast<double>(partIndex);
            }
          pd->GetPointData()->SetScalars(partArray);
          partArray->FastDelete();
          }
        }
      ++dataset;
      }
    ++level;
    }
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::ExecutePartOnUniformGrid(const char *arrayName,
//                                                 vtkImageData *input,
                                                 vtkUniformGrid *input,
                                                 
                                                 vtkPolyData *output)
{
  
  vtkPolyData* tmp;
  vtkDataArray* cellVolumeFraction;
  int* dims;

  vtkTimerLog::MarkStartEvent("Execute Part");

  // First things first.
  // Convert Cell data array to point data array.
  // Pass cell data.
  this->Data->CopyStructure(input);

  // Do not pass cell volume fraction data.
  this->Data->GetCellData()->CopyFieldOff(arrayName);
  vtkDataArray* scalars = input->GetCellData()->GetScalars();
  if (scalars && strcmp(arrayName, scalars->GetName()) == 0)
    { // I do not know why the reader sets attributes, but ....
    this->Data->GetCellData()->CopyScalarsOff();
    }

  this->Data->GetCellData()->PassData(input->GetCellData());
  // Only convert single volume fraction array to point data.
  // Other attributes will have to be viewed as cell data.
  cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  if (cellVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find cell array " << arrayName);
    return;
    }
  if (cellVolumeFraction->GetDataType() != VTK_DOUBLE &&
      cellVolumeFraction->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro("Expecting volume fraction to be of type float or double.");
    return;
    }

  dims = input->GetDimensions();
  this->PointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                   this->PointVolumeFraction, dims);

  this->Data->GetPointData()->SetScalars(this->PointVolumeFraction);

  // Create the contour surface.
  vtkTimerLog::MarkStartEvent("CTH Contour");
  this->Contour->Update();
  vtkTimerLog::MarkEndEvent("CTH Contour");

  // Create the capping surface for the contour and append.
  tmp = this->Surface->GetOutput();
#if 1
  vtkTimerLog::MarkStartEvent("Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("surface");

  // Clip surface less than volume fraction 0.5.
  tmp = this->Clip0->GetOutput();
  vtkTimerLog::MarkStartEvent("Clip Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("Clip Surface");

  vtkTimerLog::MarkStartEvent("Append");
  this->Append1->Update();
  vtkTimerLog::MarkEndEvent("Append");

  tmp = this->Append1->GetOutput();
  
  if (this->ClipPlane)
    {
    this->Append2->Update();
    tmp = this->Append2->GetOutput();
    }
  output->CopyStructure(tmp);
  output->GetCellData()->PassData(tmp->GetCellData());

  // Get rid of extra ghost levels.
  output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);

  // Add a name for this part.
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
  sprintf(str, "%s", arrayName);
  output->GetFieldData()->AddArray(nameArray);
  nameArray->FastDelete();
#endif
  vtkTimerLog::MarkEndEvent("Execute Part");
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::CreateInternalPipeline()
{
  // Objects common to both pipelines
  this->PointVolumeFraction=vtkDoubleArray::New();
  
  // Uniform grid case pipeline
  
 this->Data = vtkUniformGrid::New();
//  this->Data = vtkImageData::New();
 
  this->Contour=vtkContourFilter::New();
  this->Contour->SetInput(this->Data);
  this->Contour->SetValue(0, 0.5);
  
  this->Append1 = vtkAppendPolyData::New();
  this->Append1->AddInput(this->Contour->GetOutput());
  this->Surface=vtkDataSetSurfaceFilter::New();
  this->Surface->SetInput(this->Data);
  
  // Clip surface less than volume fraction 0.5.
  this->Clip0 = vtkClipPolyData::New();
  this->Clip0->SetInput(this->Surface->GetOutput());
  this->Clip0->SetValue(0.5);
  this->Append1->AddInput(this->Clip0->GetOutput());
  
  if(this->ClipPlane)
    {
    // We need to append iso and capped surfaces.
    this->Append2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    this->Clip1=vtkClipPolyData::New();
    this->Clip1->SetInput(this->Append1->GetOutput());
    this->Clip1->SetClipFunction(this->ClipPlane);
    this->Append2->AddInput(this->Clip1->GetOutput());
    
    // We need to create a capping surface.
    this->Cut = vtkCutter::New();
    this->Cut->SetInput(this->Data);
    this->Cut->SetCutFunction(this->ClipPlane);
    this->Cut->SetValue(0, 0.0);
    this->Clip2 = vtkClipPolyData::New();
    this->Clip2->SetInput(this->Cut->GetOutput());
    this->Clip2->SetValue(0.5);
    this->Append2->AddInput(this->Clip2->GetOutput());
    }
  
  // Rectilinear grid case pipeline
  
  this->RData = vtkRectilinearGrid::New();
  
  this->RContour=vtkContourFilter::New();
  this->RContour->SetInput(this->RData);
  this->RContour->SetValue(0, 0.5);
  
  this->RAppend1 = vtkAppendPolyData::New();
  this->RAppend1->AddInput(this->RContour->GetOutput());
  this->RSurface=vtkDataSetSurfaceFilter::New();
  this->RSurface->SetInput(this->RData);
  
  // Clip surface less than volume fraction 0.5.
  this->RClip0 = vtkClipPolyData::New();
  this->RClip0->SetInput(this->RSurface->GetOutput());
  this->RClip0->SetValue(0.5);
  this->RAppend1->AddInput(this->RClip0->GetOutput());
  
  if(this->ClipPlane)
    {
    // We need to append iso and capped surfaces.
    this->RAppend2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    this->RClip1=vtkClipPolyData::New();
    this->RClip1->SetInput(this->RAppend1->GetOutput());
    this->RClip1->SetClipFunction(this->ClipPlane);
    this->RAppend2->AddInput(this->RClip1->GetOutput());
    
    // We need to create a capping surface.
    this->RCut = vtkCutter::New();
    this->RCut->SetInput(this->RData);
    this->RCut->SetCutFunction(this->ClipPlane);
    this->RCut->SetValue(0, 0.0);
    this->RClip2 = vtkClipPolyData::New();
    this->RClip2->SetInput(this->RCut->GetOutput());
    this->RClip2->SetValue(0.5);
    this->RAppend2->AddInput(this->RClip2->GetOutput());
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
  if(this->Append1!=0)
    {
    this->Append1->Delete();
    this->Append1=0;
    }
  if(this->Surface!=0)
    {
    this->Surface->Delete();
    this->Surface=0;
    }
   if(this->Clip0!=0)
    {
    this->Clip0->Delete();
    this->Clip0=0;
    }
   if(this->Append1!=0)
    {
    this->Append1->Delete();
    this->Append1=0;
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
  if(this->RAppend1!=0)
    {
    this->RAppend1->Delete();
    this->RAppend1=0;
    }
  if(this->RSurface!=0)
    {
    this->RSurface->Delete();
    this->RSurface=0;
    }
   if(this->RClip0!=0)
    {
    this->RClip0->Delete();
    this->RClip0=0;
    }
   if(this->RAppend1!=0)
    {
    this->RAppend1->Delete();
    this->RAppend1=0;
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
void vtkExtractCTHPart::ExecutePartOnRectilinearGrid(const char *arrayName,
                                                     vtkRectilinearGrid *input,
                                                     vtkPolyData *output)
{
  vtkPolyData* tmp;
  vtkDataArray* cellVolumeFraction;
  int *dims;

  vtkTimerLog::MarkStartEvent("Execute Part");

  // First things first.
  // Convert Cell data array to point data array.
  // Pass cell data.
  this->RData->CopyStructure(input);

  // Do not pass cell volume fraction data.
  this->RData->GetCellData()->CopyFieldOff(arrayName);
  vtkDataArray* scalars = input->GetCellData()->GetScalars();
  if (scalars && strcmp(arrayName, scalars->GetName()) == 0)
    { // I do not know why the reader sets attributes, but ....
    this->RData->GetCellData()->CopyScalarsOff();
    }

  this->RData->GetCellData()->PassData(input->GetCellData());
  // Only convert single volume fraction array to point data.
  // Other attributes will have to be viewed as cell data.
  cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  if (cellVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find cell array " << arrayName);
    return;
    }
  if (cellVolumeFraction->GetDataType() != VTK_DOUBLE &&
      cellVolumeFraction->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro("Expecting volume fraction to be of type double.");
    return;
    }
  dims = input->GetDimensions();
  this->PointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                   this->PointVolumeFraction, dims);
  this->RData->GetPointData()->SetScalars(this->PointVolumeFraction);

  // Create the contour surface.
  vtkTimerLog::MarkStartEvent("CTH Contour");
  this->RContour->Update();
  vtkTimerLog::MarkEndEvent("CTH Contour");

  // Create the capping surface for the contour and append.
  tmp = this->RSurface->GetOutput();

  vtkTimerLog::MarkStartEvent("Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("surface");

  // Clip surface less than volume fraction 0.5.
  tmp = this->RClip0->GetOutput();
  vtkTimerLog::MarkStartEvent("Clip Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("Clip Surface");
  
  vtkTimerLog::MarkStartEvent("Append");
  this->RAppend1->Update();
  vtkTimerLog::MarkEndEvent("Append");

  tmp = this->RAppend1->GetOutput();
  
  if (this->ClipPlane)
    {
    // We need to append iso and capped surfaces.
    // Clip the volume fraction iso surface.
    // We need to create a capping surface.
    this->RAppend2->Update();
    tmp = this->RAppend2->GetOutput();
    }

  output->CopyStructure(tmp);
  output->GetCellData()->PassData(tmp->GetCellData());

  // Get rid of extra ghost levels.
  output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);

  // Add a name for this part.
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
  sprintf(str, "%s", arrayName);
  output->GetFieldData()->AddArray(nameArray);
  nameArray->FastDelete();
  vtkTimerLog::MarkEndEvent("Execute Part");
}

//-----------------------------------------------------------------------------
void vtkExtractCTHPart::ExecuteCellDataToPointData(
  vtkDataArray *cellVolumeFraction, 
  vtkDoubleArray *pointVolumeFraction,
  int *dims)
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
  // Increments are for the point array.
  jInc = dims[0];
  kInc = (dims[1]) * jInc;
  
  pPoint = pointVolumeFraction->GetPointer(0);
//  pCell = static_cast<double*>(cellVolumeFraction->GetVoidPointer(0));

  // Initialize the point data to 0.
  memset(pPoint, 0,  dims[0]*dims[1]*dims[2]*sizeof(double));

  int index=0;
  // Loop thorugh the cells.
  for (k = 0; k < kEnd; ++k)
    {
    for (j = 0; j < jEnd; ++j)
      {
      for (i = 0; i < iEnd; ++i)
        {
        // Add cell value to all points of cell.
        double value=cellVolumeFraction->GetTuple1(index);
        *pPoint += value;
        pPoint[1] += value;
        pPoint[jInc] += value;
        pPoint[1+jInc] += value;
        pPoint[kInc] += value;
        pPoint[kInc+1] += value;
        pPoint[kInc+jInc] += value;
        pPoint[kInc+jInc+1] += value;

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
  for (k = 0; k <= kEnd; ++k)
    {
    // Just a fancy fast way to compute the number of cell neighbors of a
    // point.
    if (k == 1)
      {
      count = count << 1;
      }
    if (k == kEnd)
      {
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
  vtkstd::vector<vtkstd::string>::iterator it;
  for ( it = this->Internals->VolumeArrayNames.begin();
    it != this->Internals->VolumeArrayNames.end();
    ++ it )
    {
    os << i2 << it->c_str() << endl;
    }
  if (this->ClipPlane)
    {
    os << indent << "ClipPlane:\n";
    this->ClipPlane->PrintSelf(os, indent.GetNextIndent());
    }
  else  
    {
    os << indent << "ClipPlane: NULL\n";
    }
}

