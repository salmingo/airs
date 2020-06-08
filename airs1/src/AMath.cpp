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

bool AMath::LUdecmp(int n, double *a, int *idx) {
	int i, j, k, imax, nm1(n - 1), np(1);
	double amax, asum, t;
	double *aptr, *aptr1, *aptr2;
	double *scale = new double[n];

	// 统计行归一比例尺
	for (j = 0, aptr = a; j < n; ++j) {
		amax = 0.0;
		for (i = 0; i < n; ++i, ++aptr) {
			if ((t = fabs(*aptr)) > amax) amax = t;
		}
		if (amax < AEPS) {// 奇异矩阵
			delete []scale;
			return false;
		}
		scale[j] = 1.0 / amax;
	}
	// LU分解
	for (j = 0, aptr = a; j < n; ++j, aptr += (n + 1)) {
		// 初始化L(不含对角线)
		for (i = 0, aptr1 = a; i < j; ++i, aptr1 += n) {
			asum = aptr1[j];
			for (k = 0, aptr2 = a + j; k < i; ++k, aptr2 += n) {
				asum -= aptr1[k] * *aptr2;
			}
			aptr1[j] = asum;
		}
		// 初始化U(含对角线)
		amax = 0.0;
		for (i = j, aptr1 = a + i * n; i < n; ++i, aptr1 += n) {
			asum = aptr1[j];
			for (k = 0, aptr2 = a + j; k < j; ++k, aptr2 += n) {
				asum -= aptr1[k] * *aptr2;
			}
			aptr1[j] = asum;
			t = scale[i] * fabs(asum);
			if (t >= amax) {
				imax = i;
				amax = t;
			}
		}
		// 检查并交换行
		if (j != imax) {
			for (k = 0, aptr1 = a + imax * n, aptr2 = a + j * n; k < n; ++k, ++aptr1, ++aptr2) {
				t = *aptr1;
				*aptr1 = *aptr2;
				*aptr2 = t;
			}
			np = -np;	// 行置换次数的奇偶性
			scale[imax] = scale[j];
		}
		idx[j] = imax;
		// 绕轴归一
		if (j != nm1) {
			t = 1. / *aptr;
			for (i = j + 1, aptr1 = a + (j + 1) * n + j; i < n; ++i, aptr1 += n) {
				*aptr1 = *aptr1 * t;
			}
		}
	}
	delete []scale;

	// 检查分解后矩阵的奇异性: 对角线上有0
	for (j = 0, aptr = a; j < n; ++j, aptr += (n + 1)) {
		if (*aptr < AEPS) return false;
	}
	return true;
}

bool AMath::LUbksb(int n, double *a, int *idx, double *b) {
	int i, j, k, ii(-1);
	double sum;
	double *aptr, *bptr;

	for (i = 0, aptr = a; i < n; ++i, aptr += n) {// 正向替代
		k = idx[i];
		sum = b[k];
		b[k] = b[i];

		if (ii != -1) {
			for (j = ii, bptr = b + ii; j < i; ++j, ++bptr) {
				sum -= (aptr[j] * *bptr);
			}
		}
		else if (fabs(sum) > AEPS) ii = i;
		b[i] = sum;
	}
	for (i = n - 1, aptr = a + (n - 1) * n; i >= 0; --i, aptr -= n) {// 反向替代, 求解
		sum = b[i];
		for (j = i + 1, bptr = b + i + 1; j < n; ++j, ++bptr) {
			sum -= (aptr[j] * *bptr);
		}
		if (fabs(aptr[i]) < AEPS) return false;	// 奇异矩阵
		b[i] = sum / aptr[i];
	}
	return true;
}

// 计算逆矩阵
bool AMath::MatrixInvert(int n, double *a) {
	int n2 = n * n;
	double *y = new double[n2];
	int *idx  = new int[n];
	double *yptr;
	int i;
	bool rslt(false);

	bzero(y, sizeof(double) * n2);
	for (i = 0, yptr = y; i < n; ++i, yptr += (n + 1)) *yptr = 1.0;
	if (LUdecmp(n, a, idx)) {
		yptr = a;
		for (int j = 0; j < n; ++j) {
			for (i = 0; i < n; ++i, ++yptr) {
				printf ("%.1f  ", *yptr);
			}
			printf ("\n");
		}

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

// 计算转置矩阵
void AMath::MatrixTranspose(int m, int n, double *a, double *b) {
	int i, j;
	double *aptr, *bptr;

	for (j = 0, aptr = a; j < m; ++j) {
		for (i = 0, bptr = b + j; i < n; ++i, ++aptr, bptr += m) {
			*bptr = *aptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
