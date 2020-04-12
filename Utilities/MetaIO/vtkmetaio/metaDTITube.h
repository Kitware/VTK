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


#ifndef ITKMetaIO_METADTITUBE_H
#define ITKMetaIO_METADTITUBE_H


#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable: 4251 )
#endif

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaDTITube (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaDTITubeFiles.
 *
 * \author Julien Jomier
 *
 * \date May 22, 2002
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT DTITubePnt
{
public:

  typedef std::pair<std::string,float>  FieldType;
  typedef std::vector<FieldType>        FieldListType;

  DTITubePnt(int dim);

  ~DTITubePnt();

  const FieldListType & GetExtraFields() const;

  void AddField(const char* name, float value);

  float GetField(const char* name) const;

  unsigned int m_Dim;
  float* m_X;
  float* m_TensorMatrix;

  FieldListType m_ExtraFields;
};




class METAIO_EXPORT MetaDTITube : public MetaObject
{

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef std::list<DTITubePnt*> PointListType;
   typedef std::pair<std::string,unsigned int> PositionType;

   ////
    //
    // Constructors & Destructor
    //
    ////
    MetaDTITube(void);

    MetaDTITube(const char *_headerName);

    MetaDTITube(const MetaDTITube *_DTITube);

    MetaDTITube(unsigned int dim);

    ~MetaDTITube(void) override;

    void PrintInfo(void) const override;

    void CopyInfo(const MetaObject * _object) override;

    //    NPoints(...)
    //       Required Field
    //       Number of points which compose the DTITube
    void  NPoints(int npnt);
    int   NPoints(void) const;

    //    PointDim(...)
    //       Required Field
    //       Definition of points
    void        PointDim(const char* pointDim);
    const char* PointDim(void) const;

    //    Root(...)
    //       Optional Field
    //       Set if this DTITube is a root
    void  Root(bool root);
    bool   Root(void) const;


    //    ParentPoint(...)
    //       Optional Field
    //       Set the point number of the parent DTITube where the branch occurs
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

    int m_ParentPoint;  // "ParentPoint = "     -1

    bool m_Root;         // "Root = "            False

    int m_NPoints;      // "NPoints = "         0

    std::string m_PointDim; // "PointDim = "       "x y z r"

    PointListType m_PointList;
    MET_ValueEnumType m_ElementType;
    std::vector<PositionType> m_Positions;

    int GetPosition(const char*) const;
};

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
