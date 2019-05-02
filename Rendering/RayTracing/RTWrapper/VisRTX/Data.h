#pragma once

#include "../Types.h"

#include <cstring>

namespace RTW
{
    class Data : public Object
    {
    public:
        static size_t GetElementSize(RTWDataType type)
        {
            switch (type)
            {
            case RTW_OBJECT:
                return sizeof(Object*);
            case RTW_UCHAR:
                return 1;
            case RTW_UCHAR2:
            case RTW_SHORT:
            case RTW_USHORT:
                return 2;
            case RTW_UCHAR3:
                return 3;
            case RTW_UCHAR4:
            case RTW_INT:
            case RTW_FLOAT:
                return 4;
            case RTW_INT2:
            case RTW_FLOAT2:
            case RTW_DOUBLE:
                return 8;
            case RTW_INT3:
            case RTW_FLOAT3:
                return 12;
            case RTW_INT4:
            case RTW_FLOAT4:
            case RTW_FLOAT3A:
                return 16;

            default:
                return 0;
            }
        }

    public:
        Data(size_t numElements, RTWDataType type, const void *source, const uint32_t dataCreationFlags = 0)
        {
            this->numElements = numElements;
            this->type = type;
            this->elementSize = GetElementSize(type);
            this->shared = dataCreationFlags & RTW_DATA_SHARED_BUFFER;

            if (this->shared)
            {
                this->data = reinterpret_cast<uint8_t*>(const_cast<void*>(source));
            }
            else
            {
                size_t size = numElements * this->elementSize;
                this->data = new uint8_t[size];
                memcpy(this->data, source, size);
            }

            // Increase references
            if (type == RTW_OBJECT)
            {
                for (size_t i = 0; i < this->numElements; ++i)
                {
                    Object* obj = reinterpret_cast<Object**>(this->data)[i];
                    if (obj)
                        obj->AddRef();
                }
            }

            this->dirty = true;
        }

        ~Data()
        {
            // Release references
            if (type == RTW_OBJECT)
            {
                for (size_t i = 0; i < this->numElements; ++i)
                {
                    Object* obj = reinterpret_cast<Object**>(this->data)[i];
                    if (obj)
                        obj->Release();
                }
            }

            if (!this->shared)
                delete[] this->data;
        }

        void Commit() override
        {
            // Committing data marks it as dirty (useful for shared memory)
            this->dirty = true;
        }

        size_t GetNumElements() const
        {
            return this->numElements;
        }

        RTWDataType GetDataType() const
        {
            return this->type;
        }

        size_t GetElementSize() const
        {
            return GetElementSize(this->type);
        }

        void* GetData() const
        {
            return reinterpret_cast<void*>(this->data);
        }

        bool IsShared() const
        {
            return this->shared;
        }

        bool CheckDirty()
        {
            bool d = this->dirty;
            this->dirty = false;
            return d;
        }

    private:
        size_t numElements = 0;
        RTWDataType type;
        size_t elementSize = 0;
        uint8_t* data = nullptr;
        bool shared = false;
        bool dirty = true;
    };
}
