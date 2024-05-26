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
            ];
            inputsFrom = [ foobar ];
          };
        };
      });
}
