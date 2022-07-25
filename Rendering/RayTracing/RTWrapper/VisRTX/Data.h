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
            case RTW_UCHAR:
                return 1;
            case RTW_VEC2UC:
            case RTW_SHORT:
            case RTW_USHORT:
                return 2;
            case RTW_VEC3UC:
                return 3;
            case RTW_VEC4UC:
            case RTW_INT:
            case RTW_UINT:
            case RTW_FLOAT:
                return 4;
            case RTW_VEC2I:
            case RTW_VEC2UI:
            case RTW_VEC2F:
            case RTW_DOUBLE:
                return 8;
            case RTW_VEC3I:
            case RTW_VEC3UI:
            case RTW_VEC3F:
                return 12;
            case RTW_VEC4I:
            case RTW_VEC4UI:
            case RTW_VEC4F:
                return 16;

            default:
                if(type >= RTW_OBJECT && type <= RTW_WORLD)
                    return sizeof(Object *);
                else return 0;
            }
        }

    public:

        Data(const void *source, RTWDataType type, size_t width, bool shared = false)
            : Data(source, type, width, 1, 1, shared) {};

        Data(const void *source, RTWDataType type, size_t width, size_t height, bool shared = false)
            : Data(source, type, width, height, 1, shared) {};

        Data(const void *source, RTWDataType type, size_t width, size_t height, size_t depth, bool shared = false)
            : Object(RTW_DATA)
        {
            this->width = width;
            this->height = height;
            this->depth = depth;
            this->type = type;
            this->elementSize = GetElementSize(type);
            this->shared = shared;

            if (this->shared)
            {
                this->data = reinterpret_cast<uint8_t*>(const_cast<void*>(source));
            }
            else
            {
                size_t size = GetNumElements() * this->elementSize;
                this->data = new uint8_t[size];
                memcpy(this->data, source, size);
            }

            // Increase references
            if(type >= RTW_OBJECT && type <= RTW_WORLD)
            {
                for (size_t i = 0; i < GetNumElements(); ++i)
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
            if(type >= RTW_OBJECT && type <= RTW_WORLD)
            {
                for (size_t i = 0; i < GetNumElements(); ++i)
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
            return this->width * this->height * this->depth;
        }

        size_t GetWidth() const
        {
            return this->width;
        }

        size_t GetHeight() const
        {
            return this->height;
        }

        size_t GetDepth() const
        {
            return this->depth;
        }

        RTWDataType GetElementDataType() const
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
        size_t width = 0, height = 1, depth = 1;
        RTWDataType type;
        size_t elementSize = 0;
        uint8_t* data = nullptr;
        bool shared = false;
        bool dirty = true;
    };
}
