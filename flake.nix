{
  description = "A simple bar, launcher, control center, and notification daemon.";

  inputs = {
    nixpkgs = {
      url = "github:nixos/nixpkgs/nixos-unstable";
    };
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        haskell = pkgs.haskellPackages.ghcWithPackages (p:
          with p; [
            haskell-language-server
            base
            bytestring
            containers
            optics
            ansi-terminal
            attoparsec
            cabal-doctest
            directory
            doctest
            filepath
            mtl
            pretty-show
            process
            regex-tdfa
            safe
            xdg-basedir
            hsc2hs
            xml-conduit
          ]);
        haskell-shell = pkgs.mkShell {
          name = "Haskell";
          packages = [
            haskell
            pkgs.gdb
            pkgs.pkg-config
            pkgs.cabal-install
            pkgs.meson
            pkgs.ninja
            pkgs.gobject-introspection
            pkgs.sassc
          ];
          nativeBuildInputs = with pkgs; [ glib pcre2 libsysprof-capture gtk4 expat xorg.libXdmcp util-linux libselinux libsepol lerc fribidi libthai libdatrie ];
          hardeningDisable = [ "fortify" ];
        };
        foobar = (pkgs.callPackage ./nix/package.nix {});
      in
      {
        packages = {
          inherit foobar;
          default = foobar;
        };

        devShells = {
          default = pkgs.mkShell {
            name = "foobar";
            packages = [
              pkgs.libnotify # for testing the notification daemon
              pkgs.clang-tools
              pkgs.gdb
            ];
            inputsFrom = [ foobar ];
            hardeningDisable = [ "fortify" ];
          };
          haskell = haskell-shell;
        };
      });
}
