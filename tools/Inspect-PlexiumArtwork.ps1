# Original read-only Windows artwork diagnostic. No FTP credentials or DB writes.
param(
 [Parameter(Mandatory=$true)][string]$PackageRoot,
 [string]$MountedMirror,
 [string]$StagedMirror,
 [string]$RegisteredMirror
)
$ErrorActionPreference = 'Stop'
$expectedTitle = 'PPSA85421'
$metadata = Get-Content -LiteralPath (Join-Path $PackageRoot 'sce_sys/param.json') -Raw | ConvertFrom-Json
if ($metadata.titleId -ne $expectedTitle) { throw 'Unexpected title identity; stop before touching the console.' }
$reference = Get-FileHash -LiteralPath (Join-Path $PackageRoot 'sce_sys/icon0.png') -Algorithm SHA256
$results = @([pscustomobject]@{Stage='Package';Present=$true;SHA256=$reference.Hash;MatchesPackage=$true})
foreach ($stage in @(@('Mounted',$MountedMirror),@('Staged',$StagedMirror),@('Registered',$RegisteredMirror))) {
 if (-not $stage[1]) { $results += [pscustomobject]@{Stage=$stage[0];Present='Not supplied';SHA256='';MatchesPackage='Unverified'};continue }
 $icon = Join-Path $stage[1] 'sce_sys/icon0.png'
 if (Test-Path -LiteralPath $icon) {
  $hash = Get-FileHash -LiteralPath $icon -Algorithm SHA256
  $results += [pscustomobject]@{Stage=$stage[0];Present=$true;SHA256=$hash.Hash;MatchesPackage=($hash.Hash -eq $reference.Hash)}
 } else { $results += [pscustomobject]@{Stage=$stage[0];Present=$false;SHA256='';MatchesPackage=$false} }
}
$results | Format-Table -AutoSize
Write-Output 'Read-only local mirror comparison. ShadowMount discovery, duplicate registrations, launcher references and actual shell cache remain console acceptance checks. No console state was changed.'
