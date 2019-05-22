const static char* panorama_image_play_frag    =   STRINGIFY(
    precision mediump float;
    varying vec2 v_TextureUV;
    uniform float u_DarkScreenTickCount;
    uniform sampler2D   u_LutTexture;
    uniform float       u_LogoAngle;
    uniform sampler2D   u_LogoTexture;
    uniform float       u_LutSize;
    uniform sampler2D u_PanoramaImageTexture;

    uniform int      u_GpuImage;
    const highp vec3 W = vec3(0.2125, 0.7154, 0.0721);
    const vec2 TexSize = vec2(100.0, 100.0);
    const vec4 bkColor = vec4(0.5, 0.5, 0.5, 1.0);
    const highp float stepX = 1.0/480.0;
    const highp float stepY = 1.0/240.0;
    void main(void){
        if(u_DarkScreenTickCount == 0.0){
            vec4 r_Color = texture2D(u_PanoramaImageTexture, v_TextureUV);
            if(v_TextureUV.y>u_LogoAngle){
                highp float radius = (1.0 - v_TextureUV.y)*0.5/(1.0-u_LogoAngle);
                vec4 r_LogoColor = texture2D(u_LogoTexture, vec2(0.5 + radius*cos(radians(v_TextureUV.x*360.+90.)), 0.5 + radius*sin(radians(v_TextureUV.x*360.+90.))));
                r_Color = vec4(r_Color.rgb*(1.0 - r_LogoColor.a)+r_LogoColor.rgb*r_LogoColor.a, 1.0);
            }
            if (u_LutSize > 0.0){
                r_Color = r_Color.bgra;
                highp float fullLength = u_LutSize * u_LutSize ;
                highp float strength     =   1.0;
                highp float rawColorR    =   1.0 - r_Color.r;
                highp float rawColorB    =   (r_Color.b ) * (u_LutSize-1.0) + 0.0;
                highp float rawColorG    =   r_Color.g * (u_LutSize-1.0);
                if (rawColorB <= 1.0){
                     rawColorB = 0.0 ;
                }
                if (rawColorB >= 31.0 ){
                     rawColorB  = 31.0 ;
                }
                highp float g0 = floor(rawColorG);
                highp float g1 = g0 + 1.0;
                highp float alpha = fract(rawColorG) * strength;
                highp float newXpos0       =    g0 *  u_LutSize   + (rawColorB)  + 0.5 ;
                highp float newXpos1       =    g1 *  u_LutSize   + (rawColorB)  + 0.5 ;
                highp vec2 vLut2d0         =   vec2(newXpos0/fullLength, rawColorR);
                highp vec2 vLut2d1         =   vec2(newXpos1/fullLength, rawColorR);
                highp vec3 new3dcolor0     =   texture2D(u_LutTexture, vLut2d0).bgr;
                highp vec3 new3dcolor1     =   texture2D(u_LutTexture, vLut2d1).bgr;
                highp vec3 finalColor      =   mix(new3dcolor0, new3dcolor1, alpha);
                r_Color =   vec4(finalColor , r_Color.a);
            }
            if(u_GpuImage == 1){
                highp float minX = v_TextureUV.x-stepX;
                highp float maxX = v_TextureUV.x+stepX;
                highp float minY = v_TextureUV.y-stepY;
                highp float maxY = v_TextureUV.y+stepY;
                if(minX<0.0){
                    minX = 0.0;
                }
                if(maxX>1.0){
                    maxX = 1.0;
                }
                if(minY<0.0){
                    minY = 0.0;
                }
                if(maxY>1.0){
                    maxY = 1.0;
                }
                highp float average = r_Color.r + r_Color.g + r_Color.b;
                vec4 leftDown = texture2D(u_PanoramaImageTexture, vec2(minX, maxY));
                vec4 centerDown = texture2D(u_PanoramaImageTexture, vec2(v_TextureUV.x, maxY));
                vec4 rightDown = texture2D(u_PanoramaImageTexture, vec2(maxX, maxY));
                vec4 leftCenter = texture2D(u_PanoramaImageTexture, vec2(minX, v_TextureUV.y));
                vec4 rightCenter = texture2D(u_PanoramaImageTexture, vec2(maxX, v_TextureUV.y));
                vec4 leftUp = texture2D(u_PanoramaImageTexture, vec2(minX, minY));
                vec4 centerUp = texture2D(u_PanoramaImageTexture, vec2(v_TextureUV.x, minY));
                vec4 rightUp = texture2D(u_PanoramaImageTexture, vec2(maxX, minY));
                highp float averageColor = leftDown.r + leftDown.g + leftDown.b + centerDown.r + centerDown.g + centerDown.b +rightDown.r + rightDown.g + rightDown.b +leftCenter.r + leftCenter.g + leftCenter.b +rightCenter.r + rightCenter.g + rightCenter.b +leftUp.r + leftUp.g + leftUp.b +centerUp.r + centerUp.g + centerUp.b +rightUp.r + rightUp.g + rightUp.b ;
                averageColor = averageColor/8.0;
                if((averageColor-average)>0.1351){
                    gl_FragColor = vec4(average/3.0, average/3.0, average/3.0, r_Color.a*average/3.0);
                }else{
                    gl_FragColor = vec4(1.0, 1.0, 1.0, r_Color.a);
                }
            }else{
                gl_FragColor = r_Color;
            }
        }else{
            gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
);