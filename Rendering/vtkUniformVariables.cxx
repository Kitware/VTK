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
vtkCxxRevisionMacro(vtkUniformVariables, "1.4");
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
    ClassTypeMatrix=2
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
  
protected:
  int Size;
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
        ui->SetValues(value);
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
        uf->SetValues(value);
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
        um->SetValues(value);
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
