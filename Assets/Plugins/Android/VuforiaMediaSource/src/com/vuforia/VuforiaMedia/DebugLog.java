/*==============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc.
All Rights Reserved.
Confidential and Proprietary - Protected under copyright and other laws.
==============================================================================*/

package com.vuforia.VuforiaMedia;

import android.util.Log;

/** DebugLog is a support class for the QCAR samples applications.
 * 
 *  Exposes functionality for logging.
 *  
 * */

public class DebugLog
{
    private static final String LOGTAG = "VuforiaMedia";

    /** Logging functions to generate ADB logcat messages. */

    public static final void LOGE(String nMessage)
    {
        Log.e(LOGTAG, nMessage);
    }
    
    public static final void LOGW(String nMessage)
    {
        Log.w(LOGTAG, nMessage);
    }
    
    public static final void LOGD(String nMessage)
    {
        Log.d(LOGTAG, nMessage);
    }
    
    public static final void LOGI(String nMessage)
    {
        Log.i(LOGTAG, nMessage);
    }
}
