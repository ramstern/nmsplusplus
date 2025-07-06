#include "../header/MonoLayer.h"
#include "../header/engine_io.h"

MonoLayer mono_layer = MonoLayer("libMBIN.dll");

static void PrintCallback(const char* msg, mono_bool is_stdout)
{
    Logger::Info("{0}", msg);
}

static void LogCallback(const char* log_domain, const char* log_level, const char* msg, mono_bool fatal, void* user_data)
{
    Logger::Info("[{0}]: {1}", log_domain, msg);
    //printf("%s\n", msg);
    if(fatal) Logger::Critical("Critical error!");
}

MonoLayer::MonoLayer(const char* dll)
{
	Logger::Info("Initializing mono...");

    mono_trace_set_level_string("debug");
    mono_trace_set_mask_string("asm");
    mono_trace_set_log_handler(&LogCallback, nullptr);

	mono_set_assemblies_path("lib/mono/");

	MonoDomain* _root_domain = mono_jit_init("Runtime");
	if (!_root_domain)
	{
		Logger::Critical("Failed to initialize jit compiler!");
		return;
	}
	root_domain = _root_domain;

	app_domain = mono_domain_create_appdomain(const_cast<char*>("AppDomain"), nullptr);
	mono_domain_set(app_domain, true);
    mono_domain_set_config(app_domain, "lib/mono/", "machine.config");
        

    assembly = LoadCSharpAssembly("libMBIN.dll");
    //PrintAssemblyTypes();
}

MonoLayer::~MonoLayer()
{
    if (app_domain)
    {
        mono_domain_unload(app_domain);

        app_domain = nullptr;
    }

    
    if (root_domain)
    {
        mono_domain_set(root_domain, false);

        mono_jit_cleanup(root_domain);
        root_domain = nullptr;
    }
}

void MonoLayer::LogMonoObjectFields(MonoObject* obj)
{
    std::vector<MonoClassField*> mono_fields;
    void* iter = nullptr;
    MonoClassField* field;

    MonoClass* object_class = mono_object_get_class(obj);
    const char* class_name = mono_class_get_name(object_class);

    while ((field = mono_class_get_fields(object_class, &iter)))
    {
        mono_fields.push_back(field);
    }

    for (MonoClassField* field : mono_fields)
    {
        const char* field_name = mono_field_get_name(field);

        // Get the field value as a MonoObject*
        MonoObject* field_value_obj = mono_field_get_value_object(app_domain, field, obj);

        // Handle null values
        if (field_value_obj == nullptr) {
            Logger::Info("{0} = null", field_name);
            continue;
        }

        // Convert the field value to a string
        MonoObject* exception;
        MonoString* field_value_str = mono_object_to_string(field_value_obj, &exception);
        
        if(exception) HandleMonoException(exception, class_name);
        
        char* fieldValueCString = mono_string_to_utf8(field_value_str);

        Logger::Info("{0} = {1}", field_name, fieldValueCString);

        mono_free(fieldValueCString);
    }

}

MonoAssembly* MonoLayer::LoadCSharpAssembly(const std::string& assembly_path)
{
    uint32_t file_size = 0;
    char* file_data = IO::ReadBytes(assembly_path, file_size);

    MonoImageOpenStatus status;
    MonoImage* image = mono_image_open_from_data_full(file_data, file_size, 1, &status, 0);

    if (status != MONO_IMAGE_OK)
    {
        const char* error_message = mono_image_strerror(status);

		Logger::Error("Mono image open from data failed! {0}", error_message);

        return nullptr;
    }
    MonoAssembly* assembly = mono_assembly_load_from_full(image, assembly_path.c_str(), &status, 0);

    if(!assembly)
    {
        Logger::Error("Failed to load assembly!");
    }

    mono_image_close(image);

    //free the file data
    delete[] file_data;

    return assembly;
}

MonoClass* MonoLayer::GetClass(const char* const& namespace_name, const char* const& class_name)
{
    MonoImage* image = mono_assembly_get_image(assembly);
    MonoClass* _class = mono_class_from_name_case(image, namespace_name, class_name);

    if(!_class)
    {
        Logger::Error("No class found with name {0}.{1}", namespace_name, class_name);
        return nullptr;
    }

    return _class;
}

void MonoLayer::HandleMonoException(MonoObject* exception, const char*& class_name)
{
    if(!exception) return;

    MonoClass* exception_class = mono_object_get_class(exception);

    MonoProperty* message_property = mono_class_get_property_from_name(exception_class, "Message");
    MonoMethod* get_message_method = mono_property_get_get_method(message_property);

    MonoString* exception_message = (MonoString*)mono_runtime_invoke(get_message_method, exception, nullptr, nullptr);
    char* exception_message_cstr = mono_string_to_utf8(exception_message);

    Logger::Error("Exception occurred in class {0}: {1}", class_name, exception_message_cstr);

    mono_free(exception_message_cstr);

    MonoProperty* stack_trace_property = mono_class_get_property_from_name(exception_class, "StackTrace");
    MonoMethod* get_stack_trace_method = mono_property_get_get_method(stack_trace_property);

    MonoString* exception_stack_trace = (MonoString*)mono_runtime_invoke(get_stack_trace_method, exception, nullptr, nullptr);
    char* exception_stack_trace_cstr = mono_string_to_utf8(exception_stack_trace);

    Logger::Error("Exception stack trace: {0}", exception_stack_trace_cstr);

    mono_free(exception_stack_trace_cstr);

    HandleInnerExceptions(exception);
}

void MonoLayer::HandleInnerExceptions(MonoObject* exception)
{
    MonoClass* exception_class = mono_object_get_class(exception);

    MonoProperty* inner_exception_property = mono_class_get_property_from_name(exception_class, "InnerException");
    MonoMethod* get_inner_exception_method = mono_property_get_get_method(inner_exception_property);
    MonoObject* inner_exception = mono_runtime_invoke(get_inner_exception_method, exception, nullptr, nullptr);

    if (inner_exception) {
        MonoClass* inner_exception_class = mono_object_get_class(inner_exception);

        // Get the inner exception message
        MonoProperty* inner_message_property = mono_class_get_property_from_name(inner_exception_class, "Message");
        MonoMethod* inner_get_message_method = mono_property_get_get_method(inner_message_property);

        MonoString* inner_exception_message = (MonoString*)mono_runtime_invoke(inner_get_message_method, inner_exception, nullptr, nullptr);
        char* inner_message_cstr = mono_string_to_utf8(inner_exception_message);
        //std::cout << "Inner Exception Message: " << innerMessage << std::endl;
        Logger::Error("Inner exception occurred: {0}", inner_message_cstr);
        mono_free(inner_message_cstr);

        HandleInnerExceptions(inner_exception);
    }
}

void MonoLayer::PrintAssemblyTypes()
{
    MonoImage* image = mono_assembly_get_image(assembly);
    const MonoTableInfo* type_definitions_table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
    int32_t num_types = mono_table_info_get_rows(type_definitions_table);

    for (int32_t i = 0; i < num_types; i++)
    {
        uint32_t cols[MONO_TYPEDEF_SIZE];
        mono_metadata_decode_row(type_definitions_table, i, cols, MONO_TYPEDEF_SIZE);

        const char* _namespace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
        const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

        if(strcmp(_namespace, "") == 0)
        {
            Logger::Info("{0}", name);
        }
        else 
        {
            Logger::Info("{0}.{1}", _namespace, name);
        }
    }
}

void MonoLayer::PrepareArgsVec(std::vector<void*>&) {} // base case for recursion

void* MonoLayer::ObjectConverter::ConvertToMonoObject(int value)
{
    // only for int32 rn FIXME
    MonoClass* int_class = mono_get_int32_class();
    return mono_value_box(mono_layer.app_domain, int_class, &value);
}

void* MonoLayer::ObjectConverter::ConvertToMonoObject(float value)
{
    MonoClass* float_class = mono_get_single_class();
    return mono_value_box(mono_layer.app_domain,float_class, &value);
}

void* MonoLayer::ObjectConverter::ConvertToMonoObject(const std::string& value)
{
    return mono_string_new(mono_layer.app_domain, value.c_str());
}
