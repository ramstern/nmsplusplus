#pragma once
//nms++ interface header

#include <string>
#include "mono/metadata/object-forward.h"
#include <iostream>
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

	static void WriteBytes(const std::string& out, const unsigned char* bytes, std::ios::fmtflags flags = 16);

	//either exml or mbin
	static MonoObject* LoadFile(const std::string& path);

	//has to have .mxml or .mbin in the path
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