# clinfo

_query OpenCL platform and device parameters_


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

Just type: `clinfo`

`clinfo` displays the OpenCL platform and device parameters in a human readable form.

By default, clinfo only displays a few basic parameters.
You can print them all with `-a` (or `--all`).
You can also query specific parameters by listing them as arguments.
For instance use `clinfo CL_DEVICE_TYPE` to query the device type of each device for each platform.

You can restrict the output to a single platform and even to a single device.
For instance use `clinfo 0:1` to restrict the output to device #1 of platform #0.
To find out what is the number of a platform or a device, you can use `-l` (or `--list`).

The following options are available:

* `-a` or `--all `: Display all parameters.
* `-h` or `--help`: Display a short help notice.
* `-l` or `--list`: List platforms and devices.
* `-r` or `--raw `: Raw output (by default the values are pretty-printed).

## License

The source for clinfo is released under the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.
