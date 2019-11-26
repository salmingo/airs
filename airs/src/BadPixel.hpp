/**
 * @struct BadPixel 维护坏像素列表
 * @version 0.1
 * @date 2019-11-18
 */

#ifndef BADPIXEL_H_
#define BADPIXEL_H_

/*!
 * @struct PixelBad 定义坏像素
 * @note
 * - (x, y): XY位置. x==-1时对应坏列; y==-1时对应坏行
 * - (x, y)不允许同时是-1
 * - 坏行或列的判据阈值==2, 坏像素的判据阈值==1
 */
class PixelBad {
protected:
	int x, y;	//< 坏像素位置
	int limit;	//< 相等的判定阈值

public:
	PixelBad() {
		x = y = -1;
		limit = 1;
	}

	/*!
	 * @brief 设置坐标
	 */
	void SetPixel(int _x, int _y) {
		if (!(_x == -1 && _y == -1)) {
			x = _x;
			y = _y;

			if (x == -1 || y == -1) limit = 2;
			else limit = 1;
		}
	}
};

#endif
