#pragma once
#include <string>
#include "mono/metadata/object-forward.h"

class IO
{
public:
	struct ResourceHandle
	{
		std::string path;
		MonoObject* obj = nullptr;
	};

	static void Initialize();

	static char* ReadBytes(const std::string& path, uint32_t& out_size);

	static void WriteBytes(const std::string& out, const unsigned char* bytes, int flags = 16);

	//either exml or mbin
	static MonoObject* LoadFile(const std::string& path);

	//has to have .exml or .mbin in the path
	static void Write(MonoObject* to_write, const std::string& path);
	
	//get a native object from a MonoObject
	template <typename NativeType>
	static NativeType GetNativeObject(MonoObject* from_object);

	//pushes data from a native type to a monoobject. returns success.
	template <typename NativeType>
	static bool PushData(NativeType& native_type, MonoObject* to_object);

	//immediate edit an exml or mbin in place.
	template <typename NativeType, typename Func>
	static void ImmediateEdit(struct IO::ResourceHandle& handle, Func&& edit);

private:
	static MonoClass* nms_template;
	static MonoClass* file_io;
	static MonoObject* file_io_obj;

	static void WriteExml(MonoObject* to_write, const std::string& path);
	static void WriteMbin(MonoObject* to_write, const std::string& path);
};
#include "details/generics_impl.hpp"


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

	//Generate native c++ types from libmbin.dll. outputs to (workingdir)/generated/
	void GenerateCppWrappers();

	std::string GenerateEnumCode(std::multimap<std::string, EnumInfo>& multimapped_enums);

	//internal
	std::unordered_map<std::string, std::string> GenerateTypesStructs(std::multimap<std::string, TypeInfo>& multimapped_types);

	//internal
	std::unordered_set<std::string> RetrieveDependencies(const std::string& self_name, const std::string& from_struct);

	//internal
	void ExtractTypesFromStruct(const char* find_namespace, const std::string& from_struct, std::vector<std::string>& types);

	//internal
	bool Visit(const std::string& struct_name, std::unordered_set<std::string>& visited,
		std::unordered_set<std::string>& temp_marks, std::vector<std::string>& sorted_struct_names,
		const std::unordered_map<std::string, std::unordered_set<std::string>>& dependency_graph);

	//internal
	std::string MapCppType(FieldInfo& field_info, bool const* is_array);
	//internal
	void ConvertMemberNamespace(FieldInfo& field_info);
	//internal
	void ConvertNamespace(std::string& _namespace);
	//internal
	void HandleEnumCornerCases(std::string& _type);
	//internal
	void VectorTypePassFix(std::string& full_name);
	//internal
	std::string ParseToCppMember(FieldInfo& info);
};