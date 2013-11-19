// elastic net coordinate descent method

#include "adp/rmatrix.h"
#include "adp/rvector.h"

#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>

extern "C"
{
    using adp::Rvector;
    using adp::Rmatrix;

    // ----------------------------------------------------------------------

    double soft_thresh (double z, double lambda)
    {
        if (z > 0 && z > lambda) return (z - lambda);
        if (z < 0 && -z > lambda) return (z + lambda);
        return 0;
    }

    SEXP elcd(SEXP rxx, SEXP rxy, SEXP rmx, SEXP rmy, SEXP rsx, SEXP rsy,
              SEXP ralpha, SEXP rlambda, SEXP rstandardize, SEXP ractive_set,
              SEXP rmaxit, SEXP rtol, SEXP rN, SEXP rcoef, SEXP riter,
              SEXP rloglik)
    {
        Rmatrix<double> xx(rxx);
        Rvector<double> xy(rxy);
        Rvector<double> mx(rmx);
        double my = *(REAL(rmy));
        Rvector<double> sx(rsx);
        double sy = *(REAL(rsy));
        Rvector<double> coef(rcoef);
        double alpha = *(REAL(ralpha));
        double lambda = *(REAL(rlambda));
        bool standardize = *(LOGICAL(rstandardize));
        bool active_set = *(LOGICAL(ractive_set));
        int maxit = *(INTEGER(rmaxit));
        double tol = *(REAL(rtol));
        int N = *(INTEGER(rN));
        double* iter = REAL(riter);
        double* loklik = REAL(rloglik);
        
        int n = coef.size() - 1;
        double* prev = new double[n];
        for (int i = 0; i < n; i++) prev[i] = coef(i);
        bool active_now = true;
        double al = alpha * lambda;
        double denom = lambda * (1 - alpha);

        int count = 0;
        do {
            for (int i = 0; i < n; i++) {
                if (active_set && active_now) continue;
                double sum = 0;
                for (int j = 0; j < n; j++)
                    if (coef(j) != 0) sum += xx(i,j) * coef(j);
                double z;
                if (standardize)
                    z = (xy(i) - sum) / N + coef(i);
                else
                    z = (xy(i) - coef(i)*mx(i) - sum) / N + coef(i);
                coef(i) = soft_thresh(z, al) / (xx(i,i)/N + denom);
            }
            count++;
            if (count > maxit) break;
            double diff = 0;
            for (int i = 0; i < n; i++)
                if (prev[i] != 0)
                    diff += fabs((prev[i] - coef(i)) / prev[i]);
                else
                    diff += fabs(prev[i] - coef(i));
            diff /= n;
            if (diff < tol) {
                if (active_now && active_set)
                    active_now = false;
                else
                    break;
            } else {
                if (! active_now) active_now = true;
            }
            if (!standardize) {
                coef(n) = my;
                for (int i = 0; i < n; i++)
                    coef(n) = coef(n) - mx(i) * coef(i);
            } 
            for (int i = 0; i < n; i++) prev[i] = coef(i);
        } while (true);

        // compute the log-likelihood
        *loglik = 0;
        if (standardize) {
            *loglik += 1;
            for (int i = 0; i < n; i++)
                *loglik += 2 * coef(i) * xy(i);
        } else {
            tmp_int = coef(n) - my;
            *loglik += sy * sy + 2 * tmp_int * my;
            for (int i = 0; i < n; i++)
                *loglik += 2 * coef(n) * coef(i) * mx
                    + 2 * coef(i) * xy(i);
        }
        for (int i = 0; i < n; i++)
            for (int j = i; j < n; j++)
                if (i == j)
                    *loglik += coef(i) * coef(j) * xx(i,i);
                else
                    *loglik += 2 * coef(i) * coef(j) * xx(i,j);
        *loglik /= N;
        for (int i = 0; i < n; i++)
            *loglik += denom * coef(i) * coef(i) / 2 + lambda * fabs(coef(i));
        
        // un-standardize if standardize = TRUE
        if (standardize) {
            coef(n) = my;
            for (int i = 0; i < n; i++) {
                coef(i) = coef(i) / sx(i);
                coef(n) = coef(n) - coef(i) * mx(i);
            }
        }

        *iter = count; // number of iterations

        delete [] prev;
        return R_NilValue;
    }
}

