<# Installe Boost avec vcpkg, valide le programme et conserve les mesures en Release. #>
param(
    [ValidateSet('msvc', 'llvm')][string]$Compilateur = 'msvc',
    [string]$RacineVcpkg = $env:VCPKG_ROOT,
    [ValidateRange(1, 1000000000)][long]$Iterations = 5000000,
    [uint32]$Graine = 42
)
$ErrorActionPreference = 'Stop'
$racineProjet = Split-Path -Parent $PSScriptRoot
if (-not $RacineVcpkg) {
    $outilDetection = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (Test-Path -LiteralPath $outilDetection) {
        $installationVisual = & $outilDetection -latest -products '*' `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($installationVisual) { $RacineVcpkg = Join-Path $installationVisual 'VC/vcpkg' }
    }
}
if (-not $RacineVcpkg) { throw 'Fournir -RacineVcpkg ou definir VCPKG_ROOT' }
$fichierChaine = Join-Path $RacineVcpkg 'scripts/buildsystems/vcpkg.cmake'
if (-not (Test-Path -LiteralPath $fichierChaine)) { throw "Chaine vcpkg absente : $fichierChaine" }
$dossierCompilation = Join-Path $racineProjet "build-boost-$Compilateur"
$dossierPaquets = Join-Path $racineProjet 'build-comparaison-vcpkg/vcpkg_installed'
$null = New-Item -ItemType Directory -Path $dossierCompilation -Force
$optionsConfiguration = @(
    '-S', $racineProjet, '-B', $dossierCompilation,
    "-DCMAKE_TOOLCHAIN_FILE=$fichierChaine", "-DVCPKG_INSTALLED_DIR=$dossierPaquets",
    '-DVCPKG_TARGET_TRIPLET=x64-windows', '-DCMAKE_BUILD_TYPE=Release',
    '-DMINI_OPENMETHOD_COMPARAISON_BOOST=ON', '-DMINI_OPENMETHOD_REFLEXION=AUTO',
    '-DMINI_OPENMETHOD_TESTS=ON', '-DMINI_OPENMETHOD_EXEMPLES=ON'
)
if ($Compilateur -eq 'llvm') {
    $executableClang = Join-Path $env:ProgramFiles 'LLVM/bin/clang++.exe'
    if (-not (Test-Path -LiteralPath $executableClang)) { throw "Clang absent : $executableClang" }
    $optionsConfiguration += @('-G', 'Ninja', "-DCMAKE_CXX_COMPILER=$executableClang")
    $executableMesure = Join-Path $dossierCompilation 'comparer_boost.exe'
} else {
    $optionsConfiguration += @('-A', 'x64')
    $executableMesure = Join-Path $dossierCompilation 'Release/comparer_boost.exe'
}
cmake @optionsConfiguration 2>&1 | Tee-Object -FilePath (Join-Path $dossierCompilation 'configuration.log')
if ($LASTEXITCODE -ne 0) { throw 'Configuration de la comparaison impossible' }
cmake --build $dossierCompilation --config Release --parallel 2 2>&1 |
    Tee-Object -FilePath (Join-Path $dossierCompilation 'compilation.log')
if ($LASTEXITCODE -ne 0) { throw 'Compilation de la comparaison impossible' }
ctest --test-dir $dossierCompilation -C Release --output-on-failure --parallel 2 2>&1 |
    Tee-Object -FilePath (Join-Path $dossierCompilation 'tests.log')
if ($LASTEXITCODE -ne 0) { throw 'Echec des tests : ne pas exploiter les mesures' }
& $executableMesure $Iterations $Graine 2>&1 |
    Tee-Object -FilePath (Join-Path $dossierCompilation 'comparaison.txt')
if ($LASTEXITCODE -ne 0) { throw 'Echec de la comparaison' }
