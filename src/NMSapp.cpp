#include "../header/NMSapp.h"
#include "../header/GeneratedCppTypes.h"
#include "nms++/nms++.h"
#include <filesystem>
#include <unordered_set>
#include <array>

NMSapp::NMSapp()
{
	IO::Initialize();

	using namespace NMS_GameComponents;
	using namespace NMS;
	
	//return;

	using DirectoryIterator = std::filesystem::directory_iterator;

	//gather data about base nms object lists (GcExternalObjectList)
	std::vector<GcObjectSpawnData> distant_objects;
	std::vector<GcObjectSpawnData> landmark_objects;
	std::vector<GcObjectSpawnData> normal_objects;
	std::vector<GcObjectSpawnData> detail_objects;

	/*std::vector<GcObjectSpawnData::OverlapStyleEnum> overlapstyles;*/
	//std::vector<GcSeed> seeds;

	for(const auto& directory_entry : DirectoryIterator("nms_baseobjects/"))
	{
		Logger::Info("path : {}", directory_entry.path().string().c_str());

		IO::ResourceHandle base_object_list_handle = {directory_entry.path().string()};

		const GcExternalObjectList base_object_list = IO::TryGetNativeObject<GcExternalObjectList>(base_object_list_handle);
		distant_objects.insert(distant_objects.end(), base_object_list.Objects.DistantObjects.begin(), base_object_list.Objects.DistantObjects.end());
		landmark_objects.insert(landmark_objects.end(), base_object_list.Objects.Landmarks.begin(), base_object_list.Objects.Landmarks.end());
		normal_objects.insert(normal_objects.end(), base_object_list.Objects.Objects.begin(), base_object_list.Objects.Objects.end());
		detail_objects.insert(detail_objects.end(), base_object_list.Objects.DetailObjects.begin(), base_object_list.Objects.DetailObjects.end());
	}
	
	std::unordered_set<std::string> invalid_placement_names;
	std::unordered_set<std::string> invalid_placement_objects;

	std::array<std::string, 10> hard_valid_placement_names = 
	{
		"BIOMEPLANT", "FOREST", "FLORACLUMP", "BARRENROCKCLUMP", "SPARSECLUMP", "GRASS", "BLANKETCLUMP", "ROCKCLUMP", "JAMESPATCH", "GRASSCLUMP"
	};
	std::array<std::string, 4> filename_blacklist = 
	{
		"MODELS/PLANETS/BIOMES/COMMON/CRYSTALS/MEDIUM/",
		"MODELS/PLANETS/BIOMES/COMMON/CRYSTALS/SMALL/",
		"MODELS/PLANETS/BIOMES/COMMON/INTERACTIVE/",
		"MODELS/PLANETS/BIOMES/COMMON/INTERACTIVEFLORA/"
	};


	std::unordered_set<size_t> duplicate_indices;
	for (size_t o = 0; o < distant_objects.size(); o++)
	{
		auto& object = distant_objects[o];
		object.PlacementPriority = GcObjectSpawnData::PlacementPriorityEnum::High;
		//object.OverlapStyle = GcObjectSpawnData::OverlapStyleEnum::None;
		object.MinHeight = -2000.f;
		object.MaxHeight = 2000.f;

		if(object.Placement.Value.find("X") != std::string::npos)
		{
			invalid_placement_names.insert(object.Placement.Value);
			invalid_placement_objects.insert(object.Resource.Filename.Value);
		}
		for (size_t i = 0; i < filename_blacklist.size(); i++)
		{
			if (object.Resource.Filename.Value.find(filename_blacklist[i]) != std::string::npos)
			{
				duplicate_indices.insert(o);
				break;
			}
		}
	}
	for (size_t x = 0; x < distant_objects.size(); x++)
	{
		for (size_t y = x + 1; y < distant_objects.size(); y++)
		{
			if (x == y) continue;

			if (distant_objects[x] == distant_objects[y])
			{
				Logger::Info("duplicate: {}", distant_objects[x].Resource.Filename.Value.c_str());
				duplicate_indices.insert(x);
			}
		}
	}
	std::vector<size_t> desc_sorted_indices(duplicate_indices.begin(), duplicate_indices.end());
	std::sort(desc_sorted_indices.begin(), desc_sorted_indices.end(), std::greater<>());
	for (auto idx : desc_sorted_indices)
	{
		distant_objects.erase(distant_objects.begin() + idx);
	}

	duplicate_indices.clear();
	for (size_t o = 0; o < landmark_objects.size(); o++)
	{
		auto& object = landmark_objects[o];
		object.PlacementPriority = GcObjectSpawnData::PlacementPriorityEnum::High;
		//object.OverlapStyle = GcObjectSpawnData::OverlapStyleEnum::None;
		object.MinHeight = -2000.f;
		object.MaxHeight = 2000.f;

		if (object.Placement.Value.find("X") != std::string::npos)
		{
			invalid_placement_names.insert(object.Placement.Value);
			invalid_placement_objects.insert(object.Resource.Filename.Value);
		}
		for (size_t i = 0; i < filename_blacklist.size(); i++)
		{
			if (object.Resource.Filename.Value.find(filename_blacklist[i]) != std::string::npos)
			{
				duplicate_indices.insert(o);
				break;
			}
		}
	}
	for (size_t x = 0; x < landmark_objects.size(); x++)
	{
		for (size_t y = x + 1; y < landmark_objects.size(); y++)
		{
			if (x == y) continue;

			if (landmark_objects[x] == landmark_objects[y])
			{
				Logger::Info("duplicate: {}", landmark_objects[x].Resource.Filename.Value.c_str());
				duplicate_indices.insert(x);
			}
		}
	}
	desc_sorted_indices = std::vector<size_t>(duplicate_indices.begin(), duplicate_indices.end());
	std::sort(desc_sorted_indices.begin(), desc_sorted_indices.end(), std::greater<>());
	for (auto idx : desc_sorted_indices)
	{
		landmark_objects.erase(landmark_objects.begin() + idx);
	}

	duplicate_indices.clear();
	for (size_t o = 0; o < normal_objects.size(); o++)
	{
		auto& object = normal_objects[o];
		object.PlacementPriority = GcObjectSpawnData::PlacementPriorityEnum::High;
		//object.OverlapStyle = GcObjectSpawnData::OverlapStyleEnum::None;
		object.MinHeight = -2000.f;
		object.MaxHeight = 2000.f;

		if (object.Placement.Value.find("X") != std::string::npos)
		{
			invalid_placement_names.insert(object.Placement.Value);
			invalid_placement_objects.insert(object.Resource.Filename.Value);
		}
		for (size_t i = 0; i < filename_blacklist.size(); i++)
		{
			if (object.Resource.Filename.Value.find(filename_blacklist[i]) != std::string::npos)
			{
				duplicate_indices.insert(o);
				break;
			}
		}
	}
	for (size_t x = 0; x < normal_objects.size(); x++)
	{
		for (size_t y = x + 1; y < normal_objects.size(); y++)
		{
			if (x == y) continue;

			if (normal_objects[x] == normal_objects[y])
			{
				Logger::Info("duplicate: {}", normal_objects[x].Resource.Filename.Value.c_str());
				duplicate_indices.insert(x);
			}
		}
	}
	desc_sorted_indices = std::vector<size_t>(duplicate_indices.begin(), duplicate_indices.end());
	std::sort(desc_sorted_indices.begin(), desc_sorted_indices.end(), std::greater<>());
	for (auto idx : desc_sorted_indices)
	{
		normal_objects.erase(normal_objects.begin() + idx);
	}


	duplicate_indices.clear();
	for (size_t o = 0; o < detail_objects.size(); o++)
	{
		auto& object = detail_objects[o];
		object.PlacementPriority = GcObjectSpawnData::PlacementPriorityEnum::High;
		//object.OverlapStyle = GcObjectSpawnData::OverlapStyleEnum::None;
		object.MinHeight = -2000.f;
		object.MaxHeight = 2000.f;

		if (object.Placement.Value.find("X") != std::string::npos)
		{
			invalid_placement_names.insert(object.Placement.Value);
			invalid_placement_objects.insert(object.Resource.Filename.Value);
		}
		for (size_t i = 0; i < filename_blacklist.size(); i++)
		{
			if (object.Resource.Filename.Value.find(filename_blacklist[i]) != std::string::npos)
			{
				duplicate_indices.insert(o);
				break;
			}
		}
	}
	for (size_t x = 0; x < detail_objects.size(); x++)
	{
		for (size_t y = x + 1; y < detail_objects.size(); y++)
		{
			if (x == y) continue;

			if (detail_objects[x] == detail_objects[y])
			{
				Logger::Info("duplicate: {}", detail_objects[x].Resource.Filename.Value.c_str());
				duplicate_indices.insert(x);
			}
		}
	}
	desc_sorted_indices = std::vector<size_t>(duplicate_indices.begin(), duplicate_indices.end());
	std::sort(desc_sorted_indices.begin(), desc_sorted_indices.end(), std::greater<>());
	for (auto idx : desc_sorted_indices)
	{
		detail_objects.erase(detail_objects.begin() + idx);
	}

	std::vector<std::string> distant_object_filenames;
	std::vector<std::string> landmark_object_filenames;
	std::vector<std::string> normal_object_filenames;
	std::vector<std::string> detail_object_filenames;

	std::unordered_set<std::string> seen_names;
	std::unordered_map<std::string, unsigned int> name_iteration_map;


	constexpr unsigned int distant_entry_count = 1;
	constexpr unsigned int landmark_entry_count = 2;
	constexpr unsigned int object_entry_count = 2;
	constexpr unsigned int detail_entry_count = 4;

	//write each object into its own GcExternalObjectList
	for (auto& distant_object : distant_objects)
	{
		GcExternalObjectList external_distant_object;

		auto iter = hard_valid_placement_names.begin();
		for (size_t i = 0; i < distant_entry_count; i++)
		{
			std::string temp = distant_object.Placement.Value;
			external_distant_object.Objects.DistantObjects.push_back(distant_object);

			while (temp == *iter) iter++;
			distant_object.Placement.Value = *iter;
		}

		IO::ResourceHandle new_external_object_handle = IO::PushData(external_distant_object);

		size_t slash_offset = distant_object.Resource.Filename.Value.find_last_of("/");
		size_t dot_offset = distant_object.Resource.Filename.Value.find_first_of(".");

		std::string name = distant_object.Resource.Filename.Value.substr(slash_offset+1, dot_offset-slash_offset-1);
		if(!seen_names.insert(name).second)
		{
			name += "_VAR_" + std::to_string(++name_iteration_map[name]); 
		}

		IO::Write(new_external_object_handle, "output_objects/DISTANT/" + name + ".MBIN");
		distant_object_filenames.push_back("METADATA/SIMULATION/SOLARSYSTEM/BIOMES/FFPG/DISTANT/" + name + ".MBIN");
	}
	seen_names.clear();
	name_iteration_map.clear();

	for (auto& landmark_object : landmark_objects)
	{
		GcExternalObjectList external_landmark_object;

		auto iter = hard_valid_placement_names.begin();
		for (size_t i = 0; i < landmark_entry_count; i++)
		{
			std::string temp = landmark_object.Placement.Value;
			external_landmark_object.Objects.Landmarks.push_back(landmark_object);

			while (temp == *iter) iter++;
			landmark_object.Placement.Value = *iter;
		}

		IO::ResourceHandle new_external_object_handle = IO::PushData(external_landmark_object);

		size_t slash_offset = landmark_object.Resource.Filename.Value.find_last_of("/");
		size_t dot_offset = landmark_object.Resource.Filename.Value.find_first_of(".");

		std::string name = landmark_object.Resource.Filename.Value.substr(slash_offset + 1, dot_offset - slash_offset - 1);
		if (!seen_names.insert(name).second)
		{
			name += "_VAR_" + std::to_string(++name_iteration_map[name]);
		}

		IO::Write(new_external_object_handle, "output_objects/LANDMARK/" + name + ".MBIN");
		landmark_object_filenames.push_back("METADATA/SIMULATION/SOLARSYSTEM/BIOMES/FFPG/LANDMARK/" + name + ".MBIN");
	}
	seen_names.clear();
	name_iteration_map.clear();

	for (auto& normal_object : normal_objects)
	{
		GcExternalObjectList external_normal_object;

		auto iter = hard_valid_placement_names.begin();
		for (size_t i = 0; i < object_entry_count; i++)
		{
			std::string temp = normal_object.Placement.Value;
			external_normal_object.Objects.Objects.push_back(normal_object);

			while (temp == *iter) iter++;
			normal_object.Placement.Value = *iter;
		}

		IO::ResourceHandle new_external_object_handle = IO::PushData(external_normal_object);

		size_t slash_offset = normal_object.Resource.Filename.Value.find_last_of("/");
		size_t dot_offset = normal_object.Resource.Filename.Value.find_first_of(".");

		std::string name = normal_object.Resource.Filename.Value.substr(slash_offset + 1, dot_offset - slash_offset - 1);
		if (!seen_names.insert(name).second)
		{
			name += "_VAR_" + std::to_string(++name_iteration_map[name]);
		}

		IO::Write(new_external_object_handle, "output_objects/OBJECTS/" + name + ".MBIN");
		normal_object_filenames.push_back("METADATA/SIMULATION/SOLARSYSTEM/BIOMES/FFPG/OBJECT/" + name + ".MBIN");
	}
	seen_names.clear();
	name_iteration_map.clear();

	for (auto& detail_object : detail_objects)
	{
		GcExternalObjectList external_detail_object;

		auto iter = hard_valid_placement_names.begin();
		for (size_t i = 0; i < detail_entry_count; i++)
		{
			std::string temp = detail_object.Placement.Value;
			external_detail_object.Objects.DetailObjects.push_back(detail_object);

			while (temp == *iter) iter++;
			detail_object.Placement.Value = *iter;
		}

		IO::ResourceHandle new_external_object_handle = IO::PushData(external_detail_object);

		size_t slash_offset = detail_object.Resource.Filename.Value.find_last_of("/");
		size_t dot_offset = detail_object.Resource.Filename.Value.find_first_of(".");

		std::string name = detail_object.Resource.Filename.Value.substr(slash_offset + 1, dot_offset - slash_offset - 1);
		if (!seen_names.insert(name).second)
		{
			name += "_VAR_" + std::to_string(++name_iteration_map[name]);
		}

		IO::Write(new_external_object_handle, "output_objects/DETAIL/" + name + ".MBIN");
		detail_object_filenames.push_back("METADATA/SIMULATION/SOLARSYSTEM/BIOMES/FFPG/DETAIL/" + name + ".MBIN");
	}

	//gather data about base nms biomes.
	//as luck has it right now, as much biome sub types exist as unique colour palettes. so we prioritize uber biome count on colour palettes.
	std::unordered_set<std::string> colourpalettes_strings;
	std::unordered_map<std::string, std::string> tiletypes_map;
	std::unordered_map<std::string, std::string> texturefile_map;
	std::unordered_map<std::string, float> darkness_variations;
	//we can do more with clouds, because it has a lot of min/max values
	std::unordered_map<std::string, GcBiomeCloudSettings> cloud_settings;
	std::unordered_map<std::string, GcTerrainControls> terraincontrols_map;
	std::vector<GcScreenFilterOption> screen_filter_options;
	std::unordered_set<GcScreenFilters::ScreenFilterEnum> seen_enums;

	for(const auto& directory_entry : DirectoryIterator("nms_basebiomes/"))
	{
		Logger::Info("path: {}", directory_entry.path().string().c_str());

		IO::ResourceHandle base_biome_handle = {directory_entry.path().string()};

		const GcBiomeData base_biome_data = IO::TryGetNativeObject<GcBiomeData>(base_biome_handle);
		colourpalettes_strings.insert(base_biome_data.ColourPaletteFile.Value);
		texturefile_map.emplace(base_biome_data.ColourPaletteFile.Value, base_biome_data.TextureFile.Value);
		tiletypes_map.emplace(base_biome_data.ColourPaletteFile.Value, base_biome_data.TileTypesFile.Value);

		darkness_variations.emplace(base_biome_data.ColourPaletteFile.Value, base_biome_data.DarknessVariation);

		cloud_settings.emplace(base_biome_data.ColourPaletteFile.Value, base_biome_data.CloudSettings);

		terraincontrols_map.emplace(base_biome_data.ColourPaletteFile.Value, base_biome_data.Terrain);

		for(auto biome_filter_opt : base_biome_data.FilterOptions)
		{
			if(seen_enums.insert(biome_filter_opt.Filter.ScreenFilter).second)
			{
				if(biome_filter_opt.Weight > 1.f) biome_filter_opt.Weight = 1.f;

				screen_filter_options.push_back(biome_filter_opt);		
			}
		}
	}

	//pass to add missing filters
	for (size_t i = 0; i < 83; i++)
	{
		if (seen_enums.insert(static_cast<GcScreenFilters::ScreenFilterEnum>(i)).second)
		{
			GcScreenFilterOption unused_option;
			unused_option.Filter.ScreenFilter = static_cast<GcScreenFilters::ScreenFilterEnum>(i);
			unused_option.Weight = 0.05f;

			screen_filter_options.push_back(unused_option);
		}
	}

	GcExternalObjectListOptions distant_object_options{};
	distant_object_options.Name.Value = "DISTANT";
	distant_object_options.Probability = 1.f;
	distant_object_options.SeasonalProbabilityOverride = 1.f;
	distant_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	distant_object_options.AllowLimiting = true;
	distant_object_options.Order = 0;
	for (const auto& distant_file : distant_object_filenames)
	{
		NMS::VariableSizeString option_string;
		option_string.Value = distant_file;
		distant_object_options.Options.push_back(option_string);
	}

	GcExternalObjectListOptions landmark_object_options{};
	landmark_object_options.Name.Value = "LANDMARK";
	landmark_object_options.Probability = 1.f;
	landmark_object_options.SeasonalProbabilityOverride = 1.f;
	landmark_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	landmark_object_options.AllowLimiting = false;
	landmark_object_options.Order = 0;
	for (const auto& landmark_file : landmark_object_filenames)
	{
		NMS::VariableSizeString option_string;
		option_string.Value = landmark_file;
		landmark_object_options.Options.push_back(option_string);
	}

	GcExternalObjectListOptions normal_object_options{};
	normal_object_options.Name.Value = "OBJECT";
	normal_object_options.Probability = 1.f;
	normal_object_options.SeasonalProbabilityOverride = 1.f;
	normal_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	normal_object_options.AllowLimiting = false;
	normal_object_options.Order = 0;
	for (const auto& object_file : normal_object_filenames)
	{
		NMS::VariableSizeString option_string;
		option_string.Value = object_file;
		normal_object_options.Options.push_back(option_string);
	}

	GcExternalObjectListOptions detail_object_options{};
	detail_object_options.Name.Value = "DETAIL";
	detail_object_options.Probability = 1.f;
	detail_object_options.SeasonalProbabilityOverride = 1.f;
	detail_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	detail_object_options.AllowLimiting = false;
	detail_object_options.Order = 0;
	for (const auto& detail_file : detail_object_filenames)
	{
		NMS::VariableSizeString option_string;
		option_string.Value = detail_file;
		detail_object_options.Options.push_back(option_string);
	}

	//make uberbiome variations based on colour palettes
	// dont forget to adjust weight for same texture files biomes in BIOMEFILENAMES!
	distant_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Mountain;
	landmark_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Mountain;
	normal_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Mountain;
	detail_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Mountain;

	unsigned int uber_idx = 0;
	for(const auto& palette_string : colourpalettes_strings)
	{
		GcBiomeData uber_biome{};

		uber_biome.TextureFile.Value = texturefile_map[palette_string];
		uber_biome.OverlayFile.Value = "METADATA/SIMULATION/SOLARSYSTEM/OVERLAYS/TEST.MBIN";
		uber_biome.TileTypesFile.Value = tiletypes_map[palette_string];
		uber_biome.ColourPaletteFile.Value = palette_string;

		//i dont really care about mining substance data rn
		uber_biome.MiningSubstance1.UseRarity = true;
		uber_biome.MiningSubstance2.UseRarity = true;
		uber_biome.MiningSubstance3.UseRarity = true;

		uber_biome.FuelMultiplier = 1.f;

		for (size_t opt = 0; opt < 5; opt++)
		{
			for (size_t weight = 0; weight < 17; weight++)
			{
				uber_biome.WeatherOptions[opt].WeatherWeightings[weight] = 1.f;
			}
		}

		uber_biome.DarknessVariation = darkness_variations[palette_string];

		uber_biome.CloudSettings = cloud_settings[palette_string];

		uber_biome.Terrain = terraincontrols_map[palette_string];

		uber_biome.ExternalObjectLists.push_back(distant_object_options);
		uber_biome.ExternalObjectLists.push_back(landmark_object_options);
		uber_biome.ExternalObjectLists.push_back(normal_object_options);
		uber_biome.ExternalObjectLists.push_back(detail_object_options);

		uber_biome.FilterOptions = screen_filter_options;

		IO::ResourceHandle uber_biome_handle = IO::PushData(uber_biome);
		IO::Write(uber_biome_handle, std::string("output_uberbiomes/") + "UBERBIOME" + std::to_string(uber_idx) + ".MBIN");

		uber_idx++;
	}

	//detect multiple same texturefile entries to influence weight of uberbiome
	std::unordered_map<std::string, unsigned int> biome_texturecount_map;
	for(auto& [palette_str, tex_str] : texturefile_map)
	{
		biome_texturecount_map[tex_str]++;
	}

	//edit the biomefilenames to include uberbiomes
	IO::ResourceHandle biomefilenames_handle = {"nms_biomemisc/BIOMEFILENAMES.MBIN"};
	GcBiomeFileList base_filenames = IO::TryGetNativeObject<GcBiomeFileList>(biomefilenames_handle);

	for (size_t i = 0; i < 17; i++)
	{
		base_filenames.BiomeFiles[i].FileOptions.clear();

		auto palette_iter = colourpalettes_strings.begin();

		for (size_t opt = 0; opt < colourpalettes_strings.size(); opt++)
		{
			unsigned int tex_used_count = biome_texturecount_map[texturefile_map[*palette_iter++]];

			GcBiomeFileListOption biome_file_option{};
			biome_file_option.Weight = 1.f / static_cast<float>(tex_used_count);
			biome_file_option.PurpleSystemWeight = 1.f / static_cast<float>(tex_used_count);
			biome_file_option.SubType.BiomeSubType = static_cast<GcBiomeSubType::BiomeSubTypeEnum>(opt+1);
			biome_file_option.Filename.Value = "METADATA/SIMULATION/SOLARSYSTEM/BIOMES/FFPG/BIOMES/UBERBIOME" + std::to_string(opt) + ".MXML";

			base_filenames.BiomeFiles[i].FileOptions.push_back(biome_file_option);
		}
	}

	//add to biomefilenames base tiletype
	base_filenames.CommonExternalObjectLists.clear(); //removes underwater, underground, mountain, crystalscave, underwatervfx

	distant_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	landmark_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	normal_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;
	detail_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Base;

	base_filenames.CommonExternalObjectLists.push_back(distant_object_options);
	base_filenames.CommonExternalObjectLists.push_back(landmark_object_options);
	landmark_object_options.Name.Value = "LANDMARKTWO";
	base_filenames.CommonExternalObjectLists.push_back(landmark_object_options);
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	normal_object_options.Name.Value = "OBJECTTWO";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	normal_object_options.Name.Value = "OBJECTTHREE";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "DETAILTWO";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "DETAILTHREE";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "DETAILFOUR";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);

	distant_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Cave;
	landmark_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Cave;
	normal_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Cave;
	detail_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Cave;

	distant_object_options.Name.Value = "CDISTANT";
	base_filenames.CommonExternalObjectLists.push_back(distant_object_options);
	landmark_object_options.Name.Value = "CLANDMARK";
	base_filenames.CommonExternalObjectLists.push_back(landmark_object_options);
	landmark_object_options.Name.Value = "CLANDMARKTWO";
	base_filenames.CommonExternalObjectLists.push_back(landmark_object_options);
	normal_object_options.Name.Value = "COBJECT";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	normal_object_options.Name.Value = "COBJECTTWO";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	normal_object_options.Name.Value = "COBJECTTHREE";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	detail_object_options.Name.Value = "CDETAIL";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "CDETAILTWO";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "CDETAILTHREE";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "CDETAILFOUR";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);

	distant_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Underwater;
	landmark_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Underwater;
	normal_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Underwater;
	detail_object_options.TileType.TileType = GcTerrainTileType::TileTypeEnum::Underwater;

	distant_object_options.Name.Value = "UDISTANT";
	base_filenames.CommonExternalObjectLists.push_back(distant_object_options);
	landmark_object_options.Name.Value = "ULANDMARK";
	base_filenames.CommonExternalObjectLists.push_back(landmark_object_options);
	landmark_object_options.Name.Value = "ULANDMARKTWO";
	base_filenames.CommonExternalObjectLists.push_back(landmark_object_options);
	normal_object_options.Name.Value = "UOBJECT";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	normal_object_options.Name.Value = "UOBJECTTWO";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	normal_object_options.Name.Value = "UOBJECTTHREE";
	base_filenames.CommonExternalObjectLists.push_back(normal_object_options);
	detail_object_options.Name.Value = "UDETAIL";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "UDETAILTWO";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "UDETAILTHREE";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);
	detail_object_options.Name.Value = "UDETAILFOUR";
	base_filenames.CommonExternalObjectLists.push_back(detail_object_options);

	IO::PushData(base_filenames, biomefilenames_handle);
	IO::Write(biomefilenames_handle, "output_biomefilenames/BIOMEFILENAMES.MBIN");

	//Edit the biomelistperstartype to make it uniform across
	IO::ResourceHandle biomelistperstartype_handle = {"nms_biomemisc/BIOMELISTPERSTARTYPE.MBIN"};
	GcBiomeListPerStarType biomelist = IO::TryGetNativeObject<GcBiomeListPerStarType>(biomelistperstartype_handle);

	for (size_t star = 0; star < 5; star++)
	{
		for (size_t i = 0; i < 16; i++)
		{
			biomelist.StarType[star].BiomeProbability[i] = 1.f;
			biomelist.StarType[star].PrimeBiomeProbability[i] = 1.f;
		}
		
		//All type to 0
		biomelist.StarType[star].BiomeProbability[16] = 0.f;
		biomelist.StarType[star].PrimeBiomeProbability[16] = 0.f;
	}

	for (size_t i = 0; i < 16; i++)
	{
		biomelist.LushYellow.BiomeProbability[i] = 1.f;
		biomelist.LushYellow.PrimeBiomeProbability[i] = 1.f;
	}
	biomelist.LushYellow.BiomeProbability[16] = 0.f;
	biomelist.LushYellow.PrimeBiomeProbability[16] = 0.f;

	for (size_t i = 0; i < 16; i++)
	{
		biomelist.AbandonedYellow.BiomeProbability[i] = 1.f;
		biomelist.AbandonedYellow.PrimeBiomeProbability[i] = 1.f;
	}
	biomelist.AbandonedYellow.BiomeProbability[16] = 0.f;
	biomelist.AbandonedYellow.PrimeBiomeProbability[16] = 0.f;

	biomelist.ConvertDeadToWeird = 0.f;

	IO::PushData(biomelist, biomelistperstartype_handle);

	IO::Write(biomelistperstartype_handle, "output_biomelist/BIOMELISTPERSTARTYPE.MBIN");
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
