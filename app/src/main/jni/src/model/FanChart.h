#pragma once
#include <math.h>
#include <string.h>
#include "Singleton.h"
class FanChart{
	public:
        FanChart();
        ~FanChart();
        void    init(int perVertex, float radius, float latitude, float longitude);
        int     _size;
        float*  _vertexUVBuff;
    private:
        int     _perVertex;
        float   _radius;
        float   _latitude;
        float   _longitude;
    public:
        void    draw();
};