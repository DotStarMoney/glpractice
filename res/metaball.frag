//frag
//:toy shader demonstrating metaball raytracing
#version 330 core

layout(origin_upper_left) in vec4 gl_FragCoord;
out vec4 color;

uniform float scrx;
uniform float scry;
uniform float view_plane_depth;
uniform float view_plane_width;
uniform float view_plane_height;
uniform float cube_side;
uniform mat4 ray_transform;
uniform vec3 camera_pos;
uniform vec3 bpos0;
uniform vec3 bpos1;
uniform sampler2D wall_texture;

struct lbv
{
	float x;
	float g;
	int g_i;
};

vec3 gradient(vec3 _p0, vec3 _p1, vec3 _p)
{
	vec3 pd;
	vec3 d;
	vec3 ret;
	float qdist;
	
	ret = vec3(0.0f, 0.0f, 0.0f);
	
	d = _p - _p0;
	pd = -2.0f * d;
	qdist = dot(d, d);
	qdist *= qdist;
	ret += pd / qdist;
	
	d = _p - _p1;
	pd = -2.0f * d;
	qdist = dot(d, d);
	qdist *= qdist;
	ret += pd / qdist;
	
	return normalize(ret);
}

lbv raycast(
	vec3 _p0, 
	vec3 _p1, 
	float _mr, 
	vec3 _r_p, 
	vec3 _r_v, 
	float _t0,
	float _t1)
{
	vec3 lx;
	vec3 lg;
	lbv c;
	lbv ret;
	float zc;
	
	ret.g = 0.0f;
	ret.x = 0.0f;
	ret.g_i = 0;

	lg = _r_v;
	lx = _r_p - _p0;
	c.g = 2.0f * dot(lx, lg);
	c.x = dot(lx, lx);
	c.g_i = 0;
	if (c.g < 0)
	{
		zc = -c.x / c.g + _t0;
		if(_t1 >= zc) 
		{
			c.g_i = 1;
		}
		else
		{
			c.g = -c.g / (c.x*(c.x + c.g*(_t1 - _t0)));
		}
	}
	else
	{
		c.g = -c.g / (c.x*c.x);
	}
	c.x = 1.0f / c.x;
	
	ret.g = ret.g + c.g;
	ret.x = ret.x + c.x;
	ret.g_i = ret.g_i | c.g_i;
	
	//lg = _r_v;
	lx = _r_p - _p1; //_p1
	c.g = 2.0f * dot(lx, lg);
	c.x = dot(lx, lx);
	c.g_i = 0;
	if (c.g < 0)
	{
		zc = -c.x / c.g + _t0;
		if (_t1 >= zc) c.g_i = 1;
		c.g = -c.g / (c.x*(c.x + c.g*(_t1 - _t0)));
	}
	else
	{
		c.g = -c.g / (c.x*c.x);
	}
	c.x = 1.0f / c.x;

	ret.g = ret.g + c.g;
	ret.x = ret.x + c.x;
	ret.g_i = ret.g_i | c.g_i;
	
	ret.x = ret.x - _mr;
		
	return ret;
}



void main()
{
	vec4 ray_v;
	vec3 ray;
	vec2 tex_coords;
	vec3 normal;
	vec3 ray_base;
	vec4 tt_color;
	vec4 s_color;
	vec4 d_color;
	vec3 reflected;
	int trace_i;
	int escape_i;
	float cube_half_s;
	int side;
	float bound_n;
	float ttx;
	float tty;
	float ttz;
	float tt_bound;
	float interval;
	float t0;
	float t1;
	float lb_interval;
	float d_strength;
	float dp;
	vec3 light_d;
	lbv field;
	bool first_bounce;

	
	ray_v.x = ((gl_FragCoord.x / scrx) - 0.5f) * view_plane_width;
	ray_v.y = ((gl_FragCoord.y / scry) - 0.5f) * view_plane_height;
	ray_v.z = view_plane_depth;
	ray_v.w = 1.0f;
	ray_v = ray_transform * normalize(ray_v);
	
	cube_half_s = cube_side * 0.5f;

	light_d = normalize(vec3(-1.0f, 1.0f, 0.0f));
	
	interval = 5.0f;
	t0 = 0.0f;
	ray_base = camera_pos;
	normal = vec3(0.0f, 0.0f, 0.0f);
	escape_i = 0;
	first_bounce = true;
	s_color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	d_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	for(trace_i = 0; trace_i < 75; trace_i++)
	{
		t1 = t0 + interval;
		ray = t0 * ray_v.xyz + ray_base;
		
		field = raycast(
					bpos0,
					bpos1,
					0.2f,
					ray,
					ray_v.xyz,
					t0,
					t1);
				
		if(field.g_i == 1)
		{
			interval *= 0.25f;
		}
		else
		{
			if(field.g < 0.0f)
			{
				t0 += interval;
				interval *= 2.0f;
			}
			else
			{
				lb_interval = -field.x / field.g;
				if(interval < lb_interval)
				{
					t0 += interval;
					interval *= 2.0f;
				}
				else
				{
					interval = lb_interval;
					t0 += interval;
				}
			}
		}
		
		escape_i = max(escape_i - 1, 0);
		if((field.x > -0.0004f) && (escape_i == 0))
		{
			interval = 5.0f;
			t0 = 0.0f;
			ray_base = ray;
			normal = gradient(
						bpos0,
						bpos1,
						ray_base);
			ray_v.xyz = ray_v.xyz - 2.0f*normal*dot(ray_v.xyz, normal);
			escape_i = 10;
			if(first_bounce)
			{
				dp = (dot(normal, light_d) + 1.0f) * 0.5f;
				d_strength = clamp(pow(dp, 20), 0.0f, 1.0f);
				s_color = vec4(1.0f, 1.0f, 1.0f, 1.0f) * d_strength * 0.75;
				d_color = vec4(1.0f, 1.0f, 1.0f, 1.0f) * (0.5f + dp*0.5f);
			}				
				
			first_bounce = false;
		}
		
		if((ray.x < -cube_half_s) || (ray.x > cube_half_s) || 
			(ray.y < -cube_half_s) || (ray.y > cube_half_s) ||
			(ray.z < -cube_half_s) || (ray.z > cube_half_s)) break;
	}
	
	if(ray_v.x < 0)
	{
		ttx = (-cube_half_s - camera_pos.x) / ray_v.x;
	}
	else
	{
		ttx = (cube_half_s - camera_pos.x) / ray_v.x;
	}
	if(ray_v.y < 0)
	{
		tty = (-cube_half_s - camera_pos.y) / ray_v.y;
	}
	else
	{
		tty = (cube_half_s - camera_pos.y) / ray_v.y;
	}
	if(ray_v.z < 0)
	{
		ttz = (-cube_half_s - camera_pos.z) / ray_v.z;
	}
	else
	{
		ttz = (cube_half_s - camera_pos.z) / ray_v.z;
	}
	side = 0;
	tt_bound = ttx;
	if(ttz < tt_bound)
	{
		side = 1;
		tt_bound = ttz;
	}
	if(tty < tt_bound)
	{
		side = 2;
		tt_bound = tty;
	}
	ray = camera_pos + tt_bound*ray_v.xyz;
	switch(side)
	{
	case 0:
		tex_coords = vec2(ray.z, ray.y);
		tt_color = vec4(0.8f, 0.9f, 1.0f, 1.0f);
		break;
	case 1:
		tex_coords = vec2(ray.x, ray.y);
		tt_color = vec4(0.6f, 0.8f, 0.7f, 1.0f);
		break;
	case 2:
		tex_coords = vec2(ray.x, ray.z);
		tt_color = vec4(0.6f, 0.4f, 0.5f, 1.0f);
		break;
	}
	tex_coords = tex_coords * 0.05 + vec2(0.5f, 0.5f);
		
	
	color = min(texture(wall_texture, tex_coords)*tt_color*d_color + s_color, 
		vec4(1.0f, 1.0f, 1.0f, 1.0f));

	
} 