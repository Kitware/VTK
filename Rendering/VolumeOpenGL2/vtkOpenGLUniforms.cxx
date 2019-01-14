#include "vtkOpenGLUniforms.h"
#include "vtkObjectFactory.h"
#include "vtkShaderProgram.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include <vector>
#include <cstring>

vtkStandardNewMacro(vtkOpenGLUniforms)

// temporary patch: Some Android builds don't have std::to_string
#include <sstream>
namespace patch
{
    template < typename T >
    std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

class vtkCustomUniform
{
public:
    virtual ~vtkCustomUniform() {}
    virtual std::string GetGlslDec( const std::string & ) { return std::string(); }
    virtual bool SetUniform( const char *, vtkShaderProgram * ) { return false; }
    virtual void PrintSelf( const char *, ostream&, vtkIndent ) {}
};

class vtkCustomUniformi : public vtkCustomUniform
{
public:
    vtkCustomUniformi( int val ) { value = val; }
    void SetValue( int val ) { value = val; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform int ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniformi( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": " << value << endl;}
protected:
    int value;
};

class vtkCustomUniformf : public vtkCustomUniform
{
public:
    vtkCustomUniformf( float val ) { value = val; }
    void SetValue( float val ) { value = val; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform float ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniformf( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": " << value << endl;}
protected:
    float value;
};

class vtkCustomUniform2i : public vtkCustomUniform
{
public:
    vtkCustomUniform2i( const int val[2] ) { value[0] = val[0]; value[1] = val[1]; }
    void SetValue( const int val[2] ) { value[0] = val[0]; value[1] = val[1]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform ivec2 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform2i( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << " )" << endl;}
protected:
    int value[2];
};

class vtkCustomUniform2f : public vtkCustomUniform
{
public:
    vtkCustomUniform2f( const float val[2] ) { value[0] = val[0]; value[1] = val[1]; }
    void SetValue( const float val[2] ) { value[0] = val[0]; value[1] = val[1]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform vec2 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform2f( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << " )" << endl;}
protected:
    float value[2];
};

class vtkCustomUniform3f : public vtkCustomUniform
{
public:
    vtkCustomUniform3f( const float val[3] ) { value[0] = val[0]; value[1] = val[1]; value[2] = val[2]; }
    void SetValue( const float val[3] ) { value[0] = val[0]; value[1] = val[1]; value[2] = val[2]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform vec3 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform3f( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << ", " << value[2] << " )" << endl;}
protected:
    float value[3];
};

class vtkCustomUniform3d : public vtkCustomUniform
{
public:
    vtkCustomUniform3d( const double val[3] ) { value[0] = val[0]; value[1] = val[1]; value[2] = val[2]; }
    void SetValue( const double val[3] ) { value[0] = val[0]; value[1] = val[1]; value[2] = val[2]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform vec3 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform3f( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << ", " << value[2] << " )" << endl;}
protected:
    double value[3];
};

class vtkCustomUniform4f : public vtkCustomUniform
{
public:
    vtkCustomUniform4f( const float val[4] ) { value[0] = val[0]; value[1] = val[1]; value[2] = val[2]; value[3] = val[3]; }
    void SetValue( const float val[4] ) { value[0] = val[0]; value[1] = val[1]; value[2] = val[2]; value[3] = val[3]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform vec4 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform4f( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << ", " << value[2] << ", " << value[3] << " )" << endl;}
protected:
    float value[4];
};

class vtkCustomUniform3uc : public vtkCustomUniform
{
public:
    vtkCustomUniform3uc( const unsigned char v[3] ) { value[0] = v[0]; value[1] = v[1]; value[2] = v[2]; }
    void SetValue( const unsigned char v[3] ) { value[0] = v[0]; value[1] = v[1]; value[2] = v[2]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform vec3 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform3uc( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << ", " << value[2] << " )" << endl;}
protected:
    unsigned char value[3];
};

class vtkCustomUniform4uc : public vtkCustomUniform
{
public:
    vtkCustomUniform4uc( const unsigned char v[4] ) { value[0] = v[0]; value[1] = v[1]; value[2] = v[2]; value[3] = v[3]; }
    void SetValue( const unsigned char v[4] ) { value[0] = v[0]; value[1] = v[1]; value[2] = v[2]; value[3] = v[3]; }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform vec4 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniform4uc( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": ( " << value[0] << ", " << value[1] << ", " << value[2] << ", " << value[3] << " )" << endl;}
protected:
    unsigned char value[4];
};

class vtkCustomUniformVtkMatrix3x3 : public vtkCustomUniform
{
public:
    vtkCustomUniformVtkMatrix3x3( vtkMatrix3x3 * v ) { value = vtkMatrix3x3::New(); value->DeepCopy( v ); }
    void SetValue( vtkMatrix3x3 * v ) { value = vtkMatrix3x3::New(); value->DeepCopy( v ); }
    virtual ~vtkCustomUniformVtkMatrix3x3() override { value->Delete(); }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform mat3 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniformMatrix( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << endl; value->PrintSelf(os,indent); os << endl;}
protected:
    vtkMatrix3x3 *value;
};

class vtkCustomUniformVtkMatrix4x4 : public vtkCustomUniform
{
public:
    vtkCustomUniformVtkMatrix4x4( vtkMatrix4x4 * v ) { value = vtkMatrix4x4::New(); value->DeepCopy( v ); }
    void SetValue( vtkMatrix4x4 * v ) { value = vtkMatrix4x4::New(); value->DeepCopy( v ); }
    virtual ~vtkCustomUniformVtkMatrix4x4() override { value->Delete(); }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform mat4 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniformMatrix( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << endl; value->PrintSelf(os,indent); os << endl;}
protected:
    vtkMatrix4x4 *value;
};

class vtkCustomUniformMatrix3x3 : public vtkCustomUniform
{
public:
    vtkCustomUniformMatrix3x3( float * v ) { std::memcpy( value, v, sizeof value ); }
    void SetValue( float * v ) { std::memcpy( value, v, sizeof value ); }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform mat3 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniformMatrix3x3( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    {
        os << indent << name << ": " << endl;
        for( int i = 0; i < 3; ++i )
        {
            os << indent << "( ";
            for( int j = 0; j < 3; ++j )
            {
                os << value[i*3+j];
                if( j < 2 ) os << ", ";
            }
            os << " )" << endl;
        }
        os << endl;
    }
protected:
    float value[9];
};

class vtkCustomUniformMatrix4x4 : public vtkCustomUniform
{
public:
    vtkCustomUniformMatrix4x4( float * v ) { std::memcpy( value, v, sizeof value ); }
    void SetValue( float * v ) { std::memcpy( value, v, sizeof value ); }
    std::string GetGlslDec( const std::string & name ) override { return std::string("uniform mat4 ") + name + ";\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override { return p->SetUniformMatrix4x4( name, value ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    {
        os << indent << name << ": " << endl;
        for( int i = 0; i < 4; ++i )
        {
            os << indent << "( ";
            for( int j = 0; j < 4; ++j )
            {
                os << value[i*4+j];
                if( j < 3 ) os << ", ";
            }
            os << " )" << endl;
        }
        os << endl;
    }
protected:
    float value[16];
};

class vtkCustomUniform1iv : public vtkCustomUniform
{
public:
    vtkCustomUniform1iv( const int count, const int *v ) { SetValue(count,v); }
    void SetValue( const int count, const int *v ) { value.assign( v, v + count ); }
    std::string GetGlslDec( const std::string & name ) override
    { return std::string("uniform int ") + name + "[" + patch::to_string(value.size()) + "];\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override
    { return p->SetUniform1iv( name, static_cast<int>(value.size()), value.data() ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": "; for( auto & v : value ) os << v << " "; os << endl; }
protected:
    std::vector<int> value;
};

class vtkCustomUniform1fv : public vtkCustomUniform
{
public:
    vtkCustomUniform1fv( const int count, const float *v ) { SetValue(count,v); }
    void SetValue( const int count, const float *v ) { value.assign( v, v + count ); }
    std::string GetGlslDec( const std::string & name ) override
    { return std::string("uniform float ") + name + "[" + patch::to_string(value.size()) + "];\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override
    { return p->SetUniform1fv( name, static_cast<int>(value.size()), value.data() ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    { os << indent << name << ": "; for( auto & v : value ) os << v << " "; os << endl; }
protected:
    std::vector<float> value;
};

class vtkCustomUniform2fv : public vtkCustomUniform
{
public:
    vtkCustomUniform2fv( const int count, const float (*v)[2] ) { SetValue(count,v); }
    void SetValue( const int count, const float (*v)[2] )
    { value.assign( reinterpret_cast<const float*>(v), reinterpret_cast<const float*>(v + 2*count ) ); }
    std::string GetGlslDec( const std::string & name ) override
    { return std::string("uniform vec2 ") + name + "[" + patch::to_string(value.size()/2) + "];\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override
    { return p->SetUniform2fv( name, static_cast<int>(value.size()/2), reinterpret_cast<const float(*)[2]>(value.data()) ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    {
        os << indent << name << ": ";
        for( std::vector<float>::size_type i = 0; i < value.size()/2; ++i )
            os << "( " << value[2*i] << ", " << value[2*i+1] << " ) ";
        os << endl;
    }
protected:
    std::vector<float> value;
};

class vtkCustomUniform3fv : public vtkCustomUniform
{
public:
    vtkCustomUniform3fv( const int count, const float (*v)[3] ) { SetValue(count,v); }
    void SetValue( const int count, const float (*v)[3] ) { value.assign( reinterpret_cast<const float*>(v), reinterpret_cast<const float*>(v + 3*count) ); }
    std::string GetGlslDec( const std::string & name ) override
    { return std::string("uniform vec3 ") + name + "[" + patch::to_string(value.size()/3) + "];\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override
    { return p->SetUniform3fv( name, static_cast<int>(value.size()/3), reinterpret_cast<const float(*)[3]>(value.data()) ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    {
        os << indent << name << ": ";
        for( std::vector<float>::size_type i = 0; i < value.size()/3; ++i )
            os << "( " << value[3*i] << ", " << value[3*i+1] << ", " << value[3*i+2] << " ) ";
        os << endl;
    }
protected:
    std::vector<float> value;
};

class vtkCustomUniform4fv : public vtkCustomUniform
{
public:
    vtkCustomUniform4fv( const int count, const float (*v)[4] ) { SetValue(count,v); }
    void SetValue( const int count, const float (*v)[4] ) { value.assign( reinterpret_cast<const float*>(v), reinterpret_cast<const float*>(v + 4*count) ); }
    std::string GetGlslDec( const std::string & name ) override
    { return std::string("uniform vec4 ") + name + "[" + patch::to_string(value.size()/4) + "];\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override
    { return p->SetUniform4fv( name, static_cast<int>(value.size()/4), reinterpret_cast<const float(*)[4]>(value.data()) ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    {
        os << indent << name << ": ";
        for( std::vector<float>::size_type i = 0; i < value.size()/4; ++i )
            os << "( " << value[4*i] << ", " << value[4*i+1] << ", " << value[4*i+2] << ", " << value[4*i+3] << " ) ";
        os << endl;
    }
protected:
    std::vector<float> value;
};

class vtkCustomUniformMatrix4x4v : public vtkCustomUniform
{
public:
    vtkCustomUniformMatrix4x4v( const int count, const float *v ) { SetValue(count,v); }
    void SetValue( const int count, const float *v ) { value.assign( v, v + 16*count ); }
    std::string GetGlslDec( const std::string & name ) override
    { return std::string("uniform mat4 ") + name + "[" + patch::to_string(value.size()/16) + "];\n"; }
    bool SetUniform( const char * name, vtkShaderProgram * p ) override
    { return p->SetUniformMatrix4x4v( name, static_cast<int>(value.size()/16), value.data() ); }
    void PrintSelf( const char * name, ostream& os, vtkIndent indent ) override
    {
        os << indent << name << ": " << endl;
        for( std::vector<float>::size_type mat = 0; mat < value.size()/16; ++mat )
        {
            for( std::vector<float>::size_type i = 0; i < 4; ++i )
            {
                os << indent << "( ";
                for( std::vector<float>::size_type j = 0; j < 4; ++j )
                {
                    os << value[mat*16+i*4+j];
                    if( j < 3 ) os << ", ";
                }
                os << " )" << endl;
            }
            os << endl << endl;
        }
    }
protected:
    std::vector<float> value;
};

class vtkUniformInternals : public vtkObject
{

public:

    static vtkUniformInternals *New();
    vtkTypeMacro(vtkUniformInternals, vtkObject);
    void PrintSelf(ostream& os, vtkIndent indent) override
    {
        for( auto & uni : Uniforms )
        {
            uni.second->PrintSelf( uni.first.c_str(), os, indent );
        }
    }

    template< class dataT, class uniformT >
    void AddUniform( const char * name, dataT defaultValue )
    {
        UniformMapT::iterator it = Uniforms.find( name );
        if( it != Uniforms.end() )
        {
            vtkErrorMacro( << "vtkOpenGLUniform: overwriting existing uniform variable: " << name << endl );
            delete (*it).second;
            Uniforms.erase( it );
        }
        Uniforms[std::string(name)] = new uniformT(defaultValue);
        Modified();
    }

    template< class dataT, class uniformT >
    void AddUniform( const char * name, int count, dataT defaultValue )
    {
        UniformMapT::iterator it = Uniforms.find( name );
        if( it != Uniforms.end() )
        {
            vtkErrorMacro( << "vtkOpenGLUniform: overwriting existing uniform variable: " << name << endl );
            delete (*it).second;
            Uniforms.erase( it );
        }
        Uniforms[std::string(name)] = new uniformT(count, defaultValue);
        Modified();
    }

    void RemoveUniform(const char *name)
    {
        UniformMapT::iterator it = Uniforms.find( name );
        if( it != Uniforms.end() )
        {
            delete (*it).second;
            Uniforms.erase( it );
        }
        Modified();
    }

    void RemoveAllUniforms()
    {
        for( auto & uni : Uniforms )
        {
            delete uni.second;
        }
        Uniforms.clear();
        Modified();
    }

    template< class dataT, class uniformT >
    void SetUniform( const char *name, const dataT value )
    {
        UniformMapT::iterator it = Uniforms.find( name );
        if( it != Uniforms.end() )
        {
            uniformT * uni = dynamic_cast<uniformT*>(it->second);
            uni->SetValue( value );
        }
        else
        {
            vtkErrorMacro( << "Trying to set the value of undefined uniform variable: " << name << endl );
        }
    }

    template< class dataT, class uniformT >
    void SetUniform( const char *name, int count, const dataT value )
    {
        UniformMapT::iterator it = Uniforms.find( name );
        if( it != Uniforms.end() )
        {
            uniformT * uni = dynamic_cast<uniformT*>(it->second);
            uni->SetValue( count, value );
        }
        else
        {
            vtkErrorMacro( << "Trying to set the value of undefined uniform variable: " << name << endl );
        }
    }

    std::string GetDeclarations()
    {
        std::string res;
        for( auto & uni : Uniforms )
        {
            res += uni.second->GetGlslDec( uni.first );
        }
        return res;
    }

    bool SetUniforms( vtkShaderProgram * p )
    {
        bool res = true;
        for( auto & uni : Uniforms )
        {
            bool r = uni.second->SetUniform( uni.first.c_str(), p );
            if( !r )
                vtkErrorMacro( << "vtkOpenGLUniform: couldn't set custom uniform variable " << uni.first << endl );
            res &= r;
        }
        return res;
    }

protected:

    vtkUniformInternals() {}
    ~vtkUniformInternals() override
    {
        RemoveAllUniforms();
    }

private:
    vtkUniformInternals(const vtkUniformInternals&) = delete;
    void operator=(const vtkUniformInternals&) = delete;

    typedef std::map<std::string,vtkCustomUniform*> UniformMapT;
    UniformMapT Uniforms;
};

vtkStandardNewMacro(vtkUniformInternals);

vtkOpenGLUniforms::vtkOpenGLUniforms()
{
    this->Internals = vtkUniformInternals::New();
}

vtkOpenGLUniforms::~vtkOpenGLUniforms()
{
    this->Internals->Delete();
}

void vtkOpenGLUniforms::AddUniformi (const char *name, int defaultValue)
{ this->Internals->AddUniform<int,vtkCustomUniformi>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniformf(const char *name, float defaultValue)
{ this->Internals->AddUniform<float,vtkCustomUniformf>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform2i(const char *name, const int defaultValue[2])
{ this->Internals->AddUniform<const int[2],vtkCustomUniform2i>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform2f(const char *name, const float defaultValue[2])
{ this->Internals->AddUniform<const float[2],vtkCustomUniform2f>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform3f(const char *name, const float defaultValue[3])
{ this->Internals->AddUniform<const float[3],vtkCustomUniform3f>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform3f(const char *name, const double defaultValue[3])
{ this->Internals->AddUniform<const double[3],vtkCustomUniform3d>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform4f(const char *name, const float defaultValue[4])
{ this->Internals->AddUniform<const float[4],vtkCustomUniform4f>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform3uc(const char *name, const unsigned char defaultValue[3]) // maybe remove
{ this->Internals->AddUniform<const unsigned char[3],vtkCustomUniform3uc>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform4uc(const char *name, const unsigned char defaultValue[4]) // maybe remove
{ this->Internals->AddUniform<const unsigned char[4],vtkCustomUniform4uc>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniformMatrix(const char *name, vtkMatrix3x3 *defaultValue)
{ this->Internals->AddUniform<vtkMatrix3x3*,vtkCustomUniformVtkMatrix3x3>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniformMatrix(const char *name, vtkMatrix4x4 *defaultValue)
{ this->Internals->AddUniform<vtkMatrix4x4*,vtkCustomUniformVtkMatrix4x4>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniformMatrix3x3(const char *name, float *defaultValue)
{ this->Internals->AddUniform<float*,vtkCustomUniformMatrix3x3>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniformMatrix4x4(const char *name, float *defaultValue)
{ this->Internals->AddUniform<float*,vtkCustomUniformMatrix4x4>( name, defaultValue ); }
void vtkOpenGLUniforms::AddUniform1iv(const char *name, const int count, const int *f)
{ this->Internals->AddUniform<const int *,vtkCustomUniform1iv>(name,count,f); }
void vtkOpenGLUniforms::AddUniform1fv (const char *name, const int count, const float *f)
{ this->Internals->AddUniform<const float *,vtkCustomUniform1fv>(name,count,f); }
void vtkOpenGLUniforms::AddUniform2fv (const char *name, const int count, const float(*f)[2])
{ this->Internals->AddUniform<const float(*)[2],vtkCustomUniform2fv>(name,count,f); }
void vtkOpenGLUniforms::AddUniform3fv (const char *name, const int count, const float(*f)[3])
{ this->Internals->AddUniform<const float(*)[3],vtkCustomUniform3fv>(name,count,f); }
void vtkOpenGLUniforms::AddUniform4fv (const char *name, const int count, const float(*f)[4])
{ this->Internals->AddUniform<const float(*)[4],vtkCustomUniform4fv>(name,count,f); }
void vtkOpenGLUniforms::AddUniformMatrix4x4v (const char *name, const int count, float *v)
{ this->Internals->AddUniform<float *,vtkCustomUniformMatrix4x4v>(name,count,v); }

void vtkOpenGLUniforms::RemoveUniform(const char *name) { this->Internals->RemoveUniform(name); }
void vtkOpenGLUniforms::RemoveAllUniforms() { this->Internals->RemoveAllUniforms(); }

void vtkOpenGLUniforms::SetUniformi (const char *name, int v)
{ this->Internals->SetUniform<int,vtkCustomUniformi>(name, v); }
void vtkOpenGLUniforms::SetUniformf (const char *name, float v)
{ this->Internals->SetUniform<float,vtkCustomUniformf>(name,v); }
void vtkOpenGLUniforms::SetUniform2i (const char *name, const int v[2])
{ this->Internals->SetUniform<const int[2],vtkCustomUniform2i>(name,v); }
void vtkOpenGLUniforms::SetUniform2f (const char *name, const float v[2])
{ this->Internals->SetUniform<const float[2],vtkCustomUniform2f>(name,v); }
void vtkOpenGLUniforms::SetUniform3f (const char *name, const float v[3])
{ this->Internals->SetUniform<const float[3],vtkCustomUniform3f>(name,v); }
void vtkOpenGLUniforms::SetUniform3f (const char *name, const double v[3])
{ this->Internals->SetUniform<const double[3],vtkCustomUniform3d>(name,v); }
void vtkOpenGLUniforms::SetUniform4f (const char *name, const float v[4])
{ this->Internals->SetUniform<const float[4],vtkCustomUniform4f>(name,v); }
void vtkOpenGLUniforms::SetUniform3uc (const char *name, const unsigned char v[3])
{ this->Internals->SetUniform<const unsigned char[3],vtkCustomUniform3uc>(name,v); }
void vtkOpenGLUniforms::SetUniform4uc (const char *name, const unsigned char v[4])
{ this->Internals->SetUniform<const unsigned char[4],vtkCustomUniform4uc>(name,v); }
void vtkOpenGLUniforms::SetUniformMatrix (const char *name, vtkMatrix3x3 *v)
{ this->Internals->SetUniform<vtkMatrix3x3*,vtkCustomUniformVtkMatrix3x3>(name,v); }
void vtkOpenGLUniforms::SetUniformMatrix (const char *name, vtkMatrix4x4 *v)
{ this->Internals->SetUniform<vtkMatrix4x4*,vtkCustomUniformVtkMatrix4x4>(name,v); }
void vtkOpenGLUniforms::SetUniformMatrix3x3 (const char *name, float *v)
{ this->Internals->SetUniform<float*,vtkCustomUniformMatrix3x3>(name,v); }
void vtkOpenGLUniforms::SetUniformMatrix4x4 (const char *name, float *v)
{ this->Internals->SetUniform<float*,vtkCustomUniformMatrix4x4>(name,v); }
void vtkOpenGLUniforms::SetUniform1iv (const char *name, const int count, const int *f)
{ this->Internals->SetUniform<const int *,vtkCustomUniform1iv>(name,count,f); }
void vtkOpenGLUniforms::SetUniform1fv (const char *name, const int count, const float *f)
{ this->Internals->SetUniform<const float*,vtkCustomUniform1fv>(name,count,f); }
void vtkOpenGLUniforms::SetUniform2fv (const char *name, const int count, const float(*f)[2])
{ this->Internals->SetUniform<const float(*)[2],vtkCustomUniform2fv>(name,count,f); }
void vtkOpenGLUniforms::SetUniform3fv (const char *name, const int count, const float(*f)[3])
{ this->Internals->SetUniform<const float(*)[3],vtkCustomUniform3fv>(name,count,f); }
void vtkOpenGLUniforms::SetUniform4fv (const char *name, const int count, const float(*f)[4])
{ this->Internals->SetUniform<const float(*)[4],vtkCustomUniform4fv>(name,count,f); }
void vtkOpenGLUniforms::SetUniformMatrix4x4v (const char *name, const int count, float *v)
{ this->Internals->SetUniform<float*,vtkCustomUniformMatrix4x4v>(name,count,v); }

std::string vtkOpenGLUniforms::GetDeclarations()
{ return this->Internals->GetDeclarations(); }

bool vtkOpenGLUniforms::SetUniforms( vtkShaderProgram * p )
{ return this->Internals->SetUniforms(p); }

vtkMTimeType vtkOpenGLUniforms::GetUniformListMTime()
{
    return this->Internals->GetMTime();
}

void vtkOpenGLUniforms::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
    this->Internals->PrintSelf(os,indent);
}
