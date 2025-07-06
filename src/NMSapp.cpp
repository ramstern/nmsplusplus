#include "../header/NMSapp.h"
#include "../header/GeneratedCppTypes.h"
#include "NMSgen/NMSgen.h"

NMSapp::NMSapp()
{
	IO::Initialize();

	using namespace NMS_GameComponents;
	using namespace NMS;
	
	CSharpInterpreter::GenerateCppWrappers();

	IO::ResourceHandle barren_big_props_biome = {"mbin/BARRENBIGPROPSBIOME.MBIN"};

	IO::ImmediateEdit<GcBiomeData>(barren_big_props_biome, [](GcBiomeData& biome_data)
		{
			GcScreenFilterOption option{};
			option.Filter = GcScreenFilters();
			option.Weight = -1.f;

			biome_data.FilterOptions.push_back(option);
			biome_data.FloraLifeLocID.Value = "testogkdkdkdkdkdkdkdkdkdkdkdkdd";
			biome_data.FuelMultiplier = 2.f;

			biome_data.WeatherOptions[0].WeatherWeightings[0] = 1.6f;

			biome_data.ExternalObjectLists[0].TileType.TileType = GcTerrainTileType::TileTypeEnum::Substance;
		});
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
