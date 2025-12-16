{
  lib,
  stdenv,
  pkg-config,
  scdoc,
  installShellFiles,
  cairo,
  libxkbcommon,
  pango,
  wayland,
  wayland-scanner,
  wayland-protocols,
  xorg,
  # customization
  backend ? "both",
  patches ? [ ],
  # wks file support
  wksFile ? null,
  wksContent ? null,
  wksDirs ? null,
}:
with builtins;
with lib;
let
  # Backend validation and configuration
  validBackends = [
    "both"
    "x11"
    "wayland"
  ];
  backendValid = elem backend validBackends;

  backends = {
    hasX11 = backend == "both" || backend == "x11";
    hasWayland = backend == "both" || backend == "wayland";
  };

  # WKS configuration and validation
  wks = rec {
    # Input validation
    hasContent = wksContent != null;
    hasFile = wksFile != null;
    hasDirs = wksDirs != null && (isList wksDirs && length wksDirs > 0);

    # Validation flags
    contentConflicts = hasContent && hasFile;
    dirsRequiresEntryPoint = wksDirs != null && !(hasFile || hasContent);
    dirsValid = wksDirs == null || (isList wksDirs && length wksDirs > 0);

    # Determine which wks source to use
    fileToUse = if hasContent then builtins.toFile "key_chords.wks" wksContent else wksFile;

    # Whether we're using custom wks configuration
    isCustom = fileToUse != null || hasDirs;

    # Determine Make target based on backend and wks usage
    makeTarget =
      if !isCustom then
        (if backend == "both" then "all" else backend)
      else if backend == "both" then
        "from-wks"
      else
        "from-wks-${backend}";
  };

  # Helper functions for preBuild
  copyWksFile = file: "cp ${file} config/key_chords.wks";

  copyWksDirs =
    dirs:
    concatMapStringsSep "\n" (d: ''
      cp -r ${d} config/${baseNameOf d}
    '') dirs;

in
# Assertions
assert assertMsg backendValid
  "Invalid backend '${backend}'. Must be one of: ${concatStringsSep ", " validBackends}";
assert assertMsg (!wks.contentConflicts) "wksContent cannot be used with wksFile";
assert assertMsg (
  !wks.dirsRequiresEntryPoint
) "wksDirs requires either wksFile or wksContent to be specified";
assert assertMsg wks.dirsValid "wksDirs must be a non-empty list if provided";

stdenv.mkDerivation (finalAttrs: {
  pname = "wk";
  version = lib.trim (readFile ./VERSION);

  src =
    with fileset;
    toSource {
      root = ./.;
      fileset = unions [
        ./VERSION
        ./Makefile
        # Include all config files except generated key_chords.h
        (difference ./config (maybeMissing ./config/key_chords.h))
        ./completions
        ./man
        ./src
      ];
    };

  strictDeps = true;

  nativeBuildInputs = [
    pkg-config
    scdoc
    installShellFiles
  ]
  ++ optionals backends.hasWayland [
    wayland-scanner
  ];

  buildInputs = [
    cairo
    pango
  ]
  ++ optionals backends.hasWayland [
    # Wayland
    libxkbcommon
    wayland
    wayland-protocols
  ]
  ++ optionals backends.hasX11 [
    # X11
    xorg.libX11
    xorg.libXinerama
  ];

  inherit patches;

  makeFlags = [ "PREFIX=$(out)" ];

  # When using custom wks, copy it to config/
  preBuild = optionalString wks.isCustom (
    if wks.hasContent && wks.hasDirs then
      # Mode 1: wksContent as entry point + wksDirs for dependencies
      ''
        ${copyWksFile wks.fileToUse}
        ${copyWksDirs wksDirs}
      ''
    else if wks.hasContent then
      # Mode 2: Inline wks content only
      copyWksFile wks.fileToUse
    else if wks.hasFile && wks.hasDirs then
      # Mode 3: wksFile as entry point + wksDirs for dependencies
      ''
        ${copyWksFile wksFile}
        ${copyWksDirs wksDirs}
      ''
    else
      # Mode 4: Single wksFile only
      copyWksFile wksFile
  );

  buildPhase = ''
    runHook preBuild
    make ${wks.makeTarget}
    make man
    runHook postBuild
  '';

  passthru = {
    # Backend information
    inherit backend;
    inherit (backends) hasX11 hasWayland;

    # WKS configuration info
    isCustomWks = wks.isCustom;
  };

  meta = {
    homepage = "https://github.com/3L0C/wk";
    description = "Which-Key via X11 and Wayland";
    longDescription = ''
      wk displays available key chords in a popup window, inspired by
      emacs-which-key. It supports both X11 and Wayland backends with a
      custom scripting language (wks files) for defining key chord mappings.
      Features include nested key chord prefixes, hooks, chord arrays, and
      comprehensive preprocessor macros for customization.
    '';
    license = licenses.gpl3Plus;
    platforms = platforms.linux;
    mainProgram = "wk";
  };
})
