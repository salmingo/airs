/**
 * @class AMath 一些天文数据处理中使用的数学工具
 * @version 1.0
 * @date 2020-06-04
 * @author Xiaomeng Lu
 */

#ifndef SRC_AMATH_H_
#define SRC_AMATH_H_

#include <vector>
using std::vector;

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
class AMath {
/* 数据类型 */
public:
	/*!
	 * @struct LUdcmp LU矩阵分解
	 */
	struct LUdcmp {
		int n;				/// n=矩阵高度=矩阵宽度
		int np;				/// 行置换次数. >= 0: 次数; < 0: 奇异矩阵
		double *luptr;		/// LU分解结果指针==输入矩阵指针
		vector<int> idx;	/// LU分解过程中的行置换索引

	public:
		LUdcmp() {
			n = np = -1;
			luptr = NULL;
		}

		/*!
		 * @brief 重置LU分解输入矩阵
		 * @param _n   矩阵宽度=矩阵高度=n
		 * @param ptr  矩阵存储地址转换为行优先存储的一维指针
		 */
		void Reset(int _n, double *_ptr) {
			if (_n != n) idx.resize(_n);
			n     = _n;
			np    = 0;
			luptr = _ptr;
		}
		/*!
		 * @brief 判断是否奇异矩阵
		 * @return
		 * 奇异矩阵标志
		 */
		bool IsSingular() {
			return np < 0;
		}
	};

public:
	AMath();
	virtual ~AMath();

protected:
	LUdcmp ludcmp_;		/// LU矩阵分解

public:
	/*!
	 * @brief 线性最小二乘拟合
	 * @param m  样本数量
	 * @param n  基函数数量
	 * @param x  每个基函数对每个自变量的计算结果, 其数学形式是二维数组.
	 *           每行对应一个基函数, 每列对应一个自变量.
	 *           数组维度是: n*m; 矩阵
	 * @param y  样本的因变量. 数组维度是: m*1; 矢量
	 * @param c  待拟合系数. 数组维度是: n*1; 矢量
	 * @return
	 * 拟合成功标志. true: 成功; false: 失败
	 * @note
	 * - 初始版本. 待升级: 使用范式函数替代自变量表述方式
	 * - 数组x和y由用户接口填充数据
	 * - 最小二乘拟合使用逆矩阵. 当矩阵无对应逆矩阵时, 求解失败.
	 *   例如: 样本数量不足
	 */
	bool LSFitLinear(int m, int n, double *x, double *y, double *c);
	/*!
	 * @brief LU分解: LU decompose
	 * @param n    矩阵维度
	 * @param a    n*n二维矩阵
	 * @return
	 * LU分解成功标志
	 * @note
	 * 分解结果存储在原矩阵中
	 */
	bool LUdcmp(int n, double *a);
	/*!
	 * @brief LU反向替代法求解未知参数
	 * @param b   等式右侧, n*1矢量
 	 * @param x   求解结果, n*1矢量
	 * @return
	 * 求解结果
	 * @note
	 * - 求解方程A×X=B中的X
	 * - 若A是奇异矩阵则求解失败
	 * - 调用顺序:
	 *   1. LUdcmp
	 *   2. LUsolve
	 */
	bool LUsolve(double *b, double *x);
	/*!
	 * @brief LU反向替代法求解未知参数
	 * @param m   矩阵宽度, 即列数
	 * @param b   等式右侧, n*m矩阵
 	 * @param x   求解结果, n*m矩阵
	 * @return
	 * 求解结果
	 * @note
	 * - 求解方程A×X=B中的X
	 * - 若A是奇异矩阵则求解失败
	 * - 调用顺序:
	 *   1. LUdcmp
	 *   2. LUsolve
	 */
	bool LUsolve(int m, double *b, double *x);
	/*!
	 * @brief 计算n*n矩阵的行列式
	 * @return
	 * 行列式
	 */
	double Det();
	/*!
	 * @brief 计算逆矩阵
	 * @param n     二维矩阵维度
	 * @param a     输入: 原始矩阵; 输出: 逆矩阵
	 * @return
	 * 逆矩阵求解结果. true: 成功; false: 失败
	 * @note
	 * - 使用LU分解和反向替代求解逆矩阵
	 */
	bool MatrixInvert(int n, double *a);
	/*!
	 * @brief 计算矩阵乘积
	 * @param m  矩阵L的高度
	 * @param p  矩阵L的宽度==矩阵R的高度
	 * @param n  矩阵R的宽度
	 * @param L  m*p矩阵
	 * @param R  p*n矩阵
	 * @param Y  矩阵乘积, m*n矩阵
	 * @note
	 * - Y的存储空间由用户管理
	 */
	void MatrixMultiply(int m, int p, int n, double *L, double *R, double *Y);
	/*!
	 * @brief 计算转置矩阵
	 * @param m     原始矩阵高度==转置矩阵宽度
	 * @param n     原始矩阵宽度==转置矩阵高度
	 * @param a     原始矩阵
	 * @param b     转置矩阵
	 * @note
	 * - 转置矩阵方法1: 使用m*n缓冲区, 算法复杂度m*n
	 */
	template <typename T>
	void MatrixTranspose(int m, int n, T *a, T *b) {
		int row, col;
		T *aptr, *bptr;

		for (row = 0, bptr = b; row < n; ++row) {
			for (col = 0, aptr = a + row; col < m; ++col, ++bptr, aptr += n) {
				*bptr = *aptr;
			}
		}
	}
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* SRC_AMATH_H_ */
