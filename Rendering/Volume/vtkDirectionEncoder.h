// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkDirectionEncoder
 * @brief   encode a direction into a one or two byte value
 *
 *
 * Given a direction, encode it into an integer value. This value should
 * be less than 65536, which is the maximum number of encoded directions
 * supported by this superclass. A direction encoded is used to encode
 * normals in a volume for use during volume rendering, and the
 * amount of space that is allocated per normal is 2 bytes.
 * This is an abstract superclass - see the subclasses for specific
 * implementation details.
 *
 * @sa
 * vtkRecursiveSphereDirectionEncoder
 */

#ifndef vtkDirectionEncoder_h
#define vtkDirectionEncoder_h

#include "vtkObject.h"
#include "vtkRenderingVolumeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGVOLUME_EXPORT vtkDirectionEncoder : public vtkObject
{
public:
  ///@{
  /**
   * Get the name of this class
   */
  vtkTypeMacro(vtkDirectionEncoder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Given a normal vector n, return the encoded direction
   */
  virtual int GetEncodedDirection(float n[3]) = 0;

  /**
   * / Given an encoded value, return a pointer to the normal vector
   */
  virtual float* GetDecodedGradient(int value) VTK_SIZEHINT(3) = 0;

  /**
   * Return the number of encoded directions
   */
  virtual int GetNumberOfEncodedDirections() = 0;

  /**
   * Get the decoded gradient table. There are
   * this->GetNumberOfEncodedDirections() entries in the table, each
   * containing a normal (direction) vector. This is a flat structure -
   * 3 times the number of directions floats in an array.
   */
  virtual float* GetDecodedGradientTable() = 0;

protected:
  vtkDirectionEncoder() = default;
  ~vtkDirectionEncoder() override = default;

private:
  vtkDirectionEncoder(const vtkDirectionEncoder&) = delete;
  void operator=(const vtkDirectionEncoder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
