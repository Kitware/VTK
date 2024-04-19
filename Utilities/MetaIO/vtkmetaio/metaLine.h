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

#ifndef ITKMetaIO_METALINE_H
#  define ITKMetaIO_METALINE_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  ifdef _MSC_VER
#    pragma warning(disable : 4251)
#  endif

#  include <list>


/*!    MetaLine (.h and .cxx)
 *
 * Description:
 *    Reads and Writes MetaLineFiles.
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

class LinePnt
{
public:
  explicit LinePnt(int dim);

  ~LinePnt();

  unsigned int m_Dim;
  float *      m_X;
  float **     m_V;
  float        m_Color[4]{};
};


class METAIO_EXPORT MetaLine : public MetaObject
{

  // PUBLIC
public:
  typedef std::list<LinePnt *> PointListType;
  // Constructors & Destructor
  MetaLine();

  explicit MetaLine(const char * _headerName);

  explicit MetaLine(const MetaLine * _line);

  explicit MetaLine(unsigned int dim);

  ~MetaLine() override;

  void
  PrintInfo() const override;

  void
  CopyInfo(const MetaObject * _object) override;


  //    NPoints(...)
  //       Required Field
  //       Number of points which compose the line
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
