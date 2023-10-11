OUT_PATH         = x64/Debug/ewu_oct_2023_gamejam.exe
OUT_PATH_RELEASE = x64/Release/ewu_oct_2023_gamejam.exe

.PHONY: all
all:
	@make build
	@make run

.PHONY: build
build:
	@msbuild.exe -m -noLogo -p:Configuration=Debug

.PHONY: run
run:
	@(cd ewu_oct_2023_gamejam && ../$(OUT_PATH))

.PHONY: release
release:
	@make build_release
	@make run_release

.PHONY: build_release
build_release:
	@msbuild.exe -m -noLogo -p:Configuration=Release

.PHONY: run_release
run_release:
	@(cd ewu_oct_2023_gamejam && ../$(OUT_PATH_RELEASE))

.PHONY: clean
clean:
	@msbuild.exe -m -noLogo -t:Clean

.PHONY: clean_release
clean_release:
	@msbuild.exe -m -noLogo -t:Clean -p:Configuration=Release

