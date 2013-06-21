# clinfo

Dumps all the OpenCL 1.1 variables from each device of each platform.


## Build

`clinfo` is made from a single C source file. To build:

	make

To install:

	make install

To uninstall:

	make uninstall

You can change the installation prefix by setting `prefix` and `bindir`.
The default location is `/usr/local/bin`. If you would rather install in your
home directory (`$(HOME)/bin`), you can do:

	make install prefix=$(HOME)


## Run

	clinfo


## License

The source for clinfo is released under the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.
