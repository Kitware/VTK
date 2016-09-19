/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHierarchicalBins.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractHierarchicalBins - manipulate the output of
// vtkHierarchicalBinningFilter

// .SECTION Description
// vtkExtractHierarchicalBins enables users to extract data from the output
// of vtkHierarchicalBinningFilter. Points at a particular level, or at a
// level and bin number, can be filtered to the output. To perform these
// operations, the output must contain points sorted into bins (the
// vtkPoints), with offsets pointing to the beginning of each bin (a
// vtkFieldData array named "BinOffsets").
//

// .SECTION Caveats
// This class has been threaded with vtkSMPTools. Using TBB or other
// non-sequential type (set in the CMake variable
// VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.

// .SECTION See Also
// vtkFiltersPointsFilter vtkRadiusOutlierRemoval vtkStatisticalOutlierRemoval
// vtkThresholdPoints vtkImplicitFunction vtkExtractGeoemtry
// vtkFitImplicitFunction

#ifndef vtkExtractHierarchicalBins_h
#define vtkExtractHierarchicalBins_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

class vtkHierarchicalBinningFilter;
class vtkPointSet;


class VTKFILTERSPOINTS_EXPORT vtkExtractHierarchicalBins : public vtkPointCloudFilter
{
public:
  // Description:
  // Standard methods for instantiating, obtaining type information, and
  // printing information.
  static vtkExtractHierarchicalBins *New();
  vtkTypeMacro(vtkExtractHierarchicalBins,vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the level to extract. If non-negative, with a negative bin
  // number, then all points at this level are extracted and sent to the
  // output. If negative, then the points from the specified bin are sent to
  // the output. If both the level and bin number are negative values, then the
  // input is sent to the output. By default the 0th level is extracted.
  vtkSetMacro(Level,int);
  vtkGetMacro(Level,int);

  // Description:
  // Specify the bin number to extract. If a non-negative value, then the
  // points from the bin number specified are extracted. If negative, then
  // entire levels of points are extacted (assuming the Level is
  // non-negative). Note that the bin tree is flattened, a particular
  // bin number may refer to a bin on any level.
  vtkSetMacro(Bin,int);
  vtkGetMacro(Bin,int);

  // Description:
  // Specify the vtkHierarchicalBinningFilter to query for relavant
  // information. Make sure that this filter has executed prior to the execution of
  // this filter. (This is generally a safe bet if connected in a pipeline.)
  virtual void SetBinningFilter(vtkHierarchicalBinningFilter*);
  vtkGetObjectMacro(BinningFilter,vtkHierarchicalBinningFilter);


protected:
  vtkExtractHierarchicalBins();
  ~vtkExtractHierarchicalBins();

  // Users can extract points from a particular level or bin.
  int Level;
  int Bin;
  vtkHierarchicalBinningFilter *BinningFilter;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  virtual int FilterPoints(vtkPointSet *input);

private:
  vtkExtractHierarchicalBins(const vtkExtractHierarchicalBins&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractHierarchicalBins&) VTK_DELETE_FUNCTION;

};

#endif
