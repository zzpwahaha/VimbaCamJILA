#pragma once
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_cdf.h>

#include <QVector>

#include <algorithm>
#include <tuple>

class Gaussian1DFit
{
    struct data
    {
        double* x;
        double* y;
        size_t n;
    };

    enum
    {
        numOfPara = 4
    };
    /* model function: a * exp( -1/2 * [ (t - b) / c ]^2 ) + d */
private:
    gsl_vector*                      p0; /* initial fitting parameters:amplitude,center,width,offset*/
    gsl_vector*                      p; /* fitting parameters */
    gsl_vector*                      f; /* residual yi-y(xi) */
    gsl_matrix*                      covar; /*covariance matrix, not yet multiplied by sigma, so is not variance-covariance matrix*/
    
    gsl_multifit_nlinear_fdf         fdf;
    gsl_multifit_nlinear_parameters  fdf_params;
    gsl_multifit_nlinear_workspace*  work;

    data                             fit_data;

    size_t                           max_iter;
    double                           ptol; /* tolerance on fitting parameter p */
    double                           gtol; /* tolerance on gradient */
    double                           ftol;

    int                              info;/*fiting stop reason, for debug*/
    double                           rss; /*mean of sum of residual square, dof corrected*/
    gsl_vector*                      confid95;

public:
    Gaussian1DFit() {};
    Gaussian1DFit(size_t n, double* datax, double* datay,
        double a0, double b0, double c0, double d0,
        size_t max_iter = 200, double ptol = 1.0e-8, double gtol = 1.0e-8, double ftol = 1.0e-8);
    ~Gaussian1DFit();
    void solve_system();
    void set_data(double* datay);
    void set_data(size_t n, double* datax, double* datay, bool free = true);
    void set_initialP(double a0, double b0, double c0, double d0);

    static double gaussian(const double a, const double b, const double c, const double d, const double t);
    QVector<double> calcFittedGaussian();


    const QVector<double> fittedPara() const;
    const QVector<double> confidence95Interval() const;

private:
    static int func_f (const gsl_vector* p, void* datafit, gsl_vector* f);
    static int func_df(const gsl_vector* p, void* datafit, gsl_matrix* J);
    static int func_fvv(const gsl_vector* p, const gsl_vector* v,
        void* datafit, gsl_vector* fvv);
    

};

