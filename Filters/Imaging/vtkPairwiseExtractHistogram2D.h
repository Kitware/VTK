/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPairwiseExtractHistogram2D.h

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
// .NAME vtkPairwiseExtractHistogram2D - compute a 2D histogram between
//  all adjacent columns of an input vtkTable.
//
// .SECTION Description
//  This class computes a 2D histogram between all adjacent pairs of columns
//  of an input vtkTable. Internally it creates multiple vtkExtractHistogram2D
//  instances (one for each pair of adjacent table columns).  It also
//  manages updating histogram computations intelligently, only recomputing
//  those histograms for whom a relevant property has been altered.
//
//  Note that there are two different outputs from this filter.  One is a
//  table for which each column contains a flattened 2D histogram array.
//  The other is a vtkMultiBlockDataSet for which each block is a
//  vtkImageData representation of the 2D histogram.
//
// .SECTION See Also
//  vtkExtractHistogram2D vtkPPairwiseExtractHistogram2D
//
// .SECTION Thanks
//  Developed by David Feng and Philippe Pebay at Sandia National Laboratories
//------------------------------------------------------------------------------
#ifndef __vtkPairwiseExtractHistogram2D_h
#define __vtkPairwiseExtractHistogram2D_h

#include "vtkFiltersImagingModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"
#include "vtkSmartPointer.h"  //needed for smart pointer ivars
class vtkCollection;
class vtkExtractHistogram2D;
class vtkImageData;
class vtkIdTypeArray;
class vtkMultiBlockDataSet;

class VTKFILTERSIMAGING_EXPORT vtkPairwiseExtractHistogram2D : public vtkStatisticsAlgorithm
{
public:
  static vtkPairwiseExtractHistogram2D* New();
  vtkTypeMacro(vtkPairwiseExtractHistogram2D, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the bin dimensions of the histograms to compute
  vtkSetVector2Macro(NumberOfBins,int);
  vtkGetVector2Macro(NumberOfBins,int);

  // Description:
  // Strange method for setting an index to be used for setting custom
  // column range. This was (probably) necessary to get this class
  // to interact with the ParaView client/server message passing interface.
  vtkSetMacro(CustomColumnRangeIndex,int);
  void SetCustomColumnRangeByIndex(double,double);

  // Description:
  // More standard way to set the custom range for a particular column.
  // This makes sure that only the affected histograms know that they
  // need to be updated.
  void SetCustomColumnRange(int col, double range[2]);
  void SetCustomColumnRange(int col, double rmin, double rmax);

  // Description:
  // Set the scalar type for each of the computed histograms.
  vtkSetMacro(ScalarType,int);
  void SetScalarTypeToUnsignedInt()
    {this->SetScalarType(VTK_UNSIGNED_INT);};
  void SetScalarTypeToUnsignedLong()
    {this->SetScalarType(VTK_UNSIGNED_LONG);};
  void SetScalarTypeToUnsignedShort()
    {this->SetScalarType(VTK_UNSIGNED_SHORT);};
  void SetScalarTypeToUnsignedChar()
    {this->SetScalarType(VTK_UNSIGNED_CHAR);};
  vtkGetMacro(ScalarType,int);

  // Description:
  // Get the maximum bin count for a single histogram
  double GetMaximumBinCount(int idx);

  // Description:
  // Get the maximum bin count over all histograms
  double GetMaximumBinCount();

  // Description:
  // Compute the range of the bin located at position (binX,binY) in
  // the 2D histogram at idx.
  int GetBinRange(int idx, vtkIdType binX, vtkIdType binY, double range[4]);

  // Description:
  // Get the range of the of the bin located at 1D position index bin
  // in the 2D histogram array at idx.
  int GetBinRange(int idx, vtkIdType bin, double range[4]);

  // Description:
  // Get the width of all of the bins. Also stored in the spacing
  // ivar of the histogram image output at idx.
  void GetBinWidth(int idx, double bw[2]);

  // Description:
  // Get the histogram extents currently in use, either computed
  // or set by the user for the idx'th histogram.
  double* GetHistogramExtents(int idx);

  // Description:
  // Get the vtkImageData output of the idx'th histogram filter
  vtkImageData* GetOutputHistogramImage(int idx);

  // Description:
  // Get a pointer to the idx'th histogram filter.
  vtkExtractHistogram2D* GetHistogramFilter(int idx);

//BTX
  enum OutputIndices
  {
    HISTOGRAM_IMAGE=3
  };
//ETX

  // Description:
  // Given a collection of models, calculate aggregate model.  Not used
  virtual void Aggregate( vtkDataObjectCollection*, vtkMultiBlockDataSet* ) {}

protected:
  vtkPairwiseExtractHistogram2D();
  ~vtkPairwiseExtractHistogram2D();

  int NumberOfBins[2];
  int ScalarType;
  int CustomColumnRangeIndex;

  //BTX
  vtkSmartPointer<vtkIdTypeArray> OutputOutlierIds;
  vtkSmartPointer<vtkCollection> HistogramFilters;
  class Internals;
  Internals* Implementation;
  //ETX

  // Description:
  // Execute the calculations required by the Learn option.
  // Does the actual histogram computation works.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

  // Description:
  // Execute the calculations required by the Derive option. Not used.
  virtual void Derive( vtkMultiBlockDataSet* ) {}

  // Description:
  // Execute the assess option. Not implemented.
  virtual void Assess( vtkTable*,
                       vtkMultiBlockDataSet*,
                       vtkTable* ) {}

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* ) { return; };

  // Description:
  // Provide the appropriate assessment functor.
  virtual void SelectAssessFunctor( vtkTable* vtkNotUsed(outData),
                                    vtkDataObject* vtkNotUsed(inMeta),
                                    vtkStringArray* vtkNotUsed(rowNames),
                                    AssessFunctor*& vtkNotUsed(dfunc) ) {}

  // Description:
  // Generate a new histogram filter
  virtual vtkExtractHistogram2D* NewHistogramFilter();

  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  vtkTimeStamp BuildTime;
private:
  vtkPairwiseExtractHistogram2D(const vtkPairwiseExtractHistogram2D&); // Not implemented
  void operator=(const vtkPairwiseExtractHistogram2D&);   // Not implemented
};

#endif
