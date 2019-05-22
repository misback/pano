#include "SphereData.h"
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
    for (int a = 0; a < _perVertex; a++) {
        for (int b = 0; b < _perVertex; b++) {
            float w1 = (float)(a * perH);      float h1 = 1.0-(float) (b * perW);
            float w2 = (float)(w1 + perH);     float h2 = h1;
            float w3 = w2;                      float h3 = (float)(h1 - perW);
            float w4 = w1;                      float h4 = h3;
            float z1 = _radius*(float) (sin(a * hemisphere) * cos(b* sphere));
            float x1 = _radius*(float) (sin(a * hemisphere) * sin(b* sphere));
            float y1 = _radius*(float) cos(a * hemisphere);
            float z2 = _radius*(float) (sin((a + 1) * hemisphere) * cos(b * sphere));
            float x2 = _radius*(float) (sin((a + 1) * hemisphere) * sin(b * sphere));
            float y2 = _radius*(float) cos((a + 1) * hemisphere);
            float z3 = _radius*(float) (sin((a + 1) * hemisphere) * cos((b + 1) * sphere));
            float x3 = _radius*(float) (sin((a + 1) * hemisphere) * sin((b + 1) * sphere));
            float y3 = _radius*(float) cos((a + 1) * hemisphere);
            float z4 = _radius*(float) (sin(a * hemisphere) * cos((b + 1) * sphere));
            float x4 = _radius*(float) (sin(a * hemisphere) * sin((b + 1) * sphere));
            float y4 = _radius*(float) cos(a * hemisphere);
            int startPos	=	(a*_perVertex+b)*30;
            _vertexUVBuff[startPos + 0]     =   x1; _vertexUVBuff[startPos + 1]	    =   y1; _vertexUVBuff[startPos + 2]	    =	z1;
            _vertexUVBuff[startPos + 3]     =	h1; _vertexUVBuff[startPos + 4]	    =   w1;

            _vertexUVBuff[startPos + 5]     =	x2; _vertexUVBuff[startPos + 6]	    =	y2; _vertexUVBuff[startPos + 7]	    =	z2;
            _vertexUVBuff[startPos + 8]	    =	h2; _vertexUVBuff[startPos + 9]	    =	w2;

            _vertexUVBuff[startPos + 10]    =	x3; _vertexUVBuff[startPos + 11]	=	y3; _vertexUVBuff[startPos + 12]	=	z3;
            _vertexUVBuff[startPos + 13]	=	h3; _vertexUVBuff[startPos + 14]	=	w3;

            _vertexUVBuff[startPos + 15]	=	x3; _vertexUVBuff[startPos + 16]	=	y3; _vertexUVBuff[startPos + 17]	=	z3;
            _vertexUVBuff[startPos + 18]	=	h3; _vertexUVBuff[startPos + 19]	=	w3;

            _vertexUVBuff[startPos + 20]	=	x4; _vertexUVBuff[startPos + 21]	=	y4; _vertexUVBuff[startPos + 22]	=	z4;
            _vertexUVBuff[startPos + 23]	=	h4; _vertexUVBuff[startPos + 24]	=	w4;

            _vertexUVBuff[startPos + 25]	=	x1; _vertexUVBuff[startPos + 26]	=	y1; _vertexUVBuff[startPos + 27]	=	z1;
            _vertexUVBuff[startPos + 28]	=	h1; _vertexUVBuff[startPos + 29]	=	w1;
        }
    }
 /*   for (int a = 0; a < _perVertex; a++) {
        for (int b = 0; b < _perVertex; b++) {
            float w1 = (float)(a * perH);      float h1 = 1.0-(float) (b * perW);
            float w2 = (float)(w1 + perH);     float h2 = h1;
            float w3 = w2;                      float h3 = (float)(h1 - perW);
            float w4 = w1;                      float h4 = h3;
            float z1 = _radius*(float) (sin(a * hemisphere) * cos(b* sphere));
            float x1 = _radius*(float) (sin(a * hemisphere) * sin(b* sphere));
            float y1 = _radius*(float) cos(a * hemisphere);
            float z2 = _radius*(float) (sin((a + 1) * hemisphere) * cos(b * sphere));
            float x2 = _radius*(float) (sin((a + 1) * hemisphere) * sin(b * sphere));
            float y2 = _radius*(float) cos((a + 1) * hemisphere);
            float z3 = _radius*(float) (sin((a + 1) * hemisphere) * cos((b + 1) * sphere));
            float x3 = _radius*(float) (sin((a + 1) * hemisphere) * sin((b + 1) * sphere));
            float y3 = _radius*(float) cos((a + 1) * hemisphere);
            float z4 = _radius*(float) (sin(a * hemisphere) * cos((b + 1) * sphere));
            float x4 = _radius*(float) (sin(a * hemisphere) * sin((b + 1) * sphere));
            float y4 = _radius*(float) cos(a * hemisphere);
            int startPos	=	(a*_perVertex+b)*30;
            _vertexUVBuff[startPos + 0]     =   x1; _vertexUVBuff[startPos + 1]	    =   y1; _vertexUVBuff[startPos + 2]	    =	z1;
            _vertexUVBuff[startPos + 3]     =	h1; _vertexUVBuff[startPos + 4]	    =   w1;

            _vertexUVBuff[startPos + 5]     =	x2; _vertexUVBuff[startPos + 6]	    =	y2; _vertexUVBuff[startPos + 7]	    =	z2;
            _vertexUVBuff[startPos + 8]	    =	h2; _vertexUVBuff[startPos + 9]	    =	w2;

            _vertexUVBuff[startPos + 10]    =	x3; _vertexUVBuff[startPos + 11]	=	y3; _vertexUVBuff[startPos + 12]	=	z3;
            _vertexUVBuff[startPos + 13]	=	h3; _vertexUVBuff[startPos + 14]	=	w3;

            _vertexUVBuff[startPos + 15]	=	x3; _vertexUVBuff[startPos + 16]	=	y3; _vertexUVBuff[startPos + 17]	=	z3;
            _vertexUVBuff[startPos + 18]	=	h3; _vertexUVBuff[startPos + 19]	=	w3;

            _vertexUVBuff[startPos + 20]	=	x4; _vertexUVBuff[startPos + 21]	=	y4; _vertexUVBuff[startPos + 22]	=	z4;
            _vertexUVBuff[startPos + 23]	=	h4; _vertexUVBuff[startPos + 24]	=	w4;

            _vertexUVBuff[startPos + 25]	=	x1; _vertexUVBuff[startPos + 26]	=	y1; _vertexUVBuff[startPos + 27]	=	z1;
            _vertexUVBuff[startPos + 28]	=	h1; _vertexUVBuff[startPos + 29]	=	w1;
        }
    }*/
}
