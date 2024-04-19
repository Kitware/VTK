// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPackLabels
 * @brief   renumber segmentation labels into contiguous runs of (potentially) smaller type
 *
 * vtkPackLabels is a filter that renumbers a set of segmentation labels into
 * a contiguous sequence of label values. The input segmentation labels are
 * represented by a scalar array of arbitrary type, and the labels may be
 * non-contiguous (i.e., there may be "gaps" in the labels used to annotate
 * structured in the segmentation). After execution, the output of this
 * filter consists of (by default) an output of the same dataset type as the
 * input; however the labels are renumbered so that they are contiguous
 * (starting with value==0, [0,NumberOfLabels)). After filter execution, an
 * array of labels present in the input can be retrieved (named
 * "PackedLabels"), listed in ascending order, this is useful in various
 * filters such as discrete isocontouring filters which require
 * iso/label-values (e.g., vtkSurfaceNets3D).
 *
 * Note that this filter mostly works on input dataset types of vtkImageData
 * (segmentation maps are commonly used in medical computing). However,
 * because this filter operates on scalar point or cell data independent of
 * dataset type, it has been generalized to work on any dataset type.
 *
 * The filter also converts the input data from one type to another. By
 * default, the output labels are of an unsigned integral type large enough
 * to represent the N packed label values. It is also possible to explicitly
 * specify the type of the output annotation/label image. This conversion
 * capability often reduces the size of the output image, and can be used is
 * useful when an algorithm performs better, or requires, a certain type of
 * input data. Note however, that manual specification can be dangerous:
 * trying to pack a large number of labels into a manually specified reduced
 * precision label values may result in setting some label values to the
 * BackgroundValue.
 *
 * Finally, in advanced usage, it is possible to control how sorting of the
 * output labels is performed. By default, the labels are assorted based on
 * their associated input label values (SortByLabelValue). However, it is
 * possible to arrange the labels based on their frequency of use
 * (SortByLabelCount). Sorting is useful for gathering data statistics, or to
 * extract and display the segmented objects that are the "largest" in the
 * dataset.
 *
 * @sa
 * vtkSurfaceNets3D vtkSurfaceNets2D vtkDiscreteFlyingEdges3D
 * vtkDiscreteMarchingCubes
 */

#ifndef vtkPackLabels_h
#define vtkPackLabels_h

#include "vtkDataArray.h" // For returning list of labels
#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkIdTypeArray.h"       // For returning count of labels
#include "vtkSmartPointer.h"      // For returning list of labels

VTK_ABI_NAMESPACE_BEGIN
struct vtkLabelMap;

class VTKFILTERSCORE_EXPORT vtkPackLabels : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing an object.
   */
  static vtkPackLabels* New();
  vtkTypeMacro(vtkPackLabels, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Return the number of and list of labels found in the input label map.
   * The methods return a vtkDataArray with the same data type as the input
   * scalar type. By default, the labels are sorted in ascending order based
   * on the input data (label) values (i.e., SortByLabelValue). However, if
   * SortByLabelCount is specified, then the labels are sorted in descending
   * order based on their frequency of occurrence (i.e., their counts).
   */
  vtkDataArray* GetLabels() { return this->LabelsArray; }
  vtkIdType GetNumberOfLabels()
  {
    return (this->LabelsArray ? this->LabelsArray->GetNumberOfTuples() : 0);
  }
  ///@}

  /**
   * Return the frequency of occurrence (i.e., the count) of each label
   * returned in the LabelsArray. The methods returns a vtkIdTypeArray that is
   * implicitly ordered consistent with the LabelsArray (i.e., LabelsCount[i]
   * gives the frequency count for LabelsArray[i]). Note that if
   * SortByLabelCount is set, then the labels array and labels count are
   * sorted in descending order based on the frequency of occurrence of
   * labels. If SortByLabelValue is set, then the labels array and label
   * counts are sorted in ascending order based on input label values. Also
   * note that the size of the LabelsCount array is identical to the size
   * of the LabelsCount array.
   */
  vtkIdTypeArray* GetLabelsCount() { return this->LabelsCount; }

  /**
   * Flags to control how sorting of the labels is performed.
   */
  enum SortBy
  {
    SORT_BY_LABEL_VALUE = 0,
    SORT_BY_LABEL_COUNT
  };

  ///@{
  /**
   * Indicate whether to sort the output labels by their input scalars label
   * value (SortByLabelValue), or to sort by the frequency of occurrence of
   * the label values(SortByLabelCount). By default, sorting is performed by
   * label value.  Note that typically the background label has the highest
   * frequency of occurrence, with a label value == 0 (but this is not a
   * guarantee).
   */
  vtkSetClampMacro(SortBy, int, SORT_BY_LABEL_VALUE, SORT_BY_LABEL_COUNT);
  vtkGetMacro(SortBy, int);
  void SortByLabelValue() { this->SetSortBy(SORT_BY_LABEL_VALUE); }
  void SortByLabelCount() { this->SetSortBy(SORT_BY_LABEL_COUNT); }
  ///@}

  ///@{
  /**
   * Specify the data type of the output image. The choice for output type is
   * an unsigned integral type. Note that DEFAULT_TYPE value indicates that
   * the output data type will be of a type large enough to represent the N
   * labels present in the input (this is on by default). Explicit
   * specification of the output type can cause labels in the input scalars
   * to be replaced with the BackgroundValue in the output scalars. This
   * occurs when trying to represent N labels in a data type that cannot
   * represent all N values (e.g., trying to pack 500 labels unto an unsigned
   * char packed label map).
   */
  enum DefaultScalarType
  {
    VTK_DEFAULT_TYPE = -1
  };
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDefault() { this->SetOutputScalarType(VTK_DEFAULT_TYPE); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(VTK_UNSIGNED_CHAR); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(VTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(VTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(VTK_UNSIGNED_LONG); }
  ///@}

  ///@{
  /**
   * Specify a background label value. This value is used when the ith label is
   * i >= N where N is the number of labels that the OutputScalarType can represent.
   * So for example, when trying to pack 500 labels into an unsigned char output scalar
   * type, all i labels i>=256 would be set to this background value. By default the
   * BackgroundValue == 0.
   */
  vtkSetMacro(BackgroundValue, unsigned long);
  vtkGetMacro(BackgroundValue, unsigned long);
  ///@}

  ///@{
  /**
   * Indicate whether to pass point data, cell data, and/or field data
   * through to the output. This can be useful to limit the data being
   * processed down a pipeline, including writing output files. By default,
   * point data and cell data is passed from input to output.
   */
  vtkSetMacro(PassPointData, bool);
  vtkGetMacro(PassPointData, bool);
  vtkBooleanMacro(PassPointData, bool);
  vtkSetMacro(PassCellData, bool);
  vtkGetMacro(PassCellData, bool);
  vtkBooleanMacro(PassCellData, bool);
  vtkSetMacro(PassFieldData, bool);
  vtkGetMacro(PassFieldData, bool);
  vtkBooleanMacro(PassFieldData, bool);
  ///@}

protected:
  vtkPackLabels();
  ~vtkPackLabels() override = default;

  vtkSmartPointer<vtkDataArray> LabelsArray;
  vtkSmartPointer<vtkIdTypeArray> LabelsCount;
  int SortBy;
  int OutputScalarType;
  unsigned long BackgroundValue;
  bool PassPointData;
  bool PassCellData;
  bool PassFieldData;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPackLabels(const vtkPackLabels&) = delete;
  void operator=(const vtkPackLabels&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
