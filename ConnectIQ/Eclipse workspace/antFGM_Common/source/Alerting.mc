//!
//! Copyright 2016 DessertHunter
//!
using Toybox.System as Sys;
using Toybox.Attention as Att;
using Toybox.Test as Test;


//! 
//! 
//! 
class Alerting {

    hidden var mLowerThreshold, mUpperThreshold;
    hidden var mOldValue; // Number

    hidden var mEnableVibrate; // boolean
    hidden var mVibrateLow; // Array of class VibeProfile
    hidden var mVibrateHigh; // Array of class VibeProfile

    hidden var mEnableTone; // boolean

    //! Constructor
    function initialize(lower_threshold, upper_threshold)
    {
        Test.assertMessage(lower_threshold instanceof Toybox.Lang.Number, "lower_threshold not a Number!");
        Test.assertMessage(upper_threshold instanceof Toybox.Lang.Number, "upper_threshold not a Number!");
        Test.assertMessage(lower_threshold < upper_threshold, "lower_threshold not lower then upper_threshold!");
    
        mLowerThreshold = lower_threshold;
        mUpperThreshold = upper_threshold;
        mOldValue = (upper_threshold - lower_threshold) / 2;
        
        mEnableVibrate = (Att has :vibrate);
        if (mEnableVibrate)
        {
            mVibrateLow = [new Att.VibeProfile(100, 100), new Att.VibeProfile(0, 100), new Att.VibeProfile(100, 100)]; // Array
    		mVibrateHigh = [new Att.VibeProfile(50, 100), new Att.VibeProfile(10, 200), new Att.VibeProfile(100, 50)]; // Array
        }
        
        mEnableTone = (Att has :playTone);
    }
    
    //! Update the value
    function doUpdate(value)
    {
        Test.assertMessage(value instanceof Toybox.Lang.Number, "value is not a Number!");

        if ((value < mLowerThreshold) && (mOldValue >= mLowerThreshold))
        {
            // jetzt ist der Wert zu niedrig
            onGettingLow();
        }
        else if ((value > mUpperThreshold) && (mOldValue <= mUpperThreshold))
        {
            // jetzt ist der Wert zu hoch
            onGettingHigh();
        }
        
        mOldValue = value;
    }
    
    hidden function onGettingLow()
    {
        if (mEnableVibrate)
        {
          Att.vibrate(mVibrateLow);
        }
        
        if (mEnableTone)
        {
            Att.playTone(Att.TONE_ALERT_LO);
        }
    }
    
    hidden function onGettingHigh()
    {
        if (mEnableVibrate)
        {
          Att.vibrate(mVibrateHigh);
        }
        
        if (mEnableTone)
        {
            Att.playTone(Att.TONE_ALERT_HI);
        }
    }
}
