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
/**
 * @class   vtkProbeFilter
 * @brief   sample data values at specified point locations
 *
 * vtkProbeFilter is a filter that computes point attributes (e.g., scalars,
 * vectors, etc.) at specified point positions. The filter has two inputs:
 * the Input and Source. The Input geometric structure is passed through the
 * filter. The point attributes are computed at the Input point positions
 * by interpolating into the source data. For example, we can compute data
 * values on a plane (plane specified as Input) from a volume (Source).
 * The cell data of the source data is copied to the output based on in
 * which source cell each input point is. If an array of the same name exists
 * both in source's point and cell data, only the one from the point data is
 * probed.
 *
 * This filter can be used to resample data, or convert one dataset form into
 * another. For example, an unstructured grid (vtkUnstructuredGrid) can be
 * probed with a volume (three-dimensional vtkImageData), and then volume
 * rendering techniques can be used to visualize the results. Another example:
 * a line or curve can be used to probe data to produce x-y plots along
 * that line or curve.
*/

#ifndef vtkProbeFilter_h
#define vtkProbeFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"
#include "vtkDataSetAttributes.h" // needed for vtkDataSetAttributes::FieldList

class vtkAbstractCellLocator;
class vtkCell;
class vtkCharArray;
class vtkIdTypeArray;
class vtkImageData;
class vtkPointData;
class vtkStaticCellLocator;

class VTKFILTERSCORE_EXPORT vtkProbeFilter : public vtkDataSetAlgorithm
{
public:
  static vtkProbeFilter *New();
  vtkTypeMacro(vtkProbeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(vtkDataObject *source);
  vtkDataObject *GetSource();
  //@}

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Control whether the source point data is to be treated as categorical. If
   * the data is categorical, then the resultant data will be determined by
   * a nearest neighbor interpolation scheme.
   */
  vtkSetMacro(CategoricalData,vtkTypeBool);
  vtkGetMacro(CategoricalData,vtkTypeBool);
  vtkBooleanMacro(CategoricalData,vtkTypeBool);
  //@}

  //@{
  /**
   * This flag is used only when a piece is requested to update.  By default
   * the flag is off.  Because no spatial correspondence between input pieces
   * and source pieces is known, all of the source has to be requested no
   * matter what piece of the output is requested.  When there is a spatial
   * correspondence, the user/application can set this flag.  This hint allows
   * the breakup of the probe operation to be much more efficient.  When piece
   * m of n is requested for update by the user, then only n of m needs to
   * be requested of the source.
   */
  vtkSetMacro(SpatialMatch, vtkTypeBool);
  vtkGetMacro(SpatialMatch, vtkTypeBool);
  vtkBooleanMacro(SpatialMatch, vtkTypeBool);
  //@}

  //@{
  /**
   * Get the list of point ids in the output that contain attribute data
   * interpolated from the source.
   */
  vtkIdTypeArray *GetValidPoints();
  //@}

  //@{
  /**
   * Returns the name of the char array added to the output with values 1 for
   * valid points and 0 for invalid points.
   * Set to "vtkValidPointMask" by default.
   */
  vtkSetStringMacro(ValidPointMaskArrayName)
  vtkGetStringMacro(ValidPointMaskArrayName)
  //@}

  //@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  vtkSetMacro(PassCellArrays, vtkTypeBool);
  vtkBooleanMacro(PassCellArrays, vtkTypeBool);
  vtkGetMacro(PassCellArrays, vtkTypeBool);
  //@}
  //@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  vtkSetMacro(PassPointArrays, vtkTypeBool);
  vtkBooleanMacro(PassPointArrays, vtkTypeBool);
  vtkGetMacro(PassPointArrays, vtkTypeBool);
  //@}


  //@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  vtkSetMacro(PassFieldArrays, vtkTypeBool);
  vtkBooleanMacro(PassFieldArrays, vtkTypeBool);
  vtkGetMacro(PassFieldArrays, vtkTypeBool);
  //@}

  //@{
  /**
   * Set the tolerance used to compute whether a point in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field
   * value is ignored. Off by default.
   */
  vtkSetMacro(ComputeTolerance, bool);
  vtkBooleanMacro(ComputeTolerance, bool);
  vtkGetMacro(ComputeTolerance, bool);
  //@}

  //@{
  /**
   * Set/Get the prototype cell locator to use for probing the source dataset.
   * By default, vtkStaticCellLocator will be used.
   */
   virtual void SetCellLocatorPrototype(vtkAbstractCellLocator*);
   vtkGetObjectMacro(CellLocatorPrototype, vtkAbstractCellLocator);
  //@}

protected:
  vtkProbeFilter();
  ~vtkProbeFilter() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;

  /**
   * Call at end of RequestData() to pass attribute data respecting the
   * PassCellArrays, PassPointArrays, PassFieldArrays flags.
   */
  void PassAttributeData(
    vtkDataSet* input, vtkDataObject* source, vtkDataSet* output);

  /**
   * Equivalent to calling BuildFieldList(); InitializeForProbing(); DoProbing().
   */
  void Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output);

  /**
   * Build the field lists. This is required before calling
   * InitializeForProbing().
   */
  void BuildFieldList(vtkDataSet* source);

  /**
   * Initializes output and various arrays which keep track for probing status.
   */
  virtual void InitializeForProbing(vtkDataSet *input, vtkDataSet *output);
  virtual void InitializeOutputArrays(vtkPointData *outPD, vtkIdType numPts);

  /**
   * Probe appropriate points
   * srcIdx is the index in the PointList for the given source.
   */
  void DoProbing(vtkDataSet *input, int srcIdx, vtkDataSet *source,
                 vtkDataSet *output);

  vtkTypeBool CategoricalData;

  vtkTypeBool PassCellArrays;
  vtkTypeBool PassPointArrays;
  vtkTypeBool PassFieldArrays;

  vtkTypeBool SpatialMatch;

  double Tolerance;
  bool ComputeTolerance;

  char* ValidPointMaskArrayName;
  vtkIdTypeArray *ValidPoints;
  vtkCharArray* MaskPoints;

  vtkAbstractCellLocator* CellLocatorPrototype;

  vtkDataSetAttributes::FieldList* CellList;
  vtkDataSetAttributes::FieldList* PointList;
private:
  vtkProbeFilter(const vtkProbeFilter&) = delete;
  void operator=(const vtkProbeFilter&) = delete;

  // Probe only those points that are marked as not-probed by the MaskPoints
  // array.
  void ProbeEmptyPoints(vtkDataSet *input, int srcIdx, vtkDataSet *source,
    vtkDataSet *output);

  // A faster implementation for vtkImageData input.
  void ProbePointsImageData(vtkImageData *input, int srcIdx, vtkDataSet *source,
    vtkImageData *output);
  void ProbeImagePointsInCell(vtkCell *cell, vtkIdType cellId, vtkDataSet *source,
    int srcBlockId, const double start[3], const double spacing[3],
    const int dim[3], vtkPointData *outPD, char *maskArray, double *wtsBuff);

  class ProbeImageDataWorklet;

  class vtkVectorOfArrays;
  vtkVectorOfArrays* CellArrays;
};

#endif
