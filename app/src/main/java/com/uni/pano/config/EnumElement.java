package com.uni.pano.config;

/**
 * Created by DELL on 2017/3/13.
 */

public class EnumElement {
    public enum VIEW_MODE {
        FISH,  PERSPECTIVE, PLANET, CRYSTAL_BALL
    };
    public enum OPTION_MODE{
        FINGER, GYROSCOPE
    }
    public enum RENDER_MODE{
        SINGLE, DOUBLE
    }
    public enum CONNECT_RESULT{
        FAIL, SUCCESS
    }
    public enum DISCONNECT_RESULT{
        FAIL, SUCCESS
    }
    public enum TAKE_PHOTO_RESULT{
        //0:开始拍照,1:没有连接相机,2:正在拍照中,3:空间不够...  tickCount为连拍次数
        START, NOT_CONNECT, PHOTOING, NOT_SPACE, COMPLETE
    }
    //0:录像开始,1:没有连接相机,2:正在录制,3:空间不够
    public enum RECORD_VIDEO_RESULT{
        DEFAULT, START, NOT_CONNECT, RECORDING, NOT_SPACE, COMPLETE, USE_RECORD_SURFACE, USE_RENDER_SURFACE,UPDATE_TIME
    };

    //
    public enum ENCODE_MESSAGE{
        START, AVAILABLE,  STOP, QUIT
    };

    //
    public enum RECORD_STATE{
        START, UPDATE_TIME, STOP
    }
    //
    public enum FILTER_TYPE{
        NONE,
        GRAY_RELIEF
    }

}
