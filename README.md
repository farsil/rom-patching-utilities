# ROM patching utilities

This repository contains a set of command-line utilities that assist with
patching ROM files:

- *behead*: removes header from ROMs (first 512 bytes).
- *bpspatch*: applies a BPS patch to a headerless ROM.
- *ipspatch*: applies an IPS patch to a headered or headerless ROM.

All commands can read input from stdin and can write to stdout so that they 
are easy to chain:

```sh
behead -i headered.smc | bpspatch patch.bps -o patched.smc
```

# Build

To build all utilities execute:

```sh
make
```

Any system with GCC should be able to compile. Individual utilities may be
compiled by providing an appropriate target to `make` (for example, `make
bpspatch`).

# Install

To install the utilities execute:

```sh
make install
```

You can change the destination directory by specifying `DESTDIR` (for example:
`make install DESTDIR=/path/to/dir`).

You can uninstall the files by invoking:

```sh
make uninstall
```

# Usage

All utilities accept a `-h` argument that will display the usage string.

If BPS patching fails due to CRC mismatch, try executing `behead` first.

IPS patching has no CRC check, you must know beforehand if the patch applies 
to a headered or headerless ROM.

