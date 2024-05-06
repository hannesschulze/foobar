{
  stdenv,
  lib,
  fetchFromGitHub,
  fetchFromGitLab,
  makeWrapper,
  git,
  meson,
  ninja,
  vala,
  sassc,
  pkg-config,
  gobject-introspection,
  wayland-scanner,
  glib,
  gtk4,
  json-glib,
  librsvg,
  networkmanager,
  wayland,
  libpulseaudio,
  alsa-lib,
  upower,
  brightnessctl
}:
  let
    dep-gtk4-layer-shell = fetchFromGitHub {
      owner = "wmww";
      repo = "gtk4-layer-shell";
      rev = "v1.0.2";
      hash = "sha256-decjPkFkYy7kIjyozsB7BEmw33wzq1EQyIBrxO36984=";
    };
    dep-gvc = fetchFromGitLab {
      domain = "gitlab.gnome.org";
      owner = "GNOME";
      repo = "libgnome-volume-control";
      rev = "91f3f41490666a526ed78af744507d7ee1134323";
      hash = "sha256-lpDWVlRFngMSNfACrfJ5vRTZ2xdlwcrh4/YGcNDogys=";
    };
  in stdenv.mkDerivation {
    pname = "foobar";
    version = "1.0.0";
    src = ./..;
    postUnpack = ''
      pushd "$sourceRoot"
      cp -R --no-preserve=mode,ownership ${dep-gtk4-layer-shell} subprojects/gtk4-layer-shell
      cp -R --no-preserve=mode,ownership ${dep-gvc} subprojects/gvc
      popd
    '';
    # needed to enable support for SVG icons in GTK
    postInstall = ''
      wrapProgram "$out/bin/foobar" \
        --set GDK_PIXBUF_MODULE_FILE ${librsvg.out}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
    '';

    nativeBuildInputs = [ makeWrapper git meson ninja vala sassc pkg-config gobject-introspection wayland-scanner ];
    buildInputs = [ glib gtk4 json-glib librsvg networkmanager wayland libpulseaudio alsa-lib upower brightnessctl ];

    meta = with lib; {
      homepage = "https://github.com/hannesschulze/foobar";
      description = "A simple bar, launcher, control center, and notification daemon.";
      maintainers = [
        { name = "Hannes Schulze"; }
      ];
      license = licenses.mit;
      mainProgram = "foobar";
    };
  }
