/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformVariables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUniformVariables.h"
#include "vtkgl.h"
#include <assert.h>
#include "vtkObjectFactory.h"

#include <vtksys/stl/map>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUniformVariables);

class ltstr
{
public:
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

class vtkUniform
{
public:
  
  // just because dynamic__cast is forbidden in VTK. Sucks.
  enum
  {
    ClassTypeVectorInt=0,
    ClassTypeVectorFloat=1,
    ClassTypeMatrix=2,
    ClassTypeArrayInt=3,
    ClassTypeArrayFloat=4
  };
  
  int GetClassType() const
    {
      return this->ClassType;
    }
  
  vtkUniform()
    {
      this->ClassType=-1;
      this->Name=0;
    }
  
  const char *GetName() const
    {
      return this->Name;
    }
  
  void SetName(const char *n)
    {
      if(this->Name==0 && n==0)
        {
        return;
        }
      if(this->Name!=0 && n!=0 && strcmp(this->Name,n)==0)
        {
        return;
        }
      if(this->Name!=0)
        {
        delete[] this->Name;
        }
      if(n!=0) // copy
        {
         size_t l=strlen(n)+1;
         this->Name=new char[l];
         strncpy(this->Name,n,l);
        }
      else
        {
        this->Name=0;
        }
    }
  
  virtual ~vtkUniform()
    {
      if(this->Name!=0)
        {
        delete[] Name;
        }
    }
  
  virtual void Send(int location)=0;
  
  virtual void PrintSelf(ostream &os, vtkIndent indent)=0;
  
  virtual vtkUniform *Clone() const=0;
  
protected:
  char *Name;
  
  int ClassType; // just because dynamic__cast is forbidden in VTK. Sucks.
};

class vtkUniformVectorInt : public vtkUniform
{
public:
  vtkUniformVectorInt(int size,
                      int *values)
    {
      this->ClassType=ClassTypeVectorInt;
      this->Size=size;
      this->Values=new int[size];
      int i=0;
      while(i<size)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  virtual ~vtkUniformVectorInt()
    {
      delete[] this->Values;
    }
  
  int GetSize()
    {
      return this->Size;
    }
  
  void SetValues(int *values)
    {
      int i=0;
      while(i<this->Size)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  const int *GetValues()
    {
     return this->Values;
    }

   virtual void Send(int location)
    {
      switch(this->Size)
        {
        case 1:
          vtkgl::Uniform1i(location,this->Values[0]);
          break;
        case 2:
          vtkgl::Uniform2i(location,this->Values[0],this->Values[1]);
          break;
        case 3:
          vtkgl::Uniform3i(location,this->Values[0],this->Values[1],
                           this->Values[2]);
          break;
        case 4:
          vtkgl::Uniform4i(location,this->Values[0],this->Values[1],
                           this->Values[2],this->Values[3]);
          break;
        }
    }
  
  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
      os << indent << this->Name << " (uniform" << this->Size << "i): ";
      int i=0;
      while(i<this->Size)
        {
        os << this->Values[i];
        if(i<(this->Size-1))
          {
          os <<",";
          }
        ++i;
        }
      os << endl;
    }
  
  virtual vtkUniform *Clone() const
    {
      vtkUniformVectorInt *result=new vtkUniformVectorInt(this->Size,
                                                          this->Values);
      result->SetName(this->Name);
      return result;
    }
  
protected:
  int Size;
  int *Values;
};

class vtkUniformVectorFloat : public vtkUniform
{
public:
  vtkUniformVectorFloat(int size,
                        float *values)
    {
      this->ClassType=ClassTypeVectorFloat;
      this->Size=size;
      this->Values=new float[size];
      int i=0;
      while(i<size)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  virtual ~vtkUniformVectorFloat()
    {
      delete[] this->Values;
    }
  
  int GetSize()
    {
      return this->Size;
    }
  
  void SetValues(float *values)
    {
      int i=0;
      while(i<this->Size)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  const float *GetValues()
    {
      return this->Values;
    }
  
  virtual void Send(int location)
    {
      switch(this->Size)
        {
        case 1:
          vtkgl::Uniform1f(location,this->Values[0]);
          break;
        case 2:
          vtkgl::Uniform2f(location,this->Values[0],this->Values[1]);
          break;
        case 3:
          vtkgl::Uniform3f(location,this->Values[0],this->Values[1],
                           this->Values[2]);
          break;
        case 4:
          vtkgl::Uniform4f(location,this->Values[0],this->Values[1],
                           this->Values[2],this->Values[3]);
          break;
        }
    }
  
  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
      os << indent << this->Name << " (uniform" << this->Size << "f): ";
      int i=0;
      while(i<this->Size)
        {
        os << this->Values[i];
        if(i<(this->Size-1))
          {
          os <<",";
          }
        ++i;
        }
      os << endl;
    }
  
  virtual vtkUniform *Clone() const
    { 
      vtkUniformVectorFloat *result=new vtkUniformVectorFloat(this->Size,
                                                              this->Values);
      result->SetName(this->Name);
      return result;
    }
  
protected:
  int Size;
  float *Values;
};

class vtkUniformArrayInt : public vtkUniform
{
public:
  vtkUniformArrayInt(int size,
                     int arraySize,
                     int *values)
    {
      this->ClassType=ClassTypeArrayInt;
      this->Size=size;
      this->ArraySize=arraySize;
      this->Values=new GLint[size*arraySize];
      int i=0;
      while(i<this->Size*this->ArraySize)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  // this constructor is used inside Clone()
  // It is required because GLint is long on Mac, not int.
  vtkUniformArrayInt(int size,
                     int arraySize,
                     const GLint *values)
    {
      this->ClassType=ClassTypeArrayInt;
      this->Size=size;
      this->ArraySize=arraySize;
      this->Values=new GLint[size*arraySize];
      int i=0;
      while(i<this->Size*this->ArraySize)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  virtual ~vtkUniformArrayInt()
    {
      delete[] this->Values;
    }
  
  int GetSize()
    {
      return this->Size;
    }
  
  int GetArraySize()
    {
      return this->ArraySize;
    }
  
  void SetValues(int *values)
    {
      int i=0;
      while(i<this->Size*this->ArraySize)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  const GLint *GetValues()
    {
      return this->Values;
    }
  
  virtual void Send(int location)
    {
      switch(this->Size)
        {
        case 1:
          vtkgl::Uniform1iv(location,this->ArraySize,this->Values);
          break;
        case 2:
          vtkgl::Uniform2iv(location,this->ArraySize,this->Values);
          break;
        case 3:
          vtkgl::Uniform3iv(location,this->ArraySize,this->Values);
          break;
        case 4:
          vtkgl::Uniform4iv(location,this->ArraySize,this->Values);
          break;
        }
    }
  
  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
      os << indent << this->Name << " (uniform" << this->Size << "iv[" << this->ArraySize << "]): ";
      int j=0;
      while(j<this->ArraySize)
        {
        os << "(";
        int i=0;
        while(i<this->Size)
          {
          os << this->Values[i];
          if(i<(this->Size-1))
            {
            os <<",";
            }
          ++i;
          }
        os << endl;
        ++j;
        }
    }
  
  virtual vtkUniform *Clone() const
    {
      vtkUniformArrayInt *result=new vtkUniformArrayInt(this->Size,
                                                        this->ArraySize,
                                                        this->Values);
      result->SetName(this->Name);
      return result;
    }
  
protected:
  int Size;  // size of element (eq. to float, vec2, vec2, vec4)
  int ArraySize; // number of elements
  GLint *Values; // GLint is long on Mac
};

class vtkUniformArrayFloat : public vtkUniform
{
public:
  vtkUniformArrayFloat(int size,
                       int arraySize,
                       float *values)
    {
      this->ClassType=ClassTypeArrayFloat;
      this->Size=size;
      this->ArraySize=arraySize;
      this->Values=new float[size*arraySize];
      int i=0;
      while(i<this->Size*this->ArraySize)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  virtual ~vtkUniformArrayFloat()
    {
      delete[] this->Values;
    }
  
  int GetSize()
    {
      return this->Size;
    }
  
  int GetArraySize()
    {
      return this->ArraySize;
    }
  
  void SetValues(float *values)
    {
      int i=0;
      while(i<this->Size*this->ArraySize)
        {
        this->Values[i]=values[i];
        ++i;
        }
    }
  
  const float *GetValues()
    {
      return this->Values;
    }
  
  virtual void Send(int location)
    {
      switch(this->Size)
        {
        case 1:
          vtkgl::Uniform1fv(location,this->ArraySize,this->Values);
          break;
        case 2:
          vtkgl::Uniform2fv(location,this->ArraySize,this->Values);
          break;
        case 3:
          vtkgl::Uniform3fv(location,this->ArraySize,this->Values);
          break;
        case 4:
          vtkgl::Uniform4fv(location,this->ArraySize,this->Values);
          break;
        }
    }
  
  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
      os << indent << this->Name << " (uniform" << this->Size << "fv[" << this->ArraySize << "]): ";
      int j=0;
      while(j<this->ArraySize)
        {
        os << "(";
        int i=0;
        while(i<this->Size)
          {
          os << this->Values[i];
          if(i<(this->Size-1))
            {
            os <<",";
            }
          ++i;
          }
        os << endl;
        ++j;
        }
    }
  
  virtual vtkUniform *Clone() const
    {
      vtkUniformArrayFloat *result=new vtkUniformArrayFloat(this->Size,
                                                            this->ArraySize,
                                                            this->Values);
      result->SetName(this->Name);
      return result;
    }
  
protected:
  int Size;  // size of element (eq. to float, vec2, vec2, vec4)
  int ArraySize; // number of elements
  float *Values;
};

// rows or columns are 2,3,4.
class vtkUniformMatrix : public vtkUniform
{
public:
  vtkUniformMatrix(int rows,
                   int columns,
                   float *values)
    {
      this->ClassType=ClassTypeMatrix;
      this->Rows=rows;
      this->Columns=columns;
      this->Values=new float[rows*columns];
      int i=0;
      while(i<rows)
        {
        int j=0;
        while(j<columns)
          {
          int index=i*columns+j;
          this->Values[index]=values[index];
          ++j;
          }
        ++i;
        }
    }
  
  int GetRows()
    {
      return this->Rows;
    }
  
  int GetColumns()
    {
      return this->Columns;
    }
  
  void SetValues(float *values)
    {
      int i=0;
      while(i<this->Rows)
        {
        int j=0;
        while(j<this->Columns)
          {
          int index=i*this->Columns+j;
          this->Values[index]=values[index];
          ++j;
          }
        ++i;
        }
    }
  
  const float* GetValues()
    {
      return this->Values;
    }

  virtual void Send(int location)
    {
      switch(this->Rows)
        {
        case 2:
          switch(this->Columns)
            {
            case 2:
              vtkgl::UniformMatrix2fv(location,1,GL_FALSE,this->Values);
              break;
            case 3:
              vtkgl::UniformMatrix2x3fv(location,1,GL_FALSE,this->Values);
              break;
            case 4:
              vtkgl::UniformMatrix2x4fv(location,1,GL_FALSE,this->Values);
              break;
            }
          break;
        case 3:
           switch(this->Columns)
            {
            case 2:
              vtkgl::UniformMatrix3x2fv(location,1,GL_FALSE,this->Values);
              break;
            case 3:
              vtkgl::UniformMatrix3fv(location,1,GL_FALSE,this->Values);
              break;
            case 4:
              vtkgl::UniformMatrix3x4fv(location,1,GL_FALSE,this->Values);
              break;
            }
           break;
        case 4:
          switch(this->Columns)
            {
            case 2:
              vtkgl::UniformMatrix4x2fv(location,1,GL_FALSE,this->Values);
              break;
            case 3:
              vtkgl::UniformMatrix4x3fv(location,1,GL_FALSE,this->Values);
              break;
            case 4:
              vtkgl::UniformMatrix4fv(location,1,GL_FALSE,this->Values);
              break;
            }
          break;
        }
    }
  
  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
      os << indent << this->Name << " (matrix " << this->Rows << "x"
         << this->Columns << "): ";
      
      int i=0;
      while(i<this->Rows)
        {
        int j=0;
        while(j<this->Columns)
          {
          int index=i*this->Columns+j;
          os << this->Values[index];
          if(j<(this->Columns-1))
            os << ",";
          ++j;
          }
        os << endl;
        ++i;
        }
    }
  
  virtual vtkUniform *Clone() const
    {
      vtkUniformMatrix *result=new vtkUniformMatrix(this->Rows,this->Columns,
                                                    this->Values);
      result->SetName(this->Name);
      return result;
    }
  
protected:
  float *Values;
  int Rows;
  int Columns;
};

typedef vtksys_stl::map<const char *, vtkUniform *, ltstr> UniformMap;
typedef vtksys_stl::map<const char *, vtkUniform *, ltstr>::iterator UniformMapIt;

class vtkUniformVariablesMap
{
public:
  UniformMap Map;
  UniformMapIt It; // used for external iteration.
  
  ~vtkUniformVariablesMap()
    {
      UniformMapIt i=this->Map.begin();
      UniformMapIt e=this->Map.end();
      while(i!=e)
        {
        vtkUniform *u=(*i).second;
        if(u!=0)
          {
          delete u;
          }
        ++i;
        }
    }
};

// ----------------------------------------------------------------------------
vtkUniformVariables::vtkUniformVariables()
{
  this->Map=new vtkUniformVariablesMap;
}

// ----------------------------------------------------------------------------
vtkUniformVariables::~vtkUniformVariables()
{
  delete this->Map;
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::SetUniformi(const char *name,
                                      int numberOfComponents,
                                      int *value)
{
  assert("pre: name_exists" && name!=0);
  assert("pre: value_exists" && value!=0);
  assert("pre: valid_numberOfComponents" && numberOfComponents>=1 && numberOfComponents<=4);

  UniformMapIt cur=this->Map->Map.find(name);
  
  if(cur!=this->Map->Map.end())
    {
    vtkUniform *u=(*cur).second;
    vtkUniformVectorInt *ui=0;
    if(u->GetClassType()==vtkUniform::ClassTypeVectorInt)
      {
      ui=static_cast<vtkUniformVectorInt *>(u);
      }
    if(ui==0)
      {
      vtkErrorMacro(<<"try to overwrite a value with different type.");
      }
    else
      {
      if(ui->GetSize()!=numberOfComponents)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of components.");
        }
      else
        {
        bool changed=false;
        int i=0;
        while(!changed && i<numberOfComponents)
          {
          changed=value[i]!=ui->GetValues()[i];
          ++i;
          }
        if(changed)
          {
          ui->SetValues(value);
          this->Modified();
          }
        }
      }
    }
  else
    {
    vtkUniform *u=0;
    u=new vtkUniformVectorInt(numberOfComponents,value);
    u->SetName(name);
  
    vtksys_stl::pair<const char *, vtkUniform *> p;
    p.first=u->GetName(); // cannot be `name' because
    // we don't manage this pointer.
    p.second=u;
    
    this->Map->Map.insert(p);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::SetUniformf(const char *name,
                                      int numberOfComponents,
                                      float *value)
{
  assert("pre: name_exists" && name!=0);
  assert("pre: value_exists" && value!=0);
  assert("pre: valid_numberOfComponents" && numberOfComponents>=1 && numberOfComponents<=4);

  UniformMapIt cur=this->Map->Map.find(name);
  
  if(cur!=this->Map->Map.end())
    {
    vtkUniform *u=(*cur).second;
    vtkUniformVectorFloat *uf=0;
    if(u->GetClassType()==vtkUniform::ClassTypeVectorFloat)
      {
      uf=static_cast<vtkUniformVectorFloat *>(u);
      }
    if(uf==0)
      {
      vtkErrorMacro(<<"try to overwrite a value with different type.");
      }
    else
      {
      if(uf->GetSize()!=numberOfComponents)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of components.");
        }
      else
        {
        bool changed=false;
        int i=0;
        while(!changed && i<numberOfComponents)
          {
          changed=value[i]!=uf->GetValues()[i];
          ++i;
          }
        if(changed)
          {
          uf->SetValues(value);
          this->Modified();
          }
        }
      }
    }
  else
    {
    vtkUniform *u=0;
    u=new vtkUniformVectorFloat(numberOfComponents,value);
    u->SetName(name);
    
    vtksys_stl::pair<const char *, vtkUniform *> p;
    p.first=u->GetName(); // cannot be `name' because
    // we don't manage this pointer.
    p.second=u;
    
    this->Map->Map.insert(p);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::SetUniformiv(const char *name,
                                       int numberOfComponents,
                                       int numberOfElements,
                                       int *value)
{
  assert("pre: name_exists" && name!=0);
  assert("pre: value_exists" && value!=0);
  assert("pre: valid_numberOfComponents" && numberOfComponents>=1 && numberOfComponents<=4);
  assert("pre: valid_numberOfElements" && numberOfElements>=1);

  UniformMapIt cur=this->Map->Map.find(name);

  if(cur!=this->Map->Map.end())
    {
    vtkUniform *u=(*cur).second;
    vtkUniformArrayInt *ui=0;
    if(u->GetClassType()==vtkUniform::ClassTypeArrayInt)
      {
      ui=static_cast<vtkUniformArrayInt *>(u);
      }
    if(ui==0)
      {
      vtkErrorMacro(<<"try to overwrite a value with different type.");
      }
    else
      {
      if(ui->GetSize()!=numberOfComponents)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of components.");
        }
      if(ui->GetArraySize()!=numberOfElements)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of elements.");
        }
      else
        {
        bool changed=false;
        int i=0;
        while(!changed && i<numberOfComponents*numberOfElements)
          {
          changed=value[i]!=ui->GetValues()[i];
          ++i;
          }
        if(changed)
          {
          ui->SetValues(value);
          this->Modified();
          }
        }
      }
    }
  else
    {
    vtkUniform *u=0;
    u=new vtkUniformArrayInt(numberOfComponents,numberOfElements,value);
    u->SetName(name);

    vtksys_stl::pair<const char *, vtkUniform *> p;
    p.first=u->GetName(); // cannot be `name' because
    // we don't manage this pointer.
    p.second=u;

    this->Map->Map.insert(p);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::SetUniformfv(const char *name,
                                       int numberOfComponents,
                                       int numberOfElements,
                                       float *value)
{
  assert("pre: name_exists" && name!=0);
  assert("pre: value_exists" && value!=0);
  assert("pre: valid_numberOfComponents" && numberOfComponents>=1 && numberOfComponents<=4);
  assert("pre: valid_numberOfElements" && numberOfElements>=1);

  UniformMapIt cur=this->Map->Map.find(name);

  if(cur!=this->Map->Map.end())
    {
    vtkUniform *u=(*cur).second;
    vtkUniformArrayFloat *uf=0;
    if(u->GetClassType()==vtkUniform::ClassTypeArrayFloat)
      {
      uf=static_cast<vtkUniformArrayFloat *>(u);
      }
    if(uf==0)
      {
      vtkErrorMacro(<<"try to overwrite a value with different type.");
      }
    else
      {
      if(uf->GetSize()!=numberOfComponents)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of components.");
        }
      if(uf->GetArraySize()!=numberOfElements)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of elements.");
        }
      else
        {
        bool changed=false;
        int i=0;
        while(!changed && i<numberOfComponents*numberOfElements)
          {
          changed=value[i]!=uf->GetValues()[i];
          ++i;
          }
        if(changed)
          {
          uf->SetValues(value);
          this->Modified();
          }
        }
      }
    }
  else
    {
    vtkUniform *u=0;
    u=new vtkUniformArrayFloat(numberOfComponents,numberOfElements,value);
    u->SetName(name);

    vtksys_stl::pair<const char *, vtkUniform *> p;
    p.first=u->GetName(); // cannot be `name' because
    // we don't manage this pointer.
    p.second=u;

    this->Map->Map.insert(p);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::SetUniformMatrix(const char *name,
                                           int rows,
                                           int columns,
                                           float *value)
{
  assert("pre: name_exists" && name!=0);
  assert("pre: value_exists" && value!=0);
  assert("pre: valid_rows" && rows>=2 && rows<=4);
  assert("pre: valid_columns" && columns>=2 && columns<=4);

  UniformMapIt cur=this->Map->Map.find(name);
  
  if(cur!=this->Map->Map.end())
    {
    vtkUniform *u=(*cur).second;
    vtkUniformMatrix *um=0;
    if(u->GetClassType()==vtkUniform::ClassTypeMatrix)
      {
      um=static_cast<vtkUniformMatrix *>(u);
      }
    if(um==0)
      {
      vtkErrorMacro(<<"try to overwrite a value with different type.");
      }
    else
      {
      if(um->GetRows()!=rows || um->GetColumns()!=columns)
        {
        vtkErrorMacro(<<"try to overwrite a value of same type but different number of components.");
        }
      else
        {
        bool changed=false;
        int i=0;
        while(!changed && i<rows*columns)
          {
          changed=value[i]!=um->GetValues()[i];
          ++i;
          }
        if(changed)
          {
          um->SetValues(value);
          this->Modified();
          }
        }
      }
    }
  else
    {
    vtkUniform *u=new vtkUniformMatrix(rows,columns,value);
    u->SetName(name);
    vtksys_stl::pair<const char *, vtkUniform *> p;
    p.first=u->GetName(); // cannot be `name' because
    // we don't manage this pointer.
    p.second=u;
    this->Map->Map.insert(p);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Remove uniform `name' from the list.
void vtkUniformVariables::RemoveUniform(const char *name)
{
  UniformMapIt cur=this->Map->Map.find(name);
  if(cur!=this->Map->Map.end())
    {
    this->Map->Map.erase(cur);
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// \pre need a valid OpenGL context and a shader program in use.
void vtkUniformVariables::Send(const char *name,
                               int uniformIndex)
{
  UniformMapIt cur=this->Map->Map.find(name);
  (*cur).second->Send(uniformIndex);
}

// ----------------------------------------------------------------------------
// Description:
// Place the internal cursor on the first uniform.
void vtkUniformVariables::Start()
{
  this->Map->It=this->Map->Map.begin();
}
  
// ----------------------------------------------------------------------------
// Description:
// Is the iteration done?
bool vtkUniformVariables::IsAtEnd()
{
  return this->Map->It==this->Map->Map.end();
}
  
// ----------------------------------------------------------------------------
// Description:
// Name of the uniform at the current cursor position.
// \pre not_done: !this->IsAtEnd()
const char *vtkUniformVariables::GetCurrentName()
{
  assert("pre: not_done" && !this->IsAtEnd());
  return (*this->Map->It).first;
}
  
// ----------------------------------------------------------------------------
// Description:
// \pre need a valid OpenGL context and a shader program in use.
// \pre not_done: !this->IsAtEnd()
void vtkUniformVariables::SendCurrentUniform(int uniformIndex)
{
  assert("pre: not_done" && !this->IsAtEnd());
  (*this->Map->It).second->Send(uniformIndex);
}

// ----------------------------------------------------------------------------
// Description:
// Move the cursor to the next uniform.
// \pre not_done: !this->IsAtEnd()
void vtkUniformVariables::Next()
{
  assert("pre: not_done" && !this->IsAtEnd());
  ++this->Map->It;
}

// ----------------------------------------------------------------------------
// Description:
// Copy all the variables from `other'. Any existing variable will be
// overwritten.
// \pre other_exists: other!=0
// \pre not_self: other!=this
void vtkUniformVariables::Merge(vtkUniformVariables *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  
  other->Start();
  while(!other->IsAtEnd())
    {
    const char *name=other->GetCurrentName();
    UniformMapIt cur=other->Map->Map.find(name);
    
    vtkUniform *u1=(*cur).second;
    
    vtkUniform *u2=u1->Clone();
    vtksys_stl::pair<const char *, vtkUniform *> p;
    p.first=u2->GetName();
    p.second=u2;
    this->Map->Map.erase(p.first);
    this->Map->Map.insert(p);
    other->Next();
    }
  if(other->Map->Map.size()>0)
    {
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Copy all the variables from `other'. Any existing variable will be
// deleted first.
// \pre other_exists: other!=0
// \pre not_self: other!=this
void vtkUniformVariables::DeepCopy(vtkUniformVariables *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  
  if(this->Map->Map.size()>0)
    {
    delete this->Map;
    this->Map=new vtkUniformVariablesMap;
    this->Modified();
    }
  this->Merge(other);
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::RemoveAllUniforms()
{
  if(this->Map->Map.size()>0)
    {
    delete this->Map;
    this->Map=new vtkUniformVariablesMap;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
void vtkUniformVariables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  this->Start();
  while(!this->IsAtEnd())
    {
    const char *name=this->GetCurrentName();
    UniformMapIt cur=this->Map->Map.find(name);
    (*cur).second->PrintSelf(os,indent);
    this->Next();
    }
}
