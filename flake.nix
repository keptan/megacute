{
  description = "c++23 buildenv";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux"];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override
          {
            # Override stdenv in order to change compiler:
             stdenv = pkgs.gcc14Stdenv;
          }
          {
            packages = with pkgs; [
	    	cmake
		gtkmm4
		libcpr
		cambalache
		pkg-config
		sqlite
            ];

	    shellHook = '' echo Welcome back commander :3 '';
          };
      });
    };
}
