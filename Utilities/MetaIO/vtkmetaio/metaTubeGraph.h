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

#ifndef ITKMetaIO_METATUBEGRAPH_H
#  define ITKMetaIO_METATUBEGRAPH_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  ifdef _MSC_VER
#    pragma warning(disable : 4251)
#  endif

#  include <vector>


/*!    MetaTubeGraph (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaTubeGraph Files.
 *
 * \author Julien Jomier
 *
 * \date May 22, 2002
 */

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

class TubeGraphPnt
{
public:
  explicit TubeGraphPnt(int dim)
  {
    m_Dim = static_cast<unsigned int>(dim);
    m_GraphNode = -1;
    m_R = 0;
    m_P = 0;
    m_T = new float[m_Dim * m_Dim];
  }

  ~TubeGraphPnt() { delete[] m_T; }

  unsigned int m_Dim;
  int          m_GraphNode;
  float        m_R;
  float        m_P;
  float *      m_T;
};


class METAIO_EXPORT MetaTubeGraph : public MetaObject
{

  // PUBLIC
public:
  typedef std::vector<TubeGraphPnt *> PointListType;
  // Constructors & Destructor
  MetaTubeGraph();

  explicit MetaTubeGraph(const char * _headerName);

  explicit MetaTubeGraph(const MetaTubeGraph * _tube);

  explicit MetaTubeGraph(unsigned int dim);

  ~MetaTubeGraph() override;

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

  //    Root(...)
  //       Optional Field
  //       Set if this tube is a root
  void
  Root(int root);
  int
  Root() const;


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

  void
  M_SetupReadFields() override;

  void
  M_SetupWriteFields() override;

  bool
  M_Read() override;

  bool
  M_Write() override;

  int m_Root{}; // "Root = "            0

  int m_NPoints{}; // "NPoints = "         0

  char m_PointDim[255]{}; // "PointDim = "       "x y z r"

  PointListType m_PointList;

  MET_ValueEnumType m_ElementType;
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif

#endif
