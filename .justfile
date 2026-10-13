build_dir := "build"
config := "Debug"
llvm := "llvm@23"

build:
    cmake --build {{build_dir}} --config {{config}}

configure:
    cmake -B {{build_dir}} -G "Ninja Multi-Config" -DCMAKE_PREFIX_PATH="$(brew --prefix {{llvm}})"

test: build
    ctest --test-dir {{build_dir}} --build-config {{config}} -j8 --output-on-failure

package: build
    #!/usr/bin/env bash
    set -euo pipefail
    llvm_dir=$(sed -n 's|^LLVM_DIR:PATH=||p' "{{build_dir}}/CMakeCache.txt")
    major=$("$llvm_dir/../../../bin/llvm-config" --version | cut -d. -f1)
    archive="{{build_dir}}/package/JuceTidy-clang-tidy-$major-macos-$(uname -m).zip"
    rm -rf "{{build_dir}}/package"
    mkdir -p "{{build_dir}}/package"
    zip -j "$archive" "{{build_dir}}/{{config}}/JuceTidy.so" LICENSE

check name: build
    #!/usr/bin/env bash
    set -euo pipefail
    file="test/checks/{{name}}.cpp"
    [[ -f "$file" ]] || file="test/checks/{{name}}.hpp"
    check="{{replace_regex(name, '\..*$', '')}}"
    options=$(sed -nE "s|^// *OPTION: *([A-Za-z]+)=(.*)\$|'$check.\1': '\2'|p" "$file" | paste -sd, -)
    std=$(sed -nE 's|^// *STD: *([^ ]+).*$|\1|p' "$file")
    clang-tidy \
        --load="{{build_dir}}/{{config}}/JuceTidy.so" \
        --config="{Checks: '-*,$check', CheckOptions: {$options}}" \
        --quiet \
        "$file" \
        -- \
        -std="${std:-c++20}" \
        -isystem test/include \
        {{ if os() == "macos" { "-isysroot \"$(xcrun --show-sdk-path)\"" } else { "" } }}

release part="patch":
    #!/usr/bin/env bash
    set -euo pipefail
    current=$(sed -nE 's|^ *VERSION ([0-9]+\.[0-9]+\.[0-9]+)$|\1|p' CMakeLists.txt)
    IFS=. read -r major minor patch <<< "$current"
    case "{{part}}" in
        major) version="$((major + 1)).0.0" ;;
        minor) version="$major.$((minor + 1)).0" ;;
        patch) version="$major.$minor.$((patch + 1))" ;;
        *) echo "Unknown version part '{{part}}', expected major, minor, or patch" >&2; exit 1 ;;
    esac
    sed -i.bak -E "s|^( *VERSION )$current\$|\1$version|" CMakeLists.txt
    rm CMakeLists.txt.bak
    git commit -m "Bump version to $version" CMakeLists.txt
    git tag "v$version"
    git push --atomic origin HEAD "v$version"
