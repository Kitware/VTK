/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPushImageReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPushImageReader.h"

#include "vtkByteSwap.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPushPipeline.h"

class vtkPIRIncrementSlice : public vtkCommand
{
public:
  static vtkPIRIncrementSlice *New() { return new vtkPIRIncrementSlice;}
  // when a push is received move to the next slice,
  // when the last slice is reached, invoke a 
  // EndOfData event
  virtual void Execute(vtkObject *caller, unsigned long, void *)
    {
      vtkPushImageReader *pir = vtkPushImageReader::SafeDownCast(caller);
      if (pir)
        {
        int n = pir->GetCurrentSlice();
        n = n+1;
        // On the last slice invoke an EndOfData event
        if (n == pir->GetDataExtent()[5])
          {
          pir->InvokeEvent(vtkCommand::EndOfDataEvent,NULL);
          }
        if (n > pir->GetDataExtent()[5])
          {
          n = pir->GetDataExtent()[4];
          }
        if (n < pir->GetDataExtent()[4])
          {
          n = pir->GetDataExtent()[4];
          }
        pir->SetCurrentSlice(n);
        }
    }
};

vtkCxxRevisionMacro(vtkPushImageReader, "1.10");
vtkStandardNewMacro(vtkPushImageReader);

vtkPushImageReader::vtkPushImageReader()
{
  this->CurrentSlice = -1;
  vtkPIRIncrementSlice *is = vtkPIRIncrementSlice::New();
  this->AddObserver(vtkCommand::NextDataEvent,is);
  is->Delete();
  this->PushPipeline = NULL;
}

vtkPushImageReader::~vtkPushImageReader()
{
  if (this->PushPipeline)
    {
    this->PushPipeline = NULL;
    }
}

void vtkPushImageReader::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  
  output->SetWholeExtent(this->DataExtent[0],this->DataExtent[1],
                         this->DataExtent[2],this->DataExtent[3], 0, 0);
  output->SetSpacing(this->DataSpacing);
  output->SetOrigin(this->DataOrigin);

  output->SetScalarType(this->DataScalarType);
  output->SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}



//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkPushImageReaderUpdate(vtkPushImageReader *self, 
                              vtkImageData *data,
                              OT *outPtr)
{
  int outIncr[3];
  OT *outPtr1, *outPtr2;
  long streamRead;
  int idx1, idx2, nComponents;
  int outExtent[6];
  unsigned long count = 0;
  unsigned long target;
  
  // Get the requested extents and increments
  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);
  nComponents = data->GetNumberOfScalarComponents();
  
  // length of a row, num pixels read at a time
  int pixelRead = outExtent[1] - outExtent[0] + 1; 
  streamRead = (long)(pixelRead*nComponents*sizeof(OT));  
  
  // create a buffer to hold a row of the data
  target = (unsigned long)((outExtent[3]-outExtent[2]+1)/50.0);
  target++;
  
  // read the data row by row
  if (self->GetFileDimensionality() == 3)
    {
    self->ComputeInternalFileName(0);
    if ( !self->OpenFile() )
      {
      return;
      }
    }
  outPtr2 = outPtr;
  int currSlice = self->GetCurrentSlice();
  for (idx2 = currSlice; idx2 <= currSlice; ++idx2)
    {
    if (self->GetFileDimensionality() == 2)
      {
      self->ComputeInternalFileName(idx2);
      if ( !self->OpenFile() )
        {
        return;
        }
      }
    outPtr1 = outPtr2;
    for (idx1 = outExtent[2]; 
         !self->AbortExecute && idx1 <= outExtent[3]; ++idx1)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;
      
      // seek to the correct row
      self->SeekFile(outExtent[0],idx1,idx2);
      // read the row.
      if ( !self->GetFile()->read((char *)outPtr1, streamRead))
        {
        vtkGenericWarningMacro("File operation failed. row = " << idx1
                               << ", Read = " << streamRead
                               << ", FilePos = " << static_cast<vtkIdType>(self->GetFile()->tellg()));
        return;
        }
      // handle swapping
      if (self->GetSwapBytes() && sizeof(OT) > 1)
        {
        vtkByteSwap::SwapVoidRange(outPtr1, pixelRead*nComponents, sizeof(OT));
        }
      outPtr1 += outIncr[1];
      }
    // move to the next image in the file and data
    outPtr2 += outIncr[2];
    }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkPushImageReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  
  void *ptr = NULL;
  int *ext;
  
  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return;
    }

  ext = data->GetExtent();

  data->GetPointData()->GetScalars()->SetName("ImageFile");

  vtkDebugMacro("Reading extent: " << ext[0] << ", " << ext[1] << ", " 
        << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]);
  
  this->ComputeDataIncrements();
  
  // Call the correct templated function for the output
  ptr = data->GetScalarPointer();
  switch (this->GetDataScalarType())
    {
    vtkTemplateMacro3(vtkPushImageReaderUpdate, this, data, (VTK_TT *)(ptr));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }   
}

void vtkPushImageReader::SetPushPipeline(vtkPushPipeline *pp)
{
  // not ref counted to avoid loops
  this->PushPipeline = pp;
}

  
void vtkPushImageReader::Push()
{
  if (!this->PushPipeline)
    {
    this->PushPipeline = vtkPushPipeline::New();
    }
  this->PushPipeline->Push(this);
}

void vtkPushImageReader::Run()
{
  if (!this->PushPipeline)
    {
    this->PushPipeline = vtkPushPipeline::New();
    }
  this->PushPipeline->Run(this);
}

void vtkPushImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "CurrentSlice: " << this->CurrentSlice << "\n";
  os << indent << "PushPipeline: " << this->PushPipeline << "\n";
}
