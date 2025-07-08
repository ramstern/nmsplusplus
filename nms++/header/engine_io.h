#pragma once
#include <string>
//#include "mono/metadata/object-forward.h"
#include "../header/MonoLayer.h"
#include "../header/traits_ext.h"

#include "../header/Logger.h" 
#include "pfr.hpp"

#include <fstream>

class IO
{
public:
	struct ResourceHandle
	{
		std::string path;
		MonoObject* obj = nullptr;
	};

	static void Initialize();

	[[nodiscard]] static char* ReadBytes(const std::string& path, uint32_t& out_size);

	static void WriteBytes(const std::string& out, const unsigned char* bytes, int flags = 0);

	//either exml or mbin
	[[nodiscard]] static MonoObject* LoadFile(const std::string& path);

	//has to have .mxml or .mbin in the path
	static void Write(MonoObject* to_write, const std::string& path);
	

	//get a native object from a MonoObject
	template <typename NativeType>
	[[nodiscard]] static NativeType GetNativeObject(MonoObject* from_object);

	//pushes data from a native type to a monoobject. returns success.
	template <typename NativeType>
	static bool PushData(NativeType& native_type, MonoObject* to_object);

	//immediate edit an exml or mbin in place.
	template <typename NativeType, typename Func>
	static void ImmediateEdit(struct IO::ResourceHandle& handle, Func&& edit);

	//PAK directory contents 
	static void PAKDirectoryContents(const std::string& directory_path, const std::string& pak_file_name);

private:
	static MonoClass* nms_template;
	static MonoClass* file_io;
	static MonoObject* file_io_obj;

	static void WriteMxml(MonoObject* to_write, const std::string& path);
	static void WriteMbin(MonoObject* to_write, const std::string& path);

	static void ExecCommand(const char* command);
};

template<typename NativeType>
NativeType IO::GetNativeObject(MonoObject* from_object)
{
	NativeType type{};

	MonoClass* object_class = mono_object_get_class(from_object);

	std::vector<MonoClassField*> mono_fields;
	void* iter = nullptr;
	MonoClassField* field;

	while ((field = mono_class_get_fields(object_class, &iter)))
	{
		mono_fields.push_back(field);
		// Logger::Info("{0}", mono_field_get_name(field));
	}

	//if auto is used, this may come in handy
	constexpr std::size_t field_count = pfr::tuple_size<NativeType>::value;
	if (field_count != mono_fields.size())
	{
		Logger::Error("field count does not match! possibly wrong object -> native type mapping");
		return NativeType{};
	}

	// Map fields
	std::size_t index = 0;
	pfr::for_each_field(type, [&](auto& field_value)
		{
			MonoClassField* mono_field = mono_fields[index];

			using FieldType = std::decay_t<decltype(field_value)>;
			using BareFieldType = std::remove_reference_t<decltype(field_value)>;

			if constexpr (std::is_same_v<FieldType, uint32_t>)
			{
				uint32_t value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, int32_t>)
			{
				int32_t value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, uint64_t>)
			{
				uint64_t value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, int64_t>)
			{
				int64_t value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, int16_t>)
			{
				int16_t value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, uint16_t>)
			{
				uint16_t value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, bool>)
			{
				bool value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = (value != 0);
			}
			else if constexpr (std::is_same_v<FieldType, float>)
			{
				float value;
				mono_field_get_value(from_object, mono_field, &value);
				field_value = value;
			}
			else if constexpr (std::is_same_v<FieldType, std::string>)
			{
				MonoString* monoStr;
				mono_field_get_value(from_object, mono_field, &monoStr);
				if (monoStr)
				{
					char* utf8Str = mono_string_to_utf8(monoStr);
					field_value = utf8Str;
					mono_free(utf8Str);
				}
				else
				{
					field_value.clear();
				}
			}
			else if constexpr (std::is_array<BareFieldType>::value)
			{
				using ElementType = std::remove_extent_t<BareFieldType>;
				MonoArray* mono_array;
				mono_field_get_value(from_object, mono_field, &mono_array);
				if (mono_array)
				{
					const uint32_t length = static_cast<uint32_t>(mono_array_length(mono_array));

					for (size_t i = 0; i < length; i++)
					{
						if constexpr (std::is_class_v<ElementType>)
						{
							MonoObject* element_obj = mono_array_get(mono_array, MonoObject*, i);
							if (element_obj) {
								field_value[i] = GetNativeObject<ElementType>(element_obj);
							}
							else {
								field_value[i] = ElementType{};
							}
						}
						else {
							ElementType element_value = mono_array_get(mono_array, ElementType, i);
							field_value[i] = element_value;
						}
					}
				}
			}
			else if constexpr (std::is_enum_v<FieldType>)
			{
				FieldType type{};
				mono_field_get_value(from_object, mono_field, &type);
				field_value = type;
			}
			else if constexpr (traits_ext::is_vector<FieldType>::value)
			{
				using ElementType = typename FieldType::value_type;

				MonoObject* list_obj = mono_field_get_value_object(mono_layer.GetDomain(), mono_field, from_object);

				if (list_obj)
				{
					MonoClass* list_class = mono_object_get_class(list_obj);
					MonoProperty* count_prop = mono_class_get_property_from_name(list_class, "Count");
					MonoMethod* get_count_method = mono_property_get_get_method(count_prop);

					//void* args[] = {};
					MonoObject* count_obj = mono_runtime_invoke(get_count_method, list_obj, nullptr, nullptr);
					int count = *(int*)mono_object_unbox(count_obj);

					// Get the Item[] property (indexer)
					MonoMethod* get_item_method = mono_class_get_method_from_name(list_class, "get_Item", 1);

					field_value.reserve(count);

					for (int i = 0; i < count; ++i) {
						void* index_args[] = { &i };

						MonoObject* exception = nullptr;
						MonoObject* item_obj = mono_runtime_invoke(get_item_method, list_obj, index_args, &exception);

						const char* list_name = "Generic.List";
						mono_layer.HandleMonoException(exception, list_name);

						// Process item_obj based on ElementType
						if constexpr (std::is_class_v<ElementType>) {
							field_value.push_back(GetNativeObject<ElementType>(item_obj));
						}
						else {
							ElementType value = *(ElementType*)mono_object_unbox(item_obj);
							field_value.push_back(value);
						}
					}
				}
				else
				{
					field_value.clear();
				}
			}
			else if constexpr (std::is_class_v<FieldType>)
			{
				MonoObject* nested_object = mono_field_get_value_object(mono_layer.GetDomain(), mono_field, from_object);
				if (nested_object)
				{
					field_value = GetNativeObject<FieldType>(nested_object);
				}
				else
				{
					field_value = FieldType{};
				}
			}

			index++;
		});

	return type;
}

template<typename NativeType>
bool IO::PushData(NativeType& native_type, MonoObject* to_object)
{
	MonoClass* object_class = mono_object_get_class(to_object);

	std::vector<MonoClassField*> mono_fields;
	void* iter = nullptr;
	MonoClassField* field;


	while ((field = mono_class_get_fields(object_class, &iter)))
	{
		mono_fields.push_back(field);
		//Logger::Info("{0}", mono_field_get_name(field));
	}

	//if auto is used, this may come in handy
	constexpr std::size_t field_count = pfr::tuple_size<NativeType>::value;
	if (field_count != mono_fields.size())
	{
		Logger::Error("No matching field count results in a no-op.");
		return false;
	}

	std::size_t index = 0;
	pfr::for_each_field(native_type, [&](auto& field_value)
		{
			if (index > mono_fields.size() - 1) return;

			MonoClassField* mono_field = mono_fields[index];

			using FieldType = std::decay_t<decltype(field_value)>;
			using BareFieldType = std::remove_reference_t<decltype(field_value)>;
			//using UnderlyingType = typename traits_ext::underlying_type<FieldType>::type; 

			//Logger::Info("{0}", typeid(UnderlyingType).name());

			if constexpr (std::is_same_v<FieldType, uint32_t>)
			{
				uint32_t value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, int32_t>)
			{
				int32_t value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, uint64_t>)
			{
				uint64_t value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, int64_t>)
			{
				int64_t value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, int16_t>)
			{
				int16_t value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, uint16_t>)
			{
				uint16_t value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, bool>)
			{
				bool value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, float>)
			{
				float value = field_value;
				mono_field_set_value(to_object, mono_field, &value);
			}
			else if constexpr (std::is_same_v<FieldType, std::string>)
			{
				MonoString* mono_str = mono_string_new(mono_layer.GetDomain(), field_value.c_str());
				mono_field_set_value(to_object, mono_field, mono_str);
			}
			else if constexpr (std::is_array<BareFieldType>::value)
			{
				using ElementType = std::remove_extent_t<BareFieldType>;

				//Logger::Info("{0}", typeid(ElementType).name());

				MonoType* field_mono_type = mono_field_get_type(mono_field);
				MonoClass* element_class = mono_type_get_array_type(field_mono_type)->eklass;

				constexpr size_t array_size = std::extent_v<BareFieldType>;

				if constexpr (std::is_class_v<ElementType>)
				{
					MonoArray* mono_array;
					mono_field_get_value(to_object, mono_field, &mono_array);

					for (size_t i = 0; i < array_size; i++)
					{
						MonoObject* element_obj = mono_object_new(mono_layer.GetDomain(), element_class);
						PushData(field_value[i], element_obj);

						mono_array_setref(mono_array, i, element_obj);
					}
				}
				else
				{
					MonoArray* mono_array = mono_array_new(mono_layer.GetDomain(), element_class, array_size);

					for (size_t i = 0; i < array_size; i++)
					{
						mono_array_set(mono_array, ElementType, i, field_value[i]);
					}

					//MonoObject* element_obj = mono_value_box(mono_layer.GetDomain(), element_class, value_array);

					mono_field_set_value(to_object, mono_field, mono_array);
				}



				//if(!mono_array)
				//{
				//    const char* name = mono_field_get_name(mono_field);
				//    //Logger::Critical("Null initialized array found. NEEDS fixing! {0}", name);

				//    MonoType* arr_type = mono_field_get_type(mono_field);
				//    MonoClass* arr_class = mono_type_get_class(arr_type);
				//    mono_array = reinterpret_cast<MonoArray*>(mono_object_new(mono_layer.GetDomain(), arr_class)); 

				//    //this is a bit wack... i dont know how libmbin assigns values without initializing a float[] array.
				//    //mono_layer.LogMonoObjectFields(to_object);
				//}


			}
			else if constexpr (std::is_enum_v<FieldType>)
			{
				FieldType type = field_value;
				mono_field_set_value(to_object, mono_field, &type);
			}
			else if constexpr (traits_ext::is_vector<FieldType>::value)
			{
				using ElementType = typename FieldType::value_type;

				//Logger::Info("{0}", mono_field_get_name(mono_field));

				MonoObject* list_obj = mono_field_get_value_object(mono_layer.GetDomain(), mono_field, to_object);

				if (list_obj == nullptr)
				{
					MonoType* list_type = mono_field_get_type(mono_field);
					MonoClass* list_class = mono_class_from_mono_type(list_type);

					list_obj = mono_object_new(mono_layer.GetDomain(), list_class);

					MonoObject* exception = nullptr;
					MonoMethod* ctor = mono_class_get_method_from_name(list_class, ".ctor", 0);

					mono_runtime_invoke(ctor, list_obj, nullptr, &exception);

					const char* list_class_name = mono_class_get_name(list_class);
					mono_layer.HandleMonoException(exception, list_class_name);

					mono_field_set_value(to_object, mono_field, list_obj);
				}

				MonoClass* list_class = mono_object_get_class(list_obj);
				MonoProperty* count_prop = mono_class_get_property_from_name(list_class, "Count");
				MonoMethod* get_count_method = mono_property_get_get_method(count_prop);
				//MonoObject* count_obj = mono_runtime_invoke(get_count_method, list_obj, nullptr, nullptr);
				//int count = *(int*)mono_object_unbox(count_obj);

				MonoMethod* set_item_method = mono_class_get_method_from_name(list_class, "set_Item", 2);
				MonoMethod* add_item_method = mono_class_get_method_from_name(list_class, "Add", 1);
				MonoMethod* get_item_method = mono_class_get_method_from_name(list_class, "get_Item", 1);
				MonoMethod* clear_list_method = mono_class_get_method_from_name(list_class, "Clear", 0);

				MonoMethodSignature* add_sig = mono_method_signature(add_item_method);

				void* iter = nullptr;
				MonoType* element_type = mono_signature_get_params(add_sig, &iter);

				MonoClass* element_class = mono_class_from_mono_type(element_type);

				/*int index_p = 0;
			   void* index_args[] = { &index_p };


			   MonoObject* underlying_element_obj = mono_runtime_invoke(get_item_method, list_obj, index_args, nullptr);
			   MonoClass* element_class = mono_object_get_class(underlying_element_obj);*/

				mono_runtime_invoke(clear_list_method, list_obj, nullptr, nullptr);
				MonoObject* count_obj = mono_runtime_invoke(get_count_method, list_obj, nullptr, nullptr);
				int count = *(int*)mono_object_unbox(count_obj);
				for (int i = 0; i < field_value.size(); ++i)
				{
					void* args[1];

					if constexpr (std::is_class_v<ElementType>)
					{
						MonoObject* element_obj = mono_object_new(mono_layer.GetDomain(), element_class);

						mono_runtime_object_init(element_obj);

						ElementType& elem = field_value[i];

						//Logger::Info("elem type: {0}", typeid(elem).name());

						PushData(elem, element_obj);

						args[0] = element_obj;
					}
					else
					{
						//value types
						MonoObject* boxed_value = mono_value_box(mono_layer.GetDomain(), element_class, (void*)&field_value[i]);
						if (!boxed_value)
						{
							Logger::Error("failed to box value!");
							continue;
						}

						args[0] = boxed_value;
					}

					MonoObject* exception = nullptr;
					mono_runtime_invoke(add_item_method, list_obj, args, &exception);

					if (exception)
					{
						const char* list_class_name = mono_class_get_name(list_class);
						mono_layer.HandleMonoException(exception, list_class_name);
					}
					//}

				}
			}
			else if constexpr (std::is_class_v<FieldType>)
			{
				// Logger::Info("field name: {0}", mono_field_get_name(mono_field));
				MonoObject* nested_object = mono_field_get_value_object(mono_layer.GetDomain(), mono_field, to_object);

				if (nested_object)
				{
					FieldType field_type = field_value;
					//field_value = GetNativeObject<FieldType>(nested_object);
					PushData(field_type, nested_object);

					mono_field_set_value(to_object, mono_field, nested_object);
				}
				else
				{
					// Logger::Critical("Could not find object for {0}", typeid(FieldType).name());

					MonoType* nested_type = mono_field_get_type(mono_field);
					MonoClass* nested_class = mono_class_from_mono_type(nested_type);
					FieldType field_type = field_value;

					nested_object = mono_object_new(mono_layer.GetDomain(), nested_class);

					PushData(field_type, nested_object);

					mono_field_set_value(to_object, mono_field, nested_object);
				}
			}

			++index;
		});

	return true;
	//mono_layer.LogMonoObjectFields(to_object);
}

template<typename NativeType, typename Func>
void IO::ImmediateEdit(IO::ResourceHandle& handle, Func&& edit)
{
	handle.obj = IO::LoadFile(handle.path.c_str());

	NativeType filled_data = GetNativeObject<NativeType>(handle.obj);

	edit(filled_data);

	bool success = PushData(filled_data, handle.obj);

	if (success) IO::Write(handle.obj, handle.path.c_str());
}