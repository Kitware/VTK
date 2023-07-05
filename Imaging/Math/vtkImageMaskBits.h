// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMaskBits
 * @brief   applies a bit-mask pattern to each component.
 *
 *
 * vtkImageMaskBits applies a bit-mask pattern to each component.  The
 * bit-mask can be applied using a variety of boolean bitwise operators.
 */

#ifndef vtkImageMaskBits_h
#define vtkImageMaskBits_h

#include "vtkImageLogic.h"        //For VTK_AND, VTK_OR ...
#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGMATH_EXPORT vtkImageMaskBits : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMaskBits* New();
  vtkTypeMacro(vtkImageMaskBits, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the bit-masks. Default is 0xffffffff.
   */
  vtkSetVector4Macro(Masks, unsigned int);
  void SetMask(unsigned int mask) { this->SetMasks(mask, mask, mask, mask); }
  void SetMasks(unsigned int mask1, unsigned int mask2)
  {
    this->SetMasks(mask1, mask2, 0xffffffff, 0xffffffff);
  }
  void SetMasks(unsigned int mask1, unsigned int mask2, unsigned int mask3)
  {
    this->SetMasks(mask1, mask2, mask3, 0xffffffff);
  }
  vtkGetVector4Macro(Masks, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the boolean operator. Default is AND.
   */
  vtkSetMacro(Operation, int);
  vtkGetMacro(Operation, int);
  void SetOperationToAnd() { this->SetOperation(VTK_AND); }
  void SetOperationToOr() { this->SetOperation(VTK_OR); }
  void SetOperationToXor() { this->SetOperation(VTK_XOR); }
  void SetOperationToNand() { this->SetOperation(VTK_NAND); }
  void SetOperationToNor() { this->SetOperation(VTK_NOR); }
  ///@}

protected:
  vtkImageMaskBits();
  ~vtkImageMaskBits() override = default;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

  unsigned int Masks[4];
  int Operation;

private:
  vtkImageMaskBits(const vtkImageMaskBits&) = delete;
  void operator=(const vtkImageMaskBits&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
