# Script that synchronizes the local library dependencies
#
# Version: 20260621

Param (
	[switch]$UseHead = $false
)

$GitUrlPrefix = "https://github.com/libyal"
$LocalLibs = "libbfio libcaes libcdata libcerror libcfile libclocale libcnotify libcpath libcsplit libcthreads libfcache libfdata libfdatetime libfguid libfmos libhmac libuna"
$LocalLibs = ${LocalLibs} -split " "
$LockFile = "synclibs.lock"
$SyncLocks = @{}

$Git = "git"
$WinFlex = "..\win_flex_bison\win_flex.exe"
$WinBison = "..\win_flex_bison\win_bison.exe"

$Result = 0

If (-Not (${UseHead}))
{
	If (-Not (Test-Path -Path ${LockFile}))
	{
		Write-Warning "Missing synchronization lock file: ${LockFile}"

		Exit 1
	}
	ForEach (${Line} in Get-Content -Path ${LockFile})
	{
		${Line} = ${Line}.Trim()

		If (-Not (${Line}) -or ${Line}.StartsWith("#"))
		{
			Continue
		}
		${Fields} = ${Line} -split "\s+"

		If (${Fields}.Count -ne 3)
		{
			Write-Warning "Malformed synchronization lock: ${Line}"

			Exit 1
		}
		${SyncLocks}[${Fields}[0]] = @{
			Tag = ${Fields}[1]
			Commit = ${Fields}[2]
		}
	}
}

ForEach (${LocalLib} in ${LocalLibs})
{
	# Split will return an array of a single empty string when LocalLibs is empty.
	If (-Not (${LocalLib}))
	{
		Continue
	}
	$GitUrl = "${GitUrlPrefix}/${LocalLib}.git"

	# PowerShell will raise NativeCommandError if git writes to stdout or stderr
	# therefore 2>&1 is added and the output is stored in a variable.
	$Output = Invoke-Expression -Command "${Git} clone --depth 1 ${GitUrl} ${LocalLib}-${pid} 2>&1"

	If (-Not (Test-Path -Path ${LocalLib}-${pid}))
	{
		Write-Warning "Unable to download: ${LocalLib}"
		Write-Host ${Output}

		$Result = 1

		Continue
	}
	Push-Location "${LocalLib}-${pid}"

	Try
	{
		$Output = Invoke-Expression -Command "${Git} fetch --quiet --all --tags --prune 2>&1"

		If (-Not (${UseHead}))
		{
			${SyncLock} = ${SyncLocks}[${LocalLib}]

			If (-Not (${SyncLock}))
			{
				Write-Warning "Missing synchronization lock for: ${LocalLib}"

				$Result = 1

				Continue
			}
			Write-Host "Synchronizing: ${LocalLib} from ${GitUrl} tag $(${SyncLock}.Tag) commit $(${SyncLock}.Commit)"

			$Output = Invoke-Expression -Command "${Git} checkout --quiet $(${SyncLock}.Commit) 2>&1"
			${ResolvedCommit} = Invoke-Expression -Command "${Git} rev-parse HEAD 2>&1"
			${ResolvedTagCommit} = Invoke-Expression -Command "${Git} rev-parse refs/tags/$(${SyncLock}.Tag)^{commit} 2>&1"

			If (${ResolvedCommit} -ne ${SyncLock}.Commit -or ${ResolvedTagCommit} -ne ${SyncLock}.Commit)
			{
				Write-Warning "Synchronization lock mismatch for: ${LocalLib}"

				$Result = 1

				Continue
			}
		}
		Else
		{
			Write-Host "Synchronizing: ${LocalLib} from ${GitUrl} HEAD"
		}
	}
	Finally
	{
		Pop-Location
	}
	$LocalLibVersion = Get-Content -Path ${LocalLib}-${pid}\configure.ac |
		select -skip 4 -first 1 |
		% { $_ -Replace " \[","" } |
		% { $_ -Replace "\],","" }

	If (Test-Path ${LocalLib})
	{
		Remove-Item -Path ${LocalLib} -Force -Recurse
	}
	New-Item -ItemType directory -Path ${LocalLib} -Force | Out-Null

	If (Test-Path ${LocalLib})
	{
		Copy-Item -Path ${LocalLib}-${pid}\${LocalLib}\*.[chly] -Destination ${LocalLib}\

		Get-Content -Path ${LocalLib}-${pid}\${LocalLib}\${LocalLib}_definitions.h.in |
			% { $_ -Replace "@VERSION@",${LocalLibVersion} } > ${LocalLib}\${LocalLib}_definitions.h
	}
	Remove-Item -Path ${LocalLib}-${pid} -Force -Recurse

	$NamePrefix = ""

	ForEach (${DirectoryElement} in Get-ChildItem -Path "${LocalLib}\*.l")
	{
		$OutputFile = ${DirectoryElement} -Replace ".l$",".c"

		$NamePrefix = Split-Path -path ${DirectoryElement} -leaf
		$NamePrefix = ${NamePrefix} -Replace "^${LocalLib}_",""
		$NamePrefix = ${NamePrefix} -Replace ".l$","_"

		$WinFlexArguments = @(
			"-Cf",
			"${DirectoryElement}"
		)
		# PowerShell will raise NativeCommandError if win_flex writes to stdout or stderr
		# therefore 2>&1 is added and the output is stored in a variable.
		$Output = Invoke-Expression -Command "& '${WinFlex}' $($WinFlexArguments -join ' ') 2>&1" | %{ "$_" }
		Write-Host ${Output}

		# Moving manually since win_flex -o <filename> does not provide the expected behavior.
		Move-Item "lex.yy.c" ${OutputFile} -force
	}
	ForEach (${DirectoryElement} in Get-ChildItem -Path "${LocalLib}\*.y")
	{
		$OutputFile = ${DirectoryElement} -Replace ".y$",".c"

		$WinBisonArguments = @(
			"-d"
			"-v"
			"-l"
			"-p", "${NamePrefix}"
			"-o", "${OutputFile}"
			"${DirectoryElement}"
		)
		# PowerShell will raise NativeCommandError if win_bison writes to stdout or stderr
		# therefore 2>&1 is added and the output is stored in a variable.
		$Output = Invoke-Expression -Command "& '${WinBison}' $($WinBisonArguments -join ' ') 2>&1" | %{ "$_" }
		Write-Host ${Output}
	}
}

Exit ${Result}
