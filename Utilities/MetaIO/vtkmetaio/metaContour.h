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

#ifndef ITKMetaIO_METACONTOUR_H
#  define ITKMetaIO_METACONTOUR_H

#  include "metaUtils.h"
#  include "metaObject.h"

#  ifdef _MSC_VER
#    pragma warning(disable : 4786)
#    pragma warning(disable : 4251)
#  endif

#  include <list>

#  if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#  endif

/**
 * Description:
 *    Reads and Writes MetaContour Files.
 *
 * \author Julien Jomier
 *
 * \date March 2006
 *
 */
class METAIO_EXPORT ContourControlPnt
{
public:
  explicit ContourControlPnt(int dim);
  ~ContourControlPnt();

  unsigned int m_Dim;
  unsigned int m_Id;
  float *      m_X;
  float *      m_XPicked;
  float *      m_V;
  float        m_Color[4]{};
};


class METAIO_EXPORT ContourInterpolatedPnt
{
public:
  explicit ContourInterpolatedPnt(int dim)
  {
    m_Dim = static_cast<unsigned int>(dim);
    m_Id = 0;
    m_X = new float[m_Dim];
    // Color is red by default
    m_Color[0] = 1.0f;
    m_Color[1] = 0.0f;
    m_Color[2] = 0.0f;
    m_Color[3] = 1.0f;
  }

  ~ContourInterpolatedPnt() { delete[] m_X; }

  unsigned int m_Dim;
  float *      m_X;
  unsigned int m_Id;
  float        m_Color[4]{};
};


class METAIO_EXPORT MetaContour : public MetaObject
{

public:
  typedef std::list<ContourControlPnt *>      ControlPointListType;
  typedef std::list<ContourInterpolatedPnt *> InterpolatedPointListType;

  MetaContour();
  explicit MetaContour(const char * _headerName);
  explicit MetaContour(const MetaContour * _contour);
  explicit MetaContour(unsigned int dim);

  ~MetaContour() override;

  void
  PrintInfo() const override;
  void
  CopyInfo(const MetaObject * _object) override;

  //    NPoints(...)
  //       Required Field
  //       Number of points which compose the tube
  int
  NControlPoints() const;

  //    ControlPointDim(...)
  //       Required Field
  //       Definition of points
  void
  ControlPointDim(const char * pointDim);
  const char *
  ControlPointDim() const;

  MET_InterpolationEnumType
  Interpolation() const;
  void
  Interpolation(MET_InterpolationEnumType _interpolation);

  int
  NInterpolatedPoints() const;

  void
  InterpolatedPointDim(const char * pointDim);
  const char *
  InterpolatedPointDim() const;

  void
  Closed(bool close);
  bool
  Closed() const;

  void
  AttachedToSlice(long int slice);
  long int
  AttachedToSlice() const;

  void
  DisplayOrientation(int display);
  int
  DisplayOrientation() const;

  void
  Clear() override;

  ControlPointListType &
  GetControlPoints()
  {
    return m_ControlPointsList;
  }
  const ControlPointListType &
  GetControlPoints() const
  {
    return m_ControlPointsList;
  }

  InterpolatedPointListType &
  GetInterpolatedPoints()
  {
    return m_InterpolatedPointsList;
  }
  const InterpolatedPointListType &
  GetInterpolatedPoints() const
  {
    return m_InterpolatedPointsList;
  }

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

  int                       m_NControlPoints{};
  int                       m_NInterpolatedPoints{};
  char                      m_ControlPointDim[255]{};
  char                      m_InterpolatedPointDim[255]{};
  bool                      m_Closed{};
  MET_InterpolationEnumType m_InterpolationType;
  ControlPointListType      m_ControlPointsList;
  InterpolatedPointListType m_InterpolatedPointsList;

  int  m_DisplayOrientation{};
  long m_AttachedToSlice{};
};

#  if (METAIO_USE_NAMESPACE)
};
#  endif


#endif
