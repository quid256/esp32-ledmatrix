{
  description = "PlatformIO Development Environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = {
    self,
    nixpkgs,
  }: let
    forAllSystems = function:
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
        "aarch64-linux"
      ] (system: function (import nixpkgs {inherit system;}));
  in {
    devShells = forAllSystems (
      pkgs:
        with pkgs; {
          default = mkShell {
            packages = [
              platformio
              avrdude
            ];
          };
        }
    );
  };
}
