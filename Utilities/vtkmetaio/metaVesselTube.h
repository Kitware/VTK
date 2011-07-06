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

#ifndef ITKMetaIO_METAVESSELTUBE_H
#define ITKMetaIO_METAVESSELTUBE_H

#include "metaUtils.h"
#include "metaObject.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>


/*!    MetaVesselTube (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaVesselTubeFiles.
 *
 * \author Julien Jomier
 *
 * \date May 22, 2002
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT VesselTubePnt
{
public:

  VesselTubePnt(int dim);
  ~VesselTubePnt();

  unsigned int m_Dim;
  float* m_V1;
  float* m_V2;
  float* m_X;
  float* m_T;
  float m_Alpha1;
  float m_Alpha2;
  float m_Alpha3;
  float m_R;
  float m_Medialness;
  float m_Ridgeness;
  float m_Branchness;
  bool  m_Mark;
  float m_Color[4];
  int   m_ID;
};




class METAIO_EXPORT MetaVesselTube : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef METAIO_STL::list<VesselTubePnt*> PointListType;
    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaVesselTube(void);

    MetaVesselTube(const char *_headerName);

    MetaVesselTube(const MetaVesselTube *_VesselTube);

    MetaVesselTube(unsigned int dim);

    ~MetaVesselTube(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    //    NPoints(...)
    //       Required Field
    //       Number of points wich compose the VesselTube
    void  NPoints(int npnt);
    int   NPoints(void) const;

    //    PointDim(...)
    //       Required Field
    //       Definition of points
    void        PointDim(const char* pointDim);
    const char* PointDim(void) const;

    //    Root(...)
    //       Optional Field
    //       Set if this VesselTube is a root
    void  Root(bool root);
    bool  Root(void) const;

    //    Artery(...)
    //       Optional Field
    //       Set if this VesselTube is a root
    void  Artery(bool artery);
    bool  Artery(void) const;


    //    ParentPoint(...)
    //       Optional Field
    //       Set the point number of the parent VesselTube where the branch occurs
    void  ParentPoint(int parentpoint);
    int   ParentPoint(void) const;

    void  Clear(void);

    PointListType &  GetPoints(void) {return m_PointList;}
    const PointListType &  GetPoints(void) const {return m_PointList;}

    MET_ValueEnumType ElementType(void) const;
    void  ElementType(MET_ValueEnumType _elementType);

  ////
  //
  // PROTECTED
  //
  ////
  protected:

    bool  m_ElementByteOrderMSB;

    void  M_Destroy(void);

    void  M_SetupReadFields(void);

    void  M_SetupWriteFields(void);

    bool  M_Read(void);

    bool  M_Write(void);

    int   m_ParentPoint;  // "ParentPoint = "     -1

    bool  m_Root;         // "Root = "            false

    bool  m_Artery;         // "Artery = "            true

    int   m_NPoints;      // "NPoints = "         0

    char m_PointDim[255]; // "PointDim = "       "x y z r"

    PointListType m_PointList;
    MET_ValueEnumType m_ElementType;
  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
