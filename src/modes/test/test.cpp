#include <stak.h>
#include <iostream>

// prototypes
STAK_EXPORT {
    int init           ( );
    int update         ( float delta );
    int draw           ( );
    int shutdown       ( );
    int shutter_up     ( );
    int shutter_down   ( );
    int power_up       ( );
    int power_down     ( );
    int crank_up       ( );
    int crank_down     ( );
    int crank_rotated  ( int amount );
};



//
// function:
//
int init( ) {
    return 0;
}

//
// function:
//
int update( float delta ) {
    //std::cout << "Delta: " << delta << std::endl;
    return 0;
}

//
// function:
//
int draw( ) {
    return 0;
}

//
// function:
//
int shutdown( ) {
    return 0;
}

//
// function:
//
int shutter_up( ) {
    return 0;
}

//
// function:
//
int shutter_down( ) {
    return 0;
}

//
// function:
//
int power_up( ) {
    return 0;
}

//
// function:
//
int power_down( ) {
    return 0;
}

//
// function:
//
int crank_up( ) {
    return 0;
}

//
// function:
//
int crank_down( ) {
    return 0;
}

//
// function:
//
int crank_rotated( int amount ) {
    return 0;
}