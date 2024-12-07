// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractCTHPart
 * @brief   Generates surface of a CTH volume fraction.
 *
 * vtkExtractCTHPart is a filter that is specialized for creating
 * visualizations for a CTH simulation. CTH datasets comprise of either
 * vtkNonOverlappingAMR or a multiblock of non-overlapping rectilinear grids
 * with cell-data. Certain cell-arrays in the dataset identify the fraction of
 * a particular material present in a given cell. This goal with this filter is
 * to extract a surface contour demarcating the surface where the volume
 * fraction for a particular material is equal to the user chosen value.
 *
 * To achieve that, this filter first converts the cell-data to point-data and
 * then simply apply vtkContourFilter filter to extract the contour.
 *
 * vtkExtractCTHPart also provides the user with an option to clip the resultant
 * contour using a vtkPlane. Internally, it uses vtkClipClosedSurface to clip
 * the contour using the vtkPlane provided.
 *
 * The output of this filter is a vtkMultiBlockDataSet with one block
 * corresponding to each volume-fraction array requested. Each block itself is a
 * vtkPolyData for the contour generated on the current process (which may be
 * null, for processes where no contour is generated).
 */

#ifndef vtkExtractCTHPart_h
#define vtkExtractCTHPart_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // for using smartpointer

VTK_ABI_NAMESPACE_BEGIN
class vtkAppendPolyData;
class vtkContourFilter;
class vtkDataArray;
class vtkDataSet;
class vtkDataSetSurfaceFilter;
class vtkDoubleArray;
class vtkExtractCTHPartInternal;
class vtkImageData;
class vtkCompositeDataSet;
class vtkMultiProcessController;
class vtkPlane;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkUniformGrid;
class vtkUnsignedCharArray;
class vtkUnstructuredGrid;
class vtkExtractCTHPartFragments;

// #define EXTRACT_USE_IMAGE_DATA 1

class VTKFILTERSPARALLEL_EXPORT vtkExtractCTHPart : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractCTHPart* New();
  vtkTypeMacro(vtkExtractCTHPart, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Select cell-data arrays (volume-fraction arrays) to contour with.
   */
  void AddVolumeArrayName(const char*);
  void RemoveVolumeArrayNames();
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);
  ///@}

  ///@{
  /**
   * Get/Set the parallel controller. By default, the value returned by
   * vtkMultiBlockDataSetAlgorithm::GetGlobalController() when the object is
   * instantiated is used.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * On by default, enables logic to cap the material volume.
   */
  vtkSetMacro(Capping, bool);
  vtkGetMacro(Capping, bool);
  vtkBooleanMacro(Capping, bool);
  ///@}

  ///@{
  /**
   * Triangulate results. When set to false, the internal cut and contour filters
   * are told not to triangulate results if possible. true by default.
   */
  vtkSetMacro(GenerateTriangles, bool);
  vtkGetMacro(GenerateTriangles, bool);
  vtkBooleanMacro(GenerateTriangles, bool);
  ///@}

  ///@{
  /**
   * Generate solid geometry as results instead of 2D contours.
   * When set to true, GenerateTriangles flag will be ignored.
   * False by default.
   */
  vtkSetMacro(GenerateSolidGeometry, bool);
  vtkGetMacro(GenerateSolidGeometry, bool);
  vtkBooleanMacro(GenerateSolidGeometry, bool);
  ///@}

  ///@{
  /**
   * When set to false, the output surfaces will not hide contours extracted from
   * ghost cells. This results in overlapping contours but overcomes holes.
   * Default is set to true.
   */
  vtkSetMacro(RemoveGhostCells, bool);
  vtkGetMacro(RemoveGhostCells, bool);
  vtkBooleanMacro(RemoveGhostCells, bool);
  ///@}

  ///@{
  /**
   * Set, get or manipulate the implicit clipping plane.
   */
  void SetClipPlane(vtkPlane* clipPlane);
  vtkGetObjectMacro(ClipPlane, vtkPlane);
  ///@}

  /**
   * Look at clip plane to compute MTime.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set and get the volume fraction surface value. This value should be
   * between 0 and 1
   */
  vtkSetClampMacro(VolumeFractionSurfaceValue, double, 0.0, 1.0);
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  ///@}

protected:
  vtkExtractCTHPart();
  ~vtkExtractCTHPart() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Compute the bounds over the composite dataset, some sub-dataset
   * can be on other processors. Returns false of communication failure.
   */
  bool ComputeGlobalBounds(vtkCompositeDataSet* input);

  /**
   * Extract contour for a particular array over the entire input dataset.
   * Returns false on error.
   */
  vtkSmartPointer<vtkDataSet> ExtractContour(vtkCompositeDataSet* input, const char* arrayName);

  /**
   * Extract solids (unstructuredGrids) for a particular array
   * over the entire input dataset. Returns false on error.
   */
  vtkSmartPointer<vtkDataSet> ExtractSolid(vtkCompositeDataSet* input, const char* arrayName);

  void ExecuteFaceQuads(vtkDataSet* input, vtkPolyData* output, int maxFlag, int originExtents[6],
    int ext[6], int aAxis, int bAxis, int cAxis);

  /**
   * Is block face on axis0 (either min or max depending on the maxFlag)
   * composed of only ghost cells?
   * \pre valid_axis0: axis0>=0 && axis0<=2
   */
  int IsGhostFace(int axis0, int maxFlag, int dims[3], vtkUnsignedCharArray* ghostArray);

  void TriggerProgressEvent(double val);

  int VolumeFractionType;
  double VolumeFractionSurfaceValue;
  double VolumeFractionSurfaceValueInternal;
  bool GenerateTriangles;
  bool GenerateSolidGeometry;
  bool Capping;
  bool RemoveGhostCells;
  vtkPlane* ClipPlane;
  vtkMultiProcessController* Controller;

private:
  vtkExtractCTHPart(const vtkExtractCTHPart&) = delete;
  void operator=(const vtkExtractCTHPart&) = delete;

  class VectorOfFragments;
  class VectorOfSolids;

  /**
   * Determine the true value to use for clipping based on the data-type.
   */
  inline void DetermineSurfaceValue(int dataType);

  /**
   * Extract contour for a particular array over a particular block in the input
   * dataset.  Returns false on error.
   */
  template <class T>
  bool ExtractClippedContourOnBlock(
    vtkExtractCTHPart::VectorOfFragments& fragments, T* input, const char* arrayName);

  /**
   * Extract contour for a particular array over a particular block in the input
   * dataset.  Returns false on error.
   */
  template <class T>
  bool ExtractContourOnBlock(
    vtkExtractCTHPart::VectorOfFragments& fragments, T* input, const char* arrayName);

  /**
   * Append quads for faces of the block that actually on the bounds
   * of the hierarchical dataset. Deals with ghost cells.
   */
  template <class T>
  void ExtractExteriorSurface(vtkExtractCTHPart::VectorOfFragments& fragments, T* input);

  /**
   * Extract clipped volume for a particular array over a particular block in the input
   * dataset.  Returns false on error.
   */
  template <class T>
  bool ExtractClippedVolumeOnBlock(VectorOfSolids& solids, T* input, const char* arrayName);

  /**
   * Fast cell-data-2-point-data implementation.
   */
  void ExecuteCellDataToPointData(
    vtkDataArray* cellVolumeFraction, vtkDoubleArray* pointVolumeFraction, const int* dims);

  double ProgressShift;
  double ProgressScale;

  class ScaledProgress;
  friend class ScaledProgress;
  vtkExtractCTHPartInternal* Internals;
};
VTK_ABI_NAMESPACE_END
#endif
