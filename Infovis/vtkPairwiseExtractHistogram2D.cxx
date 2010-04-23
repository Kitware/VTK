/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkPairwiseExtractHistogram2D.cxx

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

#include "vtkPairwiseExtractHistogram2D.h"
//------------------------------------------------------------------------------
#include "vtkArrayData.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkStatisticsAlgorithmPrivate.h"
#include "vtkCollection.h"
#include "vtkExtractHistogram2D.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkImageMedian3D.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedIntArray.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()
//------------------------------------------------------------------------------
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtkstd/map>
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPairwiseExtractHistogram2D);
//------------------------------------------------------------------------------
class vtkPairwiseExtractHistogram2D::Internals
{
public:
  vtkstd::vector< vtkstd::pair< vtkStdString,vtkStdString > > ColumnPairs;
  vtkstd::map< vtkstd::string,bool > ColumnUsesCustomExtents;
  vtkstd::map< vtkstd::string,vtkstd::vector<double> > ColumnExtents;
};
//------------------------------------------------------------------------------
vtkPairwiseExtractHistogram2D::vtkPairwiseExtractHistogram2D()
{
  this->Implementation = new Internals;
  
  this->SetNumberOfOutputPorts(4);
  
  this->NumberOfBins[0] = 0;
  this->NumberOfBins[1] = 0;
  
  this->CustomColumnRangeIndex = -1;
  
  this->ScalarType = VTK_UNSIGNED_INT;
  this->HistogramFilters = vtkSmartPointer<vtkCollection>::New();
  this->BuildTime.Modified();
}
//------------------------------------------------------------------------------
vtkPairwiseExtractHistogram2D::~vtkPairwiseExtractHistogram2D()
{
  delete this->Implementation;
}
//------------------------------------------------------------------------------
void vtkPairwiseExtractHistogram2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "NumberOfBins: " << this->NumberOfBins[0] << ", " << this->NumberOfBins[1] << endl;
  os << "CustomColumnRangeIndex: " << this->CustomColumnRangeIndex << endl;
  os << "ScalarType: " << this->ScalarType << endl;
}
//------------------------------------------------------------------------------
void vtkPairwiseExtractHistogram2D::Learn(vtkTable *inData, 
                                          vtkTable* vtkNotUsed(inParameters), 
                                          vtkMultiBlockDataSet *outMeta)
{
  if ( ! outMeta ) 
    { 
    return; 
    } 
  
  if (this->NumberOfBins[0] == 0 || this->NumberOfBins[1] == 0)
    {
    vtkErrorMacro(<<"Error: histogram dimensions not set (use SetNumberOfBins).");
    return;
    }
  
  // The primary statistics table
  vtkTable* primaryTab = vtkTable::New();

  // if the number of columns in the input has changed, we'll need to do 
  // some reinitializing
  int numHistograms = inData->GetNumberOfColumns()-1;
  if (numHistograms != this->HistogramFilters->GetNumberOfItems())
    {
    // clear out the list of histogram filters
    for (int i=0; i<this->HistogramFilters->GetNumberOfItems(); i++)
      {
      this->HistogramFilters->GetItemAsObject(i)->Delete();
      }
    
    // clear out the internals
    this->HistogramFilters->RemoveAllItems();
    this->Implementation->ColumnPairs.clear();
    this->Implementation->ColumnUsesCustomExtents.clear();
    this->Implementation->ColumnExtents.clear();

    // Make a shallow copy of the input to be safely passed to internal
    // histogram filters.
    VTK_CREATE(vtkTable, inDataCopy);
    inDataCopy->ShallowCopy(inData);
    
    // fill it up with new histogram filters
    for (int i=0; i<numHistograms; i++)
      {
      vtkDataArray* col1 = vtkDataArray::SafeDownCast(inData->GetColumn(i));
      vtkDataArray* col2 = vtkDataArray::SafeDownCast(inData->GetColumn(i+1));
      
      if (!col1 || !col2)
        {
        vtkErrorMacro("All inputs must be numeric arrays.");
        return;
        }
      
      // create a new histogram filter
      vtkSmartPointer<vtkExtractHistogram2D> f;
      f.TakeReference(this->NewHistogramFilter());
      f->SetInput(inDataCopy);
      f->SetNumberOfBins(this->NumberOfBins);
      vtkstd::pair<vtkStdString,vtkStdString> colpair(inData->GetColumn(i)->GetName(),inData->GetColumn(i+1)->GetName());
      f->AddColumnPair(colpair.first.c_str(),colpair.second.c_str());
      f->SetSwapColumns(strcmp(colpair.first.c_str(),colpair.second.c_str())>=0);
      this->HistogramFilters->AddItem(f);
      
      // update the internals accordingly
      this->Implementation->ColumnPairs.push_back(colpair);
      this->Implementation->ColumnUsesCustomExtents[colpair.first.c_str()] = false;
      
      // compute the range of the the new columns, and update the internals
      double r[2] = {0,0};
      if (i == 0)
        {
        col1->GetRange(r,0);
        this->Implementation->ColumnExtents[colpair.first.c_str()].clear();
        this->Implementation->ColumnExtents[colpair.first.c_str()].push_back(r[0]);
        this->Implementation->ColumnExtents[colpair.first.c_str()].push_back(r[1]);
        }
      
      col2->GetRange(r,0);
      this->Implementation->ColumnExtents[colpair.second.c_str()].clear();
      this->Implementation->ColumnExtents[colpair.second.c_str()].push_back(r[0]);
      this->Implementation->ColumnExtents[colpair.second.c_str()].push_back(r[1]);
      }
    }
  
  // check the filters one by one and update them if necessary
  if (this->BuildTime < inData->GetMTime() ||
      this->BuildTime < this->GetMTime())
    {
    for (int i=0; i<numHistograms; i++)
      {
      
      vtkExtractHistogram2D* f = this->GetHistogramFilter(i);
      
      // if the column names have changed, that means we need to update
      vtkstd::pair<vtkStdString,vtkStdString> cols = this->Implementation->ColumnPairs[i];
      if (inData->GetColumn(i)->GetName() != cols.first ||
          inData->GetColumn(i+1)->GetName() != cols.second)
        {
        vtkstd::pair<vtkStdString,vtkStdString> newCols(inData->GetColumn(i)->GetName(),inData->GetColumn(i+1)->GetName());
        
        f->ResetRequests();
        f->AddColumnPair(newCols.first.c_str(),newCols.second.c_str());
        f->SetSwapColumns(strcmp(newCols.first.c_str(),newCols.second.c_str()) >= 0);
        f->Modified();
        
        this->Implementation->ColumnPairs[i] = newCols;
        }
      
      // if the filter extents have changed, that means we need to update
      vtkstd::pair<vtkStdString,vtkStdString> newCols(inData->GetColumn(i)->GetName(),inData->GetColumn(i+1)->GetName());
      if (this->Implementation->ColumnUsesCustomExtents[newCols.first.c_str()] || 
          this->Implementation->ColumnUsesCustomExtents[newCols.second.c_str()])
        {
        f->UseCustomHistogramExtentsOn();
        double *ext = f->GetCustomHistogramExtents();
        if (ext[0] != this->Implementation->ColumnExtents[newCols.first.c_str()][0] ||
            ext[1] != this->Implementation->ColumnExtents[newCols.first.c_str()][1] ||
            ext[2] != this->Implementation->ColumnExtents[newCols.second.c_str()][0] ||
            ext[3] != this->Implementation->ColumnExtents[newCols.second.c_str()][1])
          {
          f->SetCustomHistogramExtents(this->Implementation->ColumnExtents[newCols.first.c_str()][0],
                                       this->Implementation->ColumnExtents[newCols.first.c_str()][1],
                                       this->Implementation->ColumnExtents[newCols.second.c_str()][0],
                                       this->Implementation->ColumnExtents[newCols.second.c_str()][1]);
          }
        }
      else
        {
        f->UseCustomHistogramExtentsOff();
        }
      
      // if the number of bins has changed, that definitely means we need to update
      int* nbins = f->GetNumberOfBins();
      if (nbins[0] != this->NumberOfBins[0] ||
          nbins[1] != this->NumberOfBins[1])
        {
        f->SetNumberOfBins(this->NumberOfBins);
        }
      }
    }
  
  // update the filters as necessary
  for (int i=0; i<numHistograms; i++)
    {
    vtkExtractHistogram2D* f = this->GetHistogramFilter(i);
    if (f && (f->GetMTime() > this->BuildTime ||
              inData->GetColumn(i)->GetMTime() > this->BuildTime ||
              inData->GetColumn(i+1)->GetMTime() > this->BuildTime))
      {
      f->Update();
      }
    }
  
  // build the composite image data set
  vtkMultiBlockDataSet* outImages = vtkMultiBlockDataSet::SafeDownCast(
    this->GetOutputDataObject(vtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE));
  
  if (outImages)
    {
    outImages->SetNumberOfBlocks(numHistograms);
    for (int i=0; i<numHistograms; i++)
      {
      vtkExtractHistogram2D* f = this->GetHistogramFilter(i);
      if (f)
        {
        outImages->SetBlock(i,f->GetOutputHistogramImage());
        }
      }
    }
  
  // build the output table
  primaryTab->Initialize();
  for (int i=0; i<this->HistogramFilters->GetNumberOfItems(); i++)
    {
    vtkExtractHistogram2D* f = this->GetHistogramFilter(i);
    if (f)
      {
      if (f->GetMTime() > this->BuildTime)
        f->Update();
      primaryTab->AddColumn(f->GetOutput()->GetColumn(0));
      }
    }
  
  // Finally set first block of output meta port to primary statistics table
  outMeta->SetNumberOfBlocks( 1 );
  outMeta->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Primary Statistics" );
  outMeta->SetBlock( 0, primaryTab );

  // Clean up
  primaryTab->Delete();

  this->BuildTime.Modified();
}
//------------------------------------------------------------------------------
void vtkPairwiseExtractHistogram2D::SetCustomColumnRangeByIndex(double rmin, double rmax)
{
  this->SetCustomColumnRange(this->CustomColumnRangeIndex,rmin,rmax);
}
//------------------------------------------------------------------------------
void vtkPairwiseExtractHistogram2D::SetCustomColumnRange(int column, double rmin, double rmax)
{
  vtkTable* t = vtkTable::SafeDownCast(this->GetInputDataObject(0,0));
  if (t)
    {
    vtkAbstractArray* a = t->GetColumn(column);
    if (a)
      {
      this->Implementation->ColumnUsesCustomExtents[a->GetName()] = true;
      if (this->Implementation->ColumnExtents[a->GetName()].size() == 0)
        {
        this->Implementation->ColumnExtents[a->GetName()].push_back(rmin);
        this->Implementation->ColumnExtents[a->GetName()].push_back(rmax);
        }
      else
        {
        this->Implementation->ColumnExtents[a->GetName()][0] = rmin;
        this->Implementation->ColumnExtents[a->GetName()][1] = rmax;
        }
      this->Modified();
      }
    }
}
//------------------------------------------------------------------------------
void vtkPairwiseExtractHistogram2D::SetCustomColumnRange(int column, double range[2])
{
  this->SetCustomColumnRange(column,range[0],range[1]);
}
//------------------------------------------------------------------------------
int vtkPairwiseExtractHistogram2D::GetBinRange(int idx, vtkIdType binX, vtkIdType binY, double range[4])
{
  vtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
    {
    return f->GetBinRange(binX,binY,range);
    }
  else
    {
    return 0;
    }
}

int vtkPairwiseExtractHistogram2D::GetBinRange(int idx, vtkIdType bin, double range[4])
{
  vtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
    {
    return f->GetBinRange(bin,range);
    }
  else
    {
    return 0;
    }
}
//------------------------------------------------------------------------------
vtkExtractHistogram2D* vtkPairwiseExtractHistogram2D::GetHistogramFilter(int idx)
{
  return vtkExtractHistogram2D::SafeDownCast(this->HistogramFilters->GetItemAsObject(idx));
}
//------------------------------------------------------------------------------
vtkImageData* vtkPairwiseExtractHistogram2D::GetOutputHistogramImage(int idx)
{
  if (this->BuildTime < this->GetMTime() || 
      this->BuildTime < this->GetInputDataObject(0,0)->GetMTime())
    this->Update();
  
  vtkMultiBlockDataSet* mbds = vtkMultiBlockDataSet::SafeDownCast(
    this->GetOutputDataObject(vtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE));
  
  if (mbds)
    {
    return vtkImageData::SafeDownCast(mbds->GetBlock(idx));
    }
  return NULL;
}
//------------------------------------------------------------------------------
void vtkPairwiseExtractHistogram2D::GetBinWidth(int idx,double bw[2])
{
  vtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
    {
    f->GetBinWidth(bw);
    }
}
//------------------------------------------------------------------------------
double* vtkPairwiseExtractHistogram2D::GetHistogramExtents(int idx)
{
  vtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
    {
    return f->GetHistogramExtents();
    }
  else
    {
    return NULL;
    }
}
//------------------------------------------------------------------------------
vtkExtractHistogram2D* vtkPairwiseExtractHistogram2D::NewHistogramFilter()
{
  return vtkExtractHistogram2D::New();
}
//------------------------------------------------------------------------------
double vtkPairwiseExtractHistogram2D::GetMaximumBinCount(int idx)
{
  vtkExtractHistogram2D* f = this->GetHistogramFilter(idx);
  if (f)
    {
    return f->GetMaximumBinCount();
    }
  return -1;
}
//------------------------------------------------------------------------------
double vtkPairwiseExtractHistogram2D::GetMaximumBinCount()
{
  if( !this->GetInputDataObject(0,0) )
    return -1;

  if (this->BuildTime < this->GetMTime() || 
      this->BuildTime < this->GetInputDataObject(0,0)->GetMTime())
    this->Update();
  
  double maxcount = -1;
  for (int i=0; i<this->HistogramFilters->GetNumberOfItems(); i++)
    {
    vtkExtractHistogram2D* f = this->GetHistogramFilter(i);
    if (f)
      {
      maxcount = vtkstd::max(f->GetMaximumBinCount(),maxcount);
      }
    }
  return maxcount;
}
//------------------------------------------------------------------------------
int vtkPairwiseExtractHistogram2D::FillOutputPortInformation( int port, vtkInformation* info )
{
  if ( port == vtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE )
    {
    info->Set( vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
    return 1;
    }
  else
    {
    return this->Superclass::FillOutputPortInformation(port,info);
    }
}

