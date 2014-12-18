/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVolumeShader_h
#define vtkVolumeShader_h

#include <vtk_glew.h>

#include <string>
#include <map>

using namespace std;

class vtkVolumeShader
{
public:
    vtkVolumeShader(void);
    ~vtkVolumeShader(void);
    void LoadFromString(GLenum whichShader, const string& source);
    void LoadFromFile(GLenum whichShader, const string& filename);
    void CreateAndLinkProgram();
    void Use();
    void UnUse();
    void AddAttribute(const string& attribute);
    void AddUniform(const string& uniform);
    unsigned int GetProgram()
      {
      return this->Program;
      }

    //An indexer that returns the location of the attribute/uniform
    GLuint operator[](const string& attribute);
    GLuint operator()(const string& uniform);
    void DeleteShaderProgram();

private:
    enum ShaderType {VERTEX_SHADER, FRAGMENT_SHADER, GEOMETRY_SHADER};
    GLuint	Program;
    int TotalShaders;
    GLuint Shaders[3];//0->vertexshader, 1->fragmentshader, 2->geometryshader
    map<string,GLuint> AttributeList;
    map<string,GLuint> UniformLocationList;
};

#endif // vtkVolumeShader_h
// VTK-HeaderTest-Exclude: vtkVolumeShader.h
