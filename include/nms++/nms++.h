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
	private:
		friend class IO;

		std::string path;
		MonoObject* obj = nullptr;
		ResourceHandle(MonoObject* obj);
	};

	static void Initialize();

	static char* ReadBytes(const std::string& path, uint32_t& out_size);

	static void WriteBytes(const std::string& out, const unsigned char* bytes, std::ios::fmtflags flags = 16);

	//either mxml or mbin
	static MonoObject* LoadFile(const std::string& path);

	//uses path set in handle
	static void Write(ResourceHandle handle);
	//uses custom path, can export to both mxml and mbin and so needs to have .mxml or .mbin in the path
	static void Write(ResourceHandle handle, const std::string& path);
	
	//get a native object from a ResourceHandle
	template <typename NativeType>
	static NativeType GetNativeObject(ResourceHandle handle);

	//pushes data from a native type to a ResourceHandle. returns success.
	template <typename NativeType>
	static bool PushData(NativeType& native_type, ResourceHandle handle);

	//immediate edit an exml or mbin in place. Gets native type, pushes data, and writes all at once.
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