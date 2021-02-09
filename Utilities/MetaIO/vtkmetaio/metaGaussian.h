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

#ifndef ITKMetaIO_METAGAUSSIAN_H
#  define ITKMetaIO_METAGAUSSIAN_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  include <list>


/*!    MetaGaussian (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaGaussianFiles.
 *
 * \author Mark Foskey
 *
 * \date February 12, 2004
 *
 * Depends on:
 *    MetaUtils.h
 *    MetaObject.h
 */

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class METAIO_EXPORT MetaGaussian : public MetaObject
{

  // PUBLIC
public:
  // Constructors & Destructor
  MetaGaussian();

  explicit MetaGaussian(const char * _headerName);

  explicit MetaGaussian(const MetaGaussian * _gaussian);

  explicit MetaGaussian(unsigned int dim);

  ~MetaGaussian() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;

  void
  Clear() override;

  /** Set/Get the maximum value. */
  void
  Maximum(float val)
  {
    m_Maximum = val;
  }
  float
  Maximum() const
  {
    return m_Maximum;
  }

  /** Set/Get the radius value. */
  void
  Radius(float val)
  {
    m_Radius = val;
  }
  float
  Radius() const
  {
    return m_Radius;
  }

  /** Set/Get the sigma value. */
  void
  Sigma(float val)
  {
    m_Sigma = val;
  }
  float
  Sigma() const
  {
    return m_Sigma;
  }

  // PROTECTED
protected:

  /** Set up the fields to read a MetaGaussian file. */
  void
  M_SetupReadFields() override;

  /** Set up the fields to write a MetaGaussian file. */
  void
  M_SetupWriteFields() override;

  /** Read the MetaGaussian file properties. */
  bool
  M_Read() override;

  /** The maximum value of the MetaGaussian object. */
  float m_Maximum{};

  /** The radius of the MetaGaussian object. */
  float m_Radius{};

  /** The standard deviation of the MetaGaussian object. */
  float m_Sigma{};
};


#  if (METAIO_USE_NAMESPACE)
};
#  endif


#endif
