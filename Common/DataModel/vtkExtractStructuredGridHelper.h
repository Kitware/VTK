// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractStructuredGridHelper
 * @brief   helper for extracting/sub-sampling
 *  structured datasets.
 *
 *
 * vtkExtractStructuredGridHelper provides some common functionality that is
 * used by filters that extract and sub-sample structured data. Specifically,
 * it provides functionality for calculating the mapping from the output extent
 * of each process to the input extent.
 *
 * @sa
 * vtkExtractGrid vtkExtractVOI vtkExtractRectilinearGrid
 */

#ifndef vtkExtractStructuredGridHelper_h
#define vtkExtractStructuredGridHelper_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkPointData;
class vtkPoints;
VTK_ABI_NAMESPACE_END

namespace vtk
{
namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

struct vtkIndexMap;

VTK_ABI_NAMESPACE_END
} // END namespace detail
} // END namespace vtk

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkExtractStructuredGridHelper : public vtkObject
{
public:
  static vtkExtractStructuredGridHelper* New();
  vtkTypeMacro(vtkExtractStructuredGridHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Get & Set Macros
  vtkGetVector6Macro(OutputWholeExtent, int);

  /**
   * \brief Initializes the index map.
   * \param voi the extent of the volume of interest
   * \param wholeExt the whole extent of the domain
   * \param sampleRate the sampling rate
   * \param includeBoundary indicates whether to include the boundary or not.
   */
  void Initialize(int voi[6], int wholeExt[6], int sampleRate[3], bool includeBoundary);

  /**
   * Returns true if the helper is properly initialized.
   */
  bool IsValid() const;

  /**
   * \brief Returns the size along a given dimension
   * \param dim the dimension in query
   * \pre dim >= 0 && dim < 3
   */
  int GetSize(int dim);

  /**
   * \brief Given a dimension and output index, return the corresponding
   * extent index. This method should be used to convert array indices,
   * such as the coordinate arrays for rectilinear grids.
   * \param dim the data dimension
   * \param outIdx The output index along the given dimension.
   * \pre dim >= 0 && dim < 3
   * \pre outIdx >= 0 && outIdx < this->GetSize( dim )
   * \return The input extent index along the given dimension.
   * \sa GetMappedExtentValue
   * \sa GetMappedExtentValueFromIndex
   */
  int GetMappedIndex(int dim, int outIdx);

  /**
   * \brief Given a dimension and output extent value, return the corresponding
   * input extent index. This method should be used to compute extent
   * indices from extent values.
   * \param dim the data dimension
   * \param outExtVal The output extent value along the given dimension.
   * \pre dim >= 0 && dim < 3
   * \pre outExtVal >= this->GetOutputWholeExtent()[2*dim] &&
   * outExtVal <= this->GetOutputWholeExtent()[2*dim+1]
   * \return The input extent index along the given dimension.
   * \sa GetMappedExtentValue
   * \sa GetMappedExtentValueFromIndex
   */
  int GetMappedIndexFromExtentValue(int dim, int outExtVal);

  /**
   * \brief Given a dimension and output extent value, return the corresponding
   * input extent value. This method should be used to convert extent values.
   * \param dim the data dimension.
   * \param outExtVal The output extent value along the given dimension.
   * \pre dim >= 0 && dim < 3
   * \pre outExtVal >= this->GetOutputWholeExtent()[2*dim] &&
   * outExtVal <= this->GetOutputWholeExtent()[2*dim+1]
   * \return The input extent value along the given dimension.
   * \sa GetMappedIndex
   * \sa GetMappedExtentValueFromIndex
   */
  int GetMappedExtentValue(int dim, int outExtVal);

  /**
   * \brief Given a dimension and output extent index, return the corresponding
   * input extent value. This method should be used to compute extent values
   * from extent indices.
   * \param dim the data dimension.
   * \param outIdx The output index along the given dimension.
   * \pre dim >= 0 && dim < 3
   * \pre outIdx >= 0 && outIdx < this->GetSize( dim )
   * \return The input extent value along the given dimension.
   * \sa GetMappedIndex
   * \sa GetMappedExtentValue
   */
  int GetMappedExtentValueFromIndex(int dim, int outIdx);

  /**
   * \brief Returns the begin & end extent that intersects with the VOI
   * \param inExt the input extent
   * \param voi the volume of interest
   * \param begin the begin extent
   * \param end the end extent
   */
  void ComputeBeginAndEnd(int inExt[6], int voi[6], int begin[3], int end[3]);

  /**
   * \brief Copies the points & point data to the output.
   * \param inExt the input grid extent.
   * \param outExt the output grid extent.
   * \param pd pointer to the input point data.
   * \param inpnts pointer to the input points, or nullptr if uniform grid.
   * \param outPD point to the output point data.
   * \param outpnts pointer to the output points, or nullptr if uniform grid.
   * \pre pd != nullptr.
   * \pre outPD != nullptr.
   */
  void CopyPointsAndPointData(int inExt[6], int outExt[6], vtkPointData* pd, vtkPoints* inpnts,
    vtkPointData* outPD, vtkPoints* outpnts);

  /**
   * \brief Copies the cell data to the output.
   * \param inExt the input grid extent.
   * \param outExt the output grid extent.
   * \param cd the input cell data.
   * \param outCD the output cell data.
   * \pre cd != nullptr.
   * \pre outCD != nullptr.
   */
  void CopyCellData(int inExt[6], int outExt[6], vtkCellData* cd, vtkCellData* outCD);

  /**
   * Calculate the VOI for a partitioned structured dataset. This method sets
   * \a partitionedVOI to the VOI that extracts as much of the
   * \a partitionedExtent as possible while considering the \a globalVOI, the
   * \a sampleRate, and the boundary conditions.
   * \param globalVOI The full VOI for the entire distributed dataset.
   * \param partitionedExtent Extent of the process's partitioned input data.
   * \param sampleRate The sampling rate in each dimension.
   * \param includeBoundary Whether or not to include the boundary of the VOI,
   * even if it doesn't fit the spacing.
   * \param partitionedVOI The extent of the process's partitioned dataset that
   * should be extracted by a serial extraction filter.
   */
  static void GetPartitionedVOI(const int globalVOI[6], const int partitionedExtent[6],
    const int sampleRate[3], bool includeBoundary, int partitionedVOI[6]);
  /**
   * Calculate the partitioned output extent for a partitioned structured
   * dataset. This method sets \a partitionedOutputExtent to the correct extent
   * of an extracted dataset, such that it properly fits with the other
   * partitioned pieces while considering the \a globalVOI, the
   * \a sampleRate, and the boundary conditions.
   * \param globalVOI The full VOI for the entire distributed dataset.
   * \param partitionedVOI The VOI used in the serial extraction.
   * \param outputWholeExtent The output extent of the full dataset.
   * \param sampleRate The sampling rate in each dimension.
   * \param includeBoundary Whether or not to include the boundary of the VOI,
   * even if it doesn't fit the spacing.
   * \param partitionedOutputExtent The correct output extent of the extracted
   * dataset.
   */
  static void GetPartitionedOutputExtent(const int globalVOI[6], const int partitionedVOI[6],
    const int outputWholeExtent[6], const int sampleRate[3], bool includeBoundary,
    int partitionedOutputExtent[6]);

protected:
  vtkExtractStructuredGridHelper();
  ~vtkExtractStructuredGridHelper() override;

  // Input parameters -- used to reinitialize when data changes.
  int VOI[6];
  int InputWholeExtent[6];
  int SampleRate[3];
  bool IncludeBoundary;

  int OutputWholeExtent[6];
  vtk::detail::vtkIndexMap* IndexMap;

  /**
   * \brief Invalidates the output extent.
   */
  void Invalidate();

private:
  vtkExtractStructuredGridHelper(const vtkExtractStructuredGridHelper&) = delete;
  void operator=(const vtkExtractStructuredGridHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKEXTRACTSTRUCTUREDGRIDHELPER_H_ */
