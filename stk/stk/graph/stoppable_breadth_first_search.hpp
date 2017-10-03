//
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//! Copyright � 2017 Brandon Kohn with ideas from StackOverflow.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef STK_GRAPH_BREADTH_FIRST_SEARCH_HPP
#define STK_GRAPH_BREADTH_FIRST_SEARCH_HPP

/*
  Breadth First Search Algorithm (Cormen, Leiserson, and Rivest p. 470)
*/
#include <boost/graph/breadth_first_search.hpp>

namespace boost {

  template <class Visitor, class Graph>
  struct StoppableBFSVisitorConcept {
    void constraints() {
      BOOST_CONCEPT_ASSERT(( CopyConstructibleConcept<Visitor> ));
      vis.initialize_vertex(u, g);
      vis.discover_vertex(u, g);
      vis.examine_vertex(u, g);
      vis.examine_edge(e, g);
      vis.tree_edge(e, g);
      vis.non_tree_edge(e, g);
      vis.gray_target(e, g);
      vis.black_target(e, g);
      vis.finish_vertex(u, g);
      bool result = vis.should_stop(u, g);
    }
    Visitor vis;
    Graph g;
    typename graph_traits<Graph>::vertex_descriptor u;
    typename graph_traits<Graph>::edge_descriptor e;
  };

  // Multiple-source version
  template <class IncidenceGraph, class Buffer, class BFSVisitor,
            class ColorMap, class SourceIterator>
  void stoppable_breadth_first_visit
    (const IncidenceGraph& g,
     SourceIterator sources_begin, SourceIterator sources_end,
     Buffer& Q, BFSVisitor vis, ColorMap color)
  {
    BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<IncidenceGraph> ));
    typedef graph_traits<IncidenceGraph> GTraits;
    typedef typename GTraits::vertex_descriptor Vertex;
    BOOST_CONCEPT_ASSERT(( BFSVisitorConcept<BFSVisitor, IncidenceGraph> ));
    BOOST_CONCEPT_ASSERT(( ReadWritePropertyMapConcept<ColorMap, Vertex> ));
    typedef typename property_traits<ColorMap>::value_type ColorValue;
    typedef color_traits<ColorValue> Color;
    typename GTraits::out_edge_iterator ei, ei_end;

    for (; sources_begin != sources_end; ++sources_begin) {
      Vertex s = *sources_begin;
      put(color, s, Color::gray());           vis.discover_vertex(s, g);
      Q.push(s);
    }
    while (! Q.empty()) {
      Vertex u = Q.top(); Q.pop();
      if (vis.should_stop(u, g))
          return;
      vis.examine_vertex(u, g);

      for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei) {
        Vertex v = target(*ei, g);            vis.examine_edge(*ei, g);
        ColorValue v_color = get(color, v);
        if (v_color == Color::white()) {      vis.tree_edge(*ei, g);
          put(color, v, Color::gray());       vis.discover_vertex(v, g);
          Q.push(v);
        } else {                              vis.non_tree_edge(*ei, g);
          if (v_color == Color::gray())       vis.gray_target(*ei, g);
          else                                vis.black_target(*ei, g);
        }
      } // end for
      put(color, u, Color::black());          vis.finish_vertex(u, g);
    } // end while
  } // stoppable_breadth_first_visit

  // Single-source version
  template <class IncidenceGraph, class Buffer, class BFSVisitor, class ColorMap>
  void stoppable_breadth_first_visit
    (const IncidenceGraph& g,
     typename graph_traits<IncidenceGraph>::vertex_descriptor s,
     Buffer& Q, BFSVisitor vis, ColorMap color)
  {
    typename graph_traits<IncidenceGraph>::vertex_descriptor sources[1] = {s};
    stoppable_breadth_first_visit(g, sources, sources + 1, Q, vis, color);
  }


  template <class VertexListGraph, class SourceIterator,
            class Buffer, class BFSVisitor,
            class ColorMap>
  void stoppable_breadth_first_search
    (const VertexListGraph& g,
     SourceIterator sources_begin, SourceIterator sources_end,
     Buffer& Q, BFSVisitor vis, ColorMap color)
  {
    // Initialization
    typedef typename property_traits<ColorMap>::value_type ColorValue;
    typedef color_traits<ColorValue> Color;
    typename boost::graph_traits<VertexListGraph>::vertex_iterator i, i_end;
    for (boost::tie(i, i_end) = vertices(g); i != i_end; ++i) {
      vis.initialize_vertex(*i, g);
      put(color, *i, Color::white());
    }
    stoppable_breadth_first_visit(g, sources_begin, sources_end, Q, vis, color);
  }

  template <class VertexListGraph, class Buffer, class BFSVisitor,
            class ColorMap>
  void stoppable_breadth_first_search
    (const VertexListGraph& g,
     typename graph_traits<VertexListGraph>::vertex_descriptor s,
     Buffer& Q, BFSVisitor vis, ColorMap color)
  {
    typename graph_traits<VertexListGraph>::vertex_descriptor sources[1] = {s};
    stoppable_breadth_first_search(g, sources, sources + 1, Q, vis, color);
  }

  namespace graph { struct stoppable_bfs_visitor_event_not_overridden {}; }

  struct on_should_stop {
      enum { num = detail::on_edge_not_minimized_num + 1 };
  };

  //========================================================================
  // The invoke_visitors() function

  namespace detail {
      template <class Visitor, class T, class Graph>
      inline bool invoke_should_stop_dispatch(Visitor& v, T x, Graph& g, mpl::true_)
      {
          return v(x, g);
      }

      template <class Visitor, class T, class Graph>
      inline bool invoke_should_stop_dispatch(Visitor&, T, Graph&, mpl::false_)
      {
          return false;
      }
  } // namespace detail

  template <class Visitor, class Rest, class T, class Graph>
  inline bool invoke_should_stop_visitors(std::pair<Visitor, Rest>& vlist, T x, Graph& g)
  {
      typedef typename Visitor::event_filter Category;
      typedef typename is_same<Category, ::boost::on_should_stop>::type IsSameTag;
      if (detail::invoke_should_stop_dispatch(vlist.first, x, g, IsSameTag()))
          return true;
      return invoke_should_stop_visitors(vlist.second, x, g);
  }
  template <class Visitor, class T, class Graph>
  inline bool invoke_should_stop_visitors(Visitor& v, T x, Graph& g)
  {
      typedef typename Visitor::event_filter Category;
      typedef typename is_same<Category, ::boost::on_should_stop>::type IsSameTag;
      return detail::invoke_should_stop_dispatch(v, x, g, IsSameTag());
  }

  template <class Visitors = null_visitor>
  class stoppable_bfs_visitor {
  public:
    stoppable_bfs_visitor() { }
    stoppable_bfs_visitor(Visitors vis) : m_vis(vis) { }

    template <class Vertex, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    initialize_vertex(Vertex u, Graph& g)
    {
      invoke_visitors(m_vis, u, g, ::boost::on_initialize_vertex());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Vertex, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    discover_vertex(Vertex u, Graph& g)
    {
      invoke_visitors(m_vis, u, g, ::boost::on_discover_vertex());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Vertex, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    examine_vertex(Vertex u, Graph& g)
    {
      invoke_visitors(m_vis, u, g, ::boost::on_examine_vertex());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Edge, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    examine_edge(Edge e, Graph& g)
    {
      invoke_visitors(m_vis, e, g, ::boost::on_examine_edge());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Edge, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    tree_edge(Edge e, Graph& g)
    {
      invoke_visitors(m_vis, e, g, ::boost::on_tree_edge());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Edge, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    non_tree_edge(Edge e, Graph& g)
    {
      invoke_visitors(m_vis, e, g, ::boost::on_non_tree_edge());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Edge, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    gray_target(Edge e, Graph& g)
    {
      invoke_visitors(m_vis, e, g, ::boost::on_gray_target());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Edge, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    black_target(Edge e, Graph& g)
    {
      invoke_visitors(m_vis, e, g, ::boost::on_black_target());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Vertex, class Graph>
    graph::stoppable_bfs_visitor_event_not_overridden
    finish_vertex(Vertex u, Graph& g)
    {
      invoke_visitors(m_vis, u, g, ::boost::on_finish_vertex());
      return graph::stoppable_bfs_visitor_event_not_overridden();
    }

    template <class Vertex, class Graph>
    bool should_stop(Vertex u, Graph& g)
    {
        return invoke_should_stop_visitors(m_vis, u, g);
    }

    BOOST_GRAPH_EVENT_STUB(on_initialize_vertex,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_discover_vertex,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_examine_vertex,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_examine_edge,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_tree_edge,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_non_tree_edge,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_gray_target,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_black_target,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_finish_vertex,stoppable_bfs)
    BOOST_GRAPH_EVENT_STUB(on_should_stop, stoppable_bfs)

  protected:
    Visitors m_vis;
  };
  template <class Visitors>
  stoppable_bfs_visitor<Visitors>
  make_stoppable_bfs_visitor(Visitors vis) {
    return stoppable_bfs_visitor<Visitors>(vis);
  }
  typedef stoppable_bfs_visitor<> default_stoppable_bfs_visitor;


  namespace detail {

    template <class VertexListGraph, class ColorMap, class BFSVisitor,
      class P, class T, class R>
    void stoppable_bfs_helper
      (VertexListGraph& g,
       typename graph_traits<VertexListGraph>::vertex_descriptor s,
       ColorMap color,
       BFSVisitor vis,
       const bgl_named_params<P, T, R>& params,
       boost::mpl::false_)
    {
      typedef graph_traits<VertexListGraph> Traits;
      // Buffer default
      typedef typename Traits::vertex_descriptor Vertex;
      typedef boost::queue<Vertex> queue_t;
      queue_t Q;
      stoppable_breadth_first_search
        (g, s,
         choose_param(get_param(params, buffer_param_t()), boost::ref(Q)).get(),
         vis, color);
    }

#ifdef BOOST_GRAPH_USE_MPI
    template <class DistributedGraph, class ColorMap, class BFSVisitor,
              class P, class T, class R>
    void stoppable_bfs_helper
      (DistributedGraph& g,
       typename graph_traits<DistributedGraph>::vertex_descriptor s,
       ColorMap color,
       BFSVisitor vis,
       const bgl_named_params<P, T, R>& params,
       boost::mpl::true_);
#endif // BOOST_GRAPH_USE_MPI

    //-------------------------------------------------------------------------
    // Choose between default color and color parameters. Using
    // function dispatching so that we don't require vertex index if
    // the color default is not being used.

    template <class ColorMap>
    struct stoppable_bfs_dispatch {
      template <class VertexListGraph, class P, class T, class R>
      static void apply
      (VertexListGraph& g,
       typename graph_traits<VertexListGraph>::vertex_descriptor s,
       const bgl_named_params<P, T, R>& params,
       ColorMap color)
      {
        stoppable_bfs_helper
          (g, s, color,
           choose_param(get_param(params, graph_visitor),
                        make_stoppable_bfs_visitor(null_visitor())),
           params,
           boost::mpl::bool_<
             boost::is_base_and_derived<
               distributed_graph_tag,
               typename graph_traits<VertexListGraph>::traversal_category>::value>());
      }
    };

    template <>
    struct stoppable_bfs_dispatch<param_not_found> {
      template <class VertexListGraph, class P, class T, class R>
      static void apply
      (VertexListGraph& g,
       typename graph_traits<VertexListGraph>::vertex_descriptor s,
       const bgl_named_params<P, T, R>& params,
       param_not_found)
      {
        null_visitor null_vis;

        stoppable_bfs_helper
          (g, s,
           make_two_bit_color_map
           (num_vertices(g),
            choose_const_pmap(get_param(params, vertex_index),
                              g, vertex_index)),
           choose_param(get_param(params, graph_visitor),
                        make_stoppable_bfs_visitor(null_vis)),
           params,
           boost::mpl::bool_<
             boost::is_base_and_derived<
               distributed_graph_tag,
               typename graph_traits<VertexListGraph>::traversal_category>::value>());
      }
    };

  } // namespace detail

#if 1
  // Named Parameter Variant
  template <class VertexListGraph, class P, class T, class R>
  void stoppable_breadth_first_search
    (const VertexListGraph& g,
     typename graph_traits<VertexListGraph>::vertex_descriptor s,
     const bgl_named_params<P, T, R>& params)
  {
    // The graph is passed by *const* reference so that graph adaptors
    // (temporaries) can be passed into this function. However, the
    // graph is not really const since we may write to property maps
    // of the graph.
    VertexListGraph& ng = const_cast<VertexListGraph&>(g);
    typedef typename get_param_type< vertex_color_t, bgl_named_params<P,T,R> >::type C;
    detail::stoppable_bfs_dispatch<C>::apply(ng, s, params,
                                   get_param(params, vertex_color));
  }
#endif


  // This version does not initialize colors, user has to.

  template <class IncidenceGraph, class P, class T, class R>
  void stoppable_breadth_first_visit
    (const IncidenceGraph& g,
     typename graph_traits<IncidenceGraph>::vertex_descriptor s,
     const bgl_named_params<P, T, R>& params)
  {
    // The graph is passed by *const* reference so that graph adaptors
    // (temporaries) can be passed into this function. However, the
    // graph is not really const since we may write to property maps
    // of the graph.
    IncidenceGraph& ng = const_cast<IncidenceGraph&>(g);

    typedef graph_traits<IncidenceGraph> Traits;
    // Buffer default
    typedef typename Traits::vertex_descriptor vertex_descriptor;
    typedef boost::queue<vertex_descriptor> queue_t;
    queue_t Q;

    stoppable_breadth_first_visit
      (ng, s,
       choose_param(get_param(params, buffer_param_t()), boost::ref(Q)).get(),
       choose_param(get_param(params, graph_visitor),
                    make_stoppable_bfs_visitor(null_visitor())),
       choose_pmap(get_param(params, vertex_color), ng, vertex_color)
       );
  }

  namespace graph {
    namespace detail {
      template <typename Graph, typename Source>
      struct stoppable_breadth_first_search_impl {
        typedef void result_type;
        template <typename ArgPack>
        void operator()(const Graph& g, const Source& source, const ArgPack& arg_pack) {
          using namespace boost::graph::keywords;
          typename boost::graph_traits<Graph>::vertex_descriptor sources[1] = {source};
          boost::queue<typename boost::graph_traits<Graph>::vertex_descriptor> Q;
          boost::stoppable_breadth_first_search(g,
                                      &sources[0],
                                      &sources[1],
                                      boost::unwrap_ref(arg_pack[_buffer | boost::ref(Q)]),
                                      arg_pack[_visitor | make_stoppable_bfs_visitor(null_visitor())],
                                      boost::detail::make_color_map_from_arg_pack(g, arg_pack));
        }
      };
    }
    BOOST_GRAPH_MAKE_FORWARDING_FUNCTION(stoppable_breadth_first_search, 2, 4)
  }

#if 0
  // Named Parameter Variant
  BOOST_GRAPH_MAKE_OLD_STYLE_PARAMETER_FUNCTION(stoppable_breadth_first_search, 2)
#endif

  template <typename Vertex, typename Tag>
  struct goal_reached_stopper : public boost::base_visitor<goal_reached_stopper<Vertex, Tag>>
  {
	  using event_filter = Tag;
	  goal_reached_stopper(Vertex goal)
		  : m_goal(goal)
	  {}

	  template <typename Graph>
	  bool operator()(Vertex e, const Graph&)
	  {
		  return e == m_goal;
	  }

	  Vertex m_goal;
  };

  template <typename Vertex, typename Tag>
  inline goal_reached_stopper<Vertex, Tag> stop_at_goal(Vertex goal, Tag)
  {
	  return goal_reached_stopper<Vertex, Tag>(goal);
  }

} // namespace boost

#endif // STK_GRAPH_BREADTH_FIRST_SEARCH_HPP
