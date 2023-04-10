/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPackLabels.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPackLabels
 * @brief   renumber segmentation labels into contiguous runs of (potentially) smaller type
 *
 * vtkPackLabels is a filter that renumbers a set of segmentation labels into
 * a contiguous sequence of label values. The input segmentation labels are
 * represented by an image of arbitrary type, and the labels may be
 * non-contiguous (i.e., there may be "gaps" in the labels used to annotate
 * structured in the segmentation). After execution, the output of this
 * filter consists of an output image with the same input dimensions/extent
 * and spacing, however the labels are renumbered so that they are contiguous
 * (starting with value==0, [0,NumberOfLabels)). After filter execution, an
 * array of labels present in the input can be retrieved, listed in ascending
 * order, this is useful in various filters such as isocontouring filters
 * which require iso/label-values.
 *
 * The filter also converts the input data from one type to another. By
 * default, the output labels are of an unsigned integral type large enough
 * to represent the N packed label values. It is also possible to explicitly
 * specify the type of the output annotation/label image. This conversion
 * capability often reduces the size of the output image, and can be used is
 * useful when an algorithm performs better, or requires, a certain type of
 * input data. Note however, that manual specification can be dangerous:
 * trying to pack a large number of labels into a manually specified reduced
 * precision can result in conversion issues. By default, the filter will
 * select the unsigned integral type that can represent the N annotation
 * labels.
 *
 * @sa
 * vtkSurfaceNets3D vtkSurfaceNets2D vtkDiscreteFlyingEdges3D
 * vtkDiscreteMarchingCubes
 */

#ifndef vtkPackLabels_h
#define vtkPackLabels_h

#include "vtkDataArray.h"         // For returning list of labels
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkImageAlgorithm.h"
#include "vtkSmartPointer.h" // For returning list of labels

VTK_ABI_NAMESPACE_BEGIN
struct vtkLabelMap;

class VTKFILTERSCORE_EXPORT vtkPackLabels : public vtkImageAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing an object.
   */
  static vtkPackLabels* New();
  vtkTypeMacro(vtkPackLabels, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Return the number of and list of labels found in the input label map.
   * The methods return a vtkDataArray with the same data type as the input
   * scalar type. The labels are listed in ascending order.
   */
  vtkDataArray* GetLabels() { return this->LabelsArray; }
  vtkIdType GetNumberOfLabels()
  {
    return (this->LabelsArray ? this->LabelsArray->GetNumberOfTuples() : 0);
  }
  ///@}

  ///@{
  /**
   * Specify the data type of the output image. The choice for output type is
   * an unsigned integral type. Note that DEFAULT_TYPE value indicates that
   * the output data type will be of a type large enough to represent the N
   * labels present in the input (this is on by default). Explicit
   * specification of the output type may result in the loss of precision if
   * not performed carefully.
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
   * Indicate whether to pass point data, cell data, or field data through to
   * the output. This can be useful to limit the data being processed down a
   * pipeline, including writing output files. By default, point data and
   * cell data is passed from input to output.
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

  /**
   * Indicate whether the input data is packed. This is useful for diagnostic
   * purposes. The input is packed is for N input labels, then the labels are
   * counting integer contiguous from [l_0....l_(N-1)].
   */
  bool IsInputPacked();

protected:
  vtkPackLabels();
  ~vtkPackLabels() override = default;

  vtkSmartPointer<vtkDataArray> LabelsArray;
  int OutputScalarType;
  bool PassPointData;
  bool PassCellData;
  bool PassFieldData;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPackLabels(const vtkPackLabels&) = delete;
  void operator=(const vtkPackLabels&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
