#version 430

struct SliceVertexData 
{
    vec4 position;
	vec4 tcs;
};
layout(std140, binding=2) buffer Slices 
{
    SliceVertexData sliceVertexes[];
};
struct LOD_info
{
    uint offset;
    float min_dist;
    float max_dist;
    float pad;
};
struct InstanceData
{
    vec4 center_self;
    vec4 center_par;
    mat4 projection_camera;
};
struct IndexData
{
    uint index;
    //uint pad;
    float mn;
    float mx;
};
layout(std140, binding=1) buffer _models_intervals
{
    readonly uvec4 models_intervals[];
};
layout(std140, binding=3) buffer _instances
{
    readonly InstanceData instances[];
};
layout(std140, binding=4) buffer _instance_indexes//same size as instances
{
    readonly IndexData instance_indexes[];
};


in vec3 in_Position;
in vec3 in_Normal;
in vec4 in_Tex;

uniform mat4 projectionCamera;
uniform vec3 imp_center;
uniform vec3 camera_pos;
uniform int slice_count;
uniform int slice_offset;
uniform float delta;
uniform float angle_step;
uniform uint id;

out vec3 tc_a;
out vec3 tc_b;
out vec3 tc_t;
out vec3 q_abt;
out vec3 ex_Normal;
out vec3 ex_FragPos;
out vec2 a_mult;

#define PI 3.1415926535897932384626433832795

mat4 translate(vec3 d)
{
	return mat4(1, 0, 0, d.x,
	            0, 1, 0, d.y,
	            0, 0, 1, d.z,
	            0, 0, 0, 1);
}
mat4 rotate(vec3 axis, float angle) 
{
  axis = normalize(axis);
  float s = sin(angle);
  float c = cos(angle);
  float oc = 1.0 - c;

  return mat4(
		oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
    oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
    oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
		0.0,                                0.0,                                0.0,                                1.0
	);
}
void main(void) {
	//Fragment Position in Model Space
	uint offset = models_intervals[id].x + gl_InstanceID;
	mat4 inst_mat = instances[instance_indexes[offset].index].projection_camera;
	a_mult = vec2(instance_indexes[offset].mn, instance_indexes[offset].mx);
	vec3 center = (inst_mat * vec4(imp_center,1.0f)).xyz;
    vec3 viewdir = normalize(center - camera_pos);
    mat3 MVI = mat3(transpose(inverse(inst_mat)));
    vec3 front = normalize(MVI * vec3(0,0,1));
    vec3 plane = normalize(MVI * vec3(1,0,0));
    vec3 up = normalize(MVI * vec3(0,1,0));
    float psi = -asin(abs(dot(up, viewdir)));
    viewdir.y = 0;
    viewdir = normalize(viewdir);
    float phi = acos(-dot(front,viewdir));
    if (dot(viewdir,plane) > 0)
        phi = 2*PI - phi;
    float a_phi = phi;
    
    int slice_n = int(floor(slice_count*(phi/(2.0f*PI)))) % slice_count;

    phi = - phi + (2.0f*PI*float(slice_n))/float(slice_count);
    
    int second_slice_n = (slice_n - int(sign(phi)) + slice_count) % slice_count; 

    slice_n += 1;
    second_slice_n += 1;

	mat4 rot = rotate(up,phi);
    mat4 rot_top = rotate(cross(up, viewdir),psi);
	
	vec4 pos = vec4(sliceVertexes[slice_offset + slice_n*4 + gl_VertexID].position.xyz,1);
	pos = vec4(center,0) + rot * (inst_mat * pos - vec4(center,0));

    float s_phi =  sign(phi)*(angle_step - abs(phi));
    mat4 rot_s = rotate(up, - s_phi);
    vec4 pos_s = vec4(sliceVertexes[slice_offset + second_slice_n*4 + gl_VertexID].position.xyz,1);
	pos_s = vec4(center,0) + rot_s * (inst_mat * pos_s - vec4(center,0));
    q_abt.y = smoothstep(0, 1, 0.5*(1 + (abs(phi) - 0.5*angle_step)/delta));
    q_abt.x = 1 - q_abt.y;
    pos = q_abt.x * pos + q_abt.y * pos_s;

    pos = vec4(center,0) + rot_top * (pos - vec4(center,0));
    mat4 rot_top_s = rotate(cross(up, viewdir),-0.5*PI + psi);
    mat4 rot_top_s2 = rotate(up, -a_phi);
    vec4 top_pos = inst_mat * vec4(sliceVertexes[slice_offset + gl_VertexID].position.xyz,1);
    top_pos = vec4(center,0) + rot_top_s2 * (top_pos - vec4(center,0));

    q_abt.z = abs(psi)/(0.5*PI);
    q_abt.xy *= (1 - q_abt.z);
    pos = (1 - q_abt.z)*pos + q_abt.z*top_pos;
	ex_FragPos = pos.xyz;
	ex_Normal = in_Normal;	//Pass Normal

	//Fragment in Screen Space
	gl_Position = projectionCamera * vec4(ex_FragPos, 1.0f);
	tc_a = sliceVertexes[slice_offset + slice_n*4 + gl_VertexID].tcs.xyz;
    tc_b = sliceVertexes[slice_offset + second_slice_n*4 + gl_VertexID].tcs.xyz;
    tc_t = sliceVertexes[slice_offset + gl_VertexID].tcs.xyz;
}
