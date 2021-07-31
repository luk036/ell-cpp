#include <cassert>
#include <ellalgo/oracles/qmi_oracle.hpp>
#include <xtensor/xview.hpp>

#define ROW(X, index) xt::view(X, index, xt::all())
#define COLUMN(X, index) xt::view(X, xt::all(), index)

using Arr = xt::xarray<double, xt::layout_type::row_major>;
using Cut = std::tuple<Arr, double>;

/*!
 * @brief
 *
 * @param[in] x
 * @return std::optional<Cut>
 */
std::optional<Cut> qmi_oracle::operator()(const Arr& x) {    
    this->_count = 0;
    this->_nx = x.shape()[0];

    auto getA = [&, this](size_t i, size_t j) -> double {  // ???
        assert(i >= j);
        if (this->_count < i + 1) {
            this->_count = i + 1;
            ROW(this->_Fx, i) = COLUMN(this->_F0, i);
            for (auto k = 0U; k != this->_nx; ++k) {
                ROW(this->_Fx, i) -= COLUMN(this->_F[k], i) * x(k);
            }
        }
        auto a = -xt::sum(ROW(this->_Fx, i) * ROW(this->_Fx, j))();
        if (i == j) {
            a += this->_t;
        }
        return a;
    };

    if (this->_Q.factor(getA)) {
        return {};
    }

    const auto ep = this->_Q.witness();
    const auto [start, stop] = this->_Q.p;
    const auto v = xt::view(this->_Q.witness_vec, xt::range(start, stop));
    const auto Fxp = ROW(this->_Fx, xt::range(start, stop));
    const auto Av = xt::sum(v * Fxp);
    auto g = zeros({this->_nx});
    for (auto k = 0U; k != this->_nx; ++k) {
        const auto Fkp = ROW(this->_F[k], xt::range(start, stop));
        g(k) = -2 * xt::sum(xt::sum(v * Fkp) * Av)();
    }
    return {{std::move(g), ep}};
}
