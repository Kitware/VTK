/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METATRANSFORM_H
#  define ITKMetaIO_METATRANSFORM_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  include <list>


/*!    MetaTransform (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaTransformFiles.
 *
 * \author Julien Jomier
 *
 * \date Feb 14, 2005
 *
 * Depends on:
 *    MetaUtils.h
 *    MetaObject.h
 */

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif


class METAIO_EXPORT MetaTransform : public MetaObject
{

  // PUBLIC
public:
  // Constructors & Destructor
  MetaTransform();

  explicit MetaTransform(const char * _headerName);

  explicit MetaTransform(const MetaTransform * _group);

  explicit MetaTransform(unsigned int dim);

  ~MetaTransform() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;

  void
  Clear() override;

  // Set/Get the parameters of the transforms
  const double *
  Parameters() const;
  void
  Parameters(unsigned int dimension, const double * _parameters);

  unsigned int
  NParameters() const
  {
    return parametersDimension;
  }

  unsigned int
  TransformOrder() const
  {
    return transformOrder;
  }
  void
  TransformOrder(unsigned int order)
  {
    transformOrder = order;
  }

  // Set/Get the grid spacing
  const double *
  GridSpacing() const;
  void
  GridSpacing(const double * _gridSpacing);

  // Set/Get the grid origin
  const double *
  GridOrigin() const;
  void
  GridOrigin(const double * _gridOrigin);

  // Set/Get the grid region size
  const double *
  GridRegionSize() const;
  void
  GridRegionSize(const double * _gridRegionSize);

  // Set/Get the grid region index
  const double *
  GridRegionIndex() const;
  void
  GridRegionIndex(const double * _gridRegionIndex);

  // PROTECTED
protected:
  void
  M_SetupReadFields() override;

  void
  M_SetupWriteFields() override;

  bool
  M_Read() override;
  bool
  M_Write() override;

  double *     parameters{};
  unsigned int parametersDimension{};
  unsigned int transformOrder{};

  // This ivars are used for the BSplineTransform
  double gridSpacing[100]{};
  double gridOrigin[100]{};
  double gridRegionSize[100]{};
  double gridRegionIndex[100]{};
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif


#endif
