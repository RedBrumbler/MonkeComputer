if (-not $env:qmodName) 
{
    $env:qmodName = "MonkeComputer"
}  
if (-not $env:module_id)
{
    $env:module_id = "monkecomputer"
}

$zip = $env:qmodName + ".zip"
$qmod = $env:qmodName + ".qmod"

$mod = "./mod.json"
$modJson = Get-Content $mod -Raw | ConvertFrom-Json

$allLibs = Get-ChildItem ./libs/arm64-v8a/*.so | Select -Expand Name
$libs = @()


$cover = "./" + $modJson.coverImage
$fileList = @($mod)

if ((-not ($cover -eq "./")) -and (Test-Path $cover))
{
    $fileList += ,$cover
}

$bannedLibList = @("modloader", "codegen", "gorilla-utils")

$modlib = @()

foreach ($lib in $allLibs) 
{
    # ignore modloader
    $contains = 0
    foreach ($ban in $bannedLibList)
    {
        if ($lib.Contains($ban))
        {
            $contains = 1
            break
        }
    }

    if ($contains)
    {
        continue
    }
    # if its the actual mod lib (for computer this is not used though)
    else
    {
        if ($lib.Contains($env:module_id))
        {
            $path = "./libs/arm64-v8a/" + $lib;
            $fileList += ,$path;
            $modlib += ,$lib;
            continue
        }
    }
    $path = "./libs/arm64-v8a/" + $lib
    $fileList += ,$path
    $libs += ,$lib
}

$extraFiles = Get-ChildItem ./ExtraFiles/* | Select -Expand Name

foreach ($file in $extraFiles)
{
    $path = "./ExtraFiles/" + $file
    $fileList += ,$path
} 

# update version from qpm json
$qpm = "./qpm.json"
$qpmJson = Get-Content $qpm | ConvertFrom-Json 
$modJson.version = $qpmJson.info.version

# add the thing to the libs list because we don't need it as a mod file
$libs += ,$modlib[0]

#echo "adding extra lib beatsaber hook to library"
#$libs += "libbeatsaber-hook_2_2_4.so"
#$fileList += "./extern/libbeatsaber-hook_2_2_4.so"

#$modJson.modFiles = $modlib
$modJson.libraryFiles = $libs
$modText = $modJson | ConvertTo-Json -Depth 50 -EscapeHandling EscapeNonAscii

Set-Content $mod $modText

# if the qmod exists, rename it to zip to update it, we'll rename it back later
if (Test-Path $qmod) 
{
    move-item -Force $qmod $zip
}

Compress-Archive -Path $fileList -DestinationPath $zip -Update

& move-item -Force $zip $qmod
