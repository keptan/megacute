{
	description = "cmake and other libs we need";
	inputs =
	{
		nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
	};

	outputs = { self, nixpkgs, ...}: let
		system = "x86_64-linux";
		in
		{
			devShells."${system}".default = let
			pkgs = import nixpkgs 
			{
				inherit system;
			};
			in pkgs.mkShell
			{
				packages = with pkgs;
				[
					cmake
					gtkmm4
					jsoncpp
					libcpr
				];

				shellHook = '' echo Welcome Back Commander :3'';
			};
		};
}
