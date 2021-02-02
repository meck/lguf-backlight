# LG UltraFine display backlight adjustment with nix overlay

Based on:
- https://github.com/velum/lguf-brightness

Redone in plain C with a argument based interface and a NixOS overlay. For use
in scripting, i.e. with a window manager. 

## Usage

### Single step

* `lguf-backlight inc`
* `lguf-backlight dec`

### Multiple steps
* `lguf-backlight inc 3`
* `lguf-backlight dec 8`


## Installation (NixOS)

Import the overlay something like this:

```nix
nixpkgs.overlays = [
(import (builtins.fetchTarball {
  url = "https://github.com/meck/lguf-backlight/archive/master.tar.gz"))
];

environment.systemPackages = [ pkgs.lguf-backlight ];
```

Add the program and the `udev` rule to access the monitor unprivileged:

```nix
services.udev.packages = [ pkgs.lguf-backlight ];
```

