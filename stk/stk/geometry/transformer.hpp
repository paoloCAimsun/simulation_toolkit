//
//! Copyright � 2017
//! Brandon Kohn
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef STK_GEOMETRY_TRANSFORMER_HPP
#define STK_GEOMETRY_TRANSFORMER_HPP

#include <geometrix/tensor/matrix.hpp>
#include <geometrix/tags.hpp>
#include <geometrix/tensor/homogeneous_adaptor.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <type_traits>

namespace stk {
    
    namespace detail {
        template <unsigned int N, unsigned int D>
        struct identity_setter
        {
            BOOST_STATIC_CONSTANT(unsigned int, R = N / D);
            BOOST_STATIC_CONSTANT(unsigned int, C = N % D);
            
            template <typename Matrix>
            static void apply(Matrix& m)
            {
                m[R][C] = get<typename geometrix::type_at<Matrix, R, C>::type, R, C>();
                identity_setter<N-1, D>::apply(m);
            }
            
        private:

            template <typename T, unsigned int I, unsigned int J>
            static T get(typename std::enable_if<I != J>::type * = nullptr)
            {
                return geometrix::constants::zero<T>();
            }

            template <typename T, unsigned int I, unsigned int J>
            static T get(typename std::enable_if<I == J>::type * = nullptr)
            {
                return geometrix::constants::one<T>();
            }

        };
        
        template <unsigned int D>
        struct identity_setter<0, D>
        {
            template <typename Matrix>
            static void apply(Matrix& m)
            {
                m[0][0] = geometrix::constants::one<typename geometrix::type_at<Matrix,0,0>::type>();
            }
        };
    }
    
    class transformer2
    {
        BOOST_STATIC_CONSTANT(unsigned int, dimensionality = 2 + 1);
        using element_type = double;
        using transform_matrix = geometrix::matrix<element_type, dimensionality, dimensionality>;
        
    public:
    
        transformer2()
        {
            reset();
        }
        
        template <typename Matrix>
        transformer2(const Matrix& m)
        : m_transform(geometrix::construct<transform_matrix>(m))
        {}
        
        void reset()
        {
            detail::identity_setter<dimensionality * dimensionality - 1, dimensionality>::apply(m_transform);
        }
        
        transformer2& translate(const vector2& v)
        {
            using namespace geometrix;
            
            transform_matrix m = 
            {
                1.0, 0.0, get<0>(v).value()
              , 0.0, 1.0, get<1>(v).value()
              , 0.0, 0.0, 1.0 
            };
            
            m_transform = construct<transform_matrix>(m_transform * m);
            
            return *this;
        }
        
        transformer2& rotate(const point2& origin, const units::angle& theta)
        {
            using namespace geometrix;
            using std::cos;
            using std::sin;
            
            transform_matrix t = 
            {
                1.0, 0.0, get<0>(origin).value()
              , 0.0, 1.0, get<1>(origin).value()
              , 0.0, 0.0, 1.0 
            };
            
            auto sint = sin(theta);
            auto cost = cos(theta);
            transform_matrix r = 
            {
                cost, -sint, 0.0
              , sint, cost, 0.0
              , 0.0, 0.0, 1.0
            };
            
            transform_matrix tp = 
            {
                1.0, 0.0, -get<0>(origin).value()
              , 0.0, 1.0, -get<1>(origin).value()
              , 0.0, 0.0, 1.0 
            };

            m_transform = construct<transform_matrix>(m_transform * t * r * tp);
            
            return *this;
        }
                
        transformer2& rotate(const units::angle& theta)
        {
            using namespace geometrix;
            using std::cos;
            using std::sin;
                        
            auto sint = sin(theta);
            auto cost = cos(theta);
            transform_matrix r = 
            {
                cost, -sint, 0.0
              , sint, cost, 0.0
              , 0.0, 0.0, 1.0
            };
            
            m_transform = construct<transform_matrix>(m_transform * r);
            
            return *this;
        }
        
        transformer2& rotate(const dimensionless2& a, const dimensionless2& b)
        {
            using namespace geometrix;
            
            auto sint = exterior_product_area(a,b);
            auto cost = dot_product(a,b);
            transform_matrix r = 
            {
                cost, -sint, 0.0
              , sint, cost, 0.0
              , 0.0, 0.0, 1.0
            };
            
            m_transform = construct<transform_matrix>(m_transform * r);
            
            return *this;
        }

        template <typename Geometry>
        Geometry operator()(const Geometry& p) const
        {
            return transform(p, typename geometrix::geometry_tag_of<Geometry>::type());
        }

    private:
        
        template <typename Point>
        Point transform(const Point& p, geometrix::geometry_tags::point_tag) const
        {
            using namespace geometrix;
            return construct<Point>(m_transform * as_positional_homogeneous<typename arithmetic_type_of<Point>::type>(p));
        }

        template <typename Vector>
        Vector transform(const Vector& v, geometrix::geometry_tags::vector_tag) const
        {
            using namespace geometrix;
            return construct<Vector>(m_transform * as_vectoral_homogeneous<typename arithmetic_type_of<Vector>::type>(v));
        }

        template <typename Segment>
        Segment transform(const Segment& s, geometrix::geometry_tags::segment_tag) const
        {
            using namespace geometrix;
            return construct<Segment>(transform(get_start(s), geometry_tags::point_tag()), transform(get_end(s), geometry_tags::point_tag()));
        }

        template <typename Polyline>
        Polyline transform(const Polyline& p, geometrix::geometry_tags::polyline_tag) const
        {
            using namespace geometrix;
            using namespace boost::adaptors;
            using point_t = typename point_sequence_traits<Polyline>::point_type;

            Polyline result;
            boost::copy(p | transformed([this](const point_t& p) { return transform(p, geometry_tags::point_tag()); }), std::back_inserter(result));

            return result;
        }

        template <typename Polygon>
        Polygon transform(const Polygon& p, geometrix::geometry_tags::polygon_tag) const
        {
            using namespace geometrix;
            using namespace boost::adaptors;
            using point_t = typename point_sequence_traits<Polygon>::point_type;

            Polygon result;
            boost::copy(p | transformed([this](const point_t& p) { return transform(p, geometry_tags::point_tag()); }), std::back_inserter(result));

            return result;
        }

        transform_matrix m_transform;

    };

    class transformer3
    {
        BOOST_STATIC_CONSTANT(unsigned int, dimensionality = 3 + 1);
        using element_type = double;
        using transform_matrix = geometrix::matrix<element_type, dimensionality, dimensionality>;

    public:

        transformer3()
        {
            reset();
        }

        template <typename Matrix>
        transformer3(const Matrix& m)
            : m_transform(geometrix::construct<transform_matrix>(m))
        {}

        void reset()
        {
            detail::identity_setter<dimensionality * dimensionality - 1, dimensionality>::apply(m_transform);
        }

        transformer3& translate(const vector3& v)
        {
            using namespace geometrix;

            transform_matrix m =
            {
                  1.0, 0.0, 0.0, get<0>(v).value()
                , 0.0, 1.0, 0.0, get<1>(v).value()
                , 0.0, 0.0, 1.0, get<2>(v).value()
                , 0.0, 0.0, 0.0, 1.0
            };

            m_transform = construct<transform_matrix>(m_transform * m);

            return *this;
        }

        transformer3& rotate_x(const units::angle& roll)
        {
            using namespace geometrix;
            using std::cos;
            using std::sin;

            auto sinr = sin(roll);
            auto cosr = cos(roll);

            transform_matrix r =
            {
                    1,  0,  0,  0
                ,   0,  cosr,   -sinr,  0
                ,   0,  sinr,   cosr, 0
                ,   0,  0,  0,  1
            };

            m_transform = construct<transform_matrix>(m_transform * r);

            return *this;
        }

        transformer3& rotate_y(const units::angle& pitch)
        {
            using namespace geometrix;
            using std::cos;
            using std::sin;
            auto sinp = sin(pitch);
            auto cosp = cos(pitch);
            transform_matrix r =
            {
                    cosp,   0,  sinp,   0
                ,   0,  1,  0,  0
                ,   -sinp,  0,  cosp, 0
                ,   0,  0,  0,  1
            };

            m_transform = construct<transform_matrix>(m_transform * r);

            return *this;
        }

        transformer3& rotate_z(const units::angle& yaw)
        {
            using namespace geometrix;
            using std::cos;
            using std::sin;

            auto sinw = sin(yaw);
            auto cosw = cos(yaw);
            transform_matrix r =
            {
                    cosw,   -sinw,  0,  0
                ,   sinw,   cosw,   0,  0
                ,   0,  0,  1,  0
                ,   0,  0,  0,  1
            };

            m_transform = construct<transform_matrix>(m_transform * r);

            return *this;
        }

        template <typename Geometry>
        Geometry operator()(const Geometry& p) const
        {
            return transform(p, typename geometrix::geometry_tag_of<Geometry>::type());
        }

    private:

        template <typename Point>
        Point transform(const Point& p, geometrix::geometry_tags::point_tag) const
        {
            using namespace geometrix;
            return construct<Point>(m_transform * as_positional_homogeneous<typename arithmetic_type_of<Point>::type>(p));
        }

        template <typename Vector>
        Vector transform(const Vector& v, geometrix::geometry_tags::vector_tag) const
        {
            using namespace geometrix;
            return construct<Vector>(m_transform * as_vectoral_homogeneous<typename arithmetic_type_of<Vector>::type>(v));
        }

        template <typename Segment>
        Segment transform(const Segment& s, geometrix::geometry_tags::segment_tag) const
        {
            using namespace geometrix;
            return construct<Segment>(transform(get_start(s), geometry_tags::point_tag()), transform(get_end(s), geometry_tags::point_tag()));
        }

        template <typename Polyline>
        Polyline transform(const Polyline& p, geometrix::geometry_tags::polyline_tag) const
        {
            using namespace geometrix;
            using namespace boost::adaptors;
            using point_t = typename point_sequence_traits<Polyline>::point_type;

            Polyline result;
            boost::copy(p | transformed([this](const point_t& p) { return transform(p, geometry_tags::point_tag()); }), std::back_inserter(result));

            return result;
        }

        template <typename Polygon>
        Polygon transform(const Polygon& p, geometrix::geometry_tags::polygon_tag) const
        {
            using namespace geometrix;
            using namespace boost::adaptors;
            using point_t = typename point_sequence_traits<Polygon>::point_type;

            Polygon result;
            boost::copy(p | transformed([this](const point_t& p) { return transform(p, geometry_tags::point_tag()); }), std::back_inserter(result));

            return result;
        }

        transform_matrix m_transform;

    };

	struct premultiplication_policy
	{
		template <typename A, typename B>
		void operator()(A& a, const B& b) const
		{
			a = geometrix::construct<A>(b * a);
		}
	};

	struct postmultiplication_policy
	{
		template <typename A, typename B>
		void operator()(A& a, const B& b) const
		{
			a = geometrix::construct<A>(a * b);
		}
	};

	template <unsigned int D>
	class transformer_base
	{
	protected:
		BOOST_STATIC_CONSTANT(unsigned int, dimensionality = D + 1);
		using element_type = double;
		using transform_matrix = geometrix::matrix<element_type, dimensionality, dimensionality>;

		transform_matrix m_transform;

	public:

		transformer_base()
		{
			reset();
		}

		template <typename Matrix>
		transformer_base(const Matrix& m)
			: m_transform(geometrix::construct<transform_matrix>(m))
		{}

		void reset()
		{
			detail::identity_setter<dimensionality * dimensionality - 1, dimensionality>::apply(m_transform);
		}
		
	};

	template <unsigned int D, typename MultiplicationPolicy>
	struct transformer_operation_layer {};

	template <typename MultiplicationPolicy>
	struct transformer_operation_layer<2, MultiplicationPolicy> : transformer_base<2>
	{
		transformer_operation_layer() = default;

		template <typename Matrix>
		transformer_operation_layer(const Matrix& m)
			: transformer_base(m)
		{}

		transformer_operation_layer& translate(const vector2& v)
		{
			using namespace geometrix;

			transform_matrix m =
			{
				1.0, 0.0, get<0>(v).value()
				, 0.0, 1.0, get<1>(v).value()
				, 0.0, 0.0, 1.0
			};

			MultiplicationPolicy()(m_transform, m);

			return *this;
		}
		
		transformer_operation_layer& rotate(const units::angle& theta)
		{
			using namespace geometrix;
			using std::cos;
			using std::sin;

			auto sint = sin(theta);
			auto cost = cos(theta);
			transform_matrix r =
			{
				cost, -sint, 0.0
				, sint, cost, 0.0
				, 0.0, 0.0, 1.0
			};

			MultiplicationPolicy()(m_transform, r);

			return *this;
		}

		transformer_operation_layer& rotate(const dimensionless2& a, const dimensionless2& b)
		{
			using namespace geometrix;

			auto sint = exterior_product_area(a, b);
			auto cost = dot_product(a, b);
			transform_matrix r =
			{
				cost, -sint, 0.0
				, sint, cost, 0.0
				, 0.0, 0.0, 1.0
			};

			MultiplicationPolicy()(m_transform, r);

			return *this;
		}
	};

	template <typename MultiplicationPolicy>
	struct transformer_operation_layer<3, MultiplicationPolicy> : transformer_base<3>
	{
		transformer_operation_layer() = default;

		template <typename Matrix>
		transformer_operation_layer(const Matrix& m)
			: transformer_base(m)
		{}

		transformer_operation_layer& translate(const vector3& v)
		{
			using namespace geometrix;

			transform_matrix m =
			{
				1.0, 0.0, 0.0, get<0>(v).value()
				, 0.0, 1.0, 0.0, get<1>(v).value()
				, 0.0, 0.0, 1.0, get<2>(v).value()
				, 0.0, 0.0, 0.0, 1.0
			};

			//m_transform = construct<transform_matrix>(m_transform * m);
			MultiplicationPolicy()(m_transform, m);

			return *this;
		}

		transformer_operation_layer& rotate_x(const units::angle& roll)
		{
			using namespace geometrix;
			using std::cos;
			using std::sin;

			auto sinr = sin(roll);
			auto cosr = cos(roll);

			transform_matrix r =
			{
				1,  0,  0,  0
				,   0,  cosr,   -sinr,  0
				,   0,  sinr,   cosr, 0
				,   0,  0,  0,  1
			};

			//m_transform = construct<transform_matrix>(m_transform * r);
			MultiplicationPolicy()(m_transform, r);

			return *this;
		}

		transformer_operation_layer& rotate_y(const units::angle& pitch)
		{
			using namespace geometrix;
			using std::cos;
			using std::sin;
			auto sinp = sin(pitch);
			auto cosp = cos(pitch);
			transform_matrix r =
			{
				cosp,   0,  sinp,   0
				,   0,  1,  0,  0
				,   -sinp,  0,  cosp, 0
				,   0,  0,  0,  1
			};

			//m_transform = construct<transform_matrix>(m_transform * r);
			MultiplicationPolicy()(m_transform, r);

			return *this;
		}

		transformer_operation_layer& rotate_z(const units::angle& yaw)
		{
			using namespace geometrix;
			using std::cos;
			using std::sin;

			auto sinw = sin(yaw);
			auto cosw = cos(yaw);
			transform_matrix r =
			{
				cosw,   -sinw,  0,  0
				,   sinw,   cosw,   0,  0
				,   0,  0,  1,  0
				,   0,  0,  0,  1
			};

			//m_transform = construct<transform_matrix>(m_transform * r);
			MultiplicationPolicy()(m_transform, r);

			return *this;
		}
	};

	template <unsigned int D, typename MultiplicationPolicy = postmultiplication_policy>
	class transformer : public transformer_operation_layer<D, MultiplicationPolicy>
	{

	public:

		transformer() = default;

		template <typename Matrix>
		transformer(const Matrix& m)
			: transformer_operation_layer(m)
		{}

		template <typename Geometry>
		Geometry operator()(const Geometry& p) const
		{
			using namespace geometrix;
			static_assert(dimension_of<Geometry>::value == D, "transform called with incompatible geometry dimensions");
			return transform(p, typename geometrix::geometry_tag_of<Geometry>::type());
		}

	private:

		template <typename Point>
		Point transform(const Point& p, geometrix::geometry_tags::point_tag) const
		{
			using namespace geometrix;
			return construct<Point>(m_transform * as_positional_homogeneous<typename arithmetic_type_of<Point>::type>(p));
		}

		template <typename Vector>
		Vector transform(const Vector& v, geometrix::geometry_tags::vector_tag) const
		{
			using namespace geometrix;
			return construct<Vector>(m_transform * as_vectoral_homogeneous<typename arithmetic_type_of<Vector>::type>(v));
		}

		template <typename Segment>
		Segment transform(const Segment& s, geometrix::geometry_tags::segment_tag) const
		{
			using namespace geometrix;
			return construct<Segment>(transform(get_start(s), geometry_tags::point_tag()), transform(get_end(s), geometry_tags::point_tag()));
		}

		template <typename Polyline>
		Polyline transform(const Polyline& p, geometrix::geometry_tags::polyline_tag) const
		{
			using namespace geometrix;
			using namespace boost::adaptors;
			using point_t = typename point_sequence_traits<Polyline>::point_type;

			Polyline result;
			boost::copy(p | transformed([this](const point_t& p) { return transform(p, geometry_tags::point_tag()); }), std::back_inserter(result));

			return result;
		}

		template <typename Polygon>
		Polygon transform(const Polygon& p, geometrix::geometry_tags::polygon_tag) const
		{
			using namespace geometrix;
			using namespace boost::adaptors;
			using point_t = typename point_sequence_traits<Polygon>::point_type;

			Polygon result;
			boost::copy(p | transformed([this](const point_t& p) { return transform(p, geometry_tags::point_tag()); }), std::back_inserter(result));

			return result;
		}
	};
}//! namespace stk;

#endif//! STK_GEOMETRY_TRANSFORMER_HPP
