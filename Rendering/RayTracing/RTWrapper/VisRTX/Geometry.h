#pragma once

#include "../Types.h"
#include "Material.h"

#include <VisRTX.h>
#include <cassert>

namespace RTW
{
    class Geometry : public Object
    {
        friend class Model;

    public:
        Geometry(const std::string& type)
        {
            VisRTX::Context* rtx = VisRTX_GetContext();

            if (type == "triangles" || type == "trianglemesh")
                this->geometry = rtx->CreateTriangleGeometry();
            else if (type == "spheres")
                this->geometry = rtx->CreateSphereGeometry();
            else if (type == "cylinders")
                this->geometry = rtx->CreateCylinderGeometry();
            else if (type == "isosurfaces")
                ; // not implemented
            else
            {
                std::cerr << "Error: Unhandled geometry type \"" << type << "\"" << std::endl;
                assert(false);
            }
        }

        ~Geometry()
        {
            this->geometry->Release();
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

                Data* vertex = this->GetObject<Data>({ "position", "vertex" });
                Data* index = this->GetObject<Data>({ "index" });
                if (vertex && index)
                {
                    uint32_t numTriangles = static_cast<uint32_t>(index->GetNumElements());
                    VisRTX::Vec3ui* triangles = reinterpret_cast<VisRTX::Vec3ui*>(index->GetData());
                    assert(index->GetDataType() == RTW_INT3);

                    uint32_t numVertices = static_cast<uint32_t>(vertex->GetNumElements());
                    VisRTX::Vec3f* vertices = reinterpret_cast<VisRTX::Vec3f*>(vertex->GetData());
                    assert(vertex->GetDataType() == RTW_FLOAT3);

                    VisRTX::Vec3f* normals = nullptr;
                    Data* normal = this->GetObject<Data>({ "vertex.normal" });
                    if (normal)
                    {
                        normals = reinterpret_cast<VisRTX::Vec3f*>(normal->GetData());
                        assert(normal->GetDataType() == RTW_FLOAT3);
                    }

                    tri->SetTriangles(numTriangles, triangles, numVertices, vertices, normals);


                    Data* color = this->GetObject<Data>({ "vertex.color" });
                    if (color)
                    {
                        VisRTX::Vec4f* colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetDataType() == RTW_FLOAT4);
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
                        assert(texcoord->GetDataType() == RTW_FLOAT2);
                        tri->SetTexCoords(texcoords);
                    }
                    else
                    {
                        tri->SetTexCoords(nullptr);
                    }


                    Data* materialList = GetObject<Data>({ "materialList" });
                    Data* materialIndices = GetObject<Data>({ "prim.materialID" });
                    if (materialList && materialIndices)
                    {
                        assert(materialList->GetDataType() == RTW_OBJECT);
                        assert(materialIndices->GetDataType() == RTW_INT);

                        std::vector<VisRTX::Material*> triangleMaterials;
                        triangleMaterials.resize(numTriangles);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());
                        int* indices = reinterpret_cast<int*>(materialIndices->GetData());

                        for (uint32_t i = 0; i < numTriangles; ++i)
                        {
                            int triIndex = indices[i];
                            if (triIndex >= 0)
                            {
                                Material* materialHandle = materials[triIndex];
                                if (materialHandle)
                                    triangleMaterials[i] = materialHandle->material;
                            }
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

                Data* spheres = GetObject<Data>({ "spheres" });
                if (spheres)
                {
                    VisRTX::Vec4f* colors = nullptr;
                    Data* color = GetObject<Data>({ "color" });
                    if (color)
                    {
                        colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetDataType() == RTW_FLOAT4);
                    }

                    int32_t bytesPerSphere = this->Get1i({ "bytes_per_sphere" }, 16, nullptr);
                    int32_t offsetCenter = this->Get1i({ "offset_center" }, 0, nullptr);
                    int32_t offsetRadius = this->Get1i({ "offset_radius" }, -1, nullptr);
                    int32_t offsetColorIndex = this->Get1i({ "offset_colorID" }, -1, nullptr);

                    uint32_t numSpheres = spheres->GetNumElements() * spheres->GetElementSize() / bytesPerSphere;

                    sphere->SetSpheresAndColors(numSpheres, spheres->GetData(), bytesPerSphere, offsetCenter, offsetRadius, offsetColorIndex, colors);

                    Data* texcoord = GetObject<Data>({ "texcoord" });
                    if (texcoord)
                    {
                        VisRTX::Vec2f* texcoords = reinterpret_cast<VisRTX::Vec2f*>(texcoord->GetData());
                        assert(texcoord->GetDataType() == RTW_FLOAT2);
                        sphere->SetTexCoords(texcoords);
                    }
                    else
                    {
                        sphere->SetTexCoords(nullptr);
                    }

                    Data* materialList = GetObject<Data>({ "materialList" });
                    int offset_materialID = this->Get1i({ "offset_materialID" }, -1);
                    if (materialList && offset_materialID >= 0)
                    {
                        assert(materialList->GetDataType() == RTW_OBJECT);

                        std::vector<VisRTX::Material*> sphereMaterials;
                        sphereMaterials.resize(numSpheres);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());

                        const uint8_t* base = reinterpret_cast<const uint8_t*>(spheres->GetData());

                        for (uint32_t i = 0; i < numSpheres; ++i)
                        {
                            int index = *reinterpret_cast<const int*>(base + i * bytesPerSphere + offset_materialID);
                            if (index >= 0)
                            {
                                Material* materialHandle = materials[index];
                                if (materialHandle)
                                    sphereMaterials[i] = materialHandle->material;
                            }
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
                if (this->Get1f({ "radius" }, &radius))
                    sphere->SetRadius(radius);
            }

            /*
             * Cylinders
             */
            else if (this->geometry->GetType() == VisRTX::GeometryType::CYLINDERS)
            {
                VisRTX::CylinderGeometry* cyl = dynamic_cast<VisRTX::CylinderGeometry*>(this->geometry);

                Data* cylinders = GetObject<Data>({ "cylinders" });
                if (cylinders)
                {
                    VisRTX::Vec4f* colors = nullptr;
                    Data* color = GetObject<Data>({ "color" });
                    if (color)
                    {
                        colors = reinterpret_cast<VisRTX::Vec4f*>(color->GetData());
                        assert(color->GetDataType() == RTW_FLOAT4);
                    }

                    int32_t bytesPerCylinder = this->Get1i({ "bytes_per_cylinder" }, 24, nullptr);
                    int32_t offsetVertex0 = this->Get1i({ "offset_v0" }, 0, nullptr);
                    int32_t offsetVertex1 = this->Get1i({ "offset_v1" }, 12, nullptr);
                    int32_t offsetRadius = this->Get1i({ "offset_radius" }, -1, nullptr);

                    uint32_t numCylinders = cylinders->GetNumElements() * cylinders->GetElementSize() / bytesPerCylinder;

                    cyl->SetCylindersAndColors(numCylinders, cylinders->GetData(), bytesPerCylinder, offsetVertex0, offsetVertex1, offsetRadius, colors);

                    Data* texcoord = GetObject<Data>({ "texcoord" });
                    if (texcoord)
                    {
                        VisRTX::Vec2f* texcoords = reinterpret_cast<VisRTX::Vec2f*>(texcoord->GetData());
                        assert(texcoord->GetDataType() == RTW_FLOAT2);
                        cyl->SetTexCoords(texcoords);
                    }
                    else
                    {
                        cyl->SetTexCoords(nullptr);
                    }

                    Data* materialList = GetObject<Data>({ "materialList" });
                    int offset_materialID = this->Get1i({ "offset_materialID" }, -1);
                    if (materialList && offset_materialID >= 0)
                    {
                        assert(materialList->GetDataType() == RTW_OBJECT);

                        std::vector<VisRTX::Material*> cylinderMaterials;
                        cylinderMaterials.resize(numCylinders);

                        Material** materials = reinterpret_cast<Material**>(materialList->GetData());

                        const uint8_t* base = reinterpret_cast<const uint8_t*>(cylinders->GetData());

                        for (uint32_t i = 0; i < numCylinders; ++i)
                        {
                            int index = *reinterpret_cast<const int*>(base + i * bytesPerCylinder + offset_materialID);
                            if (index >= 0)
                            {
                                Material* materialHandle = materials[index];
                                if (materialHandle)
                                    cylinderMaterials[i] = materialHandle->material;
                            }
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
                if (this->Get1f({ "radius" }, &radius))
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
