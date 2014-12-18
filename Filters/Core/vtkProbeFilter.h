/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProbeFilter - sample data values at specified point locations
// .SECTION Description
// vtkProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at specified point positions. The filter has two inputs:
// the Input and Source. The Input geometric structure is passed through the
// filter. The point attributes are computed at the Input point positions
// by interpolating into the source data. For example, we can compute data
// values on a plane (plane specified as Input) from a volume (Source).
// The cell data of the source data is copied to the output based on in
// which source cell each input point is. If an array of the same name exists
// both in source's point and cell data, only the one from the point data is
// probed.
//
// This filter can be used to resample data, or convert one dataset form into
// another. For example, an unstructured grid (vtkUnstructuredGrid) can be
// probed with a volume (three-dimensional vtkImageData), and then volume
// rendering techniques can be used to visualize the results. Another example:
// a line or curve can be used to probe data to produce x-y plots along
// that line or curve.

#ifndef vtkProbeFilter_h
#define vtkProbeFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"
#include "vtkDataSetAttributes.h" // needed for vtkDataSetAttributes::FieldList

class vtkIdTypeArray;
class vtkCharArray;
class vtkMaskPoints;

class VTKFILTERSCORE_EXPORT vtkProbeFilter : public vtkDataSetAlgorithm
{
public:
  static vtkProbeFilter *New();
  vtkTypeMacro(vtkProbeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  void SetSourceData(vtkDataObject *source);
  vtkDataObject *GetSource();

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used. New style. Equivalent to SetInputConnection(1, algOutput).
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
  // Get the list of point ids in the output that contain attribute data
  // interpolated from the source.
  vtkGetObjectMacro(ValidPoints, vtkIdTypeArray);

  // Description:
  // Returns the name of the char array added to the output with values 1 for
  // valid points and 0 for invalid points.
  // Set to "vtkValidPointMask" by default.
  vtkSetStringMacro(ValidPointMaskArrayName)
  vtkGetStringMacro(ValidPointMaskArrayName)

  // Description:
  // Shallow copy the input cell data arrays to the output.
  // Off by default.
  vtkSetMacro(PassCellArrays, int);
  vtkBooleanMacro(PassCellArrays, int);
  vtkGetMacro(PassCellArrays, int);
  // Description:
  // Shallow copy the input point data arrays to the output
  // Off by default.
  vtkSetMacro(PassPointArrays, int);
  vtkBooleanMacro(PassPointArrays, int);
  vtkGetMacro(PassPointArrays, int);


  // Description:
  // Set whether to pass the field-data arrays from the Input i.e. the input
  // providing the geometry to the output. On by default.
  vtkSetMacro(PassFieldArrays, int);
  vtkBooleanMacro(PassFieldArrays, int);
  vtkGetMacro(PassFieldArrays, int);

  // Description:
  // Set the tolerance used to compute whether a point in the
  // source is in a cell of the input.  This value is only used
  // if ComputeTolerance is off.
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  // Description:
  // Set whether to use the Tolerance field or precompute the tolerance.
  // When on, the tolerance will be computed and the field
  // value is ignored. Off by default.
  vtkSetMacro(ComputeTolerance, bool);
  vtkBooleanMacro(ComputeTolerance, bool);
  vtkGetMacro(ComputeTolerance, bool);

//BTX
protected:
  vtkProbeFilter();
  ~vtkProbeFilter();

  int PassCellArrays;
  int PassPointArrays;
  int PassFieldArrays;

  int SpatialMatch;

  double Tolerance;
  bool ComputeTolerance;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

  // Description:
  // Call at end of RequestData() to pass attribute data respecting the
  // PassCellArrays, PassPointArrays, PassFieldArrays flags.
  void PassAttributeData(
    vtkDataSet* input, vtkDataObject* source, vtkDataSet* output);

  // Description:
  // Equivalent to calling InitializeForProbing(); ProbeEmptyPoints().
  void Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output);

  // Description:
  // Build the field lists. This is required before calling
  // InitializeForProbing().
  void BuildFieldList(vtkDataSet* source);

  // Description:
  // Initializes output and various arrays which keep track for probing status.
  virtual void InitializeForProbing(vtkDataSet *input, vtkDataSet *output);

  // Description:
  // Probe only those points that are marked as not-probed by the MaskPoints
  // array.
  // srcIdx is the index in the PointList for the given source.
  void ProbeEmptyPoints(vtkDataSet *input, int srcIdx, vtkDataSet *source,
    vtkDataSet *output);

  char* ValidPointMaskArrayName;
  vtkIdTypeArray *ValidPoints;
  vtkCharArray* MaskPoints;
  int NumberOfValidPoints;

  // Agreed, this is sort of a hack to allow subclasses to override the default
  // behavior of this filter to call NullPoint() for every point that is
  // not-a-hit when probing. This makes it possible for subclasses to initialize
  // the arrays with different defaults.
  bool UseNullPoint;

  vtkDataSetAttributes::FieldList* CellList;
  vtkDataSetAttributes::FieldList* PointList;
private:
  vtkProbeFilter(const vtkProbeFilter&);  // Not implemented.
  void operator=(const vtkProbeFilter&);  // Not implemented.

  class vtkVectorOfArrays;
  vtkVectorOfArrays* CellArrays;
//ETX
};

#endif
