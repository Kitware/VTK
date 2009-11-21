/*=========================================================================

  Program:   MetaIO
  Module:    metaObject.h
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

#ifndef ITKMetaIO_METAOBJECT_H
#define ITKMetaIO_METAOBJECT_H

#include "metaUtils.h"
#include "metaEvent.h"

#ifdef _MSC_VER
#pragma warning ( disable: 4251 )
#endif


#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class METAIO_EXPORT MetaObject
  {
  ////
  //
  // PROTECTED
  //
  ////
  protected:

      typedef METAIO_STL::vector<MET_FieldRecordType *> FieldsContainerType;

      METAIO_STREAM::ifstream * m_ReadStream;
      METAIO_STREAM::ofstream * m_WriteStream;

      FieldsContainerType m_Fields;
      FieldsContainerType m_UserDefinedWriteFields;
      FieldsContainerType m_UserDefinedReadFields;

      char  m_FileName[255];

      char  m_Comment[255];            // "Comment = "       ""

      char  m_ObjectTypeName[255];     // "ObjectType = "    defined by suffix
      char  m_ObjectSubTypeName[255];  // "ObjectSubType = " defined by suffix

      int   m_NDims;                   // "NDims = "         required

      double m_Offset[10];             // "Offset = "          0,0,0
      double m_TransformMatrix[100];   // "TransformMatrix = " 1,0,0,0,1,0,0,0,1
      double m_CenterOfRotation[10];   // "CenterOfRotation = "  0 0 0

      MET_OrientationEnumType m_AnatomicalOrientation[10];

      MET_DistanceUnitsEnumType m_DistanceUnits;   // "DistanceUnits = mm"

      float m_ElementSpacing[10];   // "ElementSpacing = "   0,0,0

      float m_Color[4];             // "Color = "            1.0, 0.0, 0.0, 1.0

      char  m_AcquisitionDate[255]; // "AcquisitionDate = "  "2007.03.21"
 
      int   m_ID;                   // "ID = "               0

      int   m_ParentID;             // "ParentID = "         -1

      char  m_Name[255];            // "Name = "             ""

      bool  m_BinaryData;           // "BinaryData = "      False

      bool  m_BinaryDataByteOrderMSB;

      METAIO_STL::streamoff m_CompressedDataSize;
      // Used internally to set if the dataSize should be written
      bool m_WriteCompressedDataSize; 
      bool m_CompressedData;
      
      virtual void M_Destroy(void);

      virtual void M_SetupReadFields(void);

      virtual void M_SetupWriteFields(void);

      virtual bool M_Read(void);

      virtual bool M_Write(void);
    
      virtual void M_PrepareNewReadStream();

      MetaEvent*     m_Event;
      //MET_FieldRecordType * M_GetFieldRecord(const char * _fieldName);
      //int   M_GetFieldRecordNumber(const char * _fieldName);

      unsigned int m_DoublePrecision;

  /////
  //
  // PUBLIC
  //
  ////
  public:

      ////
      // Constructors & Destructor
      ////
      MetaObject(void);
      MetaObject(const char * _fileName);
      MetaObject(unsigned int dim);

      virtual ~MetaObject(void);

      void  FileName(const char *_fileName);
      const char  * FileName(void) const;

      virtual void  CopyInfo(const MetaObject * _object);

      bool  Read(const char * _fileName=NULL);

      bool  ReadStream(int _nDims, METAIO_STREAM::ifstream * _stream);

      bool  Write(const char * _fileName=NULL);

      virtual bool  Append(const char *_headName=NULL);

      ////
      //
      // Common fields
      //
      ////

      //    PrintMetaInfo()
      //       Writes image parameters to stdout
      virtual void  PrintInfo(void) const;

      //    Comment(...)
      //       Optional Field
      //       Arbitrary string
      const char  * Comment(void) const;
      void    Comment(const char * _comment);

      const char  * ObjectTypeName(void) const;
      void    ObjectTypeName(const char * _objectTypeName);
      const char  * ObjectSubTypeName(void) const;
      void    ObjectSubTypeName(const char * _objectSubTypeName);

      //    NDims()
      //       REQUIRED Field
      //       Number of dimensions to the image
      int   NDims(void) const;

      //    Offset(...)
      //       Optional Field
      //       Physical location (in millimeters and wrt machine coordinate
      //         system or the patient) of the first element in the image
      const double * Offset(void) const;
      double Offset(int _i) const;
      void  Offset(const double * _position);
      void  Offset(int _i, double _value);
      const double * Position(void) const;
      double Position(int _i) const;
      void  Position(const double * _position);
      void  Position(int _i, double _value);
      const double * Origin(void) const;
      double Origin(int _i) const;
      void  Origin(const double * _position);
      void  Origin(int _i, double _value);

      //    TransformMatrix(...)
      //       Optional Field
      //       Physical orientation of the object as an NDims x NDims matrix
      const double * TransformMatrix(void) const;
      double TransformMatrix(int _i, int _j) const;
      void  TransformMatrix(const double * _orientation);
      void  TransformMatrix(int _i, int _j, double _value);
      const double * Rotation(void) const;
      double Rotation(int _i, int _j) const;
      void  Rotation(const double * _orientation);
      void  Rotation(int _i, int _j, double _value);
      const double * Orientation(void) const;
      double Orientation(int _i, int _j) const;
      void  Orientation(const double * _orientation);
      void  Orientation(int _i, int _j, double _value);

      //
      //
      //
      const double * CenterOfRotation(void) const;
      double CenterOfRotation(int _i) const;
      void  CenterOfRotation(const double * _position);
      void  CenterOfRotation(int _i, double _value);

      //
      //
      //
      const char * DistanceUnitsName(void) const;
      MET_DistanceUnitsEnumType DistanceUnits(void) const;
      void DistanceUnits(MET_DistanceUnitsEnumType _distanceUnits);
      void DistanceUnits(const char * _distanceUnits);

      const char * AnatomicalOrientationAcronym(void) const;
      const MET_OrientationEnumType * AnatomicalOrientation(void) const;
      MET_OrientationEnumType AnatomicalOrientation(int _dim) const;
      void AnatomicalOrientation(const char *_ao);
      void AnatomicalOrientation(const MET_OrientationEnumType *_ao);
      void AnatomicalOrientation(int _dim, MET_OrientationEnumType _ao);
      void AnatomicalOrientation(int _dim, char ao);

      
      //    ElementSpacing(...)
      //       Optional Field
      //       Physical Spacing (in same units as position)
      const float * ElementSpacing(void) const;
      float ElementSpacing(int _i) const;
      void  ElementSpacing(const float * _elementSpacing);
      void  ElementSpacing(int _i, float _value);

      //    Name(...)
      //       Optional Field
      //       Name of the current metaObject
      void  Name(const char *_Name);
      const char  * Name(void) const;

      //    Color(...)
      //       Optional Field
      //       Color of the current metaObject   
      const float * Color(void) const;
      void  Color(float _r, float _g, float _b, float _a);
      void  Color(const float * _color);    
 
      //    ID(...)
      //       Optional Field
      //       ID number of the current metaObject
      void ID(int _id);
      int  ID(void) const;

      //    ParentID(...)
      //       Optional Field
      //       ID number of the parent  metaObject
      void  ParentID(int _parentId);
      int   ParentID(void) const;

      //    AcquisitionDate(...)
      //       Optional Field
      //       YYYY.MM.DD is the recommended format
      void  AcquisitionDate(const char * _acquisitionDate);
      const char *  AcquisitionDate(void) const;

      //    BinaryData(...)
      //       Optional Field
      //       Data is binary or not
      void  BinaryData(bool _binaryData);
      bool  BinaryData(void) const;

      void  BinaryDataByteOrderMSB(bool _binaryDataByteOrderMSB);
      bool  BinaryDataByteOrderMSB(void) const;


      void  CompressedData(bool _compressedData);
      bool  CompressedData(void) const;


      virtual void Clear(void);

      void ClearFields(void);

      bool InitializeEssential(int m_NDims);

      //
      //
      // User's field definitions 
      bool AddUserField(const char* _fieldName, MET_ValueEnumType _type,
                        int _length=0, bool _required=true,
                        int _dependsOn=-1);

      // Add a user's field
      template <class T>
      bool AddUserField(const char* _fieldName, MET_ValueEnumType _type,
                        int _length, T *_v, bool _required=true,
                        int _dependsOn=-1 )
        {
        MET_FieldRecordType* mFw = new MET_FieldRecordType;
        MET_InitWriteField(mFw, _fieldName, _type, _length,_v);
        m_UserDefinedWriteFields.push_back(mFw);

        MET_FieldRecordType* mFr = new MET_FieldRecordType;
        MET_InitReadField(mFr,_fieldName, _type, _required,_dependsOn,_length);
        m_UserDefinedReadFields.push_back(mFr);

        return true;
        }

      // Clear UserFields
      void ClearUserFields();

      // Get the user field
      void* GetUserField(const char* _name);
      void SetEvent(MetaEvent* event) {m_Event = event;}

      // Set the double precision for writing
      void SetDoublePrecision(unsigned int precision) 
        {
        m_DoublePrecision = precision;
        }
      unsigned int GetDoublePrecision() 
        {
        return m_DoublePrecision;
        }

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
