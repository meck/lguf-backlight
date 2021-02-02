{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  nativeBuildInputs = [ pkgs.bear ];
  inputsFrom = [ (pkgs.callPackage ./derivation.nix { inherit pkgs; }) ];
}
