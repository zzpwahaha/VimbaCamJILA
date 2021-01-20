#include "Gaussian1DFit.h"



Gaussian1DFit::Gaussian1DFit(size_t n, double* datax, double* datay,
    double a0, double b0, double c0, double d0,
    size_t max_iter, double ptol, double gtol, double ftol) :
    max_iter(max_iter), ptol(ptol), gtol(gtol), ftol(ftol),
    info(-1), rss(-1)
{
    fdf_params = gsl_multifit_nlinear_default_parameters();
    fdf_params.trs = gsl_multifit_nlinear_trs_lmaccel;

    set_data(n, datax, datay, false);

    fdf.f = Gaussian1DFit::func_f;
    fdf.df = Gaussian1DFit::func_df;
    fdf.fvv = Gaussian1DFit::func_fvv;
    fdf.n = n;
    fdf.p = numOfPara;
    fdf.params = &fit_data;

    covar = gsl_matrix_alloc(numOfPara, numOfPara);
    confid95 = gsl_vector_alloc(numOfPara);
    p0 = gsl_vector_alloc(numOfPara);
    set_initialP(a0, b0, c0, d0);

    work = gsl_multifit_nlinear_alloc(
        gsl_multifit_nlinear_trust, &fdf_params, n, numOfPara);
    f = gsl_multifit_nlinear_residual(work);
    p = gsl_multifit_nlinear_position(work);
}

double Gaussian1DFit::gaussian
(const double a, const double b, const double c, const double d, const double t)
{
    const double z = (t - b) / c;
    return (a * exp(-0.5 * z * z) + d);
}

QVector<double> Gaussian1DFit::calcFittedGaussian()
{
    QVector<double> tmp(fit_data.n, 0.0);
    const double a = gsl_vector_get(p, 0);
    const double b = gsl_vector_get(p, 1);
    const double c = gsl_vector_get(p, 2);
    const double d = gsl_vector_get(p, 3);
    size_t i = 0;
    std::for_each(tmp.begin(), tmp.end(), [this, i, a, b, c, d](auto& t1) mutable {
        t1 += gaussian(a, b, c, d, fit_data.x[i]);
        i++; });
    
    return tmp;
}

const QVector<double> Gaussian1DFit::fittedPara() const
{
    return QVector<double>(p->data, p->data + p->size * p->stride);
}

const QVector<double> Gaussian1DFit::confidence95Interval() const
{
    return QVector<double>(confid95->data, confid95->data + confid95->size * confid95->stride);
}

int Gaussian1DFit::func_f(const gsl_vector* p, void* datafit, gsl_vector* f)
{
    data* dataf = static_cast<data*>(datafit);
    const double& a = gsl_vector_get(p, 0);
    const double& b = gsl_vector_get(p, 1);
    const double& c = gsl_vector_get(p, 2);
    const double& d = gsl_vector_get(p, 3);

    for (size_t i = 0; i < dataf->n; ++i)
    {
        double y = gaussian(a, b, c, d, dataf->x[i]);

        gsl_vector_set(f, i, dataf->y[i] - y);
    }

    return GSL_SUCCESS;
}

int Gaussian1DFit::func_df(const gsl_vector* p, void* datafit, gsl_matrix* J)
{
    struct data* dataf = (struct data*)datafit;
    const double& a = gsl_vector_get(p, 0);
    const double& b = gsl_vector_get(p, 1);
    const double& c = gsl_vector_get(p, 2);
    const double& d = gsl_vector_get(p, 2);

    for (size_t i = 0; i < dataf->n; ++i)
    {
        const double tmp = (dataf->x[i] - b) / c;
        const double ei = exp(-0.5 * tmp * tmp);
        /*the minus sign comes from f=yi-gaussian(ti)*/
        gsl_matrix_set(J, i, 0, -ei);
        gsl_matrix_set(J, i, 1, -(a / c) * ei * tmp);
        gsl_matrix_set(J, i, 2, -(a / c) * ei * tmp * tmp);
        gsl_matrix_set(J, i, 3, -1);
    }

    return GSL_SUCCESS;
}

int Gaussian1DFit::func_fvv(const gsl_vector* p, const gsl_vector* v,
    void* datafit, gsl_vector* fvv)
{
    data* dataf = static_cast<data*>(datafit);
    const double& a = gsl_vector_get(p, 0);
    const double& b = gsl_vector_get(p, 1);
    const double& c = gsl_vector_get(p, 2);
    const double& d = gsl_vector_get(p, 3);
    const double& va = gsl_vector_get(v, 0);
    const double& vb = gsl_vector_get(v, 1);
    const double& vc = gsl_vector_get(v, 2);
    const double& vd = gsl_vector_get(v, 3);

    for (size_t i = 0; i < dataf->n; ++i)
    {
        double tmp = (dataf->x[i] - b) / c;
        double ei = exp(-0.5 * tmp * tmp);
        double Dab = -tmp * ei / c;
        double Dac = -tmp * tmp * ei / c;
        double Dbb = a * ei / (c * c) * (1.0 - tmp * tmp);
        double Dbc = a * tmp * ei / (c * c) * (2.0 - tmp * tmp);
        double Dcc = a * tmp * tmp * ei / (c * c) * (3.0 - tmp * tmp);
        double sum;

        sum = 2.0 * va * vb * Dab +
            2.0 * va * vc * Dac +
            vb * vb * Dbb +
            2.0 * vb * vc * Dbc +
            vc * vc * Dcc;

        gsl_vector_set(fvv, i, sum);
    }

    return GSL_SUCCESS;
}

void Gaussian1DFit::solve_system()
{
    double rss0, rcond;

    /* initialize solver */
    gsl_multifit_nlinear_init(p0, &fdf, work);

    /* store initial cost */
    gsl_blas_ddot(f, f, &rss0);
    rss0 *= 1.0 / static_cast<double>(fit_data.n - numOfPara);

    /* iterate until convergence */
    gsl_multifit_nlinear_driver(max_iter, ptol, gtol, ftol,
        NULL, NULL, &info, work);

    /* store final cost */
    gsl_blas_ddot(f, f, &rss);
    rss *= 1.0 / static_cast<double>(fit_data.n - numOfPara);
    /* store cond(J(x)) */
    gsl_multifit_nlinear_rcond(&rcond, work);

    /* compute covariance of best fit parameters */
    gsl_multifit_nlinear_covar(gsl_multifit_nlinear_jac(work), 0.0, covar);
    auto covDiag = gsl_matrix_diagonal(covar);
    gsl_vector_swap(confid95, &covDiag.vector);
    double coef95 = gsl_cdf_tdist_Qinv(0.025, fit_data.n - numOfPara);
    std::for_each(confid95->data, confid95->data + confid95->size * confid95->stride,
        [this, coef95](auto& tmp) {tmp = coef95 * sqrt(rss * tmp); });


    //gsl_vector_memcpy(p, y);

    /* print summary for debug*/
    //fprintf(stderr, "NITER         = %zu\n", gsl_multifit_nlinear_niter(work));
    //fprintf(stderr, "NFEV          = %zu\n", fdf.nevalf);
    //fprintf(stderr, "NJEV          = %zu\n", fdf.nevaldf);
    //fprintf(stderr, "NAEV          = %zu\n", fdf.nevalfvv);
    //fprintf(stderr, "initial cost  = %.12e\n", rss0 );
    //fprintf(stderr, "final cost    = %.12e\n", rss);
    //fprintf(stderr, "95%% confi a   = %.12e\n", 
    //    sqrt(rss) * sqrt(gsl_matrix_get(covar, 0, 0)));
    //fprintf(stderr, "95%% confi b   = %.12e\n",
    //    sqrt(rss) * sqrt(gsl_matrix_get(covar, 1, 1)));
    //fprintf(stderr, "95%% confi c   = %.12e\n",
    //    sqrt(rss) * sqrt(gsl_matrix_get(covar, 2, 2)));
    //fprintf(stderr, "95%% confi d   = %.12e\n",
    //    sqrt(rss) * sqrt(gsl_matrix_get(covar, 3, 3)));
    //fprintf(stderr, "final x       = (%.12e, %.12e, %.12e,%.12e)\n",
    //    gsl_vector_get(p, 0), gsl_vector_get(p, 1), gsl_vector_get(p, 2), gsl_vector_get(p, 3));
    //fprintf(stderr, "final cond(J) = %.12e\n", 1.0 / rcond);
    //fprintf(stderr, "reason for stopping: %s\n",
    //    (info == 1) ? "small step size" : "small gradient");
    //fprintf(stderr, "t-distribution Qinv (0.025) : %.12e", gsl_cdf_tdist_Qinv(0.025, fit_data.n - numOfPara));

}

void Gaussian1DFit::set_data(double* datay)
{
    fit_data.y = datay;
}

void Gaussian1DFit::set_data(size_t n, double* datax, double* datay, bool free)
{
    fit_data.n = n;
    fit_data.x = datax;
    fit_data.y = datay;

    fdf.n = n;
    if (free) { gsl_multifit_nlinear_free(work); }
    work = gsl_multifit_nlinear_alloc(
        gsl_multifit_nlinear_trust, &fdf_params, n, numOfPara);
    f = gsl_multifit_nlinear_residual(work);
    p = gsl_multifit_nlinear_position(work);
}

void Gaussian1DFit::set_initialP(double a0, double b0, double c0, double d0)
{
    gsl_vector_set(p0, 0, a0);
    gsl_vector_set(p0, 1, b0);
    gsl_vector_set(p0, 2, c0);
    gsl_vector_set(p0, 3, d0);
}

Gaussian1DFit::~Gaussian1DFit()
{
    gsl_multifit_nlinear_free(work);
    gsl_vector_free(p0);   
    gsl_matrix_free(covar);
}