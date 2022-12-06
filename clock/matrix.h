#pragma once

#include <d2d1_3.h>

class Matrix3 {
	D2D1::Matrix3x2F inner;
public:

	Matrix3() {}
	Matrix3(const D2D1::Matrix3x2F m): inner(m) {}

	const D2D1::Matrix3x2F& getInner() {
		return inner;
	}

	Matrix3& operator=(const Matrix3& oth) {
		inner = oth.inner;
		return *this;
	};

	Matrix3 operator*(const Matrix3& oth) {
		Matrix3 out;
		out.inner = inner;
		out.inner.SetProduct(out.inner, oth.inner);
	};

	Matrix3& operator*=(const Matrix3& oth) {
		inner.SetProduct(inner, oth.inner);
		return *this;
	};
};


