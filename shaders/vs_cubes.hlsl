$input a_position, a_normal
$output v_view, v_normal

/*
 * Copyright 2011-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include <bgfx_shader.sh>
//#include "../common/common.sh"

uniform vec4 u_params[4];

void main()
{
   float3 cam_pos = u_params[0].xyz;
   gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
   v_view = cam_pos - mul(u_model[0], vec4(a_position, 1.0)).xyz;
   v_normal = mul(u_model[0], float4(a_normal, 0.0)).xyz;
}
