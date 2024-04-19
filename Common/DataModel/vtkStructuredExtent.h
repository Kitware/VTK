// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredExtent
 * @brief   helper class to aid working with structured
 *  extents.
 *
 *
 *  vtkStructuredExtent is an helper class that helps in arithmetic with
 *  structured extents. It defines a bunch of static methods (most of which are
 *  inlined) to aid in dealing with extents.
 */

#ifndef vtkStructuredExtent_h
#define vtkStructuredExtent_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkStructuredExtent : public vtkObject
{
public:
  static vtkStructuredExtent* New();
  vtkTypeMacro(vtkStructuredExtent, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Clamps \c ext to fit in \c wholeExt.
   */
  static void Clamp(int ext[6], const int wholeExt[6]);

  /**
   * Returns true if \c ext is fits within \c wholeExt with at least 1 dimension
   * smaller than the \c wholeExt.
   */
  static bool StrictlySmaller(const int ext[6], const int wholeExt[6]);

  /**
   * Returns if \c ext fits within \c wholeExt. Unlike StrictlySmaller, this
   * method returns true even if \c ext == \c wholeExt.
   */
  static bool Smaller(const int ext[6], const int wholeExt[6]);

  /**
   * Grows the \c ext on each side by the given \c count.
   */
  static void Grow(int ext[6], int count);

  /**
   * Grows the \c ext on each side by the given \c count
   * while keeping it limited to the \c wholeExt.
   */
  static void Grow(int ext[6], int count, int wholeExt[6]);

  /**
   * Makes \c ext relative to \c wholeExt.
   */
  static void Transform(int ext[6], int wholeExt[6]);

  /**
   * Given the extents, computes the dimensions.
   */
  static void GetDimensions(const int ext[6], int dims[3]);

protected:
  vtkStructuredExtent();
  ~vtkStructuredExtent() override;

private:
  vtkStructuredExtent(const vtkStructuredExtent&) = delete;
  void operator=(const vtkStructuredExtent&) = delete;
};

//----------------------------------------------------------------------------
inline void vtkStructuredExtent::Clamp(int ext[6], const int wholeExt[6])
{
  ext[0] = (ext[0] < wholeExt[0]) ? wholeExt[0] : ext[0];
  ext[1] = (ext[1] > wholeExt[1]) ? wholeExt[1] : ext[1];

  ext[2] = (ext[2] < wholeExt[2]) ? wholeExt[2] : ext[2];
  ext[3] = (ext[3] > wholeExt[3]) ? wholeExt[3] : ext[3];

  ext[4] = (ext[4] < wholeExt[4]) ? wholeExt[4] : ext[4];
  ext[5] = (ext[5] > wholeExt[5]) ? wholeExt[5] : ext[5];
}

//----------------------------------------------------------------------------
inline bool vtkStructuredExtent::Smaller(const int ext[6], const int wholeExt[6])
{
  if (ext[0] < wholeExt[0] || ext[0] > wholeExt[0 + 1] || ext[0 + 1] < wholeExt[0] ||
    ext[0 + 1] > wholeExt[0 + 1])
  {
    return false;
  }

  if (ext[2] < wholeExt[2] || ext[2] > wholeExt[2 + 1] || ext[2 + 1] < wholeExt[2] ||
    ext[2 + 1] > wholeExt[2 + 1])
  {
    return false;
  }

  if (ext[4] < wholeExt[4] || ext[4] > wholeExt[4 + 1] || ext[4 + 1] < wholeExt[4] ||
    ext[4 + 1] > wholeExt[4 + 1])
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
inline bool vtkStructuredExtent::StrictlySmaller(const int ext[6], const int wholeExt[6])
{
  if (!vtkStructuredExtent::Smaller(ext, wholeExt))
  {
    return false;
  }

  if (ext[0] > wholeExt[0] || ext[1] < wholeExt[1] || ext[2] > wholeExt[2] ||
    ext[3] < wholeExt[3] || ext[4] > wholeExt[4] || ext[5] < wholeExt[5])
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
inline void vtkStructuredExtent::Grow(int ext[6], int count)
{
  ext[0] -= count;
  ext[2] -= count;
  ext[4] -= count;

  ext[1] += count;
  ext[3] += count;
  ext[5] += count;
}

//----------------------------------------------------------------------------
inline void vtkStructuredExtent::Grow(int ext[6], int count, int wholeExt[6])
{
  vtkStructuredExtent::Grow(ext, count);
  vtkStructuredExtent::Clamp(ext, wholeExt);
}

//----------------------------------------------------------------------------
inline void vtkStructuredExtent::Transform(int ext[6], int wholeExt[6])
{
  ext[0] -= wholeExt[0];
  ext[1] -= wholeExt[0];

  ext[2] -= wholeExt[2];
  ext[3] -= wholeExt[2];

  ext[4] -= wholeExt[4];
  ext[5] -= wholeExt[4];
}

//----------------------------------------------------------------------------
inline void vtkStructuredExtent::GetDimensions(const int ext[6], int dims[3])
{
  dims[0] = ext[1] - ext[0] + 1;
  dims[1] = ext[3] - ext[2] + 1;
  dims[2] = ext[5] - ext[4] + 1;
}

VTK_ABI_NAMESPACE_END
#endif
