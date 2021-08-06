cbuffer MatrixBuffer {
	matrix mat;
	matrix viewMat;
	matrix projMat;
	float heightRatio;
	uint flag;
	uint2 reversed;
};

struct VS_INPUT
{
	float3 pos : POSITION;
	float2 uv : UV;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 color : COLOR;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 posV : POSITION2;
	float2 uv : UV;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 color : COLOR;
};


//Texture2D<float> heightTex;
//SamplerState sam;



VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	

	if (flag & 8 || flag & 16) {
		/*if (flag & 256) {
			float3 height0 = float3(-0.5f, heightTex.SampleLevel(sam, input.uv, 0, int2(-1, -1)).r * heightRatio, 0.5f);
			float3 height1 = float3(0.f, heightTex.SampleLevel(sam, input.uv, 0, int2(0, -1)).r * heightRatio, 0.5f);
			float3 height3 = float3(-0.5f, heightTex.SampleLevel(sam, input.uv, 0, int2(-1, 0)).r * heightRatio, 0.f);
			float3 height5 = float3(0.5f, heightTex.SampleLevel(sam, input.uv, 0, int2(1, 0)).r * heightRatio, 0.f);
			float3 height7 = float3(0.f, heightTex.SampleLevel(sam, input.uv, 0, int2(0, 1)).r * heightRatio, -0.5f);
			float3 height8 = float3(0.5f, heightTex.SampleLevel(sam, input.uv, 0, int2(1, 1)).r * heightRatio, -0.5f);

			input.normal = float3(0.f, 0.f, 0.f);
			input.normal += cross(height4 - height0, height3 - height0);
			input.normal += cross(height1 - height0, height4 - height0);
			input.normal += cross(height5 - height1, height4 - height1);
			input.normal += cross(height4 - height3, height7 - height3);
			input.normal += cross(height8 - height4, height7 - height4);
			input.normal += cross(height5 - height4, height8 - height4);
			input.normal = normalize(input.normal);
		}*/
		output.normal = mul(input.normal, (float3x3)mat);
		output.tangent = float3(0, 0, 0);

		if (flag & 32) {
			output.tangent = mul(input.tangent, (float3x3)mat);
		}
	}

	output.pos = mul(float4(input.pos, 1.f), mat);

	//float3 height4;
	//if (flag & 256) {
	//	height4 = float3(0, heightTex.SampleLevel(sam, input.uv, 0) * heightRatio, 0);
	//	output.posV.y += height4.y;
	//}

	output.posV = mul(output.pos, viewMat).xyz;
	output.pos = mul(float4(output.posV, 1.f), projMat);
	/*output.posV = posT.xyz / posT.w;*/




	output.color = input.color;

	return output;
}

