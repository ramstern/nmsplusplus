#include "../header/NMSapp.h"
#include "../header/GeneratedCppTypes.h"
#include "nms++/nms++.h"

NMSapp::NMSapp()
{
	IO::Initialize();

	using namespace NMS_GameComponents;
	using namespace NMS;
	
	//CSharpInterpreter::GenerateCppWrappers();

	IO::ResourceHandle biome_handle = {"mbin/BARRENBIGPROPSBIOME.MBIN"};

	GcBiomeData barren_big_props_biome = IO::TryGetNativeObject<GcBiomeData>(biome_handle);

	GcBiomeData new_biome_data = barren_big_props_biome;
	new_biome_data.FuelMultiplier = 10.f;

	IO::ResourceHandle new_biome_handle = IO::PushData(new_biome_data);
	IO::Write(new_biome_handle, "mbin/NEWBIOME.MBIN");

	IO::PAKDirectoryContents("exml", "test.pak");
}

NMSapp::~NMSapp()
{

}

void NMSapp::Tick()
{
	while(true)
	{

	}
}
