#pragma once

#include <list>
#include <unordered_map>
#include <utility>

#include "enums.h" // IWYU pragma: keep

template<typename Key, typename Value>
class lru_cache
{
    public:
        using Pair = std::pair<Key, Value>;

        void insert( int limit, const Key &, const Value & );
        Value get( const Key &, const Value &default_ ) const;
        void remove( const Key & );

        void clear();
        const std::list<Pair> &list() const;
    private:
        void trim( int limit );
        std::list<Pair> ordered_list;
        std::unordered_map<Key, typename std::list<Pair>::iterator> map;
};


