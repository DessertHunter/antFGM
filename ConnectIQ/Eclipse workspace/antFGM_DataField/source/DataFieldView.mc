using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.System as Sys;
using Toybox.Math as Math;
using Toybox.ActivityMonitor as Act;


class DataFieldView extends Ui.DataField {

    hidden var mSensor; // class ANT_FGM_Sensor
    hidden var mValue;
    
    const ALERTING_LOWER_THRESHOLD = 80; // Unterer Schwellwert
    const ALERTING_UPPER_THRESHOLD = 200; // Oberer Schwellwert
    hidden var mAlerting; // class Aletring
    
    hidden var mFitRecording; // class FitRecording;
    

    enum {
        GLUCOSE_PREDICTION_FALLING, 
        GLUCOSE_PREDICTION_FALLING_SLOW, 
        GLUCOSE_PREDICTION_CONSTANT, 
        GLUCOSE_PREDICTION_RISING_SLOW, 
        GLUCOSE_PREDICTION_RISING,
        GLUCOSE_PREDICTION_SIZE = 5,
        GLUCOSE_PREDICTION_UNKOWN = -1
    }
    hidden var mArrows = new [GLUCOSE_PREDICTION_SIZE];
    hidden var mArrowIndex = GLUCOSE_PREDICTION_CONSTANT;

    //! Constructor
    function initialize(sensor)
    {
        DataField.initialize();

        mSensor = sensor;
        mValue = 0.0;
        
        mAlerting = new Alerting(ALERTING_LOWER_THRESHOLD, ALERTING_UPPER_THRESHOLD);
        
        mFitRecording = new FitRecording(self);
    }
    
    //! This is called each time a lap is created, so increment the lap number.
    function onTimerLap()
    {
    	mFitRecording.onTimerLap();
    }

    //! The timer was started, so set the state to running.
    function onTimerStart()
    {
       mFitRecording.setTimerRunning(true);
    }

    //! The timer was stopped, so set the state to stopped.
    function onTimerStop()
    {
        mFitRecording.setTimerRunning(false);
    }

    //! The timer was started, so set the state to running.
    function onTimerPause()
    {
        mFitRecording.setTimerRunning(false);
    }

    //! The timer was stopped, so set the state to stopped.
    function onTimerResume()
    {
        mFitRecording.setTimerRunning(true);
    }

	//! The timer was reeset, so reset all our tracking variables
    function onTimerReset() {
        mFitRecording.onTimerReset();
    }
    

    //! Set your layout here. Anytime the size of obscurity of
    //! the draw context is changed this will be called.
    function onLayout(dc) {
        // Layout laden, in Abhängigkeit der Verdeckung z.B. bei runden Displays
        var obscurityFlags = DataField.getObscurityFlags();

        // Top left quadrant so we'll use the top left layout
        if (obscurityFlags == (OBSCURE_TOP | OBSCURE_LEFT)) {
            View.setLayout(Rez.Layouts.TopLeftLayout(dc));

        // Top right quadrant so we'll use the top right layout
        } else if (obscurityFlags == (OBSCURE_TOP | OBSCURE_RIGHT)) {
            View.setLayout(Rez.Layouts.TopRightLayout(dc));

        // Bottom left quadrant so we'll use the bottom left layout
        } else if (obscurityFlags == (OBSCURE_BOTTOM | OBSCURE_LEFT)) {
            View.setLayout(Rez.Layouts.BottomLeftLayout(dc));

        // Bottom right quadrant so we'll use the bottom right layout
        } else if (obscurityFlags == (OBSCURE_BOTTOM | OBSCURE_RIGHT)) {
            View.setLayout(Rez.Layouts.BottomRightLayout(dc));

        // Use the generic, centered layout
        } else {
            View.setLayout(Rez.Layouts.MainLayout(dc));
            var labelView = View.findDrawableById("label");
            labelView.locY = labelView.locY - 20; // 16
            var valueView = View.findDrawableById("value");
            valueView.locY = valueView.locY + 7;
        }
        
        // Prediction
        mArrows[GLUCOSE_PREDICTION_FALLING] = Ui.loadResource( Rez.Drawables.id_arrow_falling );
        mArrows[GLUCOSE_PREDICTION_FALLING_SLOW] = Ui.loadResource( Rez.Drawables.id_arrow_falling_slow );
        mArrows[GLUCOSE_PREDICTION_CONSTANT] = Ui.loadResource( Rez.Drawables.id_arrow_constant );
        mArrows[GLUCOSE_PREDICTION_RISING_SLOW] = Ui.loadResource( Rez.Drawables.id_arrow_rising_slow );
        mArrows[GLUCOSE_PREDICTION_RISING] = Ui.loadResource( Rez.Drawables.id_arrow_rising );
        
        return true;
    }

    //! The given info object contains all the current workout
    //! information. Calculate a value and save it locally in this method.
    function compute(info) {
        // See Activity.Info in the documentation for available information.
        // mValue = 0.0;
        
        mFitRecording.compute(mSensor);
        
        // DEBUG: Pfeil Animation
        if (mArrowIndex < (GLUCOSE_PREDICTION_SIZE-1))
        {
            mArrowIndex = mArrowIndex + 1;
        }
        else
        {
            // Einmal durchgewechselt...
            mArrowIndex = GLUCOSE_PREDICTION_FALLING;
        }
        
        // Die Sensorwerte werden bei Empfang durch Ui.requestUpdate() aktualisiert, daher brauchen wir hier nichts machen!
    }

    //! Display the value you computed here. This will be called
    //! once a second when the data field is visible.
    function onUpdate(dc) {
        // Set the background color
        var bg_drawable = View.findDrawableById("Background");
        if (bg_drawable instanceof Background)
        {
            bg_drawable.setColor(getBackgroundColor());
            bg_drawable.setRecording(mFitRecording.getIsRecording());
        }

        // Set the foreground color and value
        var value = View.findDrawableById("value");
        if (getBackgroundColor() == Gfx.COLOR_BLACK) {
            value.setColor(Gfx.COLOR_WHITE);
        } else {
            value.setColor(Gfx.COLOR_BLACK);
        }
        
        if (mValue instanceof Toybox.Lang.Number)
        {
            value.setText(mValue.format("%.2f"));
        }
        else if (mValue instanceof Toybox.Lang.String)
        {
            value.setText(mValue);
        }
        else
        {
            value.setText("---");
            
            // DEBUG
            // Sys.println("mValue is not a number or string!");
        }

        // Call parent's onUpdate(dc) to redraw the layout
        View.onUpdate(dc);
        
        /**********************************************************************************/
        // Ab hier Überzeichnen wir das Layout
        try {
            // Update status
            var status_text_x =  10;
            var status_text_y =  dc.getHeight() - 25;
            dc.setColor(Gfx.COLOR_BLUE, Gfx.COLOR_TRANSPARENT);
                        
            if(mSensor == null)
            {
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "No Channel!", Gfx.TEXT_JUSTIFY_LEFT);
            }
            else if (mSensor.searching)
            {
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "Searching...", Gfx.TEXT_JUSTIFY_LEFT);
            }
            else if (!mSensor.searching)
            {
                var glucose_value = mSensor.data.glucoseValue;
                mValue = glucose_value.format("%u");
                
                mAlerting.doUpdate(glucose_value);
                
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "Connected", Gfx.TEXT_JUSTIFY_LEFT);
            }
            else
            {
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "Unknown!", Gfx.TEXT_JUSTIFY_LEFT);
            }
            
            // Arrow rechts mittig mit bisschen Platz zum Rand platzieren
            var arrow_x = dc.getWidth() - (32+5);
            var arrow_y = dc.getHeight()/2 - (32/2);
            dc.drawBitmap(arrow_x, arrow_y, mArrows[mArrowIndex]);
        }
        catch( ex instanceof UnexpectedTypeException ) {
            // Code to handle the throw of UnexpectedTypeException
            Sys.println("UnexpectedTypeException in onUpdate");
        }
        catch( ex ) {
            // Code to catch all execeptions
            Sys.println("UnexpectedTypeException in onUpdate");
            Sys.printStackTrace();
        }
        finally {
            // Code to execute when
        }
    }
}
