#ifndef TILED_ALGORITHMS_CPU_H_INCLUDED
#define TILED_ALGORITHMS_CPU_H_INCLUDED

#include <hpx/future.hpp>
#include "mkl_adapter.hpp"
#include "uncertainty.hpp"
#include "gp_functions_grad.hpp"
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// Tiled Cholesky Algorithms
void right_looking_cholesky_tiled_mkl(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                                      std::size_t N,
                                      std::size_t n_tiles)

{
  for (std::size_t k = 0; k < n_tiles; k++)
  {
    // POTRF
    ft_tiles[k * n_tiles + k] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_potrf), "cholesky_tiled"), ft_tiles[k * n_tiles + k], N);
    for (std::size_t m = k + 1; m < n_tiles; m++)
    {
      // TRSM
      ft_tiles[m * n_tiles + k] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsm), "cholesky_tiled"), ft_tiles[k * n_tiles + k], ft_tiles[m * n_tiles + k], N);
    }
    for (std::size_t m = k + 1; m < n_tiles; m++)
    {
      // SYRK
      ft_tiles[m * n_tiles + m] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_syrk), "cholesky_tiled"), ft_tiles[m * n_tiles + m], ft_tiles[m * n_tiles + k], N);
      for (std::size_t n = k + 1; n < m; n++)
      {
        // GEMM
        ft_tiles[m * n_tiles + n] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm), "cholesky_tiled"), ft_tiles[m * n_tiles + k],
                                                  ft_tiles[n * n_tiles + k], ft_tiles[m * n_tiles + n], N);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Tiled Triangular Solve Algorithms
void forward_solve_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                         std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                         std::size_t N,
                         std::size_t n_tiles)
{
  for (std::size_t k = 0; k < n_tiles; k++)
  {
    // TRSM
    ft_rhs[k] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsv_l), "triangular_solve_tiled"), ft_tiles[k * n_tiles + k], ft_rhs[k], N);
    for (std::size_t m = k + 1; m < n_tiles; m++)
    {
      // GEMV
      ft_rhs[m] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemv_l), "triangular_solve_tiled"), ft_tiles[m * n_tiles + k], ft_rhs[k], ft_rhs[m], N);
    }
  }
}

void backward_solve_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                          std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                          std::size_t N,
                          std::size_t n_tiles)
{
  for (int k = n_tiles - 1; k >= 0; k--) // int instead of std::size_t for last comparison
  {
    // TRSM
    ft_rhs[k] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsv_u), "triangular_solve_tiled"), ft_tiles[k * n_tiles + k], ft_rhs[k], N);
    for (int m = k - 1; m >= 0; m--) // int instead of std::size_t for last comparison
    {
      // GEMV
      ft_rhs[m] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemv_u), "triangular_solve_tiled"), ft_tiles[k * n_tiles + m], ft_rhs[k], ft_rhs[m], N);
    }
  }
}

// Tiled Triangular Solve Algorithms for Mtrices (K * X = B)
void forward_solve_tiled_matrix(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                                std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                                std::size_t N,
                                std::size_t M,
                                std::size_t n_tiles,
                                std::size_t m_tiles)
{
  for (std::size_t c = 0; c < m_tiles; c++)
  {
    for (std::size_t k = 0; k < n_tiles; k++)
    {
      // TRSM
      ft_rhs[k * m_tiles + c] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsm_l_matrix), "triangular_solve_tiled_matrix"), ft_tiles[k * n_tiles + k],
                                              ft_rhs[k * m_tiles + c], N, M);
      for (std::size_t m = k + 1; m < n_tiles; m++)
      {
        // GEMV
        ft_rhs[m * m_tiles + c] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm_l_matrix), "triangular_solve_tiled_matrix"), ft_tiles[m * n_tiles + k],
                                                ft_rhs[k * m_tiles + c], ft_rhs[m * m_tiles + c], N, M);
      }
    }
  }
}

void backward_solve_tiled_matrix(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                                 std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                                 std::size_t N,
                                 std::size_t M,
                                 std::size_t n_tiles,
                                 std::size_t m_tiles)
{
  for (int c = 0; c < m_tiles; c++)
  {
    for (int k = n_tiles - 1; k >= 0; k--) // int instead of std::size_t for last comparison
    {
      // TRSM
      ft_rhs[k * m_tiles + c] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsm_u_matrix), "triangular_solve_tiled_matrix"), ft_tiles[k * n_tiles + k],
                                              ft_rhs[k * m_tiles + c], N, M);
      for (int m = k - 1; m >= 0; m--) // int instead of std::size_t for last comparison
      {
        // GEMV
        ft_rhs[m * m_tiles + c] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm_u_matrix), "triangular_solve_tiled_matrix"), ft_tiles[k * n_tiles + m],
                                                ft_rhs[k * m_tiles + c], ft_rhs[m * m_tiles + c], N, M);
      }
    }
  }
}

//////// Triangular solve A_M,N * K_NxN = K_MxN -> A_MxN = K_MxN * K^-1_NxN
// Tiled Triangular Solve Algorithms for Mtrices (K * X = B)
void forward_solve_KK_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                            std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                            std::size_t N,
                            std::size_t M,
                            std::size_t n_tiles,
                            std::size_t m_tiles)
{
  for (std::size_t r = 0; r < m_tiles; r++)
  {
    for (std::size_t c = 0; c < n_tiles; c++)
    {
      // TRSM
      ft_rhs[r * n_tiles + c] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsm_u_KK), "triangular_solve_tiled_matrix"), ft_tiles[c * n_tiles + c],
                                              ft_rhs[r * n_tiles + c], N, M);
      for (std::size_t m = c + 1; m < n_tiles; m++)
      {
        // GEMV
        ft_rhs[r * n_tiles + m] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm_u_KK), "triangular_solve_tiled_matrix"), ft_tiles[m * n_tiles + c],
                                                ft_rhs[r * n_tiles + c], ft_rhs[r * n_tiles + m], N, M);
      }
    }
  }
}

void backward_solve_KK_tiled(std::vector<hpx::shared_future<std::vector<double>>> &K_tiles,
                             std::vector<hpx::shared_future<std::vector<double>>> &CrossK_tiles,
                             std::size_t N,
                             std::size_t M,
                             std::size_t n_tiles,
                             std::size_t m_tiles)
{
  for (int r = 0; r < m_tiles; r++)
  {
    for (int c = n_tiles - 1; c >= 0; c--) // int instead of std::size_t for last comparison
    {
      // TRSM
      CrossK_tiles[r * n_tiles + c] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_trsm_l_KK), "triangular_solve_tiled_matrix"), K_tiles[c * n_tiles + c],
                                                    CrossK_tiles[r * n_tiles + c], N, M);
      for (int m = c - 1; m >= 0; m--) // int instead of std::size_t for last comparison
      {
        // GEMV
        CrossK_tiles[r * n_tiles + m] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm_l_KK), "triangular_solve_tiled_matrix"), K_tiles[c * n_tiles + m],
                                                      CrossK_tiles[r * n_tiles + c], CrossK_tiles[r * n_tiles + m], N, M);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Tiled Loss
void compute_loss_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                        std::vector<hpx::shared_future<std::vector<double>>> &ft_alpha,
                        std::vector<hpx::shared_future<std::vector<double>>> &ft_y,
                        hpx::shared_future<double> &loss,
                        std::size_t N,
                        std::size_t n_tiles)
{
  std::vector<hpx::shared_future<double>> loss_tiled;
  loss_tiled.resize(n_tiles);
  for (std::size_t k = 0; k < n_tiles; k++)
  {
    loss_tiled[k] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&compute_loss), "loss_tiled"), ft_tiles[k * n_tiles + k], ft_alpha[k], ft_y[k], N);
  }

  loss = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&add_losses), "loss_tiled"), loss_tiled, N, n_tiles);
}

// Tiled Prediction
void prediction_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                      std::vector<hpx::shared_future<std::vector<double>>> &ft_vector,
                      std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                      std::size_t N_row,
                      std::size_t N_col,
                      std::size_t n_tiles,
                      std::size_t m_tiles)
{
  for (std::size_t k = 0; k < m_tiles; k++)
  {
    for (std::size_t m = 0; m < n_tiles; m++)
    {
      ft_rhs[k] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemv_p), "prediction_tiled"), ft_tiles[k * n_tiles + m], ft_vector[m], ft_rhs[k], N_row, N_col);
    }
  }
}

// Tiled Diagonal of Posterior Covariance Matrix
void posterior_covariance_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_CC_tiles,
                                std::vector<hpx::shared_future<std::vector<double>>> &ft_tCC_tiles,
                                std::vector<hpx::shared_future<std::vector<double>>> &ft_K_tiles,
                                std::size_t N,
                                std::size_t M,
                                std::size_t n_tiles,
                                std::size_t m_tiles)
{
  for (int i = 0; i < m_tiles; ++i)
  {
    for (int j = i; j < i + 1; ++j)
    { // outer two loops used for diagonal tiles of prior K
      for (int m = 0; m < n_tiles; ++m)
      { // Compute inner product to obtain diagonal elements of (K_MxN * (K^-1_NxN * K_NxM))
        ft_K_tiles[i * m_tiles + j] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm_uncertainty_matrix), "posterior_tiled"), ft_CC_tiles[i * n_tiles + m],
                                                    ft_tCC_tiles[m * m_tiles + i], ft_K_tiles[i * m_tiles + j], N, M);
      }
    }
  }
}

// Tiled Prediction Uncertainty
void prediction_uncertainty_tiled(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                                  std::vector<hpx::shared_future<std::vector<double>>> &ft_vector,
                                  std::size_t M,
                                  std::size_t m_tiles)
{
  for (std::size_t i = 0; i < m_tiles; i++)
  {
    ft_vector[i] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&diag), "uncertainty_tiled"), ft_tiles[i * m_tiles + i], M);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Fill y*y^T*inv(K)-I Cholesky Algorithms
void update_grad_K_tiled_mkl(std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                             const std::vector<hpx::shared_future<std::vector<double>>> &ft_v1,
                             const std::vector<hpx::shared_future<std::vector<double>>> &ft_v2,
                             std::size_t N,
                             std::size_t n_tiles)

{
  for (std::size_t i = 0; i < n_tiles; i++)
  {
    for (std::size_t j = 0; j < n_tiles; j++)
    {
      ft_tiles[i * n_tiles + j] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_ger), "gradient_tiled"), ft_tiles[i * n_tiles + j], ft_v1[i], ft_v2[j], N);
    }
  }
}

// Perform a gradient scent step for selected hyperparameter
void update_hyperparameter(const std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                           const std::vector<hpx::shared_future<std::vector<double>>> &ft_rhs,
                           double *hyperparameters,
                           std::size_t N,
                           std::size_t n_tiles,
                           std::vector<hpx::shared_future<double>> &m_T,
                           std::vector<hpx::shared_future<double>> &v_T,
                           const std::vector<hpx::shared_future<double>> &beta1_T,
                           const std::vector<hpx::shared_future<double>> &beta2_T,
                           int iter,
                           int param_idx)
{
  if (param_idx == 0 || param_idx == 1)
  {
    std::vector<hpx::shared_future<std::vector<double>>> diag_tiles;
    diag_tiles.resize(n_tiles);
    for (std::size_t d = 0; d < n_tiles; d++)
    {
      diag_tiles[d] = hpx::async(hpx::annotated_function(&gen_tile_zeros_diag, "assemble_tiled"), N);
    }

    // Compute diagonal tiles using GEMM
    for (std::size_t i = 0; i < n_tiles; i++)
    {
      for (std::size_t j = 0; j < n_tiles; j++)
      {
        diag_tiles[i] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&mkl_gemm_diag), "gradient_tiled"), ft_tiles[i * n_tiles + j],
                                      ft_rhs[j * n_tiles + i], diag_tiles[i], N);
      }
    }
    // compute trace of diag_tiles
    hpx::shared_future<double> gradient = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&compute_gradient), "gradient_tiled"), diag_tiles, N, n_tiles);
    // transform hyperparameter to unconstrained form
    hpx::shared_future<double> unconstrained_param = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&to_unconstrained), "gradient_tiled"), hyperparameters[param_idx], false);
    // update moments
    m_T[param_idx] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&update_fist_moment), "gradient_tiled"), gradient, m_T[param_idx], hyperparameters[4]);
    v_T[param_idx] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&update_second_moment), "gradient_tiled"), gradient, v_T[param_idx], hyperparameters[5]);
    // update parameter
    hpx::shared_future<double> updated_param = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&update_param), "gradient_tiled"), unconstrained_param,
                                                             hyperparameters, gradient, m_T[param_idx], v_T[param_idx], beta1_T, beta2_T, iter);
    // transform hyperparameter to constrained form
    hyperparameters[param_idx] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&to_constrained), "gradient_tiled"), updated_param, false).get();
  }
  else
  {
    // Throw an exception for invalid param_idx
    throw std::invalid_argument("Invalid param_idx");
  }
}

// Update noise variance using gradient decent
void update_noise_variance(const std::vector<hpx::shared_future<std::vector<double>>> &ft_tiles,
                           double *hyperparameters,
                           std::size_t N,
                           std::size_t n_tiles,
                           std::vector<hpx::shared_future<double>> &m_T,
                           std::vector<hpx::shared_future<double>> &v_T,
                           const std::vector<hpx::shared_future<double>> &beta1_T,
                           const std::vector<hpx::shared_future<double>> &beta2_T,
                           int iter)
{
  // compute gradient ^= trace of diag_tiles
  hpx::shared_future<double> gradient = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&compute_gradient_noise), "gradient_tiled"), ft_tiles, hyperparameters, N, n_tiles);
  // transform hyperparameter to unconstrained form
  hpx::shared_future<double> unconstrained_param = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&to_unconstrained), "gradient_tiled"), hyperparameters[2], true);
  // update moments
  m_T[2] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&update_fist_moment), "gradient_tiled"), gradient, m_T[2], hyperparameters[4]);
  v_T[2] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&update_second_moment), "gradient_tiled"), gradient, v_T[2], hyperparameters[5]);
  // update parameter
  hpx::shared_future<double> updated_param = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&update_param), "gradient_tiled"), unconstrained_param,
                                                           hyperparameters, gradient, m_T[2], v_T[2], beta1_T, beta2_T, iter);
  // transform hyperparameter to constrained form
  hyperparameters[2] = hpx::dataflow(hpx::annotated_function(hpx::unwrapping(&to_constrained), "gradient_tiled"), updated_param, true).get();
}
#endif
