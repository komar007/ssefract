{
  description = "ssefract — fractal renderer and SDL browser (i686, SDL 1.2)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      # 32-bit package set — provides i686 glibc, SDL, SDL_ttf, and a
      # multilib-capable stdenv whose gcc already targets i686 by default.
      pkgs_i686 = pkgs.pkgsi686Linux;
    in
    {
      # ── Build ─────────────────────────────────────────────────────
      packages.${system} = {
        default = self.packages.${system}.ssefract;

        ssefract = pkgs_i686.stdenv.mkDerivation {
          pname = "ssefract";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [ pkgs.nasm ];
          buildInputs = with pkgs_i686; [ SDL SDL_ttf ];

          # The i686 stdenv's gcc already targets i386, so drop the
          # Makefile's hardcoded -m32 to avoid a redundant flag
          # (-m32 is harmless but cleaner without).
          buildPhase = ''
            runHook preBuild

            # The stdenv adds -Werror=implicit-function-declaration and
            # -Werror=incompatible-pointer-types.  The original Makefile
            # has neither; relax back to warnings.
            NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types"

            # sdl12-compat puts SDL.h at include/SDL/SDL.h, but
            # SDL_ttf.h does #include "SDL.h" and the stdenv only
            # adds the include/ parent.  Fix: add the SDL/ subdir.
            # ASSETS_DIR tells the code where fonts + palettes live (.so are
            # always resolved relative to the executable via /proc/self/exe).
            SDATA="$out/share/ssefract"
            CFLAGS="$NIX_CFLAGS_COMPILE -DASSETS_DIR=\"$SDATA\" -I${pkgs_i686.SDL}/include/SDL"
            LDFLAGS="$NIX_LDFLAGS"
            # bare gcc chokes on -rpath; keep -L, drop the rest
            LDFLAGS=$(echo "$LDFLAGS" | sed 's/-rpath [^ ]*//g')
            # 32-bit fractal shared objects
            $CC $CFLAGS -std=c99 -fPIC -shared fractal.c -o fractal_c.so
            nasm -f elf fractal.asm
            # NASM doesn't emit .note.GNU-stack → kernel assumes execstack.
            # The asm is pure computation; tell ld the stack is not executable.
            ld -s -shared -soname fractal_asm.so -o fractal_asm.so fractal.o -melf_i386 -z noexecstack

            # compile objects
            for src in browser.c benchmark.c fractal_api.c io.c \
                       render.c threaded_generator.c gui.c; do
              $CC $CFLAGS -std=c99 -O2 -g -c "$src"
            done

            # link binaries
            $CC $LDFLAGS -std=c99 -o browser browser.o io.o fractal_api.o \
                threaded_generator.o gui.o \
                -lm -ldl -lpthread -lSDL -lSDL_ttf

            $CC $LDFLAGS -std=c99 -o render  render.o io.o fractal_api.o \
                threaded_generator.o -lm -ldl -lpthread

            $CC $LDFLAGS -std=c99 -o benchmark benchmark.o fractal_api.o \
                -lm -ldl

            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall
            mkdir -p "$out/bin" "$out/lib" "$out/share/ssefract"
            cp browser render benchmark "$out/bin/"
            # .so files next to exe (get_exe_dir resolves here)
            cp fractal_asm.so fractal_c.so "$out/bin/"
            cp fractal_asm.so fractal_c.so "$out/lib/"
            cp *.pal mono.ttf "$out/share/ssefract/"
            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "Fractal renderer (CLI + SDL browser), 32-bit";
            mainProgram = "browser";
            platforms = [ "x86_64-linux" "i686-linux" ];
            # FIXME: add a real license file to the repo, then replace this.
            license = licenses.free;
            longDescription = ''
              ssefract renders Mandelbrot / Julia fractals via hand-written
              assembly and C implementations loaded as shared objects.

              Targets built:
                browser   — SDL 1.2 GUI with palette cycling and zoom
                render    — CLI batch renderer (outputs BMP)
                benchmark — throughput measurement

              At runtime cd to $out/share/ssefract so the SDL browser
              can find mono.ttf and the *.pal colour-palette files.
            '';
          };
        };
      };

      # ── Dev shell ─────────────────────────────────────────────────
      devShells.${system}.default = pkgs.mkShell {
        name = "ssefract-dev";

        packages = with pkgs; [ nasm ];

        buildInputs = with pkgs_i686; [
          SDL
          SDL_ttf
          stdenv.cc     # i686 gcc so -m32 "just works"
        ];

        shellHook = ''
          echo "══ ssefract dev shell ══"
          echo "  arch:  i686 (32-bit)"
          echo "  build: make  or  gcc -std=c99 -m32 ..."
          echo ""
        '';
      };

      # ── Formatter (keep the flake tidy) ──────────────────────────
      formatter.${system} = pkgs.nixfmt-rfc-style;
    };
}