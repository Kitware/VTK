/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkImageBlockReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageBlockReader.h"
#include "vtkStructuredPointsReader.h"
#include "vtkImageTranslateExtent.h"

//----------------------------------------------------------------------------
vtkImageBlockReader::vtkImageBlockReader()
{
  int idx;

  this->FilePattern = NULL;
  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 1;
  this->Overlap = 0;

  for (idx = 0; idx < 6; ++idx)
    {
    this->WholeExtent[idx] = 0;
    }
  this->ScalarType = VTK_FLOAT;
  this->NumberOfScalarComponents = 1;

  this->XExtents = NULL;
  this->YExtents = NULL;
  this->ZExtents = NULL;
}

//----------------------------------------------------------------------------
vtkImageBlockReader::~vtkImageBlockReader()
{
  this->SetFilePattern(NULL);
  if (this->XExtents)
    {
    this->DeleteBlockExtents();
    }
}

//----------------------------------------------------------------------------
void vtkImageBlockReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  vtkImageSource::PrintSelf(os,indent);
  
  os << indent << "FilePattern: " << this->FilePattern << endl;
  os << indent << "Overlap: " << this->Overlap << endl;
  os << indent << "Divisions: " << this->Divisions[0] << ", "
     << this->Divisions[1] << ", " << this->Divisions[2] << endl;

  os << indent << "WholeExtent: (" << this->WholeExtent[0]
     << "," << this->WholeExtent[1];
  for (idx = 1; idx < 3; ++idx)
    {
    os << indent << ", " << this->WholeExtent[idx * 2]
       << "," << this->WholeExtent[idx*2 + 1];
    }

  os << indent << "NumberOfScalarComponents: " 
     << this->NumberOfScalarComponents << endl;
  os << indent << "ScalarType: " << this->ScalarType << endl;
}


//----------------------------------------------------------------------------
void vtkImageBlockReader::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();

  output->SetScalarType(this->ScalarType);
  output->SetWholeExtent(this->WholeExtent);
  output->SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}


//----------------------------------------------------------------------------
void vtkImageBlockReader::Execute(vtkImageData *data)
{ 
  int *ext = data->GetExtent();

  this->ComputeBlockExtents();
  this->Read(data, ext);
}

//----------------------------------------------------------------------------
void vtkImageBlockReader::Read(vtkImageData *data, int *ext)
{ 
  int extent[6];
  int best, size, start, end;
  int bMin, bMax;
  int i, xIdx, yIdx, zIdx;

  // choose the best block
  xIdx = yIdx = zIdx = -1;
  // X
  best = 0;
  for (i = 0; i < this->Divisions[0]; ++i)
    {
    // intersection
    bMin = this->XExtents[i*2];
    bMax = this->XExtents[i*2+1];
    if (bMin < ext[0]) { bMin = ext[0]; }
    if (bMax > ext[1]) { bMax = ext[1]; }
    // pick the block with the biggest overlap
    size = bMax - bMin + 1;
    if (size > best)
      {
      best = size;
      xIdx = i;
      extent[0] = bMin;
      extent[1] = bMax;
      }
    }
  // Y
  best = 0;
  for (i = 0; i < this->Divisions[1]; ++i)
    {
    // intersection
    bMin = this->YExtents[i*2];
    bMax = this->YExtents[i*2+1];
    if (bMin < ext[2]) { bMin = ext[2]; }
    if (bMax > ext[3]) { bMax = ext[3]; }
    // pick the block with the biggest overlap
    size = bMax - bMin + 1;
    if (size > best)
      {
      best = size;
      yIdx = i;
      extent[2] = bMin;
      extent[3] = bMax;
      }
    }
  // Z
  best = 0;
  for (i = 0; i < this->Divisions[2]; ++i)
    {
    // intersection
    bMin = this->ZExtents[i*2];
    bMax = this->ZExtents[i*2+1];
    if (bMin < ext[4]) { bMin = ext[4]; }
    if (bMax > ext[5]) { bMax = ext[5]; }
    // pick the block with the biggest overlap
    size = bMax - bMin + 1;
    if (size > best)
      {
      best = size;
      zIdx = i;
      extent[4] = bMin;
      extent[5] = bMax;
      }
    }


  if (xIdx == -1 || yIdx == -1 || zIdx == -1)
    {
    vtkErrorMacro("No overlap");
    return;
    }

  this->ReadBlock(xIdx, yIdx, zIdx, data, extent);
  this->ReadRemainder(data, ext, extent);
}

//----------------------------------------------------------------------------
// if ext is larger than doneExt, then ext-doneExt is read (even if it is
// a complex shape.)
void vtkImageBlockReader::ReadRemainder(vtkImageData *data, int *ext,
                                        int *doneExt)
{
  int newExt[6];

  memcpy(newExt, ext, 6*sizeof(int));

  // greedy: just pick the first we come to.
  // X
  if (newExt[0] < doneExt[0])
    {
    // set newExt to the next extent to read recursively.
    newExt[1] = doneExt[0]-1;
    this->Read(data, newExt);
    // now set the newExt back (minus the extent just read).
    newExt[0] = doneExt[0];
    newExt[1] = ext[1];
    }
  if (newExt[1] > doneExt[1])
    {
    // set newExt to the next extent to read recursively.
    newExt[0] = doneExt[1]+1;
    this->Read(data, newExt);
    // now set the newExt back (minus the extent just read).
    newExt[1] = doneExt[1];
    newExt[0] = doneExt[0];
    }
  // Y
  if (newExt[2] < doneExt[2])
    {
    // set newExt to the next extent to read recursively.
    newExt[3] = doneExt[2]-1;
    this->Read(data, newExt);
    // now set the newExt back (minus the extent just read).
    newExt[2] = doneExt[2];
    newExt[3] = ext[3];
    }
  if (newExt[3] > doneExt[3])
    {
    // set newExt to the next extent to read recursively.
    newExt[2] = doneExt[3]+1;
    this->Read(data, newExt);
    // now set the newExt back (minus the extent just read).
    newExt[3] = doneExt[3];
    newExt[2] = doneExt[2];
    }
  // Z
  if (newExt[4] < doneExt[4])
    {
    // set newExt to the next extent to read recursively.
    newExt[5] = doneExt[4]-1;
    this->Read(data, newExt);
    // now set the newExt back (minus the extent just read).
    newExt[4] = doneExt[4];
    newExt[5] = ext[5];
    }
  if (newExt[5] > doneExt[5])
    {
    // set newExt to the next extent to read recursively.
    newExt[4] = doneExt[5]+1;
    this->Read(data, newExt);
    // now set the newExt back (minus the extent just read).
    newExt[5] = doneExt[5];
    newExt[4] = doneExt[4];
    }
}


//----------------------------------------------------------------------------
// ext is completely contained in the block.
void vtkImageBlockReader::ReadBlock(int xIdx, int yIdx, int zIdx, 
                                    vtkImageData *data, int *ext)
{
  vtkStructuredPointsReader *reader = vtkStructuredPointsReader::New();
  vtkImageTranslateExtent *trans = vtkImageTranslateExtent::New();
  char *fileName;


  // Allocate a string for the filename.
  fileName = new char[strlen(this->FilePattern) + 50];
  sprintf(fileName, this->FilePattern, xIdx, yIdx, zIdx);
  reader->SetFileName(fileName);
  trans->SetInput(reader->GetOutput());
  trans->SetTranslation(this->XExtents[xIdx*2], this->YExtents[yIdx*2], 
                        this->ZExtents[zIdx*2]);
  trans->Update();

  vtkDebugMacro("reading block " << fileName
      << ": extent " << ext[0] << ", " << ext[1] << ", "
      << ext[2] << ", " << ext[3] << ", "
      << ext[4] << ", " << ext[5]);

  data->CopyAndCastFrom(trans->GetOutput(), ext);

  reader->Delete();
  trans->Delete();
  delete fileName;
}

//----------------------------------------------------------------------------
void vtkImageBlockReader::ComputeBlockExtents()
{
  int *wholeExt = this->WholeExtent;
  int i, temp;

  if (this->XExtents)
    {
    this->DeleteBlockExtents();
    }

  this->XExtents = new int[this->Divisions[0]*2];
  this->YExtents = new int[this->Divisions[1]*2];
  this->ZExtents = new int[this->Divisions[2]*2];

  // X
  for (i = 0; i < this->Divisions[0]; ++i)
    {
    temp = wholeExt[1] - wholeExt[0] + 1 
            + (this->Divisions[0]-1)*this->Overlap;
    this->XExtents[i*2] = wholeExt[0] + i*temp/this->Divisions[0]
            - i*this->Overlap;
    this->XExtents[i*2+1] = (wholeExt[0] + (i+1)*temp/this->Divisions[0]) - 1
            - i*this->Overlap;
    }
  // Y
  for (i = 0; i < this->Divisions[1]; ++i)
    {
    temp = wholeExt[3] - wholeExt[2] + 1 
            + (this->Divisions[1]-1)*this->Overlap;
    this->YExtents[i*2] = wholeExt[2] + i*temp/this->Divisions[1]
            - i*this->Overlap;
    this->YExtents[i*2+1] = (wholeExt[2] + (i+1)*temp/this->Divisions[1]) - 1
            - i*this->Overlap;
    }
  // Z
  for (i = 0; i < this->Divisions[2]; ++i)
    {
    temp = wholeExt[5] - wholeExt[4] + 1 
            + (this->Divisions[2]-1)*this->Overlap;
    this->ZExtents[i*2] = wholeExt[4] + i*temp/this->Divisions[2]
            - i*this->Overlap;
    this->ZExtents[i*2+1] = (wholeExt[4] + (i+1)*temp/this->Divisions[2]) - 1
            - i*this->Overlap;
    }

}

//----------------------------------------------------------------------------
void vtkImageBlockReader::DeleteBlockExtents()
{
  delete this->XExtents;
  this->XExtents = NULL;

  delete this->YExtents;
  this->YExtents = NULL;

  delete this->ZExtents;
  this->ZExtents = NULL;

}

//----------------------------------------------------------------------------
// Don't split up blocks.
void vtkImageBlockReader::ModifyOutputUpdateExtent()
{
  int updateExtent[6];
  int min, max, num;
  int i;

  this->ComputeBlockExtents();

  this->GetOutput()->GetUpdateExtent(updateExtent);
 
  // start with the smallest min, and largest max, 
  // and throw away as many blocks as possible.
  // X -------------
  num = this->Divisions[0];
  for (i = 0; i < num; ++i)
    {
    if (XExtents[2*i] <= updateExtent[0])
      {
      min = XExtents[2*i];
      }
    if (XExtents[(num-i)*2 - 1] >= updateExtent[1])
      {
      max = XExtents[(num-i)*2 - 1];
      }
    }
  updateExtent[0] = min;
  updateExtent[1] = max;
  // Y -------------
  num = this->Divisions[1];
  for (i = 0; i < num; ++i)
    {
    if (YExtents[2*i] <= updateExtent[2])
      {
      min = YExtents[2*i];
      }
    if (YExtents[(num-i)*2 - 1] >= updateExtent[3])
      {
      max = YExtents[(num-i)*2 - 1];
      }
    }
  updateExtent[2] = min;
  updateExtent[3] = max;
  // Z -------------
  num = this->Divisions[2];
  for (i = 0; i < num; ++i)
    {
    if (ZExtents[2*i] <= updateExtent[4])
      {
      min = ZExtents[2*i];
      }
    if (ZExtents[(num-i)*2 - 1] >= updateExtent[5])
      {
      max = ZExtents[(num-i)*2 - 1];
      }
    }
  updateExtent[4] = min;
  updateExtent[5] = max;

  this->GetOutput()->SetUpdateExtent(updateExtent);
}



