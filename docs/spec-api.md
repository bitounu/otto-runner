
libjpeg
bcm2835
+ rpiuserland
		 -lGLESv2 
		 -lEGL 
		 -lopenmaxil 
		 -lbcm_host 
		 -lvcos 
		 -lvchiq_arm 
		 -lkhrn_client 
		 -lvchostif 
		 -lvcilcs 
		 -lvcfiled_check 




# Stak API Specification
This document defines the specification for the stak api design. It is requested that all public facing stak apis adhere to the standards defined in this document. Any discussion on api design change discussions belong in a github issue.

## Common function processes
### Naming scheme
```cpp
// call function the_thing at interval in milliseconds
int error = stak_subsystem_do_the_thing(subsystem, the_thing, interval);
```
Functions should clearly explain their purpose through the use of their name and arguments. Very little documentation should be required for understanding the execution flow and usage of a function.

- Function and argument names should be all lowercase.
- Function names should begin with the name of the library they belong to, which in this case is _stak_.
- Following the parent library name is the specific subsystem, _subsystem_, prefixed with an underscore.
- The rest of the function name should explain the specific nature of the function, with all words separated by underscores for readability.


### Subsystem initialization
```cpp
subsys_state_s* subsystem = stak_subsystem_initialize(starting_time);
```
Here, the subsystem is initialized. A pointer to a new structure is returned containing state information for that specific subsystem. This should return a null pointer if any error has occurred during initialization.

### Subsystem shutdown
```
int error = stak_subsystem_shutdown(subsystem);
```
Here, the subsystem state instance should be the argument _shutdown_.


### Modification
```cpp
int error = stak_subsystem_update(subsystem, current_time);
```
Modification functions should take the state instance pointer as the first argument, and any following arguments may be defined by specific functionality needed. Built-in data types should be passed by value wherever possible. Structures should be passed by pointer.

### Error handling

```cpp
if(error) {
	// handle errors here
}
```
With the exception of initialization functions, all stak functions should return an integer representing if an error occurred during execution. If no error occurred, a function should return 0. Any other return value should represent an error having occurred in the function and actual error code values can be specific for each subsystem.


## Common verbiage reference.

- **initialize** - Initialize should be used when referring to setting up a high level subsystem.
- **shutdown** - Shutdown should be used when bringing down a high level subsystem.
- **open** - Open should be used when working with low level data, files, hardware.
- **close** - Open should be used when working with low level data, files, hardware.


int stak_timer_create()
int stak_timer_destroy()
int stak_timer_update()
int stak_timer_get_time()
int stak_timer_reset()
