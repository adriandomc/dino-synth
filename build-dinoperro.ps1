# SPDX-License-Identifier: GPL-3.0-only
[CmdletBinding()]
param(
    [string]$Distro = "Ubuntu",
    [switch]$Test
)

$ErrorActionPreference = "Stop"
Push-Location $PSScriptRoot
try {
    & wsl.exe -d $Distro -u root -- sh tools/build_dinoperro.sh
    if ($LASTEXITCODE -ne 0) { throw "Falló la compilación de SLOOP 2.8.3 + Dinoperro." }
    if ($Test) {
        & wsl.exe -d $Distro -u root -- sh tests/run_dinoperro_tests.sh
        if ($LASTEXITCODE -ne 0) { throw "Fallaron las pruebas de SLOOP 2.8.3 + Dinoperro." }
    }
    Write-Output "Firmware: $PSScriptRoot/build/felucca.fwsc"
} finally {
    Pop-Location
}
