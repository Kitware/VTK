/*=========================================================================

  Program:   MetaIO
  Module:    metaDTITube.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaTypes.h"


#ifndef ITKMetaIO_METADTITUBE_H
#define ITKMetaIO_METADTITUBE_H


#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
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

  typedef METAIO_STL::pair<METAIO_STL::string,float>  FieldType;
  typedef METAIO_STL::vector<FieldType>        FieldListType;

  const FieldListType & GetExtraFields() const {return m_ExtraFields;}
  void AddField(const char* name, float value)
    {
    FieldType field(name,value);
    m_ExtraFields.push_back(field);
    }
  float GetField(const char* name) const
    {
    FieldListType::const_iterator it = m_ExtraFields.begin();
    while(it != m_ExtraFields.end())
      {
      if(!strcmp((*it).first.c_str(),name))
        {
        return (*it).second;
        }
      ++it;
      }
    return -1;
    }

  DTITubePnt(int dim)
  { 
    m_Dim = dim;
    m_X = new float[m_Dim];
    m_TensorMatrix = new float[6];
  
    unsigned int i=0;
    for(i=0;i<m_Dim;i++)
      {
      m_X[i] = 0;
      }
    
    // Initialize the tensor matrix to identity
    for(i=0;i<6;i++)
      {
      m_TensorMatrix[i] = 0;
      }
    m_TensorMatrix[0] = 1;
    m_TensorMatrix[3] = 1;
    m_TensorMatrix[5] = 1;
  }

  ~DTITubePnt()
  {
    delete []m_X;
    delete []m_TensorMatrix;
    m_ExtraFields.clear();
  };
  
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

   typedef METAIO_STL::list<DTITubePnt*> PointListType;
   typedef METAIO_STL::pair<METAIO_STL::string,unsigned int> PositionType;

   ////
    //
    // Constructors & Destructor
    //
    ////
    MetaDTITube(void);

    MetaDTITube(const char *_headerName);   

    MetaDTITube(const MetaDTITube *_DTITube); 
    
    MetaDTITube(unsigned int dim);

    ~MetaDTITube(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    //    NPoints(...)
    //       Required Field
    //       Number of points wich compose the DTITube
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

    int m_ParentPoint;  // "ParentPoint = "     -1

    bool m_Root;         // "Root = "            False

    int m_NPoints;      // "NPoints = "         0

    METAIO_STL::string m_PointDim; // "PointDim = "       "x y z r"

    PointListType m_PointList;
    MET_ValueEnumType m_ElementType;
    METAIO_STL::vector<PositionType> m_Positions;

    int GetPosition(const char*) const;
  };

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
