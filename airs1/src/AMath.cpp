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
bool LSFitLinear(int m, int n, double *x, double *y, double *c) {
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
	if (LUdecmp(n, A, idx)) {
		LUbksb(n, A, idx, Y);
		memcpy(c, Y, sizeof(double) * n);
		rslt = true;
	}

	delete []A;
	delete []Y;
	delete []idx;
	return rslt;
}

bool LUdecmp(int n, double *a, int *idx) {
	bool rslt(true);
	int row, col, k, imax, np(1);
	double EPS(1.0E-20);
	double amax, asum, t;
	double *aptr, *aptr1, *aptr2;
	double *scale = new double[n];

	// 统计行归一比例因子
	for (row = 0, aptr = a; row < n; ++row) {// 遍历: 行
		amax = 0.0;
		for (col = 0; col < n; ++col, ++aptr) {// 遍历: 列
			if ((t = fabs(*aptr)) > amax) amax = t;
		}
		if (amax < EPS) {// 奇异矩阵
			delete []scale;
			return false;
		}
		scale[row] = 1.0 / amax;
	}

	// LU分解
	for (col = 0; col < n && rslt; ++col) {
		for (row = 0, aptr = a; row < col; ++row, aptr += n) {
			asum = aptr[col];
			for (k = 0, aptr1 = aptr, aptr2 = a + col; k < row; ++k, ++aptr1, aptr2 += n) {
				asum -= (*aptr1 * *aptr2);
			}
			aptr[col] = asum;
		}

		for (row = col, amax = 0.0, aptr = a; row < n; ++row, aptr += n) {
			asum = aptr[col];
			for (k = 0, aptr1 = aptr, aptr2 = a + col; k < row; ++k, ++aptr1, aptr2 += n) {
				asum -= (*aptr1 * *aptr2);
			}
			aptr[col] = asum;

			if ((t = scale[row] * fabs(asum)) >= amax) {
				imax = row;
				amax = t;
			}
		}

		if (col != imax) {
			for (k = 0, aptr1 = a + imax * n, aptr2 = a + col * n; k < n; ++k, ++aptr1, ++aptr2) {
				t      = *aptr1;
				*aptr1 = *aptr2;
				*aptr2 = t;
			}
			np = -np;	// 行交换次数的奇偶性
			scale[imax] = scale[col];
		}
		idx[col] = imax;

		if (fabs(t = a[col * n + col]) < EPS) rslt = false;
		else {
			t = 1.0 / t;
			for (row = col + 1, aptr = a + row * n + col; row < n; ++row, aptr += n) {
				*aptr = *aptr * t;
			}
		}
	}
	delete []scale;

	if (rslt) {
		printf ("LUdecmp result:\n");
		for (row = 0, aptr = a; row < n; ++row) {
			for (col = 0; col < n; ++col, ++aptr) {
				printf ("%f  ", *aptr);
			}
			printf ("\n");
		}
		printf ("\n");
	}

	return rslt;
}

bool LUbksb(int n, double *a, int *idx, double *b) {
	bool rslt(true);
	int row, col, k, ii(-1);
	double EPS(1.0E-20);
	double sum;
	double *aptr, *aptr1, *bptr;

	for (row = 0, aptr = a; row < n; ++row, aptr += n) {
		k      = idx[row];
		sum    = b[k];
		b[k]   = b[row];
		if (ii != -1) {
			for (col = ii, aptr1 = aptr + col, bptr = b + col; col < row; ++col, ++aptr1, ++bptr) {
				sum -= (*aptr1 * *bptr);
			}
		}
		else if (fabs(sum) > EPS) ii = row;
		b[row] = sum;
	}

	for (row = n - 1, aptr = a + row * n; row >= 0 && rslt; --row, aptr -= n) {
		rslt = fabs(aptr[row]) > EPS;
		if (rslt) {
			sum = b[row];
			for (col = row + 1, aptr1 = aptr + col, bptr = b + col; col < n; ++col, ++aptr1, ++bptr) {
				sum -= (*aptr1 * *bptr);
			}
			b[row] = sum / aptr[row];
		}
	}

	return rslt;
}

// 计算逆矩阵
bool MatrixInvert(int n, double *a) {
	int n2 = n * n;
	double *y = new double[n2];
	int *idx  = new int[n];
	double *yptr;
	int i;
	bool rslt(false);

	bzero(y, sizeof(double) * n2);
	for (i = 0, yptr = y; i < n; ++i, yptr += (n + 1)) *yptr = 1.0;
	if (LUdecmp(n, a, idx)) {
		for (i = 0, yptr = y; i < n; ++i, ++yptr) {
			LUbksb(n, a, idx, yptr);
		}
		memcpy(a, y, sizeof(double) * n2);
		rslt = true;
	}

	delete []y;
	delete []idx;
	return rslt;
}

// 计算矩阵乘积
void MatrixMultiply(int m, int p, int n, double *L, double *R, double *Y) {
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
