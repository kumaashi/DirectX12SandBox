#include <stdio.h>
#include <string.h>
#include <DirectXMath.h>

union matrix {
	struct
	{
		float m00, m01, m02, m03;
		float m10, m11, m12, m13;
		float m20, m21, m22, m23;
		float m30, m31, m32, m33;
	};
	float m[16];
	void print()
	{
		for (int i = 0; i < 16; i++)
		{
			if ((i % 4) == 0)
				printf("\n");
			printf("%.4f, ", m[i]);
		}
	}
	void identity()
	{
		memset(m, 0, sizeof(m));
		m00 = m11 = m22 = m33 = 1;
	}
	void translate(float x, float y, float z)
	{
		m30 += x;
		m31 += y;
		m32 += z;
	}
	void scale(float x, float y, float z)
	{
		m00 *= x;
		m11 *= y;
		m22 *= z;
	}
};
int main() {
	matrix ctx;
	ctx.identity();
	ctx.scale(1, 2, 3);
	ctx.print();
        return 0;
}
