{
  lib,
  stdenv,
  pkg-config,
  scdoc,
  cairo,
  libxkbcommon,
  pango,
  wayland,
  wayland-scanner,
  wayland-protocols,
  xorg,
  # customization
  backend ? "both",
  patches ? [],
  # wks file support
  wksFile ? null,
  wksContent ? null,
}:
with builtins;
with lib; let
  version = lib.trim (readFile ./VERSION);

  # Validate backend parameter
  validBackends = ["both" "x11" "wayland"];
  backendValid = elem backend validBackends;

  # Validate wks inputs
  wksInputsValid = wksFile == null || wksContent == null;

  # Determine which wks source to use
  wksFileToUse =
    if wksContent != null
    then builtins.toFile "key_chords.wks" wksContent
    else wksFile;

  # Whether we're using custom wks
  usingCustomWks = wksFileToUse != null;

  # Determine Make target based on backend and wks usage
  makeTarget =
    if usingCustomWks
    then
      if backend == "both"
      then "from-wks"
      else if backend == "x11"
      then "from-wks-x11"
      else "from-wks-wayland"
    else
      if backend == "both"
      then "all"
      else backend;

  # Backend feature flags
  hasX11 = backend == "both" || backend == "x11";
  hasWayland = backend == "both" || backend == "wayland";

in
  assert assertMsg backendValid
    "Invalid backend '${backend}'. Must be one of: ${concatStringsSep ", " validBackends}";
  assert assertMsg wksInputsValid
    "Cannot specify both wksFile and wksContent - they are mutually exclusive";
  stdenv.mkDerivation (finalAttrs: {
    src = with fileset;
      toSource {
        root = ./.;
        fileset = unions [
          ./VERSION
          ./Makefile
          # Include all config files except generated key_chords.h
          (difference ./config (maybeMissing ./config/key_chords.h))
          ./man
          ./src
        ];
      };
    inherit version;
    pname = "wk";

    strictDeps = true;
    nativeBuildInputs = [
      pkg-config
      scdoc
    ] ++ optionals hasWayland [
      wayland-scanner
    ];

    buildInputs = [
      cairo
      pango
    ] ++ optionals hasWayland [
      # Wayland
      libxkbcommon
      wayland
      wayland-protocols
    ] ++ optionals hasX11 [
      # X11
      xorg.libX11
      xorg.libXinerama
    ];

    inherit patches;

    makeFlags = ["PREFIX=$(out)"];

    # When using custom wks, copy it to config/
    preBuild = optionalString usingCustomWks ''
      cp ${wksFileToUse} config/key_chords.wks
    '';

    buildPhase = ''
      runHook preBuild
      make ${makeTarget}
      runHook postBuild
    '';

    meta = {
      homepage = "https://github.com/3L0C/wk";
      description = "Which-Key via X11 and Wayland";
      license = licenses.gpl3Plus;
      platforms = platforms.linux;
      mainProgram = "wk";
    };
  })
