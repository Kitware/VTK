// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRectilinearGridClip
 * @brief   Reduces the image extent of the input.
 *
 * vtkRectilinearGridClip  will make an image smaller.  The output must have
 * an image extent which is the subset of the input.  The filter has two
 * modes of operation:
 * 1: By default, the data is not copied in this filter.
 * Only the whole extent is modified.
 * 2: If ClipDataOn is set, then you will get no more that the clipped
 * extent.
 */

#ifndef vtkRectilinearGridClip_h
#define vtkRectilinearGridClip_h

// I did not make this a subclass of in place filter because
// the references on the data do not matter. I make no modifications
// to the data.
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkRectilinearGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkRectilinearGridClip : public vtkRectilinearGridAlgorithm
{
public:
  static vtkRectilinearGridClip* New();
  vtkTypeMacro(vtkRectilinearGridClip, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The whole extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6], vtkInformation* outInfo = nullptr);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int* GetOutputWholeExtent() { return this->OutputWholeExtent; }
  ///@}

  void ResetOutputWholeExtent();

  ///@{
  /**
   * By default, ClipData is off, and only the WholeExtent is modified.
   * the data's extent may actually be larger.  When this flag is on,
   * the data extent will be no more than the OutputWholeExtent.
   */
  vtkSetMacro(ClipData, vtkTypeBool);
  vtkGetMacro(ClipData, vtkTypeBool);
  vtkBooleanMacro(ClipData, vtkTypeBool);
  ///@}

protected:
  vtkRectilinearGridClip();
  ~vtkRectilinearGridClip() override = default;

  // Time when OutputImageExtent was computed.
  vtkTimeStamp CTime;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputWholeExtent[6];

  vtkTypeBool ClipData;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void CopyData(vtkRectilinearGrid* inData, vtkRectilinearGrid* outData, int* ext);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkRectilinearGridClip(const vtkRectilinearGridClip&) = delete;
  void operator=(const vtkRectilinearGridClip&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
