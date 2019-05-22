const static char* panorama_camera_play_frag    =   STRINGIFY(

    \n#extension GL_OES_EGL_image_external : require\n
    precision mediump float;
    varying vec2 v_TextureUV;
    uniform samplerExternalOES u_CameraTextureOES1;
    uniform samplerExternalOES u_CameraTextureOES2;
    uniform sampler2D   u_LogoTexture;
    uniform float       u_LogoAngle;
    uniform sampler2D   u_LutTexture;
    uniform float       u_LutSize;

    uniform int      u_GpuImage;
    const highp float stepX = 0.001042;
    const highp float stepY = 0.002084;
    const highp vec3 WW = vec3(0.299,0.587,0.114);

    void main(void){
        vec4 r_Color;
        if(v_TextureUV.y<0.5){
            r_Color = texture2D(u_CameraTextureOES1, vec2(v_TextureUV.x, v_TextureUV.y*2.0)) ;
        }else{
            r_Color = texture2D(u_CameraTextureOES2, vec2(v_TextureUV.x, (v_TextureUV.y-0.5)*2.0));
        }
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
            vec4 centerDown = texture2D(u_CameraTextureOES1, vec2(v_TextureUV.x, maxY));
            vec4 leftCenter = texture2D(u_CameraTextureOES1, vec2(minX, v_TextureUV.y));
            vec4 rightCenter = texture2D(u_CameraTextureOES1, vec2(maxX, v_TextureUV.y));
            vec4 centerUp = texture2D(u_CameraTextureOES1, vec2(v_TextureUV.x, minY));
            vec4 average = centerDown+leftCenter+rightCenter+centerUp;
            float threshold = dot(average.rgb, WW) * 0.25;
            float gray1 = dot(r_Color.rgb, WW);
            vec3 maxValue = vec3(0.,0.,0.);
            maxValue.r = max(maxValue.r,centerDown.r);
            maxValue.r = max(maxValue.r,leftCenter.r);
            maxValue.r = max(maxValue.r,rightCenter.r);
            maxValue.r = max(maxValue.r,centerUp.r);
            maxValue.g = max(maxValue.g,centerDown.g);
            maxValue.g = max(maxValue.g,leftCenter.g);
            maxValue.g = max(maxValue.g,rightCenter.g);
            maxValue.g = max(maxValue.g,centerUp.g);
            maxValue.b = max(maxValue.b,centerDown.b);
            maxValue.b = max(maxValue.b,leftCenter.b);
            maxValue.b = max(maxValue.b,rightCenter.b);
            maxValue.b = max(maxValue.b,centerUp.b);
            float gray2 = dot(maxValue, WW);
            float contour = gray1 / gray2;
            float alpha = max(1.0,gray1>threshold?1.0:(gray1/threshold));
            float result = mix(gray1, contour, alpha);
            gl_FragColor = vec4(result,result,result, r_Color.w);
        }else if(u_GpuImage == 2){
            gl_FragColor = vec4(0.393*r_Color.r+0.769*r_Color.g+0.189*r_Color.b, 0.349*r_Color.r+0.686*r_Color.g+0.168*r_Color.b, 0.272*r_Color.r+0.534*r_Color.g+0.131*r_Color.b, r_Color.a);
        }else{
            gl_FragColor = r_Color;
        }
    }
);