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
  wksFiles ? null,
}:
with builtins;
with lib; let
  version = lib.trim (readFile ./VERSION);

  # Validate backend parameter
  validBackends = ["both" "x11" "wayland"];
  backendValid = elem backend validBackends;

  # Validate wks inputs - wksContent is mutually exclusive with wksFile/wksFiles
  # but wksFile and wksFiles can work together
  wksContentConflicts = wksContent != null && (wksFile != null || wksFiles != null);
  wksInputsValid = !wksContentConflicts;

  # Validate wksFiles is a non-empty list if provided
  wksFilesValid = wksFiles == null || (isList wksFiles && length wksFiles > 0);

  # Determine operating modes
  hasWksFile = wksFile != null;
  hasWksFiles = wksFiles != null && wksFilesValid;
  hasWksContent = wksContent != null;

  # Base directory for relative path computation (when wksFile + wksFiles used together)
  baseDir = if hasWksFile then dirOf wksFile else null;

  # Helper function to compute relative path from baseDir to file
  getRelativePath = file: base:
    let
      fileStr = toString file;
      baseStr = toString base;
      # Ensure base ends with /
      baseDirStr = if lib.hasSuffix "/" baseStr then baseStr else baseStr + "/";
    in
      if lib.hasPrefix baseDirStr fileStr
      then lib.removePrefix baseDirStr fileStr
      else baseNameOf file;  # Fallback to basename if not under base

  # Generate a main.wks that includes all files from wksFiles (for backward compat mode)
  generatedMainWks =
    if hasWksFiles && !hasWksFile
    then builtins.toFile "main.wks" (
      concatMapStringsSep "\n"
        (f: ":include \"${baseNameOf f}\"")
        wksFiles
    )
    else null;

  # Determine which wks source to use
  wksFileToUse =
    if hasWksContent
    then builtins.toFile "key_chords.wks" wksContent
    else if hasWksFiles && !hasWksFile
    then generatedMainWks
    else wksFile;

  # Whether we're using custom wks
  usingCustomWks = wksFileToUse != null || (hasWksFile && hasWksFiles);

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
    "wksContent cannot be used with wksFile or wksFiles";
  assert assertMsg wksFilesValid
    "wksFiles must be a non-empty list if provided";
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
    preBuild = optionalString usingCustomWks (
      if hasWksContent then
        # Mode 1: Inline wks content
        ''
          cp ${wksFileToUse} config/key_chords.wks
        ''
      else if hasWksFile && hasWksFiles then
        # Mode 2: wksFile as entry point + wksFiles as dependencies with structure preservation
        ''
          # Copy main file as key_chords.wks
          cp ${wksFile} config/key_chords.wks

          # Copy dependencies preserving directory structure relative to main file's directory
          ${concatMapStringsSep "\n" (f:
            let relPath = getRelativePath f baseDir;
            in ''
              mkdir -p "config/$(dirname "${relPath}")"
              cp ${f} "config/${relPath}"
            '') wksFiles}
        ''
      else if hasWksFile then
        # Mode 3: Single wksFile only
        ''
          cp ${wksFile} config/key_chords.wks
        ''
      else
        # Mode 4: wksFiles only (backward compatibility - auto-generate main.wks)
        ''
          # Copy the generated main file
          cp ${generatedMainWks} config/key_chords.wks

          # Copy all referenced wks files flat to config/
          ${concatMapStringsSep "\n"
            (f: "cp ${f} config/${baseNameOf f}")
            wksFiles}
        ''
    );

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
