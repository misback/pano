package com.uni.pano.entity;

public class Result<T> {
    public int rtn;
    public String msg;
    public T data;

    @Override
    public String toString() {
        return "Result{" +
                "rtn=" + rtn +
                ", msg='" + msg + '\'' +
                ", data=" + data +
                '}';
    }
}
