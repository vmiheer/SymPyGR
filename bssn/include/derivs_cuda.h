/**
 * Created on: Feb 12, 2018
 * 		Author: Eminda, Akila, Eranga, Ruwan
 **/

#ifndef DERIVS_CUDA_H_
#define DERIVS_CUDA_H_

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <iostream>
#include "def.h"
// #include "thread_load_config.h"

#define CHECK_ERROR( err, msg ) if( err != cudaSuccess ) { std::cerr << "ERROR:" << cudaGetErrorName ( err ) << "  |  " << "ERROR DES: " << cudaGetErrorString( err ) << "  |  " << "User msg: " << msg << std::endl; exit( 0 ); }

#define IDX(i,j,k) ( (i) + nx * ( (j) + ny * (k) ) )

// void cuda_deriv42_x(double * output, double * dev_var_in, int u_offset, double dx, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_y(double * output, double * dev_var_in, int u_offset, double dy, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_z(double * output, double * dev_var_in, int u_offset, double dz, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_xx(double * output, double * dev_var_in, int u_offset, double dx, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_yy(double * output, double * dev_var_in, int u_offset, double dy, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_zz(double * output, double * dev_var_in, int u_offset, double dz, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_adv_x(double * output, double * dev_var_in, int u_offset, double dx, int betax, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_adv_y(double * output, double * dev_var_in, int u_offset, double dy, int betay, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_deriv42_adv_z(double * output, double * dev_var_in, int u_offset, double dz, int betaz, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_ko_deriv42_x(double * output, double * dev_var_in, int u_offset, double dx, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_ko_deriv42_y(double * output, double * dev_var_in, int u_offset, double dy, int bflag, const unsigned int * host_sz, cudaStream_t stream);

// void cuda_ko_deriv42_z(double * output, double * dev_var_in, int u_offset, double dz, int bflag, const unsigned int * host_sz, cudaStream_t stream);

void calc_deriv_wrapper(double * dev_var_out, double * dev_var_in, double hx, double hy, double hz, const unsigned int * host_sz, int bflag, cudaStream_t stream,
    #include "list_of_offset_para.h"
    ,
    #include "list_of_para.h"
);

#endif
