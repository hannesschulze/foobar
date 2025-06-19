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
          with p; [ haskell-language-server ]);
        haskell-shell = pkgs.mkShell {
          name = "Haskell";
          packages = [ haskell pkgs.gdb pkgs.pkg-config pkgs.cabal-install ];
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
