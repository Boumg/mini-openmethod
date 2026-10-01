<# Construit la chaine GCC 16.2 et conserve les compilations, tests et mesures dans build-gcc. #>
$ErrorActionPreference = 'Stop'
$racineProjet = Split-Path -Parent $PSScriptRoot
$dossierResultats = Join-Path $racineProjet 'build-gcc'
$null = New-Item -ItemType Directory -Path $dossierResultats -Force
docker build -f (Join-Path $PSScriptRoot 'Dockerfile.gcc') -t mini-openmethod-gcc:16.2 $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Construction de la chaine GCC impossible' }
docker run --rm --mount "type=bind,source=$racineProjet,target=/sources,readonly" `
    --mount "type=bind,source=$dossierResultats,target=/resultats" `
    mini-openmethod-gcc:16.2 bash /sources/outils/verifier-gcc.sh
if ($LASTEXITCODE -ne 0) { throw 'La validation GCC a echoue ; consulter build-gcc' }
