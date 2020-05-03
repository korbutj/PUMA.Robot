Texture2D lightMap : register(t0);
Texture2D shadowMap : register(t1);
SamplerState colorSampler : register(s0);

cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
}

cbuffer cbLights : register(b1)
{
	float4 lightPos;
};

cbuffer cbMapTransform : register(b2)
{
	matrix mapMtx;
}

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 worldPos : POSITION0;
	float3 norm : NORMAL0;
	float3 viewVec : TEXCOORD0;
};

static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);

//fourth element set to 0 so there will be no specular reflection
static const float4 defLightColor = float4(0.3f, 0.3f, 0.3f, 0.0f);

static const float kd = 0.5, ks = 0.2f, m = 100.0f;

float4 main(PSInput i) : SV_TARGET
{
	// TODO : 1.06 Calculate texture coordinates
	float4 trWorldPos = mul(mapMtx, float4(i.worldPos, 1.0f));
	trWorldPos = trWorldPos / trWorldPos.w;
	float3 viewVec = normalize(i.viewVec);
	float3 normal = normalize(i.norm);
	float3 lightVec = normalize(lightPos.xyz - i.worldPos);
	float3 halfVec = normalize(viewVec + lightVec);
	float3 color = surfaceColor.rgb * ambientColor;

	float4 lightColor = defLightColor;

	// TODO : 1.07 Determine light color based on light map
	lightColor = lightMap.Sample(colorSampler, trWorldPos.xy);
	// TODO : 1.09 Take into account the clipping plane when determining light color
	// TODO : 1.18 Include shadow map in light color calculation
	float3 shadowyBoi = shadowMap.Sample(colorSampler, trWorldPos.xy);

	if (lightColor.r < defLightColor.r ||
		i.worldPos.y > lightPos.y ||
		shadowyBoi.r < trWorldPos.z)
	{
		lightColor = defLightColor;
	}
	//if (shadowyBoi.r >= 0.5f && shadowyBoi.r <= 1.0f)
	//{
	//	lightColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
	//}

	color += lightColor.rgb * surfaceColor.xyz * kd * saturate(dot(normal, lightVec)); //diffuse color
	float nh = dot(normal, halfVec);
	nh = saturate(nh);
	nh = pow(nh, m);
	nh *= ks;

	//specular reflection multiplied by fourth component of light color
	//there will be no specular reflection for default light color
	//and where lightMap is transparent
	color += lightColor.rgb*lightColor.a * nh;

	return float4(saturate(color), surfaceColor.a);
}