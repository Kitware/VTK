#pragma once

#include "../Types.h"
#include "Material.h"

#include <VisRTX.h>
#include <cassert>

namespace RTW
{
    class Geometry : public Object
    {
        friend class World;

    public:
        Geometry(const std::string& type)
            : Object(RTW_GEOMETRY)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();

            if (type == "mesh")
                this->geometry = rtx->CreateTriangleGeometry();
            else if (type == "sphere")
                this->geometry = rtx->CreateSphereGeometry();
            else if (type == "curve")
                this->geometry = rtx->CreateCylinderGeometry();
            else if (type == "isosurfaces")
                ; // not implemented
            else
            {
                assert(false);
            }
        }

        ~Geometry()
        {
            if(this->geometry)
                this->geometry->Release();
            if(this->material)
                this->material->Release();
        }

        void Commit() override
        {
            if (!this->geometry)
                return;

            /*
             * Triangles
             */
            if (this->geometry->GetType() == VisRTX::GeometryType::TRIANGLES)
            {
                VisRTX::TriangleGeometry* tri = dynamic_cast<VisRTX::TriangleGeometry*>(this->geometry);

                Data* vertex = this->GetObject<Data>({ "vertex.position", "position", "vertex" });
                Data* index = this->GetObject<Data>({ "index" });
                if (vertex && index)
                {
                    uint32_t numTriangles = static_cast<uint32_t>(index->GetNumElements());
                    VisRTX::Vec3ui* triangles = reinterpret_cast<VisRTX::Vec3ui*>(index->GetData());
                    assert(index->GetElementDataType() == RTW_VEC3UI);

                    uint32_t numVertices = static_cast<uint32_t>(vertex->GetNumElements());
                    VisRTX::Vec3f* vertices = reinterpret_cast<VisRTX::Vec3f*>(vertex->GetData());
                    assert(vertex->GetElementDataType() == RTW_VEC3F);

                    VisRTX::Vec3f* normals = nullptr;
                    Data* normal = this->GetObject<Data>({ "vertex.normal" });
                    if (normal)
                    {
                        normals = reinterpret_cast<VisRTX::Vec3f*>(normal->GetData());
                        assert(normal->GetElementDataType() == RTW_VEC3F);
                    }


                    tri->SetTriangles(numTriangles, triangles, numVertices, vertices, normals);


                    Data* color = this->GetObject<Data>({ "vertex.color" });
                    if (color)
                    {
                        VisRTX::Vec4f* colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetElementDataType() == RTW_VEC4F);
                        tri->SetColors(colors);
                    }
                    else
                    {
                        tri->SetColors(nullptr);
                    }


                    Data* texcoord = GetObject<Data>({ "vertex.texcoord" });
                    if (texcoord)
                    {
                        VisRTX::Vec2f* texcoords = reinterpret_cast<VisRTX::Vec2f*>(texcoord->GetData());
                        assert(texcoord->GetElementDataType() == RTW_VEC2F);
                        tri->SetTexCoords(texcoords);
                    }
                    else
                    {
                        tri->SetTexCoords(nullptr);
                    }

                    Data* materialList = GetObject<Data>({ "material" });
                    if (materialList)
                    {
                        assert(materialList->GetElementDataType() == RTW_MATERIAL);

                        std::vector<VisRTX::Material*> triangleMaterials;
                        triangleMaterials.resize(numTriangles);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());

                        for (uint32_t i = 0; i < numTriangles; ++i)
                        {
                            Material* materialHandle = materials[i];
                            if (materialHandle)
                                triangleMaterials[i] = materialHandle->material;
                        }

                        tri->SetMaterials(triangleMaterials.data());
                    }
                    else
                    {
                        tri->SetMaterials(nullptr);
                    }
                }
                else
                {
                    tri->SetTriangles(0, nullptr, 0, nullptr, nullptr);
                    assert(false);
                }
            }

            /*
             * Spheres
             */
            else if (this->geometry->GetType() == VisRTX::GeometryType::SPHERES)
            {
                VisRTX::SphereGeometry* sphere = dynamic_cast<VisRTX::SphereGeometry*>(this->geometry);

                Data* spheres = GetObject<Data>({ "sphere.position" });
                if (spheres)
                {
                    VisRTX::Vec4f* colors = nullptr;
                    Data* color = GetObject<Data>({ "color" });
                    if (color)
                    {
                        colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetElementDataType() == RTW_VEC4F);
                        sphere->SetColors(reinterpret_cast<VisRTX::Vec4f *>(color->GetData()));
                    }
                    else
                    {
                        sphere->SetColors(nullptr);
                    }

                    Data *radii = GetObject<Data>({"sphere.radius"});
                    if (radii)
                    {
                        uint32_t numSpheres = spheres->GetNumElements();
                        sphere->SetSpheres(numSpheres,
                                           reinterpret_cast<VisRTX::Vec3f *>(spheres->GetData()),
                                           reinterpret_cast<float *>(radii->GetData()));
                    }
                    else
                    {
                        uint32_t numSpheres = spheres->GetNumElements();
                        sphere->SetSpheres(numSpheres,
                                           reinterpret_cast<VisRTX::Vec3f *>(spheres->GetData()),
                                           nullptr);
                    }


                    Data* texcoord = GetObject<Data>({ "sphere.texcoord" });
                    if (texcoord)
                    {
                        VisRTX::Vec2f* texcoords = reinterpret_cast<VisRTX::Vec2f*>(texcoord->GetData());
                        assert(texcoord->GetElementDataType() == RTW_VEC2F);
                        sphere->SetTexCoords(texcoords);
                    }
                    else
                    {
                        sphere->SetTexCoords(nullptr);
                    }

                    Data* materialList = GetObject<Data>({ "material" });
                    if (materialList)
                    {
                        assert(materialList->GetElementDataType() == RTW_MATERIAL);

                        uint32_t numSpheres = spheres->GetNumElements();
                        std::vector<VisRTX::Material*> sphereMaterials;
                        sphereMaterials.resize(numSpheres);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());

                        for (uint32_t i = 0; i < numSpheres; ++i)
                        {
                            Material* materialHandle = materials[i];
                            if (materialHandle)
                                sphereMaterials[i] = materialHandle->material;
                        }

                        sphere->SetMaterials(sphereMaterials.data());
                    }
                    else
                    {
                        sphere->SetMaterials(nullptr);
                    }
                }
                else
                {
                    assert(false);
                }

                float radius;
                if (this->GetFloat({ "radius" }, &radius))
                    sphere->SetRadius(radius);
            }

            /*
             * Cylinders
             */
            else if (this->geometry->GetType() == VisRTX::GeometryType::CYLINDERS)
            {
                VisRTX::CylinderGeometry* cyl = dynamic_cast<VisRTX::CylinderGeometry*>(this->geometry);

                //Non-scale-array variant
                Data* cylinders = GetObject<Data>({ "vertex.position" });
                if (cylinders)
                {
                    VisRTX::Vec4f* colors = nullptr;
                    Data* color = GetObject<Data>({ "color" });
                    if (color)
                    {
                        colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetElementDataType() == RTW_VEC4F);
                    }

                    int32_t bytesPerCylinder = this->GetInt({ "bytes_per_cylinder" }, 24, nullptr);
                    int32_t offsetVertex0 = this->GetInt({ "offset_v0" }, 0, nullptr);
                    int32_t offsetVertex1 = this->GetInt({ "offset_v1" }, 12, nullptr);
                    int32_t offsetRadius = this->GetInt({ "offset_radius" }, -1, nullptr);

                    uint32_t numCylinders = cylinders->GetNumElements() * cylinders->GetElementSize() / bytesPerCylinder;
                    cyl->SetCylindersAndColors(numCylinders, cylinders->GetData(), bytesPerCylinder, offsetVertex0, offsetVertex1, offsetRadius, colors);


                    Data* texcoord = GetObject<Data>({ "vertex.texcoord" });
                    if (texcoord)
                    {
                        VisRTX::Vec2f* texcoords = reinterpret_cast<VisRTX::Vec2f*>(texcoord->GetData());
                        assert(texcoord->GetElementDataType() == RTW_VEC2F);
                        cyl->SetTexCoords(texcoords);
                    }
                    else
                    {
                        cyl->SetTexCoords(nullptr);
                    }

                    Data* materialList = GetObject<Data>({ "material" });
                    if (materialList)
                    {
                        assert(materialList->GetElementDataType() == RTW_MATERIAL);

                        std::vector<VisRTX::Material*> cylinderMaterials;
                        cylinderMaterials.resize(numCylinders);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());

                        for (uint32_t i = 0; i < numCylinders; ++i)
                        {
                            Material* materialHandle = materials[i];
                            if (materialHandle)
                                cylinderMaterials[i] = materialHandle->material;
                        }

                        cyl->SetMaterials(cylinderMaterials.data());
                    }
                    else
                    {
                        cyl->SetMaterials(nullptr);
                    }
                }
                else if(cylinders = GetObject<Data>({"vertex.position_radius"}))
                {
                    //Scale-array variant
                    VisRTX::Vec4f* colors = nullptr;
                    Data* color = GetObject<Data>({ "color" });
                    if (color)
                    {
                        colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetElementDataType() == RTW_VEC4F);
                    }

                    int32_t bytesPerCylinder = this->GetInt({ "bytes_per_cylinder" }, 64, nullptr);
                    int32_t offsetVertex0 = this->GetInt({ "offset_v0" }, 0, nullptr);
                    int32_t offsetVertex1 = this->GetInt({ "offset_v1" }, 32, nullptr);
                    int32_t offsetRadius = this->GetInt({ "offset_radius" }, 12, nullptr);

                    uint32_t numCylinders = cylinders->GetNumElements() * cylinders->GetElementSize() / bytesPerCylinder;
                    cyl->SetCylindersAndColors(numCylinders, cylinders->GetData(), bytesPerCylinder, offsetVertex0, offsetVertex1, offsetRadius, colors);


                    Data* texcoord = GetObject<Data>({ "vertex.texcoord" });
                    if (texcoord)
                    {
                        VisRTX::Vec2f* texcoords = reinterpret_cast<VisRTX::Vec2f*>(texcoord->GetData());
                        assert(texcoord->GetElementDataType() == RTW_VEC2F);
                        cyl->SetTexCoords(texcoords);
                    }
                    else
                    {
                        cyl->SetTexCoords(nullptr);
                    }

                    Data* materialList = GetObject<Data>({ "material" });
                    if (materialList)
                    {
                        assert(materialList->GetElementDataType() == RTW_MATERIAL);

                        std::vector<VisRTX::Material*> cylinderMaterials;
                        cylinderMaterials.resize(numCylinders);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());

                        for (uint32_t i = 0; i < numCylinders; ++i)
                        {
                            Material* materialHandle = materials[i];
                            if (materialHandle)
                                cylinderMaterials[i] = materialHandle->material;
                        }

                        cyl->SetMaterials(cylinderMaterials.data());
                    }
                    else
                    {
                        cyl->SetMaterials(nullptr);
                    }
                }
                else
                {
                    assert(false);
                }

                float radius;
                if (this->GetFloat({ "radius" }, &radius))
                    cyl->SetRadius(radius);
            }

            else
            {
                assert(false);
            }


            // Set a default material if none is set
            if (!this->material)
            {
                VisRTX::Context* rtx = VisRTX_GetContext();
                this->geometry->SetMaterial(rtx->CreateBasicMaterial());
            }
        }

        void SetMaterial(Material* material)
        {
            if (!this->geometry)
                return;

            // Release current material
            if (this->material)
                this->material->Release();

            if (material)
            {
                this->geometry->SetMaterial(material->material);
                this->material = material;
                this->material->AddRef();
            }
            else
            {
                this->geometry->SetMaterial(nullptr);
                this->material = nullptr;
            }
        }

    private:
        VisRTX::Geometry* geometry = nullptr;
        Material* material = nullptr;
    };
}
