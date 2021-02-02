self: super: {
  lguf-backlight = super.pkgs.callPackage ./derivation.nix { pkgs = super.pkgs; };
}

