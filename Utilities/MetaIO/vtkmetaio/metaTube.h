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

#ifndef ITKMetaIO_METATUBE_H
#define ITKMetaIO_METATUBE_H

#include "metaUtils.h"
#include "metaObject.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif

#include <list>


/*!    MetaTube (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaTubeFiles.
 *
 * \author Julien Jomier
 *
 * \date May 22, 2002
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT TubePnt
{
public:

  TubePnt(int dim);
  ~TubePnt();

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
  float m_Curvature;
  float m_Levelness;
  float m_Roundness;
  float m_Intensity;
  bool  m_Mark;
  float m_Color[4];
  int   m_ID;
};




class METAIO_EXPORT MetaTube : public MetaObject
{

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef std::list<TubePnt*> PointListType;
    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaTube(void);

    MetaTube(const char *_headerName);

    MetaTube(const MetaTube *_Tube);

    MetaTube(unsigned int dim);

    ~MetaTube(void) override;

    void PrintInfo(void) const override;

    void CopyInfo(const MetaObject * _object) override;

    //    NPoints(...)
    //       Required Field
    //       Number of points which compose the Tube
    void  NPoints(int npnt);
    int   NPoints(void) const;

    //    PointDim(...)
    //       Required Field
    //       Definition of points
    void        PointDim(const char* pointDim);
    const char* PointDim(void) const;

    //    Root(...)
    //       Optional Field
    //       Set if this Tube is a root
    void  Root(bool root);
    bool  Root(void) const;

    //    Artery(...)
    //       Optional Field
    //       Set if this Tube is a root
    void  Artery(bool artery);
    bool  Artery(void) const;


    //    ParentPoint(...)
    //       Optional Field
    //       Set the point number of the parent Tube where the branch occurs
    void  ParentPoint(int parentpoint);
    int   ParentPoint(void) const;

    void  Clear(void) override;

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

    void  M_Destroy(void) override;

    void  M_SetupReadFields(void) override;

    void  M_SetupWriteFields(void) override;

    bool  M_Read(void) override;

    bool  M_Write(void) override;

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
