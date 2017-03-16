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
package ch.serverbox.android.osciprime.sources;

import android.os.Handler;
import android.util.Log;
import ch.serverbox.android.osciprime.OPC;
import ch.serverbox.android.osciprime.OsciPreferences;
import ch.serverbox.android.osciprime.audio.FpgaAdapter;

public class FpgaSource extends SourceBase{

	private final FpgaAdapter mFpgaAdapter;
	public FpgaSource(Handler sink, OsciPreferences pref) {
		super(sink,pref);
		mFpgaAdapter = new FpgaAdapter(this);
	}

	@Override
	public int cBlocksize() {
		return 500;
	}

	@Override
	public int cSignedNess() {
		return SIGNEDNESS_SIGNED;
	}

	@Override
	public int cRange() {
		return RANGE_SHORT;
	}

	@Override 
	public GainTripplet[] cGainTrippletsCh1() {
		return new GainTripplet[]{
				new GainTripplet((byte)0x20, "2.5[V]", 1.04f, 2.5f),
				new GainTripplet((byte)0x40, "5.0[V]", 0.52f, 5.0f),
		};
	}
	@Override
	public GainTripplet[] cGainTrippletsCh2() {
		return new GainTripplet[]{
				new GainTripplet((byte)0x02, "2.5[V]", 1.0f, 2.5f),
				new GainTripplet((byte)0x04, "5.0[V]", 0.5f, 5.0f),
		};
	}

	@Override
	public TimeDivisionPair[] cTimeDivisionPairs() {
		return new TimeDivisionPair[]{
				new TimeDivisionPair(1, String.format("%2d [uS]", 500), 500),
				new TimeDivisionPair(2, String.format("%2d [uS]", 1000), 1000)
		};
	}

	@Override
	public void loop() {
		mFpgaAdapter.startSampling();
	}

	@Override
	public void stop() {
		mFpgaAdapter.stopSampling();
	}

	@Override
	public void quit() {
		mFpgaAdapter.quit();
	}

	@Override
	public int cSourceId() {
		return OPC.SOURCE_FPGA;
	}

	@Override
	public void sendCommand(byte cmd) {
		// TODO Auto-generated method stub
		switch( cmd ) {
		case 0x10:
			mFpgaAdapter.MaxCh = 0;
			break;
		case 0x11:
			mFpgaAdapter.MaxCh = 1;
			break;
		}
		
		switch(cmd) {
		case 0x20:
			mFpgaAdapter.TiCh = 0;
			break;
		case 0x21:
			mFpgaAdapter.TiCh = 1;
			break;
		case 0x22:
			mFpgaAdapter.TiCh = 2;
			break;
		case 0x23:
			mFpgaAdapter.TiCh = 3;
			break;
		}
		Log.e("kkk", "byte="+cmd);
	}
	
	
}
