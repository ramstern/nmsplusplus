#include "../header/engine_io.h"
#include <filesystem>

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

//either .mbin or .exml files
MonoObject* IO::LoadFile(const std::string& path)
{
	return mono_layer.RuntimeInvoke(file_io, file_io_obj, "LoadFile", path.c_str());
}

void IO::Write(MonoObject* to_write, const std::string& path)
{
	if(!to_write) 
	{
		Logger::Error("To be written object was nullptr!");
		return;
	}

	size_t p = path.find_last_of(".");
	if(p == std::string::npos) 
	{
		Logger::Error("No file extension provided!");
		return;
	}
	std::string extension = path.substr(p);

	if(extension == ".EXML")
	{
		WriteExml(to_write, path);
	}
	else if(extension == ".MBIN")
	{
		WriteMbin(to_write, path);
	}
	else 
	{
		Logger::Error("[IO] Incorrect file extension: {0}", extension.c_str());
	}
}

void IO::WriteExml(MonoObject* to_write, const std::string& path)
{
	mono_layer.RuntimeInvoke(nms_template, to_write, "WriteToExml", path.c_str());
}

void IO::WriteMbin(MonoObject* to_write, const std::string& path)
{
	mono_layer.RuntimeInvoke(nms_template, to_write, "WriteToMbin", path.c_str());
}
