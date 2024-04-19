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

#ifndef ITKMetaIO_METAELLIPSE_H
#  define ITKMetaIO_METAELLIPSE_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  include <list>


/*!    MetaEllipse (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaEllipseFiles.
 *
 * \author Julien Jomier
 *
 * \date May 22, 2002
 *
 * Depends on:
 *    MetaUtils.h
 *    MetaObject.h
 */

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif


class METAIO_EXPORT MetaEllipse : public MetaObject
{

  // PUBLIC
public:
  // Constructors & Destructor
  MetaEllipse();

  explicit MetaEllipse(const char * _headerName);

  explicit MetaEllipse(const MetaEllipse * _ellipse);

  explicit MetaEllipse(unsigned int dim);

  ~MetaEllipse() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;

  void
  Clear() override;

  void
  Radius(const float * radius);
  void
  Radius(float radius);
  void
  Radius(float r1, float r2);
  void
  Radius(float r1, float r2, float r3);
  const float *
  Radius() const;


  // PROTECTED
protected:

  void
  M_SetupReadFields() override;

  void
  M_SetupWriteFields() override;

  bool
  M_Read() override;

  float m_Radius[100]{}; // "Radius = "     0
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif


#endif
