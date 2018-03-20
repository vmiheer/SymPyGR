/*
 * kernal.h
 *
 *  Created on: March 12, 2018
 *      Author: eminda
 */

#ifndef BSSNEQN_SOLVE_H
#define BSSNEQN_SOLVE_H

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

__global__ void cuda_callculateBSSN(
double *grad_0_alpha,
double *grad_1_alpha,
double *grad_2_alpha,
double *grad_0_beta0,
double *grad_1_beta0,
double *grad_2_beta0,
double *grad_0_beta1,
double *grad_1_beta1,
double *grad_2_beta1,
double *grad_0_beta2,
double *grad_1_beta2,
double *grad_2_beta2,
double *grad_0_B0,
double *grad_1_B0,
double *grad_2_B0,
double *grad_0_B1,
double *grad_1_B1,
double *grad_2_B1,
double *grad_0_B2,
double *grad_1_B2,
double *grad_2_B2,
double *grad_0_chi,
double *grad_1_chi,
double *grad_2_chi,
double *grad_0_Gt0,
double *grad_1_Gt0,
double *grad_2_Gt0,
double *grad_0_Gt1,
double *grad_1_Gt1,
double *grad_2_Gt1,
double *grad_0_Gt2,
double *grad_1_Gt2,
double *grad_2_Gt2,
double *grad_0_K,
double *grad_1_K,
double *grad_2_K,
double *grad_0_gt0,
double *grad_1_gt0,
double *grad_2_gt0,
double *grad_0_gt1,
double *grad_1_gt1,
double *grad_2_gt1,
double *grad_0_gt2,
double *grad_1_gt2,
double *grad_2_gt2,
double *grad_0_gt3,
double *grad_1_gt3,
double *grad_2_gt3,
double *grad_0_gt4,
double *grad_1_gt4,
double *grad_2_gt4,
double *grad_0_gt5,
double *grad_1_gt5,
double *grad_2_gt5,
double *grad_0_At0,
double *grad_1_At0,
double *grad_2_At0,
double *grad_0_At1,
double *grad_1_At1,
double *grad_2_At1,
double *grad_0_At2,
double *grad_1_At2,
double *grad_2_At2,
double *grad_0_At3,
double *grad_1_At3,
double *grad_2_At3,
double *grad_0_At4,
double *grad_1_At4,
double *grad_2_At4,
double *grad_0_At5,
double *grad_1_At5,
double *grad_2_At5,
double *grad2_0_0_gt0,
double *grad2_0_1_gt0,
double *grad2_0_2_gt0,
double *grad2_1_1_gt0,
double *grad2_1_2_gt0,
double *grad2_2_2_gt0,
double *grad2_0_0_gt1,
double *grad2_0_1_gt1,
double *grad2_0_2_gt1,
double *grad2_1_1_gt1,
double *grad2_1_2_gt1,
double *grad2_2_2_gt1,
double *grad2_0_0_gt2,
double *grad2_0_1_gt2,
double *grad2_0_2_gt2,
double *grad2_1_1_gt2,
double *grad2_1_2_gt2,
double *grad2_2_2_gt2,
double *grad2_0_0_gt3,
double *grad2_0_1_gt3,
double *grad2_0_2_gt3,
double *grad2_1_1_gt3,
double *grad2_1_2_gt3,
double *grad2_2_2_gt3,
double *grad2_0_0_gt4,
double *grad2_0_1_gt4,
double *grad2_0_2_gt4,
double *grad2_1_1_gt4,
double *grad2_1_2_gt4,
double *grad2_2_2_gt4,
double *grad2_0_0_gt5,
double *grad2_0_1_gt5,
double *grad2_0_2_gt5,
double *grad2_1_1_gt5,
double *grad2_1_2_gt5,
double *grad2_2_2_gt5,
double *grad2_0_0_chi,
double *grad2_0_1_chi,
double *grad2_0_2_chi,
double *grad2_1_1_chi,
double *grad2_1_2_chi,
double *grad2_2_2_chi,
double *grad2_0_0_alpha,
double *grad2_0_1_alpha,
double *grad2_0_2_alpha,
double *grad2_1_1_alpha,
double *grad2_1_2_alpha,
double *grad2_2_2_alpha,
double *grad2_0_0_beta0,
double *grad2_0_1_beta0,
double *grad2_0_2_beta0,
double *grad2_1_1_beta0,
double *grad2_1_2_beta0,
double *grad2_2_2_beta0,
double *grad2_0_0_beta1,
double *grad2_0_1_beta1,
double *grad2_0_2_beta1,
double *grad2_1_1_beta1,
double *grad2_1_2_beta1,
double *grad2_2_2_beta1,
double *grad2_0_0_beta2,
double *grad2_0_1_beta2,
double *grad2_0_2_beta2,
double *grad2_1_1_beta2,
double *grad2_1_2_beta2,
double *grad2_2_2_beta2,
double *agrad_0_gt0,
double *agrad_1_gt0,
double *agrad_2_gt0,
double *agrad_0_gt1,
double *agrad_1_gt1,
double *agrad_2_gt1,
double *agrad_0_gt2,
double *agrad_1_gt2,
double *agrad_2_gt2,
double *agrad_0_gt3,
double *agrad_1_gt3,
double *agrad_2_gt3,
double *agrad_0_gt4,
double *agrad_1_gt4,
double *agrad_2_gt4,
double *agrad_0_gt5,
double *agrad_1_gt5,
double *agrad_2_gt5,
double *agrad_0_At0,
double *agrad_1_At0,
double *agrad_2_At0,
double *agrad_0_At1,
double *agrad_1_At1,
double *agrad_2_At1,
double *agrad_0_At2,
double *agrad_1_At2,
double *agrad_2_At2,
double *agrad_0_At3,
double *agrad_1_At3,
double *agrad_2_At3,
double *agrad_0_At4,
double *agrad_1_At4,
double *agrad_2_At4,
double *agrad_0_At5,
double *agrad_1_At5,
double *agrad_2_At5,
double *agrad_0_alpha,
double *agrad_1_alpha,
double *agrad_2_alpha,
double *agrad_0_beta0,
double *agrad_1_beta0,
double *agrad_2_beta0,
double *agrad_0_beta1,
double *agrad_1_beta1,
double *agrad_2_beta1,
double *agrad_0_beta2,
double *agrad_1_beta2,
double *agrad_2_beta2,
double *agrad_0_chi,
double *agrad_1_chi,
double *agrad_2_chi,
double *agrad_0_Gt0,
double *agrad_1_Gt0,
double *agrad_2_Gt0,
double *agrad_0_Gt1,
double *agrad_1_Gt1,
double *agrad_2_Gt1,
double *agrad_0_Gt2,
double *agrad_1_Gt2,
double *agrad_2_Gt2,
double *agrad_0_K,
double *agrad_1_K,
double *agrad_2_K,
double *agrad_0_B0,
double *agrad_1_B0,
double *agrad_2_B0,
double *agrad_0_B1,
double *agrad_1_B1,
double *agrad_2_B1,
double *agrad_0_B2,
double *agrad_1_B2,
double *agrad_2_B2,
int *dev_alphaInt,
int *dev_chiInt,
int *dev_KInt,
int *dev_gt0Int,
int *dev_gt1Int,
int *dev_gt2Int,
int *dev_gt3Int,
int *dev_gt4Int,
int *dev_gt5Int,
int *dev_beta0Int,
int *dev_beta1Int,
int *dev_beta2Int,
int *dev_At0Int,
int *dev_At1Int,
int *dev_At2Int,
int *dev_At3Int,
int *dev_At4Int,
int *dev_At5Int,
int *dev_Gt0Int,
int *dev_Gt1Int,
int *dev_Gt2Int,
int *dev_B0Int,
int *dev_B1Int,
int *dev_B2Int,
unsigned int *lambda,
double *lambda_f,
const double *dev_pmin,
const int *dev_sz,
double *dev_hx,
double *dev_hy,
double *dev_hz,
double *dev_var_in,
double *dev_var_out,
int *dev_sizeArray
);
	
void callculateBSSN_EQ(
    double *grad_0_alpha,
    double *grad_1_alpha,
    double *grad_2_alpha,
    double *grad_0_beta0,
    double *grad_1_beta0,
    double *grad_2_beta0,
    double *grad_0_beta1,
    double *grad_1_beta1,
    double *grad_2_beta1,
    double *grad_0_beta2,
    double *grad_1_beta2,
    double *grad_2_beta2,
    double *grad_0_B0,
    double *grad_1_B0,
    double *grad_2_B0,
    double *grad_0_B1,
    double *grad_1_B1,
    double *grad_2_B1,
    double *grad_0_B2,
    double *grad_1_B2,
    double *grad_2_B2,
    double *grad_0_chi,
    double *grad_1_chi,
    double *grad_2_chi,
    double *grad_0_Gt0,
    double *grad_1_Gt0,
    double *grad_2_Gt0,
    double *grad_0_Gt1,
    double *grad_1_Gt1,
    double *grad_2_Gt1,
    double *grad_0_Gt2,
    double *grad_1_Gt2,
    double *grad_2_Gt2,
    double *grad_0_K,
    double *grad_1_K,
    double *grad_2_K,
    double *grad_0_gt0,
    double *grad_1_gt0,
    double *grad_2_gt0,
    double *grad_0_gt1,
    double *grad_1_gt1,
    double *grad_2_gt1,
    double *grad_0_gt2,
    double *grad_1_gt2,
    double *grad_2_gt2,
    double *grad_0_gt3,
    double *grad_1_gt3,
    double *grad_2_gt3,
    double *grad_0_gt4,
    double *grad_1_gt4,
    double *grad_2_gt4,
    double *grad_0_gt5,
    double *grad_1_gt5,
    double *grad_2_gt5,
    double *grad_0_At0,
    double *grad_1_At0,
    double *grad_2_At0,
    double *grad_0_At1,
    double *grad_1_At1,
    double *grad_2_At1,
    double *grad_0_At2,
    double *grad_1_At2,
    double *grad_2_At2,
    double *grad_0_At3,
    double *grad_1_At3,
    double *grad_2_At3,
    double *grad_0_At4,
    double *grad_1_At4,
    double *grad_2_At4,
    double *grad_0_At5,
    double *grad_1_At5,
    double *grad_2_At5,
    double *grad2_0_0_gt0,
    double *grad2_0_1_gt0,
    double *grad2_0_2_gt0,
    double *grad2_1_1_gt0,
    double *grad2_1_2_gt0,
    double *grad2_2_2_gt0,
    double *grad2_0_0_gt1,
    double *grad2_0_1_gt1,
    double *grad2_0_2_gt1,
    double *grad2_1_1_gt1,
    double *grad2_1_2_gt1,
    double *grad2_2_2_gt1,
    double *grad2_0_0_gt2,
    double *grad2_0_1_gt2,
    double *grad2_0_2_gt2,
    double *grad2_1_1_gt2,
    double *grad2_1_2_gt2,
    double *grad2_2_2_gt2,
    double *grad2_0_0_gt3,
    double *grad2_0_1_gt3,
    double *grad2_0_2_gt3,
    double *grad2_1_1_gt3,
    double *grad2_1_2_gt3,
    double *grad2_2_2_gt3,
    double *grad2_0_0_gt4,
    double *grad2_0_1_gt4,
    double *grad2_0_2_gt4,
    double *grad2_1_1_gt4,
    double *grad2_1_2_gt4,
    double *grad2_2_2_gt4,
    double *grad2_0_0_gt5,
    double *grad2_0_1_gt5,
    double *grad2_0_2_gt5,
    double *grad2_1_1_gt5,
    double *grad2_1_2_gt5,
    double *grad2_2_2_gt5,
    double *grad2_0_0_chi,
    double *grad2_0_1_chi,
    double *grad2_0_2_chi,
    double *grad2_1_1_chi,
    double *grad2_1_2_chi,
    double *grad2_2_2_chi,
    double *grad2_0_0_alpha,
    double *grad2_0_1_alpha,
    double *grad2_0_2_alpha,
    double *grad2_1_1_alpha,
    double *grad2_1_2_alpha,
    double *grad2_2_2_alpha,
    double *grad2_0_0_beta0,
    double *grad2_0_1_beta0,
    double *grad2_0_2_beta0,
    double *grad2_1_1_beta0,
    double *grad2_1_2_beta0,
    double *grad2_2_2_beta0,
    double *grad2_0_0_beta1,
    double *grad2_0_1_beta1,
    double *grad2_0_2_beta1,
    double *grad2_1_1_beta1,
    double *grad2_1_2_beta1,
    double *grad2_2_2_beta1,
    double *grad2_0_0_beta2,
    double *grad2_0_1_beta2,
    double *grad2_0_2_beta2,
    double *grad2_1_1_beta2,
    double *grad2_1_2_beta2,
    double *grad2_2_2_beta2,
    double *agrad_0_gt0,
    double *agrad_1_gt0,
    double *agrad_2_gt0,
    double *agrad_0_gt1,
    double *agrad_1_gt1,
    double *agrad_2_gt1,
    double *agrad_0_gt2,
    double *agrad_1_gt2,
    double *agrad_2_gt2,
    double *agrad_0_gt3,
    double *agrad_1_gt3,
    double *agrad_2_gt3,
    double *agrad_0_gt4,
    double *agrad_1_gt4,
    double *agrad_2_gt4,
    double *agrad_0_gt5,
    double *agrad_1_gt5,
    double *agrad_2_gt5,
    double *agrad_0_At0,
    double *agrad_1_At0,
    double *agrad_2_At0,
    double *agrad_0_At1,
    double *agrad_1_At1,
    double *agrad_2_At1,
    double *agrad_0_At2,
    double *agrad_1_At2,
    double *agrad_2_At2,
    double *agrad_0_At3,
    double *agrad_1_At3,
    double *agrad_2_At3,
    double *agrad_0_At4,
    double *agrad_1_At4,
    double *agrad_2_At4,
    double *agrad_0_At5,
    double *agrad_1_At5,
    double *agrad_2_At5,
    double *agrad_0_alpha,
    double *agrad_1_alpha,
    double *agrad_2_alpha,
    double *agrad_0_beta0,
    double *agrad_1_beta0,
    double *agrad_2_beta0,
    double *agrad_0_beta1,
    double *agrad_1_beta1,
    double *agrad_2_beta1,
    double *agrad_0_beta2,
    double *agrad_1_beta2,
    double *agrad_2_beta2,
    double *agrad_0_chi,
    double *agrad_1_chi,
    double *agrad_2_chi,
    double *agrad_0_Gt0,
    double *agrad_1_Gt0,
    double *agrad_2_Gt0,
    double *agrad_0_Gt1,
    double *agrad_1_Gt1,
    double *agrad_2_Gt1,
    double *agrad_0_Gt2,
    double *agrad_1_Gt2,
    double *agrad_2_Gt2,
    double *agrad_0_K,
    double *agrad_1_K,
    double *agrad_2_K,
    double *agrad_0_B0,
    double *agrad_1_B0,
    double *agrad_2_B0,
    double *agrad_0_B1,
    double *agrad_1_B1,
    double *agrad_2_B1,
    double *agrad_0_B2,
    double *agrad_1_B2,
    double *agrad_2_B2,
    int *dev_alphaInt,
    int *dev_chiInt,
    int *dev_KInt,
    int *dev_gt0Int,
    int *dev_gt1Int,
    int *dev_gt2Int,
    int *dev_gt3Int,
    int *dev_gt4Int,
    int *dev_gt5Int,
    int *dev_beta0Int,
    int *dev_beta1Int,
    int *dev_beta2Int,
    int *dev_At0Int,
    int *dev_At1Int,
    int *dev_At2Int,
    int *dev_At3Int,
    int *dev_At4Int,
    int *dev_At5Int,
    int *dev_Gt0Int,
    int *dev_Gt1Int,
    int *dev_Gt2Int,
    int *dev_B0Int,
    int *dev_B1Int,
    int *dev_B2Int,
    unsigned int *dev_lambda,
    double *dev_lambda_f,
    const double *dev_pmin,
    const int *dev_sz,
    double *dev_hx,
    double *dev_hy,
    double *dev_hz,
    double *dev_var_in,
    double *dev_var_out,
    int *sizeArray
    );


#endif /* BSSNEQN_SOLVE_H_ */