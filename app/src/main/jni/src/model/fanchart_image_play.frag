const static char* panorama_image_play_frag    =   STRINGIFY(
    precision mediump float;
    varying vec2 v_TextureUV;
    uniform float u_DarkScreenTickCount;
    uniform sampler2D   u_LutTexture;
    uniform float       u_LogoAngle;
    uniform sampler2D   u_LogoTexture;
    uniform float       u_LuteSize;
    uniform sampler2D u_PanoramaImageTexture;

    uniform int      u_GpuImage;
    const highp vec3 W = vec3(0.2125, 0.7154, 0.0721);
    const vec2 TexSize = vec2(100.0, 100.0);
    const vec4 bkColor = vec4(0.5, 0.5, 0.5, 1.0);

    void main(void){
        if(u_DarkScreenTickCount == 0.0){
            vec4 r_Color = texture2D(u_PanoramaImageTexture, v_TextureUV);
            if(v_TextureUV.y>u_LogoAngle){
                highp float radius = (1.0 - v_TextureUV.y)*0.5/(1.0-u_LogoAngle);
                vec4 r_LogoColor = texture2D(u_LogoTexture, vec2(0.5 + radius*cos(radians(v_TextureUV.x*360.+90.)), 0.5 + radius*sin(radians(v_TextureUV.x*360.+90.))));
                r_Color = vec4(r_Color.rgb*(1.0 - r_LogoColor.a)+r_LogoColor.rgb*r_LogoColor.a, 1.0);
            }
            if (u_LuteSize > 0.0){
                r_Color = r_Color.bgra;
                highp float fullLength = u_LuteSize * u_LuteSize ;
                highp float strength     =   1.0;
                highp float rawColorR    =   1.0 - r_Color.r;
                highp float rawColorB    =   (r_Color.b ) * (u_LuteSize-1.0) + 0.0;
                highp float rawColorG    =   r_Color.g * (u_LuteSize-1.0);
                if (rawColorB <= 1.0){
                     rawColorB = 0.0 ;
                }
                if (rawColorB >= 31.0 ){
                     rawColorB  = 31.0 ;
                }
                highp float g0 = floor(rawColorG);
                highp float g1 = g0 + 1.0;
                highp float alpha = fract(rawColorG) * strength;
                highp float newXpos0       =    g0 *  u_LuteSize   + (rawColorB)  + 0.5 ;
                highp float newXpos1       =    g1 *  u_LuteSize   + (rawColorB)  + 0.5 ;
                highp vec2 vLut2d0         =   vec2(newXpos0/fullLength, rawColorR);
                highp vec2 vLut2d1         =   vec2(newXpos1/fullLength, rawColorR);
                highp vec3 new3dcolor0     =   texture2D(u_LutTexture, vLut2d0).bgr;
                highp vec3 new3dcolor1     =   texture2D(u_LutTexture, vLut2d1).bgr;
                highp vec3 finalColor      =   mix(new3dcolor0, new3dcolor1, alpha);
                r_Color =   vec4(finalColor , r_Color.a);
            }
            if(u_GpuImage == 1){
                vec2 upLeftUV = vec2(v_TextureUV.x-1.0/TexSize.x, v_TextureUV.y-1.0/TexSize.y);
                vec4 upLeftColor = texture2D(u_PanoramaImageTexture, upLeftUV);
                vec4 delColor = r_Color - upLeftColor;
                float luminance = dot(delColor.rgb, W);
                gl_FragColor = vec4(vec3(luminance), 0.0) + bkColor;
            }else{
                gl_FragColor = r_Color;
            }
        }else{
            gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
);