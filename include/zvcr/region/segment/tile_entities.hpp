#pragma once

#include <map>
#include <string>
#include <vector>
#include <zvcr/common/definitions.hpp>

namespace zvcr::region::segment::tile_entities {

    // we only officially support 1.20.4-1.20.6!!!
    // see https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol_History
#if PROTOCOL_VERSION == 765
    constexpr auto TOTAL_TILE_ENTITIES = 41;
#elif PROTOCOL_VERSION == 766
    constexpr auto TOTAL_TILE_ENTITIES = 44;
#endif

    struct TileEntityCountInfo {
        std::vector<uint16_t> counts{};
        time_t timestamp{};
    };

    using TileEntityCounts = std::vector<TileEntityCountInfo>;

    enum class TileEntityType {
        FURNACE = 0,
        CHEST = 1,
        TRAPPED_CHEST = 2,
        ENDER_CHEST = 3,
        JUKEBOX = 4,
        DISPENSER = 5,
        DROPPER = 6,
        SIGN = 7,
        HANGING_SIGN = 8,
        MOB_SPAWNER = 9,
        PISTON = 10,
        BREWING_STAND = 11,
        ENCHANTING_TABLE = 12,
        END_PORTAL = 13,
        BEACON = 14,
        SKULL = 15,
        DAYLIGHT_DETECTOR = 16,
        HOPPER = 17,
        COMPARATOR = 18,
        BANNER = 19,
        STRUCTURE_BLOCK = 20,
        END_GATEWAY = 21,
        COMMAND_BLOCK = 22,
        SHULKER_BOX = 23,
        BED = 24,
        CONDUIT = 25,
        BARREL = 26,
        SMOKER = 27,
        BLAST_FURNACE = 28,
        LECTERN = 29,
        BELL = 30,
        JIGSAW = 31,
        CAMPFIRE = 32,
        BEEHIVE = 33,
        SCULK_SENSOR = 34,
        CALIBRATED_SCULK_SENSOR = 35,
        SCULK_CATALYST = 36,
        SCULK_SHRIEKER = 37,
        CHISELED_BOOKSHELF = 38,
        BRUSHABLE_BLOCK = 39,
        DECORATED_POT = 40,
#if PROTOCOL_VERSION > 766 // 1.20.5+
        CRAFTER = 41,
        TRIAL_SPAWNER = 42,
        VAULT = 43
#endif
    };

    static const std::map<TileEntityType, std::string> TileEntityTypeToString = {
        {TileEntityType::FURNACE, "furnace"},
        {TileEntityType::CHEST, "chest"},
        {TileEntityType::TRAPPED_CHEST, "trapped_chest"},
        {TileEntityType::ENDER_CHEST, "ender_chest"},
        {TileEntityType::JUKEBOX, "jukebox"},
        {TileEntityType::DISPENSER, "dispenser"},
        {TileEntityType::DROPPER, "dropper"},
        {TileEntityType::SIGN, "sign"},
        {TileEntityType::HANGING_SIGN, "hanging_sign"},
        {TileEntityType::MOB_SPAWNER, "mob_spawner"},
        {TileEntityType::PISTON, "piston"},
        {TileEntityType::BREWING_STAND, "brewing_stand"},
        {TileEntityType::ENCHANTING_TABLE, "enchanting_table"},
        {TileEntityType::END_PORTAL, "end_portal"},
        {TileEntityType::BEACON, "beacon"},
        {TileEntityType::SKULL, "skull"},
        {TileEntityType::DAYLIGHT_DETECTOR, "daylight_detector"},
        {TileEntityType::HOPPER, "hopper"},
        {TileEntityType::COMPARATOR, "comparator"},
        {TileEntityType::BANNER, "banner"},
        {TileEntityType::STRUCTURE_BLOCK, "structure_block"},
        {TileEntityType::END_GATEWAY, "end_gateway"},
        {TileEntityType::COMMAND_BLOCK, "command_block"},
        {TileEntityType::SHULKER_BOX, "shulker_box"},
        {TileEntityType::BED, "bed"},
        {TileEntityType::CONDUIT, "conduit"},
        {TileEntityType::BARREL, "barrel"},
        {TileEntityType::SMOKER, "smoker"},
        {TileEntityType::BLAST_FURNACE, "blast_furnace"},
        {TileEntityType::LECTERN, "lectern"},
        {TileEntityType::BELL, "bell"},
        {TileEntityType::JIGSAW, "jigsaw"},
        {TileEntityType::CAMPFIRE, "campfire"},
        {TileEntityType::BEEHIVE, "beehive"},
        {TileEntityType::SCULK_SENSOR, "sculk_sensor"},
        {TileEntityType::CALIBRATED_SCULK_SENSOR, "calibrated_sculk_sensor"},
        {TileEntityType::SCULK_CATALYST, "sculk_catalyst"},
        {TileEntityType::SCULK_SHRIEKER, "sculk_shrieker"},
        {TileEntityType::CHISELED_BOOKSHELF, "chiseled_bookshelf"},
        {TileEntityType::BRUSHABLE_BLOCK, "brushable_block"},
        {TileEntityType::DECORATED_POT, "decorated_pot"},
#if PROTOCOL_VERSION > 766 // 1.20.5+
        {TileEntityType::CRAFTER, "crafter"},
        {TileEntityType::TRIAL_SPAWNER, "trial_spawner"},
        {TileEntityType::VAULT, "vault"},
#endif
    };

    static const std::map<std::string, TileEntityType> TileEntityTypeFromString = {
        {"furnace", TileEntityType::FURNACE},
        {"chest", TileEntityType::CHEST},
        {"trapped_chest", TileEntityType::TRAPPED_CHEST},
        {"ender_chest", TileEntityType::ENDER_CHEST},
        {"jukebox", TileEntityType::JUKEBOX},
        {"dispenser", TileEntityType::DISPENSER},
        {"dropper", TileEntityType::DROPPER},
        {"sign", TileEntityType::SIGN},
        {"hanging_sign", TileEntityType::HANGING_SIGN},
        {"mob_spawner", TileEntityType::MOB_SPAWNER},
        {"piston", TileEntityType::PISTON},
        {"brewing_stand", TileEntityType::BREWING_STAND},
        {"enchanting_table", TileEntityType::ENCHANTING_TABLE},
        {"end_portal", TileEntityType::END_PORTAL},
        {"beacon", TileEntityType::BEACON},
        {"skull", TileEntityType::SKULL},
        {"daylight_detector", TileEntityType::DAYLIGHT_DETECTOR},
        {"hopper", TileEntityType::HOPPER},
        {"comparator", TileEntityType::COMPARATOR},
        {"banner", TileEntityType::BANNER},
        {"structure_block", TileEntityType::STRUCTURE_BLOCK},
        {"end_gateway", TileEntityType::END_GATEWAY},
        {"command_block", TileEntityType::COMMAND_BLOCK},
        {"shulker_box", TileEntityType::SHULKER_BOX},
        {"bed", TileEntityType::BED},
        {"conduit", TileEntityType::CONDUIT},
        {"barrel", TileEntityType::BARREL},
        {"smoker", TileEntityType::SMOKER},
        {"blast_furnace", TileEntityType::BLAST_FURNACE},
        {"lectern", TileEntityType::LECTERN},
        {"bell", TileEntityType::BELL},
        {"jigsaw", TileEntityType::JIGSAW},
        {"campfire", TileEntityType::CAMPFIRE},
        {"beehive", TileEntityType::BEEHIVE},
        {"sculk_sensor", TileEntityType::SCULK_SENSOR},
        {"calibrated_sculk_sensor", TileEntityType::CALIBRATED_SCULK_SENSOR},
        {"sculk_catalyst", TileEntityType::SCULK_CATALYST},
        {"sculk_shrieker", TileEntityType::SCULK_SHRIEKER},
        {"chiseled_bookshelf", TileEntityType::CHISELED_BOOKSHELF},
        {"brushable_block", TileEntityType::BRUSHABLE_BLOCK},
        {"decorated_pot", TileEntityType::DECORATED_POT},
#if PROTOCOL_VERSION > 766 // 1.20.5+
        {"crafter", TileEntityType::CRAFTER},
        {"trial_spawner", TileEntityType::TRIAL_SPAWNER},
        {"vault", TileEntityType::VAULT},
#endif
    };

    [[nodiscard]]
    inline std::string to_string(const TileEntityType type) {
        return TileEntityTypeToString.at(type);
    }

    [[nodiscard]]
    inline TileEntityType from_string(const std::string &type) {
        return TileEntityTypeFromString.at(type);
    }

}