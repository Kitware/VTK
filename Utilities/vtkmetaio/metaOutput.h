/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    metaOutput.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __MetaOutput_H_
#define __MetaOutput_H_

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

#include "metaCommand.h"
#include <stdio.h>
#include <string>
#include <fstream>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class MetaOutputStream
{
public:
 
  MetaOutputStream() 
    {
    m_Enable = true;
    m_IsStdStream = false;
    m_Name = "";
    m_MetaOutput = NULL;
    };

  virtual ~MetaOutputStream() {};

  void SetName(const char* name)
    {
    m_Name = name;
    }
  void Enable() {m_Enable = true;}
  void Disable() {m_Enable = false;}
  void SetStdStream(METAIO_STL::ostream * stream)
    {
    m_StdStream = stream;
    m_IsStdStream = true;
    }
  bool IsStdStream()
    {
    return m_IsStdStream;
    }

  METAIO_STL::ostream * GetStdStream()
    {
    return m_StdStream;
    }

  METAIO_STL::string GetName() const
    {
    return m_Name;
    }

  bool IsEnable() const 
    {
    return m_Enable;
    }

  virtual bool Write(const char* buffer) 
    {
    if(m_IsStdStream)
      {
      *m_StdStream << buffer;
      }
    return true;
    };

  virtual bool Open() 
    {
    m_IsOpen = true;
    return true;
    }
  
  virtual bool Close() 
    {
    m_IsOpen = false;
    return true;
    }

  void SetMetaOutput(void* metaOutput)
    {
    m_MetaOutput = metaOutput;
    }

protected:

  METAIO_STL::ostream * m_StdStream;
  bool m_IsStdStream;
  bool m_Enable;
  bool m_IsOpen;
  METAIO_STL::string m_Name;
  void* m_MetaOutput;

};

class MetaFileOutputStream : public MetaOutputStream
{
public:

  MetaFileOutputStream(const char* name) : MetaOutputStream()
    {
    m_FileName = name;
    this->SetStdStream(&m_FileStream);
    }
  virtual ~MetaFileOutputStream() {};

  bool Open()
    {
    MetaOutputStream::Open();
    m_FileStream.open(m_FileName.c_str(), METAIO_STL::ios::binary | METAIO_STL::ios::out);
    return m_FileStream.is_open();
    }

  bool Close()
    {
    MetaOutputStream::Close();
    m_FileStream.close();
    return true;
    }

  METAIO_STL::string GetFileName()
    {
    return m_FileName;
    }

private:
  
  METAIO_STL::string   m_FileName;
  METAIO_STL::ofstream m_FileStream;
};

class METAIO_EXPORT MetaOutput
{
public:

  typedef enum {INT,FLOAT,CHAR,STRING,LIST,FLAG,BOOL} TypeEnumType;

  struct Field{
    METAIO_STL::string  name;
    METAIO_STL::string  description;
    METAIO_STL::string  value;
    TypeEnumType type;
    METAIO_STL::string  rangeMin;
    METAIO_STL::string  rangeMax;
    };

  typedef METAIO_STL::vector<Field>              FieldVector;
  typedef METAIO_STL::vector<MetaOutputStream*>  StreamVector;
  
  MetaOutput();
  ~MetaOutput();

  /** Add a field */
  bool AddField(METAIO_STL::string name,
                METAIO_STL::string description,
                TypeEnumType type,
                METAIO_STL::string value,
                METAIO_STL::string rangeMin = "",
                METAIO_STL::string rangeMax = ""
                );

  bool AddFloatField(METAIO_STL::string name,
                     METAIO_STL::string description,
                     float value,
                     METAIO_STL::string rangeMin = "",
                     METAIO_STL::string rangeMax = ""
                     );

  bool AddIntField(METAIO_STL::string name,
                     METAIO_STL::string description,
                     int value,
                     METAIO_STL::string rangeMin = "",
                     METAIO_STL::string rangeMax = ""
                     );

  /** Set the metaCommand for parsing */
  void SetMetaCommand(MetaCommand* metaCommand);

  /** Write the output to the connected streams */
  void Write();

  /** Add a standard stream */
  void AddStream(const char* name,METAIO_STL::ostream & stream);
  void AddStream(const char* name,MetaOutputStream * stream);

  /** Add a stream file. Helper function */
  void AddStreamFile(const char* name, const char* filename);

  /** Enable or Disable a stream */
  void EnableStream(const char* name);
  void DisableStream(const char* name);
  
  METAIO_STL::string GetHostname(void);
  METAIO_STL::string GetHostip(void);

private:
  
  METAIO_STL::string TypeToString(TypeEnumType type);

  /** Private function to fill in the buffer */
  METAIO_STL::string GenerateXML(const char* filename=NULL);
  METAIO_STL::string GetUsername(void);

  FieldVector   m_FieldVector;
  MetaCommand*  m_MetaCommand;
  StreamVector  m_StreamVector;
  METAIO_STL::string   m_CurrentVersion;

}; // end of class

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif 
