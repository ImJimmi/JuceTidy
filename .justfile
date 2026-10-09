build_dir := "build"
config := "Debug"

build:
    cmake --build {{build_dir}} --config {{config}}

test: build
    ctest --test-dir {{build_dir}} --build-config {{config}} -j8 --output-on-failure

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
