#pragma once

#include "../RTWrapper.h"

#include <VisRTX.h>

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace RTW
{
    class Data;

    class Object
    {
    public:
        Object()
        {
            this->AddRef();
        }

        virtual ~Object()
        {
            // Release all objects
            for (auto it : this->objectMap.map)
                if (it.second)
                    it.second->Release();
        }

        virtual void Commit() = 0;

    public:
        void AddRef()
        {
            ++this->refCount;
        }

        void Release()
        {
            if (--refCount <= 0)
                delete this;
        }

    private:
        int64_t refCount = 0;

    public:
        inline void SetString(const std::string& id, const std::string& s)
        {
            this->stringMap.Set(id, s);
        }

        inline const std::string GetString(const std::vector<std::string>& ids, const std::string& defaultValue = "", bool* found = nullptr) const
        {
            return this->stringMap.Get(ids, defaultValue, found);
        }

        inline bool GetString(const std::vector<std::string>& ids, std::string* result, const std::string& defaultValue = nullptr)
        {
            bool found;
            *result = this->GetString(ids, defaultValue, &found);
            return found;
        }

        template<typename T = Object>
        inline void SetObject(const std::string& id, T* object)
        {
            // Check if already exists and release
            Object* current = this->objectMap.Get({ id }, nullptr);
            if (current)
                current->Release();

            // Set new object and add reference
            if (object)
            {
                this->objectMap.Set(id, object);
                object->AddRef();
            }
            else
            {
                this->objectMap.Remove(id);
            }
        }

        template<typename T = Object>
        inline T* GetObject(const std::vector<std::string>& ids, T* defaultValue = nullptr, bool* found = nullptr) const
        {
            return reinterpret_cast<T*>(this->objectMap.Get(ids, reinterpret_cast<Object*>(defaultValue), found));
        }

        template<typename T = Object>
        inline bool GetObject(const std::vector<std::string>& ids, T** result, T* defaultValue = nullptr)
        {
            bool found;
            *result = this->GetObject<T>(ids, defaultValue, &found);
            return found;
        }

        inline void Set1i(const std::string& id, int32_t x)
        {
            this->int1Map.Set(id, x);
        }

        inline int32_t Get1i(const std::vector<std::string>& ids, int32_t defaultValue = 0, bool* found = nullptr) const
        {
            return this->int1Map.Get(ids, defaultValue, found);
        }

        inline bool Get1i(const std::vector<std::string>& ids, int32_t* result, int32_t defaultValue = 0)
        {
            bool found;
            *result = this->Get1i(ids, defaultValue, &found);
            return found;
        }

        inline void Set1f(const std::string& id, float x)
        {
            this->float1Map.Set(id, x);
        }

        inline float Get1f(const std::vector<std::string>& ids, float defaultValue = 0.0f, bool* found = nullptr) const
        {
            return this->float1Map.Get(ids, defaultValue, found);
        }

        inline bool Get1f(const std::vector<std::string>& ids, float* result, float defaultValue = 0.0f)
        {
            bool found;
            *result = this->Get1f(ids, defaultValue, &found);
            return found;
        }

        inline void Set2i(const std::string& id, int32_t x, int32_t y)
        {
          this->int2Map.Set(id, VisRTX::Vec2i(x, y));
        }

        inline VisRTX::Vec2i Get2i(const std::vector<std::string>& ids, const VisRTX::Vec2i& defaultValue = VisRTX::Vec2i(), bool* found = nullptr) const
        {
          return this->int2Map.Get(ids, defaultValue, found);
        }

        inline bool Get2i(const std::vector<std::string>& ids, VisRTX::Vec2i* result, const VisRTX::Vec2i& defaultValue = VisRTX::Vec2i())
        {
          bool found;
          *result = this->Get2i(ids, defaultValue, &found);
          return found;
        }

        inline void Set2f(const std::string& id, float x, float y)
        {
            this->float2Map.Set(id, VisRTX::Vec2f(x, y));
        }

        inline VisRTX::Vec2f Get2f(const std::vector<std::string>& ids, const VisRTX::Vec2f& defaultValue = VisRTX::Vec2f(), bool* found = nullptr) const
        {
            return this->float2Map.Get(ids, defaultValue, found);
        }

        inline bool Get2f(const std::vector<std::string>& ids, VisRTX::Vec2f* result, const VisRTX::Vec2f& defaultValue = VisRTX::Vec2f())
        {
            bool found;
            *result = this->Get2f(ids, defaultValue, &found);
            return found;
        }

        inline void Set3i(const std::string& id, int32_t x, int32_t y, int32_t z)
        {
            this->int3Map.Set(id, VisRTX::Vec3i(x, y, z));
        }

        inline VisRTX::Vec3i Get3i(const std::vector<std::string>& ids, const VisRTX::Vec3i& defaultValue = VisRTX::Vec3i(), bool* found = nullptr) const
        {
            return this->int3Map.Get(ids, defaultValue, found);
        }

        inline bool Get3i(const std::vector<std::string>& ids, VisRTX::Vec3i* result, const VisRTX::Vec3i& defaultValue = VisRTX::Vec3i())
        {
            bool found;
            *result = this->Get3i(ids, defaultValue, &found);
            return found;
        }

        inline void Set3f(const std::string& id, float x, float y, float z)
        {
            this->float3Map.Set(id, VisRTX::Vec3f(x, y, z));
        }

        inline VisRTX::Vec3f Get3f(const std::vector<std::string>& ids, const VisRTX::Vec3f& defaultValue = VisRTX::Vec3f(), bool* found = nullptr) const
        {
            return this->float3Map.Get(ids, defaultValue, found);
        }

        inline bool Get3f(const std::vector<std::string>& ids, VisRTX::Vec3f* result, const VisRTX::Vec3f& defaultValue = VisRTX::Vec3f())
        {
            bool found;
            *result = this->Get3f(ids, defaultValue, &found);
            return found;
        }

        inline void Set4f(const std::string& id, float x, float y, float z, float w)
        {
            this->float4Map.Set(id, VisRTX::Vec4f(x, y, z, w));
        }

        inline VisRTX::Vec4f Get4f(const std::vector<std::string>& ids, const VisRTX::Vec4f& defaultValue = VisRTX::Vec4f(), bool* found = nullptr) const
        {
            return this->float4Map.Get(ids, defaultValue, found);
        }

        inline bool Get4f(const std::vector<std::string>& ids, VisRTX::Vec4f* result, const VisRTX::Vec4f& defaultValue = VisRTX::Vec4f())
        {
            bool found;
            *result = this->Get4f(ids, defaultValue, &found);
            return found;
        }

        void RemoveParam(const std::string& id)
        {
            this->stringMap.Remove(id);
            this->objectMap.Remove(id);
            this->int1Map.Remove(id);
            this->float1Map.Remove(id);
            this->float2Map.Remove(id);
            this->int2Map.Remove(id);
            this->int3Map.Remove(id);
            this->float3Map.Remove(id);
            this->float4Map.Remove(id);
        }

    protected:
        void PrintAllParameters() const
        {
            for (auto it : this->stringMap.map)
                std::cout << "String: \"" << it.first << "\" -> \"" << it.second << "\"" << std::endl;

            for (auto it : this->objectMap.map)
                std::cout << "Object/Data: \"" << it.first << "\"" << std::endl;

            for (auto it : this->int1Map.map)
                std::cout << "int1: \"" << it.first << "\" -> " << it.second << std::endl;

            for (auto it : this->float1Map.map)
                std::cout << "float1: \"" << it.first << "\" -> " << it.second << std::endl;

            for (auto it : this->int2Map.map)
              std::cout << "int2: \"" << it.first << "\" -> (" << it.second.x << ", " << it.second.y << ")" << std::endl;

            for (auto it : this->float2Map.map)
                std::cout << "float2: \"" << it.first << "\" -> (" << it.second.x << ", " << it.second.y << ")" << std::endl;

            for (auto it : this->int3Map.map)
                std::cout << "int3: \"" << it.first << "\" -> (" << it.second.x << ", " << it.second.y << ", " << it.second.z << ")" << std::endl;

            for (auto it : this->float3Map.map)
                std::cout << "float3: \"" << it.first << "\" -> (" << it.second.x << ", " << it.second.y << ", " << it.second.z << ")" << std::endl;

            for (auto it : this->float4Map.map)
                std::cout << "float4: \"" << it.first << "\" -> (" << it.second.x << ", " << it.second.y << ", " << it.second.z << ", " << it.second.w << ")" << std::endl;
        }

        std::set<std::string> GetAllParameters() const
        {
            std::set<std::string> result;
            for (auto it : this->stringMap.map)
                result.insert("string " + it.first);

            for (auto it : this->objectMap.map)
                result.insert("object " + it.first);

            for (auto it : this->int1Map.map)
                result.insert("int1 " + it.first);

            for (auto it : this->float1Map.map)
                result.insert("float1 " + it.first);

            for (auto it : this->int2Map.map)
              result.insert("int2 " + it.first);

            for (auto it : this->float2Map.map)
                result.insert("float2 " + it.first);

            for (auto it : this->int3Map.map)
                result.insert("int3 " + it.first);

            for (auto it : this->float3Map.map)
                result.insert("float3 " + it.first);

            for (auto it : this->float4Map.map)
                result.insert("float4 " + it.first);
            return result;
        }

    private:
        template<typename T>
        class ParameterMap
        {
        public:
            inline void Set(const std::string& id, const T& value)
            {
                this->map[id] = value;
            }

            inline T Get(const std::vector<std::string>& ids, const T& defaultValueValue, bool* found = nullptr) const
            {
                for (const std::string& id : ids)
                {
                    auto it = this->map.find(id);
                    if (it != this->map.end())
                    {
                        if (found)
                            *found = true;
                        return (*it).second;
                    }
                }

                if (found)
                    *found = false;
                return defaultValueValue;
            }

            inline void Remove(const std::string& id)
            {
                auto it = this->map.find(id);
                if (it != this->map.end())
                    this->map.erase(it);
            }

        public:
            std::map<std::string, T> map;
        };

    private:
        ParameterMap<std::string> stringMap;
        ParameterMap<Object*> objectMap;

        ParameterMap<int32_t> int1Map;
        ParameterMap<float> float1Map;
        ParameterMap<VisRTX::Vec2f> float2Map;
        ParameterMap<VisRTX::Vec2i> int2Map;
        ParameterMap<VisRTX::Vec3i> int3Map;
        ParameterMap<VisRTX::Vec3f> float3Map;
        ParameterMap<VisRTX::Vec4f> float4Map;
    };
}
