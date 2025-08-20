{ lib,
  stdenv,
  pkg-config,
  cairo,
  libxkbcommon,
  pango,
  wayland,
  wayland-scanner,
  wayland-protocols,
  xorg,
# customization
  patches ? [ ]
}:

with builtins;
with lib;

let
  version = readFile ./VERSION;
in stdenv.mkDerivation (finalAttrs: {
  src = with fileset; toSource {
    root = ./.;
    fileset = unions [
      ./VERSION
      ./Makefile
      ./config
      ./man
      ./src
      ./clean_man_files.sh
    ];
  };
  inherit version;
  pname = "wk";

  strictDeps = true;
  nativeBuildInputs = [
    pkg-config
    wayland-scanner
  ];

  buildInputs = [
    cairo
    pango
    # Wayland
    libxkbcommon
    wayland
    wayland-protocols
    # X11
    xorg.libX11
    xorg.libXinerama
    xorg.libXft
    xorg.libXdmcp
    xorg.libpthreadstubs
    xorg.libxcb
  ];

  inherit patches;

  makeFlags = [ "PREFIX=$(out)" ];

  meta = {
    homepage = "https://github.com/3L0C/wk";
    description = "Which-Key via X11 and Wayland";
    license = licenses.gpl3Plus;
    platforms = with platforms; linux;
    mainProgram = "wk";
  };
})
