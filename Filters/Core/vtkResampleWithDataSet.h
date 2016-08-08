/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResampleWithDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkResampleWithDataset - sample point and cell data of a dataset on
// points from another dataset.
// .SECTION Description
// Similar to vtkCompositeDataProbeFilter, vtkResampleWithDataset takes two
// inputs - Input and Source, and samples the point and cell values of Source
// on to the point locations of Input. The output has the same structure as
// Input but its point data have the resampled values from Source. Unlike
// vtkCompositeDataProbeFilter, this filter support composite datasets for both
// Input and Source.
// .SECTION See Also
// vtkCompositeDataProbeFilter vtkResampleToImage

#ifndef vtkResampleWithDataSet_h
#define vtkResampleWithDataSet_h


#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkNew.h" // For vtkCompositeDataProbeFilter member variable
#include "vtkPassInputTypeAlgorithm.h"

class vtkCompositeDataProbeFilter;
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkResampleWithDataSet : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkResampleWithDataSet, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkResampleWithDataSet *New();

  // Description:
  // Specify the data set that will be probed at the input points.
  // The Input gives the geometry (the points and cells) for the output,
  // while the Source is probed (interpolated) to generate the scalars,
  // vectors, etc. for the output points based on the point locations.
  void SetSourceData(vtkDataObject *source);

  // Description:
  // Specify the data set that will be probed at the input points.
  // The Input gives the geometry (the points and cells) for the output,
  // while the Source is probed (interpolated) to generate the scalars,
  // vectors, etc. for the output points based on the point locations.
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

protected:
  vtkResampleWithDataSet();
  ~vtkResampleWithDataSet();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  //virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
  //                               vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);
  virtual int FillOutputPortInformation(int, vtkInformation *);

  // Description:
  // Get the name of the valid-points mask array.
  const char* GetMaskArrayName() const;

  // Description:
  // Mark invalid points and cells of output DataSet as hidden
  void SetBlankPointsAndCells(vtkDataSet *data);

  vtkNew<vtkCompositeDataProbeFilter> Prober;

private:
  vtkResampleWithDataSet(const vtkResampleWithDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResampleWithDataSet&) VTK_DELETE_FUNCTION;
};

#endif // vtkResampleWithDataSet_h
