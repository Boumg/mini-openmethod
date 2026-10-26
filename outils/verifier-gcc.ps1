<# Valide avec GCC 16.2 ; -Mesurer active aussi les chronometrages dans build-gcc. #>
[CmdletBinding()]
param([switch]$Mesurer)

$ErrorActionPreference = 'Stop'
$racineProjet = Split-Path -Parent $PSScriptRoot
$manifeste = Get-Content -Raw -LiteralPath (Join-Path $racineProjet 'vcpkg.json') | ConvertFrom-Json
$revisionVcpkg = $manifeste.'builtin-baseline'
if ($revisionVcpkg -notmatch '^[0-9a-fA-F]{40}$') { throw 'Revision vcpkg invalide dans le manifeste' }
$dossierResultats = Join-Path $racineProjet 'build-gcc'
$null = New-Item -ItemType Directory -Path $dossierResultats -Force
docker build -f (Join-Path $PSScriptRoot 'Dockerfile.gcc') --build-arg "VCPKG_REVISION=$revisionVcpkg" `
    -t mini-openmethod-gcc:16.2 $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Construction de la chaine GCC impossible' }
$optionsValidation = @()
if ($Mesurer) { $optionsValidation += '--mesurer' }
docker run --rm --mount "type=bind,source=$racineProjet,target=/sources,readonly" `
    --mount "type=bind,source=$dossierResultats,target=/resultats" `
    mini-openmethod-gcc:16.2 bash /sources/outils/verifier-gcc.sh @optionsValidation
if ($LASTEXITCODE -ne 0) { throw 'La validation GCC a echoue ; consulter build-gcc' }
