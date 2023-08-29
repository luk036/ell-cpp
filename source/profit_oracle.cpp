#include <ellalgo/oracles/profit_oracle.hpp>
#include <type_traits>  // for move

using Vec = std::valarray<double>;
using Cut = std::pair<Vec, double>;

/**
 * The function assess_optim assesses the optimality of a given solution based on certain conditions
 * and returns a tuple containing a cut and a boolean value.
 *
 * @param[in] y The parameter `y` is a vector of values. It is used to calculate various values in
 * the function. The specific meaning of each element in the vector depends on the context and the
 * specific implementation of the `ProfitOracle` class.
 * @param[in,out] target The `target` parameter is a reference to a `double` variable. It is used to
 * store the best-so-far value for the optimization process. The function `assess_optim` assesses
 * the optimality of a given solution and updates the `target` value if necessary.
 *
 * @return The function `assess_optim` returns a tuple containing two elements. The first element is
 * of type `Cut`, which is a struct or class that contains a vector `g` and a double `fj`. The
 * second element is of type `bool`.
 */
auto ProfitOracle::assess_optim(const Vec &y, double &target) const -> std::tuple<Cut, bool> {
    // y0 <= log k
    const auto f1 = y[0] - this->_log_k;
    if (f1 > 0.0) {
        return {{Vec{1.0, 0.0}, f1}, false};
    }

    const auto log_Cobb
        = this->_log_pA + this->_elasticities[0] * y[0] + this->_elasticities[1] * y[1];
    const Vec x = std::exp(y);
    const auto vx = this->_price_out[0] * x[0] + this->_price_out[1] * x[1];
    auto te = target + vx;

    auto fj = std::log(te) - log_Cobb;
    if (fj < 0.0) {
        te = std::exp(log_Cobb);
        target = te - vx;
        Vec g = (this->_price_out * x) / te - this->_elasticities;
        return {{std::move(g), 0.0}, true};
    }
    Vec g = (this->_price_out * x) / te - this->_elasticities;
    return {{std::move(g), fj}, false};
}

/**
 * The function assess_optim_q assesses the optimization of a given target value using a set of
 * input parameters.
 *
 * @param[in] y A vector containing the input values.
 * @param[in,out] target The "target" parameter is a reference to a double value. It is used to
 * store the best-so-far value for optimization.
 * @param retry A boolean flag indicating whether the function should retry the assessment or not.
 *
 * @return The function `assess_optim_q` returns a tuple containing the following elements:
 */
auto ProfitOracleQ::assess_optim_q(const Vec &y, double &target, bool retry)
    -> std::tuple<Cut, bool, Vec, bool> {
    if (!retry) {
        Vec x = std::exp(y);
        x = x.apply([](double n) { return std::round(n); });
        if (x[0] == 0.0) {
            x[0] = 1.0;  // nearest integer than 0
        }
        if (x[1] == 0.0) {
            x[1] = 1.0;
        }
        this->_yd = std::log(x);
    }
    auto result1 = this->_P.assess_optim(this->_yd, target);
    auto &cut = std::get<0>(result1);
    auto &shrunk = std::get<1>(result1);
    auto &g = std::get<0>(cut);
    auto &h = std::get<1>(cut);
    auto d = this->_yd - y;
    h += g[0] * d[0] + g[1] * d[1];
    return {std::move(cut), shrunk, this->_yd, false};
}
