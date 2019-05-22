#include "SphereData.h"
#include "glm.hpp"
#define NOT_VALID_FLOAT -19999.9f
SphereData::SphereData():
    _perVertex(0),
    _size(0),
    _vertexUVBuff(nullptr),
    _radius(0.f){

}
SphereData::~SphereData(){
    if(_vertexUVBuff != nullptr){
        delete [] _vertexUVBuff;
        _vertexUVBuff   =   nullptr;
    }
}
void SphereData::init(int perVertex, float radius){
    if(_perVertex == perVertex && _radius == radius){
        return;
    }
    _perVertex      =   perVertex;
    _radius         =   radius;
    if(_vertexUVBuff != nullptr){
        delete [] _vertexUVBuff;
        _vertexUVBuff   =   nullptr;
    }
    _size			=	   _perVertex * _perVertex * 6;
    _vertexUVBuff   =      new float[_perVertex * _perVertex * 30];
    double hemisphere       =      M_PI / (float) _perVertex;
    double sphere           =      2 * M_PI / (float) _perVertex;
    double perH = 1 / (double) _perVertex;
    double perW = 1 / (double) _perVertex;
    Vertex temp[_perVertex+1];
    auto lessPerVertex = _perVertex-1;
    for (int a = 0; a < _perVertex; a++) {
        Vertex temp3;
        Vertex temp4;
        if(a==0){
            for (int b = 0; b < _perVertex; b++) {
                            float w1 = (float)(a * perH);      float h1 = 1.0-(float) (b * perW);
                            float w2 = (float)(w1 + perH);     float h2 = h1;
                            float w3 = w2;                      float h3 = (float)(h1 - perW);
                            float w4 = w1;                      float h4 = h3;
                Vertex v1, v2, v3, v4;
                if(b == 0){
                    v1.z = _radius*(float) (sin(a * hemisphere) * cos(b* sphere));
                    v1.x = _radius*(float) (sin(a * hemisphere) * sin(b* sphere));
                    v1.y = _radius*(float) cos(a * hemisphere);
                    v1.u = 1.0-(float) (b * perW);
                    v1.v = (float)(a * perH);
                    v2.z = _radius*(float) (sin((a + 1) * hemisphere) * cos(b * sphere));
                    v2.x = _radius*(float) (sin((a + 1) * hemisphere) * sin(b * sphere));
                    v2.y = _radius*(float) cos((a + 1) * hemisphere);
                    v2.u = v1.u;
                    v2.v = v1.v + perH;
                    v3.z = _radius*(float) (sin((a + 1) * hemisphere) * cos((b + 1) * sphere));
                    v3.x = _radius*(float) (sin((a + 1) * hemisphere) * sin((b + 1) * sphere));
                    v3.y = _radius*(float) cos((a + 1) * hemisphere);
                    v3.u = v1.u - perW;
                    v3.v = v2.v;
                    v4.z = _radius*(float) (sin(a * hemisphere) * cos((b + 1) * sphere));
                    v4.x = _radius*(float) (sin(a * hemisphere) * sin((b + 1) * sphere));
                    v4.y = _radius*(float) cos(a * hemisphere);
                    v4.u = v3.u;
                    v4.v = v1.v;
                    temp3 = v3;
                    temp4 = v4;
                }else{
                    v1 = temp4;
                    v2 = temp3;
                    v3.z = _radius*(float) (sin((a + 1) * hemisphere) * cos((b + 1) * sphere));
                    v3.x = _radius*(float) (sin((a + 1) * hemisphere) * sin((b + 1) * sphere));
                    v3.y = _radius*(float) cos((a + 1) * hemisphere);
                    v3.u = v1.u - perW;
                    v3.v = v2.v;
                    v4.z = _radius*(float) (sin(a * hemisphere) * cos((b + 1) * sphere));
                    v4.x = _radius*(float) (sin(a * hemisphere) * sin((b + 1) * sphere));
                    v4.y = _radius*(float) cos(a * hemisphere);
                    v4.u = v3.u;
                    v4.v = v1.v;
                    temp3 = v3;
                    temp4 = v4;
                }
                int startPos	=	(a*_perVertex+b)*30;
                _vertexUVBuff[startPos + 0]     =   v1.x; _vertexUVBuff[startPos + 1]	    =   v1.y; _vertexUVBuff[startPos + 2]	    =	v1.z;
                _vertexUVBuff[startPos + 3]     =	v1.u; _vertexUVBuff[startPos + 4]	    =   v1.v;

                _vertexUVBuff[startPos + 5]     =	v2.x; _vertexUVBuff[startPos + 6]	    =	v2.y; _vertexUVBuff[startPos + 7]	    =	v2.z;
                _vertexUVBuff[startPos + 8]	    =	v2.u; _vertexUVBuff[startPos + 9]	    =	v2.v;

                _vertexUVBuff[startPos + 10]    =	v3.x; _vertexUVBuff[startPos + 11]	    =	v3.y; _vertexUVBuff[startPos + 12]	    =	v3.z;
                _vertexUVBuff[startPos + 13]	=	v3.u; _vertexUVBuff[startPos + 14]	    =	v3.v;

                _vertexUVBuff[startPos + 15]	=	v3.x; _vertexUVBuff[startPos + 16]	=	v3.y; _vertexUVBuff[startPos + 17]	=	v3.z;
                _vertexUVBuff[startPos + 18]	=	v3.u; _vertexUVBuff[startPos + 19]	=	v3.v;

                _vertexUVBuff[startPos + 20]	=	v4.x; _vertexUVBuff[startPos + 21]	=	v4.y; _vertexUVBuff[startPos + 22]	=	v4.z;
                _vertexUVBuff[startPos + 23]	=	v4.u; _vertexUVBuff[startPos + 24]	=	v4.v;

                _vertexUVBuff[startPos + 25]	=	v1.x; _vertexUVBuff[startPos + 26]	=	v1.y; _vertexUVBuff[startPos + 27]	=	v1.z;
                _vertexUVBuff[startPos + 28]	=	v1.u; _vertexUVBuff[startPos + 29]	=	v1.v;
                temp[b] = v2;
                temp[b+1] = v3;
            }
        }else{
            for (int b = 0; b < _perVertex; b++) {
                float w1 = (float)(a * perH);      float h1 = 1.0-(float) (b * perW);
                float w2 = (float)(w1 + perH);     float h2 = h1;
                float w3 = w2;                      float h3 = (float)(h1 - perW);
                float w4 = w1;                      float h4 = h3;
                Vertex v1, v2, v3, v4;
                if(b == 0){
                    v1 = temp[b];
                    v2.z = _radius*(float) (sin((a + 1) * hemisphere) * cos(b * sphere));
                    v2.x = _radius*(float) (sin((a + 1) * hemisphere) * sin(b * sphere));
                    v2.y = _radius*(float) cos((a + 1) * hemisphere);
                    v2.u = v1.u;
                    v2.v = v1.v + perH;
                    v3.z = _radius*(float) (sin((a + 1) * hemisphere) * cos((b + 1) * sphere));
                    v3.x = _radius*(float) (sin((a + 1) * hemisphere) * sin((b + 1) * sphere));
                    v3.y = _radius*(float) cos((a + 1) * hemisphere);
                    v3.u = v1.u - perW;
                    v3.v = v2.v;
                    v4 = temp[b+1];
                    temp3 = v3;
                    temp4 = v4;
                    temp[b] = v2;
                    temp[b+1] = v3;
                }else{
                    v1 = temp4;
                    v2 = temp3;
                    v3.z = _radius*(float) (sin((a + 1) * hemisphere) * cos((b + 1) * sphere));
                    v3.x = _radius*(float) (sin((a + 1) * hemisphere) * sin((b + 1) * sphere));
                    v3.y = _radius*(float) cos((a + 1) * hemisphere);
                    v3.u = v1.u - perW;
                    v3.v = v2.v;
                    v4 = temp[b+1];
                    temp3 = v3;
                    temp4 = v4;
                    temp[b] = v2;
                    temp[b+1] = v3;
                }
                int startPos	=	(a*_perVertex+b)*30;
                _vertexUVBuff[startPos + 0]     =   v1.x; _vertexUVBuff[startPos + 1]	    =   v1.y; _vertexUVBuff[startPos + 2]	    =	v1.z;
                _vertexUVBuff[startPos + 3]     =	v1.u; _vertexUVBuff[startPos + 4]	    =   v1.v;

                _vertexUVBuff[startPos + 5]     =	v2.x; _vertexUVBuff[startPos + 6]	    =	v2.y; _vertexUVBuff[startPos + 7]	    =	v2.z;
                _vertexUVBuff[startPos + 8]	    =	v2.u; _vertexUVBuff[startPos + 9]	    =	v2.v;

                _vertexUVBuff[startPos + 10]    =	v3.x; _vertexUVBuff[startPos + 11]	    =	v3.y; _vertexUVBuff[startPos + 12]	    =	v3.z;
                _vertexUVBuff[startPos + 13]	=	v3.u; _vertexUVBuff[startPos + 14]	    =	v3.v;

                _vertexUVBuff[startPos + 15]	=	v3.x; _vertexUVBuff[startPos + 16]	=	v3.y; _vertexUVBuff[startPos + 17]	=	v3.z;
                _vertexUVBuff[startPos + 18]	=	v3.u; _vertexUVBuff[startPos + 19]	=	v3.v;

                _vertexUVBuff[startPos + 20]	=	v4.x; _vertexUVBuff[startPos + 21]	=	v4.y; _vertexUVBuff[startPos + 22]	=	v4.z;
                _vertexUVBuff[startPos + 23]	=	v4.u; _vertexUVBuff[startPos + 24]	=	v4.v;

                _vertexUVBuff[startPos + 25]	=	v1.x; _vertexUVBuff[startPos + 26]	=	v1.y; _vertexUVBuff[startPos + 27]	=	v1.z;
                _vertexUVBuff[startPos + 28]	=	v1.u; _vertexUVBuff[startPos + 29]	=	v1.v;
            }
        }
    }
}
