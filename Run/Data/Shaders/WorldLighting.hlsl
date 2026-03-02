Texture2D    diffuseTexture  : register(t0);
SamplerState diffuseSampler  : register(s0);

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float3   CameraPosition;
    float    _pad0;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4   ModelColor;
};

cbuffer WorldConstants : register(b4)
{
    float4 IndoorLightColor;    
    float4 OutdoorLightColor;   
    float4 SkyColor;            

    float  FogNearDistance;     
    float  FogFarDistance;      
    float  GlobalLightScalar; 
    float  _pad;
}


struct vs_input_t
{
    float3 modelSpacePosition : POSITION;
    float4 color              : COLOR0;
    float2 uv                 : TEXCOORD0;
};

struct v2p_t
{
    float4 clipSpacePosition : SV_Position;
    float4 color             : COLOR0;
    float2 uv                : TEXCOORD0;
    float3 worldPos          : TEXCOORD1;
};

float RangeMap01(float v, float a, float b)
{
    return saturate( (v - a) / max(b - a, 1e-6f) );
}

v2p_t VertexMain(vs_input_t input)
{
    v2p_t o;

    float4 modelPos  = float4(input.modelSpacePosition, 1.0f);
    float4 worldPos  = mul(ModelToWorldTransform,   modelPos);
    float4 cameraPos = mul(WorldToCameraTransform,  worldPos);
    float4 renderPos = mul(CameraToRenderTransform, cameraPos);
    float4 clipPos   = mul(RenderToClipTransform,   renderPos);

    o.clipSpacePosition = clipPos;
    o.color   = input.color;
    o.uv      = input.uv;
    o.worldPos = worldPos.xyz;

    return o;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 tex = diffuseTexture.Sample(diffuseSampler, input.uv);

    // ??????????
    float3 totalLight = saturate(input.color.rgb);

    // a ????????0 = ?????, 1 = ?????
    float  skyFactor = saturate(input.color.a);

    // ?????? ? ???
    float3 skyLight   = totalLight * skyFactor;
    float3 blockLight = totalLight * (1.0f - skyFactor);

    // ??????????
    skyLight *= GlobalLightScalar;

    float3 finalLight = skyLight + blockLight;

    // ?????
    float3 litRgb = tex.rgb * finalLight;

    float alpha = tex.a * ModelColor.a;
    clip(alpha - 0.01f);

    float distToCam = distance(input.worldPos, CameraPosition.xyz);
    float fogFrac   = RangeMap01(distToCam, FogNearDistance, FogFarDistance);
    float fogAlpha  = saturate(fogFrac * SkyColor.a);

    float3 foggedRgb = lerp(litRgb, SkyColor.rgb, fogAlpha);
    foggedRgb *= ModelColor.rgb;

    return float4(foggedRgb, alpha);
}



