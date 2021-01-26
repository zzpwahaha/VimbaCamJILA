#include "Gaussian2DFit.h"
#include <iostream>
#include <qDebug>

Gaussian2DFit::Gaussian2DFit(
    size_t n, size_t width, size_t height, double* datax, double* datay, double* dataz,
    double A0, double x00, double y00, double a0, double b0, double c0, double D0,
    size_t max_iter, double ptol, double gtol, double ftol) :
    max_iter(max_iter), ptol(ptol), gtol(gtol), ftol(ftol),
    info(-1), rss(-1)
{
    fdf_params = gsl_multifit_nlinear_default_parameters();
    fdf_params.trs = gsl_multifit_nlinear_trs_lmaccel;
    //fdf_params.trs = gsl_multifit_nlinear_trs_lm;

    set_data(n, width, height, datax, datay, dataz, false);

    fdf.f = Gaussian2DFit::func_f;
    fdf.df = Gaussian2DFit::func_df;
    fdf.fvv = Gaussian2DFit::func_fvv;
    //fdf.fvv = NULL;
    fdf.n = n;
    fdf.p = numOfPara;
    fdf.params = &fit_data;

    covar = gsl_matrix_alloc(numOfPara, numOfPara);
    confid95 = gsl_vector_alloc(numOfPara);
    p0 = gsl_vector_alloc(numOfPara);
    jacEllipseMajorABC = gsl_vector_alloc(3);
    jacEllipseMinorABC = gsl_vector_alloc(3);

    set_initialP(A0, x00, y00, a0, b0, c0, D0);

    work = gsl_multifit_nlinear_alloc(
        gsl_multifit_nlinear_trust, &fdf_params, n, numOfPara);
    f = gsl_multifit_nlinear_residual(work);
    p = gsl_multifit_nlinear_position(work);
}

double Gaussian2DFit::gaussian2d(const double A, const double x0, const double y0,
    const double a, const double b, const double c, const double D,
    const double tx, const double ty)
{
    return A * exp( -a * (tx - x0) * (tx - x0) - c * (ty - y0) * (ty - y0) 
        - 2 * b * (tx - x0) * (ty - y0) ) + D;
}

QVector<double> Gaussian2DFit::calcFittedGaussian()
{
    QVector<double> tmp(fit_data.n, 0.0);
    const double A = gsl_vector_get(p, 0);
    const double x0 = gsl_vector_get(p, 1);
    const double y0 = gsl_vector_get(p, 2);
    const double a = gsl_vector_get(p, 3);
    const double b = gsl_vector_get(p, 4);
    const double c = gsl_vector_get(p, 5);
    const double D = gsl_vector_get(p, 6);
    size_t i = 0;
    std::for_each(tmp.begin(), tmp.end(), [this, i, A, x0, y0, a, b, c, D](auto& t1) mutable {
        t1 += gaussian2d(A, x0, y0, a, b, c, D,
            fit_data.x[i % fit_data.width], fit_data.y[i / fit_data.width]);
        i++; });

    return tmp;
}

const QVector<double> Gaussian2DFit::fittedPara() const
{
    return QVector<double>(p->data, p->data + p->size * p->stride);
}

const QVector<double> Gaussian2DFit::confidence95Interval() const
{
    return QVector<double>(confid95->data, confid95->data + confid95->size * confid95->stride);
}

const std::pair<double, double> Gaussian2DFit::MajorMinor95() const
{
    return std::make_pair(ellipseMajor95, ellipseMinor95);
}

int Gaussian2DFit::func_f(const gsl_vector* p, void* datafit, gsl_vector* f)
{
    data* dataf = static_cast<data*>(datafit);
    const double& A  = gsl_vector_get(p, 0);
    const double& x0 = gsl_vector_get(p, 1);
    const double& y0 = gsl_vector_get(p, 2);
    const double& a  = gsl_vector_get(p, 3);
    const double& b  = gsl_vector_get(p, 4);
    const double& c  = gsl_vector_get(p, 5);
    const double& D  = gsl_vector_get(p, 6);

    for (size_t i = 0; i < dataf->height; ++i)
    {
        for (size_t j = 0; j < dataf->width; j++)
        {
            double z = gaussian2d(A, x0, y0, a, b, c, D,
                dataf->x[j], dataf->y[i]);
            gsl_vector_set(f, i * dataf->width + j, dataf->z[i * dataf->width + j] - z);
        }  
    }

    return GSL_SUCCESS;
}

int Gaussian2DFit::func_df(const gsl_vector* p, void* datafit, gsl_matrix* J)
{
    struct data* dataf = (struct data*)datafit;
    const double& A = gsl_vector_get(p, 0);
    const double& x0 = gsl_vector_get(p, 1);
    const double& y0 = gsl_vector_get(p, 2);
    const double& a = gsl_vector_get(p, 3);
    const double& b = gsl_vector_get(p, 4);
    const double& c = gsl_vector_get(p, 5);
    const double& D = gsl_vector_get(p, 6);


    for (size_t i = 0; i < dataf->height; ++i)
    {
        for (size_t j = 0; j < dataf->width; j++)
        {
            const double deltax = dataf->x[j] - x0;
            const double deltay = dataf->y[i] - y0;
            const double ei = exp(-a * deltax * deltax - c * deltay * deltay - 2 * b * deltax * deltay);

            gsl_matrix_set(J, i * dataf->width + j, 0, -ei);
            gsl_matrix_set(J, i * dataf->width + j, 1, -2 * A * (a * deltax + b * deltay) * ei);
            gsl_matrix_set(J, i * dataf->width + j, 2, -2 * A * (b * deltax + c * deltay) * ei);
            gsl_matrix_set(J, i * dataf->width + j, 3, A * deltax * deltax * ei);
            gsl_matrix_set(J, i * dataf->width + j, 4, 2 * A * deltax * deltay * ei);
            gsl_matrix_set(J, i * dataf->width + j, 5, A * deltay * deltay * ei);
            gsl_matrix_set(J, i * dataf->width + j, 6, -1);
        }
    }

    return GSL_SUCCESS;
}

int Gaussian2DFit::func_fvv(const gsl_vector* p, const gsl_vector* v,
    void* datafit, gsl_vector* fvv)
{
    data* dataf = static_cast<data*>(datafit);
    const double& A = gsl_vector_get(p, 0);
    const double& x0 = gsl_vector_get(p, 1);
    const double& y0 = gsl_vector_get(p, 2);
    const double& a = gsl_vector_get(p, 3);
    const double& b = gsl_vector_get(p, 4);
    const double& c = gsl_vector_get(p, 5);
    const double& D = gsl_vector_get(p, 6);

    const double& vA = gsl_vector_get(v, 0);
    const double& vx0 = gsl_vector_get(v, 1);
    const double& vy0 = gsl_vector_get(v, 2);
    const double& va = gsl_vector_get(v, 3);
    const double& vb = gsl_vector_get(v, 4);
    const double& vc = gsl_vector_get(v, 5);
    const double& vD = gsl_vector_get(v, 6);
    
    //gsl_matrix* Dvv = gsl_matrix_alloc(numOfPara, numOfPara);

    for (size_t i = 0; i < dataf->height; ++i)
    {
        for (size_t j = 0; j < dataf->width; j++)
        {
            const double deltax = dataf->x[j] - x0;
            const double deltay = dataf->y[i] - y0;
            const double ei = exp(-a * deltax * deltax - c * deltay * deltay - 2. * b * deltax * deltay);
        
            double DA_x0 = (-2. * a * deltax - 2. * b * deltay) * ei;
            double DA_y0 = (-2. * b * deltax - 2. * c * deltay) * ei;
            double DA_a = deltax * deltax * ei;
            double DA_b = 2. * deltax * deltay * ei;
            double DA_c = deltay * deltay * ei;

            double Dx0_x0 = 2. * A * (a - 2. * a * a * deltax * deltax - 4. * a * b * deltax * deltay - 2. * b * b * deltay * deltay) * ei;
            double Dx0_y0 = 2. * A * (b - 2. * a * b * deltax * deltax - 2. * (b * b + a * c) * deltax * deltay - 2. * b * c * deltay * deltay) * ei;
            double Dx0_a = 2. * A * deltax * (-1. + a * deltax * deltax + b * deltax * deltay) * ei;
            double Dx0_b = 2. * A * deltay * (-1. + 2. * a * deltax * deltax + 2. * b * deltax * deltay) * ei;
            double Dx0_c = 2. * A * deltay * deltay * (a * deltax + b * deltay) * ei;

            double Dy0_y0 = 2. * A * (c - 2. * b * b * deltax * deltax - 4. * b * c * deltax * deltay - 2. * c * c * deltay * deltay) * ei;
            double Dy0_a = 2. * A * deltax * deltax * (b * deltax + c * deltay) * ei;
            double Dy0_b = 2. * A * deltax * (-1 + 2. * b * deltax * deltay + 2. * c * deltay * deltay) * ei;
            double Dy0_c = 2. * A * deltay * (-1 + b * deltax * deltay + c * deltay * deltay) * ei;

            double Da_a = -A * deltax * deltax * deltax * deltax * ei;
            double Da_b = -2. * A * deltax * deltax * deltax * deltay * ei;
            double Da_c = -A * deltax * deltax * deltay * deltay * ei;

            double Db_b = -4. * A * deltax * deltax * deltay * deltay * ei;
            double Db_c = -2. * A * deltax * deltay * deltay * deltay * ei;

            double Dc_c = -A * deltay * deltay * deltay * deltay * ei;
        
            double sum =
                2. * vA * vx0 * DA_x0 + 2. * vA * vy0 * DA_y0 + 2. * vA * va * DA_a + 2. * vA * vb * DA_b + 2. * vA * vc * DA_c +
                vx0 * vx0 * Dx0_x0 + 2. * vx0 * vy0 * Dx0_y0 + 2. * vx0 * va * Dx0_a + 2. * vx0 * vb * Dx0_b + 2. * vx0 * vc * Dx0_c +
                vy0 * vy0 * Dy0_y0 + 2. * vy0 * va * Dy0_a + 2. * vy0 * vb * Dy0_b + 2. * vy0 * vc * Dy0_c +
                va * va * Da_a + 2. * va * vb * Da_b + 2. * va * vc * Da_c +
                vb * vb * Db_b + 2. * vb * vc * Db_c +
                vc * vc * Dc_c;

            gsl_vector_set(fvv, i * dataf->width + j, sum);
        }
    }


    return GSL_SUCCESS;
}

void Gaussian2DFit::solve_system()
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
    
    /*calcualte confidence interval of all parameter*/
    auto covDiag = gsl_matrix_diagonal(covar);
    gsl_vector_memcpy(confid95, &covDiag.vector);
    double coef95 = gsl_cdf_tdist_Qinv(0.025, fit_data.n - numOfPara);
    std::for_each(confid95->data, confid95->data + confid95->size * confid95->stride,
        [this, coef95](auto& tmp) {tmp = coef95 * sqrt(rss * tmp); });

    /*calcualte confidence interval of transformed ellipse*/
    auto covABC = gsl_matrix_submatrix(covar, 3, 3, 3, 3);
    errorPropaABC(gsl_vector_get(p, 3), gsl_vector_get(p, 4), gsl_vector_get(p, 5), jacEllipseMajorABC, true);
    errorPropaABC(gsl_vector_get(p, 3), gsl_vector_get(p, 4), gsl_vector_get(p, 5), jacEllipseMinorABC, false);
    auto tmp = gsl_vector_alloc(3);
    //for (size_t i = 0; i < 3; i++)  qDebug() << jacEllipseMajorABC->data[i];
    //for (size_t i = 0; i < 9; i++)  qDebug() << gsl_matrix_get(&covABC.matrix,i/3,i%3);
    gsl_blas_dgemv(CBLAS_TRANSPOSE::CblasNoTrans, 1, &covABC.matrix, jacEllipseMajorABC, 0, tmp);
    //for (size_t i = 0; i < 3; i++)  qDebug() << tmp->data[i];
    gsl_blas_ddot(jacEllipseMajorABC, tmp, &ellipseMajor95);
    gsl_blas_dgemv(CBLAS_TRANSPOSE::CblasNoTrans, 1, &covABC.matrix, jacEllipseMinorABC, 0, tmp);
    gsl_blas_ddot(jacEllipseMinorABC, tmp, &ellipseMinor95);
    ellipseMajor95 = coef95 * sqrt(rss * ellipseMajor95);
    ellipseMinor95 = coef95 * sqrt(rss * ellipseMinor95);
    gsl_vector_free(tmp);


    //gsl_vector_memcpy(p, y);

    /* print summary for debug*/
    //fprintf(stderr, "NITER         = %zu\n", gsl_multifit_nlinear_niter(work));
    //fprintf(stderr, "NFEV          = %zu\n", fdf.nevalf);
    //fprintf(stderr, "NJEV          = %zu\n", fdf.nevaldf);
    //fprintf(stderr, "NAEV          = %zu\n", fdf.nevalfvv);
    //fprintf(stderr, "initial cost  = %.12e\n", rss0);
    //fprintf(stderr, "final cost    = %.12e\n", rss);
    //fprintf(stderr, "95%% confi x0   = %.12e\n",
    //    gsl_vector_get(confid95, 1));
    //fprintf(stderr, "95%% confi y0   = %.12e\n",
    //    gsl_vector_get(confid95, 2));
    //fprintf(stderr, "95%% confi a   = %.12e\n",
    //    gsl_vector_get(confid95, 3));
    //fprintf(stderr, "95%% confi b   = %.12e\n",
    //    gsl_vector_get(confid95, 4));
    //fprintf(stderr, "95%% confi c   = %.12e\n",
    //    gsl_vector_get(confid95, 5));
    //fprintf(stderr, "final x       = (%.12e, %.12e, %.12e,%.12e)\n",
    //    gsl_vector_get(p, 0), gsl_vector_get(p, 1), gsl_vector_get(p, 2), gsl_vector_get(p, 3));
    //fprintf(stderr, "final cond(J) = %.12e\n", 1.0 / rcond);
    //fprintf(stderr, "reason for stopping: %s\n",
    //    (info == 1) ? "small step size" : "small gradient");
    //fprintf(stderr, "t-distribution Qinv (0.025) : %.12e", gsl_cdf_tdist_Qinv(0.025, fit_data.n - numOfPara));

}

void Gaussian2DFit::errorPropaABC(double a, double b, double c, gsl_vector* ellipseJac, bool Major)
{
    if (Major)
    {
        double Delta = sqrt(4 * b * b + (a - c) * (a - c));
        double da = -(1. - (a - c) / Delta) / (2. * (pow(a + c - Delta, 1.5)));
        double db = 2 * b / (Delta * pow(a + c - Delta, 1.5));
        double dc = -(1. + (a - c) / Delta) / (2. * (pow(a + c - Delta, 1.5)));
        gsl_vector_set(ellipseJac, 0, da);
        gsl_vector_set(ellipseJac, 1, db);
        gsl_vector_set(ellipseJac, 2, dc);
    }
    else
    {
        double Delta = sqrt(4 * b * b + (a - c) * (a - c));
        double da = -(1. + (a - c) / Delta) / (2. * (pow(a + c + Delta, 1.5)));
        double db = -2 * b / (Delta * pow(a + c + Delta, 1.5));
        double dc = -(1. - (a - c) / Delta) / (2. * (pow(a + c + Delta, 1.5)));
        gsl_vector_set(ellipseJac, 0, da);
        gsl_vector_set(ellipseJac, 1, db);
        gsl_vector_set(ellipseJac, 2, dc);
    }

}


void Gaussian2DFit::set_data(double* dataz)
{
    fit_data.z = dataz;
}

void Gaussian2DFit::set_data(size_t n, size_t width, size_t height, 
    double* datax, double* datay, double* dataz, bool free)
{
    fit_data.n = n;
    fit_data.x = datax;
    fit_data.y = datay;
    fit_data.z = dataz;
    fit_data.width = width;
    fit_data.height = height;

    fdf.n = n;
    if (free) { gsl_multifit_nlinear_free(work); }
    work = gsl_multifit_nlinear_alloc(
        gsl_multifit_nlinear_trust, &fdf_params, n, numOfPara);
    f = gsl_multifit_nlinear_residual(work);
    p = gsl_multifit_nlinear_position(work);
}

void Gaussian2DFit::set_initialP(double A0, double x00, double y00, 
    double a0, double b0, double c0, double D0)
{
    gsl_vector_set(p0, 0, A0);
    gsl_vector_set(p0, 1, x00);
    gsl_vector_set(p0, 2, y00);
    gsl_vector_set(p0, 3, a0);
    gsl_vector_set(p0, 4, b0);
    gsl_vector_set(p0, 5, c0);
    gsl_vector_set(p0, 6, D0);
}

Gaussian2DFit::~Gaussian2DFit()
{
    gsl_multifit_nlinear_free(work);
    gsl_vector_free(p0);
    gsl_vector_free(confid95);
    gsl_vector_free(jacEllipseMajorABC);
    gsl_vector_free(jacEllipseMinorABC);
    gsl_matrix_free(covar);
}