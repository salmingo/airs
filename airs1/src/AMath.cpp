/**
 * @class AMath 一些天文数据处理中使用的数学工具
 * @version 1.0
 * @date 2020-06-04
 * @author Xiaomeng Lu
 */

#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include "ADefine.h"
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
	double *A  = new double[n * n];
	double *Y  = new double[n];
	int *idx   = new int[n];
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
		for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
		*Yptr = t;
	}
	// 拟合系数
//	if (LUdcmp(n, A, idx)) {
//		LUbksb(n, A, idx, Y);
//		memcpy(c, Y, sizeof(double) * n);
//		rslt = true;
//	}

	delete []A;
	delete []Y;
	delete []idx;
	return rslt;
}

bool AMath::LUdcmp(int n, double *a) {
	ludcmp_.Reset(n, a);

	vector<int> &idx = ludcmp_.idx;
	int &np = ludcmp_.np;
	const double TINY = 1.0E-30;
	vector<double> scale(n);
	int i, j, k, imax;
	double t, big, *ptr, *ptr1, *ptr2;

	np = 0;
	for (i = 0, ptr = a; i < n; ++i) {
		big = 0.0;
		for (j = 0; j < n; ++j, ++ptr) {
			if ((t = fabs(*ptr)) > big) big = t;
		}
		if (big < TINY) {
			np = -1;
			return false;	// 奇异矩阵
		}
		scale[i] = 1.0 / big;
	}

	for (j = 0, ptr = a; j < n; ++j, ptr += n) {
		big = 0.0;
		for (i = j, ptr1 = ptr + j; i < n; ++i, ptr1 += n) {
			if ((t = scale[i] * fabs(*ptr1)) > big) {
				big  = t;
				imax = i;
			}
		}
		if (j != imax) {
			for (k = 0, ptr1 = a + imax * n, ptr2 = ptr; k < n; ++k, ++ptr1, ++ptr2) {
				t     = *ptr1;
				*ptr1 = *ptr2;
				*ptr2 = t;
			}
			++np;
			scale[imax] = scale[j];
		}
		idx[j] = imax;
		if (ptr[j] < TINY) {
			np = -1;
			return false;
		}
		for (i = j + 1, ptr1 = ptr + n; i < n; ++i, ptr1 += n) {
			t = ptr1[j] /= ptr[j];	//??? /= => / ???
			for (k = j + 1, ptr2 = ptr + k; k < n; ++k, ++ptr2) {
				ptr1[k] -= t * *ptr2;
			}
		}
	}

	return np != 0;
}

bool AMath::LUsolve(double *b, double *x) {

	return false;
}

// 计算逆矩阵
bool AMath::MatrixInvert(int n, double *a) {
	int n2 = n * n;
	double *y = new double[n2];
	double *yptr;
	int i;
	bool rslt(false);

	bzero(y, sizeof(double) * n2);
	for (i = 0, yptr = y; i < n; ++i, yptr += (n + 1)) *yptr = 1.0;
	if (LUdcmp(n, a)) {
		for (i = 0, yptr = y; i < n; ++i, ++yptr) {
//			LUsolve(n, a, idx, yptr);
		}
		memcpy(a, y, sizeof(double) * n2);
		rslt = true;
	}

	delete []y;
	return rslt;
}

// 计算矩阵乘积
void AMath::MatrixMultiply(int m, int p, int n, double *L, double *R, double *Y) {
	int i, j, k;
	double sum;
	double *Lptr, *Rptr, *Yptr;

	for (j = 0, Yptr = Y; j < m; ++j) {
		for (i = 0; i < n; ++i, ++Yptr) {
			Lptr = L + j * p;
			Rptr = R + i;
			for (k = 0, sum = 0.0; k < p; ++k, ++Lptr, Rptr += n) {
				sum += *Lptr * *Rptr;
			}
			*Yptr = sum;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
