#pragma once
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_cdf.h>

#include <QVector>

#include <algorithm>
#include <tuple>

class Gaussian2DFit
{
    struct data
    {
        double* x;
        double* y;
        size_t n;

        double* z;
        size_t width;
        size_t height;
    };

    enum
    {
        numOfPara = 7
    };
    /* model function: A *exp(-a*(x-x0)^2 - 2*b*(x-x0)*(y-y0) - c*(y-y0)^2) + d, 
    see more detail https://en.wikipedia.org/wiki/Gaussian_function#Two-dimensional_Gaussian_function*/
private:
    gsl_vector* p0; /* initial fitting parameters:amplitude,center(x0,y0),width(a,b,c),offset*/
    gsl_vector* p; /* fitting parameters */
    gsl_vector* f; /* residual yi-y(xi) */
    gsl_matrix* covar; /*covariance matrix, not yet multiplied by sigma, so is not variance-covariance matrix*/

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
    gsl_vector*                      jacEllipseMajorABC;
    gsl_vector*                      jacEllipseMinorABC;
    double                           ellipseMajor95;
    double                           ellipseMinor95;

public:
    Gaussian2DFit() {};
    Gaussian2DFit(size_t n, size_t width, size_t height, double* datax, double* datay, double* dataz,
        double A0, double x00, double y00, double a0, double b0, double c0, double D0, 
        size_t max_iter = 200, double ptol = 1.0e-8, double gtol = 1.0e-8, double ftol = 1.0e-8);
    ~Gaussian2DFit();
    void solve_system();
    void set_data(double* datay);
    void set_data(size_t n, size_t width, size_t height, double* datax, double* datay, double* dataz, bool free = true);
    void set_initialP(double A0, double x00, double y00, double a0, double b0, double c0, double D0);

    int getInfo() const { return info; }

    static double gaussian2d(const double A, const double x0, const double y0, 
        const double a, const double b, const double c, const double D, 
        const double tx, const double ty);
    QVector<double> calcFittedGaussian();


    const QVector<double> fittedPara() const;
    const QVector<double> confidence95Interval() const;
    const std::pair<double, double> MajorMinor95() const;

private:
    static int func_f(const gsl_vector* p, void* datafit, gsl_vector* f);
    static int func_df(const gsl_vector* p, void* datafit, gsl_matrix* J);
    static int func_fvv(const gsl_vector* p, const gsl_vector* v,
        void* datafit, gsl_vector* fvv);

    void inline errorPropaABC(double a, double b, double c, gsl_vector* ellipseJac, 
        bool Major = true);/*for calculating jac of the transformed gaussian arg, now is a classical form of ellipse*/
};

