#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUVCoords;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVec;
layout(location = 5) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

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
    vec3 black = vec3(0.0);
    vec3 white = vec3(1.0);
    vec3 grey = vec3(0.333);

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(fragLightVec);
    vec3 V = normalize(fragViewVec);
    vec3 R = reflect(-L, N);

    float intensity;
    intensity = dot(N, L);

    
    float vectorMul = dot(N, L);
    float vectorNAbs = sqrt(pow(N.x, 2.0) + pow(N.y, 2.0) + pow(N.z, 2.0));
    float vectorLAbs = sqrt(pow(L.x, 2.0) + pow(L.y, 2.0) + pow(L.z, 2.0));
    
    float greyIndex = vectorMul / (vectorNAbs * vectorLAbs);

    if (greyIndex < 0)
        greyIndex = 0;  
    
    outColor = vec4(greyIndex * grey * fragColor, 1.0);
    


    /*
    float intensity;
    intensity = dot(N, L);
  
    vec2 st = gl_FragCoord.xy / vec2(userRes.x, userRes.y);     
    float mixValue = distance(st, vec2(dot(N,L)));      // center of color gradient  
    vec3 color = mix(fragColor, black, mixValue);
  
    color = mix(color, (white * (intensity - 0.5)) + black * (0.5 - intensity), 1) + fragColor;     // 1) + fragColor; // 0.25);

    outColor = vec4(color, 1.0);
    */

            // cartoon
    /*if (pow(max(dot(R, V), 0.0), 5.0) > 0.5)                      // illuminated
        color = mix(fragColor, white, intensity);              // 1 - mixValue
    else if (dot(V, N) < 0.5)                                       // edge        
        color = black;*/


        /*              // cartoon #2
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
        }*/



    /*
    if(pow(max(dot(R, V), 0.0), 5.0) > 0.5)     
        color = mix(fragColor, white, 1 - mixValue);*/


    /*
    if(pow(max(dot(R, V), 0.0), 5.0) > 0.5)         // if illuminated -> white
        color = mix(color, white, intensity);
    else if(dot(V, N) < 0.5)
        color = black;
    else if (max(dot(N, L), 0.0) >= 0.1) // && (intensity < 0.1))           
        color = color; //mix(color, black, 1 - intensity);
    else if (intensity < 0.1)
        color = color;
    else
        color = mix(color, black, intensity);*/            //color; 
        


    /*
    if (intensity > 0.5)
        color = mix(fragColor, mix(color, white, intensity * intensityMultiplier), 1);
    else
        color = mix(fragColor, mix(color, black, 0.5 - intensity * intensityMultiplier), 1); */

    /*
    if (intensity > 0.95)
        color = mix(color, white, intensity * intensityMultiplier);
    //if (intensity > 0.75)
      //  color = mix(color, white, intensity * intensityMultiplier);
    if (intensity > 0.5)
        color = mix(color, white, intensity * intensityMultiplier);
    //if (intensity > 0.25)
      //  color = mix(color, white, intensity * intensityMultiplier);
    if (intensity < 0.1)
        color = mix(color, black, 0.3 - intensity);*/
    

    /*
    if (intensity > 0.95)
        color = mix(color, white, intensity * intensityMultiplier);
    else if (intensity > 0.75)
        color = mix(color, mix(color, white, intensity * intensityMultiplier), intensity * intensityMultiplier);
    else if (intensity > 0.5)
        color = mix(color, mix(color, mix(color, white, intensity * intensityMultiplier), intensity), intensity * intensityMultiplier);
    else if (intensity > 0.25)
        color = mix(color, mix(color, mix(color, mix(color, white, intensity * intensityMultiplier), intensity), intensity), intensity * intensityMultiplier);
    else if (max(intensity, 0.0) >= 0.1)
        color = mix(color, mix(color, mix(color, mix(color, mix(color, white, intensity * intensityMultiplier), intensity), intensity), intensity), intensity * intensityMultiplier);
    else
        color = mix(color, black, 0.25 - intensity);*/

    /*
    if (intensity > 0.95)
        color = mix(color, white, mixValue * intensity);
    else 
        color = mix(color, black, 1 - mixValue * intensity);*/

    
    /*       // HOLOGRAM without color = gradient
    if (intensity > 0.95)
		outColor = vec4(1.0, 1.0, 1.0 ,1.0);
    else if (intensity > 0.5)
        outColor = vec4(fragColor / 2, 1.0);
    else if (intensity > 0.25)
        outColor = vec4(fragColor / 5, 1.0);
    else if (max(intensity, 0.0) >= 0.1)
        outColor = vec4(fragColor / 7, 1.0);
    else
        outColor = vec4(0.0, 0.0, 0.0, 1.0);*/


    
 }