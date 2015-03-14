# How to build

This project should be cross-compiled with Vagrant. See [otto-creator](https://github.com/NextThingCo/otto-creator) for details.

From the root directory run `make`

```
make clean
CC=arm-stak-linux-gnueabihf-gcc CXX=arm-stak-linux-gnueabihf-g++ make
```


# Running

Run the `main` with the menu and a mode:

```
sudo /stak/sdk/otto-sdk/build/main \
	/stak/sdk/otto-menu/build/libotto_menu.so \
	/stak/sdk/<some-mode>/build/<mode_file_name>.so
```

Note that `otto-menu`, `otto-sdk` and `otto-gif-mode` must be located at `/stak/sdk` on your Pi.
