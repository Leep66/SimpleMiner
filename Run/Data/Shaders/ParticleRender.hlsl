cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float3   CameraPosition;
    float    _pad0;
};

struct Particle
{
    float3 Position;
    float  Lifetime;
    float3 Velocity;
    float  MaxLifetime;
    float4 Color;
    float  Size;
    float  UseLight;
    float  Emissive;
    float  Stage;
};

StructuredBuffer<Particle> Particles : register(t0);

Texture2D ParticleTexture : register(t1);
SamplerState ParticleSampler : register(s0);

struct ParticleVertexInput
{
    uint VertexID   : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
};

static const float2 kQuadCorners[4] = {
    float2(-0.5, -0.5),
    float2( 0.5, -0.5),
    float2(-0.5,  0.5),
    float2( 0.5,  0.5)
};

static const float2 kQuadUVs[4] = {
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0) 
};


static const uint kQuadIndices[6] = { 0, 1, 2, 2, 1, 3 };

VSOutput ParticleVertexMain(ParticleVertexInput input)
{
    VSOutput o;

    Particle p = Particles[input.InstanceID];

    uint lid = input.VertexID % 6;
    uint ci = kQuadIndices[lid];

    float2 quad = kQuadCorners[ci] * p.Size;

    // Billboard basis
    float3 worldPos = p.Position;

    float3 viewDir = normalize(CameraPosition - worldPos);

    float3 right   = normalize(cross(float3(0,0,1), viewDir));
    float3 up      = normalize(cross(viewDir, right));

    float3 billboardPos = worldPos + right * quad.x + up * quad.y;

    float4 wp = float4(billboardPos, 1);
    float4 cp = mul(WorldToCameraTransform, wp);
    float4 rp = mul(CameraToRenderTransform, cp);
    float4 clip = mul(RenderToClipTransform, rp);

    o.Position = clip;

    o.Color = p.Color;

    o.UV = kQuadUVs[ci];

    return o;
}




float4 PixelMain(VSOutput input) : SV_Target
{
    float4 tex = ParticleTexture.Sample(ParticleSampler, input.UV);
    float4 color = tex * input.Color;

    // alpha cutoff
    if (color.a < 0.1f) discard;

    return color;
}


