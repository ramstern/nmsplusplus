#pragma once
//nms++ interface header

#include <string>
#include "mono/metadata/object-forward.h"
#include "pfr/tuple_size.hpp"
#include "pfr/functions_for.hpp"
#include <iostream>
class IO
{
public:
	struct ResourceHandle
	{
	public:
		ResourceHandle(const std::string& path);

		std::string path;
		MonoObject* obj = nullptr;
	private:
		friend class IO;
		ResourceHandle(MonoObject* obj);
	};

	static void Initialize();

	static char* ReadBytes(const std::string& path, uint32_t& out_size);

	static void WriteBytes(const std::string& out, const unsigned char* bytes, std::ios::fmtflags flags = 16);

	//either mxml or mbin
	static MonoObject* LoadFile(const std::string& path);

	//has to have .mxml or .mbin in the path
	static void Write(ResourceHandle to_write, const std::string& path);
	
	//get a native object from a ResourceHandle
	template <typename NativeType>
	static NativeType GetNativeObject(ResourceHandle handle);

	//pushes data from a native type to a ResourceHandle. returns success.
	template <typename NativeType>
	static bool PushData(NativeType& native_type, ResourceHandle handle);

	//immediate edit an exml or mbin in place.
	template <typename NativeType, typename Func>
	static void ImmediateEdit(struct IO::ResourceHandle& handle, Func&& edit);

	//PAK directory contents
	static void PAKDirectoryContents(const std::string& directory, const std::string& pak_file_name);
};

//compiler needs to see some of the definitions
#include "details/generics_impl.hpp"


namespace CSharpInterpreter
{
	//Generate native c++ types from libmbin.dll. outputs to (workingdir)/generated/
	void GenerateCppWrappers();
};