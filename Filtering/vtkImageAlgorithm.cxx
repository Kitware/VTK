/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageAlgorithm.h"

#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkImageAlgorithm, "1.24");

//----------------------------------------------------------------------------
vtkImageAlgorithm::vtkImageAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkImageAlgorithm::~vtkImageAlgorithm()
{
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int vtkImageAlgorithm::RequestData(
  vtkInformation* request,
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector* outputVector)
{
  // the default implimentation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData

  // which output port did the request come from
  int outputPort = 
    request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());

  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
      {
      outputPort = 0;
      }
  
  // get the data object
  vtkInformation *outInfo = 
    outputVector->GetInformationObject(outputPort);
  // call ExecuteData
  if (outInfo)
    {
    this->ExecuteData( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
    }
  else
    {
    this->ExecuteData(NULL);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageAlgorithm::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->RequestData(request, inputVector, outputVector);
    return 1;
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
    }

  // propagate update extent
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    this->RequestUpdateExtent(request, inputVector, outputVector);
    return 1;
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Assume that any source that implements ExecuteData 
// can handle an empty extent.
void vtkImageAlgorithm::ExecuteData(vtkDataObject *)
{
  this->Execute();
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::Execute()
{
  vtkErrorMacro(<< "Definition of Execute() method should be in subclass and you should really use the ExecuteData(vtkInformation *request,...) signature instead");
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::CopyInputArrayAttributesToOutput
(vtkInformation* vtkNotUsed( request ),
 vtkInformationVector** inputVector,
 vtkInformationVector* outputVector)
{
  // for image data to image data
  if (this->GetNumberOfInputPorts() && this->GetNumberOfOutputPorts())
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    // if the input is image data
    if (vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
      {
      vtkInformation *info = 
        this->GetInputArrayFieldInformation(0, inputVector);
      if (info)
        {
        int scalarType = info->Get( vtkDataObject::FIELD_ARRAY_TYPE());
        int numComp = info->Get( vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
        for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
          {
          vtkInformation* outInfo = outputVector->GetInformationObject(i);
          // if the output is image data
          if (vtkImageData::SafeDownCast
              (outInfo->Get(vtkDataObject::DATA_OBJECT())))
            {
            // copy scalar type and scalar number of components
            vtkDataObject::SetPointDataActiveScalarInfo(outInfo, 
                                                        scalarType, numComp);
            }
          }
        }
      }
    }
}  

//----------------------------------------------------------------------------
int vtkImageAlgorithm::RequestInformation(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // do nothing except copy scalar type info
  this->CopyInputArrayAttributesToOutput(request,inputVector,outputVector);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageAlgorithm::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::AllocateOutputData(vtkImageData *output, 
                                           int *uExtent)
{ 
  // set the extent to be the update extent
  output->SetExtent(uExtent);
  output->AllocateScalars();
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageAlgorithm::AllocateOutputData(vtkDataObject *output)
{ 
  // set the extent to be the update extent
  vtkImageData *out = vtkImageData::SafeDownCast(output);
  if (out)
    {
    // this needs to be fixed -Ken
    vtkStreamingDemandDrivenPipeline *sddp = 
      vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
    if (sddp)
      {
      int extent[6];
      sddp->GetOutputInformation(0)->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),extent);
      out->SetExtent(extent);
      }
    out->AllocateScalars();
    }
  return out;
}

//----------------------------------------------------------------------------
// by default copy the attr from the first input to the first output
void vtkImageAlgorithm::CopyAttributeData(vtkImageData *input,
                                          vtkImageData *output,
                                          vtkInformationVector **inputVector)
{
  if (!input || !output)
    {
    return;
    }
  
  int inExt[6];
  int outExt[6];
  vtkDataArray *inArray;
  vtkDataArray *outArray;

  input->GetExtent(inExt);
  output->GetExtent(outExt);

  // Do not copy the array we will be generating.
  inArray = this->GetInputArrayToProcess(0,inputVector);

  // Conditionally copy point and cell data.  Only copy if corresponding
  // indexes refer to identical points.
  double *oIn = input->GetOrigin();
  double *sIn = input->GetSpacing();
  double *oOut = output->GetOrigin();
  double *sOut = output->GetSpacing();
  if (oIn[0] == oOut[0] && oIn[1] == oOut[1] && oIn[2] == oOut[2] &&
      sIn[0] == sOut[0] && sIn[1] == sOut[1] && sIn[2] == sOut[2])   
    {
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();
    output->GetPointData()->CopyScalarsOff();

    // If the extents are the same, then pass the attribute data for
    // efficiency.
    if (inExt[0] == outExt[0] && inExt[1] == outExt[1] &&
        inExt[2] == outExt[2] && inExt[3] == outExt[3] &&
        inExt[4] == outExt[4] && inExt[5] == outExt[5])
      {// Pass
      // set the name of the output to match the input name
      outArray = output->GetPointData()->GetScalars();
      if (inArray)
        {
        outArray->SetName(inArray->GetName());
        }
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
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
          // set the name of the output to match the input name
          if (inArray)
            {
            tmp->SetName(inArray->GetName());
            }
          }
        output->GetPointData()->CopyAllocate(input->GetPointData(), 
                                             output->GetNumberOfPoints());
        if (tmp)
          { // Restore the array.
          output->GetPointData()->SetScalars(tmp);
          }
        // Now Copy The point data, but only if output is a subextent of the
        // input.
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
}


//----------------------------------------------------------------------------
vtkImageData* vtkImageAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageAlgorithm::GetOutput(int port)
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
int vtkImageAlgorithm::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageAlgorithm::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::SetInput(vtkDataObject* input)
{
  this->SetInput(0, input);
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::SetInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->SetInputConnection(index, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(index, 0);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkImageAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
    {
    return 0;
    }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageAlgorithm::GetImageDataInput(int port)
{
  return vtkImageData::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

//----------------------------------------------------------------------------
void vtkImageAlgorithm::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->AddInputConnection(index, input->GetProducerPort());
    }
}
