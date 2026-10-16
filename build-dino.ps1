# SPDX-License-Identifier: GPL-3.0-only
[CmdletBinding()]
param(
    [string]$Distro = "Ubuntu",
    [switch]$Test
)

$ErrorActionPreference = "Stop"
Push-Location $PSScriptRoot
try {
    & wsl.exe -d $Distro -u root -- sh tools/build_dino.sh
    if ($LASTEXITCODE -ne 0) { throw "La compilación DINO falló; revisa los requisitos de DINO.md." }
    if ($Test) {
        & wsl.exe -d $Distro -u root -- bash -lc "sh tests/run_dino_tests.sh"
        if ($LASTEXITCODE -ne 0) { throw "Las pruebas DINO fallaron." }
    }
    Write-Output "Firmware: $PSScriptRoot/build/felucca.fwsc"
} finally {
    Pop-Location
}
