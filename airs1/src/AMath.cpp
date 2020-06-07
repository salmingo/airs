/**
 * @class AMath 一些天文数据处理中使用的数学工具
 * @version 1.0
 * @date 2020-06-04
 * @author Xiaomeng Lu
 */

#include <math.h>
#include <string.h>
#include <strings.h>
#include "AMath.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
AMath::AMath() {

}

AMath::~AMath() {
}

bool AMath::LSFitLinear(int m, int n, double *x, double *y, double *c) {
	bool rslt(false);
	// A*c=Y ==> c = A^-1*Y
	double *A    = new double[n * n];
	double *InvA = new double[n * n];
	double *Y    = new double[n];
	double *L, *R, *Aptr, *Yptr, t;
	int i, j, k;

	bzero(A, sizeof(double) * n * n);
	bzero(Y, sizeof(double) * n);

	for (i = 0, Aptr = A, Yptr = Y; i < n; ++i, ++Yptr) {
		// 构建矩阵A
		for (j = 0; j < n; ++j, ++Aptr) {
			L = x + i * m;
			R = x + j * m;
			for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) {
				t += *L * *R;
			}
			*Aptr = t;
		}

		// 构建矢量Y
		L = x + i * m;
		R = y;
		for (k = 0, t = 0.0; k < m; ++k, ++L, ++R)
			t += *L * *R;
		*Yptr = t;
	}
	if (MatrixInvert(n, A, InvA)) {
		MatrixMultiply(n, n, 1, InvA, Y, c);
		rslt = true;
	}

	delete []A;
	delete []InvA;
	delete []Y;
	return rslt;
}

// 计算逆矩阵
bool AMath::MatrixInvert(int n, double *mat0, double *mat1) {
	return true;
}

// 计算矩阵乘积
void AMath::MatrixMultiply(int m, int p, int n, double *L, double *R, double *Y) {

}

// 计算转置矩阵
void AMath::MatrixTranspose(int m, int n, double *mat0, double *mat1) {

}

//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
