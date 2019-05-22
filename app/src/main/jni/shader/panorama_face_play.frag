const static char* panorama_face_play_frag    =   STRINGIFY(

    \n#extension GL_OES_EGL_image_external : require\n
    precision mediump float;
    varying vec2 v_TextureUV;
    uniform samplerExternalOES u_CameraTextureOES;
    void main(void){
        vec4 r_Color = texture2D(u_CameraTextureOES, v_TextureUV) ;
        float value = (r_Color.r + r_Color.g+r_Color.b)/3.0;
        gl_FragColor = vec4(value, value, value, value);
    }
);