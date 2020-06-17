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
	double *L, *R, *Aptr, *Yptr, t;
	int i, j, k;

	bzero(A, sizeof(double) * n * n);
	bzero(Y, sizeof(double) * n);

	for (i = 0, Aptr = A, Yptr = Y; i < n; ++i, ++Yptr) {
		// 构建矩阵A
		for (j = 0; j < n; ++j, ++Aptr) {
			L = x + i * m;
			R = x + j * m;
			for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
			*Aptr = t;
		}

		// 构建矢量Y
		L = x + i * m;
		R = y;
		for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
		*Yptr = t;
	}
	// 拟合系数
	if (LUdcmp(n, A)) {
		LUsolve(Y, c);
		rslt = true;
	}

	delete []A;
	delete []Y;
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

	for (i = 0, ptr = a; i < n; ++i) {
		for (j = 0, big = 0.0; j < n; ++j, ++ptr) {
			if ((t = fabs(*ptr)) > big) big = t;
		}
		if (big == 0.0) {
			np = -1;
			return false;	// 奇异矩阵
		}
		scale[i] = 1.0 / big;
	}

	for (k = 0, np = 0, ptr = a; k < n; ++k, ptr += n) {// ptr: k行首, a[k][0]
		for (i = k, big = 0.0, ptr1 = ptr + k; i < n; ++i, ptr1 += n) {// ptr1初始: a[k][k]
			if ((t = scale[i] * fabs(*ptr1)) > big) {
				big  = t;
				imax = i;
			}
		}
		if (k != imax) {
			// ptr1初始: a[imax][0]
			// ptr2初始: a[k][0]
			for (j = 0, ptr1 = a + imax * n, ptr2 = ptr; j < n; ++j, ++ptr1, ++ptr2) {
				t     = *ptr1;
				*ptr1 = *ptr2;
				*ptr2 = t;
			}
			++np;
			scale[imax] = scale[k];
		}
		idx[k] = imax;
		if (ptr[k] == 0.0) ptr[k] = TINY;
		for (i = k + 1, ptr1 = ptr + n; i < n; ++i, ptr1 += n) {// ptr1: a[i][0]=a[k+1][0]
			t = ptr1[k] /= ptr[k];
			for (j = k + 1, ptr2 = ptr + j; j < n; ++j, ++ptr2) {// ptr2: a[k][j]
				ptr1[j] -= t * *ptr2;
			}
		}
	}

	return np >= 0;
}

bool AMath::LUsolve(double *b, double *x) {
	if (ludcmp_.IsSingular()) return false;
	int n = ludcmp_.n;
	int i, j, k, ii(0);
	double sum;
	vector<int> &idx = ludcmp_.idx;
	double *luptr;

	if (x != b) memcpy(x, b, sizeof(double) * n);
	for (i = 0, luptr = ludcmp_.luptr; i < n; ++i, luptr += n) {
		k    = idx[i];
		sum  = x[k];
		x[k] = x[i];
		if (ii) {
			for (j = ii - 1; j < i; ++j) sum -= luptr[j] * x[j];
		}
		else if (sum != 0.0) ii = i + 1;
		x[i] = sum;
	}
	for (i = n - 1, luptr = ludcmp_.luptr + i * n; i >= 0; --i, luptr -= n) {
		sum  = x[i];
		for (j = i + 1; j < n; ++j) sum -= luptr[j] * x[j];
		x[i] = sum / luptr[i];
	}

	return true;
}

bool AMath::LUsolve(int m, double *b, double *x) {
	if (ludcmp_.IsSingular()) return false;
	int n = ludcmp_.n;
	int i, j;
	double *col = new double[n];
	double *ptr;

	for (j = 0; j < m; ++j) {
		for (i = 0, ptr = b + j; i < n; ++i, ptr += m) col[i] = *ptr;
		LUsolve(col, col);
		for (i = 0, ptr = x + j; i < n; ++i, ptr += m) *ptr = col[i];
	}

	delete []col;
	return true;
}

double AMath::LUDet(int n, double *a) {
	if (ludcmp_.luptr == a && ludcmp_.np < 0) return 0.0;
	if (!LUdcmp(n, a)) return 0.0;

	int i;
	double *ptr, t(1.0);

	for (i = 0, ptr = a; i < n; ++i, ptr += (n + 1)) t *= *ptr;
	if (ludcmp_.np % 2) t = -t;
	return t;
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
		LUsolve(n, y, y);
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
