#pragma once
#include "PL_base_defs.h"
#include <intrin.h>

//-----------------------------------------------
#define MAX_FLOAT          3.402823466e+38F        // max value
#define MIN_FLOAT          1.175494351e-38F        // min normalized positive value
#define UINT32MAX		   0xffffffff			
#define INV_UINT32_MAX	   2.328306437e-10F

//-----------------------------------------------


// Operations

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

//-----------------------
void buffer_free(void* buffer);
void* buffer_calloc(size_t size);
void* buffer_realloc(void* block, size_t new_size);
//-----------------------
// Collections
//----------------------
// DYNAMIC BUFFER
template<typename t, int32 capacity__ = 1, int32 overflow__ = 5, typename size_type = int32>
struct DBuffer
{
	size_type length = 0;
	size_type capacity = capacity__;
	size_type overflow_addon = overflow__;
	t* front = 0;

	inline void add(t new_member)
	{
		length++;
		if (front == 0)		//Buffer was not initilized and is being initialized here. 
		{
			front = (t*)buffer_calloc(capacity * sizeof(t));
		}
		if (length > capacity)
		{
			capacity = capacity + overflow_addon;
			t* temp = (t*)buffer_realloc(front, capacity * sizeof(t));
			ASSERT(temp);	//Not enough memory to realloc, or buffer was never initialized and realloc is trying to allocate to null pointer
			front = temp;
		}

		t* temp = front;
		temp = temp + (length - 1);
		*temp = new_member;
	}

	//Same as add but doesn't perform copy. (Use for BIG objects)
	inline void add_nocpy(t& new_member)
	{
		length++;
		if (front == 0)		//Buffer was not initilized and is being initialized here. 
		{
			front = (t*)buffer_calloc(capacity * sizeof(t));
		}
		if (length > capacity)
		{
			capacity = capacity + overflow_addon;
			t* temp = (t*)buffer_realloc(front, capacity * sizeof(t));
			ASSERT(temp);	//Not enough memory to realloc, or buffer was never initialized and realloc is trying to allocate to null pointer
			front = temp;
		}

		t* temp = front;
		temp = temp + (length - 1);
		*temp = new_member;
	}

	//Clears memory and resets length.
	FORCEDINLINE void clear_buffer()
	{
		length = 0;
		buffer_free(front);
	}

	FORCEDINLINE t& operator [](size_type index)
	{
		ASSERT(index >= 0 && index < (size_type)length);
		return (front[index]);
	}
};

// FIXED DYNAMIC BUFFER
//A wrapper for a non-resizable dynamic buffer 
template<typename t, typename size_type = int32>
struct FDBuffer
{
	size_type size;
	t* front = 0;

	//Allocates memory (initilizes to default memory of the type) and returns pointer to allocation
	inline t* allocate_preserve_type_info(int32 size_)
	{
		t tmp;
		t* front_temp = allocate(size_);
		for (int i = 0; i < size_; i++)
		{
			*front_temp++ = tmp;
		}
		return front;
	}

	//Allocates memory (0 initilized) and returns pointer to allocation
	FORCEDINLINE t* allocate(size_type size_)
	{
		size = size_;
		front = (t*)buffer_calloc(size * sizeof(t));
		return front;
	}
	//clears size and deallocates memory 
	FORCEDINLINE void clear()
	{
		size = 0;
		buffer_free(front);
	}
	FORCEDINLINE t& operator [](size_type index)
	{
		ASSERT(index >= 0 && index < size);
		return (front[index]);
	}
};


//-----------------------
// Math Data Types
//----------------------
// VEC2
template <typename t>
struct Vec2
{
	union
	{
		struct { t x, y; };
		t raw[2];
	};

	//constructors
	//Vec2() :x(0), y(0) {}
	//Vec2(t _x, t _y) : x(_x), y(_y) {}

	FORCEDINLINE t& operator [] (int32 i) { return raw[i]; };

	FORCEDINLINE Vec2<t> operator + (Vec2<t> n) { Vec2<t> ans = { x + n.x, y + n.y }; return ans; };
	FORCEDINLINE void operator += (Vec2<t> n) { x += n.x; y += n.y; };

	FORCEDINLINE Vec2<t> operator - (Vec2<t> n) { Vec2<t> ans = { x - n.x, y - n.y }; return ans; };
	FORCEDINLINE void operator -= (Vec2<t> n) { x -= n.x; y -= n.y; };

	FORCEDINLINE Vec2<t> operator * (f32 n) { Vec2<t> ans = { x * n, y * n }; return ans; }
	FORCEDINLINE void operator *= (f32 n) { x *= n; y *= n; };

	FORCEDINLINE Vec2<t> operator / (f32 n) { Vec2<t> ans = { x / n, y / n }; return ans; };

};
typedef Vec2<f32> vec2f;
typedef Vec2<uint32> vec2ui;
typedef Vec2<int32> vec2i;
typedef Vec2<uint8> vec2b;


//----------------------
// VEC3
template <typename t>
struct Vec3
{
	union
	{
		struct {
			t x;
			t y;
			t z;
		};
		struct {
			t r;
			t g;
			t b;
		};
		t raw[3] = { 0,0,0 };
	};

	//constructors
	//Vec3() : x(0), y(0), z(0) {}
	//Vec3(t _x, t _y, t _z) : x(_x), y(_y), z(_z) {}

	FORCEDINLINE t& operator [] (int32 i) { return raw[i]; };

	FORCEDINLINE Vec3<t> operator + (Vec3<t> n) { Vec3<t> ans = { x + n.x, y + n.y, z + n.z }; return ans; };
	FORCEDINLINE void operator += (Vec3<t> n) { x += n.x; y += n.y; z += n.z; };

	FORCEDINLINE Vec3<t> operator - () { Vec3<t> ans = { -x, -y, -z }; return ans; };

	FORCEDINLINE Vec3<t> operator - (Vec3<t> n) { Vec3<t> ans = { x - n.x, y - n.y, z - n.z }; return ans; };
	FORCEDINLINE void operator -= (Vec3<t> n) { x -= n.x; y -= n.y; z -= n.z; };

	FORCEDINLINE bool operator != (Vec3<t> b) { return (x == b.x && (y == b.y && z == b.z)); };

	FORCEDINLINE Vec3<t> operator * (f32 n) { Vec3<t> ans = { x * n, y * n, z * n }; return ans; };
	FORCEDINLINE void operator *= (f32 n) { x *= n; y *= n; z *= n; };

	FORCEDINLINE Vec3<t> operator / (f32 n) { Vec3<t> ans = { x / n, y / n, z / n }; return ans; };
	FORCEDINLINE Vec3<f32> inverse() { Vec3<t> ans = { 1 / x,1 / y,1 / z }; return ans; };

};
typedef Vec3<f32> vec3f;
typedef Vec3<uint32> vec3ui;
typedef Vec3<int32> vec3i;
typedef Vec3<uint8> vec3b;



//----------------------
// VEC4
template <typename t>
struct Vec4
{
	union
	{
		t raw[4] = { 0, 0, 0 ,0 };
		struct {
			t x;
			t y;
			t z;
			t w;
		};
		struct {
			Vec2<t> left_bottom, right_top;
		};
		struct {
			t r;
			t g;
			t b;
			t a;
		};
	};

	//constructors
	/*Vec4() : x(0),y(0),z(0),w(0) {}
	Vec4(Vec2<t> left_top_,Vec2<t> right_bottom_) : left_top(left_top), right_bottom(right_bottom_) {}
	Vec4(t _x, t _y, t _z, t _w) : x(_x), y(_y), z(_z), w(_w) {}
	Vec4( Vec3<t> &replace, t _w) : x(replace.x), y(replace.y), z(replace.z), w(_w) {}*/

	FORCEDINLINE t& operator [] (int32 i) { return raw[i]; };

	FORCEDINLINE Vec4<t> operator + (Vec4<t> n) { Vec4<t> ans = { x + n.x, y + n.y, z + n.z, w + n.w }; return ans; }
	FORCEDINLINE void operator += (Vec4<t> n) { x += n.x; y += n.y; z += n.z; w += n.w; }

	FORCEDINLINE Vec4<t> operator - (Vec4<t> n) { Vec4<t> ans = { x - n.x, y - n.y, z - n.z, w - n.w }; return ans; }
	FORCEDINLINE void operator -= (Vec4<t> n) { x -= n.x; y -= n.y; z -= n.z; w -= n.w; }

	FORCEDINLINE bool operator != (Vec4<t> b) { return (z == b.z && (x == b.x && (y == b.y && z == b.z))); };

	FORCEDINLINE Vec4<t> operator * (f32 n) { Vec4<t> ans = { x * n, y * n, z * n, w * n }; return ans; }
	FORCEDINLINE void operator *= (f32 n) { x *= n; y *= n; z *= n; w *= n; }

	FORCEDINLINE Vec4<t> operator / (f32 n) { Vec4<t> ans = { x / n, y / n, z / n, w / n }; return ans; }
	FORCEDINLINE f32 operator * (Vec4<t> n) { return (x * n.x) + (y * n.y) + (z * n.z); }

};
typedef Vec4<f32> vec4f;
typedef Vec4<uint32> vec4ui;
typedef Vec4<int32> vec4i;
typedef Vec4<uint8> vec4b;
typedef Vec4<int32> Tile;


//----------------------
// MAT3x3
template<typename t>
struct Mat33
{
	union
	{
		t raw[3][3] = { 1,0,0,
						0,1,0,
						0,0,1 };

		struct
		{
			t xi;
			t xj;
			t xk;
			t yi;
			t yj;
			t yk;
			t zi;
			t zj;
			t zk;
		};
	};

	FORCEDINLINE t& at(int32 row, int32 column) { return raw[row * 3 + column]; }

	FORCEDINLINE Mat33<t> operator + (Mat33<t>& n) {
		Mat33<t> ans = {
			raw[0] + n.raw[0], raw[1] + n.raw[1], raw[2] + n.raw[2],
			raw[3] + n.raw[3], raw[4] + n.raw[4], raw[5] + n.raw[5],
			raw[6] + n.raw[6], raw[7] + n.raw[7], raw[8] + n.raw[8] };
		return ans;
	}

	FORCEDINLINE Mat33<t> operator - (Mat33<t>& n) {
		Mat33<t> ans = {
			raw[0] - n.raw[0], raw[1] - n.raw[1], raw[2] - n.raw[2],
			raw[3] - n.raw[3], raw[4] - n.raw[4], raw[5] - n.raw[5],
			raw[6] - n.raw[6], raw[7] - n.raw[7], raw[8] - n.raw[8] };
		return ans;
	}

	FORCEDINLINE Mat33<t> operator * (t n) {
		Mat33<t> ans = {
			raw[0] * n, raw[1] * n, raw[2] * n,
			raw[3] * n, raw[4] * n, raw[5] * n,
			raw[6] * n, raw[7] * n, raw[8] * n };
		return ans;
	}

	FORCEDINLINE Vec3<t> operator * (Vec3<t>& n) {
		Vec3<t> ans = { xi * n.x + xj * n.y + xk * n.z, yi * n.x + yj * n.y + yk * n.z, zi * n.x + zj * n.y + zk * n.z };
		return ans;

	}

	FORCEDINLINE Mat33<t> operator * (Mat33<t>& n) {
		Mat33<t> ans = {
				xi * n.xi + xj * n.yi + xk * n.zi,
				xi * n.xj + xj * n.yj + xk * n.zj,
				xi * n.xk + xj * n.yk + xk * n.zk,
				yi * n.xi + yj * n.yi + yk * n.zi,
				yi * n.xj + yj * n.yj + yk * n.zj,
				yi * n.xk + yj * n.yk + yk * n.zk,
				zi * n.xi + zj * n.yi + zk * n.zi,
				zi * n.xj + zj * n.yj + zk * n.zj,
				zi * n.xk + zj * n.yk + zk * n.zk
		};
		return ans;
	}
};
//----------------------
// MAT4x4
template<typename t>
struct Mat44
{
	t raw[4 * 4] = { 1,0,0,0,
					 0,1,0,0,
					 0,0,1,0,
					 0,0,0,1 };

	FORCEDINLINE t& at(int32 row, int32 column) { return raw[row * 4 + column]; }

	FORCEDINLINE Mat44<t> operator + (Mat44<t>& n) {
		Mat44<t> ans = {
			raw[0] + n.raw[0], raw[1] + n.raw[1], raw[2] + n.raw[2], raw[3] + n.raw[3],
			raw[4] + n.raw[4], raw[5] + n.raw[5], raw[6] + n.raw[6], raw[7] + n.raw[7],
			raw[8] + n.raw[8], raw[9] + n.raw[9], raw[10] + n.raw[10], raw[11] + n.raw[11],
			raw[12] + n.raw[12], raw[13] + n.raw[13], raw[14] + n.raw[14], raw[15] + n.raw[15] };
		return ans;
	}

	FORCEDINLINE Mat44<t> operator - (Mat44<t>& n) {
		Mat44<t> ans = {
			raw[0] - n.raw[0], raw[1] - n.raw[1], raw[2] - n.raw[2], raw[3] - n.raw[3],
			raw[4] - n.raw[4], raw[5] - n.raw[5], raw[6] - n.raw[6], raw[7] - n.raw[7],
			raw[8] - n.raw[8], raw[9] - n.raw[9], raw[10] - n.raw[10], raw[11] - n.raw[11],
			raw[12] - n.raw[12], raw[13] - n.raw[13], raw[14] - n.raw[14], raw[15] - n.raw[15] };
		return ans;
	}

	FORCEDINLINE Mat44<t> operator * (t n) {
		Mat44<t> ans = {
			raw[0] * n, raw[1] * n, raw[2] * n, raw[3] * n,
			raw[4] * n, raw[5] * n, raw[6] * n, raw[7] * n,
			raw[8] * n, raw[9] * n, raw[10] * n, raw[11] * n,
			raw[12] * n, raw[13] * n, raw[14] * n, raw[15] * n };
		return ans;
	}

	FORCEDINLINE Vec4<t> operator * (Vec4<t>& n) {
		Vec4<t> ans = {
			raw[0] * n.raw[0] + raw[1] * n.raw[1] + raw[2] * n.raw[2] + raw[3] * n.raw[3],
			raw[4] * n.raw[0] + raw[5] * n.raw[1] + raw[6] * n.raw[2] + raw[7] * n.raw[3],
			raw[8] * n.raw[0] + raw[9] * n.raw[1] + raw[10] * n.raw[2] + raw[11] * n.raw[3],
			raw[12] * n.raw[0] + raw[13] * n.raw[1] + raw[14] * n.raw[2] + raw[15] * n.raw[3] };
		return ans;
	}

	Mat44<t> operator * (Mat44<t>& n) {
		Mat44<t> ans;
		for (int32 j = 0; j < 4; j++)
		{

			ans.raw[j * 4 + 0] = raw[j * 4 + 0] * n.raw[0] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[8] + raw[j * 4 + 3] * n.raw[12];
			ans.raw[j * 4 + 1] = raw[j * 4 + 0] * n.raw[1] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[9] + raw[j * 4 + 3] * n.raw[13];
			ans.raw[j * 4 + 2] = raw[j * 4 + 0] * n.raw[2] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[10] + raw[j * 4 + 3] * n.raw[14];
			ans.raw[j * 4 + 3] = raw[j * 4 + 0] * n.raw[3] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[11] + raw[j * 4 + 3] * n.raw[15];

		}
		return ans;
	}
};
//----------------------
//----------------------


//----------------------
// General Math
//----------------------
template<typename t>
FORCEDINLINE t min(t a, t b)
{
	return a > b ? b : a;
}
template<typename t>
FORCEDINLINE t max(t a, t b)
{
	return a > b ? a : b;
}

//TODO: get rid of this sometime later when you feel like it...no rush tho
#include <math.h>
FORCEDINLINE f64 dpow(f64 base, f64 exponent)
{
	return pow(base, exponent);
}

FORCEDINLINE f32 fpow(f32 base, f32 exponent)
{
	return powf(base, exponent);
}

FORCEDINLINE f64 sqroot(f64 real64)
{
	f64 result = _mm_cvtsd_f64(_mm_sqrt_pd(_mm_set_sd(real64)));
	return result;
}

FORCEDINLINE f32 sqroot(f32 real32)
{
	f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(real32)));
	return result;
}

//----<Vec2>-----
FORCEDINLINE f32 mag2(vec2f vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) };
}

FORCEDINLINE f32 mag(vec2f vector)
{
	return { sqroot(mag2(vector)) };
}
//----</Vec2>----

//----<Vec3>-----
FORCEDINLINE f32 mag2(vec3f vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) };
}

FORCEDINLINE f32 mag(vec3f vector)
{
	return { sqroot(mag2(vector)) };
}

FORCEDINLINE b32 normalize(vec3f& v)
{
	if (mag2(v) == 1.0f)
	{
		return false;
	}
	v = v / mag(v);
	return true;
}
//----</Vec3>----

//----<Vec4>-----
FORCEDINLINE f32 mag2(vec4f vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) + (vector.w * vector.w) };
}

FORCEDINLINE f32 mag(vec4f vector)
{
	return { sqroot(mag2(vector)) };
}

FORCEDINLINE b32 normalize(vec4f& v)
{
	if (mag2(v) == 1.0f)
	{
		return false;
	}
	v = v / mag(v);
	return true;
}
//----</Vec4>----


//Returns |p|*|n|*cos(theta) 
FORCEDINLINE f32 dot(vec3f p, vec3f n) { return (p.x * n.x) + (p.y * n.y) + (p.z * n.z); };

//Returns vector as result of multiplication of individual components
FORCEDINLINE vec3f hadamard(vec3f a, vec3f b) { vec3f ans = { a.x * b.x, a.y * b.y, a.z * b.z }; return ans; }

//Returns |a|*|b|* sin(theta) * n_cap(n_cap is normalized perpendicular to a and b)
FORCEDINLINE vec3f cross(vec3f a, vec3f b) { vec3f ans = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; return ans; }

//used to interpolate between vectors. value should be between 0 and 1.
FORCEDINLINE vec2f lerp(vec2f start, vec2f towards, f32 interpolation)
{
	return { (towards - start) / interpolation };
}

FORCEDINLINE vec3f clamp(vec3f v, f32 lower, f32 upper)
{
	v.x = max(lower, min(v.x, upper));
	v.y = max(lower, min(v.y, upper));
	v.z = max(lower, min(v.z, upper));
	return v;
}

//used to interpolate between vectors. value should be between 0 and 1.
FORCEDINLINE vec3f lerp(vec3f start, vec3f towards, f32 interpolation)
{
	return { ((towards - start) * interpolation) + start };
}

//used to interpolate between vectors. value should be between 0 and 1.
FORCEDINLINE vec4f lerp(vec4f start, vec4f towards, f32 interpolation)
{
	return { ((towards - start) * interpolation) + start };
}

//proper conversion of linear to srgb color space
inline f32 linear_to_srgb(f32 l)
{
	if (l < 0)
	{
		l = 0;
	}
	if (l > 1.0f)
	{
		l = 1.0f;
	}
	f32 s = l * 12.92f;;
	if (l > 0.0031308f)
	{
		s = 1.055f * fpow(l, 1.0f / 2.4f) - 0.055f;
	}
	return s;
}

//TODO: make SIMD version maybe?
FORCEDINLINE vec3f linear_to_srgb(vec3f l)
{
	vec3f srgb;
	srgb.r = linear_to_srgb(l.r);
	srgb.g = linear_to_srgb(l.g);
	srgb.b = linear_to_srgb(l.b);
	return srgb;
}

FORCEDINLINE vec3f rgb_gamma_correct(vec3f color)
{
	vec3f gamma_correct;
	gamma_correct.r = sqroot(color.r);
	gamma_correct.g = sqroot(color.g);
	gamma_correct.b = sqroot(color.b);
	return gamma_correct;
}

//used to convert float 0.0 to 1.0 rgb values to a byte vector
FORCEDINLINE vec3b rgb_float_to_byte(vec3f color) { vec3b ans = { (uint8)(color.r * 255.0f), (uint8)(color.g * 255.0f), (uint8)(color.b * 255.0f) };	return ans; }


struct RNG_Stream
{
	uint64 state;	//Used to keep track of streams position
	uint64 stream;	//the actual "seed". Determines which stream the RNG uses. 
					//NOTE: stream must be odd value (sequential streams will produce same result)
};

#ifdef PL_COMPILER_MSVC
	#if _MSC_VER > 1800
	#pragma warning(disable:4146)	//disables the "loss of data u32 to u64" conversion warning
	#pragma warning(disable:4244)	//disables the negation on unsigned int (rot) warning
	#endif
#endif
//implemented from https://www.pcg-random.org/
FORCEDINLINE uint32 random_u32(RNG_Stream* stream)
{
	//updating the stream state
	uint64 oldstate = stream->state;
	stream->state = oldstate * 6364136223846793005ULL + (stream->stream | 1); //"stream" must be odd value

	//generating number from stream:
	uint32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32 rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
#ifdef PL_COMPILER_MSVC
	#if _MSC_VER > 1800
	#pragma warning(default:4244)  
	#pragma warning(default:4146)  
	#endif
#endif

//returns random number between 0 and 1
FORCEDINLINE f32 rand_uni(RNG_Stream* stream)
{
	//NOTE: multiplying with 2.328306437e-10F to avoid division (make fast).
	//		this leads to 1.0f not being achievable
	f32 rd = (f32)random_u32(stream) * INV_UINT32_MAX;

	return rd;
}

//returns random number between -1 and 1
FORCEDINLINE f32 rand_bi(RNG_Stream* stream)
{
	//NOTE: rand_uni multiplying with 2.328306437e-10F to avoid division (make fast).
	//		this leads to 1.0f, 0.0f and -1.0f not being achievable
	f32 rd = -1.0f + 2.0f * rand_uni(stream);
	return rd;
}
//---------------------------------------------------------------------------

//---------------------------<ATOMICS>---------------------------------------
#ifdef PL_X64
#pragma intrinsic(_InterlockedExchangeAdd64)
//NOTE: Doesn't work on x86 
//performs atomic add and returns result(for int64 value)
FORCEDINLINE int64 interlocked_add_i64(volatile int64* data, int64 value)
{
	int64 Old = _InterlockedExchangeAdd64(data, value);
	return Old + value;
}
#endif

#pragma intrinsic(_InterlockedExchangeAdd)
//performs atomic add and returns result(for int32 value)
FORCEDINLINE int32 interlocked_add_i32(volatile int32* data, int32 value)
{
	int32 Old = _InterlockedExchangeAdd((long*)data, value);
	return Old + value;
}

#ifdef PL_X64
#pragma intrinsic(_InterlockedIncrement64)
//NOTE: Doesn't work on x86 
//returns resulting incremented value after performing locked increment(for int64 value)
FORCEDINLINE int64 interlocked_increment_i64(volatile int64* data)
{
	return _InterlockedIncrement64(data);
}
#endif

#pragma intrinsic(_InterlockedIncrement)
//returns resulting decremented value after performing locked decrement(for int32 value)
FORCEDINLINE int64 interlocked_increment_i32(volatile int32* data)
{
	return _InterlockedIncrement((volatile long*)data);
}

#ifdef PL_X64
#pragma intrinsic(_InterlockedDecrement64)
//NOTE: Doesn't work on x86 
//returns resulting decremented value after performing locked decrement(for int64 value)
FORCEDINLINE int64 interlocked_decrement_i64(volatile int64* data)
{
	return _InterlockedDecrement64(data);
}
#endif

#pragma intrinsic(_InterlockedDecrement)
//returns resulting decremented value after performing locked decrement(for int32 value)
FORCEDINLINE int64 interlocked_decrement_i32(volatile int32* data)
{
	return _InterlockedDecrement((volatile long*)data);
}

#ifdef PL_X64

#pragma intrinsic(_InterlockedExchange64)
//NOTE: Doesn't work on x86 
//returns previous value after exchanging with new value
FORCEDINLINE int64 interlocked_exchange_i64(volatile int64* data, int64 value)
{
	return _InterlockedExchange64((volatile long long*)data, value);
}
#endif

#pragma intrinsic(_InterlockedExchange)
//returns previous value after exchanging with new value
FORCEDINLINE int32 interlocked_exchange_i32(volatile int32* data, int32 value)
{
	return _InterlockedExchange((volatile long*)data, value);
}

//---------------------------</ATOMICS>--------------------------------------
