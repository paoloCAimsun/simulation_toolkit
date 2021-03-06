//
//! Copyright © 2017
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <geometrix/numeric/number_comparison_policy.hpp>
#include <geometrix/primitive/point.hpp>
#include <geometrix/algorithm/grid_traits.hpp>
#include <geometrix/algorithm/grid_2d.hpp>
#include <geometrix/algorithm/hash_grid_2d.hpp>
#include <geometrix/algorithm/orientation_grid_traversal.hpp>
#include <geometrix/algorithm/fast_voxel_grid_traversal.hpp>
#include <geometrix/primitive/segment.hpp>
#include <junction/ConcurrentMap_Leapfrog.h>
#include <stk/random/xorshift1024starphi_generator.hpp>
#include <stk/geometry/geometry_kernel.hpp>
#include <stk/utility/compressed_integer_pair.hpp>
#include <stk/utility/boost_unique_ptr.hpp>
#include <junction/Core.h>
#include <turf/Util.h>
#include <chrono>

namespace stk {
    struct compressed_integer_pair_key_traits
    {
        typedef std::uint64_t Key;
        typedef typename turf::util::BestFit<std::uint64_t>::Unsigned Hash;
        static const Key NullKey = (std::numeric_limits<Key>::max)();
        static const Hash NullHash = Hash((std::numeric_limits<Key>::max)());
        static Hash hash(Key key)
        {
            return turf::util::avalanche(Hash(key));
        }
        static Key dehash(Hash hash)
        {
            return (Key)turf::util::deavalanche(hash);
        }
    };

    template <typename T>
    struct pointer_value_traits
    {
        typedef T* Value;
        typedef typename turf::util::BestFit<T*>::Unsigned IntType;
        static const IntType NullValue = 0;
        static const IntType Redirect = 1;
    };

    template<typename Data, typename GridTraits>
    class concurrent_hash_grid_2d
    {
    public:

        typedef Data* data_ptr;
        typedef GridTraits traits_type;
        typedef stk::compressed_integer_pair key_type;
        typedef junction::ConcurrentMap_Leapfrog<std::uint64_t, Data*, stk::compressed_integer_pair_key_traits, stk::pointer_value_traits<Data>> grid_type;

        concurrent_hash_grid_2d( const GridTraits& traits )
            : m_gridTraits(traits)
            , m_grid()
        {}

        ~concurrent_hash_grid_2d()
        {
            quiesce();
            clear();
        }

        template <typename Point>
        data_ptr find_cell(const Point& point) const
        {
            BOOST_CONCEPT_ASSERT( (geometrix::Point2DConcept<Point>) );
            GEOMETRIX_ASSERT( is_contained( point ) );
            boost::uint32_t i = m_gridTraits.get_x_index(geometrix::get<0>(point));
            boost::uint32_t j = m_gridTraits.get_y_index(geometrix::get<1>(point));
            return find_cell(i,j);
        }

        data_ptr find_cell(boost::uint32_t i, boost::uint32_t j) const
        {
            stk::compressed_integer_pair p = { i, j };
            auto iter = m_grid.find(p.to_uint64());
            GEOMETRIX_ASSERT(iter.getValue() != (data_ptr)stk::pointer_value_traits<Data>::Redirect);
            return iter.getValue();
        }

        template <typename Point>
        Data& get_cell(const Point& point)
        {
            BOOST_CONCEPT_ASSERT( (geometrix::Point2DConcept<Point>) );
            GEOMETRIX_ASSERT( is_contained( point ) );
            boost::uint32_t i = m_gridTraits.get_x_index(geometrix::get<0>(point));
            boost::uint32_t j = m_gridTraits.get_y_index(geometrix::get<1>(point));
            return get_cell(i,j);
        }

        Data& get_cell(boost::uint32_t i, boost::uint32_t j)
        {
            stk::compressed_integer_pair p = { i, j };
            auto mutator = m_grid.insertOrFind(p.to_uint64());
            auto result = mutator.getValue();
            if (result == nullptr)
            {
                auto pNewData = boost::make_unique<Data>();
                result = pNewData.get();
                if (result != mutator.exchangeValue(result))
                    pNewData.release();
                result = mutator.getValue();
            }

            GEOMETRIX_ASSERT(result != nullptr);
            return *result;
        }

        const traits_type& get_traits() const { return m_gridTraits; }

        template <typename Point>
        bool is_contained( const Point& p ) const
        {
            BOOST_CONCEPT_ASSERT( (geometrix::Point2DConcept<Point>) );
            return m_gridTraits.is_contained( p );
        }

        void erase(boost::uint32_t i, boost::uint32_t j)
        {
            stk::compressed_integer_pair p = { i, j };
            auto iter = m_grid.find(p.to_uint64());
            if (iter.isValid())
                iter.eraseValue();
        }

        void clear()
        {
            auto it = typename grid_type::Iterator(m_grid);
            while(it.isValid())
            {
                auto pValue = it.getValue();
                delete pValue;
                m_grid.erase(it.getKey());
                it.next();
            };
        }

        //! This should be called when the grid is not being modified to allow threads to reclaim memory from deletions.
        void quiesce()
        {
            junction::DefaultQSBR.flush();
        }

    private:

        traits_type m_gridTraits;
        mutable grid_type m_grid;

    };

}//! namespace stk;

