#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace zvcr::region::segment::tile_entities {

#if PROTOCOL_VERSION >= 768
    constexpr auto TOTAL_TILE_ENTITIES = 45;
#elif PROTOCOL_VERSION >= 766
    constexpr auto TOTAL_TILE_ENTITIES = 44;
#elif PROTOCOL_VERSION == 765
    constexpr auto TOTAL_TILE_ENTITIES = 41;
#endif

    struct TileEntityCountInfo {
        std::vector<uint16_t> counts{};
        time_t timestamp{};
    };

    using TileEntityCounts = std::vector<TileEntityCountInfo>;

    enum class TileEntityType {
        FURNACE = 0,
        CHEST,
        TRAPPED_CHEST,
        ENDER_CHEST,
        JUKEBOX,
        DISPENSER,
        DROPPER,
        SIGN,
        HANGING_SIGN,
        MOB_SPAWNER,
#if PROTOCOL_VERSION >= 768 // 1.21.2+
        CREAKING_HEART,
#endif
        PISTON,
        BREWING_STAND,
        ENCHANTING_TABLE,
        END_PORTAL,
        BEACON,
        SKULL,
        DAYLIGHT_DETECTOR,
        HOPPER,
        COMPARATOR,
        BANNER,
        STRUCTURE_BLOCK,
        END_GATEWAY,
        COMMAND_BLOCK,
        SHULKER_BOX,
        BED,
        CONDUIT,
        BARREL,
        SMOKER,
        BLAST_FURNACE,
        LECTERN,
        BELL,
        JIGSAW,
        CAMPFIRE,
        BEEHIVE,
        SCULK_SENSOR,
        CALIBRATED_SCULK_SENSOR,
        SCULK_CATALYST,
        SCULK_SHRIEKER,
        CHISELED_BOOKSHELF,
        BRUSHABLE_BLOCK,
        DECORATED_POT,
#if PROTOCOL_VERSION >= 766 // 1.20.5+
        CRAFTER,
        TRIAL_SPAWNER,
        VAULT
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
#if PROTOCOL_VERSION >= 768 // 1.21.2+
        {TileEntityType::CREAKING_HEART, "creaking_heart"},
#endif
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
#if PROTOCOL_VERSION >= 766 // 1.20.5+
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
#if PROTOCOL_VERSION >= 768 // 1.21.2+
        {"creaking_heart", TileEntityType::CREAKING_HEART},
#endif
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
#if PROTOCOL_VERSION >= 766 // 1.20.5+
        {"crafter", TileEntityType::CRAFTER},
        {"trial_spawner", TileEntityType::TRIAL_SPAWNER},
        {"vault", TileEntityType::VAULT},
#endif
    };

    [[nodiscard]]
    inline std::string toString(const TileEntityType type) {
        return TileEntityTypeToString.at(type);
    }

    [[nodiscard]]
    inline TileEntityType fromString(const std::string& type) {
        return TileEntityTypeFromString.at(type);
    }

}