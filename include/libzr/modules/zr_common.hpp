#pragma once

#define CHUNK_SIDELENGTH 16
#define SECTION_SIZE CHUNK_SIDELENGTH * CHUNK_SIDELENGTH * CHUNK_SIDELENGTH
#define TILE_SIZE CHUNK_SIDELENGTH * CHUNK_SIDELENGTH
#define REGION_SIDELENGTH 32
#define SEGMENTS_PER_REGION REGION_SIDELENGTH * REGION_SIDELENGTH

#include <cmath>
#include <utility>
#include <cstdint>
#include <vector>
#include <bits/stdc++.h>
