#!/usr/bin/env bash
# Execute dans le conteneur ; les resultats sont conserves sur le montage /resultats.
set -euo pipefail
g++ --version
"$VCPKG_ROOT/vcpkg" install --triplet x64-linux --x-feature=tests \
    --x-manifest-root=/sources --x-install-root=/resultats/vcpkg_installed
options_vcpkg=(
    "-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    -DVCPKG_INSTALLED_DIR=/resultats/vcpkg_installed
    -DVCPKG_TARGET_TRIPLET=x64-linux -DVCPKG_MANIFEST_INSTALL=OFF
)
for configuration in cpp23:OFF cpp26:ON auto:AUTO; do
    nom=${configuration%%:*}
    mode=${configuration#*:}
    attendu=1
    if [ "$mode" = OFF ]; then attendu=0; fi
    dossier="/resultats/$nom"
    mkdir -p "$dossier"
    cmake --fresh -S /sources -B "$dossier" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        "${options_vcpkg[@]}" \
        -DMINI_OPENMETHOD_REFLEXION="$mode" -DMINI_OPENMETHOD_REFLEXION_ATTENDUE="$attendu" \
        -DMINI_OPENMETHOD_MESURES=ON 2>&1 | tee "$dossier/configuration.log"
    /usr/bin/time -f 'Compilation : %e s ; memoire maximale processus enfant : %M Ko' \
        -o "$dossier/compilation.txt" \
        cmake --build "$dossier" --clean-first --parallel 2 2>&1 | tee "$dossier/compilation.log"
    ctest --test-dir "$dossier" --output-on-failure --parallel 2 2>&1 | tee "$dossier/tests.log"
    cmake --install "$dossier" --prefix "$dossier/installation"
    cmake --fresh -S /sources/tests/installation -B "$dossier/consommateur" -G Ninja \
        "${options_vcpkg[@]}" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$dossier/installation" \
        -DMINI_OPENMETHOD_REFLEXION_ATTENDUE="$attendu"
    cmake --build "$dossier/consommateur"
    ctest --test-dir "$dossier/consommateur" --output-on-failure 2>&1 | tee "$dossier/installation.log"
    "$dossier/mesurer_dispatch" 5000000 42 | tee "$dossier/mesures.txt"
done
# Un compilateur capable de reflexion doit aussi respecter le choix explicite C++23.
g++ -std=c++26 -freflection -DMINI_OPENMETHOD_REFLEXION=0 -DMINI_OPENMETHOD_REFLEXION_ATTENDUE=0 \
    -I/sources/include -isystem /resultats/vcpkg_installed/x64-linux/include \
    /sources/tests/test_detection.cpp -o /resultats/detection-forcee
/resultats/detection-forcee
