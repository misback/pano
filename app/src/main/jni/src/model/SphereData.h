#pragma once
#include <math.h>
#include <string.h>
#include "Singleton.h"
struct Vertex{
    float x;
    float y;
    float z;
    float u;
    float v;
    Vertex(){
        x = 0.f;
        y = 0.f;
        z = 0.f;
        u = 0.f;
        v = 0.f;
    }
};
class SphereData{
	public:
        SphereData();
        ~SphereData();
        void    init(int perVertex, float radius);
        int     _size;
        float*  _vertexUVBuff;
    private:
        int     _perVertex;
        float   _radius;
};