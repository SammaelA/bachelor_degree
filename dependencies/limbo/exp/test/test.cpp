// please see the explanation in the documentation
// http://www.resibots.eu/limbo

#include <iostream>

// you can also include <limbo/limbo.hpp> but it will slow down the compilation
#include <limbo/bayes_opt/boptimizer.hpp>

using namespace limbo;

struct Params {
    struct bayes_opt_boptimizer : public defaults::bayes_opt_boptimizer {
        
    };

// depending on which internal optimizer we use, we need to import different parameters
#ifdef USE_NLOPT
    struct opt_nloptnograd : public defaults::opt_nloptnograd {
    };
#elif defined(USE_LIBCMAES)
    struct opt_cmaes : public defaults::opt_cmaes {
    };
#else
    struct opt_gridsearch : public defaults::opt_gridsearch {
    };
#endif

    struct kernel : public defaults::kernel {
        BO_PARAM(double, noise, 0.001);
    };

    struct bayes_opt_bobase : public defaults::bayes_opt_bobase {
        
    };

    struct kernel_maternfivehalves : public defaults::kernel_maternfivehalves {
    };

    struct init_randomsampling : public defaults::init_randomsampling {
        
    };

    struct stop_maxiterations : public defaults::stop_maxiterations {
        
    };

    // we use the default parameters for acqui_ucb
    struct acqui_ucb : public defaults::acqui_ucb {
    };
};

struct Eval {
    // number of input dimension (x.size())
    BO_PARAM(size_t, dim_in, 1);
    // number of dimensions of the result (res.size())
    BO_PARAM(size_t, dim_out, 1);

    // the function to be optimized
    Eigen::VectorXd operator()(const Eigen::VectorXd& x) const
    {
        double y = 0;

            // YOUR CODE HERE
            y = sin(x[0])+ cos(2*x[0]) + x[0]*x[0]*x[0]*x[0];
            // return a 1-dimensional vector
        return tools::make_vector(y);
    }
};

int main()
{
    // we use the default acquisition function / model / stat / etc.
    bayes_opt::BOptimizer<Params> boptimizer;
    // run the evaluation
    boptimizer.optimize(Eval());
    // the best sample found
    std::cout << "Best sample: " << boptimizer.best_sample()(0) << " - Best observation: " << boptimizer.best_observation()(0) << std::endl;
    return 0;
}