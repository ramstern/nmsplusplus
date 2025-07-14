#pragma once
#include <string>
#include "../header/MonoLayer.h"
#include "../header/traits_ext.h"

#include "../header/Logger.h" 

#include "pfr/tuple_size.hpp"
#include "pfr/functions_for.hpp"

#include <fstream>

class IO
{
public:

	//
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

	[[nodiscard]] static char* ReadBytes(const std::string& path, uint32_t& out_size);

	static void WriteBytes(const std::string& out, const unsigned char* bytes, std::ios::fmtflags flags = 16);

	//either exml or mbin
	[[nodiscard]] static MonoObject* LoadFile(const std::string& path);

	//uses path set in handle
	static void Write(ResourceHandle handle);
	//uses custom path, can export to both mxml and mbin and so needs to have .mxml or .mbin in the path
	static void Write(ResourceHandle handle, const std::string& path);

	//get a native object from a MonoObject
	template <typename NativeType>
	[[nodiscard]] static NativeType GetNativeObject(ResourceHandle handle);

	//pushes data from a native type to an existing handle. returns success.
	template <typename NativeType>
	static bool PushData(NativeType& native_type, ResourceHandle handle);

	//pushes data from a nativetype to a new handle. returns new handle.
	template <typename NativeType>
	static ResourceHandle PushData(NativeType& native_type);

	//immediate edit an exml or mbin in place. Gets native type, pushes data, and writes all at once.
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

#include "generics_impl.hpp"