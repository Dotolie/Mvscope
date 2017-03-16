package com.example.sensor;

import java.nio.ByteBuffer;

import android.util.Log;

/*
 * Copyright (C) 2015 MVTECH Sensor Example Program
 *
 */

public class Sensor 
{
	static final String TAG = "Sensor";
	public ByteBuffer buf16;
	public ByteBuffer buf241;
	public ByteBuffer buf242;
	public ByteBuffer buf243;
	public ByteBuffer buf244;


	public interface EventListner {
		void onCaptured(byte[] a, byte[] b);
	}
	private static EventListner mCapturedCb;
	
	public void setOnEventListner( EventListner callback) {
		mCapturedCb = callback;
	}
  
    public void onDatas(ByteBuffer aMax, ByteBuffer aAds1, ByteBuffer aAds2, ByteBuffer aAds3, ByteBuffer aAds4) {
    	
    	buf16 = aMax;
    	buf241 = aAds1;
    	buf242 = aAds2;
    	buf243 = aAds3;
    	buf244 = aAds4;
//    	Log.e(TAG, "---long="+buf.get(0));
    }
    
    public native int fpgaOpen(int freq);
    public native void fpgaAbort();
    public native int fpgaGetData();
    //flash api
    public native int fpgaDaqTest(int freq);
    public native int fpgaStop();
    public native int flashRead(String filename);
    public native int flashUpdate(String filename);
    
    //vibrator api ��������
    public native int vibratortest();
    public native int Wirelessvibtest();
    public native void CallBackInit();
    
    public native void DeviceInit();
    public native void DeviceClose();
    
    //gpio api
    public native void GpioSetDirection(String port, boolean dir);
    public native void GpioSetValue(String port, boolean value);
    public native boolean GpioGetValue(String port);

    //ethernet
    public native int connect(String server_ip, String client_ip);
    public native int disconnect();
    public native int send(String msg);

    static {
        System.loadLibrary("sensor");
        Log.e(TAG, "---------load syslib-----");
    }
}
