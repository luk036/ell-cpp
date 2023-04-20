// -*- coding: utf-8 -*-
#pragma once

#include <cassert>
#include <cmath>
#include <tuple>
#include <type_traits>

#include "ell_config.hpp"
#include "half_nonnegative.hpp"

template <typename Space>
using CuttingPlaneArrayType =
    typename std::remove_reference<Space>::type::ArrayType;

/**
 * @brief Find a point in a convex set (defined through a cutting-plane oracle).
 *
 * A function f(x) is *convex* if there always exist a g(x)
 * such that f(z) >= f(x) + g(x)' * (z - x), forall z, x in dom f.
 * Note that dom f does not need to be a convex set in our definition.
 * The affine function g' (x - xc) + beta is called a cutting-plane,
 * or a ``cut'' for short.
 * This algorithm solves the following feasibility problem:
 *
 *   find x
 *   s.t. f(x) <= 0,
 *
 * A *separation oracle* asserts that an evalution point x0 is feasible,
 * or provide a cut that separates the feasible region and x0.
 *
 * @tparam Oracle
 * @tparam Space
 * @param[in,out] omega   perform assessment on x0
 * @param[in,out] space   search Space containing x*
 * @param[in]     options maximum iteration and error tolerance etc.
 * @return Information of Cutting-plane method
 */
template <typename Oracle, typename Space>
inline auto cutting_plane_feas(Oracle &&omega, Space &&space,
                               const Options &options = Options()) -> CInfo {
  for (auto niter = 0U; niter != options.max_iter; ++niter) {
    const auto cut = omega.assess_feas(space.xc());
    if (!cut) { // feasible sol'n obtained
      return {true, niter};
    }
    const auto cutstatus = space.update(*cut); // update space
    if (cutstatus != CutStatus::Success || space.tsq() < options.tol) {
      return {false, niter};
    }
  }
  return {false, options.max_iter};
}

/**
 * @brief Cutting-plane method for solving convex problem
 *
 * @tparam Oracle
 * @tparam Space
 * @tparam Num
 * @param[in,out] omega   perform assessment on x0
 * @param[in,out] space   search Space containing x*
 * @param[in,out] tea     best-so-far optimal sol'n
 * @param[in]     options maximum iteration and error tolerance etc.
 * @return Information of Cutting-plane method
 */
template <typename Oracle, typename Space, typename Num>
inline auto cutting_plane_optim(Oracle &&omega, Space &&space, Num &&tea,
                                const Options &options = Options())
    -> std::tuple<CuttingPlaneArrayType<Space>, size_t> {
  CuttingPlaneArrayType<Space> x_best{};
  for (auto niter = 0U; niter < options.max_iter; ++niter) {
    const auto __result1 = omega.assess_optim(space.xc(), tea);
    const auto &cut = std::get<0>(__result1);
    const auto &shrunk = std::get<1>(__result1);
    const auto cutstatus = [&]() {
      if (shrunk) { // best tea obtained
        x_best = space.xc();
        return space.update(cut); // should update_cc
      } else {
        return space.update(cut);
      }
    }();
    if (cutstatus != CutStatus::Success ||
        space.tsq() < options.tol) { // no more
      return {std::move(x_best), niter};
    }
  }
  return {std::move(x_best), options.max_iter};
} // END

/**
 * @brief Cutting-plane method for solving convex discrete optimization problem
 *
 * @tparam Oracle
 * @tparam Space
 * @param[in,out] omega   perform assessment on x0
 * @param[in,out] space   search Space containing x*
 * @param[in,out] tea  best-so-far optimal sol'n
 * @param[in]     options maximum iteration and error tolerance etc.
 * @return Information of Cutting-plane method
 */
template <typename Oracle, typename Space, typename Num>
inline auto cutting_plane_q(Oracle &&omega, Space &&space, Num &&tea,
                            const Options &options = Options())
    -> std::tuple<CuttingPlaneArrayType<Space>, size_t> {
  CuttingPlaneArrayType<Space> x_best{};
  auto status = CutStatus::NoSoln; // note!!!
  auto retry = (status == CutStatus::NoEffect);

  for (auto niter = 0U; niter < options.max_iter; ++niter) {
    // auto retry = (status == CutStatus::NoEffect);
    const auto result1 = omega.assess_q(space.xc(), tea, retry);
    const auto &cut = std::get<0>(result1);
    const auto &shrunk = std::get<1>(result1);
    const auto &x0 = std::get<2>(result1);
    const auto &more_alt = std::get<3>(result1);
    if (shrunk) { // best tea obtained
      // tea = t1;
      x_best = x0; // x0
    }
    const auto cutstatus = space.update(cut);

    if (cutstatus == CutStatus::NoEffect) {
      if (!more_alt) { // more alt?
        break;         // no more alternative cut
      }
      status = cutstatus;
      retry = true;
    } else if (cutstatus == CutStatus::NoSoln) {
      return {std::move(x_best), niter};
    }
    if (space.tsq() < options.tol) { // no more
      return {std::move(x_best), niter};
    }
  }
  return {std::move(x_best), options.max_iter};
} // END

/**
 * @brief
 *
 * @tparam Oracle
 * @tparam Space
 * @param[in,out] omega   perform assessment on x0
 * @param[in,out] intvl   interval containing x*
 * @param[in]     options maximum iteration and error tolerance etc.
 * @return CInfo
 */
template <typename Oracle, typename Space>
inline auto bsearch(Oracle &&omega, Space &&intvl,
                    const Options &options = Options()) -> CInfo {
  // assume monotone
  // auto& [lower, upper] = intvl;
  auto &lower = intvl.first;
  auto &upper = intvl.second;
  assert(lower <= upper);
  const auto u_orig = upper;

  for (auto niter = 0U; niter < options.max_iter; ++niter) {
    auto tau = algo::half_nonnegative(upper - lower);
    if (tau < options.tol) { // no more
      return {upper != u_orig, niter};
    }
    auto tea = lower; // l may be `int` or `Fraction`
    tea += tau;
    if (omega.assess_bs(tea)) { // feasible sol'n obtained
      upper = tea;
    } else {
      lower = tea;
    }
  }
  return {upper != u_orig, options.max_iter};
}

/**
 * @brief
 *
 * @tparam Oracle
 * @tparam Space
 */
template <typename Oracle, typename Space> //
class bsearch_adaptor {
  using ArrayType = typename Space::ArrayType;

private:
  Oracle &_omega;
  Space &_space;
  const Options _options;

public:
  /**
   * @brief Construct a new bsearch adaptor object
   *
   * @param[in,out] omega perform assessment on x0
   * @param[in,out] space search Space containing x*
   */
  bsearch_adaptor(Oracle &omega, Space &space)
      : bsearch_adaptor{omega, space, Options()} {}

  /**
   * @brief Construct a new bsearch adaptor object
   *
   * @param[in,out] omega   perform assessment on x0
   * @param[in,out] space   search space containing x*
   * @param[in]     options maximum iteration and error tolerance etc.
   */
  bsearch_adaptor(Oracle &omega, Space &space, const Options &options)
      : _omega{omega}, _space{space}, _options{options} {}

  /**
   * @brief get best x
   *
   * @return auto
   */
  auto x_best() const -> ArrayType { return this->_space.xc(); }

  /**
   * @brief
   *
   * @tparam Num
   * @param[in,out] tea the best-so-far optimal value
   * @return bool
   */
  template <typename Num> auto assess_bs(const Num &tea) -> bool {
    Space space = this->_space.copy();
    this->_omega.update(tea);
    const auto ell_info =
        cutting_plane_feas(this->_omega, space, this->_options);
    if (ell_info.feasible) {
      this->_space.set_xc(space.xc());
    }
    return ell_info.feasible;
  }
};
