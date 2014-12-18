/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogram2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkExtractHistogram2D - compute a 2D histogram between two columns
//  of an input vtkTable.
//
// .SECTION Description
//  This class computes a 2D histogram between two columns of an input
//  vtkTable. Just as with a 1D histogram, a 2D histogram breaks
//  up the input domain into bins, and each pair of values (row in
//  the table) fits into a single bin and increments a row counter
//  for that bin.
//
//  To use this class, set the input with a table and call AddColumnPair(nameX,nameY),
//  where nameX and nameY are the names of the two columns to be used.
//
//  In addition to the number of bins (in X and Y), the domain of
//  the histogram can be customized by toggling the UseCustomHistogramExtents
//  flag and setting the CustomHistogramExtents variable to the
//  desired value.
//
// .SECTION See Also
//  vtkPExtractHistogram2D
//
// .SECTION Thanks
//  Developed by David Feng and Philippe Pebay at Sandia National Laboratories
//------------------------------------------------------------------------------
#ifndef vtkExtractHistogram2D_h
#define vtkExtractHistogram2D_h

#include "vtkFiltersImagingModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

class vtkImageData;
class vtkIdTypeArray;
class vtkMultiBlockDataSet;

class VTKFILTERSIMAGING_EXPORT vtkExtractHistogram2D : public vtkStatisticsAlgorithm
{
public:
  static vtkExtractHistogram2D* New();
  vtkTypeMacro(vtkExtractHistogram2D, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum OutputIndices
  {
    HISTOGRAM_IMAGE=3
  };
//ETX

  // Description:
  // Set/get the number of bins to be used per dimension (x,y)
  vtkSetVector2Macro(NumberOfBins,int);
  vtkGetVector2Macro(NumberOfBins,int);

  // Description:
  // Set/get the components of the arrays in the two input columns
  // to be used during histogram computation.  Defaults to component 0.
  vtkSetVector2Macro(ComponentsToProcess,int);
  vtkGetVector2Macro(ComponentsToProcess,int);

  // Description:
  // Set/get a custom domain for histogram computation.  UseCustomHistogramExtents
  // must be called for these to actually be used.
  vtkSetVector4Macro(CustomHistogramExtents,double);
  vtkGetVector4Macro(CustomHistogramExtents,double);

  // Description:
  // Use the extents in CustomHistogramExtents when computing the
  // histogram, rather than the simple range of the input columns.
  vtkSetMacro(UseCustomHistogramExtents,int);
  vtkGetMacro(UseCustomHistogramExtents,int);
  vtkBooleanMacro(UseCustomHistogramExtents,int);

  // Description:
  // Control the scalar type of the output histogram.  If the input
  // is relatively small, you can save space by using a smaller
  // data type.  Defaults to unsigned integer.
  vtkSetMacro(ScalarType,int);
  void SetScalarTypeToUnsignedInt()
    {this->SetScalarType(VTK_UNSIGNED_INT);};
  void SetScalarTypeToUnsignedLong()
    {this->SetScalarType(VTK_UNSIGNED_LONG);};
  void SetScalarTypeToUnsignedShort()
    {this->SetScalarType(VTK_UNSIGNED_SHORT);};
  void SetScalarTypeToUnsignedChar()
    {this->SetScalarType(VTK_UNSIGNED_CHAR);};
  void SetScalarTypeToFloat()
    {this->SetScalarType(VTK_FLOAT);};
  void SetScalarTypeToDouble()
    {this->SetScalarType(VTK_DOUBLE);};
  vtkGetMacro(ScalarType,int);

  // Description:
  // Access the count of the histogram bin containing the largest number
  // of input rows.
  vtkGetMacro(MaximumBinCount,double);

  // Description:
  // Compute the range of the bin located at position (binX,binY) in
  // the 2D histogram.
  int GetBinRange(vtkIdType binX, vtkIdType binY, double range[4]);

  // Description:
  // Get the range of the of the bin located at 1D position index bin
  // in the 2D histogram array.
  int GetBinRange(vtkIdType bin, double range[4]);

  // Description:
  // Get the width of all of the bins. Also stored in the spacing
  // ivar of the histogram image output.
  void GetBinWidth(double bw[2]);

  // Description:
  // Gets the data object at the histogram image output port and
  // casts it to a vtkImageData.
  vtkImageData* GetOutputHistogramImage();

  // Description:
  // Get the histogram extents currently in use, either computed
  // or set by the user.
  double* GetHistogramExtents();

  vtkSetMacro(SwapColumns,int);
  vtkGetMacro(SwapColumns,int);
  vtkBooleanMacro(SwapColumns,int);

  // Description:
  // Get/Set an optional mask that can ignore rows of the table
  virtual void SetRowMask(vtkDataArray*);
  vtkGetObjectMacro(RowMask,vtkDataArray);

  // Description:
  // Given a collection of models, calculate aggregate model. Not used.
  virtual void Aggregate( vtkDataObjectCollection*, vtkMultiBlockDataSet* ) {}

protected:
  vtkExtractHistogram2D();
  ~vtkExtractHistogram2D();

  int SwapColumns;
  int NumberOfBins[2];
  double HistogramExtents[4];
  double CustomHistogramExtents[4];
  int UseCustomHistogramExtents;
  int ComponentsToProcess[2];
  double MaximumBinCount;
  int ScalarType;
  vtkDataArray* RowMask;

  virtual int ComputeBinExtents(vtkDataArray* col1, vtkDataArray* col2);

  // Description:
  // Execute the calculations required by the Learn option.
  // This is what actually does the histogram computation.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* inMeta );

  // Description:
  // Execute the calculations required by the Derive option. Not used.
  virtual void Derive( vtkMultiBlockDataSet* ) {}

  // Description:
  // Execute the calculations required by the Test option.
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* ) { return; };

  // Description:
  // Execute the calculations required by the Assess option.
  virtual void Assess( vtkTable*,
                       vtkMultiBlockDataSet*,
                       vtkTable* ) { return; };

  // Description:
  // Provide the appropriate assessment functor. Not used.
  virtual void SelectAssessFunctor( vtkTable* vtkNotUsed(outData),
                                    vtkDataObject* vtkNotUsed(inMeta),
                                    vtkStringArray* vtkNotUsed(rowNames),
                                    AssessFunctor*& vtkNotUsed(dfunc) ) {}

  virtual int FillOutputPortInformation( int port, vtkInformation* info );

  // Description:
  // Makes sure that the image data output port has up-to-date spacing/origin/etc
  virtual int RequestInformation (vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);

  // Description:
  // Get points to the arrays that live in the two input columns
  int GetInputArrays(vtkDataArray*& col1, vtkDataArray*& col2);
private:
  vtkExtractHistogram2D(const vtkExtractHistogram2D&); // Not implemented
  void operator=(const vtkExtractHistogram2D&);   // Not implemented
};

#endif
