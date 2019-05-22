package com.uni.pano.event;

import com.uni.pano.config.EnumElement;

/**
 * Created by DELL on 2017/3/22.
 */

public class ConnectEvent {
    public EnumElement.CONNECT_RESULT connect_result;
    public ConnectEvent(EnumElement.CONNECT_RESULT connectResult){
        connect_result = connectResult;
    }
}
