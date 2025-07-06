#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <unordered_map>


namespace CSharpInterpreter
{
	struct FieldInfo
	{
		std::string name;
		std::string type_name;
		bool is_actual_enum;
		uint32_t size;
	};

	struct TypeInfo
	{
		std::string name;
		std::string namespace_name;
		std::string base_class_name;
		std::string base_class_namespace;
		std::vector<FieldInfo> field_infos;
	};

	struct EnumInfo
	{
		std::string name;
		std::string nesting_class_name;
		std::string namespace_name;
		std::string underlying_type;
		std::vector<std::pair<std::string, int64_t>> entries;
	};

	struct TypeInfosResult 
	{
		std::vector<TypeInfo> type_infos;
		std::vector<EnumInfo> enum_infos;
	};

	TypeInfosResult ExtractTypeInfos();

	void GenerateCppWrappers();

	std::string GenerateEnumCode(std::multimap<std::string, EnumInfo>& multimapped_enums);

	std::unordered_map<std::string, std::string> GenerateTypesStructs(std::multimap<std::string, TypeInfo>& multimapped_types);

	//internal
	std::unordered_set<std::string> RetrieveDependencies(const std::string& self_name, const std::string& from_struct);

	void ExtractTypesFromStruct(const char* find_namespace, const std::string& from_struct, std::vector<std::string>& types);

	bool Visit(const std::string& struct_name, std::unordered_set<std::string>& visited,
		std::unordered_set<std::string>& temp_marks, std::vector<std::string>& sorted_struct_names,
		const std::unordered_map<std::string, std::unordered_set<std::string>>& dependency_graph);

	std::string MapCppType(FieldInfo& field_info, bool const* is_array);

	void ConvertMemberNamespace(FieldInfo& field_info);
	void ConvertNamespace(std::string& _namespace);

	void HandleEnumCornerCases(std::string& _type);
	void VectorTypePassFix(std::string& full_name);

	std::string ParseToCppMember(FieldInfo& info);
};

