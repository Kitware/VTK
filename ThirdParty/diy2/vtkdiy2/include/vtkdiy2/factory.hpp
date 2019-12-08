#ifndef DIY_FACTORY_HPP
#define DIY_FACTORY_HPP

// From http://www.nirfriedman.com/2018/04/29/unforgettable-factory/
// with minor changes.

#include <memory>
#include <string>
#include <unordered_map>

namespace diy
{

template <class Base, class... Args>
class Factory
{
    public:
        template <class... T>
        static Base* make(const std::string &s, T&&... args)
        {
            return data().at(s)(std::forward<T>(args)...);
        }

        virtual std::string id() const          { return typeid(Base).name(); }

        template <class T>
        struct Registrar: Base
        {
            friend T;

            static bool registerT()
            {
                const auto name = typeid(T).name();
                Factory::data()[name] = [](Args... args) -> Base*
                {
                    return new T(std::forward<Args>(args)...);
                };
                return true;
            }
            static bool registered;

            std::string id() const override     { return typeid(T).name(); }

            private:
                Registrar(): Base(Key{}) { (void)registered; }
        };

        friend Base;

    private:
        class Key
        {
            Key(){};
            template <class T> friend struct Registrar;
        };

        using FuncType = Base* (*)(Args...);

        Factory() = default;

        static std::unordered_map<std::string, FuncType>& data()
        {
            static std::unordered_map<std::string, FuncType> s;
            return s;
        }
};

template <class Base, class... Args>
template <class T>
bool Factory<Base, Args...>::Registrar<T>::registered = Factory<Base, Args...>::Registrar<T>::registerT();

}

#endif
