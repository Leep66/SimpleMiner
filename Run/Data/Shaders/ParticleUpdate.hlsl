cbuffer ParticleSystemConstants : register(b0)
{
    float4x4 ModelMatrix;
    float    DeltaTime;
    float    SystemTime;
    float2   Padding;
};

cbuffer ParticleEmitterConstants : register(b1)
{
    float3 EmitterPosition;   float _pad0;
    float3 SpawnExtent;       float _pad1;
    float3 BaseVelocity;      float _pad2;
    float3 VelocityVariance;  float _pad3;
    float  ParticleLifetime; 
    float  LifetimeVariance;
    float  StartSize;
    float  EndSize;
    float4 StartColor;
    float4 EndColor;
    uint   SpawnBudget;
    uint   RandomSeed; 
    float  NoiseStrength;
    float  NoiseFrequency;

    float4 SubstageStartColor;
    float4 SubstageEndColor;

    float  SubstageLifetime;
    float  SubstageLifetimeVariance;
    float  SubstageStartSize;
    float  SubstageEndSize;

    float3 SubstageBaseVelocity;
    uint   UseSubstage;

    float3 SubstageVelocityVariance;
    float  substageProb;

    uint   MainBillboardType;
    uint   SubstageBillboardType;
    float2 _padBillboard;
};

static const uint BILLBOARD_WORLD_UP_FACING  = 1u;
static const uint BILLBOARD_WORLD_UP_OPPOSING= 2u;
static const uint BILLBOARD_FULL_FACING      = 3u;
static const uint BILLBOARD_FULL_OPPOSING    = 4u;

cbuffer CullingConstants : register(b5)
{
    float4x4 WorldToCamera;
    float4x4 CameraToRender;
    float4x4 RenderToClip;
    float3   CameraPos;   float _pad5;
    float2   ViewportSize;
    float    MinPixelPx;
    float    FrustumPad;
    float    LODNearDist;
    float    LODFarDist;
    float    LODMaxSkip;
    float    _pad6;
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


static const uint FORCE_GRAVITY   = 0;
static const uint FORCE_POINT     = 1;
static const uint FORCE_DIRECTION = 2;

struct ParticleForce
{
    uint   ForceType;
    float3 Direction;
    float  Strength; 
    float3 Position; 
    float  Range;    
    uint   Enabled;
    float  StartTime;
    float  EndTime;
};

float3 safe_normalize(float3 v)
{
    float len = length(v);
    return (len > 1e-6) ? (v / len) : 0.0.xxx;
}

float3 point_force_accel(float3 pos, float3 pnt, float strength, float softening, float falloffPow)
{
    float3 toP = pnt - pos;
    float  d2  = dot(toP,toP) + softening*softening;
    float  d   = sqrt(d2);
    float3 dir = (d > 1e-6) ? (toP / d) : 0.0.xxx;
    float invPow = 1.0 / pow(max(d, 1e-6), max(falloffPow, 0.0));
    return dir * (strength * invPow);
}

uint Hash(uint x) 
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

float Rand01(uint seed) 
{
    return (Hash(seed) & 0x00FFFFFF) * (1.0 / 16777216.0);
}

float2 Rand01_2(uint seed) 
{ 
    return float2(Rand01(seed), Rand01(seed * 1664525u + 1013904223u)); 
}

float3 Rand01_3(uint seed) 
{ 
    return float3(
        Rand01(seed),
        Rand01(seed * 2246822519u + 3266489917u),
        Rand01(seed * 3266489917u + 668265263u)
    );
}

float3 RandomInBox(uint seed, float3 center, float3 extent) 
{
    float3 u = Rand01_3(seed) * 2.0 - 1.0;
    return center + u * extent;
}

float3 RandomVelocity(uint seed, float3 baseV, float3 varV) 
{
    float3 u = Rand01_3(seed) * 2.0 - 1.0;
    return baseV + u * varV;
}

float RandomLifetime(uint seed, float baseLife, float varLife)
{
    float u = Rand01(seed) * 2.0 - 1.0;
    return max(1e-4, baseLife + u * varLife);
}

//------------------ 3D Noise ------------------

uint Hash3(uint3 p)
{
    uint h = p.x * 73856093u ^ p.y * 19349663u ^ p.z * 83492791u;
    return Hash(h);
}

float NoiseVal3D(float3 p)
{
    uint3 ip = (uint3)floor(p);
    float3 f = frac(p);

    float n000 = (Hash3(ip + uint3(0,0,0)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n100 = (Hash3(ip + uint3(1,0,0)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n010 = (Hash3(ip + uint3(0,1,0)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n110 = (Hash3(ip + uint3(1,1,0)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n001 = (Hash3(ip + uint3(0,0,1)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n101 = (Hash3(ip + uint3(1,0,1)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n011 = (Hash3(ip + uint3(0,1,1)) & 0x00FFFFFFu) * (1.0 / 16777216.0);
    float n111 = (Hash3(ip + uint3(1,1,1)) & 0x00FFFFFFu) * (1.0 / 16777216.0);

    float3 u = f * f * (3.0 - 2.0 * f);

    float nx00 = lerp(n000, n100, u.x);
    float nx10 = lerp(n010, n110, u.x);
    float nx01 = lerp(n001, n101, u.x);
    float nx11 = lerp(n011, n111, u.x);

    float nxy0 = lerp(nx00, nx10, u.y);
    float nxy1 = lerp(nx01, nx11, u.y);

    return lerp(nxy0, nxy1, u.z);    // [0,1]
}

float3 SampleNoiseField(float3 pos, float time)
{
    float3 p = pos * NoiseFrequency;
    float   t = time * 0.3;

    float nx = NoiseVal3D(float3(p.x + t,     p.y,       p.z      ));
    float ny = NoiseVal3D(float3(p.x,         p.y + t,   p.z      ));
    float nz = NoiseVal3D(float3(p.x,         p.y,       p.z + t  ));

    float3 dir = float3(nx, ny, nz) * 2.0 - 1.0;

    return dir * NoiseStrength;
}

//------------------ Buffers ------------------

StructuredBuffer<Particle>        InputParticles : register(t0);
StructuredBuffer<ParticleForce>   Forces         : register(t1);
RWStructuredBuffer<Particle>      OutputParticles: register(u0);
RWStructuredBuffer<uint>          SpawnCounter   : register(u1);
RWStructuredBuffer<uint>          AliveCounter   : register(u2);

//------------------ Main ---------------------

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint particleCount, outStride;
    OutputParticles.GetDimensions(particleCount, outStride);
    uint index = id.x;
    if (index >= particleCount) return;

    Particle p = InputParticles[index];

    uint stage = (uint)round(saturate(p.Stage));

    bool wasAlive = (p.Lifetime > 0.0f && p.Size > 0.0f);

    if (wasAlive)
    {
        float distToCam = distance(p.Position, CameraPos);

        float denom    = max(LODFarDist - LODNearDist, 1e-6);
        float lodT     = saturate((distToCam - LODNearDist) / denom);
        float maxSkip  = max(LODMaxSkip, 0.0);
        uint  skipFrame= (uint)floor(lodT * maxSkip + 0.5);

        if (skipFrame > 0)
        {
            uint frameIndex = (uint)(SystemTime * 60.0);
            if ((frameIndex + index) % (skipFrame + 1) != 0)
            {
                OutputParticles[index] = p;
                InterlockedAdd(AliveCounter[0], 1);
                return;
            }
        }

        float3 oldPos = p.Position;
        float3 accVec = 0.0.xxx;

        uint numForces, forceStride;
        Forces.GetDimensions(numForces, forceStride);

        [loop]
        for (uint i = 0; i < numForces; ++i)
        {
            ParticleForce f = Forces[i];

            bool on = (f.Enabled != 0) &&
                      (SystemTime >= f.StartTime) &&
                      ((f.EndTime <= 0.0f) || (SystemTime <= f.EndTime));

            if (!on) continue;

            if (f.ForceType == FORCE_GRAVITY)
            {
                accVec += safe_normalize(f.Direction) * f.Strength;
            }
            else if (f.ForceType == FORCE_DIRECTION)
            {
                float d = distance(p.Position, f.Position);
                if (f.Range <= 0.0 || d <= f.Range)
                {
                    accVec += safe_normalize(f.Direction) * f.Strength;
                }
            }
            else if (f.ForceType == FORCE_POINT)
            {
                float3 to = f.Position - p.Position;
                float d = length(to);
                if (f.Range <= 0.0 || d <= f.Range)
                {
                    accVec += point_force_accel(
                        p.Position, f.Position, f.Strength,
                        0.01, 2.0);
                }
            }
        }

        if (NoiseStrength > 0.0 && NoiseFrequency > 0.0)
        {
            float3 noiseAccel = SampleNoiseField(p.Position, SystemTime);
            accVec += noiseAccel;
        }

        p.Velocity += accVec * DeltaTime;

        float3 newPos = oldPos + p.Velocity * DeltaTime;

        static const float GROUND_Z = 0.0f;
        bool hitGround = (oldPos.z > GROUND_Z) && (newPos.z <= GROUND_Z);

        if (hitGround)
        {
            float dz = newPos.z - oldPos.z;
            float t  = (GROUND_Z - oldPos.z) / max(dz, 1e-6);
            t = saturate(t);

            newPos     = lerp(oldPos, newPos, t);
            p.Position = newPos;

            p.Lifetime = 0.0f;
            p.Velocity = 0.0.xxx;
        }
        else
        {
            p.Position = newPos;
            p.Lifetime -= DeltaTime;
        }

        if (p.Lifetime <= 0.0f)
        {
            if (UseSubstage != 0 && stage == 0u)
            {
                uint seed = index ^ RandomSeed ^ asuint(SystemTime * 123.456f);
                float prob = Rand01(seed + 99u);
                if (prob < substageProb)
                {
                    p.Position += float3(0.0f, 0.0f, 0.1f);
                    p.Velocity    = RandomVelocity(seed + 23u, SubstageBaseVelocity,      SubstageVelocityVariance);
                    p.MaxLifetime = RandomLifetime(seed + 37u, SubstageLifetime,  SubstageLifetimeVariance);
                    p.Lifetime    = p.MaxLifetime;

                    p.Color = SubstageStartColor;
                    p.Size  = SubstageStartSize;

                    p.Stage = 1.0f;
                    stage = 1u;
                }
                else
                {
                    p.Lifetime = 0.0f;
                    p.Size     = 0.0f;
                }
            }
            else
            {
                p.Lifetime = 0.0f;
                p.Size = 0.0f;
            }
        }

        if (p.Lifetime > 0.0f)
        {
            stage = (uint)round(saturate(p.Stage));

            float lifeRatio = saturate(p.Lifetime / max(1e-6, p.MaxLifetime));

            if (stage == 0u)
            {
                p.Color = lerp(EndColor,      StartColor,      lifeRatio);
                p.Size  = lerp(EndSize,       StartSize,       lifeRatio);
            }
            else
            {
                p.Color = lerp(SubstageEndColor,   SubstageStartColor,   lifeRatio);
                p.Size  = lerp(SubstageEndSize,    SubstageStartSize,    lifeRatio);
            }
        }
    }
    else
    {
        uint ticket;
        InterlockedAdd(SpawnCounter[0], 1, ticket);

        if (ticket < SpawnBudget)
        {
            uint seed = index ^ RandomSeed ^ asuint(SystemTime * 1000.0);

            p.Position    = RandomInBox(seed + 11u, EmitterPosition, SpawnExtent);
            p.Velocity    = RandomVelocity(seed + 23u, BaseVelocity, VelocityVariance);
            p.MaxLifetime = RandomLifetime(seed + 37u, ParticleLifetime, LifetimeVariance);
            p.Lifetime    = p.MaxLifetime;

            p.Color = StartColor;
            p.Size  = StartSize;

            p.Stage = 0.0f;
        }
        else
        {
            p.Velocity = float3(0,0,0);
        }
    }

    if (p.Lifetime > 0.0f)
    {
        InterlockedAdd(AliveCounter[0], 1);
    }

    OutputParticles[index] = p;
}
