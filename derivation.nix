{ stdenv, pkgs, libusb1 }:

stdenv.mkDerivation {
  name = "lguf-backlight";
  pname = "lguf-backlight";

  buildInputs = [ libusb1 ];
  src = ./.;

  installPhase = ''
    mkdir -p $out/bin
    cp bin/lguf-backlight $out/bin/lguf-backlight
    chmod +x $out/bin/lguf-backlight
    mkdir -p $out/lib/udev/rules.d
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="043e", ATTR{idProduct}=="9a40", MODE="0666"' \
      > $out/lib/udev/rules.d/99-lguf-backlight.rules
  '';

  meta = with pkgs.lib; {
    homepage = "https://github.com/meck/lguf-backlight";
    description = "Backlight adjustment for LG UltraFine";
    platforms = platforms.linux;
  };

}

