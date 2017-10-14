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
#define ITKMetaIO_METASURFACE_H

#include "metaUtils.h"
#include "metaObject.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>


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

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class SurfacePnt
{
public:

  SurfacePnt(int dim);
  ~SurfacePnt();

  unsigned int m_Dim;
  float* m_X;
  float* m_V;
  float  m_Color[4];
};




class METAIO_EXPORT MetaSurface : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef METAIO_STL::list<SurfacePnt*> PointListType;
    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaSurface(void);

    MetaSurface(const char *_headerName);

    MetaSurface(const MetaSurface *_surface);

    MetaSurface(unsigned int dim);

    ~MetaSurface(void) MET_OVERRIDE;

    void PrintInfo(void) const MET_OVERRIDE;

    void CopyInfo(const MetaObject * _object) MET_OVERRIDE;

    //    NPoints(...)
    //       Required Field
    //       Number of points wich compose the tube
    void  NPoints(int npnt);
    int   NPoints(void) const;

    //    PointDim(...)
    //       Required Field
    //       Definition of points
    void        PointDim(const char* pointDim);
    const char* PointDim(void) const;


    void  Clear(void) MET_OVERRIDE;

    PointListType & GetPoints(void) {return m_PointList;}
    const PointListType & GetPoints(void) const {return m_PointList;}

    MET_ValueEnumType ElementType(void) const;
    void  ElementType(MET_ValueEnumType _elementType);

  ////
  //
  // PROTECTED
  //
  ////
  protected:

    bool  m_ElementByteOrderMSB;

    void  M_Destroy(void) MET_OVERRIDE;

    void  M_SetupReadFields(void) MET_OVERRIDE;

    void  M_SetupWriteFields(void) MET_OVERRIDE;

    bool  M_Read(void) MET_OVERRIDE;

    bool  M_Write(void) MET_OVERRIDE;

    int m_NPoints;      // "NPoints = "         0

    char m_PointDim[255]; // "PointDim = "       "x y z r"

    PointListType m_PointList;

    MET_ValueEnumType m_ElementType;

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
