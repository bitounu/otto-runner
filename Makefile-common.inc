CFLAGS+= -Wall -fPIC -I./src
ifdef PREFIX
	CFLAGS+=--sysroot=$(PREFIX) -I$(PREFIX)/include
endif
LDFLAGS+=-lpthread \
		 -lc \
		 -ldl \
		 -lrt \
		 -lm
INCLUDES+=-I`pwd`


HEADER="\33[35m----------[\33[36;1m Stak: \33[0;33m$(BIN) \33[35m]----------\33[39m"
FAILURE="\33[36;1mStak \33[37m➜ \33[31mBuild Failed!\33[0;39m"
BUILDING="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m$<...\33[39m"
COMPLETE="\33[36mStak \33[37m➜ \33[32mBuild Complete!\33[39m"
BUILDING_DL="\33[36;1mStak \33[0;37m➜ \33[32mBuilding \33[34m[ \33[35m${notdir $@}\33[34m ]\33[39m"

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip