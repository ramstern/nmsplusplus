#include "../header/engine_io.h"
#include <filesystem>
#include <thread>

MonoClass* IO::nms_template = nullptr;
MonoClass* IO::file_io = nullptr;
MonoObject* IO::file_io_obj = nullptr;

void IO::Initialize()
{
	nms_template = mono_layer.GetClass("libMBIN", "NMSTemplate");
	file_io = mono_layer.GetClass("libMBIN", "FileIO");
	file_io_obj = mono_layer.CreateInstanceOfClass(IO::file_io);
}

char* IO::ReadBytes(const std::string& path, uint32_t& out_size)
{
	std::ifstream read_stream(path, std::ios::binary | std::ios::ate);

	if (!read_stream || read_stream.bad())
	{		
		Logger::Error("[IO] Failed to open file {0}", path);
		return nullptr;
	}

	std::streampos size = read_stream.tellg();
	read_stream.seekg(0, std::ios::beg);

	if (size == 0)
	{
		Logger::Error("[IO] File {0} is empty!", path.c_str());
		return nullptr;
	}

	char* buffer = new char[size];
	read_stream.read(buffer, size);
	read_stream.close();

	out_size = static_cast<uint32_t>(size);
	return buffer;
}

void IO::WriteBytes(const std::string& out, const unsigned char* bytes, std::ios::fmtflags flags)
{
	if(!std::filesystem::exists(out))
	{
		std::filesystem::path current_path = out;
		std::filesystem::create_directory(current_path.parent_path());
	}

	std::ofstream write_stream(out, flags);

	if(!write_stream || write_stream.bad())
	{
		Logger::Error("[IO] Failed to write file {0}", out);
		return;
	}

	write_stream << bytes;

	write_stream.close();
}

//either .mbin or .mxml files
MonoObject* IO::LoadFile(const std::string& path)
{
	size_t dot_offset = path.find_last_of('.');
	if(dot_offset == std::string::npos) 
	{
		Logger::Error("[IO] Invalid handle path: does not contain extension");
		return nullptr;
	}
	std::string extension = path.substr(dot_offset);

	if(extension != ".MXML" && extension != ".MBIN")
	{
		Logger::Error("[IO] Invalid handle path: must have mbin or mxml extension");
		return nullptr;
	}

	return mono_layer.RuntimeInvoke(file_io, file_io_obj, "LoadFile", path.c_str());
}

void IO::Write(ResourceHandle handle)
{
	IO::Write(handle, handle.path);
}

void IO::Write(ResourceHandle to_write, const std::string& path)
{
	if(!to_write.obj) 
	{
		Logger::Error("[IO] To be written object was nullptr!");
		return;
	}

	size_t p = path.find_last_of(".");
	if(p == std::string::npos) 
	{
		Logger::Error("[IO] No file extension provided!");
		return;
	}
	std::string extension = path.substr(p);

	if(extension == ".MXML")
	{
		WriteMxml(to_write.obj, path);
	}
	else if(extension == ".MBIN")
	{
		WriteMbin(to_write.obj, path);
	}
	else 
	{
		Logger::Error("[IO] Incorrect file extension: {0}", extension.c_str());
	}
}

void IO::WriteMxml(MonoObject* to_write, const std::string& path)
{
	mono_layer.RuntimeInvoke(nms_template, to_write, "WriteToExml", path.c_str());
}

void IO::WriteMbin(MonoObject* to_write, const std::string& path)
{
	mono_layer.RuntimeInvoke(nms_template, to_write, "WriteToMbin", path.c_str());
}

void IO::PAKDirectoryContents(const std::string& directory, const std::string& pak_file_name)
{
	static constexpr std::chrono::milliseconds poll_interval = std::chrono::milliseconds(100);

	std::string unpack_command = ".\\lib\\psarc\\PSArcTool.exe " + std::string("\"") + directory + std::string("\"");
	IO::ExecCommand(unpack_command.c_str());

	while(!std::filesystem::exists(std::filesystem::path("psarc.pak")))
	{
		std::this_thread::sleep_for(poll_interval);
	}

	std::filesystem::rename("psarc.pak", pak_file_name);
}

void IO::ExecCommand(const char* command)
{
	std::system(command);
}

IO::ResourceHandle::ResourceHandle(const std::string& _path)
{
	path = _path;
	obj = IO::LoadFile(path);
}

IO::ResourceHandle::ResourceHandle(MonoObject* _obj)
{
	obj = _obj;
}