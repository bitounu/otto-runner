#ifndef STAK_H
#define STAK_H
#define LOG_ERRORS 0
/*int log( const char* file, int line, const char* function,const char* string ) {
	printf( "[ %s @ %4i ] <LOG> %s - %s\n", file, line, function, string );
}*/
int error_throw( const char* file, int line, const char* function,const char* string );

#if LOG_ERRORS
	#define stak_error_throw( x ) error_throw( __FILE__, __LINE__, __FUNCTION__, x )
#else
	#define stak_error_throw( x ) -1
#endif

#define stak_log( x, ... ) printf("\33[36m[\33[35m %32s \33[36;1m@\33[0;33m %4i \33[36m] \33[0;34m %64s\33[32m  LOG \33[0;39m " x "\n",  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__ )
#define stak_error_none( ) 0
#define null_ptr() 0

#endif