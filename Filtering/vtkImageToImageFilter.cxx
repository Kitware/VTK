/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageToImageFilter.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"


//----------------------------------------------------------------------------
vtkImageToImageFilter::vtkImageToImageFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
  this->Bypass = 0;
  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
  this->InputScalarsSelection = NULL;
}

//----------------------------------------------------------------------------
vtkImageToImageFilter::~vtkImageToImageFilter()
{
  this->Threader->Delete();
  this->SetInputScalarsSelection(NULL);
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";

  if ( this->Bypass )
    {
    os << indent << "Bypass: On\n";
    }
  else
    {
    os << indent << "Bypass: Off\n";
    }
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToImageFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkImageData *>(this->Inputs[0]);
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
    if (output)
      {
      // this means that input is NULL, but the output isn't
      // in order to make this clear to filters down the line, we
      // make sure outputData is completely empty
      output->SetExtent(0, -1, 0, -1, 0, -1);
      output->SetWholeExtent(0, -1, 0, -1, 0, -1);
      output->SetUpdateExtent(0, -1, 0, -1, 0, -1);      
      output->AllocateScalars();
      }
    vtkErrorMacro(<< "ExecuteInformation: Input is not set.");
    return;
    }

  // Start with some defaults.
  output->CopyTypeSpecificInformation( input );

  // take this opportunity to modify the defaults
  this->ExecuteInformation(input, output);
}

//----------------------------------------------------------------------------
void vtkImageToImageFilter::ExecuteInformation(
           vtkImageData *vtkNotUsed(inData), vtkImageData *vtkNotUsed(outData))
{
}

//----------------------------------------------------------------------------

// Call the alternate version of this method, and use the returned input
// update extent for all inputs
void vtkImageToImageFilter::ComputeInputUpdateExtents( vtkDataObject *output )
{
  int outExt[6], inExt[6];

  output->GetUpdateExtent( outExt );

  if (this->NumberOfInputs)
    {
    this->ComputeInputUpdateExtent( inExt, outExt );
    }

  for (int idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      if (this->Inputs[idx]->GetRequestExactExtent())
        {
        int *currentExt = this->Inputs[idx]->GetUpdateExtent();
        for (int i = 0; i < 6; i += 2)
          {
          if (inExt[i] < currentExt[i] ||
              inExt[i+1] > currentExt[i+1])
            {
            this->Inputs[idx]->SetUpdateExtent( inExt );
            break;
            }
          }
        }
      else
        {
        this->Inputs[idx]->SetUpdateExtent( inExt );
        }
      }
    }  
}

// By default, simply set the input update extent to match the given output
// extent
void vtkImageToImageFilter::ComputeInputUpdateExtent( int inExt[6], 
                                                      int outExt[6] )
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
  vtkImageData *output;

  threadId = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->ThreadID;
  threadCount = static_cast<vtkMultiThreader::ThreadInfo *>(arg)->NumberOfThreads;

  str = static_cast<vtkImageThreadStruct *>(
    static_cast<vtkMultiThreader::ThreadInfo *>(arg)->UserData);
  output = str->Output;
  output->GetUpdateExtent( ext );

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
    --splitAxis;
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
  int valuesPerThread =
    static_cast<int>(ceil(range/static_cast<double>(total)));
  int maxThreadIdUsed = static_cast<int>(ceil(range/static_cast<double>(valuesPerThread))) - 1;
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
vtkImageData *vtkImageToImageFilter::AllocateOutputData(vtkDataObject *out)
{ 
  vtkImageData *output = vtkImageData::SafeDownCast(out);
  vtkImageData *input = this->GetInput();
  int inExt[6];
  int outExt[6];
  vtkDataArray *inArray;
  vtkDataArray *outArray;

  input->GetExtent(inExt);
  output->SetExtent(output->GetUpdateExtent());
  output->GetExtent(outExt);

  // Do not copy the array we will be generating.
  inArray = input->GetPointData()->GetScalars(this->InputScalarsSelection);

  // Conditionally copy point and cell data.
  // Only copy if corresponding indexes refer to identical points.
  double *oIn = input->GetOrigin();
  double *sIn = input->GetSpacing();
  double *oOut = output->GetOrigin();
  double *sOut = output->GetSpacing();
  if (oIn[0] == oOut[0] && oIn[1] == oOut[1] && oIn[2] == oOut[2] &&
      sIn[0] == sOut[0] && sIn[1] == sOut[1] && sIn[2] == sOut[2])   
    {
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();
    // Scalar copy flag trumps the array copy flag.
    if (inArray == input->GetPointData()->GetScalars())
      {
      output->GetPointData()->CopyScalarsOff();
      }
    else
      {
      output->GetPointData()->CopyFieldOff(this->InputScalarsSelection);
      }

    // If the extents are the same, then pass the attribute data for efficiency.
    if (inExt[0] == outExt[0] && inExt[1] == outExt[1] &&
        inExt[2] == outExt[2] && inExt[3] == outExt[3] &&
        inExt[4] == outExt[4] && inExt[5] == outExt[5])
      {// Pass
      output->CopyAttributes(input);
      }
    else
      {// Copy
       // Since this can be expensive to copy all of these values,
       // lets make sure there are arrays to copy (other than the scalars)
      if (input->GetPointData()->GetNumberOfArrays() > 1)
        {
        // Copy the point data.
        // CopyAllocate frees all arrays.
        // Keep the old scalar array (not being copied).
        // This is a hack, but avoids reallocation ...
        vtkDataArray *tmp = NULL;
        if ( ! output->GetPointData()->GetCopyScalars() )
          {
          tmp = output->GetPointData()->GetScalars();
          }
        output->GetPointData()->CopyAllocate(input->GetPointData(), 
                                             output->GetNumberOfPoints());
        if (tmp)
          { // Restore the array.
          output->GetPointData()->SetScalars(tmp);
          }
        // Now Copy The point data, but only if output is a subextent of the input.  
        if (outExt[0] >= inExt[0] && outExt[1] <= inExt[1] &&
            outExt[2] >= inExt[2] && outExt[3] <= inExt[3] &&
            outExt[4] >= inExt[4] && outExt[5] <= inExt[5])
          {
          output->GetPointData()->CopyStructuredData(input->GetPointData(),
                                                     inExt, outExt);
          }
        }

      if (input->GetCellData()->GetNumberOfArrays() > 0)
        {
        output->GetCellData()->CopyAllocate(input->GetCellData(), 
                                            output->GetNumberOfCells());  
        // Cell extent is one less than point extent.
        // Conditional to handle a colapsed axis (lower dimensional cells).
        if (inExt[0] < inExt[1]) {--inExt[1];}
        if (inExt[2] < inExt[3]) {--inExt[3];}
        if (inExt[4] < inExt[5]) {--inExt[5];}
        // Cell extent is one less than point extent.
        if (outExt[0] < outExt[1]) {--outExt[1];}
        if (outExt[2] < outExt[3]) {--outExt[3];}
        if (outExt[4] < outExt[5]) {--outExt[5];}
        // Now Copy The cell data, but only if output is a subextent of the input.  
        if (outExt[0] >= inExt[0] && outExt[1] <= inExt[1] &&
            outExt[2] >= inExt[2] && outExt[3] <= inExt[3] &&
            outExt[4] >= inExt[4] && outExt[5] <= inExt[5])
          {
          output->GetCellData()->CopyStructuredData(input->GetCellData(),
                                                    inExt, outExt);
          }
        }
      }
    }
  
  // Now create the scalars array that will hold the output data.
  this->ExecuteInformation();
  output->AllocateScalars();
  outArray = output->GetPointData()->GetScalars();
  if (inArray)
    {
    outArray->SetName(inArray->GetName());
    }
  return output;
}



//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
void vtkImageToImageFilter::ExecuteData(vtkDataObject *out)
{
  // Make sure the Input has been set.
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<< "ExecuteData: Input is not set.");
    return;
    }
    
  // Too many filters have floating point exceptions to execute
  // with empty input/ no request.
  if (this->UpdateExtentIsEmpty(out))
    {
    return;
    }

  vtkImageData *outData = this->AllocateOutputData(out);
  int debug = this->Debug;
  this->Debug = 0;
  this->MultiThread(this->GetInput(),outData);
  this->Debug = debug;
}

void vtkImageToImageFilter::MultiThread(vtkImageData *inData,
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
                                            int extent[6], int threadId)
{
  (void)extent;
  if (threadId == 0)
    {
    vtkErrorMacro("subclass should override ThreadedExecute!!!");
    }
}

//----------------------------------------------------------------------------
int vtkImageToImageFilter::FillInputPortInformation(int port,
                                                    vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
