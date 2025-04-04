#pragma once
#include "Vector3.h" // Vector3 構造体が必要

//Matrix4x4
struct Matrix4x4 {
    float m[4][4];

    // 行列の掛け算演算子オーバーロード
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result = {};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }

    // **平行移動成分を取得するメソッド**
    Vector3 Translation() const {
        return { m[3][0], m[3][1], m[3][2] };
    }
};
