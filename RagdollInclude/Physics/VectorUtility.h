#pragma once
#include <algorithm>
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;


//Vector3: 3次元ベクトル(x, y, z)
struct Vector3
{
	float x;
	float y;
	float z;

	//引数を省略すると (0, 0, 0)
	Vector3(float X = 0, float Y = 0, float Z = 0) { x = X; y = Y;z = Z; }

	//渡した長さで割る(正規化の分母を、自分で用意するとき用)。
	void Normalize(float distance)
	{
		x /= distance;
		y /= distance;
		z /= distance;
	}

	//長さを 1 にする。長さがほぼ 0 のときは、何もしない(0除算を避ける)
	void Normalize()
	{
		float length = std::sqrt(x * x + y * y + z * z);
		if (length < 0.000001f)
			return;
		x /= length;
		y /= length;
		z /= length;
	}

	//ベクトルの長さ
	float Length()
	{
		return  (float)std::sqrt(x * x + y * y + z * z);
	}

	//符号を反転したベクトル(-v)
	Vector3 operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	//ベクトルの加算(自分を書き換える)
	Vector3& operator+=(const Vector3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	//ベクトルの加算(新しいベクトルを返す)
	Vector3 operator +(const Vector3& other)
	{
		return
		{
			x + other.x,
			y + other.y,
			z + other.z
		};
	}

	//ベクトルの減算(自分を書き換える)
	Vector3 operator-=(const Vector3& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;

		return *this;
	}

	//ベクトルの減算(新しいベクトルを返す)
	Vector3 operator-(const Vector3& other) const
	{
		return {
			x - other.x,
			y - other.y,
			z - other.z
		};
	}

	//全成分に同じ値を足す(ベクトル + スカラーの意味。全成分に加算される点に注意)
	Vector3& operator +=(const float& sub)
	{
		x += sub;
		y += sub;
		z += sub;

		return *this;
	}

	//スカラー倍(新しいベクトルを返す)
	Vector3 operator *(float sub)const
	{
		return{
			x * sub,
			y * sub,
			z * sub
		};
	}

	//スカラー倍(自分を書き換える)
	Vector3& operator *=(const float& sub)
	{
		x *= sub;
		y *= sub;
		z *= sub;

		return *this;
	}

	//スカラーで割る(自分を書き換える)。sub が 0 のときの確認はしない
	Vector3 operator /=(float sub)
	{
		x /= sub;
		y /= sub;
		z /= sub;

		return *this;
	}

	//スカラーで割る(新しいベクトルを返す)。sub が 0 のときの確認はしない
	Vector3 operator /(float sub)
	{
		return
		{
			x / sub,
			y / sub,
			z / sub
		};
	}
};


class Vector
{
public:
	//XMFLOAT3 -> Vector3。出力先 v を渡す形(v にも書き込み、同じ値を返す)
	static Vector3 fromF3ToV3(const XMFLOAT3& other, Vector3& v)
	{
		v.x = other.x;
		v.y = other.y;
		v.z = other.z;
		return v;
	}
	//XMFLOAT3 -> Vector3。値を返す形
	static Vector3 fromF3ToV3(const XMFLOAT3& other)
	{
		Vector3 v;
		v.x = other.x;
		v.y = other.y;
		v.z = other.z;
		return { v.x,v.y,v.z };
	}

	//Vector3 -> XMFLOAT3。値を返す形
	static XMFLOAT3 fromV3ToF3(const Vector3& other)
	{
		XMFLOAT3 m;
		m.x = other.x;
		m.y = other.y;
		m.z = other.z;
		return { m.x,m.y,m.z };
	}
	//Vector3 -> XMFLOAT3。出力先 m を渡す形(m にも書き込み、同じ値を返す)
	static XMFLOAT3 fromV3ToF3(const Vector3& other, XMFLOAT3& m)
	{
		m.x = other.x;
		m.y = other.y;
		m.z = other.z;
		return m;
	}

	//ベクトルを (0, 0, 0) にする
	static void Vector3Zero(Vector3& v)
	{
		v.x = 0.0f;
		v.y = 0.0f;
		v.z = 0.0f;
	}

	//長さの二乗(平方根を取らないので軽い。長さの比較に使う)
	static float SqMagnitude(const Vector3& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	//同じ向きなら正、直角なら 0、逆向きなら負。「ある向きにどれだけ向かっているか」を測るのに使う
	static float Dot(const Vector3& v1, const Vector3& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}


	//向きは右手系(a から b へ回したとき、右ねじの進む向き)
	static Vector3 Cross(const Vector3& v1, const Vector3& v2)
	{
		Vector3 result;

		result.x = v1.y * v2.z - v1.z * v2.y;
		result.y = v1.z * v2.x - v1.x * v2.z;
		result.z = v1.x * v2.y - v1.y * v2.x;

		return result;
	}

	//長さを 1 にしたベクトルを返す(元のベクトルは変えない)。
	//長さがほぼ 0(0.0001 以下)のときは、元のベクトルをそのまま返す。
	static Vector3 Normalized(const Vector3& v)
	{
		Vector3 result;
		result = v;

		float lenght = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);

		if (lenght > 0.0001f)
		{
			result.x /= lenght;
			result.y /= lenght;
			result.z /= lenght;
		}
		return result;
	}
};


struct Quaternion
{
	float x;
	float y;
	float z;
	float w;


	//ベクトルを、この回転で回す(q * v)。
	Vector3 operator*(Vector3 v)const
	{
		Vector3 qv = { x, y, z };

		Vector3 t = Vector::Cross(qv, v) * 2.0f;

		return v + t * w + Vector::Cross(qv, t);
	}

	//回転の合成(a * b = b を先に回してから a を回す)
	Quaternion operator*(const Quaternion& q) const
	{
		return {
			w * q.x + x * q.w + y * q.z - z * q.y,
			w * q.y - x * q.z + y * q.w + z * q.x,
			w * q.z + x * q.y - y * q.x + z * q.w,
			w * q.w - x * q.x - y * q.y - z * q.z
		};
	}

	//長さを 1 にする(回転として使うには、長さ 1 が必要)。
	//長さがほぼ 0(0.0001 未満)のときは、何もしない
	void Normalize()
	{
		float length = std::sqrt(
			x * x +
			y * y +
			z * z +
			w * w
		);

		if (length < 0.0001f)
			return;

		x /= length;
		y /= length;
		z /= length;
		w /= length;
	}

	//逆回転(元に戻す回転)を返す。DirectX の関数に任せている。
	Quaternion Inverse()
	{
		XMVECTOR q = XMVectorSet(x, y, z, w);

		XMVECTOR v = XMQuaternionInverse(q);

		XMFLOAT4 r;
		XMStoreFloat4(&r, v);

		return { r.x, r.y, r.z, r.w };
	}

};

class QuaternionMath
{
public:

	static void DecomposeSwingTwist(const Quaternion& delta,
		const Vector3& twistAxis, //正規化済みのローカル軸
		Quaternion& outSwing,
		Quaternion& outTwist)
	{
		Vector3 rotAxis(delta.x, delta.y, delta.z);

		//回転軸ベクトルをtwistAxis方向へ投影した成分がTwist
		float dot = Vector::Dot(rotAxis, twistAxis);
		const float twistLength = std::sqrt(dot * dot + delta.w * delta.w);
		if (twistLength < 1e-4f)
		{
			outTwist = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
			outSwing = delta;
			return;
		}
		Vector3 projection = twistAxis * dot;

		outTwist = Quaternion(projection.x, projection.y, projection.z, delta.w);
		outTwist.Normalize();

		//delta = Swing * Twist なので Swing = delta * Twist^-1
		outSwing = delta * outTwist.Inverse();
		outSwing.Normalize();
	}

	//クォータニオンをオイラー角(度)に直す。戻り値は (x, y, z) = (pitch, yaw, roll)。
	//【計算】標準の変換式。yaw は asin なので -90 90度の範囲になる(±90度付近はジンバルロック)
	static Vector3 ToEulerAngles(const Quaternion& q)
	{
		//逆算をして計算
		float sinPitch = 2.0f * (q.w * q.x + q.y * q.z);
		float cosPitch = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);

		float pitch = atan2f(sinPitch, cosPitch);

		float sinYaw = 2.0f * (q.w * q.y - q.z * q.x);
		sinYaw = std::clamp(sinYaw, -1.0f, 1.0f);

		float yaw = asinf(sinYaw);

		float sinRoll = 2.0f * (q.w * q.z + q.x * q.y);
		float cosRoll = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);

		float roll = atan2f(sinRoll, cosRoll);

		return Vector3(
			XMConvertToDegrees(pitch),
			XMConvertToDegrees(yaw),
			XMConvertToDegrees(roll)
		);
	}

	//回転 q のうち、axis まわりの回転成分(Twist)だけを取り出す。
	//DecomposeSwingTwist の、Twist だけを求める版
	static Quaternion ExtractTwist(
		const Quaternion& q,
		const Vector3& axis)
	{
		Vector3 qAxis(
			q.x,
			q.y,
			q.z
		);

		Vector3 projection =
			axis * Vector::Dot(qAxis, axis);

		Quaternion twist(
			projection.x,
			projection.y,
			projection.z,
			q.w
		);

		twist.Normalize();

		return twist;
	}


	//XMFLOAT4 -> Quaternion。値を返す形
	static Quaternion fromF4ToQ4(const XMFLOAT4& other)
	{
		return
		{
			other.x,
			other.y,
			other.z,
			other.w
		};
	}

	//オイラー角(度。x = pitch, y = yaw, z = roll)からクォータニオンを作る。
	//回転順は DirectX の XMQuaternionRotationRollPitchYaw に従う
	static Quaternion CreateFromEulerAngles(const Vector3& angle)
	{
		float x = XMConvertToRadians(angle.x);
		float y = XMConvertToRadians(angle.y);
		float z = XMConvertToRadians(angle.z);

		XMVECTOR q = XMQuaternionRotationRollPitchYaw(
			x,
			y,
			z
		);

		XMFLOAT4 result;
		XMStoreFloat4(&result, q);

		return Quaternion(
			result.x,
			result.y,
			result.z,
			result.w
		);
	}

	//XMFLOAT4 -> Quaternion。出力先 q を渡す形(q にも書き込み、同じ値を返す)
	static Quaternion fromF4ToQ4(const XMFLOAT4& other, Quaternion& q)
	{
		q.x = other.x;
		q.y = other.y;
		q.z = other.z;
		q.w = other.w;

		return q;
	}
	//Quaternion -> XMFLOAT4。出力先 m を渡す形(m にも書き込み、同じ値を返す)
	static XMFLOAT4 fromQ4ToF4(const Quaternion& other, XMFLOAT4& m)
	{
		m.x = other.x;
		m.y = other.y;
		m.z = other.z;
		m.w = other.w;

		return m;
	}
	//Quaternion -> XMFLOAT4。値を返す形
	static XMFLOAT4 fromQ4ToF4(const Quaternion& other)
	{
		XMFLOAT4 q;
		q.x = other.x;
		q.y = other.y;
		q.z = other.z;
		q.w = other.w;
		return q;
	}

	///<summary>
	///どの軸を基準に回すかを決める関数
	///</summary>
	///<param name="axis"></param>
	///<param name="angle"></param>
	///<returns></returns>
	//軸まわりに angle(ラジアン)回す回転を作る。
	//【計算】(axis * sin(angle/2), cos(angle/2))
	//axis は、長さ 1 に正規化してから渡すこと(正規化しないと、回転の大きさが狂う)
	static Quaternion CreateFromAxisAngle(const Vector3& axis, float angle)
	{
		float halfAngle = angle * 0.5f;

		float s = sinf(halfAngle);
		float c = cosf(halfAngle);

		return Quaternion(
			axis.x * s,
			axis.y * s,
			axis.z * s,
			c
		);
	}

	//2つの回転の間を、t(0～1)の割合で補間する(正規化つきの線形補間)。
	//slerp より軽く、角度の進み方は少し不均一になるが、近い回転同士なら差は小さい。
	//内積が負のときは、反対側の回転(-b)を使う(遠回りを避けて、最短の経路で補間する)
	static Quaternion Nlerp(const Quaternion& a, const Quaternion& b, float t)
	{
		//最短経路になるよう、内積が負なら反転
		Quaternion bb = b;
		float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		if (dot < 0.0f)
		{
			bb.x = -bb.x; bb.y = -bb.y; bb.z = -bb.z; bb.w = -bb.w;
		}

		Quaternion result;
		result.x = a.x + (bb.x - a.x) * t;
		result.y = a.y + (bb.y - a.y) * t;
		result.z = a.z + (bb.z - a.z) * t;
		result.w = a.w + (bb.w - a.w) * t;
		result.Normalize();
		return result;
	}
};
