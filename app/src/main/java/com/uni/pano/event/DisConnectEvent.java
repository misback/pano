package com.uni.pano.event;

import com.uni.pano.config.EnumElement;

/**
 * Created by DELL on 2017/3/22.
 */

public class DisConnectEvent {
    public EnumElement.DISCONNECT_RESULT disconnect_result;
    public DisConnectEvent(EnumElement.DISCONNECT_RESULT disconnectResult){
        disconnect_result = disconnectResult;
    }
}
