#include "../include/adapter_cublas.hpp"

#include <cuda_runtime.h>
#include <hpx/future.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <memory>
#include <mkl_cblas.h>
#include <mkl_lapacke.h>
#include <vector>

// frequently used names
using hpx::cuda::experimental::check_cuda_error;
using cublas_future = hpx::cuda::experimental::cuda_executor::future_type;

// =============================================================================
// BLAS operations on GPU with cuBLAS (and cuSOLVER)
// =============================================================================

// BLAS level 3 operations ------------------------------------------------- {{{

hpx::shared_future<double *>
potrf(std::shared_ptr<cusolverDnHandle_t> cusolver,
      hpx::shared_future<double *> f_A,
      const std::size_t N)
{
    cudaStream_t stream;
    cusolverDnGetStream(*cusolver, &stream);

    cusolverDnParams_t params;
    cusolverDnCreateParams(&params);

    int *d_info;
    check_cuda_error(cudaMalloc(reinterpret_cast<void **>(&d_info), sizeof(int)));

    size_t workspaceInBytesOnDevice;
    void *d_work = nullptr;
    size_t workspaceInBytesOnHost;
    void *h_work = nullptr;

    double *d_A = f_A.get();

    // NOTE: Using CUBLAS_FILL_MODE_UPPER because of column-major matrix layout,
    // but actually only modifying the lower triangular part of d_A
    cublasFillMode_t fillMode = CUBLAS_FILL_MODE_UPPER;

    cusolverDnXpotrf_bufferSize(
        *cusolver, params, fillMode, N, CUDA_R_64F, d_A, N, CUDA_R_64F, &workspaceInBytesOnDevice, &workspaceInBytesOnHost);

    check_cuda_error(cudaMalloc(reinterpret_cast<void **>(&d_work), workspaceInBytesOnDevice));

    if (workspaceInBytesOnHost > 0)
    {
        h_work = reinterpret_cast<void *>(malloc(workspaceInBytesOnHost));
        if (h_work == nullptr)
        {
            throw std::runtime_error("Error: h_work not allocated.");
        }
    }

    cusolverDnXpotrf(*cusolver, params, fillMode, N, CUDA_R_64F, d_A, N, CUDA_R_64F, d_work, workspaceInBytesOnDevice, h_work, workspaceInBytesOnHost, d_info);

    check_cuda_error(cudaStreamSynchronize(stream));

    check_cuda_error(cudaFree(d_work));
    if (h_work != nullptr)
    {
        free(h_work);
    }
    check_cuda_error(cudaFree(d_info));


    return hpx::make_ready_future(d_A);
}

hpx::shared_future<double *>
trsm(std::shared_ptr<cublasHandle_t> cublas,
     hpx::shared_future<double *> f_L,
     hpx::shared_future<double *> f_A,
     const std::size_t N,
     const std::size_t M,
     const BLAS_TRANSPOSE transpose_L,
     const BLAS_SIDE side_L)
{
    cudaStream_t stream;
    cublasGetStream_v2(*cublas, &stream);

    // TRSM constants
    const double alpha = 1.0;
    std::size_t matrixSize = N * N;

    double *d_L = f_L.get();
    double *d_A = f_A.get();

    // NOTE: Changed cublasSideMode_t and CUBLAS_FILL_MODE_UPPER according to
    // cuBLAS column-major ordering

    cublasOperation_t cublas_transpose_L = cublas_transpose(transpose_L);
    cublasSideMode_t cublas_side_L = cublas_side_invert(side_L);

    // Compute TRSM on device (X, returned as A)
    cublasDtrsm(*cublas, cublas_side_L, CUBLAS_FILL_MODE_UPPER, cublas_transpose_L, CUBLAS_DIAG_NON_UNIT, N, N, &alpha, d_L, N, d_A, N);
    check_cuda_error(cudaStreamSynchronize(stream));

    return hpx::make_ready_future(d_A);
}

hpx::shared_future<double *>
syrk(std::shared_ptr<cublasHandle_t> cublas,
     hpx::shared_future<double *> f_A,
     hpx::shared_future<double *> f_B,
     const std::size_t N)
{
    cudaStream_t stream;
    cublasGetStream_v2(*cublas, &stream);

    // SYRK constants
    const double alpha = -1.0;
    const double beta = 1.0;
    std::size_t matrixSize = N * N;

    // Allocate device memory
    double *d_A = f_A.get();
    double *d_B = f_B.get();

    // Compute SYRK on device
    // NOTE: Changed CUBLAS_FILL_MODE_UPPER & CUBLAS_OP_T according to cuBLAS
    // column-major ordering
    cublasDsyrk(*cublas, CUBLAS_FILL_MODE_UPPER, CUBLAS_OP_T, N, N, &alpha, d_B, N, &beta, d_A, N);
    check_cuda_error(cudaStreamSynchronize(stream));

    return hpx::make_ready_future(d_A);
}

hpx::shared_future<double *>
gemm_cholesky(std::shared_ptr<cublasHandle_t> cublas,
     hpx::shared_future<double *> f_A,
     hpx::shared_future<double *> f_B,
     hpx::shared_future<double *> f_C,
     const std::size_t N)
{
    cudaStream_t stream;
    cublasGetStream_v2(*cublas, &stream);

    // GEMM constants
    const double alpha = -1.0;
    const double beta = 1.0;

    double *d_A = f_A.get();
    double *d_B = f_B.get();
    double *d_C = f_C.get();

    // Copy each vector d_X from device to host and print it
    std::vector<double> h_A(N * N);
    std::vector<double> h_B(N * N);
    std::vector<double> h_C(N * N);

    // Compute GEMM on device
    // NOTE: swapped order of d_A and d_B according to cuBLAS column-major
    // ordering
    cublasDgemm(*cublas, CUBLAS_OP_T, CUBLAS_OP_N, N, N, N, &alpha, d_B, N, d_A, N, &beta, d_C, N);
    check_cuda_error(cudaStreamSynchronize(stream));

    return hpx::make_ready_future(d_C);
}

// }}} ------------------------------------------ end of BLAS level 3 operations

// BLAS level 2 operations ------------------------------------------------- {{{

hpx::shared_future<std::vector<double>>
trsv(cublas_executor *cublas,
     hpx::shared_future<std::vector<double>> f_L,
     hpx::shared_future<std::vector<double>> f_a,
     const std::size_t N,
     const BLAS_TRANSPOSE transpose_L)
{
    // Allocate device memory
    double *d_L, *d_a;
    check_cuda_error(cudaMalloc((void **) &d_L, N * N * sizeof(double)));
    check_cuda_error(cudaMalloc((void **) &d_a, N * sizeof(double)));

    // Copy data from host to device
    std::vector<double> h_L = f_L.get();
    hpx::post(*cublas, cudaMemcpyAsync, d_L, h_L.data(), N * N * sizeof(double), cudaMemcpyHostToDevice);

    std::vector<double> h_a = f_a.get();
    hpx::post(*cublas, cudaMemcpyAsync, d_a, h_a.data(), N * sizeof(double), cudaMemcpyHostToDevice);

    // TRSV: In-place solve L(^T) * x = a where L lower triangular
    // notes regarding cuBLAS:
    hpx::post(*cublas, cublasDtrsv, CUBLAS_FILL_MODE_UPPER, cublas_transpose(transpose_L), CUBLAS_DIAG_NON_UNIT, N, d_L, N, d_a, 1);

    // copy the result back to the host
    cublas_future copy_device_to_host =
        hpx::async(*cublas, cudaMemcpyAsync, h_a.data(), d_a, N * sizeof(double), cudaMemcpyDeviceToHost);

    // Synchronize, then free device memory
    copy_device_to_host.get();
    check_cuda_error(cudaFree(d_L));
    check_cuda_error(cudaFree(d_a));

    // return solution vector x
    return hpx::make_ready_future(h_a);
}

hpx::shared_future<std::vector<double>>
gemv(cublas_executor *cublas,
     hpx::shared_future<std::vector<double>> f_A,
     hpx::shared_future<std::vector<double>> f_a,
     hpx::shared_future<std::vector<double>> f_b,
     const std::size_t N,
     const std::size_t M,
     const BLAS_ALPHA alpha,
     const BLAS_TRANSPOSE transpose_A)
{
    auto A = f_A.get();
    auto a = f_a.get();
    auto b = f_b.get();
    // GEMV constants
    // const double alpha = -1.0;
    const double beta = 1.0;
    // GEMV:  b{N} = b{N} - A(^T){NxM} * a{M}
    cblas_dgemv(CblasRowMajor, static_cast<CBLAS_TRANSPOSE>(transpose_A), N, M, alpha, A.data(), M, a.data(), 1, beta, b.data(), 1);
    // return updated vector b
    return hpx::make_ready_future(b);
}

hpx::shared_future<std::vector<double>>
ger(cublas_executor *cublas,
    hpx::shared_future<std::vector<double>> f_A,
    hpx::shared_future<std::vector<double>> f_x,
    hpx::shared_future<std::vector<double>> f_y,
    const std::size_t N)
{
    auto A = f_A.get();
    auto x = f_x.get();
    auto y = f_y.get();
    // GER constants
    const double alpha = -1.0;
    // GER:  A = A - x*y^T
    cblas_dger(CblasRowMajor, N, N, alpha, x.data(), 1, y.data(), 1, A.data(), N);
    // return updated A
    return hpx::make_ready_future(A);
}

hpx::shared_future<std::vector<double>>
dot_diag_syrk(cublas_executor *cublas,
              hpx::shared_future<std::vector<double>> f_A,
              hpx::shared_future<std::vector<double>> f_r,
              const std::size_t N,
              const std::size_t M)
{
    auto A = f_A.get();
    auto r = f_r.get();
    // r = r + diag(A^T * A)
    for (int j = 0; j < M; ++j)
    {
        // Extract the j-th column and compute the dot product with itself
        r[j] += cblas_ddot(N, &A[j], M, &A[j], M);
    }
    return hpx::make_ready_future(r);
}

hpx::shared_future<std::vector<double>>
dot_diag_gemm(cublas_executor *cublas,
              hpx::shared_future<std::vector<double>> f_A,
              hpx::shared_future<std::vector<double>> f_B,
              hpx::shared_future<std::vector<double>> f_r,
              const std::size_t N,
              const std::size_t M)
{
    auto A = f_A.get();
    auto B = f_B.get();
    auto r = f_r.get();
    // r = r + diag(A * B)
    for (std::size_t i = 0; i < N; ++i)
    {
        r[i] += cblas_ddot(M, &A[i * M], 1, &B[i], N);
    }
    return hpx::make_ready_future(r);
}

// }}} ------------------------------------------ end of BLAS level 2 operations

// BLAS level 1 operations ------------------------------------------------- {{{

double dot(cublas_executor *cublas,
           std::vector<double> a,
           std::vector<double> b,
           const std::size_t N)
{
    // DOT: a * b
    return cblas_ddot(N, a.data(), 1, b.data(), 1);
}

// }}} ------------------------------------------ end of BLAS level 1 operations