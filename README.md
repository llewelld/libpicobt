# Libpicobt ReadMe

The Pico project is liberating humanity from passwords. See https://www.mypico.org.

Libpicobt provides an interface for Bluetooth that abstracts away the underlying
differences between Windows and Linux.

## Documentation

For more details on the libpicobt API and how to build the entire Pico stack, see the developer docs.

https://docs.mypico.org/developer/

## Install from source

You'll need to ensure you've installed the [build dependencies](https://docs.mypico.org/developer/libpicobt/#deps) before you attempt to compile and install libpicobt. If you're using Ubuntu 16.04, you can install all the build dependencies using `apt`.

```
sudo apt install openssh-client git libbluetooth-dev cmake dh-exec gcovr check pkg-config \
  doxygen graphviz
```

Assuming you've got all these, download the latest version from the git repository and move inside the project folder.

```
git clone git@github.com:mypico/libpicobt.git
cd libpicobt
```

You can now build using cmake with the following commands:

```
cmake .
make
```

After this, the cleanest way to install it is to build the deb or rpm packages and install these:

```
make package
sudo dpkg -i packages/libpicobt_0.0.1_amd64-run.deb
sudo dpkg -i packages/libpicobt_0.0.1_amd64-dev.deb
```

## License

Libpicobt is released under the AGPL licence. Read COPYING for information.

## Contributing

We welcome comments and contributions to the project. If you're interested in contributing please see here: https://get.mypico.org/cla/

## Contact and Links

More information can be found at: http://mypico.org

The Pico project team:
 * Frank Stajano (PI), Frank.Stajano@cl.cam.ac.uk
 * David Llewellyn-Jones, David.Llewellyn-Jones@cl.cam.ac.uk
 * Claudio Dettoni, cd611@cl.cam.ac.uk
 * Seb Aebischer, seb.aebischer@cl.cam.ac.uk
 * Kat Krol, kat.krol@cl.cam.ac.uk
 * David Harrison, David.Harrison@cl.cam.ac.uk

