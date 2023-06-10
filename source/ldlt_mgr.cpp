#include <ellalgo/oracles/ldlt_mgr.hpp>
#include <functional>

  auto LDLTMgr::factor(std::function<double(size_t, size_t)> get_matrix_elem) -> bool {
    this->p = {0U, 0U};
    // auto &[start, stop] = this->p;
    auto &start = this->p.first;
    auto &stop = this->p.second;

    for (auto i = 0U; i != this->_n; ++i) {
      // auto j = start;
      auto d = get_matrix_elem(i, start);
      for (auto j = start; j != i; ++j) {
        this->T(j, i) = d;
        this->T(i, j) = d / this->T(j, j); // note: T(j, i) here!
        auto s = j + 1;
        d = get_matrix_elem(i, s);
        for (auto k = start; k != s; ++k) {
          d -= this->T(i, k) * this->T(k, s);
        }
      }
      this->T(i, i) = d;

      if (d <= 0.0) {
        stop = i + 1;
        break;
      }
    }

    return this->is_spd();
  }

  auto LDLTMgr::factor_with_allow_semidefinte(std::function<double(size_t, size_t)> get_matrix_elem) -> bool {
    this->p = {0U, 0U};
    // auto &[start, stop] = this->p;
    auto &start = this->p.first;
    auto &stop = this->p.second;

    for (auto i = 0U; i != this->_n; ++i) {
      // auto j = start;
      auto d = get_matrix_elem(i, start);
      for (auto j = start; j != i; ++j) {
        this->T(j, i) = d;
        this->T(i, j) = d / this->T(j, j); // note: T(j, i) here!
        auto s = j + 1;
        d = get_matrix_elem(i, s);
        for (auto k = start; k != s; ++k) {
          d -= this->T(i, k) * this->T(k, s);
        }
      }
      this->T(i, i) = d;

      if (d < 0.0) {
        // this->stop = i + 1;
        stop = i + 1;
        break;
      }
      if (d == 0.0) {
        start = i + 1;
        // restart at i + 1, special as an LMI oracle
      }
    }
    return this->is_spd();
  }


  auto LDLTMgr::witness() -> double {
    assert(!this->is_spd());

    // const auto& [start, n] = this->p;
    const auto &start = this->p.first;
    const auto &n = this->p.second;
    auto m = n - 1; // assume stop > 0
    this->witness_vec[m] = 1.0;
    for (auto i = m; i > start; --i) {
      this->witness_vec[i - 1] = 0.0;
      for (auto k = i; k != n; ++k) {
        this->witness_vec[i - 1] -= this->T(k, i - 1) * this->witness_vec[k];
      }
    }
    return -this->T(m, m);
  }

