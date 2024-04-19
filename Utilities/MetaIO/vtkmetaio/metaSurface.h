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

#ifndef ITKMetaIO_METASURFACE_H
#  define ITKMetaIO_METASURFACE_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  ifdef _MSC_VER
#    pragma warning(disable : 4251)
#  endif

#  include <list>


/*!    MetaSurface (.h and .cxx)
 *
 * Description:
 *    Reads and Writes MetaSurfaceFiles.
 *
 * \author Julien Jomier
 *
 * \date July 02, 2002
 *
 */

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class SurfacePnt
{
public:
  explicit SurfacePnt(int dim);
  ~SurfacePnt();

  unsigned int m_Dim;
  float *      m_X;
  float *      m_V;
  float        m_Color[4]{};
};


class METAIO_EXPORT MetaSurface : public MetaObject
{

  // PUBLIC
public:
  typedef std::list<SurfacePnt *> PointListType;
  // Constructors & Destructor
  MetaSurface();

  explicit MetaSurface(const char * _headerName);

  explicit MetaSurface(const MetaSurface * _surface);

  explicit MetaSurface(unsigned int dim);

  ~MetaSurface() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;

  //    NPoints(...)
  //       Required Field
  //       Number of points which compose the tube
  void
  NPoints(int npnt);
  int
  NPoints() const;

  //    PointDim(...)
  //       Required Field
  //       Definition of points
  void
  PointDim(const char * pointDim);
  const char *
  PointDim() const;


  void
  Clear() override;

  PointListType &
  GetPoints()
  {
    return m_PointList;
  }
  const PointListType &
  GetPoints() const
  {
    return m_PointList;
  }

  MET_ValueEnumType
  ElementType() const;
  void
  ElementType(MET_ValueEnumType _elementType);

  // PROTECTED
protected:
  bool m_ElementByteOrderMSB{};

  void
  M_SetupReadFields() override;

  void
  M_SetupWriteFields() override;

  bool
  M_Read() override;

  bool
  M_Write() override;

  int m_NPoints{}; // "NPoints = "         0

  char m_PointDim[255]{}; // "PointDim = "       "x y z r"

  PointListType m_PointList;

  MET_ValueEnumType m_ElementType;
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
