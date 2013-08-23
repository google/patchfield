/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.noisepages.nettoyeur.patchfield.internal;

import android.annotation.TargetApi;
import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import android.util.Log;

/**
 * This class illustrates how to query OpenSL config parameters on Jelly Bean MR1 while maintaining
 * backward compatibility with older versions of Android. The trick is to place the new API calls in
 * an inner class that will only be loaded if we're running on JB MR1 or later.
 */
public abstract class OpenSlParams {

  /**
   * @return The recommended sample rate in Hz.
   */
  public abstract int getSampleRate();

  /**
   * @return The recommended buffer size in frames.
   */
  public abstract int getBufferSize();

  /**
   * @param context, e.g., the current activity.
   * @return OpenSlParams instance for the given context.
   */
  public static OpenSlParams createInstance(Context context) {
    return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
        ? new JellyBeanMr1OpenSlParams(context)
        : new DefaultOpenSlParams();
  }

  private OpenSlParams() {
    // Not meant to be instantiated except here.
  }

  // Implementation for Jelly Bean MR1 or later.
  @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
  private static class JellyBeanMr1OpenSlParams extends OpenSlParams {

    private final int sampleRate;
    private final int bufferSize;

    private JellyBeanMr1OpenSlParams(Context context) {
      AudioManager am = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
      // Provide default values in case config lookup fails.
      int sr = 44100;
      int bs = 64;
      try {
        // If possible, query the native sample rate and buffer size.
        sr = Integer.parseInt(am.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
        bs = Integer.parseInt(am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER));
      } catch (NumberFormatException e) {
        Log.w(getClass().getName(), "Failed to read native OpenSL config: " + e);
      }
      sampleRate = sr;
      bufferSize = bs;
    }

    @Override
    public int getSampleRate() {
      return sampleRate;
    }

    @Override
    public int getBufferSize() {
      return bufferSize;
    }
  };

  // Default factory for Jelly Bean or older.
  private static class DefaultOpenSlParams extends OpenSlParams {
    @Override
    public int getSampleRate() {
      return 44100;
    }

    @Override
    public int getBufferSize() {
      return 64;
    }
  };
}
