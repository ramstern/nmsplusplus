#pragma once
#include <string>
#include "mono/metadata/assembly.h"
#include "mono/jit/jit.h"
#include "mono/utils/mono-logger.h"
#include "../header/Logger.h"
#include <vector>

class MonoLayer
{
public:
	MonoLayer(const char* dll);

	~MonoLayer();

	MonoClass* GetClass(const char* const& namespace_name, const char* const& class_name);

	template <typename...Args>
	MonoObject* CreateInstanceOfClass(MonoClass* mono_class, Args&&...);

    template <typename...Args>
    MonoObject* RuntimeInvoke(MonoClass* _class, MonoObject* object, const char* const& invokable_name, Args&&... args);

    MonoAssembly* GetAssembly() const {return assembly;}
    MonoDomain* GetDomain() const {return app_domain;}

    void LogMonoObjectFields(MonoObject* obj);

    void HandleMonoException(MonoObject* exception, const char*& class_name);


    struct ObjectConverter
    {
        void* ConvertToMonoObject(int value);
        void* ConvertToMonoObject(float value);
        void* ConvertToMonoObject(const std::string& value);
    }object_converter;

private:
    MonoAssembly* LoadCSharpAssembly(const std::string& assemblyPath);
    void PrintAssemblyTypes();


    void HandleInnerExceptions(MonoObject* exception);

    template <typename T, typename... Rest>
    void PrepareArgsVec(std::vector<void*>& args_vec, T&& first, Rest&&... rest);

    void PrepareArgsVec(std::vector<void*>&);

    

    MonoAssembly* assembly = nullptr;
    MonoDomain* root_domain = nullptr;
    MonoDomain* app_domain = nullptr;



};

template <typename...Args>
MonoObject* MonoLayer::CreateInstanceOfClass(MonoClass* mono_class, Args&&... args)
{
    MonoObject* mono_object = mono_object_new(app_domain, mono_class);
    const char* mono_class_name = mono_class_get_name(mono_class);

    if (!mono_object)
    {
        Logger::Error("Failed to create an instance of class {0}", mono_class_name);
        return nullptr;
    }

    RuntimeInvoke(mono_class, mono_object, ".ctor", std::forward(args)...);

    return mono_object;
}

template<typename ...Args>
MonoObject* MonoLayer::RuntimeInvoke(MonoClass* _class, MonoObject* object, const char* const& invokable_name, Args&& ...args)
{
    const char* mono_class_name = mono_class_get_name(_class);
    MonoMethod* invokable = mono_class_get_method_from_name(_class, invokable_name, sizeof...(Args));
    if (!invokable)
    {
        Logger::Error("Invalid parameter arguments for invokable {0}::{1}", mono_class_name, invokable_name);
        return nullptr;
    }

    std::vector<void*> args_vector;
    Logger::Info("args size: {0}", sizeof...(Args));
    if(sizeof...(Args) > 0)
    {
        PrepareArgsVec(args_vector, std::forward<Args>(args)...);
    }

    MonoObject* exception = nullptr;
    MonoObject* return_object = mono_runtime_invoke(invokable, object, args_vector.empty() ? nullptr : args_vector.data(), &exception); 

    if (exception)
    {
        HandleMonoException(exception, mono_class_name);
        return nullptr;
    }

    return return_object; 
}

template <typename T, typename... Rest>
void MonoLayer::PrepareArgsVec(std::vector<void*>& args_vec, T&& first, Rest&&... rest)
{
    args_vec.push_back(object_converter.ConvertToMonoObject(std::forward<T>(first)));

    PrepareArgsVec(args_vec, std::forward<Rest>(rest)...);
}

extern MonoLayer mono_layer;