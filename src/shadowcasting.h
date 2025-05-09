#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <string>

#include "game_constants.h"
#include "lightmap.h"

struct point;
struct tripoint;
struct diagonal_blocks;

// For light we store four values, depending on the direction that the light
// comes from.  This allows us to determine whether the side of the wall the
// player is looking at is lit.
// For non-opaque tiles direction doesn't matter so we just use the single
// default_ value.
enum class quadrant {
    NE, SE, SW, NW,
    default_ = NE
};

struct four_quadrants {
    four_quadrants() = default;
    explicit constexpr four_quadrants( float v ) : values{{v, v, v, v}} {}

    std::array<float, 4> values;

    float &operator[]( quadrant q ) {
        return values[static_cast<int>( q )];
    }
    float operator[]( quadrant q ) const {
        return values[static_cast<int>( q )];
    }
    void fill( float v ) {
        values[0] = v;
        values[1] = v;
        values[2] = v;
        values[3] = v;
    }
    float max() const {
        return *std::max_element( values.begin(), values.end() );
    }
    std::string to_string() const;

    friend four_quadrants operator*( const four_quadrants &l, const four_quadrants &r ) {
        four_quadrants result;
        std::transform( l.values.begin(), l.values.end(), r.values.begin(),
                        result.values.begin(), std::multiplies<float>() );
        return result;
    }

    friend four_quadrants elementwise_max( const four_quadrants &l, const four_quadrants &r ) {
        four_quadrants result;
        std::transform( l.values.begin(), l.values.end(), r.values.begin(),
        result.values.begin(), []( float l, float r ) {
            return std::max( l, r );
        } );
        return result;
    }

    friend four_quadrants elementwise_max( const four_quadrants &l, const float r ) {
        four_quadrants result( l );
        for( float &v : result.values ) {
            // This looks like it should be v = std::max( v, r ) doesn't it?
            // It turns out this is one simple trick that mingw-w64 HATES,
            // triggering constant crashes when the above code was used.
            if( v < r ) {
                v = r;
            }
        }
        return result;
    }
};

// Hoisted to header and inlined so the test in tests/shadowcasting_test.cpp can use it.
// Beer-Lambert law says attenuation is going to be equal to
// 1 / (e^al) where a = coefficient of absorption and l = length.
// Factoring out length, we get 1 / (e^((a1*a2*a3*...*an)*l))
// We merge all of the absorption values by taking their cumulative average.
inline float sight_calc( const float &numerator, const float &transparency, const int &distance )
{
    return numerator / std::exp( transparency * distance );
}
inline bool sight_check( const float &transparency, const float &/*intensity*/ )
{
    return transparency > LIGHT_TRANSPARENCY_SOLID;
}
inline void update_light( float &update, const float &new_value, quadrant )
{
    update = std::max( update, new_value );
}
inline void update_light_quadrants( four_quadrants &update, const float &new_value, quadrant q )
{
    update[q] = std::max( update[q], new_value );
}
inline float accumulate_transparency( const float &cumulative_transparency,
                                      const float &current_transparency, const int &distance )
{
    return ( ( distance - 1 ) * cumulative_transparency + current_transparency ) / distance;
}

inline float sight_from_lookup( const float &numerator, const float &transparency,
                                const int &/*distance*/ )
{
    return numerator * transparency;
}

template<typename T, typename Out, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & ) >
void castLightAll( Out( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
                   const T( &input_array )[MAPSIZE_X][MAPSIZE_Y],
                   const diagonal_blocks( &blocked_array )[MAPSIZE_X][MAPSIZE_Y],
                   point offset, int offsetDistance = 0,
                   T numerator = 1.0 );

template<typename T, typename Out, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & ),
         T( *lookup_calc )( const T &, const T &, const int & )>
void castLightAllWithLookup( Out( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
                             const T( &input_array )[MAPSIZE_X][MAPSIZE_Y],
                             const diagonal_blocks( &blocked_array )[MAPSIZE_X][MAPSIZE_Y],
                             const point &offset, int offsetDistance = 0, T numerator = 1.0 );

template<typename T>
using array_of_grids_of = std::array<T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS>;

// TODO: Generalize the floor check, allow semi-transparent floors
template< typename T, T( *calc )( const T &, const T &, const int & ),
          bool( *check )( const T &, const T & ),
          T( *accumulate )( const T &, const T &, const int & ) >
void cast_zlight(
    const array_of_grids_of<T> &output_caches,
    const array_of_grids_of<const T> &input_arrays,
    const array_of_grids_of<const bool> &floor_caches,
    const array_of_grids_of<const diagonal_blocks> &blocked_caches,
    const tripoint &origin, int offset_distance, T numerator );


