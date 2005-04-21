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
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkTimerLog.h"

#ifdef VTK_USE_PATENTED
#include "vtkKitwareCutter.h"
#endif

#include <math.h>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkExtractCTHPart, "1.4");
vtkStandardNewMacro(vtkExtractCTHPart);
vtkCxxSetObjectMacro(vtkExtractCTHPart,ClipPlane,vtkPlane);

//------------------------------------------------------------------------------
//==============================================================================
class vtkExtractCTHPartInternal
{
public:
  vtkstd::vector<vtkstd::string> VolumeArrayNames;

};
//==============================================================================
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
vtkExtractCTHPart::vtkExtractCTHPart()
{
  this->Internals = new vtkExtractCTHPartInternal;
  this->ClipPlane = 0;
}

//------------------------------------------------------------------------------
vtkExtractCTHPart::~vtkExtractCTHPart()
{
  this->SetClipPlane(NULL);
  delete this->Internals;
  this->Internals = 0;
}

//------------------------------------------------------------------------------
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



//--------------------------------------------------------------------------
void vtkExtractCTHPart::RemoveAllVolumeArrayNames()
{
  int num, idx;

  num = this->GetNumberOfOutputs();
  for (idx = 0; idx < num; ++idx)
    {
    this->SetOutput(idx, NULL);
    }

  this->Internals->VolumeArrayNames.erase(
    this->Internals->VolumeArrayNames.begin(),
    this->Internals->VolumeArrayNames.end());
}

//--------------------------------------------------------------------------
void vtkExtractCTHPart::AddVolumeArrayName(char* arrayName)
{
  if ( !arrayName )
    {
    return;
    }
  vtkPolyData* d = vtkPolyData::New();
  int num = this->GetNumberOfOutputs();
  this->Internals->VolumeArrayNames.push_back(arrayName);
  this->SetOutput(num, d);
  d->Delete();
  d = NULL;
}

//--------------------------------------------------------------------------
int vtkExtractCTHPart::GetNumberOfVolumeArrayNames()
{
  return this->Internals->VolumeArrayNames.size();
}

//--------------------------------------------------------------------------
const char* vtkExtractCTHPart::GetVolumeArrayName(int idx)
{
  if ( idx < 0 || idx > static_cast<int>(this->Internals->VolumeArrayNames.size()) )
    {
    return 0;
    }
  return this->Internals->VolumeArrayNames[idx].c_str();
}

//--------------------------------------------------------------------------
void vtkExtractCTHPart::SetOutput(int idx, vtkPolyData* d)
{
  this->vtkSource::SetNthOutput(idx, d);  
}

//----------------------------------------------------------------------------
vtkPolyData* vtkExtractCTHPart::GetOutput(int idx)
{
  return (vtkPolyData *) this->vtkSource::GetOutput(idx); 
}

//----------------------------------------------------------------------------
int vtkExtractCTHPart::GetNumberOfOutputs()
{
  return this->GetNumberOfVolumeArrayNames();
}


//--------------------------------------------------------------------------
void vtkExtractCTHPart::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataSet *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }

  input->SetUpdateExtent(piece, numPieces, ghostLevel+1);
  // Force input to clip.
  // We could deal with larger extents if we create a 
  // rectilinear grid synchronized templates.
  input->RequestExactExtentOn();
}


//------------------------------------------------------------------------------
void vtkExtractCTHPart::Execute()
{
  int idx, num;
  int idx2, numPts;
  const char* arrayName;
  vtkPolyData* output;


  num = this->GetNumberOfVolumeArrayNames();
  for (idx = 0; idx < num; ++idx)
    {
    arrayName = this->GetVolumeArrayName(idx);
    output = this->GetOutput(idx);
    this->ExecutePart(arrayName, output);

    // In the future we might be able to select the rgb color here.
    if (num > 1)
      {
      // Add scalars to color this part.
      numPts = output->GetNumberOfPoints();
      vtkFloatArray *partArray = vtkFloatArray::New();
      partArray->SetName("Part Index");
      float *p = partArray->WritePointer(0, numPts);
      for (idx2 = 0; idx2 < numPts; ++idx2)
        {
        p[idx2] = (float)(idx);
        }
      output->GetPointData()->SetScalars(partArray);
      partArray->Delete();
      }
    } 
}

//------------------------------------------------------------------------------
void vtkExtractCTHPart::ExecutePart(const char* arrayName, vtkPolyData* output)
{
  vtkRectilinearGrid* input = this->GetInput();
  vtkRectilinearGrid* data = vtkRectilinearGrid::New();
  vtkPolyData* tmp;
  vtkDataArray* cellVolumeFraction;
  vtkFloatArray* pointVolumeFraction;
  vtkClipPolyData *clip0;
  vtkDataSetSurfaceFilter *surface;
  vtkAppendPolyData *append1;
  vtkAppendPolyData *append2 = NULL;
  int* dims;

  vtkTimerLog::MarkStartEvent("Execute Part");

  // First things first.
  // Convert Cell data array to point data array.
  // Pass cell data.
  data->CopyStructure(input);

  // Do not pass cell volume fraction data.
  data->GetCellData()->CopyFieldOff(arrayName);
  vtkDataArray* scalars = input->GetCellData()->GetScalars();
  if (scalars && strcmp(arrayName, scalars->GetName()) == 0)
    { // I do not know why the reader sets attributes, but ....
    data->GetCellData()->CopyScalarsOff();
    }

  data->GetCellData()->PassData(input->GetCellData());
  // Only convert single volume fraction array to point data.
  // Other attributes will have to be viewed as cell data.
  cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  if (cellVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find cell array " << arrayName);
    data->Delete();
    return;
    }
  if (cellVolumeFraction->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro("Expecting volume fraction to be of type float.");
    data->Delete();
    return;
    }
  pointVolumeFraction = vtkFloatArray::New();
  dims = input->GetDimensions();
  pointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                   pointVolumeFraction, dims);
  data->GetPointData()->SetScalars(pointVolumeFraction);

  // Create the contour surface.
  vtkContourFilter *contour = vtkContourFilter::New();
  contour->SetInput(data);
  contour->SetValue(0, 0.5);

  vtkTimerLog::MarkStartEvent("CTH Contour");
  contour->Update();
  vtkTimerLog::MarkEndEvent("CTH Contour");

  // Create the capping surface for the contour and append.
  append1 = vtkAppendPolyData::New();
  append1->AddInput(contour->GetOutput());
  surface = vtkDataSetSurfaceFilter::New();
  surface->SetInput(data);
  tmp = surface->GetOutput();

  vtkTimerLog::MarkStartEvent("Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("surface");

  // Clip surface less than volume fraction 0.5.
  clip0 = vtkClipPolyData::New();
  clip0->SetInput(surface->GetOutput());
  clip0->SetValue(0.5);
  tmp = clip0->GetOutput();
  vtkTimerLog::MarkStartEvent("Clip Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("Clip Surface");
  append1->AddInput(clip0->GetOutput());

  vtkTimerLog::MarkStartEvent("Append");
  append1->Update();
  vtkTimerLog::MarkEndEvent("Append");

  tmp = append1->GetOutput();
  
  if (this->ClipPlane)
    {
    vtkClipPolyData *clip1, *clip2;
    // We need to append iso and capped surfaces.
    append2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    clip1 = vtkClipPolyData::New();
    clip1->SetInput(tmp);
    clip1->SetClipFunction(this->ClipPlane);
    append2->AddInput(clip1->GetOutput());
    // We need to create a capping surface.
#ifdef VTK_USE_PATENTED
    vtkCutter *cut = vtkKitwareCutter::New();
#else
    vtkCutter *cut = vtkCutter::New();
#endif
    cut->SetInput(data);
    cut->SetCutFunction(this->ClipPlane);
    cut->SetValue(0, 0.0);
    clip2 = vtkClipPolyData::New();
    clip2->SetInput(cut->GetOutput());
    clip2->SetValue(0.5);
    append2->AddInput(clip2->GetOutput());
    append2->Update();
    tmp = append2->GetOutput();
    clip1->Delete();
    clip1 = NULL;
    cut->Delete();
    cut = NULL;
    clip2->Delete();
    clip2 = NULL;
    }

  output->CopyStructure(tmp);
  output->GetCellData()->PassData(tmp->GetCellData());

  // Get rid of extra ghost levels.
  output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);

  data->Delete();
  contour->Delete();
  surface->Delete();
  clip0->Delete();
  append1->Delete();
  if (append2)
    {
    append2->Delete();
    }
  pointVolumeFraction->Delete();

  // Add a name for this part.
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
  sprintf(str, "%s", arrayName);
  output->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();
  vtkTimerLog::MarkEndEvent("Execute Part");
}

//------------------------------------------------------------------------------
void vtkExtractCTHPart::ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, int *dims)
{
  int count;
  int i, j, k;
  int iEnd, jEnd, kEnd;
  int jInc, kInc;
  float *pPoint;
  float *pCell;

  pointVolumeFraction->SetName(cellVolumeFraction->GetName());

  iEnd = dims[0]-1;
  jEnd = dims[1]-1;
  kEnd = dims[2]-1;
  // Increments are for the point array.
  jInc = dims[0];
  kInc = (dims[1]) * jInc;
  
  pPoint = pointVolumeFraction->GetPointer(0);
  pCell = (float*)(cellVolumeFraction->GetVoidPointer(0));

  // Initialize the point data to 0.
  memset(pPoint, 0,  dims[0]*dims[1]*dims[2]*sizeof(float));

  // Loop thorugh the cells.
  for (k = 0; k < kEnd; ++k)
    {
    for (j = 0; j < jEnd; ++j)
      {
      for (i = 0; i < iEnd; ++i)
        {
        // Add cell value to all points of cell.
        *pPoint += *pCell;
        pPoint[1] += *pCell;
        pPoint[jInc] += *pCell;
        pPoint[1+jInc] += *pCell;
        pPoint[kInc] += *pCell;
        pPoint[kInc+1] += *pCell;
        pPoint[kInc+jInc] += *pCell;
        pPoint[kInc+jInc+1] += *pCell;

        // Increment pointers
        ++pPoint;
        ++pCell;
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
    // Just a fancy fast way to compute the number of cell neighbors of a point.
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
      // Just a fancy fast way to compute the number of cell neighbors of a point.
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
        // Just a fancy fast way to compute the number of cell neighbors of a point.
        if (i == 1)
          {
          count = count << 1;
          }
        if (i == iEnd)
          {
          count = count >> 1;
          }
        *pPoint = *pPoint / (float)(count);
        ++pPoint;
        }
      }
    }
}


//------------------------------------------------------------------------------
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

