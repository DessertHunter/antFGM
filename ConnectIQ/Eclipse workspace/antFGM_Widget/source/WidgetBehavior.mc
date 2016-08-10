//!
//! Copyright 2016 by DessertHunter.
//!
using Toybox.WatchUi as Ui;
using Toybox.System as Sys;

// BehaviorDelegate handles behavior inputs. A BehaviorDelegate differs from
// an InputDelegate in that it acts upon device independent behaviours such as
// next page and previous page. On touch screen devices these behaviors might be
// mapped to the basic swipe left and right inputs, while on non-touch screen
// devices these behaviors might be mapped to actual keys.
// BehaviorDelegate extends InputDelegate so it can also act on basic inputs as
// well. If a BehaviorDelegate does return true for a function, indicating that
// the input was used, then the InputDelegate function that corresponds to the
// behavior will be called.
// @see InputDelegate
class WidgetBehavior extends Ui.BehaviorDelegate
{
    hidden var mSensor; // class ANT_FGM_Sensor
    hidden var mPageNo;
    const INITIAL_PAGE_NO = 0;
    
    //! Constructor
    function initialize(sensor) {
        BehaviorDelegate.initialize(); // You should always call the parent's initializer

        mSensor = sensor;
        mPageNo = INITIAL_PAGE_NO;
    }
    
    hidden function switchToNextPage()
    {
      if (INITIAL_PAGE_NO == mPageNo)
      {
        // ur nächstne Seite: Infoseite
        mPageNo++;
        Ui.switchToView(new LibreANT_WidgetInfoView(mSensor), self, SLIDE_RIGHT);
      }
      else
      {
        // Wieder zur Hauptseite
        Ui.switchToView(new LibreANT_WidgetMainView(mSensor), self, SLIDE_RIGHT);
        mPageNo = INITIAL_PAGE_NO;
      }
    }

    // Diese Funktion ist überladen vom InputDelegate, da der BehaviorDelegete die nicht so im Simultaro ausführt wie gewünscht
    function onSwipe(evt)
    {
        var swipe = evt.getDirection();

        if( swipe == SWIPE_RIGHT )
        {
            Sys.println("SWIPE_RIGHT");
            self.onNextPage(); // Ersatzweise
        }
        else if( swipe == SWIPE_LEFT )
        {
            Sys.println("SWIPE_LEFT");
            self.onPreviousPage(); // Ersatzweise
        }

        return true;
    }
    
    // Diese Funktion ist überladen vom InputDelegate
    //! When a screen tap event occurs, onTap() is called. This is sent if the user taps (quickly presses and releases) the screen.
    function onTap(evt) 
    {
        //Sys.println("ON_TAP");
        self.onNextPage(); // Ersatzweise

        return true;
    }


    // When a next page behavior occurs, onNextPage() is called.
    // @return [Boolean] true if handled, false otherwise
    function onNextPage()
    {
        //Sys.println("NEXT_PAGE");
        switchToNextPage();
        return true;
    }

    // When a previous page behavior occurs, onPreviousPage() is called.
    // @return [Boolean] true if handled, false otherwise
    function onPreviousPage()
    {
        Sys.println("PREVIOUS_PAGE");
        return false; // TODO_CS: true?
    }

    // When a menu behavior occurs, onMenu() is called.
    // @return [Boolean] true if handled, false otherwise
    function onMenu()
    {
        Sys.println("ON_MENU");
        return false;
    }

    // When a back behavior occurs, onBack() is called.
    // @return [Boolean] true if handled, false otherwise
    function onBack()
    {
        // Taste "Zurück" beendet auf der vivoactive HR das Widget
        Sys.println("ON_BACK");
        return false;
    } 

    // When a next mode behavior occurs, onNextMode() is called.
    // @return [Boolean] true if handled, false otherwise
    function onNextMode()
    {
        Sys.println("ON_NEXT_MODE");
        return false;
    }

    // When a previous mode behavior occurs, onPreviousMode() is called.
    // @return [Boolean] true if handled, false otherwise
    function onPreviousMode()
    {
        Sys.println("ON_PREVIOUS_MODE");
        return false;
    }
}