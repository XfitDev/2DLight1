cbuffer ColorMatrixBuffer {
	matrix colorMat;
	uint flag;
	float diffuse;
	float specular;
	float ambient;
	float3 viewPos;
	float reversed;
};


struct Light {
	uint type;
	float3 direction;
	float3 pos;
	float3 color;
	float spot;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float3 posV : POSITION2;
	float2 uv : UV;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 color : COLOR;
};

Texture2D tex;
Texture2D normal;
SamplerState sam;

StructuredBuffer<Light> lights;

TextureCube reflectTex;

Texture2D specularTex;

float4 main(PS_INPUT _input) :SV_Target{
	float4 outColor;
	if (flag & 4) {
		outColor = mul(_input.color, colorMat);
	} else outColor = mul(tex.Sample(sam, _input.uv), colorMat);


	if (flag & 16) {
		if (flag & 32) {
			float3 normalT = 2.f * normal.Sample(sam, _input.uv).xyz - 1.f;
			float3 N = _input.normal;
			float3 T = normalize(_input.tangent - dot(_input.tangent, N) * N);
			float3 B = cross(N, T);

			float3x3 TBN = float3x3(T, B, N);

			_input.normal = mul(normalT, TBN);
		} else {
			_input.normal = normalize(_input.normal);
		}
		float specularT = specular;
		if (flag & 512) {
			specularT *= 1;
		}

		if (flag & 128) {
			float re = specularTex.Sample(sam, _input.uv).x;
			outColor = outColor * (1.f - specularT * re) + reflectTex.Sample(sam, _input.normal) * specularT * re;
		}

		uint lightLen, stride;
		lights.GetDimensions(lightLen, stride);

		float3 oColor = outColor.rgb;
		outColor.rgb = float3(0.f, 0.f, 0.f);
		for (uint i = 0; i < lightLen; i++) {
			if (lights[i].type == 0) {
				float ld = dot(_input.normal, -lights[i].direction);
				if (ld > 0.f) {
					if (specularT > 0.f) {
						outColor.rgb += (ld * diffuse +
							ambient) * lights[i].color * oColor + pow(max(dot(reflect(lights[i].direction, _input.normal), normalize(viewPos - _input.posV)), 0), 3) * specularT * lights[i].color * 0.5;
					} else {
						outColor.rgb += (ld * diffuse + ambient) * oColor * lights[i].color;
					}
				} else {
					outColor.rgb += ambient * oColor * lights[i].color;
				}
			} else if (lights[i].type == 1) {
				float dis = length(_input.posV - lights[i].pos);
				if (dis < 50.f) {
					float3 direction = normalize(_input.posV - lights[i].pos);
					float ld = dot(_input.normal, -direction);
					if (ld > 0.f) {
						if (specularT > 0.f) {
							outColor.rgb += ((ld * diffuse +
								pow(max(dot(reflect(direction, _input.normal), normalize(viewPos - _input.posV)), 0), 3) * specularT) / dis
								+ ambient) * lights[i].color * oColor;
						} else {
							outColor.rgb += ((ld * diffuse) / dis + ambient) * oColor * lights[i].color;
						}
					} else {
						outColor.rgb += ambient * oColor * lights[i].color;
					}
				}
			} else if (lights[i].type == 2) {
				float dis = length(_input.posV - lights[i].pos);
				if (dis < 50.f) {
					float3 direction = normalize(_input.posV - lights[i].pos);
					float ld = dot(_input.normal, -direction);
					if (ld > 0.f) {
						float spot = pow(max(dot(direction, lights[i].direction), 0), lights[i].spot);

						if (specularT > 0.f) {
							outColor.rgb += ((ld * diffuse +
								pow(max(dot(reflect(direction, _input.normal), normalize(viewPos - _input.posV)), 0), 3) * specularT) / dis * spot
								+ ambient) * lights[i].color * oColor;
						} else {
							outColor.rgb += ((ld * diffuse) / dis * spot + ambient) * oColor * lights[i].color;
						}
					} else {
						outColor.rgb += ambient * oColor * lights[i].color;
					}
				}
			}
		}
	}

	return outColor;
}
