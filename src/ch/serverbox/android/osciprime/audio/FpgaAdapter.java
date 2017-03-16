/**
  * This file is part of OsciPrime
  *
  * Copyright (C) 2011 - Manuel Di Cerbo, Andreas Rudolf
  * 
  * Nexus-Computing GmbH, Switzerland 2011
  *
  * OsciPrime is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * OsciPrime is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with OsciPrime; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin St, Fifth Floor, 
  * Boston, MA  02110-1301  USA
  */
package ch.serverbox.android.osciprime.audio;

import com.example.sensor.Sensor;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public class FpgaAdapter {

	private Looper mLooper = null;
	private Handler mHandler = null;
	private boolean mStop = false;
	private boolean mStopped = false;
	private final ch.serverbox.android.osciprime.sources.FpgaSource mFpgaSource;
	
	private Sensor mSensor;
	private final Object mLock = new Object();

	public int MaxCh = 0;
	public int TiCh = 0;
	
	public FpgaAdapter(ch.serverbox.android.osciprime.sources.FpgaSource fpgaSource){
		mFpgaSource = fpgaSource;
		mSensor = new Sensor();
		mFpgaSamplingThread.start();
		synchronized (mLock) {			
			while(mHandler == null){
				try {
					mLock.wait();
				} catch (InterruptedException e) {
					e("can't wait for lock, interrupted");
					e.printStackTrace();
				}
			}
		}
	}

	public void startSampling(){
		mHandler.post(mFpgaLoop);
	}
	
	public void stopSampling(){
		mStop = true;
		synchronized (mLock) {
			while(!mStopped){
				try {
					mLock.wait();
				} catch (InterruptedException e) {
					e("can't wait for lock");
					e.printStackTrace();
				}
			}
		}
		mStopped = false;//clean up for next time
		mStop = false;
		l("stopped");
	}

	public void quit(){
		l("quitting ...");
		if(mLooper ==  null)
			return;
		mLooper.quit();
		try {
			mFpgaSamplingThread.join();
		} catch (InterruptedException e) {
			e("could not join AudioSamplingthread, interrupted");
			e.printStackTrace();
		}
		l("threads joined ...");
	}

	private Thread mFpgaSamplingThread = new Thread(new Runnable() {
		public void run() {
			Looper.prepare();
			synchronized (mLock) {
				mHandler = new Handler();
				mLooper = Looper.myLooper();
				mLock.notifyAll();
			}
			Looper.loop();
		}
	});

	public static final int NEW_BYTE_SAMPLES = 99;
	public static final int NEW_SHORT_SAMPLES = 100;
	
	private Runnable mFpgaLoop = new Runnable() {
		public void run() {
			final int BLOCK_SIZE = mFpgaSource.cBlocksize();
			int[] dataBufferCh1 = new int[BLOCK_SIZE];
			int[] dataBufferCh2 = new int[BLOCK_SIZE];

			l("Block size: "+BLOCK_SIZE);
			
			if( mSensor.fpgaOpen(25) != 0 ) {
				e("fpgaOpen error !!");
				
				mStop = true;
			}
			
			for(;;){//this is the loop
				synchronized(mLock){
					if(mStop){//break condition
						mStopped = true;
						mLock.notifyAll();
						mSensor.fpgaAbort();
						return;
					}						
				}
				try {
					Thread.sleep(10);
				} catch (InterruptedException e) {
					return;
				}
				int nRet = mSensor.fpgaGetData();
				int val = 0;
				if( nRet == 0 ) {
					mSensor.buf16.position(0);
					mSensor.buf241.position(0);
					mSensor.buf242.position(0);
					mSensor.buf243.position(0);
					mSensor.buf244.position(0);
					for (int i = 0; i < BLOCK_SIZE; i++) {
						if(MaxCh == 0) {
							dataBufferCh1[i] = mSensor.buf16.getChar()-32768;
							mSensor.buf16.getChar();
						}
						else {
							mSensor.buf16.getChar();
							dataBufferCh1[i] = mSensor.buf16.getChar()-32768;
						}
						switch( TiCh ) {
						case 0:
							val = mSensor.buf241.getInt();
							break;
						case 1:
							val = mSensor.buf242.getInt();
							break;
						case 2:
							val = mSensor.buf243.getInt();
							break;
						case 3:
							val = mSensor.buf244.getInt();
							break;
						}
						
						val <<=8;
						val >>=16;
						dataBufferCh2[i] = val; 
//						Log.e("mmm","i="+i+","+val);
					}

					mFpgaSource.onNewSamples(dataBufferCh1, dataBufferCh2);
				}
			}
		}
	};
	
	private void e(String msg){
		Log.e("FpgaAdapter", ">==< "+msg+" >==<");
	}
	private void l(String msg){
		Log.d("FpgaAdapter", ">==< "+msg+" >==<");
	}
}
