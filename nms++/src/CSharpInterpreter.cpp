#include "../header/CSharpInterpreter.h"
#include "../header/engine_io.h"
#include <sstream>
#include <set>
#include "../header/MonoLayer.h"

namespace CSharpInterpreter
{
	TypeInfosResult CSharpInterpreter::ExtractTypeInfos()
	{
		TypeInfosResult result{};

		MonoImage* image = mono_assembly_get_image(mono_layer.GetAssembly());
		const MonoTableInfo* type_definitions_table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		uint32_t type_count = mono_table_info_get_rows(type_definitions_table);

		const MonoTableInfo* constants_table = mono_image_get_table_info(image, MONO_TABLE_CONSTANT);
		int constants_count = mono_table_info_get_rows(constants_table);

		for (unsigned int i = 0; i < type_count; i++)
		{
			uint32_t data_indexer[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(type_definitions_table, i, data_indexer, MONO_TYPEDEF_SIZE);
			uint32_t type_token = mono_metadata_make_token(MONO_TABLE_TYPEDEF, i + 1);

			MonoClass* type_class = mono_class_get(image, type_token);//mono_class_from_name(image, type_info.namespace_name.c_str(), type_info.name.c_str());
			if(!type_class) 
			{
				Logger::Warning("[CSharpInterpreter] Could not find class {0}", mono_metadata_string_heap(image, data_indexer[MONO_TYPEDEF_NAME]));
				continue;
			}

			if(mono_class_is_enum(type_class))
			{
				EnumInfo enum_info;
				enum_info.name = mono_class_get_name(type_class);//mono_class_get_name(type_class);
				
				MonoClass* nesting_class = mono_class_get_nesting_type(type_class);
				if(nesting_class)
				{
					enum_info.nesting_class_name = mono_class_get_name(nesting_class);
					enum_info.namespace_name = mono_class_get_namespace(nesting_class);
				}

				Logger::Info("[CSharpInterpreter] Identified enum {0}::{1}",enum_info.namespace_name, enum_info.name);

				void* iter = nullptr;
				MonoClassField* field;
				std::unordered_map<uint32_t, std::string> field_token_name_map;

				while ((field = mono_class_get_fields(type_class, &iter))) {
					const char* field_name = mono_field_get_name(field);
					// skip special fields like 'value__'
					if (strcmp(field_name, "value__") == 0) continue;

					uint32_t field_token = mono_class_get_field_token(field);

					field_token_name_map[field_token] = field_name;
				}

				for (int i = 0; i < constants_count; ++i) {
					uint32_t cols[MONO_CONSTANT_SIZE];
					mono_metadata_decode_row(constants_table, i, cols, MONO_CONSTANT_SIZE);

					uint32_t type = cols[MONO_CONSTANT_TYPE];        // type of the constant

					switch (type) {
					case MONO_TYPE_I1:
						enum_info.underlying_type = "int8_t";
						break;
					case MONO_TYPE_U1:
						enum_info.underlying_type = "uint8_t";
						break;
					case MONO_TYPE_I2:
						enum_info.underlying_type = "int16_t";
						break;
					case MONO_TYPE_U2:
						enum_info.underlying_type = "uint16_t";
						break;
					case MONO_TYPE_I4:
						enum_info.underlying_type = "int32_t";
						break;
					case MONO_TYPE_U4:
						enum_info.underlying_type = "uint32_t";
						break;
					case MONO_TYPE_I8:
						enum_info.underlying_type = "int64_t";
						break;
					case MONO_TYPE_U8:
						enum_info.underlying_type = "uint64_t";
						break;
					}

					uint32_t parentIndex = cols[MONO_CONSTANT_PARENT]; // Index into the Field or Param table
					uint32_t valueIndex = cols[MONO_CONSTANT_VALUE];  // Index into the Blob heap

					uint32_t parentCodedIndex = cols[MONO_CONSTANT_PARENT];

					// Decode the coded index
					uint32_t tag = parentCodedIndex & 0x3;         // Lower 2 bits for the tag
					uint32_t index = parentCodedIndex >> 2;        // Remaining bits for the index

					uint32_t tableType;
					switch (tag) {
					case 0:
						tableType = MONO_TABLE_FIELD;
						break;
					case 1:
						tableType = MONO_TABLE_PARAM;
						break;
					case 2:
						tableType = MONO_TABLE_PROPERTY;
						break;
					default:
						continue;  // Skip if tag is not recognized
					}

					if (tableType == MONO_TABLE_FIELD) {
						// construct the full token for the field
						uint32_t fieldToken = mono_metadata_make_token(MONO_TABLE_FIELD, index);

						// check if this field token is in our enum fields
						auto it = field_token_name_map.find(fieldToken);
						if (it != field_token_name_map.end()) {
							const std::string& fieldName = it->second;

							const char* blob = nullptr;
							uint32_t blobLength = 0;
							blob = mono_metadata_blob_heap(image, valueIndex);
							blobLength = mono_metadata_decode_blob_size(blob, &blob);

							// read based on type
							int64_t enumValue = 0;
							switch (type) {
							case MONO_TYPE_I1:
								enumValue = *(int8_t*)blob;
								break;
							case MONO_TYPE_U1:
								enumValue = *(uint8_t*)blob;
								break;
							case MONO_TYPE_I2:
								enumValue = *(int16_t*)blob;
								break;
							case MONO_TYPE_U2:
								enumValue = *(uint16_t*)blob;
								break;
							case MONO_TYPE_I4:
								enumValue = *(int32_t*)blob;
								break;
							case MONO_TYPE_U4:
								enumValue = *(uint32_t*)blob;
								break;
							case MONO_TYPE_I8:
								enumValue = *(int64_t*)blob;
								break;
							case MONO_TYPE_U8:
								enumValue = *(uint64_t*)blob;
								break;
							default:
								Logger::Error("[CSharpInterpreter] Unhandled enum underlying type: {0}", (int)type);
								continue;
							}

							enum_info.entries.emplace_back(fieldName, enumValue);
						}
					}
				}

				result.enum_infos.push_back(enum_info);
			}
			else 
			{
				TypeInfo type_info;

				type_info.name = mono_metadata_string_heap(image, data_indexer[MONO_TYPEDEF_NAME]);
				type_info.namespace_name = mono_metadata_string_heap(image, data_indexer[MONO_TYPEDEF_NAMESPACE]);

				MonoClass* base_class = mono_class_get_parent(type_class);
				if (base_class) //most likely to be NMSTemplate
				{
					type_info.base_class_name = mono_class_get_name(base_class);
					type_info.base_class_namespace = mono_class_get_namespace(base_class);
				}
				else
				{
					Logger::Warning("[CSharpInterpreter] class {0}::{1} does not have a base class, may need handling...", type_info.namespace_name, type_info.name);
				}

				//field processing
				void* iter = nullptr;
				MonoClassField* field;
				while (field = mono_class_get_fields(type_class, &iter))
				{
					FieldInfo field_info;
					field_info.name = mono_field_get_name(field);

					MonoType* type = mono_field_get_type(field);

					field_info.is_actual_enum = false;
					if(mono_class_is_enum(mono_class_from_mono_type(mono_field_get_type(field))))
					{
						field_info.is_actual_enum = true;
					}
					
					field_info.type_name = mono_type_get_name(type);

					MonoCustomAttrInfo* attr_info = mono_custom_attrs_from_field(type_class, field);
					if(attr_info)
					{
						for (int i = 0; i < attr_info->num_attrs; i++)
						{
							MonoCustomAttrEntry entry = attr_info->attrs[i];
							MonoClass* attr_class = mono_method_get_class(entry.ctor);
							const char* attr_class_name = mono_class_get_name(attr_class);

							if(strcmp(attr_class_name, "NMSAttribute") == 0)
							{
								MonoObject* attr_instance = mono_custom_attrs_get_attr(attr_info, attr_class);
								if(attr_instance)
								{
									MonoProperty* size_property = mono_class_get_property_from_name(attr_class, "Size");
									if(size_property)
									{
										//the field is an array.
										MonoObject* size_value = mono_property_get_value(size_property, attr_instance, nullptr, nullptr);
										uint32_t size_value_unboxed = *reinterpret_cast<uint32_t*>(mono_object_unbox(size_value));
										if(size_value_unboxed) Logger::Info("[CSharpInterpreter] Found NMSAttribute with size, decoded size of {0}", size_value_unboxed);
										field_info.size = size_value_unboxed;
									}
								}
							}
						}
					}

					type_info.field_infos.push_back(field_info);
				}

				//method processing?
				//...

				result.type_infos.push_back(type_info);
			}
		}

		return result;
	}

	void GenerateCppWrappers()
	{
		TypeInfosResult infos_result = ExtractTypeInfos();

		std::multimap<std::string, TypeInfo> multimapped_types;
		for (TypeInfo& type_info : infos_result.type_infos)
		{
			if(type_info.name == "HashMap`1" ) continue;

			//remove libmbin from namespace names
			size_t libmbin_namespace_offset = type_info.namespace_name.find("libMBIN.");
			if(libmbin_namespace_offset != std::string::npos) 
			{
				type_info.namespace_name.replace(libmbin_namespace_offset, 8, "");
			}

			multimapped_types.emplace(type_info.namespace_name, type_info);
		}

		std::unordered_map<std::string, std::string> name_namespace_map;
		for (const TypeInfo& type_info : infos_result.type_infos)
		{
			if (type_info.name == "HashMap`1") continue;

			name_namespace_map.emplace(type_info.name, type_info.namespace_name);
		}

		std::unordered_map<std::string, std::string> write_datas = GenerateTypesStructs(multimapped_types);

		std::unordered_multimap<std::string, EnumInfo> multimapped_enums;
		for (const EnumInfo& enum_info : infos_result.enum_infos)
		{
			multimapped_enums.emplace(enum_info.nesting_class_name, enum_info);
		}

		for (auto iter = write_datas.begin(); iter != write_datas.end(); iter++)
		{
			//get possible enum, and insert it into the struct
			auto range = multimapped_enums.equal_range(iter->first);
			if (range.first != multimapped_enums.end())
			{
				size_t open_bracket_pos = write_datas[iter->first].find_first_of('{');
				for (auto& it = range.first; it != range.second; ++it)
				{
					std::stringstream enum_stringstream;
					enum_stringstream << "\n\tenum class " << it->second.name << " : " << it->second.underlying_type << "\n\t{\n";
					for (const std::pair<std::string, uint64_t>& field : it->second.entries)
					{
						enum_stringstream << "	\t" << field.first << " = " << field.second << ",\n";
					}
					enum_stringstream << "\t};\n";

					write_datas[iter->first].insert(open_bracket_pos + 1, enum_stringstream.str().data());
				}
			}

			////NMSTemplate libMBIN namespace fix
			//size_t nmstemplate_offset = iter->second.find("NMSTemplate");
			//size_t linkable_nmstemplate_offset = iter->second.find("LinkableNMSTemplate");
			//if(nmstemplate_offset != std::string::npos && linkable_nmstemplate_offset == std::string::npos)
			//{
			//	iter->second.insert(nmstemplate_offset, "libMBIN::");
			//}
		}
		

		//detect dependencies
		std::unordered_map<std::string, std::unordered_set<std::string>> dependency_graph;
		for (auto& [name, _struct] : write_datas)
		{
			std::unordered_set<std::string> dependencies = RetrieveDependencies(name, _struct);

			for (const auto& dependency : dependencies)
			{
				Logger::Info("[CSharpInterpreter] Detected dependency for {0}: {1}", name, dependency);
			}

			dependency_graph.emplace(name, dependencies);
		}

		std::vector<std::string> sorted_struct_names; 
		std::unordered_set<std::string> visited; 
		std::unordered_set<std::string> temp_marks;
		// Perform topological sort
		for (auto& [struct_name, _] : dependency_graph) 
		{
			/*if (struct_name == "GcInventoryCostData")
			{
				Logger::Info("");
			}*/

			if (visited.find(struct_name) == visited.end()) 
			{
				if (!Visit(struct_name, visited, temp_marks, sorted_struct_names, dependency_graph)) 
				{
					//cyclic dependency, check it
					Logger::Error("Cyclic dependency detected involving {0}", struct_name);
					//return;
				}
			}
		}

		std::stringstream ss;
		std::string last_namespace = "Boo";
		ss << "//Generated by NMSgen CSharpInterpreter\n\n";

		ss << "#pragma once\n\n";

		ss << "#include <string>\n";
		ss << "#include <vector>\n\n";

		//ss << "namespace libMBIN\n{\n";
		ss << "struct NMSTemplate {};\n\n";
		//ss << "} // namespace libMBIN\n\n";

		for (auto iter = sorted_struct_names.begin(); iter != sorted_struct_names.end(); iter++)
		{
			bool write_namespace = false;
			std::string associated_namespace = name_namespace_map[*iter];
			ConvertNamespace(associated_namespace);
			if (associated_namespace != last_namespace) write_namespace = true;

			bool has_namespace = associated_namespace != "";

			//ignore
			if (!has_namespace) continue;

			//namespace begin
			if (write_namespace)
			{
				ss << "namespace " << associated_namespace << "\n{\n\n";
				last_namespace = associated_namespace;
			}

			std::unordered_set<std::string>& type_dependencies = dependency_graph[*iter];
			for (auto dependency_iter = type_dependencies.begin(); dependency_iter != type_dependencies.end(); dependency_iter++)
			{
				ss << "//Dependency: " << *dependency_iter << "\n"; 
			}

			ss << write_datas[*iter];

			auto iter_peek = std::next(iter);

			//namespace end
			if (iter_peek != sorted_struct_names.end() )
			{
				std::string next_namespace = name_namespace_map[*iter_peek];
				ConvertNamespace(next_namespace);
				if(next_namespace != last_namespace)
				{
					ss << "\n} //namespace " << associated_namespace << "\n\n";
				}
			}
			else 
			{
				ss << "\n} //namespace " << associated_namespace << "\n";
			}
		}

		IO::WriteBytes("generated/GeneratedCppTypes.h", reinterpret_cast<const unsigned char*>(ss.str().data()));

		

		/*std::string write_data = GenerateEnumCode(multimapped_enums);
		
		IO::WriteBytes("generated/GeneratedCppEnums.h", reinterpret_cast<const unsigned char*>(write_data.data()));*/
	}

	std::string GenerateEnumCode(std::multimap<std::string, EnumInfo>& multimapped_enums)
	{
		std::string last_namespace = "Boo";
		std::string last_struct = "Woo";
		std::stringstream ss2;
		
		ss2 << "//Generated by NMSgen CSharpInterpreter\n\n";

		ss2 << "#pragma once\n\n";

		ss2 << "#include <stdint.h>\n\n";

		for (auto iter = multimapped_enums.begin(); iter != multimapped_enums.end(); iter++)
		{
			bool write_namespace = false;
			bool write_struct = false;
			if (iter->first != last_namespace) write_namespace = true;
			if (iter->second.nesting_class_name != last_struct) write_struct = true;

			const EnumInfo& enum_info = iter->second;
			bool has_namespace = !enum_info.namespace_name.empty();

			//namespace begin
			if (write_namespace)
			{
				std::string _namespace = enum_info.namespace_name;
				ConvertNamespace(_namespace);
				ss2 << "namespace " << _namespace << "\n{\n";
				last_namespace = enum_info.namespace_name;
			}

			//declaration begin
			if (write_struct)
			{
				ss2 << "struct " << enum_info.nesting_class_name << "_holder" << " \n{\n";
				last_struct = enum_info.nesting_class_name;
			}

			ss2 << "enum class " << enum_info.name << " : " << enum_info.underlying_type << "\n{\n";
			for (const std::pair<std::string, uint64_t>& field : enum_info.entries)
			{
				ss2 << "	" << field.first << " = " << field.second << ",\n";
			}
			ss2 << "};\n";

			auto iter_peek = std::next(iter);

			//declaration end
			if (iter_peek != multimapped_enums.end() && iter_peek->second.nesting_class_name != last_struct) ss2 << "};\n\n";

			//namespace end
			if (iter_peek != multimapped_enums.end() && iter_peek->first != last_namespace)
			{
				ss2 << "\n} //namespace " << enum_info.namespace_name << "\n\n";
			}
			else if (iter_peek == multimapped_enums.end())
			{
				ss2 << "};\n";
				ss2 << "\n} //namespace " << enum_info.namespace_name << "\n\n";
			}
		}

		return ss2.str();
	}

	std::unordered_map<std::string, std::string> GenerateTypesStructs(std::multimap<std::string, TypeInfo>& multimapped_types)
	{
		std::unordered_map<std::string, std::string> write_datas;

		for (auto iter = multimapped_types.begin(); iter != multimapped_types.end(); iter++)
		{
			std::stringstream ss;

			TypeInfo& type_info = iter->second;
			bool has_namespace = !type_info.namespace_name.empty();
			bool has_base_class = !type_info.base_class_name.empty();

			//ignore
			if (!has_namespace || type_info.base_class_name == "Object" || type_info.namespace_name == "libMBIN") continue;

			//declaration begin
			/*if (has_base_class)
			{
				ss << "struct " << type_info.name << " : public " << type_info.base_class_namespace << "::" << type_info.base_class_name << "\n{\n";
			}
			else*/
			{
				ss << "struct " << type_info.name << "\n{\n";
			}

			for (FieldInfo& field : type_info.field_infos)
			{
				std::string cpp_member = ParseToCppMember(field);
				ss << "	" << cpp_member << ";\n";
			}

			//declaration end
			ss << "};\n\n";

			write_datas.emplace(type_info.name, ss.str());
		}

		return write_datas;
	}

	//https://stackoverflow.com/a/46711735
	constexpr uint32_t hash(std::string_view data) noexcept {
		uint32_t hash = 5381;

		for (const char* c = data.data(); c < data.data() + data.size(); ++c)
			hash = ((hash << 5) + hash) + (unsigned char)*c;

		return hash;
	}

	std::set<std::string> surrogates;
	std::unordered_set<std::string> RetrieveDependencies(const std::string& self_name, const std::string& from_struct)
	{
		std::vector<std::string> dependencies_vec;
		
		ExtractTypesFromStruct("NMS_GameComponents::", from_struct, dependencies_vec);
		ExtractTypesFromStruct("NMS_Toolkit::", from_struct, dependencies_vec);
		ExtractTypesFromStruct("NMS_Globals::", from_struct, dependencies_vec);
		ExtractTypesFromStruct("NMS::", from_struct, dependencies_vec);

		if(self_name == "GcModelExplosionRule")
		{
			printf("sym");
		}

		//> cancel (vector types)
		for (size_t i = 0; i < dependencies_vec.size(); i++)
		{
			size_t p = dependencies_vec[i].find(">");
			if (p != std::string::npos)
			{
				dependencies_vec[i].replace(p, strlen(">"), "");
			}
		}

		size_t offset = 0;
		size_t enum_p = from_struct.find("enum class", offset);
		std::vector<std::string> enum_names;
		while(enum_p != std::string::npos)
		{
			size_t start_pos = enum_p + 11;
			size_t end_pos = from_struct.find_first_of(' ', start_pos);
			enum_names.push_back(from_struct.substr(start_pos, end_pos - start_pos));

			offset = end_pos;
			enum_p = from_struct.find("enum class", offset);
		}

		//self-declared enum cancel and self cancel
		auto iter = dependencies_vec.begin();
		while(iter != dependencies_vec.end())
		{
			bool has_enum = false;
			for (auto& s : enum_names)
			{
				if(iter->find(s) != std::string::npos) 
				{
					has_enum = true;
				}
			}

			bool same = *iter == self_name;
			if(same || has_enum) 
			{
				iter = dependencies_vec.erase(iter);
			}
			else iter++;
		}

		std::unordered_set<std::string> dependencies_set;

		for (size_t i = 0; i < dependencies_vec.size(); i++)
		{
			dependencies_set.insert(dependencies_vec[i]);
		}

		return dependencies_set;
	}

	void ExtractTypesFromStruct(const char* find_namespace, const std::string& from_struct, std::vector<std::string>& types)
	{
		size_t pos = from_struct.find(find_namespace);
		while (pos != std::string::npos)
		{
			size_t end_pos = from_struct.find_first_of(" ", pos);
			size_t str_len = strlen(find_namespace);
			size_t length = end_pos - (pos + str_len);
			types.push_back(from_struct.substr(pos + str_len, length));

			pos = from_struct.find(find_namespace, pos + 1);
		}
	}

	bool Visit(const std::string& struct_name, std::unordered_set<std::string>& visited, std::unordered_set<std::string>& temp_marks, std::vector<std::string>& sorted_struct_names, const std::unordered_map<std::string, std::unordered_set<std::string>>& dependency_graph)
	{
		if (temp_marks.find(struct_name) != temp_marks.end()) {
			// Detected a cycle
			Logger::Error("Cyclic dependency detected involving struct: {0}", struct_name);
			return false;
		}
		if (visited.find(struct_name) == visited.end()) {
			temp_marks.insert(struct_name);
			for (const auto& dep : dependency_graph.at(struct_name)) {
				if (!Visit(dep, visited, temp_marks, sorted_struct_names, dependency_graph)) {
					return false;
				}
			}
			temp_marks.erase(struct_name);
			visited.insert(struct_name);
			sorted_struct_names.push_back(struct_name);
		}
		return true;
	}

	std::string MapCppType(FieldInfo& field_info, bool* is_array)
	{
		//generic list + hashmap transformer
		constexpr const char* generic_list = "System.Collections.Generic.List";
		constexpr const char* hashmap = "libMBIN.NMS.HashMap";

		size_t pos = field_info.type_name.find(generic_list);
		if(pos != std::string::npos)
		{
			field_info.type_name.replace(pos, strlen(generic_list), "std::vector");
			ConvertMemberNamespace(field_info);
			VectorTypePassFix(field_info.type_name);
			return field_info.type_name;
		}

		pos = field_info.type_name.find(hashmap);
		if (pos != std::string::npos)
		{
			field_info.type_name.replace(pos, strlen(hashmap), "std::vector");
			ConvertMemberNamespace(field_info);
			VectorTypePassFix(field_info.type_name);
			return field_info.type_name;
		}

		//remove array brackets and set flags
		pos = field_info.type_name.find("[]");
		if(pos != std::string::npos)
		{
			field_info.type_name.replace(pos, strlen("[]"), "");
			*is_array = true;
		}

		uint32_t switcher = hash(field_info.type_name);
		switch(switcher)
		{
		case hash("System.Boolean"):
			return "bool";
		case hash("System.Single"):
			return "float";
		case hash("System.String"):
			return std::string("//max length: ") + std::to_string(field_info.size) + "\n\tstd::string";
		case hash("System.Byte"):
			return "uint8_t";
		case hash("System.SByte"):
			return "int8_t";
		case hash("System.Int16"):
			return "int16_t";
		case hash("System.UInt16"):
			return "uint16_t";
		case hash("System.Int32"):
			return "int32_t";
		case hash("System.Int64"):
			return "int64_t";
		case hash("System.UInt32"):
			return "uint32_t";
		case hash("System.UInt64"):
			return "uint64_t";
		case hash("System.Double"):
			return "double";
		default:
			if(surrogates.find(field_info.type_name) == surrogates.end())
			{
				surrogates.emplace(field_info.type_name);
				Logger::Info("[CSharpInterpreter] Possibly implement surrogate for {0}", field_info.type_name);
			}
			
			ConvertMemberNamespace(field_info);
			return field_info.type_name;
		}
	}
	void ConvertMemberNamespace(FieldInfo& field_info)
	{
		size_t libmbin_namespace_offset = field_info.type_name.find("libMBIN.");
		if(libmbin_namespace_offset != std::string::npos)
		{
			field_info.type_name.replace(libmbin_namespace_offset, 8, "");
		}

		ConvertNamespace(field_info.type_name);

		size_t pos = field_info.type_name.find_last_of("_");
		if(pos != std::string::npos)
		{
			field_info.type_name.replace(pos, strlen("_"), "::");
		}
		

		if(field_info.is_actual_enum)
		{
			size_t pos = field_info.type_name.find_last_of("_");
			if (pos != std::string::npos)
			{
				field_info.type_name.replace(pos, strlen("_"), "::");
			}
			HandleEnumCornerCases(field_info.type_name);

			/*size_t pos2 = field_info.type_name.find_last_of("::");
			if(pos2 != std::string::npos)
			{
				field_info.type_name.insert(pos2-1, "_holder");
			}*/
		}
	}
	void ConvertNamespace(std::string& _namespace)
	{
		size_t pos = _namespace.find(".");
		if (pos != std::string::npos)
		{
			_namespace.replace(pos, strlen("."), "_");
			ConvertNamespace(_namespace);
		}
	}
	//this here is pretty dumb
	void HandleEnumCornerCases(std::string& _type)
	{
		if (_type == "NMS_GameComponents_GcAccessibleOverride_Layout::AccessibleOverride::LayoutEnum")
		{
			_type = "NMS_GameComponents::GcAccessibleOverride_Layout::AccessibleOverride_LayoutEnum";
		}
		else if (_type == "NMS_GameComponents_GcAccessibleOverride_Text::AccessibleOverride::TextEnum")
		{
			_type = "NMS_GameComponents::GcAccessibleOverride_Text::AccessibleOverride_TextEnum";
		}
		else if(_type == "NMS_GameComponents_GcVROverride_Layout::VROverride::LayoutEnum")
		{
			_type = "NMS_GameComponents::GcVROverride_Layout::VROverride_LayoutEnum";
		}
		else if(_type == "NMS_GameComponents_GcVROverride_Text::VROverride::TextEnum")
		{
			_type = "NMS_GameComponents::GcVROverride_Text::VROverride_TextEnum";
		}
	}
	
	void VectorTypePassFix(std::string& full_name)
	{
		size_t p = full_name.find("System::Int32");
		if(p != std::string::npos)
		{
			full_name.replace(p, strlen("System::Int32"), "int32_t");
			return;
		}

		p = full_name.find("System::UInt32");
		if(p != std::string::npos)
		{
			full_name.replace(p, strlen("System::UInt32"), "uint32_t");
			return;
		}

		p = full_name.find("System::Single");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("System::Single"), "float");
			return;
		}

		p = full_name.find("System::Byte");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("System::Byte"), "int8_t");
			return;
		}

		p = full_name.find("System::UInt16");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("System::UInt16"), "uint16_t");
			return;
		}

		p = full_name.find("System::Int16");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("System::Int16"), "int16_t");
			return;
		}

		p = full_name.find("System::UInt64");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("System::UInt64"), "uint64_t");
			return;
		}

		p = full_name.find("System::Int64");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("System::Int64"), "int64_t");
			return;
		}

		p = full_name.find("NMS_GameComponents_GcAccessibleOverride::Layout");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("NMS_GameComponents_GcAccessibleOverride::Layout"), "NMS_GameComponents::GcAccessibleOverride_Layout");
			return;
		}

		p = full_name.find("NMS_GameComponents_GcVROverride::Layout");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("NMS_GameComponents_GcVROverride::Layout"), "NMS_GameComponents::GcVROverride_Layout");
			return;
		}

		p = full_name.find("NMS_GameComponents_GcAccessibleOverride::Text");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("NMS_GameComponents_GcAccessibleOverride::Text"), "NMS_GameComponents::GcAccessibleOverride_Text");
			return;
		}

		p = full_name.find("NMS_GameComponents_GcVROverride::Text");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("NMS_GameComponents_GcVROverride::Text"), "NMS_GameComponents::GcVROverride_Text");
			return;
		}

		p = full_name.find("NMS_Toolkit_TkMaterialUniform::Float");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("NMS_Toolkit_TkMaterialUniform::Float"), "NMS_Toolkit::TkMaterialUniform_Float");
			return;
		}

		p = full_name.find("NMS_Toolkit_TkMaterialUniform::UInt");
		if (p != std::string::npos)
		{
			full_name.replace(p, strlen("NMS_Toolkit_TkMaterialUniform::UInt"), "NMS_Toolkit::TkMaterialUniform_UInt");
			return;
		}
	}
	std::string ParseToCppMember(FieldInfo& info)
	{
		bool is_array = false;
		std::string cpp_type = MapCppType(info, &is_array);

		if(is_array && info.size > 0)
		{
			return cpp_type + " " + info.name + "[" + std::to_string(info.size) + "]";
		}
		else return cpp_type + " " + info.name;
	}
}


