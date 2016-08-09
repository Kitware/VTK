/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBinCellDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBinCellDataFilter - bin source cell data into input cells.
// .SECTION Description
// vtkBinCellDataFilter takes a source mesh containing scalar cell data, an
// input mesh and a set of bin values and bins the source mesh's scalar cell
// data into the cells of the input mesh. The resulting output mesh is identical
// to the input mesh, with an additional cell data field, with tuple size equal
// to the number of bins + 1, that represents a histogram of the cell data
// values for all of the source cells whose centroid lie within the input cell.
//
// This filter is useful for analyzing the efficacy of an input mesh's ability
// to represent the cell data of the source mesh.

#ifndef vtkBinCellDataFilter_h
#define vtkBinCellDataFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"
#include "vtkDataSetAttributes.h" // needed for vtkDataSetAttributes::FieldList

#include "vtkContourValues.h" // Needed for inline methods

class vtkIdTypeArray;
class vtkCharArray;
class vtkMaskPoints;
class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkBinCellDataFilter : public vtkDataSetAlgorithm
{
public:
  typedef vtkContourValues vtkBinValues;

  vtkTypeMacro(vtkBinCellDataFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (VTK_DOUBLE_MIN, VTK_DOUBLE_MAX) and
  // a single bin.
  static vtkBinCellDataFilter *New();

  // Description:
  // Methods to set / get bin values.
  void SetValue(int i, double value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *binValues);
  void SetNumberOfBins(int numBins);
  int GetNumberOfBins();
  void GenerateValues(int numBins, double range[2]);
  void GenerateValues(int numBins, double rangeStart, double rangeEnd);

  // Description:
  // Specify the data set whose cells will be counted.
  // The Input gives the geometry (the points and cells) for the output,
  // while the Source is used to determine how many source cells lie within
  // each input cell.
  void SetSourceData(vtkDataObject *source);
  vtkDataObject *GetSource();

  // Description:
  // Specify the data set whose cells will be counted.
  // The Input gives the geometry (the points and cells) for the output,
  // while the Source is used to determine how many source cells lie within
  // each input cell.
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  // Description:
  // This flag is used only when a piece is requested to update.  By default
  // the flag is off.  Because no spatial correspondence between input pieces
  // and source pieces is known, all of the source has to be requested no
  // matter what piece of the output is requested.  When there is a spatial
  // correspondence, the user/application can set this flag.  This hint allows
  // the breakup of the probe operation to be much more efficient.  When piece
  // m of n is requested for update by the user, then only n of m needs to
  // be requested of the source.
  vtkSetMacro(SpatialMatch, int);
  vtkGetMacro(SpatialMatch, int);
  vtkBooleanMacro(SpatialMatch, int);

  // Description:
  // Returns the name of the id array added to the output that holds the count
  // of source cells whose centroids are within input cells.
  // Set to "CellCount" by default.
  vtkSetStringMacro(BinnedDataArrayName)
  vtkGetStringMacro(BinnedDataArrayName)

  // Description:
  // Set whether to store the number of nonzero bins for each cell.
  // On by default.
  vtkSetMacro(StoreNumberOfNonzeroBins, bool);
  vtkBooleanMacro(StoreNumberOfNonzeroBins, bool);
  vtkGetMacro(StoreNumberOfNonzeroBins, bool);

  // Description:
  // Returns the name of the id array added to the output that holds the number
  // of nonzero bins per cell.
  // Set to "NumberOfNonzeroBins" by default.
  vtkSetStringMacro(NumberOfNonzeroBinsArrayName)
  vtkGetStringMacro(NumberOfNonzeroBinsArrayName)

  // Description:
  // Set the tolerance used to compute whether a cell centroid in the
  // source is in a cell of the input.  This value is only used
  // if ComputeTolerance is off.
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  // Description:
  // Set whether to use the Tolerance field or precompute the tolerance.
  // When on, the tolerance will be computed and the field value is ignored.
  // Off by default.
  vtkSetMacro(ComputeTolerance, bool);
  vtkBooleanMacro(ComputeTolerance, bool);
  vtkGetMacro(ComputeTolerance, bool);

  // Description:
  // Set/get which component of the scalar array to bin; defaults to 0.
  vtkSetMacro(ArrayComponent,int);
  vtkGetMacro(ArrayComponent,int);

protected:
  vtkBinCellDataFilter();
  ~vtkBinCellDataFilter();

  int SpatialMatch;

  bool StoreNumberOfNonzeroBins;
  double Tolerance;
  bool ComputeTolerance;
  int ArrayComponent;

  vtkBinValues *BinValues;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  char* BinnedDataArrayName;
  char* NumberOfNonzeroBinsArrayName;

private:
  vtkBinCellDataFilter(const vtkBinCellDataFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBinCellDataFilter&) VTK_DELETE_FUNCTION;
};

// Description:
// Set a particular bin value at bin number i. The index i ranges
// between 0<=i<NumberOfBins.
inline void vtkBinCellDataFilter::SetValue(int i, double value)
{this->BinValues->SetValue(i,value);}

// Description:
// Get the ith bin value.
inline double vtkBinCellDataFilter::GetValue(int i)
{return this->BinValues->GetValue(i);}

// Description:
// Get a pointer to an array of bin values. There will be
// GetNumberOfBins() values in the list.
inline double *vtkBinCellDataFilter::GetValues()
{return this->BinValues->GetValues();}

// Description:
// Fill a supplied list with bin values. There will be
// GetNumberOfBins() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkBinCellDataFilter::GetValues(double *binValues)
{this->BinValues->GetValues(binValues);}

// Description:
// Set the number of bins to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkBinCellDataFilter::SetNumberOfBins(int number)
{this->BinValues->SetNumberOfContours(number);}

// Description:
// Get the number of bins in the list of bin values, not counting the overflow
// bin.
inline int vtkBinCellDataFilter::GetNumberOfBins()
{return this->BinValues->GetNumberOfContours();}

// Description:
// Generate numBins equally spaced bin values between specified
// range. Bin values will include min/max range values.
inline void vtkBinCellDataFilter::GenerateValues(int numBins, double range[2])
{this->BinValues->GenerateValues(numBins, range);}

// Description:
// Generate numBins equally spaced bin values between specified
// range. Bin values will include min/max range values.
inline void vtkBinCellDataFilter::GenerateValues(int numBins, double
                                                 rangeStart, double rangeEnd)
{this->BinValues->GenerateValues(numBins, rangeStart, rangeEnd);}

#endif
