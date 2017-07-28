$input v_view, v_normal

#include <bgfx_shader.sh>

static const float k_pi = 3.141592;

uniform vec4 u_params[4];

float sqr(float x)
{
   return x * x;
}

float3 linear_to_sRGB(float3 x)
{
   bool3 c = x < 0.0031308;
   float3 s_dark = 12.92 * x;
   float3 s_light = 1.055 * pow(x, 1.0/2.4) - 0.055;
   return s_light;//min(s_dark, s_light);
}

float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

float2 hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), radicalInverse_VdC(i));
}

struct Surface
{
   float3 colour;
   float3 view;
   float3 normal;
   float roughness;
   float F0;
   float n_o_v;
};

Surface make_surface(float3 albedo, float3 normal, float3 view, float roughness, float F0)
{
   Surface s;
   s.colour = albedo;
   s.view = normalize(view);
   s.normal = normalize(normal);
   s.n_o_v = dot(s.normal, s.view);
   s.roughness = max(0.01, roughness);
   s.F0 = 0.04 + 0.96 * F0;
   return s;
}

struct Interface
{
   float v_o_l;
   float n_o_l;

   float v_o_h;
   float n_o_h;
   float l_o_h;
};

Interface make_interface(Surface s, float3 l_dir)
{
   float3 h_dir = normalize(l_dir + s.view);

   Interface i;
   i.v_o_l = dot(s.view, l_dir);
   i.n_o_l = dot(s.normal, l_dir);
   i.v_o_h = dot(s.view, h_dir);
   i.n_o_h = dot(s.normal, h_dir);
   i.l_o_h = dot(l_dir, h_dir);
   return i;
}

float3 importance_sample_diffuse(float2 Xi, float3 N )
{
    float CosTheta = 1.0-Xi.y;
    float SinTheta = sqrt(1.0-CosTheta*CosTheta);
    float Phi = 2*k_pi*Xi.x;

    float3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );

    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float3 importance_sample_GGX(float2 Xi, float roughness, float3 N)
{
    float a = roughness * roughness;

    float Phi = 2 * k_pi * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a*a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);

    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);

    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float fresnel(Surface s, Interface i)
{
   // Schlick approximation
   float k = max(0.0, s.n_o_v);
   return s.F0 + (1 - s.F0) * pow(1 - k, 5.0);
}

float ggx_ndf(Surface s, Interface i)
{
   float k = max(0.0, i.n_o_h);
   float a = sqr(s.roughness);
   float a_2 = sqr(a);
   float den = k_pi*sqr(sqr(k)*(a_2-1)+1.0);
   return a_2 / den;
}

float g1(float k, float a)
{
   return a / (a*(1-k)+k);
}

float geometric_term(Surface s, Interface i)
{
   float k = sqr(s.roughness)*sqrt(2.0 / k_pi);
   //float k = sqr(s.roughness+1.0)/8.0;
   return g1(k, s.n_o_v) * g1(k, i.n_o_l);
}

float geometric_term_correlated(Surface s, Interface i)
{
   float a = sqr(s.roughness);
   float a_2 = sqr(a);
   float div = 2.0 * i.n_o_l * s.n_o_v;
   float den1 = s.n_o_v * sqrt(a_2 + 1 - a_2 * sqr(i.n_o_l));
   float den2 = i.n_o_l * sqrt(a_2 + 1 - a_2 * sqr(s.n_o_v));
   return div / (den1 + den2);
}


void main()
{
   float3 l_dir = normalize(u_params[1].xyz);
   float3 l_col = u_params[2].xyz;
   Surface s = make_surface(u_params[3].rgb, v_normal, v_view, u_params[1].w, u_params[0].w);
   Interface i = make_interface(s, l_dir);

   const uint sample_count = 256;
   for (uint idx = 0; idx < sample_count; ++idx)
   {
      float2 X_i = hammersley(idx, sample_count);
      float3 H = importance_sample_GGX(X_i, s.roughness, s.normal);
      float3 L = 2.0f * dot(s.view, H) * H - s.view;
   }

   float3 lambert_diffuse =  s.colour / k_pi;
   float nOl = max(0.0, i.n_o_l);
   float D = ggx_ndf(s, i);
   float F = fresnel(s, i);
   float G = geometric_term(s, i);
   float G_corr = geometric_term_correlated(s, i);
   float3 spec_uncorrelated = l_col * D * G * F;
   float3 spec_correlated = l_col * D * G_corr * F;
   float spec_norm = 4.0 * s.n_o_v * i.n_o_l;

   float3 final = float3(0.0, 0.0, 0.0);
   //if (u_params[3].w)
   {
   }
   //if (u_params[2].w)
   float3 diff = l_col * lambert_diffuse;
   if (s.normal.y < 0.0)
   {
      final += diff  * (1 - s.F0);
      final += spec_uncorrelated / spec_norm;
   }
   else
   {
      final += diff * 1.05 * (1 - F) * (1-pow(1-s.n_o_v, 5.0));
      final += spec_correlated / spec_norm;
   }
// max(0.0, i.n_o_l)
   gl_FragColor = float4(final * max(0.0, i.n_o_l), 1.0);//float4(diff + spec, 1.0);
}
