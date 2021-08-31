#version 330 core
in vec2 texCoord;

#define RENDER_DIST 100

struct Camera
{
    vec3 pos;
    float cosYaw;
    float cosPitch;
    float sinYaw;
    float sinPitch;
    vec2 frustumDiv;
};

uniform Camera camera;
uniform vec2 screenSize;

uniform vec3 color;

// thanks, http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

#define PI 3.14159265359f
float atan2(in float y, in float x)
{
    bool s = (abs(x) > abs(y));
    return mix(PI/2.0 - atan(x,y), atan(y,x), s);
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

int Iterations = 30;
float Power = 10;
float Bailout = 2;

float DE(vec3 pos) {
	vec3 z = pos;
	float dr = 1.0;
	float r = 0.0;
	for (int i = 0; i < Iterations ; i++) {
		r = length(z);
		if (r > Bailout) break;
		
		// convert to polar coordinates
		float theta = acos(z.z / r);
		float phi = atan(z.y, z.x);
		dr = pow(r, Power - 1.0) * Power * dr + 1.0;
		
		// scale and rotate the point
		float zr = pow(r, Power);
		theta *= Power;
		phi *= Power;
		
		// convert back to cartesian coordinates
		z = zr*vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
		z+=pos;
	}
	return 0.5 * log(r) * r / dr;
}

bool rayMarch(in vec3 pos, in vec3 dir, out float travelDist, out int steps, out vec4 resColor)
{
    bool hit = false;
    steps = 0;

    while(!hit) { // march!
        float dist = DE(pos);

        if(steps == 0)
            dist *= rand(dir.xy);

        if(travelDist > RENDER_DIST)
            return false;

        if(dist < 0.00001)
            return true;

        pos += dir * dist;
        travelDist += dist;
        steps++;

        if(steps > 100)
            return false;
    }

    return hit;
}

vec3 getPixel(in vec2 pixel_coords)
{
    vec2 frustumRay = (pixel_coords - (0.5 * screenSize)) / camera.frustumDiv;

    // rotate frustum space to world space
    float temp = camera.cosPitch + frustumRay.y * camera.sinPitch;
    
    vec3 rayDir = normalize(vec3(frustumRay.x * camera.cosYaw + temp * camera.sinYaw,
                                 frustumRay.y * camera.cosPitch - camera.sinPitch,
                                 temp * camera.cosYaw - frustumRay.x * camera.sinYaw));
    
    // raymarch outputs
    float dist = rand(pixel_coords / 100.f) * 1.f;
    int steps = 0;
    vec4 resColor;
    bool hit = rayMarch(camera.pos, rayDir, dist, steps, resColor);

    return color * (float(steps) / 40.f);
}

void main() {
    vec2 pixelCoord = gl_FragCoord.xy;
    //pixelCoord.y *= -1;

    gl_FragColor = vec4(getPixel(pixelCoord), 0.0);
}