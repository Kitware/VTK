/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageToImageFilter.h"

//----------------------------------------------------------------------------
vtkImageToImageFilter::vtkImageToImageFilter()
{
  this->Bypass = 0;
  this->BypassWasOn = 0;
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
}

//----------------------------------------------------------------------------
vtkImageToImageFilter::~vtkImageToImageFilter()
{
  this->Threader->Delete();
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Bypass: " << this->Bypass << "\n";
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";

  vtkImageSource::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToImageFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}



//----------------------------------------------------------------------------
// This method can be overriden in a subclass to compute the output
// Information: WholeExtent, Spacing, Origin, ScalarType and
// NumberOfScalarComponents.
void vtkImageToImageFilter::ExecuteInformation()
{
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();

  // Make sure the Input has been set.
  if ( input == NULL || output == NULL)
    {
    vtkErrorMacro(<< "UpdateInformation: Input is not set.");
    return;
    }

  // Set the dafaults.
  // Setting defaults will modify the data if the sublcass overrides the 
  // defaults.  But this should be OK because ExecuteTime (and UpdateTime)
  // should be out of date anyway if this method is being called.
  
  // Done in superclass now.
  //output->CopyInformation(input);

  if (this->Bypass == 0)
    {
    // for legacy
    this->LegacyHack = 1;
    this->ExecuteImageInformation();
    if (this->LegacyHack)
      {
      vtkWarningMacro("ExecuteImageInformation will not be supported in the future.\n"
        << "You should write an ExecuteInformation(vtkImageData*, vtkImageData*)");
      return;
      }
    
    this->ExecuteInformation(input, output);
    }
}
//----------------------------------------------------------------------------
void vtkImageToImageFilter::ExecuteInformation(
           vtkImageData *vtkNotUsed(inData), vtkImageData *vtkNotUsed(outData))
{
}


//----------------------------------------------------------------------------
int vtkImageToImageFilter::ComputeDivisionExtents(vtkDataObject *output,
						  int idx, int numDivisions)
{
  vtkImageData *input = this->GetInput();
  int actualSplits;
  int *outExt, inExt[6];
  
  if (input == NULL)
    {
    vtkErrorMacro("No input");
    return 0;
    }
  
  outExt = this->GetOutput()->GetUpdateExtent();
  actualSplits = this->SplitExtent(this->ExecuteExtent, outExt, 
				   idx, numDivisions);
  
  if (idx < actualSplits)
    { // yes this is a valid piece.
    this->ComputeRequiredInputUpdateExtent(inExt, this->ExecuteExtent);
    input->SetUpdateExtent(inExt);
    return 1;
    }
  else
    {
    // We could not split to this piece.
    return 0;
    }
}

//----------------------------------------------------------------------------
// This method can be overriden in a subclass to compute the input
// UpdateExtent needed to generate the output UpdateExtent.
// By default the input is set to the same as the output before this
// method is called.
void vtkImageToImageFilter::ComputeRequiredInputUpdateExtent(int inExt[6], 
						     int outExt[6])
{
  memcpy(inExt,outExt,sizeof(int)*6);
}





struct vtkImageThreadStruct
{
  vtkImageToImageFilter *Filter;
  vtkImageData   *Input;
  vtkImageData   *Output;
};



// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. Its just a pain to calculate
// the correct extent.
VTK_THREAD_RETURN_TYPE vtkImageThreadedExecute( void *arg )
{
  vtkImageThreadStruct *str;
  int ext[6], splitExt[6], total;
  int threadId, threadCount;
  
  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;

  str = (vtkImageThreadStruct *)(((ThreadInfoStruct *)(arg))->UserData);
  memcpy(ext,str->Filter->GetExecuteExtent(), sizeof(int)*6);

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, ext, threadId, threadCount);
  //total = 1;
  
  if (threadId < total)
    {
    str->Filter->ThreadedExecute(str->Input, str->Output, splitExt, threadId);
    }
  // else
  //   {
  //   otherwise don't use this thread. Sometimes the threads dont
  //   break up very well and it is just as efficient to leave a 
  //   few threads idle.
  //   }
  
  return VTK_THREAD_RETURN_VALUE;
}


void vtkImageToImageFilter::StreamExecuteStart()
{
  vtkImageData *output = this->GetOutput();

  // We need to be careful if Bypass has been toggled.
  if (this->Bypass == 0)
    {
    if (this->BypassWasOn)
      {
      // We were bypassing this filter (causing pointers to be copied from
      // input to output), now we are not bypassing the filter.  Need to reset
      // the output so we do not use the "copied" references.
      output->GetPointData()->Initialize();
      this->BypassWasOn = 0;
      }
    }
  else
    {
    this->BypassWasOn = 1;
    }

  //
  // Call vtkImageSource's StreamExecuteStart
  //
  this->vtkImageSource::StreamExecuteStart();
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
void vtkImageToImageFilter::Execute()
{
  if (this->Bypass == 0)
    {
    this->Execute(this->GetInput(), this->GetOutput());
    }
  else
    {
    vtkImageData *inData = this->GetInput();

    if ( inData == NULL)
      {
      vtkErrorMacro("No Input.");
      return;
      }    
    this->GetOutput()->GetPointData()->PassData(inData->GetPointData());
    this->GetOutput()->SetExtent( this->GetInput()->GetExtent() );
    }
}

//----------------------------------------------------------------------------
// This assumes that there is no overlap of pieces.
// Not a good assumption! OH Well....
int vtkImageToImageFilter::GetNumberOfStreamDivisions()
{
  vtkImageData *input = this->GetInput();
  float fraction;
  int *ext;
  int num;
  
  if (input == NULL)
    {
    return 1;
    }
  
  // What fraction of whole extent is the OUTPUT UpdateExtent.
  ext = this->GetOutput()->GetWholeExtent();
  fraction = (ext[1]-ext[0]+1) * (ext[3]-ext[2]+1) * (ext[5]-ext[4]+1);
  ext = this->GetOutput()->GetUpdateExtent();
  fraction = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1) / fraction;
  
  // Estimated memory size is of the WholeExtent
  num =(int)(fraction *
	     input->GetEstimatedWholeMemorySize() / input->GetMemoryLimit());
  
  if (num < 1)
    {
    return 1;
    }
  return num;
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::Execute(vtkImageData *inData, 
				    vtkImageData *outData)
{
  vtkImageThreadStruct str;
  
  str.Filter = this;
  str.Input = inData;
  str.Output = outData;
  
  this->Threader->SetNumberOfThreads(this->NumberOfThreads);
  
  // setup threading and the invoke threadedExecute
  this->Threader->SetSingleMethod(vtkImageThreadedExecute, &str);
  this->Threader->SingleMethodExecute();
}


//----------------------------------------------------------------------------
// The execute method created by the subclass.
void vtkImageToImageFilter::ThreadedExecute(vtkImageData *vtkNotUsed(inData), 
				     vtkImageData *vtkNotUsed(outData),
				     int extent[6], int vtkNotUsed(threadId))
{
  extent = extent;
  vtkErrorMacro("subclass should override this method!!!");
}

//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".  
// If 1 is returned, the extent cannot be split.
int vtkImageToImageFilter::SplitExtent(int splitExt[6], int startExt[6], 
				       int num, int total)
{
  int splitAxis;
  int min, max;

  vtkDebugMacro("SplitExtent: ( " << startExt[0] << ", " << startExt[1] << ", "
		<< startExt[2] << ", " << startExt[3] << ", "
		<< startExt[4] << ", " << startExt[5] << "), " 
		<< num << " of " << total);

  // start with same extent
  memcpy(splitExt, startExt, 6 * sizeof(int));

  splitAxis = 2;
  min = startExt[4];
  max = startExt[5];
  while (min == max)
    {
    splitAxis--;
    if (splitAxis < 0)
      { // cannot split
      vtkDebugMacro("  Cannot Split");
      return 1;
      }
    min = startExt[splitAxis*2];
    max = startExt[splitAxis*2+1];
    }

  // determine the actual number of pieces that will be generated
  int range = max - min + 1;
  int valuesPerThread = (int)ceil(range/(double)total);
  int maxThreadIdUsed = (int)ceil(range/(double)valuesPerThread) - 1;
  if (num < maxThreadIdUsed)
    {
    splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
    splitExt[splitAxis*2+1] = splitExt[splitAxis*2] + valuesPerThread - 1;
    }
  if (num == maxThreadIdUsed)
    {
    splitExt[splitAxis*2] = splitExt[splitAxis*2] + num*valuesPerThread;
    }
  
  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
		<< splitExt[2] << ", " << splitExt[3] << ", "
		<< splitExt[4] << ", " << splitExt[5] << ")");

  return maxThreadIdUsed + 1;
}





//----------------------------------------------------------------------------
void vtkImageToImageFilter::SetInputMemoryLimit(int limit)
{
  vtkImageData *input = this->GetInput();
  
  if ( input == NULL)
    {
    vtkErrorMacro("Input must be set before InputMemoryLimit.");
    }
  
  input->SetMemoryLimit(limit);
}

//----------------------------------------------------------------------------
long vtkImageToImageFilter::GetInputMemoryLimit()
{
  vtkImageData *input = this->GetInput();
  
  if ( input == NULL)
    {
    vtkErrorMacro("Input must be set before you can get InputMemoryLimit.");
    return 1000000;
    }
  
  return input->GetMemoryLimit();
}
