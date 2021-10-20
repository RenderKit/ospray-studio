// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/traits/rktraits.h"
#include <vector>
#include <memory>

namespace ospray {
  namespace sg {

  struct Node;

  using NodePtr = std::shared_ptr<Node>;

  // Data to track during traversal ///////////////////////////////////////////

  struct TraversalContext
  {
    std::string name;
    int level{0};
  };

  // Base node visitor interface //////////////////////////////////////////////

  struct Visitor
  {
    // NOTE: return value means "continue traversal"
    // functor called before children
    virtual bool operator()(Node &node, TraversalContext &ctx) = 0;

    // postChildren called after children have been traversed
    virtual void postChildren(Node &, TraversalContext &);

    virtual ~Visitor() = default;
  };

  inline void Visitor::postChildren(Node &, TraversalContext &) {}

  // Visitor type traits //////////////////////////////////////////////////////

  // NOTE(jda) - This checks at compile time if T implements the method
  //            'bool T::visit(Node &node, TraversalContext &ctx)'.
//#ifdef _WIN32
#if 1
  template <typename T>
  using has_valid_visit_operator_method =
      rkcommon::traits::has_operator_method<T>;
#else
  template <typename T>
  struct has_valid_visit_operator_method
  {
    using TASK_T = typename std::decay<T>::type;

    template <typename P1, typename P2>
    using t_param = bool (TASK_T::*)(P1 &, P2 &);
    template <typename P1, typename P2>
    using t_param_const = bool (TASK_T::*)(P1 &, P2 &) const;
    using operator_t    = decltype(&TASK_T::operator());

    using params_are_valid =
        std::is_same<t_param<Node, TraversalContext>, operator_t>;

    using params_are_valid_and_is_const =
        std::is_same<t_param_const<Node, TraversalContext>, operator_t>;

    static const bool value =
        traits::has_operator_method<T>::value &&
        (params_are_valid::value || params_are_valid_and_is_const::value);
  };
#endif

  template <typename VISITOR_T>
  struct is_valid_visitor
  {
    using BASIC_VISITOR_T = typename std::decay<VISITOR_T>::type;

    static const bool value =
        (std::is_base_of<Visitor, BASIC_VISITOR_T>::value ||
         has_valid_visit_operator_method<VISITOR_T>::value);
  };

  template <typename VISITOR_T>
  using is_valid_visitor_t =
      rkcommon::traits::enable_if_t<is_valid_visitor<VISITOR_T>::value>;

  }  // namespace sg
} // namespace ospray
