// Set = 0, Binding = 0
[[vk::binding(0, 0)]]
cbuffer Camera
{
    float4x4 View;
    float4x4 Proj;
    float3 Pos;
}

struct MaterialInfo
{
    float4 AlbedoFactor;
    float4 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
    int HasNormalMap;
    int HasEmissiveMap;
    int HasMetallicMap;
    int HasRoughnessMap;

    int _padding[4];
}

// Set = 1, Binding = 3
[[vk::binding(3, 1)]]
cbuffer ModelData
{
    float4x4 Model;
    float4x4 InvModel;

    MaterialInfo MatInfo;
}

enum class RenderFlags
{
    ALBEDO,
    UVS,
    GEO_NORMAL,
    SHADING_NORMAL,
    NORMAL_MAP,
    TANGENT,
    BITANGENT,
    OCCLUSION,
    EMISSIVE,
    METALLIC,
    ROUGHNESS,
    NONE,
    COUNT
}

// Set = 1, Binding = 0
[[vk::binding(0, 1)]]
cbuffer RenderFlags
{
    RenderFlags renderMode;
}

struct DirectionalLight
{
    float3 Direction;
    float _pad0;
    float3 Color;
    float _pad1;

    float Intensity; // 36 bytes
}

struct PointLight
{
    float3 Position;
    float _pad0;
    float3 Color;
    float _pad1;

    float Intensity;
    float Range; 
    float _pad2;
    float _pad3;
}

// Set = 1, Binding = 1
[[vk::binding(1, 1)]]
cbuffer SceneInfo
{
    DirectionalLight dirLight;
    int PointLightCount;
    int2 _pad2; // 64 bytes
}

// Set = 1, Binding = 2
[[vk::binding(2, 1)]]
StructuredBuffer<PointLight> pointLights;

// Set = 1, Binding = 4
[[vk::binding(4, 1)]]
SamplerState linearSampler;
// Set = 1, Binding = 5
[[vk::binding(5, 1)]]
SamplerState iblSampler;

// Set = 1, Binding = 6..10
[[vk::binding(6, 1)]]
Texture2D baseColor;
[[vk::binding(7, 1)]]
Texture2D normalMap;
[[vk::binding(8, 1)]]
Texture2D occlusionMap;
[[vk::binding(9, 1)]]
Texture2D roughMetalMap;
[[vk::binding(10, 1)]]
Texture2D emissiveMap;

// Set = 1, Binding = 11..13
[[vk::binding(11, 1)]]
TextureCube<float4> specularCubemap;
[[vk::binding(12, 1)]]
TextureCube<float4> diffuseCubemap;
[[vk::binding(13, 1)]]
Texture2D<float4> brdfLUT;

struct VertexInput
{
    float3 Position : ATTRIB0;
    float3 Color : ATTRIB1;
    float3 Normal : ATTRIB2;
    float4 Tangent : ATTRIB3;
    float2 TexCoord : ATTRIB4;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 FragPosition : POSITION0;
    float3 Color : COLOR0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 BiTangent : BITANGENT0;
    float2 TexCoord : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(Model, float4(input.Position, 1.0f));
    output.Position = mul(Proj, mul(View, worldPosition));
    output.FragPosition = worldPosition;
    output.Color = input.Color;
    output.Normal = mul((float3x3)InvModel, input.Normal);
    output.Tangent = mul((float3x3)InvModel, input.Tangent.xyz);
    output.BiTangent = cross(output.Normal, output.Tangent.xyz) * input.Tangent.w;
    output.TexCoord = input.TexCoord;
    return output;
}

float3 UnpackNormal(const float3 n)
{
    return n * 2.0f - 1.0f;
}

float3 ApplyNormalMap(const float3x3 tbn, float3 normalTs, const float2 uv)
{
    return normalize(mul(normalTs, tbn));
}

static const float PI = 3.14159265359f;
static const float INVPI = 1.0f / PI;

float3 F_schlick(in float3 f0, in float LoH)
{
    return (f0 + (1.0f - f0) * pow(1.0f - LoH, 5.0f));
}

float DistributionGGX(float NoH, float a)
{
    float a2 = a * a;
    float NdotH2 = NoH * NoH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(float NoV, float NoL, float k)
{
    float ggx1 = GeometrySchlickGGX(NoV, k);
    float ggx2 = GeometrySchlickGGX(NoL, k);

    return ggx1 * ggx2;
}

float3 CalculateFRough(float3 F0, float ndotv, float roughness)
{
    return F0 +
           (max(float3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - ndotv, 0.0f, 1.0f), 5.0f);
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target
{
    float4 albedo = pow(baseColor.Sample(linearSampler, input.TexCoord), 1.0f / 2.2f);
    albedo *= MatInfo.AlbedoFactor;
    float4 occlusion = pow(occlusionMap.Sample(linearSampler, input.TexCoord), 1.0f / 2.2f);
    float4 roughMetallic = pow(roughMetalMap.Sample(linearSampler, input.TexCoord), 1.0f / 2.2f);

    float3 normal = normalize(input.Normal);

    float3 light = normalize(dirLight.Direction);

    if (MatInfo.HasNormalMap == 1)
    {
        float3 normalTs = pow(normalMap.Sample(linearSampler, input.TexCoord).rgb, 1.0f / 2.2f);
        float3 T = normalize(input.Tangent);
        float3 B = normalize(input.BiTangent);
        const float3x3 tbn = float3x3(T, B, normal);
        normal = ApplyNormalMap(tbn, UnpackNormal(normalTs), input.TexCoord);
    }

    float roughness = MatInfo.RoughnessFactor;
    float metallic = MatInfo.MetallicFactor;
    if (MatInfo.HasRoughnessMap == 1)
        roughness *= roughMetallic.g;
    if (MatInfo.HasMetallicMap == 1)
        metallic *= roughMetallic.b;

    float3 emissive = float3(0.0f);
    if (MatInfo.HasEmissiveMap == 1)
    {
        emissive = pow(emissiveMap.Sample(linearSampler, input.TexCoord).rgb, 1.0f / 2.2f);
        emissive *= MatInfo.EmissiveFactor.xyz;
    }

    float4 final = float4(1.0f);

    switch (renderMode)
    {
    case RenderFlags::NONE:
        float3 V = normalize(Pos - input.FragPosition.xyz);
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, metallic);

        float3 H = normalize(V + (-light));

        float ndotl = max(dot(normal, -light), 0.001f);

        float ndotv = max(dot(normal, V), 0.001f);
        float ndoth = max(dot(normal, H), 0.001f);

        float NDF = DistributionGGX(ndoth, roughness);
        float G = GeometrySmith(ndotv, ndotl, roughness);
        float3 F = F_schlick(F0, ndotv);

        float3 numerator = NDF * G * F;
        float denominator = 4.0f * ndotv * ndotl;
        float3 specular = max(numerator / denominator, 0.01f);

        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metallic;

        float3 lightOut =
            (kD * albedo.rgb * INVPI + specular) * dirLight.Color * dirLight.Intensity * ndotl;

        // Point Light Contribution
        for (int i = 0; i < PointLightCount; i++)
        {
            PointLight pointLight = pointLights[i];

            float3 toLight = pointLight.Position - input.FragPosition.xyz;
            float dist = length(toLight);
            light = normalize(toLight);

            float attenuation = saturate(1.0f - dist / pointLight.Range);
            attenuation *= attenuation;

            if (attenuation <= 0.01f)
                continue;

            ndotl = max(dot(normal, -light), 0.001f);

            NDF = DistributionGGX(ndoth, roughness);
            G = GeometrySmith(ndotv, ndotl, roughness);
            F = F_schlick(F0, ndotv);

            numerator = NDF * G * F;
            denominator = 4.0f * ndotv * ndotl;
            specular = max(numerator / denominator, 0.01f);

            kS = F;
            kD = 1.0f - kS;
            kD *= 1.0f - metallic;

            float3 pointLightRadiance = pointLight.Color * pointLight.Intensity * attenuation;

            lightOut += (kD * albedo.rgb * INVPI + specular) * pointLightRadiance * ndotl;
        }

        // float3 ambient = 0.4f * albedo.rgb;

        // Environment Lighting
        float3 R = reflect(-V, normal);
        float3 Ks = CalculateFRough(F0, ndotv, roughness);
        float3 Kd = 1.0f - Ks;
        Kd *= 1.0f - metallic;

        float3 prefilteredColor =
            pow(specularCubemap.SampleLevel(iblSampler, R, roughness * 6.0f).rgb, 1 / 2.2f);
        float4 irradiance = pow(diffuseCubemap.Sample(iblSampler, normal), 1 / 2.2f);
        float2 brdf = brdfLUT.Sample(iblSampler, float2(ndotv, roughness)).rg;
        float3 specularIBL = prefilteredColor * Ks; //* (F * brdf.x + brdf.y);
        float3 diffuse = albedo.rgb * irradiance.rgb;
        float3 ambient = (Kd * diffuse + specularIBL);

        final = float4(lightOut + ambient + emissive.rgb, albedo.a);
        break;
    case RenderFlags::ALBEDO:
        final = albedo;
        break;
    case RenderFlags::UVS:
        final = float4(input.TexCoord, 0.0f, 1.0f);
        break;
    case RenderFlags::EMISSIVE:
        final = float4(emissive, 1.0f);
        break;
    case RenderFlags::GEO_NORMAL:
        final = float4(normalize(input.Normal) * 0.5f + 0.5f, 1.0f);
        break;
    case RenderFlags::NORMAL_MAP:
        float3 normalTs = pow(normalMap.Sample(linearSampler, input.TexCoord).xyz, 1.0f / 2.2f);
        final = float4(normalTs, 1.0f);
        break;
    case RenderFlags::SHADING_NORMAL:
        final = float4(normal * 0.5f + 0.5f, 1.0f);
        break;
    case RenderFlags::TANGENT:
        final = float4(normalize(input.Tangent) * 0.5f + 0.5f, 1.0f);
        break;
    case RenderFlags::BITANGENT:
        final = float4(normalize(input.BiTangent) * 0.5f + 0.5f, 1.0f);
        break;
    case RenderFlags::METALLIC:
        final = float4(metallic);
        break;
    case RenderFlags::ROUGHNESS:
        final = float4(roughness);
        break;
    case RenderFlags::OCCLUSION:
        final = occlusion;
        break;
    }

    return pow(final, 2.2f);
}
