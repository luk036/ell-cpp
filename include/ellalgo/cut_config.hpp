#pragma once

#include <cstddef>

enum class CutStatus { Success, NoSoln, SmallEnough, NoEffect };

/**
 * @brief Options
 *
 */
struct Options {
  unsigned int max_iter = 2000; //!< maximum number of iterations
  double tol = 1e-8;            //!< error tolerance
};

/**
 * @brief CInfo
 *
 */
struct CInfo {
  bool feasible;
  size_t num_iters;
  CutStatus status;
};