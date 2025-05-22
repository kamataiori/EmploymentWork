#pragma once
#include "CharacterBase.h"
#include "Collider.h"
#include "SphereCollider.h"
#include "PlayerBullet.h"
#include <list>

class Player : public CharacterBase, public SphereCollider
{
public:

	Player(BaseScene* baseScene_) : CharacterBase(baseScene_),SphereCollider(sphere){}

	void Initialize() override;

	void Update() override;

	void Draw() override;

	void Draw2D();

	void Debug() override;

	void DrawBullets();


private:

	inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
		return a + (b - a) * t;
	}

	Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
		Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
		};
		return result;
	}

	Vector3 Multiply(const Vector3& v, const Matrix4x4& m) {
		Vector3 result;
		result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
		result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
		return result;
	}

	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result{};

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            result.m[y][x] =
                m1.m[y][0] * m2.m[0][x] +
                m1.m[y][1] * m2.m[1][x] +
                m1.m[y][2] * m2.m[2][x] +
                m1.m[y][3] * m2.m[3][x];
        }
    }

    return result;
}


	Matrix4x4 MakeRotateMatrix(const Vector3& rotate) {
		Matrix4x4 resultX = MakeRotateXMatrix(rotate.x);
		Matrix4x4 resultY = MakeRotateYMatrix(rotate.y);
		Matrix4x4 resultZ = MakeRotateZMatrix(rotate.z);
		return Multiply(resultX, Multiply(resultY, resultZ));
	}

private:

	Vector3 velocity_{};

	std::unique_ptr<Sprite> reticleSprite_;

	/*std::unique_ptr<Object3d> reticle3D_;
	Transform reticleTransform_;*/

	std::list<std::unique_ptr<PlayerBullet>> bullets_;
	int bulletStock_ = 10;
	bool canShoot_ = true;


};

