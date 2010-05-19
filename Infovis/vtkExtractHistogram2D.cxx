/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkExtractHistogram2D.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkExtractHistogram2D.h"
//------------------------------------------------------------------------------
#include "vtkArrayData.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageMedian3D.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedIntArray.h"
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkExtractHistogram2D);
vtkCxxSetObjectMacro(vtkExtractHistogram2D,RowMask,vtkDataArray);
//------------------------------------------------------------------------------
// Figure out which histogram bin a pair of values fit into
static inline int vtkExtractHistogram2DComputeBin(vtkIdType& bin1, 
                                                  vtkIdType& bin2, 
                                                  double v1, 
                                                  double v2, 
                                                  double *exts, 
                                                  int* nbins, 
                                                  double* bwi)
{
  // Make they fit within the extents
  if (v1 < exts[0] || v1 > exts[1] || v2 < exts[2] || v2 > exts[3])
    return 0;

  // as usual, boundary cases are annoying
  bin1 = (v1 == exts[1]) ? 
    nbins[0]-1 :
    static_cast<vtkIdType>(floor((v1-exts[0])*bwi[0]));

  bin2 = (v2 == exts[3]) ?
    nbins[1]-1 : 
    static_cast<vtkIdType>(floor((v2-exts[2])*bwi[1]));
  return 1;
}
//------------------------------------------------------------------------------
vtkExtractHistogram2D::vtkExtractHistogram2D()
{
  this->SetNumberOfOutputPorts(4);

  this->NumberOfBins[0] = 0;
  this->NumberOfBins[1] = 0;

  this->HistogramExtents[0] = 0.0;
  this->HistogramExtents[1] = 0.0;
  this->HistogramExtents[2] = 0.0;
  this->HistogramExtents[3] = 0.0;

  this->CustomHistogramExtents[0] = 0.0;
  this->CustomHistogramExtents[1] = 0.0;
  this->CustomHistogramExtents[2] = 0.0;
  this->CustomHistogramExtents[3] = 0.0;

  this->ComponentsToProcess[0] = 0;
  this->ComponentsToProcess[1] = 0;
  
  this->UseCustomHistogramExtents = 0;
  this->MaximumBinCount = 0;
  this->ScalarType = VTK_UNSIGNED_INT;
  
  this->SwapColumns = 0;
  
  this->RowMask = 0;
}
//------------------------------------------------------------------------------
vtkExtractHistogram2D::~vtkExtractHistogram2D()
{
  if (this->RowMask)
    this->RowMask->Delete();
}
//------------------------------------------------------------------------------
void vtkExtractHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ScalarType: " << this->ScalarType << endl;
  os << indent << "ComponentsToProcess: " << this->ComponentsToProcess[0] << ", " << this->ComponentsToProcess[1] << endl;
  os << indent << "UseCustomHistogramExtents: " << this->UseCustomHistogramExtents << endl;
  os << indent << "MaximumBinCount: " << this->MaximumBinCount << endl;
  os << indent << "SwapColumns: " << this->SwapColumns << endl;
  os << indent << "NumberOfBins: " << this->NumberOfBins[0] << ", " << this->NumberOfBins[1] << endl;
  os << indent << "CustomHistogramExtents: " << this->CustomHistogramExtents[0] << ", " << this->CustomHistogramExtents[1] << ", " << this->CustomHistogramExtents[2] << ", " << this->CustomHistogramExtents[3] << endl;
  os << indent << "RowMask: " << this->RowMask << endl;
}
//------------------------------------------------------------------------------
void vtkExtractHistogram2D::Learn(vtkTable *vtkNotUsed(inData), 
                                  vtkTable* vtkNotUsed(inParameters), 
                                  vtkDataObject *outMetaDO)
{

  vtkTable* outMeta = vtkTable::SafeDownCast( outMetaDO ); 
  vtkImageData* outImage = vtkImageData::SafeDownCast(
    this->GetOutputDataObject(vtkExtractHistogram2D::HISTOGRAM_IMAGE));

  if ( !outMeta ) 
    { 
    return; 
    } 

  if (this->NumberOfBins[0] == 0 || this->NumberOfBins[1] == 0)
    {
    vtkErrorMacro(<<"Error: histogram dimensions not set (use SetNumberOfBins).");
    return;
    }

  vtkDataArray* col1=NULL, *col2=NULL; 
  if (! this->GetInputArrays(col1,col2))
    {
    return;
    }

  this->ComputeBinExtents(col1,col2);
  


  int numValues = col1->GetNumberOfTuples();
  if (numValues != col2->GetNumberOfTuples())
    {
    vtkErrorMacro(<<"Error: columns must have same length.");
    return;
    }

  // compute the bin width
  double binWidth[2] = {0.0,0.0};
  this->GetBinWidth(binWidth);

  // allocate the output image
  // vtkImageData is already smart about allocating arrays, so we'll just
  // let it take care of that for us.
  outImage->Initialize();
  outImage->SetScalarType(this->ScalarType);
  outImage->SetExtent(0,this->NumberOfBins[0]-1,0,this->NumberOfBins[1]-1,0,0);
  outImage->SetSpacing(binWidth[0],binWidth[1],0.0);

  // allocate only if necessary
  if (!outImage->GetPointData() ||
      !outImage->GetPointData()->GetScalars() ||
      outImage->GetPointData()->GetScalars()->GetNumberOfTuples() != this->NumberOfBins[0]*this->NumberOfBins[1])
    {
    outImage->AllocateScalars();
    }

  outImage->GetPointData()->GetScalars()->FillComponent(0,0);
  outImage->GetPointData()->GetScalars()->SetName("histogram");

  vtkDataArray* histogram = outImage->GetPointData()->GetScalars();
  if (!histogram)
    {
    vtkErrorMacro(<<"Error: histogram array not allocated.");
    return;
    }

  vtkIdType bin1,bin2,idx;
  double v1,v2,ct;
  double bwi[2] = {1.0/binWidth[0],1.0/binWidth[1]};

  bool useRowMask = this->RowMask && 
    this->RowMask->GetNumberOfTuples() == col1->GetNumberOfTuples();

  // compute the histogram.  
  this->MaximumBinCount = 0;
  for (int i=0; i<numValues; i++)
    {
    v1 = col1->GetComponent(i,this->ComponentsToProcess[0]);
    v2 = col2->GetComponent(i,this->ComponentsToProcess[1]);

    if (useRowMask && !this->RowMask->GetComponent(i,0))
      continue;

    if (!::vtkExtractHistogram2DComputeBin(bin1,bin2,v1,v2,this->GetHistogramExtents(),this->NumberOfBins,bwi))
      continue;
   

    idx = (bin1 + this->NumberOfBins[0]*bin2);

    ct = histogram->GetComponent(idx,0)+1;
    histogram->SetComponent(idx,0,ct);

    if (ct > this->MaximumBinCount)
      {
      this->MaximumBinCount = static_cast<vtkIdType>(ct);
      }
    }

  outMeta->Initialize();
  outMeta->AddColumn(histogram);
}

//------------------------------------------------------------------------------
int vtkExtractHistogram2D::GetBinRange(vtkIdType binX, vtkIdType binY, double range[4])
{
  double binWidth[2] = {0.0,0.0};
  double *ext = this->GetHistogramExtents();
  this->GetBinWidth(binWidth);

  range[0] = ext[0] + binX*binWidth[0];
  range[1] = ext[0] + (binX+1)*binWidth[0];

  range[2] = ext[2] + binY*binWidth[1];
  range[3] = ext[2] + (binY+1)*binWidth[1];
  return 1;
}
//------------------------------------------------------------------------------
int vtkExtractHistogram2D::GetBinRange(vtkIdType bin, double range[4])
{
  vtkIdType binX = bin % this->NumberOfBins[0];
  vtkIdType binY = bin / this->NumberOfBins[0];
  return this->GetBinRange(binX,binY,range);
}
//------------------------------------------------------------------------------
vtkImageData* vtkExtractHistogram2D::GetOutputHistogramImage()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(vtkExtractHistogram2D::HISTOGRAM_IMAGE));//this->OutputImage;
}
//------------------------------------------------------------------------------
int vtkExtractHistogram2D::GetInputArrays(vtkDataArray*& col1, vtkDataArray*& col2)
{
  vtkTable* inData = vtkTable::SafeDownCast(this->GetInputDataObject(0,0));
  
  if (this->Internals->Requests.size() > 0)
    {
    vtkStdString colName;

    this->Internals->GetColumnForRequest( 0, (this->SwapColumns != 0), colName );
    col1 = vtkDataArray::SafeDownCast( inData->GetColumnByName( colName ) );

    this->Internals->GetColumnForRequest( 0, (this->SwapColumns == 0), colName );
    col2 = vtkDataArray::SafeDownCast( inData->GetColumnByName( colName ) );
    }
  else
    {
    col1 = vtkDataArray::SafeDownCast( inData->GetColumn( 0 ) );
    col2 = vtkDataArray::SafeDownCast( inData->GetColumn( 1 ) );
    }

  if (!col2)
    col2=col1;

  if (!col1)
    {
    vtkErrorMacro(<<"Error: could not find first column.");
    return 0;
    }

  if (!col2)
    {
    vtkErrorMacro(<<"Error: could not find second column.");
    return 0;
    }

  if (col1->GetNumberOfComponents() <= this->ComponentsToProcess[0])
    {
    vtkErrorMacro(<<"Error: first column doesn't contain component " << this->ComponentsToProcess[0] << ".");
    return 0;
    }

  if (col2->GetNumberOfComponents() <= this->ComponentsToProcess[1])
    {
    vtkErrorMacro(<<"Error: second column doesn't contain component " << this->ComponentsToProcess[1] << ".");
    return 0;
    }

  return 1;
}
//------------------------------------------------------------------------------
void vtkExtractHistogram2D::GetBinWidth(double bw[2])
{
  double* ext = this->GetHistogramExtents();
  bw[0] = (ext[1] - ext[0]) / static_cast<double>(this->NumberOfBins[0]);
  bw[1] = (ext[3] - ext[2]) / static_cast<double>(this->NumberOfBins[1]);
}
//------------------------------------------------------------------------------
double* vtkExtractHistogram2D::GetHistogramExtents()
{
  if (this->UseCustomHistogramExtents)
    return this->CustomHistogramExtents;
  else
    return this->HistogramExtents;
}
//------------------------------------------------------------------------------
int vtkExtractHistogram2D::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == vtkExtractHistogram2D::HISTOGRAM_IMAGE )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkImageData" );
    return 1;
    }
  else
    {
    return this->Superclass::FillOutputPortInformation(port,info);
    }
}

//------------------------------------------------------------------------------
// cribbed from vtkImageReader2
int vtkExtractHistogram2D::RequestInformation(vtkInformation *vtkNotUsed(request), 
                                              vtkInformationVector **vtkNotUsed(inputVector), 
                                              vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(vtkExtractHistogram2D::HISTOGRAM_IMAGE);

  vtkDataArray* col1=NULL, *col2=NULL; 
  if (! this->GetInputArrays(col1,col2))
    {
    return 0;
    }

  this->ComputeBinExtents(col1,col2);

  double bw[2] = {0,0};
  double* hext = this->GetHistogramExtents();
  this->GetBinWidth(bw);

  int ext[6] = {0,this->NumberOfBins[0]-1,0,this->NumberOfBins[1]-1,0,0};
  double sp[3]  = {bw[0],bw[1],0.0};
  double o[3] = {hext[0],hext[2],0.0};
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext, 6);
  outInfo->Set(vtkDataObject::SPACING(), sp, 3);
  outInfo->Set(vtkDataObject::ORIGIN(),  o, 3);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->ScalarType, 1);
  return 1;
}
//------------------------------------------------------------------------------
int vtkExtractHistogram2D::ComputeBinExtents(vtkDataArray* col1, vtkDataArray* col2)
{
  if (!col1 || !col2)
    return 0;

  // update histogram extents, if necessary
  if (!this->UseCustomHistogramExtents)
    {
    col1->GetRange(this->HistogramExtents,this->ComponentsToProcess[0]);
    col2->GetRange(this->HistogramExtents+2,this->ComponentsToProcess[1]);
    }

  return 1;
}
