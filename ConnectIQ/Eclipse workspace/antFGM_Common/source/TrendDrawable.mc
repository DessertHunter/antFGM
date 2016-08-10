//!
//! Copyright 2016 DessertHunter
//!
using Toybox.WatchUi as Ui;
using Toybox.Application as App;
using Toybox.Graphics as Gfx;

class TrendDrawable extends Ui.Drawable {

    hidden var mColor;
    hidden var mX, mY, mRadius;
        
    enum {
        GLUCOSE_PREDICTION_UNKNOWN,
        GLUCOSE_PREDICTION_FALLING, 
        GLUCOSE_PREDICTION_FALLING_SLOW, 
        GLUCOSE_PREDICTION_CONSTANT, 
        GLUCOSE_PREDICTION_RISING_SLOW, 
        GLUCOSE_PREDICTION_RISING,
        GLUCOSE_PREDICTION_SIZE
    }
    //hidden var mArrowRezIds = new [GLUCOSE_PREDICTION_SIZE];
    hidden var mArrows = new [GLUCOSE_PREDICTION_SIZE];
    hidden var mArrowIndex = GLUCOSE_PREDICTION_UNKNOWN;

    function initialize(params) {
        // You should always call the parent's initializer and
        // in this case you should pass the params along as size
        // and location values may be defined.
        Drawable.initialize(params); // params = {:identifier => "Trend" };

        // Get any extra values you wish to use out of the params Dictionary
        mColor = params.get(:color);
        mX = params.get(:x);
        mY = params.get(:y);
        mRadius = params.get(:radius);
        
        // Prediction
        //mArrowRezIds[GLUCOSE_PREDICTION_UNKNOWN] = Rez.Drawables.id_arrow_unknown;
        //mArrowRezIds[GLUCOSE_PREDICTION_FALLING] = Rez.Drawables.id_arrow_falling;
        //mArrowRezIds[GLUCOSE_PREDICTION_FALLING_SLOW] = Rez.Drawables.id_arrow_falling_slow;
        //mArrowRezIds[GLUCOSE_PREDICTION_CONSTANT] = Rez.Drawables.id_arrow_constant;
        //mArrowRezIds[GLUCOSE_PREDICTION_RISING_SLOW] = Rez.Drawables.id_arrow_rising_slow;
        //mArrowRezIds[GLUCOSE_PREDICTION_RISING] = Rez.Drawables.id_arrow_rising;
        mArrows[GLUCOSE_PREDICTION_UNKNOWN] = Ui.loadResource( Rez.Drawables.id_arrow_unknown );
        mArrows[GLUCOSE_PREDICTION_FALLING] = Ui.loadResource( Rez.Drawables.id_arrow_falling );
        mArrows[GLUCOSE_PREDICTION_FALLING_SLOW] = Ui.loadResource( Rez.Drawables.id_arrow_falling_slow );
        mArrows[GLUCOSE_PREDICTION_CONSTANT] = Ui.loadResource( Rez.Drawables.id_arrow_constant );
        mArrows[GLUCOSE_PREDICTION_RISING_SLOW] = Ui.loadResource( Rez.Drawables.id_arrow_rising_slow );
        mArrows[GLUCOSE_PREDICTION_RISING] = Ui.loadResource( Rez.Drawables.id_arrow_rising );
    }

    function setColor(color) {
        mColor = color;
    }
    
    function setArrow(arrow_index) {
        mArrowIndex = arrow_index;
    }
    
    function draw(dc) {
        dc.setColor(mColor, Gfx.COLOR_TRANSPARENT);
        dc.fillCircle(mX, mY, mRadius);
        
        // Arrow rechts mittig mit bisschen Platz zum Rand platzieren
        //View.findDrawableById("arrow").setBitmap(mArrowRezIds[mArrowIndex]);
        
        dc.drawBitmap(mX-16, mY-16, mArrows[mArrowIndex]);
    }
}
