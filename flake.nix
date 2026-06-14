{
  description = "Nix Development shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages;
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          bash
          cmake
          gnumake
          libedit
          ncurses
          zlib
          zstd
          llvm.clang
          llvm.llvm
        ];

        hardeningDisable = [ "fortify" ];

        LLVM_DIR = "${llvm.llvm.dev}/lib/cmake/llvm";
      };
    };
}
