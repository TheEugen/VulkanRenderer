#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUVCoords;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVec;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

////
layout(binding = 2) uniform userResolution
{
    int x;
    int y;
} userRes;

layout(push_constant) uniform PushConstants
{
    bool usePhong;
    bool noTexture;
    bool useSplitscreen;
} pushConsts;

void main() 
{
    // Phong reflection model

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(fragLightVec);
    vec3 V = normalize(fragViewVec);
    vec3 R = reflect(-L, N);

    vec4 white = vec4(1.0);

    if (pushConsts.usePhong)
    {
        vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(1.35f);
        vec3 ambient, diffuse;

        if (pushConsts.noTexture)
        {
            ambient = fragColor * 0.1f;
            diffuse = 1 * max(dot(N, L), 0.0) * fragColor;            // 1 = kd const
            outColor = vec4(ambient + diffuse + specular, 1.0);
        }           
        else
        {
            ambient = vec3(texture(tex, fragUVCoords)) * 0.1;
            diffuse = 1 * max(dot(N, L), 0.0) * vec3(texture(tex, fragUVCoords)); 
            
            outColor = vec4(ambient + diffuse + specular, 1.0);
            
            if(pow(max(dot(R, V), 0.0), 5.0) > 0.5)         // if illuminated -> mix white with texture
                outColor = mix(outColor, white, 0.05);
        }        
    }
    else
    {
            // cartoon shader
        if(pow(max(dot(R, V), 0.0), 5.0) > 0.5)         // if illuminated -> white
        {
            outColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
        else if(dot(V, N) < 0.5)
        {
            // fragColor / 10
            outColor = vec4(0.0, 0.0, 0.0, 1.0);       // if edge of model -> dark
        }
        else if (max(dot(N, L), 0.0) >= 0.1)            
        {
            outColor = vec4(fragColor, 1.0);
        }
        else
        {
            outColor = vec4(fragColor / 5, 1.0);
        }
   
    }
}

