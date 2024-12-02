#ifndef TILED_ALGORITHMS_GPU_H
#define TILED_ALGORITHMS_GPU_H

#include "gp_optimizer.hpp"
#include <hpx/modules/async_cuda.hpp>
#include <vector>

namespace gpu
{

// Tiled Cholesky Algorithm -------------------------------------------- {{{

/**
 * @brief Perform right-looking Cholesky decomposition.
 *
 * @param cublas CUBLAS executor.
 * @param ft_tiles Matrix represented as a vector of tiles, containing the
 *        covariance matrix, afterwards the Cholesky decomposition.
 * @param N Size of the matrix.
 * @param n_tiles Number of tiles.
 */
void right_looking_cholesky_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::size_t N,
    std::size_t n_tiles);

// }}} ------------------------------------- end of Tiled Cholesky Algorithm

// Tiled Triangular Solve Algorithms ----------------------------------- {{{

void forward_solve_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
    std::size_t N,
    std::size_t n_tiles);

void backward_solve_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
    std::size_t N,
    std::size_t n_tiles);

// Tiled Triangular Solve Algorithms for matrices (K * X = B)
void forward_solve_tiled_matrix(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
    std::size_t N,
    std::size_t M,
    std::size_t n_tiles,
    std::size_t m_tiles);

void backward_solve_tiled_matrix(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
    std::size_t N,
    std::size_t M,
    std::size_t n_tiles,
    std::size_t m_tiles);

// }}} ---------------------------- end of Tiled Triangular Solve Algorithms

// Triangular solve A_M,N * K_NxN = K_MxN -> A_MxN = K_MxN * K^-1_NxN
// Tiled Triangular Solve Algorithms for Matrices (K * X = B)
void forward_solve_KcK_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
    std::size_t N,
    std::size_t M,
    std::size_t n_tiles,
    std::size_t m_tiles);

void compute_gemm_of_invK_y(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_invK,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_y,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_alpha,
    std::size_t N,
    std::size_t n_tiles);

// Tiled Loss
void compute_loss_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_alpha,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_y,
    hpx::shared_future<double> &loss,
    std::size_t N,
    std::size_t n_tiles);

// Tiled Prediction
void prediction_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_vector,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
    std::size_t N_row,
    std::size_t N_col,
    std::size_t n_tiles,
    std::size_t m_tiles);

// Tiled Diagonal of Posterior Covariance Matrix
void posterior_covariance_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tCC_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_inter_tiles,
    std::size_t N,
    std::size_t M,
    std::size_t n_tiles,
    std::size_t m_tiles);

// Tiled Diagonal of Posterior Covariance Matrix
void full_cov_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tCC_tiles,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_priorK,
    std::size_t N,
    std::size_t M,
    std::size_t n_tiles,
    std::size_t m_tiles);

// Tiled Prediction Uncertainty
void prediction_uncertainty_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_priorK,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_inter,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_vector,
    std::size_t M,
    std::size_t m_tiles);

// Tiled Prediction Uncertainty
void pred_uncer_tiled(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_priorK,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_vector,
    std::size_t M,
    std::size_t m_tiles);

// Compute I-y*y^T*inv(K)
void update_grad_K_tiled_mkl(
    std::vector<hpx::cuda::experimental::cublas_executor> cublas,
    std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_v1,
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_v2,
    std::size_t N,
    std::size_t n_tiles);

double update_lengthscale(
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_invK,
    const std::vector<hpx::shared_future<std::vector<double>>> &
        ft_gradparam,
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_alpha,
    gpxpy_hyper::SEKParams sek_params,
    gpxpy_hyper::AdamParams adam_params,
    std::size_t N,
    std::size_t n_tiles,
    std::vector<hpx::shared_future<double>> &m_T,
    std::vector<hpx::shared_future<double>> &v_T,
    const std::vector<hpx::shared_future<double>> &beta1_T,
    const std::vector<hpx::shared_future<double>> &beta2_T,
    int iter);

double update_vertical_lengthscale(
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_invK,
    const std::vector<hpx::shared_future<std::vector<double>>> &
        ft_gradparam,
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_alpha,
    gpxpy_hyper::SEKParams sek_params,
    gpxpy_hyper::AdamParams adam_params,
    std::size_t N,
    std::size_t n_tiles,
    std::vector<hpx::shared_future<double>> &m_T,
    std::vector<hpx::shared_future<double>> &v_T,
    const std::vector<hpx::shared_future<double>> &beta1_T,
    const std::vector<hpx::shared_future<double>> &beta2_T,
    int iter);

// Update noise variance using gradient decent + Adam
double update_noise_variance(
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_invK,
    const std::vector<hpx::shared_future<std::vector<double>>> &ft_alpha,
    gpxpy_hyper::SEKParams sek_params,
    gpxpy_hyper::AdamParams adam_params,
    std::size_t N,
    std::size_t n_tiles,
    std::vector<hpx::shared_future<double>> &m_T,
    std::vector<hpx::shared_future<double>> &v_T,
    const std::vector<hpx::shared_future<double>> &beta1_T,
    const std::vector<hpx::shared_future<double>> &beta2_T,
    int iter);

}  // end of namespace gpu

#endif  // end of TILED_ALGORITHMS_GPU_H